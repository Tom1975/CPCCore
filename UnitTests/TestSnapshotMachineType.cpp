#include "gtest/gtest.h"

#include "TestUtils.h"
#include "MachineSettings.h"

#include <vector>

// Byte 6D of a .SNA records which machine the snapshot was taken on, and the
// load side acts on it. These cover both directions, plus the one value the
// format and the other emulators disagree about.

namespace
{

EmulatorEngine* NewBootedMachine(DirectoriesImp& dirImp, CDisplay& display, Log& log,
                                 SoundFactory& soundFactory, ConfigurationManager& conf_manager,
                                 const char* section, int slices)
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

   for (int i = 0; i < slices; ++i)
      machine->RunTimeSlice();

   return machine;
}

}  // namespace

TEST(SnapshotMachineType, ClassicMachineIsRecordedAsSuch)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128", 20);

   ASSERT_EQ(MachineSettings::OLD_6128, machine->GetMachineType());
   ASSERT_FALSE(machine->IsPLUS());

   std::vector<unsigned char> saved;
   ASSERT_TRUE(machine->SaveSnapshotNow(saved));
   const unsigned char cpc_type = saved[0x6D];

   delete machine;

   EXPECT_EQ(2, cpc_type);
}

TEST(SnapshotMachineType, PlusMachineIsRecordedAsSuch)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128PLUS", 20);

   ASSERT_EQ(MachineSettings::PLUS_6128, machine->GetMachineType());
   ASSERT_TRUE(machine->IsPLUS());

   std::vector<unsigned char> saved;
   ASSERT_TRUE(machine->SaveSnapshotNow(saved));
   const unsigned char cpc_type = saved[0x6D];

   delete machine;

   // 4 is "6128 Plus" in the format. Recording 2 here, as the writer used to,
   // makes the load side turn the machine back into a plain 6128.
   EXPECT_EQ(4, cpc_type);
}

// The regression this is really about: a Plus snapshot used to come back as a
// 6128, which also dropped the selected ROM and paged different RAM banks.
TEST(SnapshotMachineType, PlusMachineIsStillAPlusAfterTheRoundTrip)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128PLUS", 20);

   std::vector<unsigned char> saved;
   ASSERT_TRUE(machine->SaveSnapshotNow(saved));
   const unsigned char rom_at_save = machine->GetMem()->GetSelectedRom();

   for (int i = 0; i < 10; ++i)
      machine->RunTimeSlice();

   ASSERT_TRUE(machine->LoadSnapshotNow(&saved[0], saved.size()));

   const bool plus_after_restore = machine->IsPLUS();
   const int type_after_restore = machine->GetMachineType();
   const unsigned char rom_after_restore = machine->GetMem()->GetSelectedRom();

   delete machine;

   EXPECT_TRUE(plus_after_restore);
   EXPECT_EQ(MachineSettings::PLUS_6128, type_after_restore);
   EXPECT_EQ(rom_at_save, rom_after_restore);
}

// The format calls 3 "unknown". Every other implementation that reads this byte
// takes it as a Plus -- Caprice32 writes 3 for its own Plus model, Arnold's
// version 3 switch falls through to 6128 Plus, CPCEC clamps anything above 3
// onto its Plus type -- so an image carrying 3 is a mislabelled Plus snapshot
// rather than a genuinely unknown machine. This pins that reading, and with it
// our ability to load Caprice32's Plus snapshots.
TEST(SnapshotMachineType, UnknownTypeIsTakenAsAPlus)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128", 20);

   std::vector<unsigned char> saved;
   ASSERT_TRUE(machine->SaveSnapshotNow(saved));
   ASSERT_EQ(3, saved[0x10]) << "the type byte is only acted on for version 3 images";
   ASSERT_FALSE(machine->IsPLUS());

   saved[0x6D] = 3;
   ASSERT_TRUE(machine->LoadSnapshotNow(&saved[0], saved.size()));

   const bool plus_after_restore = machine->IsPLUS();

   delete machine;

   EXPECT_TRUE(plus_after_restore);
}
