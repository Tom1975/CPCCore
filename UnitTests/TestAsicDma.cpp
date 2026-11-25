#include "gtest/gtest.h"

#include "TestUtils.h"
#include "Motherboard.h"

#include <sstream>
#include <vector>

// DMA sequencer of the ASIC (VGA.cpp, GateArray::HandleDMA ; DMA.cpp, HsyncTick), against the measures of
// plusdma (CLAUDESTRAD, tools/plusdma) on a real 6128 Plus (photo of 2026-11-20, UnitTests/disquettes_test/
// PLUSDMA) :
// - section A : the INT flag of DCSR, read d us after a synchro : one channel alone sets it 3 us after the
//   HSYNC rise, any of the three ; the 3 channels together 5, 6 and 7 us after it, channel 0 first ;
// - section C : PAUSE 16, prescaler p1, then p2 written after k lines : lines until the STOP.

namespace
{

struct PlusMachine
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = nullptr;

   PlusMachine()
   {
      machine = new EmulatorEngine();
      display.Init(false);
      display.Show(false);
      machine->SetDirectories(&dirImp);
      machine->SetLog(&log);
      machine->SetConfigurationManager(&conf_manager);
      machine->Init(&display, &soundFactory);
      machine->GetMem()->Initialisation();
      machine->LoadConfiguration("CONF/CPC6128PLUSFR.cfg");
      machine->Reinit();
      machine->SetFixedSpeed(true);
      machine->SetSpeedLimit(EmulatorEngine::E_FULL);
      for (int i = 0; i < 10; ++i)
         machine->RunTimeSlice(true);
      // RMR2 : ASIC registers in 4000-7FFF
      machine->GetMem()->SetRmr2(0x18);
      // The Z80 waits on DI : HALT, the firmware does not run
      machine->GetMem()->Set(0x8000, 0xF3);
      machine->GetMem()->Set(0x8001, 0x76);
      machine->GetProc()->PrepareForFetch(0x8000);
      Step(8);
      // DMA stopped, flags cleared, prescalers 0
      Write(0x6C0F, 0x00);
      Write(0x6C0F, 0x70);
      Write(0x6C02, 0); Write(0x6C06, 0); Write(0x6C0A, 0);
   }
   ~PlusMachine() { delete machine; }

   void Write(unsigned short address, unsigned char data) { machine->GetMem()->Set(address, data); }
   unsigned char Dcsr() { return machine->GetMem()->GetDCSR(); }

   // One microsecond ; true if the HSYNC pin rose during it
   bool Step(int us = 1)
   {
      bool rise = false;
      for (int i = 0; i < us; ++i)
      {
         const bool before = machine->GetMotherboard()->GetSig()->h_sync_;
         machine->GetMotherboard()->DebugNew(4);
         rise |= !before && machine->GetMotherboard()->GetSig()->h_sync_;
      }
      return rise;
   }
   void ToNextHsync() { while (!Step()) {} }

   // DMA program of the channel, at 0x9000 + 0x100 * channel
   void Program(int channel, std::initializer_list<unsigned short> instructions)
   {
      unsigned short address = 0x9000 + 0x100 * channel;
      Write(0x6C00 + 4 * channel, address & 0xFF);
      Write(0x6C01 + 4 * channel, address >> 8);
      for (unsigned short instruction : instructions)
      {
         Write(address++, instruction & 0xFF);
         Write(address++, instruction >> 8);
      }
   }
};

const unsigned short kInt = 0x4010, kStop = 0x4020;

// Microseconds from the HSYNC rise to the INT flag of each channel of the mask, enabled 10 us after an HSYNC
std::vector<int> IntFlagDelays(unsigned char mask)
{
   PlusMachine m;
   for (int c = 0; c < 3; ++c)
      m.Program(c, { kInt | kStop });
   m.ToNextHsync();
   m.Step(10);
   m.Write(0x6C0F, mask);
   m.ToNextHsync();
   std::vector<int> delays(3, -1);
   for (int us = 0; us < 20; ++us)
   {
      for (int c = 0; c < 3; ++c)
         if (delays[c] < 0 && (m.Dcsr() & (0x40 >> c)))
            delays[c] = us;
      m.Step();
   }
   return delays;
}

}

