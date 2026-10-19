#include "gtest/gtest.h"

#include "TestUtils.h"
#include "MachineState.h"

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

// Extended cartridge (.xpr), as written by rasm's BUILDCPR EXTENDED: RIFF "CXME",
// an "NBBK" chunk giving the pages per bank (16-bit big endian), then "CX"
// chunks numbered in 16-bit big endian.
struct XprBlock
{
   unsigned int declared_size;
   unsigned int data_size;
   unsigned char fill;
};

std::vector<unsigned char> BuildXpr(bool with_nbbk, unsigned int pages_per_bank, const std::vector<XprBlock>& blocks)
{
   std::vector<unsigned char> xpr = { 'R', 'I', 'F', 'F', 0, 0, 0, 0, 'C', 'X', 'M', 'E' };
   if (with_nbbk)
   {
      const unsigned char nbbk[] = { 'N', 'B', 'B', 'K', 2, 0, 0, 0,
                                     static_cast<unsigned char>(pages_per_bank >> 8),
                                     static_cast<unsigned char>(pages_per_bank) };
      xpr.insert(xpr.end(), nbbk, nbbk + sizeof(nbbk));
   }
   unsigned int number = 0;
   for (const XprBlock& block : blocks)
   {
      xpr.push_back('C');
      xpr.push_back('X');
      xpr.push_back(static_cast<unsigned char>(number >> 8));
      xpr.push_back(static_cast<unsigned char>(number));
      ++number;
      for (int shift = 0; shift < 32; shift += 8)
         xpr.push_back(static_cast<unsigned char>(block.declared_size >> shift));
      xpr.insert(xpr.end(), block.data_size, block.fill);
   }
   const unsigned int riff_size = static_cast<unsigned int>(xpr.size() - 8);
   for (int shift = 0; shift < 32; shift += 8)
      xpr[4 + shift / 8] = static_cast<unsigned char>(riff_size >> shift);
   return xpr;
}

int LoadXpr(EmulatorEngine* machine, std::vector<unsigned char> xpr)
{
   return machine->LoadXprFromBuffer(xpr.data(), static_cast<int>(xpr.size()));
}

// 32 pages per bank, page 0 of bank 0 filled with first, page 0 of bank 1 with
// second, the pages in between left short.
std::vector<XprBlock> TwoBanks(unsigned char first, unsigned char second, unsigned int second_size = 0x4000)
{
   std::vector<XprBlock> blocks = { { 0x4000, 0x4000, first } };
   for (int page = 1; page < 32; ++page)
      blocks.push_back({ 0x10, 0x10, 0x00 });
   blocks.push_back({ second_size, second_size, second });
   return blocks;
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

// A legacy .cpr may hold up to 96 pages (1.5 MB): rasm's BUILDCPR LEGACY writes
// cb00..cb95 and refuses more, and the PicoGX cartridge runs such files.
TEST_F(CprLoader, RefusesAPageNumberBeyondTheCartridge)
{
   EXPECT_EQ(-1, Load(machine_, BuildCpr({ { "cb96", 0x4000, 0x4000, 0x55 } })));
   EXPECT_EQ(-1, Load(machine_, BuildCpr({ { "cb99", 0x0010, 0x0010, 0x55 } })));
}

// Pages 32-63 and 64-95 are the second and third 512 KB blocks. The PicoGX
// switches block when cartridge page 0 is read at &3FFE (second), &3FFD (third)
// or &3FFF (base), see https://github.com/Neo2003/PicoGX/blob/master/Programming.md
// "Accessing more than 512kb". On the Plus, page 0 is the lower ROM at &0000.
TEST_F(CprLoader, SwitchesBlocksOfALegacyCartridgeLargerThan512KB)
{
   ASSERT_EQ(0, Load(machine_, BuildCpr({ { "cb00", 0x4000, 0x4000, 0x11 },
                                          { "cb32", 0x4000, 0x4000, 0x22 },
                                          { "cb64", 0x4000, 0x4000, 0x33 },
                                          { "cb95", 0x0010, 0x0010, 0x44 } })));
   Memory* mem = machine_->GetMem();

   EXPECT_EQ(0x11, mem->Get(0x0100));
   mem->Get(0x3FFE);
   EXPECT_EQ(0x22, mem->Get(0x0100)) << "second block";
   mem->Get(0x3FFD);
   EXPECT_EQ(0x33, mem->Get(0x0100)) << "third block";
   mem->Get(0x3FFF);
   EXPECT_EQ(0x11, mem->Get(0x0100)) << "back to the base block";
}

// A cartridge of 512 KB or less has a single block: the same reads are plain
// reads and change nothing.
TEST_F(CprLoader, DoesNotSwitchBlocksOnA512KBCartridge)
{
   Memory* mem = machine_->GetMem();
   EXPECT_EQ(0xA5, mem->Get(0x0100));
   mem->Get(0x3FFE);
   EXPECT_EQ(0xA5, mem->Get(0x0100));
}

// A cartridge larger than 512 KB is several blocks and the running program
// chooses one, so which block is selected is machine state, not media. A state
// taken while the second block was mapped has to come back on that block: the
// cartridge content is the same either way, so nothing else in the state says
// which one the program was using.
TEST_F(CprLoader, CarriesTheSelectedCartridgeBlock)
{
   ASSERT_EQ(0, Load(machine_, BuildCpr({ { "cb00", 0x4000, 0x4000, 0x11 },
                                          { "cb32", 0x4000, 0x4000, 0x22 },
                                          { "cb64", 0x4000, 0x4000, 0x33 } })));
   Memory* mem = machine_->GetMem();

   mem->Get(0x3FFE);
   ASSERT_EQ(0x22, mem->Get(0x0100)) << "the test needs the second block selected";

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine_, state));

   mem->Get(0x3FFF);
   ASSERT_EQ(0x11, mem->Get(0x0100)) << "back to the base block";

   ASSERT_TRUE(MachineState::Load(machine_, &state[0], state.size()));
   EXPECT_EQ(0x22, mem->Get(0x0100)) << "the state was taken on the second block";
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
                                           { "cb96", 0x0010, 0x0010, 0x66 } })));

   EXPECT_EQ(crc_before, machine_->GetMem()->GetCartridgeCrc());
   EXPECT_TRUE(PageIsFilledWith(machine_, 0, 0, 0x4000, 0xA5));
}

