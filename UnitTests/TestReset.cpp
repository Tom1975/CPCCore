#include "gtest/gtest.h"

#include "TestUtils.h"
#include "Motherboard.h"

#include <string>
#include <vector>

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

// Power on is a known state, whatever the machine was doing.
//
// A program changes the CRTC registers, the screen mode, the inks, the memory
// configuration, the tape motor and, on a CPC+, the ASIC (sprites, split, palette),
// then the machine is switched off and on at some point of a line. A few seconds
// later the screen must be the one of a machine switched off and on at rest: the
// monitor, which powers the computer, restarts with it, and the colour latches of
// the Gate Array start from 0.

namespace
{

const int kScreenInts = 1024 * 624;   // CDisplay framebuffer (UnitTests/Display.cpp)

EmulatorEngine* BootConfig(DirectoriesImp& dirImp, CDisplay& display, Log& log,
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
   for (int i = 0; i < 100; ++i)
      machine->RunTimeSlice(true);
   return machine;
}

void Out(std::vector<unsigned char>& code, unsigned short port)
{
   code.insert(code.end(), { 0x01, (unsigned char)(port & 0xFF), (unsigned char)(port >> 8),   // LD BC,port
                             0xED, 0x49 });                                                    // OUT (C),C
}

std::vector<unsigned char> UpsetTheHardware(bool plus)
{
   std::vector<unsigned char> code = { 0xF3 };   // DI
   if (plus)
   {
      const unsigned char unlock[] = { 0xFF, 0x00, 0xFF, 0x77, 0xB3, 0x51, 0xA8, 0xD4, 0x62,
                                       0x39, 0x9C, 0x46, 0x2B, 0x15, 0x8A, 0xCD, 0xEE };
      for (unsigned char b : unlock) Out(code, 0xBC00 | b);
      Out(code, 0x7FB8);   // RMR2 : ASIC registers in 4000-7FFF
      // Sprite 0 position and magnification, split, soft scroll, palette, interrupt line
      const unsigned short address[] = { 0x6000, 0x6002, 0x6004, 0x6801, 0x6802, 0x6804, 0x6400, 0x6420, 0x6800 };
      const unsigned char value[]    = { 0x40,   0x40,   0x0F,   0x50,   0x30,   0x77,   0x0F,   0xF0,   0x60   };
      for (int i = 0; i < 9; ++i)
         code.insert(code.end(), { 0x3E, value[i],                                                       // LD A,n
                                   0x32, (unsigned char)(address[i] & 0xFF), (unsigned char)(address[i] >> 8) });  // LD (nn),A
      // Sprite 0 pixels
      code.insert(code.end(), { 0x21, 0x00, 0x40, 0x11, 0x01, 0x40, 0x36, 0x0F, 0x01, 0xFF, 0x00, 0xED, 0xB0 });
   }
   const unsigned char reg[] = { 0, 1, 2, 3, 4, 6, 7, 9, 12, 13, 8 };
   const unsigned char val[] = { 0x20, 0x10, 0x18, 0x05, 0x10, 0x05, 0x08, 0x03, 0x10, 0x55, 0x30 };
   for (int i = 0; i < 11; ++i) { Out(code, 0xBC00 | reg[i]); Out(code, 0xBD00 | val[i]); }
   Out(code, 0x7F00); Out(code, 0x7F4B);   // pen 0 : white
   Out(code, 0x7F10); Out(code, 0x7F4C);   // border : red
   Out(code, 0x7F8E);                      // mode 2, ROMs off
   Out(code, 0x7FC2);                      // RAM configuration 2
   Out(code, 0xF610);                      // tape motor on
   code.insert(code.end(), { 0x18, 0xFE }); // JR $
   return code;
}

std::vector<int> ScreenAfterOnOff(const char* cfg, bool upset, int ticks_before_onoff)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = BootConfig(dirImp, display, log, soundFactory, conf_manager, cfg);

   if (upset)
   {
      const std::vector<unsigned char> code = UpsetTheHardware(std::string(cfg).find("PLUS") != std::string::npos);
      for (size_t i = 0; i < code.size(); ++i)
         machine->GetMem()->Set((unsigned short)(0x8000 + i), code[i]);
      machine->GetProc()->PrepareForFetch(0x8000);
      for (int i = 0; i < 10; ++i) machine->RunTimeSlice(true);
   }
   if (ticks_before_onoff != 0)
      machine->GetMotherboard()->DebugNew(ticks_before_onoff);

   machine->OnOff();
   for (int i = 0; i < 250; ++i) machine->RunTimeSlice(true);

   const int* buffer = display.GetVideoBuffer(0);
   std::vector<int> screen(buffer, buffer + kScreenInts);
   delete machine;
   return screen;
}

}  // namespace

TEST(Reset, PowerOnGivesTheBootScreenWhateverTheMachineWasDoing)
{
   const char* configs[] = { "CONF/CPC6128FR.cfg", "CONF/CPC6128PLUSFR.cfg" };
   for (const char* cfg : configs)
   {
      SCOPED_TRACE(cfg);
      const std::vector<int> reference = ScreenAfterOnOff(cfg, false, 0);
      // 30011 T-states : the switch happens in the middle of a line, and of an instruction
      for (int ticks : { 0, 30011 })
         for (bool upset : { false, true })
         {
            if (!upset && ticks == 0) continue;
            SCOPED_TRACE(std::string(upset ? "upset" : "at rest") + ", ticks " + std::to_string(ticks));
            EXPECT_TRUE(reference == ScreenAfterOnOff(cfg, upset, ticks));
         }
   }
}