// Section A, one channel alone : 3 us after the HSYNC rise, for each of the three
TEST(AsicDma, OneChannelAloneExecutesThreeMicrosecondsAfterTheHsync)
{
   EXPECT_EQ(3, IntFlagDelays(0x1)[0]);
   EXPECT_EQ(3, IntFlagDelays(0x2)[1]);
   EXPECT_EQ(3, IntFlagDelays(0x4)[2]);
}

// Section A, the three channels : fetched one after the other, then executed one after the other
TEST(AsicDma, ThreeChannelsExecuteAfterTheThreeFetches)
{
   const std::vector<int> d = IntFlagDelays(0x7);
   EXPECT_EQ(5, d[0]);
   EXPECT_EQ(6, d[1]);
   EXPECT_EQ(7, d[2]);
}

// Two channels : two fetches, then two executions, whatever the channels
TEST(AsicDma, TwoChannelsExecuteAfterTheTwoFetches)
{
   const std::vector<int> d01 = IntFlagDelays(0x3);
   EXPECT_EQ(4, d01[0]);
   EXPECT_EQ(5, d01[1]);
   const std::vector<int> d12 = IntFlagDelays(0x6);
   EXPECT_EQ(4, d12[1]);
   EXPECT_EQ(5, d12[2]);
}

namespace
{
// HSYNC from the one of the INT to the one of the STOP : INT, PAUSE 16, STOP, prescaler p1, p2 written
// `write_at` us after the k-th HSYNC after the INT
int LinesToStop(int p1, int p2, int k, int write_at)
{
   PlusMachine m;
   m.Write(0x6C02, (unsigned char)p1);
   m.Program(0, { kInt, 0x1000 | 16, kStop });
   m.ToNextHsync();
   m.Step(10);
   m.Write(0x6C0F, 0x01);
   m.ToNextHsync();                                         // HSYNC of the INT
   int lines = 0;
   for (int us = 0; lines < 400; ++us)
   {
      if (lines == k && us == write_at)
         m.Write(0x6C02, (unsigned char)p2);
      if (m.Step())
      {
         ++lines;
         us = 0;
      }
      if ((m.Dcsr() & 0x01) == 0)
         return lines;
   }
   return -1;
}

// plusdma, section C, real 6128 Plus : lines counted by the program from its write to the STOP
const char* kPause16[][2] = {
   { "4>4", "081 076 076 076 076 076 071 071 071 071 071 066 066 066 066 066" },
   { "4>1", "033 031 031 031 031 031 029 029 029 029 029 027 027 027 027 027" },
   { "4>8", "145 136 136 136 136 136 127 127 127 127 127 118 118 118 118 118" },
   { "1>1", "033 031 031 029 029 027 027 025 025 023 023 021 021 019 019 017" },
   { "1>4", "081 076 076 071 071 066 066 061 061 056 056 051 051 046 046 041" },
   { "0>0", "017 016 015 014 013 012 011 010 009 008 007 006 005 004 003 002" },
   { "0>3", "065 061 057 053 049 045 041 037 033 029 025 021 017 013 009 005" },
   { "3>0", "017 016 016 016 016 015 015 015 015 014 014 014 014 013 013 013" },
};
}

// Section C : the whole table. The program counts the lines from its write, in the line k : k + its count is
// the HSYNC of the STOP after the one of the INT.
// The write is latched until the next HSYNC : the same result 1 us after the HSYNC, in the middle of the
// line, and 1 us before the next one.
TEST(AsicDma, PauseAndPrescalerWritesGiveTheTableOfTheRealPlus)
{
   for (const auto& row : kPause16)
   {
      const int p1 = row[0][0] - '0', p2 = row[0][2] - '0';
      std::istringstream values(row[1]);
      for (int k = 0; k < 16; ++k)
      {
         int expected;
         values >> expected;
         for (int write_at : { 1, 30, 62 })
            EXPECT_EQ(expected + k, LinesToStop(p1, p2, k, write_at))
               << row[0] << " k=" << k << " write " << write_at << " us after the HSYNC";
      }
   }
}