TEST_F(CprLoader, LoadsAnExtendedCartridgeAndSwitchesBanks)
{
   ASSERT_EQ(0, LoadXpr(machine_, BuildXpr(true, 32, TwoBanks(0x11, 0x22))));
   Memory* mem = machine_->GetMem();

   EXPECT_EQ(0x11, mem->Get(0x0100));
   mem->Get(0x3FFE);
   EXPECT_EQ(0x22, mem->Get(0x0100)) << "second bank";
   mem->Get(0x3FFF);
   EXPECT_EQ(0x11, mem->Get(0x0100)) << "back to the first bank";
}

// A bank's page past the end of its block reads zero, not whatever the heap held.
TEST_F(CprLoader, ClearsTheBanksOfAnExtendedCartridge)
{
   ASSERT_EQ(0, LoadXpr(machine_, BuildXpr(true, 32, TwoBanks(0x11, 0x22, 0x0100))));
   Memory* mem = machine_->GetMem();

   mem->Get(0x3FFE);
   EXPECT_EQ(0x22, mem->Get(0x0000));
   EXPECT_EQ(0x00, mem->Get(0x0200)) << "past the 256-byte block of bank 1 page 0";
}

TEST_F(CprLoader, RefusesAnExtendedCartridgeWithMorePagesPerBankThanABankHolds)
{
   std::vector<XprBlock> blocks(40, XprBlock{ 0x10, 0x10, 0x66 });
   EXPECT_EQ(-1, LoadXpr(machine_, BuildXpr(true, 33, blocks)));
}

TEST_F(CprLoader, RefusesAnExtendedCartridgeWithoutAValidPageCount)
{
   std::vector<XprBlock> blocks(2, XprBlock{ 0x10, 0x10, 0x66 });
   EXPECT_EQ(-1, LoadXpr(machine_, BuildXpr(true, 0, blocks)));
   EXPECT_EQ(-1, LoadXpr(machine_, BuildXpr(false, 0, blocks)));
}

TEST_F(CprLoader, RefusesAnExtendedCartridgePageLargerThan16KB)
{
   EXPECT_EQ(-1, LoadXpr(machine_, BuildXpr(true, 32, { { 0x4001, 0x4001, 0x66 } })));
}

TEST_F(CprLoader, RefusesAnExtendedCartridgeBlockSizeWithTheTopBitSet)
{
   EXPECT_EQ(-1, LoadXpr(machine_, BuildXpr(true, 32, { { 0x80000010, 0x10, 0x66 } })));
}

// One page per bank and 65 one-byte blocks would allocate 65 banks of 512 KB from a
// file of a few hundred bytes.
TEST_F(CprLoader, RefusesAnExtendedCartridgeWithTooManyBanks)
{
   std::vector<XprBlock> blocks(65, XprBlock{ 1, 1, 0 });
   EXPECT_EQ(-1, LoadXpr(machine_, BuildXpr(true, 1, blocks)));
}
