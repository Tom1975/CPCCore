#include "gtest/gtest.h"

#include "TestUtils.h"
#include "MachineState.h"
#include "Motherboard.h"

#include <string>
#include <vector>

// Expansion RAM: 64 KB, 128 KB and 576 KB machines, and the ways a state can be
// applied to the wrong one.
//
// The .SNA carries the base bank and at most one expansion page, so everything
// above 128 KB used to vanish. These use the configurations Sugarbox ships --
// CPC464FR for 64 KB, CPC6128FR for 128 KB, CPC6128SymbOs for 576 KB -- rather
// than fixtures invented for the test, so what is covered is what a user runs.
//
// Every page is written through the bus, with the machine paging it in the way
// a program would, and with a different pattern per page. A test that only
// checked "some data came back" would pass while pages were swapped.

namespace
{

EmulatorEngine* BootWithConfig(DirectoriesImp& dirImp, CDisplay& display, Log& log,
                               SoundFactory& soundFactory, ConfigurationManager& conf_manager,
                               const char* cfg)
{
   EmulatorEngine* machine = new EmulatorEngine();
   display.Init(false);
   display.Show(false);
   machine->SetDirectories(&dirImp);
   machine->SetLog(&log);
   machine->SetConfigurationManager(&conf_manager);
   machine->Init(&display, &soundFactory);
   machine->GetMem()->Initialisation();
   machine->LoadConfiguration(cfg);
   machine->Reinit();
   machine->SetFixedSpeed(true);
   machine->SetSpeedLimit(EmulatorEngine::E_FULL);
   for (int i = 0; i < 20; ++i)
      machine->RunTimeSlice();
   return machine;
}

int AvailablePages(EmulatorEngine* machine)
{
   const bool* available = machine->GetMem()->GetAvailableRam();
   int n = 0;
   for (int i = 0; i < 8; ++i) if (available[i]) ++n;
   return n;
}

// Page `page` fully mapped over 0x0000-0xFFFF is RAM configuration 2; the byte
// at 0x4000 then lands in that page's second bank, away from the screen and the
// firmware's own variables.
const unsigned short kProbe = 0x4000;

void PoisonPages(EmulatorEngine* machine, unsigned char seed)
{
   const bool* available = machine->GetMem()->GetAvailableRam();
   for (int page = 0; page < 8; ++page)
   {
      if (!available[page]) continue;
      machine->GetMem()->ConnectBank((unsigned char)page, 0, 2);
      machine->GetMem()->Set(kProbe, (unsigned char)(seed + page));
   }
   machine->GetMem()->ConnectBank(0, 0, 0);
}

std::vector<int> ReadPages(EmulatorEngine* machine)
{
   const bool* available = machine->GetMem()->GetAvailableRam();
   std::vector<int> out;
   for (int page = 0; page < 8; ++page)
   {
      if (!available[page]) { out.push_back(-1); continue; }
      machine->GetMem()->ConnectBank((unsigned char)page, 0, 2);
      out.push_back(machine->GetMem()->Get(kProbe));
   }
   machine->GetMem()->ConnectBank(0, 0, 0);
   return out;
}

struct Machine
{
   const char* cfg;
   const char* label;
   int expected_pages;
};

const Machine k64K  = { "CONF/CPC464FR.cfg",     "64K",  0 };
const Machine k128K = { "CONF/CPC6128FR.cfg",    "128K", 1 };
const Machine k576K = { "CONF/CPC6128SymbOs.cfg","576K", 8 };

}  // namespace

