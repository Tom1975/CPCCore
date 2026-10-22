#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>

#include "gtest/gtest.h"
#include "Machine.h"
#include "Display.h"
#include "FDC.h"
#include "DskTypeManager.h"
#include "TestWorkspace.h"
#include "FormatTypeEDSK.h"
#include <filesystem>
#include <algorithm>

/////////////////////////////////////////////////////////////
/// Helper functions


class SoundFactory : public ISoundFactory
{
   virtual ISound* GetSound(const char* pName) { return nullptr; };
   virtual const char* GetSoundName(ISound*) { return "Mock sound"; };

   virtual const char* GetFirstSoundName() { return nullptr; };
   virtual const char* GetNextSoundName() { return nullptr; };
};

class DirectoriesImp : public IDirectories
{
public:
   virtual const char* GetBaseDirectory()
   {
      return ".";
   }
};


/*void InitBinary(char* binary_to_load, unsigned short addr_ok, unsigned short addr_ko)
{
   // Creation dela machine
   DirectoriesImp dirImp;
   CDisplay display;
   display.Init();
   display.Show(true);

   SoundFactory soundFactory;
   EmulatorEngine* machine = new EmulatorEngine("."));
   machine->SetDirectories(&dirImp);

   machine->SetLog(NULL);
   machine->Init(&display, &soundFactory);
   machine->GetMem()->Initialisation();
   machine->SetFixedSpeed(true);
   machine->GetMem()->Initialisation();

   machine->LoadConfiguration("6128", ".\\TestConf.ini");
   machine->Reinit();

   // Load binary
   machine->LoadBinInt(binary_to_load);

   // Set breakpoints : To OK, KO, finisehd
   machine->AddBreakpoint(addr_ok);  // KO
   machine->AddBreakpoint(addr_ko);  // Error at the end

   bool finished = false;
   while (!finished)
   {
      machine->RunTimeSlice(false); // Run debug
                                    // Check next break : OK = continue
      if (machine->GetProc()->GetPC() == addr_ko)
      {
         finished = true; FAIL() << "Error at the end !"; break;
      }
      else if (machine->GetProc()->GetPC() == addr_ok)
      {
         finished = true; SUCCEED();
      }
   }
}*/

/////////////////////////////////////////////////////////////
// Head load timings
//
// The FDC is driven alone, one Tick = 2 us (one MFM impulse). References :
// - uPD765 datasheet : HUT in 16 ms steps at 8 MHz ; the CPC clocks it at 4 MHz, every time is doubled ;
// - 64nops, "A la decouverte du FDC" (episodes 2 and 3), measured on a CPC : a command waits 1800 to 3800 NOPs
//   before its execution, only 90 to 100 NOPs when it follows the previous one by less than 28000 to 31000 NOPs.
namespace
{
class FdcBench
{
public:
   FdcBench(unsigned char hut = 1)
   {
      fdc_.Init(&types_);
      fdc_.Reset();
      fdc_.LoadDisk(0, TestWorkspace::Fixture("./res/DSK/Bad Cat (UK) (Face A) (1987) [Original] (Weak Sectors) (GAPS).dsk").c_str(), false);
      fdc_.Out(0xFA7E, 1);    // motor on
      Run(600000);
      Command({ 0x03, (unsigned char)(0xA0 | hut), 0x03 });    // Specify, as AMSDOS
      Command({ 0x07, 0x00 });                                  // Recalibrate
      Run(200000);
      Command({ 0x08 });                                        // Sense interrupt status
      ReadResults();
   }

   void Run(unsigned int ticks) { for (unsigned int i = 0; i < ticks; i++) fdc_.Tick(); }
   unsigned char Status() { return fdc_.In(0xFB7E); }

   void Command(std::initializer_list<unsigned char> bytes)
   {
      for (unsigned char b : bytes)
      {
         WaitStatus(0xC0, 0x80);
         fdc_.Out(0xFB7F, b);
         fdc_.Tick();
      }
   }

