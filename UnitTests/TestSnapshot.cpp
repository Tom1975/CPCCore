#include "gtest/gtest.h"

#include "TestUtils.h"
#include "Snapshot.h"

#include <vector>

static EmulatorEngine* NewBootedMachine(DirectoriesImp& dirImp, CDisplay& display, Log& log,
                                        SoundFactory& soundFactory, ConfigurationManager& conf_manager,
                                        int slices)
{
   EmulatorEngine* machine = new EmulatorEngine();

   display.Init(false);
   display.Show(false);

   machine->SetDirectories(&dirImp);
   machine->SetLog(&log);
   machine->SetConfigurationManager(&conf_manager);
   machine->Init(&display, &soundFactory);
   machine->GetMem()->Initialisation();
   machine->LoadConfiguration("./TestConf.ini", "./TestConf_0.ini");
   machine->Reinit();
   machine->SetFixedSpeed(true);
   machine->SetSpeedLimit(EmulatorEngine::E_FULL);

   for (int i = 0; i < slices; ++i)
      machine->RunTimeSlice();

   return machine;
}

// A snapshot taken in memory must load back from memory, with no file anywhere
// in the round trip.
TEST(Snapshot, MemoryRoundTripRestoresTheMachine)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, 20);

   std::vector<unsigned char> saved;
   ASSERT_TRUE(machine->SaveSnapshotNow(saved));

   const unsigned short pc_at_save = machine->GetProc()->GetPC();

   for (int i = 0; i < 10; ++i)
      machine->RunTimeSlice();

   ASSERT_TRUE(machine->LoadSnapshotNow(&saved[0], saved.size()));
   const unsigned short pc_after_restore = machine->GetProc()->GetPC();

   delete machine;

   // Only what a .SNA actually carries can be asserted here. A byte-identical
   // re-save would additionally require tape, expansion and full FDC state to
   // be in the image, which the format has no room for.
   EXPECT_EQ(pc_at_save, pc_after_restore);
}

// Chunk length is 32 bit little endian. Decoding the two high bytes with an 8
// bit shift is invisible below 0x10000 but makes a 64K MEM chunk read as 256,
// which desynchronises every chunk after it -- so images from other emulators
// fail to load.
TEST(Snapshot, ChunkLengthDecodesAllFourBytes)
{
   unsigned char low_bytes[8] = { 'M','E','M','0', 0x34, 0x12, 0x00, 0x00 };
   EXPECT_EQ(CSnapshot::DecodeChunkLength(low_bytes), 0x1234u);

   unsigned char mem64k[8] = { 'M','E','M','0', 0x00, 0x00, 0x01, 0x00 };
   EXPECT_EQ(CSnapshot::DecodeChunkLength(mem64k), 0x10000u);

   unsigned char top_byte[8] = { 'M','E','M','0', 0x00, 0x00, 0x00, 0x01 };
   EXPECT_EQ(CSnapshot::DecodeChunkLength(top_byte), 0x1000000u);

   unsigned char every_byte[8] = { 'M','E','M','0', 0x78, 0x56, 0x34, 0x12 };
   EXPECT_EQ(CSnapshot::DecodeChunkLength(every_byte), 0x12345678u);
}

// A truncated image must be rejected or stop cleanly, never read past its end.
TEST(Snapshot, TruncatedImageIsRejected)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, 20);

   std::vector<unsigned char> image;
   ASSERT_TRUE(machine->SaveSnapshotNow(image));

   EXPECT_FALSE(machine->LoadSnapshotNow(&image[0], 8)) << "a header-sized fragment was accepted";

   std::vector<unsigned char> not_a_snapshot(0x200, 0x5A);
   EXPECT_FALSE(machine->LoadSnapshotNow(&not_a_snapshot[0], not_a_snapshot.size()))
      << "an image without the MV - SNA signature was accepted";

   // Half an image: the chunk walk must stop at the end instead of running off it.
   std::vector<unsigned char> half(image.begin(), image.begin() + image.size() / 2);
   machine->LoadSnapshotNow(&half[0], half.size());
   delete machine;
}