TEST(MachineStateRam, CarriesEveryExpansionPage)
{
   const Machine machines[] = { k64K, k128K, k576K };

   for (size_t m = 0; m < sizeof(machines) / sizeof(machines[0]); ++m)
   {
      SCOPED_TRACE(machines[m].label);

      DirectoriesImp dirImp; CDisplay display; Log log;
      SoundFactory soundFactory; ConfigurationManager conf_manager;
      EmulatorEngine* machine =
         BootWithConfig(dirImp, display, log, soundFactory, conf_manager, machines[m].cfg);

      ASSERT_EQ(machines[m].expected_pages, AvailablePages(machine))
         << machines[m].cfg << " did not give the memory this case is about";

      // The base bank is written the same way whether or not the machine has
      // expansion pages, so the 64 KB case still proves a round trip.
      machine->GetMem()->ConnectBank(0, 0, 0);
      machine->GetMem()->Set(kProbe, 0x5A);

      PoisonPages(machine, 0xA0);
      const std::vector<int> written = ReadPages(machine);

      std::vector<unsigned char> state;
      ASSERT_TRUE(MachineState::Save(machine, state));

      // Overwrite everything, so a restore that does nothing cannot pass.
      machine->GetMem()->ConnectBank(0, 0, 0);
      machine->GetMem()->Set(kProbe, 0xC3);
      PoisonPages(machine, 0x10);
      if (machines[m].expected_pages > 0)
         ASSERT_NE(written, ReadPages(machine))
            << "the overwrite did not take, so this case would prove nothing";
      machine->GetMem()->ConnectBank(0, 0, 0);
      ASSERT_EQ(0xC3, machine->GetMem()->Get(kProbe))
         << "the base bank overwrite did not take";

      ASSERT_TRUE(MachineState::Load(machine, &state[0], state.size()));
      const std::vector<int> restored = ReadPages(machine);
      machine->GetMem()->ConnectBank(0, 0, 0);
      const int base_restored = machine->GetMem()->Get(kProbe);

      const size_t size = state.size();
      delete machine;

      EXPECT_EQ(written, restored) << "an expansion page did not come back";
      EXPECT_EQ(0x5A, base_restored) << "the base bank did not come back";
      fprintf(stderr, "  RAM %-5s %d pages, state %zu bytes (%.1f KB)\n",
         machines[m].label, machines[m].expected_pages, size, size / 1024.0);
   }
}

// The nasty cases: a state is only meaningful against a machine with the same
// memory. Applying one to the wrong machine would drop pages silently in one
// direction and leave stale ones behind in the other.
TEST(MachineStateRam, RefusesAStateFromAMachineWithDifferentMemory)
{
   struct Pair { const Machine* saver; const Machine* loader; };
   const Pair pairs[] = {
      { &k576K, &k128K }, { &k128K, &k576K },
      { &k576K, &k64K  }, { &k64K,  &k576K },
      { &k128K, &k64K  }, { &k64K,  &k128K },
   };

   for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]); ++i)
   {
      SCOPED_TRACE(std::string(pairs[i].saver->label) + " -> " + pairs[i].loader->label);

      std::vector<unsigned char> state;
      {
         DirectoriesImp dirImp; CDisplay display; Log log;
         SoundFactory soundFactory; ConfigurationManager conf_manager;
         EmulatorEngine* saver =
            BootWithConfig(dirImp, display, log, soundFactory, conf_manager, pairs[i].saver->cfg);
         ASSERT_EQ(pairs[i].saver->expected_pages, AvailablePages(saver));
         ASSERT_TRUE(MachineState::Save(saver, state));
         delete saver;
      }

      DirectoriesImp dirImp; CDisplay display; Log log;
      SoundFactory soundFactory; ConfigurationManager conf_manager;
      EmulatorEngine* loader =
         BootWithConfig(dirImp, display, log, soundFactory, conf_manager, pairs[i].loader->cfg);
      ASSERT_EQ(pairs[i].loader->expected_pages, AvailablePages(loader));

      const bool loaded = MachineState::Load(loader, &state[0], state.size());
      delete loader;

      EXPECT_FALSE(loaded)
         << pairs[i].saver->label << " state was applied to a " << pairs[i].loader->label
         << " machine instead of being refused";
   }
}