   // Ticks until (status & mask) == value
   unsigned int WaitStatus(unsigned char mask, unsigned char value)
   {
      unsigned int ticks = 0;
      while ((Status() & mask) != value && ticks < 1000000) { fdc_.Tick(); ticks++; }
      return ticks;
   }

   void ReadResults()
   {
      for (unsigned int guard = 0; guard < 1000000; guard++)
      {
         const unsigned char s = Status();
         if ((s & 0xD0) == 0xD0) fdc_.In(0xFB7F);
         else if ((s & 0x90) == 0x80) return;
         fdc_.Tick();
      }
      ADD_FAILURE() << "no end of the result phase";
   }

   // Read ID : ticks from its last byte to the start of the execution (EXM)
   unsigned int ReadIdStart()
   {
      Command({ 0x4A, 0x00 });
      const unsigned int ticks = WaitStatus(0x20, 0x20);
      ReadResults();
      return ticks;
   }

   DskTypeManager types_;
   FDC fdc_;
};
}

TEST(FDC_HeadLoad, ReadIdWithTheHeadUnloadedWaitsTwoToFourMilliseconds)
{
   FdcBench bench;
   unsigned int shortest = 0xFFFFFFFF, longest = 0;
   for (unsigned int i = 0; i < 8; i++)
   {
      bench.Run(50000 + i * 137);    // 100 ms : the head is unloaded
      const unsigned int ticks = bench.ReadIdStart();
      SCOPED_TRACE(testing::Message() << "read id " << i);
      EXPECT_GE(ticks, 1000u);       // 2 ms
      EXPECT_LE(ticks, 2000u);       // 4 ms
      shortest = std::min(shortest, ticks);
      longest = std::max(longest, ticks);
   }
   // The phase of the internal timer is free : the wait varies
   EXPECT_GT(longest - shortest, 400u);
}

TEST(FDC_HeadLoad, ChainedReadIdStartsAfterAboutOneHundredMicroseconds)
{
   FdcBench bench;
   bench.ReadIdStart();
   const unsigned int ticks = bench.ReadIdStart();
   EXPECT_GE(ticks, 45u);   // 90 us
   EXPECT_LE(ticks, 50u);   // 100 us
}

TEST(FDC_HeadLoad, HeadUnloadsThirtyTwoMillisecondsAfterTheExecution)
{
   FdcBench bench;   // HUT = 1 : 16 ms at 8 MHz, 32 ms at 4 MHz
   bench.ReadIdStart();
   bench.Run(14000);                      // 28 ms : still loaded
   EXPECT_LE(bench.ReadIdStart(), 50u);
   bench.Run(16500);                      // 33 ms : unloaded
   EXPECT_GE(bench.ReadIdStart(), 1000u);
}

TEST(FDC_HeadLoad, HeadUnloadTimeComesFromSpecify)
{
   FdcBench bench(2);   // HUT = 2 : 64 ms
   bench.ReadIdStart();
   bench.Run(20000);                      // 40 ms : still loaded
   EXPECT_LE(bench.ReadIdStart(), 50u);
}

TEST(FDC_HeadLoad, StatusIsBusyThenExecutionThenDirection)
{
   FdcBench bench;
   bench.Run(50000);
   bench.Command({ 0x4A, 0x00 });
   bench.Run(500);                                  // during the head load
   EXPECT_EQ(0x10, bench.Status());
   bench.WaitStatus(0x20, 0x20);
   EXPECT_EQ(0x30, bench.Status());                 // EXM first
   bench.Run(1);
   EXPECT_EQ(0x70, bench.Status());                 // then DIO, until the ID is found
   bench.ReadResults();
}

