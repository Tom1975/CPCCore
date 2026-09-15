#include "gtest/gtest.h"

#include "TestUtils.h"
#include "MachineState.h"
#include "Motherboard.h"
#include "DMA.h"
#include "PlayCity.h"

#include <cstring>
#include <vector>

// The Plus DMA channels and the PlayCity expansion.
//
// What this proves, and what it does not: the other MachineState tests run a
// whole machine and compare two runs, which is the strong form. Neither of
// these components can be driven that way here -- the unit tests never plug
// PlayCity in (the libretro wrapper does that, through sig->exp_list_), and
// reaching the DMA channels from emulated code needs a Plus program doing sound
// DMA. So these tests set the state directly, round trip it, and check it comes
// back. That catches a field left out of a chunk, which is the mistake actually
// being guarded against here; it does not prove the components then behave the
// same, and no claim is made that it does.

namespace
{

EmulatorEngine* Boot(DirectoriesImp& dirImp, CDisplay& display, Log& log,
                     SoundFactory& soundFactory, ConfigurationManager& conf_manager,
                     const char* section)
{
   EmulatorEngine* machine = new EmulatorEngine();
   display.Init(false);
   display.Show(false);
   machine->SetDirectories(&dirImp);
   machine->SetLog(&log);
   machine->SetConfigurationManager(&conf_manager);
   machine->Init(&display, &soundFactory);
   machine->GetMem()->Initialisation();
   machine->LoadConfiguration(section, "./TestConf.ini");
   machine->Reinit();
   machine->SetFixedSpeed(true);
   machine->SetSpeedLimit(EmulatorEngine::E_FULL);
   for (int i = 0; i < 100; ++i)
      machine->RunTimeSlice();
   return machine;
}

// Raw bytes of a component, so a field missing from a chunk shows up whether or
// not anyone remembered to name it.
std::vector<unsigned char> Bytes(const void* p, size_t n)
{
   const unsigned char* b = static_cast<const unsigned char*>(p);
   return std::vector<unsigned char>(b, b + n);
}

size_t CountDiffering(const std::vector<unsigned char>& a, const void* p)
{
   const unsigned char* b = static_cast<const unsigned char*>(p);
   size_t n = 0;
   for (size_t i = 0; i < a.size(); ++i)
      if (a[i] != b[i]) ++n;
   return n;
}

}  // namespace

TEST(MachineStateExpansions, CarriesTheDmaChannels)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = Boot(dirImp, display, log, soundFactory, conf_manager, "6128PLUS");

   Motherboard* board = machine->GetMotherboard();

   // A channel mid-program: the .SNA's CPC+ chunk has fields for only three of
   // these, so the rest is exactly what used to be lost.
   for (int i = 0; i < 3; ++i)
   {
      DMA* d = board->GetDMA(i);
      d->pause_counter_ = 100 + i;
      d->repeat_counter_ = 200 + i;
      d->repeat_addr_ = (unsigned short)(0x4000 + i);
      d->curent_instr_ = (unsigned short)(0x1234 + i);
      d->enable_next_ = true;
      d->ppr_ = (unsigned char)(0x10 + i);
      d->interrupt_on_ = true;
      d->prescalar_ = (unsigned char)(3 + i);
      d->prescalar_counter_ = (unsigned char)(1 + i);
   }

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine, state));

   std::vector<std::vector<unsigned char> > before;
   for (int i = 0; i < 3; ++i)
      before.push_back(Bytes(board->GetDMA(i), sizeof(DMA)));

   // Scribble over it, so a restore that does nothing cannot pass.
   for (int i = 0; i < 3; ++i)
   {
      DMA* d = board->GetDMA(i);
      d->pause_counter_ = 0; d->repeat_counter_ = 0; d->repeat_addr_ = 0;
      d->curent_instr_ = 0; d->enable_next_ = false; d->ppr_ = 0;
      d->interrupt_on_ = false; d->prescalar_ = 0; d->prescalar_counter_ = 0;
   }
   ASSERT_NE(0u, CountDiffering(before[0], board->GetDMA(0)))
      << "the scribble did not take, so a pass would prove nothing";

   ASSERT_TRUE(MachineState::Load(machine, &state[0], state.size()));

   size_t differing = 0;
   for (int i = 0; i < 3; ++i)
      differing += CountDiffering(before[i], board->GetDMA(i));

   delete machine;
   EXPECT_EQ(0u, differing) << "a DMA channel did not come back as it was saved";
}

TEST(MachineStateExpansions, CarriesPlayCity)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = Boot(dirImp, display, log, soundFactory, conf_manager, "6128");

   Motherboard* board = machine->GetMotherboard();
   PlayCity* pc = board->GetPlayCity();

   // Plug it in the way the libretro wrapper does, so the CTC and the two sound
   // chips are actually clocked while the machine runs.
   CSig* sig = machine->GetSig();
   sig->nb_expansion_ = 0;
   sig->exp_list_[sig->nb_expansion_++] = pc;

   // Drive it: select a channel on each chip and give the CTC a time constant.
   unsigned char data;
   data = 7;    pc->Out(0xF884, data);   // register select, right chip
   data = 0x3E; pc->Out(0xF984, data);   // mixer: tone A only
   data = 0;    pc->Out(0xF888, data);   // register select, left chip
   data = 0x80; pc->Out(0xF988, data);
   data = 0xA5; pc->Out(0xF880, data);   // CTC channel 0 control
   data = 0x40; pc->Out(0xF880, data);   // ... and its time constant

   for (int i = 0; i < 50; ++i)
      machine->RunTimeSlice();

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine, state));

   // After the save, not before: saving steps the machine to the next Z80
   // instruction boundary, and PlayCity is clocked while it does.
   const std::vector<unsigned char> before = Bytes(pc, sizeof(PlayCity));

   for (int i = 0; i < 50; ++i)
      machine->RunTimeSlice();
   ASSERT_NE(0u, CountDiffering(before, pc))
      << "PlayCity did not change while running, so this test would pass either way";

   ASSERT_TRUE(MachineState::Load(machine, &state[0], state.size()));

   const size_t differing = CountDiffering(before, pc);
   delete machine;

   EXPECT_EQ(0u, differing) << "PlayCity did not come back as it was saved";
}