// A truncated or padded expansion chunk must be refused rather than read past
// its end or leave pages half written.
TEST(MachineStateRam, RefusesAMalformedExpansionChunk)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      BootWithConfig(dirImp, display, log, soundFactory, conf_manager, k576K.cfg);
   ASSERT_EQ(8, AvailablePages(machine));

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine, state));
   ASSERT_TRUE(MachineState::Load(machine, &state[0], state.size()))
      << "the unmodified state must load, or the cases below prove nothing";

   // Find the expansion chunk and corrupt its declared length.
   const unsigned int sna_len = state[8] | (state[9] << 8)
                              | (state[10] << 16) | ((unsigned int)state[11] << 24);
   size_t at = 12 + sna_len;
   size_t xram_at = 0;
   while (at + 8 <= state.size())
   {
      const unsigned int id = state[at] | (state[at+1] << 8)
                            | (state[at+2] << 16) | ((unsigned int)state[at+3] << 24);
      const unsigned int len = state[at+4] | (state[at+5] << 8)
                             | (state[at+6] << 16) | ((unsigned int)state[at+7] << 24);
      if (id == 0x5852414D) { xram_at = at; break; }
      at += 8 + len;
   }
   ASSERT_NE(0u, xram_at) << "no expansion chunk in a 576K state";

   std::vector<unsigned char> shortened = state;
   shortened[xram_at + 4] -= 1;                 // one byte less than written
   EXPECT_FALSE(MachineState::Load(machine, &shortened[0], shortened.size()));

   std::vector<unsigned char> wrong_mask = state;
   wrong_mask[xram_at + 8] = 0x03;              // claim two pages, carry eight
   EXPECT_FALSE(MachineState::Load(machine, &wrong_mask[0], wrong_mask.size()));

   std::vector<unsigned char> cut = state;
   cut.resize(xram_at + 12);                    // chunk header, no payload
   EXPECT_FALSE(MachineState::Load(machine, &cut[0], cut.size()));

   delete machine;
}

// A refused state must leave the machine untouched, not part restored.
//
// The chunks are applied one after another, so a refusal partway used to leave
// the .SNA and every earlier chunk already in place: the frontend reports a
// failed load and the user carries on with a machine that is neither where it
// was nor where the state wanted it.
TEST(MachineStateRam, ARefusedStateLeavesTheMachineAlone)
{
   std::vector<unsigned char> foreign;
   {
      DirectoriesImp dirImp; CDisplay display; Log log;
      SoundFactory soundFactory; ConfigurationManager conf_manager;
      EmulatorEngine* other =
         BootWithConfig(dirImp, display, log, soundFactory, conf_manager, k576K.cfg);
      ASSERT_EQ(8, AvailablePages(other));
      PoisonPages(other, 0x70);
      ASSERT_TRUE(MachineState::Save(other, foreign));
      delete other;
   }

   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      BootWithConfig(dirImp, display, log, soundFactory, conf_manager, k128K.cfg);
   ASSERT_EQ(1, AvailablePages(machine));

   machine->GetMem()->ConnectBank(0, 0, 0);
   machine->GetMem()->Set(kProbe, 0x3C);
   PoisonPages(machine, 0xE0);

   // Fingerprint the machine widely enough that a partial restore shows up.
   const std::vector<int> pages_before = ReadPages(machine);
   machine->GetMem()->ConnectBank(0, 0, 0);
   const int base_before = machine->GetMem()->Get(kProbe);
   const unsigned short pc_before = machine->GetProc()->pc_;
   const unsigned short sp_before = machine->GetProc()->sp_;
   const unsigned char hcc_before = machine->GetCRTC()->hcc_;
   const unsigned char vcc_before = machine->GetCRTC()->vcc_;

   ASSERT_FALSE(MachineState::Load(machine, &foreign[0], foreign.size()))
      << "a 576K state was accepted by a 128K machine";

   const std::vector<int> pages_after = ReadPages(machine);
   machine->GetMem()->ConnectBank(0, 0, 0);
   const int base_after = machine->GetMem()->Get(kProbe);
   const unsigned short pc_after = machine->GetProc()->pc_;
   const unsigned short sp_after = machine->GetProc()->sp_;
   const unsigned char hcc_after = machine->GetCRTC()->hcc_;
   const unsigned char vcc_after = machine->GetCRTC()->vcc_;

   delete machine;

   EXPECT_EQ(pages_before, pages_after)   << "RAM was modified by a refused load";
   EXPECT_EQ(base_before, base_after)     << "the base bank was modified by a refused load";
   EXPECT_EQ(pc_before, pc_after)         << "the CPU was moved by a refused load";
   EXPECT_EQ(sp_before, sp_after)         << "the CPU was moved by a refused load";
   EXPECT_EQ(hcc_before, hcc_after)       << "the CRTC was moved by a refused load";
   EXPECT_EQ(vcc_before, vcc_after)       << "the CRTC was moved by a refused load";
}
