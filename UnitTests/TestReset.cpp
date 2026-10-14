#include "gtest/gtest.h"

#include "TestUtils.h"
#include "Motherboard.h"

// Reset versus power on.
//
// The RESET line restarts the chips -- the Z80 fetches from 0x0000 again and the
// memory goes back to its initial configuration -- but the RAM is not touched:
// that is what the CSL "reset soft" asks for. Switching the machine off and on
// loses the RAM content.

namespace
{

EmulatorEngine* Boot128K(DirectoriesImp& dirImp, CDisplay& display, Log& log,
                         SoundFactory& soundFactory, ConfigurationManager& conf_manager)
{
   EmulatorEngine* machine = new EmulatorEngine();
   display.Init(false);
   display.Show(false);
   machine->SetDirectories(&dirImp);
   machine->SetLog(&log);
   machine->SetConfigurationManager(&conf_manager);
   machine->Init(&display, &soundFactory);
   machine->GetMem()->Initialisation();
   machine->LoadConfiguration("CONF/CPC6128FR.cfg");
   machine->Reinit();
   machine->SetFixedSpeed(true);
   machine->SetSpeedLimit(EmulatorEngine::E_FULL);
   for (int i = 0; i < 20; ++i)
      machine->RunTimeSlice();
   return machine;
}

// 0x4000 with RAM configuration 0 is the base bank 1; with configuration 2 it is
// the bank 1 of the expansion page.
const unsigned short kProbe = 0x4000;
const unsigned char kBase = 0x5A;
const unsigned char kExpansion = 0xA5;

// Writes both probes, and leaves the expansion page mapped and the Z80 elsewhere,
// so that a reset has something to put back.
void WriteProbes(EmulatorEngine* machine)
{
   machine->GetMem()->ConnectBank(0, 0, 0);
   machine->GetMem()->Set(kProbe, kBase);
   machine->GetMem()->ConnectBank(0, 0, 2);
   machine->GetMem()->Set(kProbe, kExpansion);
   machine->GetProc()->pc_ = 0x1234;
}

}  // namespace

TEST(Reset, SoftResetRestartsTheMachineAndKeepsTheRam)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = Boot128K(dirImp, display, log, soundFactory, conf_manager);

   WriteProbes(machine);
   machine->Reset();

   EXPECT_EQ(0x0000, machine->GetProc()->pc_);
   // Back to configuration 0 without being asked: the base bank shows.
   EXPECT_EQ(kBase, machine->GetMem()->Get(kProbe));
   machine->GetMem()->ConnectBank(0, 0, 2);
   EXPECT_EQ(kExpansion, machine->GetMem()->Get(kProbe));

   delete machine;
}

TEST(Reset, PowerOnClearsTheRam)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = Boot128K(dirImp, display, log, soundFactory, conf_manager);

   WriteProbes(machine);
   machine->OnOff();

   EXPECT_EQ(0x0000, machine->GetProc()->pc_);
   EXPECT_NE(kBase, machine->GetMem()->Get(kProbe));
   machine->GetMem()->ConnectBank(0, 0, 2);
   EXPECT_NE(kExpansion, machine->GetMem()->Get(kProbe));

   delete machine;
}

// TEMPORARY probe: is the screen after OnOff always the boot screen?
#include <cstdio>
#include <vector>
static unsigned long long HashScreen(CDisplay& display)
{
   unsigned long long h = 1469598103934665603ULL;
   for (int y = 0; y < display.GetHeight(); ++y)
   {
      int* line = display.GetVideoBuffer(y);
      if (line == nullptr) continue;
      for (int x = 0; x < display.GetWidth(); ++x) { h ^= (unsigned)line[x]; h *= 1099511628211ULL; }
   }
   return h;
}

static EmulatorEngine* BootCfg(DirectoriesImp& dirImp, CDisplay& display, Log& log,
                         SoundFactory& soundFactory, ConfigurationManager& conf_manager, const char* cfg)
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
   for (int i = 0; i < 100; ++i)
      machine->RunTimeSlice(true);
   return machine;
}

