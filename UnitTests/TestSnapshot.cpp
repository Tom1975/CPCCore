#include "gtest/gtest.h"

#include "TestUtils.h"

#include <vector>

static std::vector<unsigned char> ReadFile(const char* path)
{
   std::vector<unsigned char> out;
   FILE* f = fopen(path, "rb");
   if (f == nullptr) return out;
   fseek(f, 0, SEEK_END);
   long size = ftell(f);
   rewind(f);
   if (size > 0)
   {
      out.resize((size_t)size);
      if (fread(&out[0], 1, (size_t)size, f) != (size_t)size) out.clear();
   }
   fclose(f);
   return out;
}

// SaveSnapshotNow() must write the file before it returns, and must not let the
// machine run on while doing it. Two saves in a row therefore have to produce
// exactly the same bytes: the first one stops on an instruction boundary, and
// the second finds the machine already there and writes it unchanged.
TEST(Snapshot, SaveSnapshotNowDoesNotAdvanceTheMachine)
{
   const char* kFirst = "./snapshot_now_first.sna";
   const char* kSecond = "./snapshot_now_second.sna";

   DirectoriesImp dirImp;
   CDisplay display;
   Log log;
   SoundFactory soundFactory;
   ConfigurationManager conf_manager;
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

   for (int i = 0; i < 20; ++i)
      machine->RunTimeSlice();

   ASSERT_TRUE(machine->SaveSnapshotNow(kFirst));
   ASSERT_TRUE(machine->SaveSnapshotNow(kSecond));

   std::vector<unsigned char> first = ReadFile(kFirst);
   std::vector<unsigned char> second = ReadFile(kSecond);
   remove(kFirst);
   remove(kSecond);
   delete machine;

   ASSERT_FALSE(first.empty()) << "no snapshot was written";
   EXPECT_EQ(first.size(), second.size());
   EXPECT_TRUE(first == second) << "the machine advanced between two consecutive saves";
}
