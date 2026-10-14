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