static void Out(std::vector<unsigned char>& c, unsigned short port)
{
   c.push_back(0x01); c.push_back(port & 0xFF); c.push_back(port >> 8);  // LD BC,port
   c.push_back(0xED); c.push_back(0x49);                                 // OUT (C),C
}

static std::vector<unsigned char> Perturb(bool plus)
{
   std::vector<unsigned char> c;
   c.push_back(0xF3); // DI
   if (plus)
   {
      const unsigned char seq[] = { 0xFF,0x00,0xFF,0x77,0xB3,0x51,0xA8,0xD4,0x62,0x39,0x9C,0x46,0x2B,0x15,0x8A,0xCD,0xEE };
      for (unsigned char b : seq) Out(c, 0xBC00 | b);
      Out(c, 0x7FB8);                                       // RMR2: ASIC registers at 4000
      const unsigned short addr[] = { 0x6000, 0x6002, 0x6004, 0x6801, 0x6802, 0x6804, 0x6400, 0x6420, 0x6800, 0x6C0F };
      const unsigned char  val[]  = { 0x40,   0x40,   0x0F,   0x50,   0x30,   0x77,   0x0F,   0xF0,   0x60,   0x07 };
      for (int i = 0; i < 10; ++i) { c.push_back(0x3E); c.push_back(val[i]); c.push_back(0x32); c.push_back(addr[i] & 0xFF); c.push_back(addr[i] >> 8); }
      // sprite 0 pixels
      c.insert(c.end(), { 0x21,0x00,0x40, 0x11,0x01,0x40, 0x36,0x0F, 0x01,0xFF,0x00, 0xED,0xB0 });
   }
   const unsigned char regs[] = { 0, 1, 2, 3, 4, 6, 7, 9, 12, 13, 8 };
   const unsigned char vals[] = { 0x20, 0x10, 0x18, 0x05, 0x10, 0x05, 0x08, 0x03, 0x10, 0x55, 0x30 };
   for (int i = 0; i < 11; ++i) { Out(c, 0xBC00 | regs[i]); Out(c, 0xBD00 | vals[i]); }
   Out(c, 0x7F00); Out(c, 0x7F4B);   // pen 0 white
   Out(c, 0x7F10); Out(c, 0x7F4C);   // border red
   Out(c, 0x7F8E);                   // mode 2, ROMs off
   Out(c, 0x7FC2);                   // RAM configuration 2
   Out(c, 0xF610);                   // tape motor on
   c.push_back(0x18); c.push_back(0xFE); // JR $
   return c;
}

TEST(Reset, ProbeOnOffScreen)
{
   const char* cfgs[] = { "CONF/CPC6128FR.cfg", "CONF/CPC464FR.cfg", "CONF/CPC6128FRCRTC0.cfg", "CONF/CPC6128FRCRTC1.cfg", "CONF/CPC6128PLUSFR.cfg" };
   const int extra_ticks[] = { 0, 3, 1023, 30011 };
   for (const char* cfg : cfgs)
   {
      const bool plus = std::string(cfg).find("PLUS") != std::string::npos;
      for (int perturb = 0; perturb < 2; ++perturb)
         for (int t : extra_ticks)
         {
            DirectoriesImp dirImp; CDisplay display; Log log;
            SoundFactory soundFactory; ConfigurationManager conf_manager;
            EmulatorEngine* machine = BootCfg(dirImp, display, log, soundFactory, conf_manager, cfg);
            unsigned long long before = 0;
            if (perturb)
            {
               std::vector<unsigned char> code = Perturb(plus);
               for (size_t i = 0; i < code.size(); ++i) machine->GetMem()->Set((unsigned short)(0x8000 + i), code[i]);
               machine->GetProc()->PrepareForFetch(0x8000);
               for (int i = 0; i < 10; ++i) machine->RunTimeSlice(true);
               before = HashScreen(display);
            }
            if (t) machine->GetMotherboard()->DebugNew(t);
            machine->OnOff();
            for (int i = 0; i < 250; ++i) machine->RunTimeSlice(true);
            printf("PROBE %-26s perturb=%d ticks=%5d before=%016llx after=%016llx\n", cfg, perturb, t, before, HashScreen(display));
            delete machine;
         }
   }
}
