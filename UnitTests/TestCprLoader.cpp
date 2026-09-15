#include "gtest/gtest.h"

#include "TestUtils.h"

#include <cstring>
#include <string>
#include <vector>

// LoadCprFromBuffer is handed whatever file the user opens, so a malformed or
// hostile .cpr must be refused before anything is written. A cartridge holds 32
// pages of 16 KB; the loader used to accept any page number below 256 and any
// block no larger than the whole file, which wrote past the cartridge buffer.
//
// The images are built here rather than shipped as files, so each test says
// exactly which field is wrong.

namespace
{

EmulatorEngine* NewBootedMachine(DirectoriesImp& dirImp, CDisplay& display, Log& log,
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
   machine->LoadConfiguration("GX4000", "./TestConf.ini");
   machine->Reinit();
   machine->SetFixedSpeed(true);
   machine->SetSpeedLimit(EmulatorEngine::E_FULL);

   return machine;
}

struct Chunk
{
   std::string id;             // "cb00" .. "cb31", or anything else
   unsigned int declared_size; // what the chunk header claims
   unsigned int data_size;     // how many bytes actually follow
   unsigned char fill;
};

std::vector<unsigned char> BuildCpr(const std::vector<Chunk>& chunks)
{
   std::vector<unsigned char> cpr = { 'R', 'I', 'F', 'F', 0, 0, 0, 0, 'A', 'M', 'S', '!' };
   for (const Chunk& chunk : chunks)
   {
      cpr.insert(cpr.end(), chunk.id.begin(), chunk.id.end());
      for (int shift = 0; shift < 32; shift += 8)
         cpr.push_back(static_cast<unsigned char>(chunk.declared_size >> shift));
      cpr.insert(cpr.end(), chunk.data_size, chunk.fill);
   }
   const unsigned int riff_size = static_cast<unsigned int>(cpr.size() - 8);
   for (int shift = 0; shift < 32; shift += 8)
      cpr[4 + shift / 8] = static_cast<unsigned char>(riff_size >> shift);
   return cpr;
}

int Load(EmulatorEngine* machine, std::vector<unsigned char> cpr)
{
   return machine->LoadCprFromBuffer(cpr.data(), static_cast<int>(cpr.size()));
}

bool PageIsFilledWith(EmulatorEngine* machine, int page, unsigned int from, unsigned int to, unsigned char value)
{
   const unsigned char* bank = machine->GetMem()->GetCartridge(page);
   for (unsigned int i = from; i < to; ++i)
   {
      if (bank[i] != value)
         return false;
   }
   return true;
}

class CprLoader : public ::testing::Test
{
protected:
   void SetUp() override
   {
      machine_ = NewBootedMachine(dirImp_, display_, log_, soundFactory_, conf_manager_);
      ASSERT_EQ(0, Load(machine_, BuildCpr({ { "cb00", 0x4000, 0x4000, 0xA5 } })));
   }
   void TearDown() override { delete machine_; }

   DirectoriesImp dirImp_;
   CDisplay display_;
   Log log_;
   SoundFactory soundFactory_;
   ConfigurationManager conf_manager_;
   EmulatorEngine* machine_ = nullptr;
};

}  // namespace

TEST_F(CprLoader, LoadsAWellFormedCartridge)
{
   ASSERT_EQ(0, Load(machine_, BuildCpr({ { "cb00", 0x4000, 0x4000, 0x11 },
                                          { "cb31", 0x0100, 0x0100, 0x22 } })));

   EXPECT_TRUE(PageIsFilledWith(machine_, 0, 0, 0x4000, 0x11));
   EXPECT_TRUE(PageIsFilledWith(machine_, 31, 0, 0x100, 0x22));
}

// A page shorter than 16 KB, or a page the new file does not have at all, must
// not keep the bytes of the cartridge loaded before it.
TEST_F(CprLoader, DoesNotLeaveThePreviousCartridgeInPagesItDoesNotFill)
{
   ASSERT_EQ(0, Load(machine_, BuildCpr({ { "cb00", 0x4000, 0x4000, 0x33 },
                                          { "cb01", 0x4000, 0x4000, 0x33 } })));
   ASSERT_EQ(0, Load(machine_, BuildCpr({ { "cb00", 0x0100, 0x0100, 0x44 } })));

   EXPECT_TRUE(PageIsFilledWith(machine_, 0, 0, 0x100, 0x44));
   EXPECT_TRUE(PageIsFilledWith(machine_, 0, 0x100, 0x4000, 0x00)) << "rest of page 0";
   EXPECT_TRUE(PageIsFilledWith(machine_, 1, 0, 0x4000, 0x00)) << "page 1, absent from the new file";
}

TEST_F(CprLoader, RefusesAPageNumberBeyondTheCartridge)
{
   EXPECT_EQ(-1, Load(machine_, BuildCpr({ { "cb32", 0x4000, 0x4000, 0x55 } })));
   EXPECT_EQ(-1, Load(machine_, BuildCpr({ { "cb99", 0x0010, 0x0010, 0x55 } })));
}

TEST_F(CprLoader, RefusesAChunkThatIsNotAPage)
{
   EXPECT_EQ(-1, Load(machine_, BuildCpr({ { "cbx1", 0x0010, 0x0010, 0x55 } })));
   EXPECT_EQ(-1, Load(machine_, BuildCpr({ { "abcd", 0x0010, 0x0010, 0x55 } })));
}

TEST_F(CprLoader, RefusesAPageLargerThan16KB)
{
   EXPECT_EQ(-1, Load(machine_, BuildCpr({ { "cb00", 0x4001, 0x4001, 0x55 } })));
}

// The second block claims 16 KB but only 16 bytes follow. The whole file is
// still larger than 16 KB, so a check against the file size alone lets it read
// past the end of the buffer.
TEST_F(CprLoader, RefusesABlockThatRunsPastTheEndOfTheFile)
{
   EXPECT_EQ(-1, Load(machine_, BuildCpr({ { "cb00", 0x4000, 0x4000, 0x55 },
                                           { "cb01", 0x4000, 0x0010, 0x55 } })));
}

TEST_F(CprLoader, RefusesABlockSizeWithTheTopBitSet)
{
   EXPECT_EQ(-1, Load(machine_, BuildCpr({ { "cb00", 0x80000010, 0x0010, 0x55 } })));
}

TEST_F(CprLoader, RefusesSomethingThatIsNotACartridge)
{
   std::vector<unsigned char> not_a_cpr(64, 0);
   EXPECT_EQ(-1, Load(machine_, not_a_cpr));
   EXPECT_EQ(-1, machine_->LoadCprFromBuffer(not_a_cpr.data(), 11));
}

// A refused file must leave the cartridge that was already inserted untouched,
// rather than ejecting it and leaving the machine with nothing.
TEST_F(CprLoader, KeepsTheCurrentCartridgeWhenTheNewOneIsRefused)
{
   const unsigned int crc_before = machine_->GetMem()->GetCartridgeCrc();
   ASSERT_NE(0u, crc_before);

   EXPECT_EQ(-1, Load(machine_, BuildCpr({ { "cb00", 0x4000, 0x4000, 0x66 },
                                           { "cb40", 0x0010, 0x0010, 0x66 } })));

   EXPECT_EQ(crc_before, machine_->GetMem()->GetCartridgeCrc());
   EXPECT_TRUE(PageIsFilledWith(machine_, 0, 0, 0x4000, 0xA5));
}
