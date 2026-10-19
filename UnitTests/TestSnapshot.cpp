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
   machine->LoadConfiguration("6128", "./TestConf.ini");
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

// The PSG register-select latch must come back as it was saved. Restoring the
// register file replays all 16 registers through the bus, and each write first
// selects a register, so the latch ends up at 15 -- an assignment made before
// that loop is simply overwritten.
TEST(Snapshot, PsgRegisterSelectSurvivesTheRestore)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, 20);

   std::vector<unsigned char> saved;
   ASSERT_TRUE(machine->SaveSnapshotNow(saved));

   // Whatever the firmware left selected, as the image actually records it.
   const unsigned char selected_at_save = saved[0x5A];
   ASSERT_NE(15, selected_at_save)
      << "register 15 was selected at save time, so this test cannot tell a "
         "restored latch from one left behind by the replay loop";

   for (int i = 0; i < 10; ++i)
      machine->RunTimeSlice();

   ASSERT_TRUE(machine->LoadSnapshotNow(&saved[0], saved.size()));
   const unsigned char selected_after_restore = machine->GetPSG()->GetRegisterAdress();

   delete machine;

   EXPECT_EQ(selected_at_save, selected_after_restore);
}

// Byte B0 bit 0 is VSYNC and bit 1 is HSYNC. Writing them the other way round
// is invisible whenever the two happen to agree, and swaps them whenever they
// do not -- both within our own round trip, since the load side reads them in
// the specified order, and for any other emulator reading the image.
TEST(Snapshot, CrtcSyncFlagsAreSavedInTheirSpecifiedBits)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, 20);

   bool seen_vsync_and_hsync_differing = false;

   for (int i = 0; i < 120; ++i)
   {
      machine->RunTimeSlice();

      std::vector<unsigned char> saved;
      ASSERT_TRUE(machine->SaveSnapshotNow(saved));

      // Sampled after the save, which advances to an instruction boundary
      // before building the header.
      const bool vsync = machine->GetCRTC()->ff4_;          // CRTC drives v_sync_
      const bool hsync = machine->GetSig()->h_sync_;

      EXPECT_EQ(vsync, (saved[0xB0] & 0x01) != 0) << "B0 bit 0 is VSYNC, slice " << i;
      EXPECT_EQ(hsync, (saved[0xB0] & 0x02) != 0) << "B0 bit 1 is HSYNC, slice " << i;

      if (vsync != hsync)
         seen_vsync_and_hsync_differing = true;
   }

   delete machine;

   ASSERT_TRUE(seen_vsync_and_hsync_differing)
      << "vsync and hsync agreed in every sample, so a transposed write would "
         "have passed unnoticed";
}