// A disk saved as "name.dsk" is written to that name, not "name.dsk.DSK" (the extension is compared
// whatever its case)
TEST(FDC_SaveDisk, KeepsAnExtensionWrittenInLowerCase)
{
   FdcBench bench;
   const std::filesystem::path dir = std::filesystem::temp_directory_path() / "cpccore_savedisk_test";
   std::filesystem::create_directories(dir);
   const std::filesystem::path lower = dir / "saved.dsk";
   const std::filesystem::path none = dir / "saved_no_extension";
   std::filesystem::remove(lower);
   std::filesystem::remove(dir / "saved.dsk.DSK");
   std::filesystem::remove(dir / "saved_no_extension.DSK");
   FormatTypeEDSK edsk;
   bench.fdc_.WriteDiskAs(0, lower.string().c_str(), &edsk);
   bench.fdc_.WriteDiskAs(0, none.string().c_str(), &edsk);
   EXPECT_TRUE(std::filesystem::exists(lower));
   EXPECT_FALSE(std::filesystem::exists(dir / "saved.dsk.DSK"));
   EXPECT_TRUE(std::filesystem::exists(dir / "saved_no_extension.DSK"));
   std::filesystem::remove_all(dir);
}

namespace
{
// Read Data of the sector &41 of the track 0, the data and the result bytes read as soon as the status
// allows it (every tick) ; returns the result bytes.
std::vector<unsigned char> ReadDataResultsAtOnce(FdcBench& bench)
{
   bench.Command({ 0x46, 0x00, 0x00, 0x00, 0x41, 0x02, 0x41, 0x2A, 0xFF });
   std::vector<unsigned char> results;
   for (unsigned int g = 0; g < 3000000; g++)
   {
      const unsigned char st = bench.Status();
      if ((st & 0xF0) == 0xF0) bench.fdc_.In(0xFB7F);                         // data
      else if ((st & 0xF0) == 0xD0) results.push_back(bench.fdc_.In(0xFB7F)); // result
      else if (!results.empty() && (st & 0x10) == 0) break;                   // end of the command
      bench.fdc_.Tick();
   }
   return results;
}
}

// The result phase of a read gives its 7 bytes once, even read on the first tick it is offered
TEST(FDC_Result, ReadDataGivesSevenResultBytesReadAtOnce)
{
   FdcBench bench;
   const std::vector<unsigned char> r = ReadDataResultsAtOnce(bench);
   ASSERT_EQ(7u, r.size());
   EXPECT_EQ(0x40, r[0] & 0xC0);   // ST0 : abnormal termination (no TC on the CPC)
   EXPECT_EQ(0x80, r[1]);          // ST1 : end of cylinder
   EXPECT_EQ(0x41, r[5]);          // R
   EXPECT_EQ(0x02, r[6]);          // N
}

namespace
{
// Read Data of the sector &41 of the track 0, each data byte read 'delay' ticks (2 us) after it is
// offered ; returns ST1.
unsigned char St1WhenServedAfter(unsigned int delay)
{
   FdcBench bench;
   bench.Command({ 0x46, 0x00, 0x00, 0x00, 0x41, 0x02, 0x41, 0x2A, 0xFF });
   std::vector<unsigned char> results;
   unsigned int waiting = 0;
   for (unsigned int g = 0; g < 3000000; g++)
   {
      const unsigned char st = bench.Status();
      if ((st & 0xF0) == 0xF0)
      {
         if (++waiting > delay) { bench.fdc_.In(0xFB7F); waiting = 0; }
      }
      else if ((st & 0xF0) == 0xD0) results.push_back(bench.fdc_.In(0xFB7F));
      else if (!results.empty() && (st & 0x10) == 0) break;
      bench.fdc_.Tick();
   }
   return results.size() > 1 ? results[1] : 0xFF;
}
}

// 64nops, datasheet (13 us in MFM at 8 MHz, 4 MHz on the CPC) : a data byte must be read within 26 us
TEST(FDC_Overrun, ADataByteMustBeReadWithinTwentySixMicroseconds)
{
   EXPECT_EQ(0x00, St1WhenServedAfter(12) & 0x10);   // 24 us : in time
   EXPECT_EQ(0x00, St1WhenServedAfter(13) & 0x10);   // 26 us : in time
   EXPECT_EQ(0x10, St1WhenServedAfter(14) & 0x10);   // 28 us : overrun
}
