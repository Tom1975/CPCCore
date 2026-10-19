#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

// Tests written from "The Amstrad CPC CRTC Compendium" v1.11 (Logon System, CC BY-NC-ND 4.0),
// chapter 27 : interrupts (GATE ARRAY counter R52, Z80A acknowledge, CRTC HSYNC).
// A failing test is DISABLED_ : it documents a behaviour the emulation does not implement yet.

#include "CompendiumMachine.h"

#include "gtest/gtest.h"

#include <cstdio>

using namespace compendium;

namespace
{
// IM 1 handler at &38 : EI : RET.
void InstallEiRetHandler(Machine& m) { m.Load(0x38, { z80::kEi, 0xC9 }); }

// NOP sled from 'from' to &BFFC, then JP back to 'from'.
void NopSled(Machine& m, unsigned short from)
{
   for (unsigned int a = from; a < 0xBFFD; ++a) m.Ram()[a] = z80::kNop;
   m.Load(0xBFFD, { 0xC3, (unsigned char)(from & 0xFF), (unsigned char)(from >> 8) });
}

// Standard screen of the firmware (R2=46, R3=&8E, R7=30, R9=7) : VSYNC on line kVSyncLine.
const int kR2 = 46;
const int kVSyncLine = 30 * 8;

int Line(Machine& m) { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_; }

// Cycles until the CRTC is on 'line' of the frame (C4 * 8 + C9, standard screen) with C0 = hcc.
bool UntilPosition(Machine& m, int line, int hcc)
{
   return m.CyclesUntil([&]() { return Line(m) == line && m.Crtc().hcc_ == hcc; }, 4 * 20000) >= 0;
}

// Cycles until the cycle where the Z80 fetches the opcode at 'address' ; returns the cycle count.
int CyclesUntilFetch(Machine& m, unsigned short address, int cap)
{
   return m.CyclesUntil([&]() { return m.Cpu().pc_ == address + 1; }, cap);
}

// DI : IM 1 : lower ROM off (&38 in RAM), R52 reset.
std::vector<unsigned char> Prologue(bool reset_r52 = true)
{
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::kIm1);
   z80::Append(p, z80::OutGateArray(reset_r52 ? z80::kRmrRamOnlyResetR52 : z80::kRmrRamOnly));
   return p;
}

// Runs 'program' followed by a NOP sled.
void RunWithSled(Machine& m, const std::vector<unsigned char>& program)
{
   NopSled(m, (unsigned short)(kProgram + program.size()));
   m.Run(program);
}

// Number of interrupt acknowledges started within 'cycles'.
int CountInterrupts(Machine& m, int cycles)
{
   int count = 0;
   bool previous = m.InInterruptAcknowledge();
   for (int i = 0; i < cycles; ++i)
   {
      m.Cycle();
      bool now = m.InInterruptAcknowledge();
      if (now && !previous) ++count;
      previous = now;
   }
   return count;
}

// Runs to the start of the n-th interrupt acknowledge ; returns false if it never comes.
bool UntilInterrupt(Machine& m, int n, int cap_cycles)
{
   bool previous = m.InInterruptAcknowledge();
   for (int i = 0; i < cap_cycles; ++i)
   {
      m.Cycle();
      bool now = m.InInterruptAcknowledge();
      if (now && !previous && --n == 0) return true;
      previous = now;
   }
   return false;
}
}

// 27.1 : R52 counts the ends of HSYNC, whatever their length (R3 = 1 included).
TEST(Compendium_27, R52CountsEveryHSyncEndEvenWithR3One)
{
   Machine m(CRTC::HD6845S);
   ASSERT_TRUE(UntilPosition(m, 5 * 8, 0));
   std::vector<unsigned char> p = Prologue();
   z80::Append(p, z80::WriteCrtc(3, 0x81));
   z80::Append(p, z80::kJrSelf);
   m.Run(p);
   ASSERT_TRUE(UntilPosition(m, 5 * 8 + 10, 0));
   EXPECT_EQ(10, m.Ga().interrupt_counter_);
}

// 27.1 : 52 lines of 64 us between two interrupts : 6 interrupts per frame of 312 lines (300 Hz).
// Counted over one frame once R52 is synchronised by the VSYNC.
TEST(Compendium_27, SixInterruptsPerFrame)
{
   Machine m(CRTC::HD6845S);
   InstallEiRetHandler(m);
   std::vector<unsigned char> p = Prologue();
   p.push_back(z80::kEi);
   RunWithSled(m, p);
   ASSERT_TRUE(UntilPosition(m, kVSyncLine + 2, 0));
   EXPECT_EQ(6, CountInterrupts(m, 4 * 19968));
}

// 27.6.2 to 27.6.5 : the code is interrupted R3+1 us after C0vs = R2 on CRTC 0, 1, 2 and R3+2 us
// on CRTC 3, 4 (their HSYNC follows the display, 1 us later). R3 = 0 : no HSYNC, no interrupt on
// CRTC 0, 1 ; a 16 us HSYNC on CRTC 2 (17 us) and 3, 4 (18 us).
// The interrupted position is C0 at the start of the acknowledge, the code being a NOP sled.
TEST(Compendium_27, InterruptPositionFollowsR3)
{
   struct Case { CRTC::TypeCRTC type; unsigned char r3; int after_r2; };   // after_r2 < 0 : none
   const Case cases[] = {
      { CRTC::HD6845S, 0x8E, 15 }, { CRTC::HD6845S, 0x88, 9 }, { CRTC::HD6845S, 0x81, 2 }, { CRTC::HD6845S, 0x80, -1 },
      { CRTC::UM6845R, 0x8E, 15 }, { CRTC::UM6845R, 0x88, 9 }, { CRTC::UM6845R, 0x81, 2 }, { CRTC::UM6845R, 0x80, -1 },
      { CRTC::MC6845, 0x8E, 15 },  { CRTC::MC6845, 0x88, 9 },  { CRTC::MC6845, 0x81, 2 },  { CRTC::MC6845, 0x80, 17 },
      { CRTC::AMS40489, 0x8E, 16 }, { CRTC::AMS40489, 0x88, 10 }, { CRTC::AMS40489, 0x81, 3 }, { CRTC::AMS40489, 0x80, 18 },
      { CRTC::AMS40226, 0x8E, 16 }, { CRTC::AMS40226, 0x88, 10 }, { CRTC::AMS40226, 0x81, 3 }, { CRTC::AMS40226, 0x80, 18 },
   };
   for (const Case& c : cases)
   {
      SCOPED_TRACE(testing::Message() << "CRTC type " << (int)c.type << ", R3 = " << (int)c.r3);
      Machine m(c.type);
      InstallEiRetHandler(m);
      std::vector<unsigned char> p = Prologue();
      z80::Append(p, z80::WriteCrtc(3, c.r3));
      p.push_back(z80::kEi);
      RunWithSled(m, p);
      // The second interrupt : the first one may be the one armed before the program.
      const bool interrupted = UntilInterrupt(m, 2, 4 * 20000);
      if (c.after_r2 < 0)
      {
         EXPECT_FALSE(interrupted);
         continue;
      }
      ASSERT_TRUE(interrupted);
      EXPECT_EQ((kR2 + c.after_r2) % 64, m.Crtc().hcc_);
   }
}

// 27.2, 27.3.2 : R52 goes back to 0 at the end of the 2nd HSYNC after the start of the VSYNC.
TEST(Compendium_27, VSyncResetsR52AtTheEndOfTheSecondHSync)
{
   Machine m(CRTC::HD6845S);
   std::vector<unsigned char> p = Prologue();
   z80::Append(p, z80::kJrSelf);
   m.Run(p);
   ASSERT_TRUE(UntilPosition(m, kVSyncLine + 1, kR2 + 16));
   EXPECT_EQ(0, m.Ga().interrupt_counter_);
   ASSERT_TRUE(UntilPosition(m, kVSyncLine + 2, kR2 + 16));
   EXPECT_EQ(1, m.Ga().interrupt_counter_);
}

// 27.3.2 : at the end of the 2nd HSYNC after the start of the VSYNC, an interrupt is requested
// only if bit 5 of R52 is set. R52 is reset k lines before the VSYNC line (before its HSYNC end).
TEST(Compendium_27, VSyncRequestsAnInterruptOnlyIfBit5IsSet)
{
   for (int k : { 40, 20 })
   {
      SCOPED_TRACE(testing::Message() << "R52 reset " << k << " lines before the VSYNC");
      Machine m(CRTC::HD6845S);
      InstallEiRetHandler(m);
      ASSERT_TRUE(UntilPosition(m, kVSyncLine - k, 0));
      std::vector<unsigned char> p = Prologue();
      p.push_back(z80::kEi);
      RunWithSled(m, p);
      ASSERT_TRUE(UntilInterrupt(m, 1, 4 * 20000));
      if (k == 40)
         EXPECT_EQ(kVSyncLine + 1, Line(m));         // R52 = 41 then 42 : bit 5 set
      else
         EXPECT_EQ(kVSyncLine + 1 + 52, Line(m));    // R52 = 21 then 22 : next interrupt 52 lines later
   }
}

// 27.2 : R52 goes back to 0 when it goes past 51, also during the 2 HSYNC that follow the start of
// the VSYNC. R52 = 51 at the start of the VSYNC : it wraps at the end of the 1st HSYNC, which
// requests the interrupt ; at the end of the 2nd HSYNC, R52 = 1 (bit 5 clear) : no interrupt.
// DISABLED : GateArray::Tick() does not test R52 = 52 while it waits for the 2 HSYNC of the VSYNC :
// R52 reaches 53 and the interrupt comes one line late, at the end of the 2nd HSYNC.
TEST(Compendium_27, DISABLED_R52WrapsDuringTheVSyncWindow)
{
   Machine m(CRTC::HD6845S);
   InstallEiRetHandler(m);
   ASSERT_TRUE(UntilPosition(m, kVSyncLine - 51, 0));
   std::vector<unsigned char> p = Prologue();
   p.push_back(z80::kEi);
   RunWithSled(m, p);
   ASSERT_TRUE(UntilInterrupt(m, 1, 4 * 20000));
   EXPECT_EQ(kVSyncLine, Line(m));
   EXPECT_EQ(kR2 + 15, m.Crtc().hcc_);
}

// 27.3.1, 27.3.3 : with interrupts disabled, R52 keeps counting, but only one interrupt stays
// pending : after EI, a single interrupt is taken.
TEST(Compendium_27, OnlyOneInterruptPending)
{
   Machine m(CRTC::HD6845S);
   InstallEiRetHandler(m);
   std::vector<unsigned char> p = Prologue();
   RunWithSled(m, p);
   ASSERT_GE(m.CyclesUntil([&]() { return m.Sig().int_; }, 4 * 20000), 0);
   CountInterrupts(m, 4 * 52 * 64 * 2);   // two more wraps of R52, interrupts disabled
   RunWithSled(m, { z80::kEi });
   EXPECT_EQ(1, CountInterrupts(m, 4 * 200));
}

// 27.2 : when the pending interrupt is taken while R52 went on counting, bit 5 of R52 is cleared.
TEST(Compendium_27, AcknowledgeClearsBit5OfR52)
{
   Machine m(CRTC::HD6845S);
   InstallEiRetHandler(m);
   RunWithSled(m, Prologue());
   ASSERT_GE(m.CyclesUntil([&]() { return m.Sig().int_; }, 4 * 20000), 0);
   ASSERT_GE(m.CyclesUntil([&]() { return m.Ga().interrupt_counter_ == 40; }, 4 * 20000), 0);
   RunWithSled(m, { z80::kEi });
   ASSERT_TRUE(UntilInterrupt(m, 1, 4 * 100));
   m.CyclesUntil([&]() { return !m.InInterruptAcknowledge(); }, 100);
   EXPECT_EQ(40 & 0x1F, m.Ga().interrupt_counter_);
}

// 27.3.3 : an interrupt cannot occur on the instruction that follows EI ; a sequence of EI defers
// it until the end of the sequence.
TEST(Compendium_27, EiDefersTheInterruptByOneInstruction)
{
   const std::vector<std::vector<unsigned char>> programs = {
      { z80::kEi },                          // interrupted after EI + 1 NOP
      { z80::kEi, z80::kEi, z80::kEi },      // interrupted after the last EI + 1 NOP
   };
   for (const auto& program : programs)
   {
      SCOPED_TRACE(testing::Message() << program.size() << " EI");
      Machine m(CRTC::HD6845S);
      InstallEiRetHandler(m);
      RunWithSled(m, Prologue());
      ASSERT_GE(m.CyclesUntil([&]() { return m.Sig().int_; }, 4 * 20000), 0);
      RunWithSled(m, program);
      ASSERT_TRUE(UntilInterrupt(m, 1, 4 * 100));
      EXPECT_EQ(kProgram + program.size() + 1, m.Cpu().pc_);
   }
}

// 27.3.3 : HALT repeats 1 us NOPs until the interrupt : it is interrupted at the same position as
// a NOP sled.
TEST(Compendium_27, HaltIsInterruptedLikeANopSled)
{
   Machine m(CRTC::HD6845S);
   InstallEiRetHandler(m);
   std::vector<unsigned char> p = Prologue();
   p.push_back(z80::kEi);
   z80::Append(p, { 0x76, 0x18, 0xFD });   // HALT : JR HALT
   m.Run(p);
   ASSERT_TRUE(UntilInterrupt(m, 2, 4 * 20000));
   EXPECT_EQ(kR2 + 15, m.Crtc().hcc_);
}

// Microseconds from the fetch of the last instruction of a NOP sled before the interrupt to the
// fetch of the first instruction of the handler at 'handler' : 1 us (the NOP) + the interrupt call.
// The emulated Z80 moves PC at a variable cycle of its fetch : rounded to the nearest microsecond.
int MicrosecondsFromLastNopToHandler(Machine& m, unsigned short handler)
{
   unsigned short pc = m.Cpu().pc_;
   int last_fetch = -1;
   for (int i = 0; i < 4 * 20000; ++i)
   {
      m.Cycle();
      if (m.Cpu().pc_ == pc) continue;
      const bool fetch = (m.Cpu().pc_ == (unsigned short)(pc + 1));   // a jump to the handler is no fetch
      pc = m.Cpu().pc_;
      if (pc == handler + 1) return (i - last_fetch + 2) / 4;
      if (fetch) last_fetch = i;
   }
   return -1;
}

// 27.4 : RST #38 lasts 4 us when called by the code, the IM 1 interrupt call to #38 lasts 5 us.
TEST(Compendium_27, Im1CallLastsFiveMicroseconds)
{
   {
      Machine m(CRTC::HD6845S);
      m.Load(0x38, z80::kJrSelf);
      std::vector<unsigned char> p = Prologue();
      const unsigned short rst = (unsigned short)(kProgram + p.size());
      p.push_back(0xFF);   // RST #38
      m.Run(p);
      ASSERT_GE(CyclesUntilFetch(m, rst, 400), 0);
      EXPECT_EQ(4 * 4, CyclesUntilFetch(m, 0x38, 400));
   }
   {
      Machine m(CRTC::HD6845S);
      m.Load(0x38, z80::kJrSelf);
      std::vector<unsigned char> p = Prologue();
      p.push_back(z80::kEi);
      RunWithSled(m, p);
      EXPECT_EQ(1 + 5, MicrosecondsFromLastNopToHandler(m, 0x38));
   }
}

// 27.5 : IM 2 : the call through the vector table lasts 7 us. The low byte of the table address
// is undetermined on a CPC "old" : the table holds 257 times the same byte.
// DISABLED : the emulated IM 2 call lasts 8 us.
TEST(Compendium_27, DISABLED_Im2CallLastsSevenMicroseconds)
{
   Machine m(CRTC::HD6845S);
   for (unsigned int a = 0x3000; a <= 0x3100; ++a) m.Ram()[a] = 0x31;
   m.Load(0x3131, z80::kJrSelf);
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, { 0x3E, 0x30, 0xED, 0x47 });   // LD A,&30 : LD I,A
   z80::Append(p, z80::kIm2);
   z80::Append(p, z80::OutGateArray(z80::kRmrRamOnlyResetR52));
   p.push_back(z80::kEi);
   RunWithSled(m, p);
   EXPECT_EQ(1 + 7, MicrosecondsFromLastNopToHandler(m, 0x3131));
}

// 27.2 (remark) : an R52 reset through RMR bit 4 on the last microsecond of the HSYNC
// (C0 = R2+R3-1) wins over the increment of the end of the HSYNC : R52 = 0 after it. One
// microsecond earlier (C0 = R2+R3-2), R52 is reset, then incremented : R52 = 1.
// Returns R52 just after the end of the HSYNC of the line where RMR resets it at C0 = reset_hcc.
int R52AfterHSyncEndWithResetAt(int reset_hcc)
{
   for (int attempt = 0; attempt < 16; ++attempt)
   {
      const int nops = attempt & 1;
      Machine m(CRTC::HD6845S);
      if (!UntilPosition(m, kVSyncLine + 12, kR2 + attempt / 2)) return -1;
      std::vector<unsigned char> p = { z80::kDi };
      p.insert(p.end(), nops, z80::kNop);
      z80::Append(p, z80::OutGateArray(z80::kRmrRamOnlyResetR52));
      z80::Append(p, z80::kJrSelf);
      m.Run(p);
      int hcc_of_reset = -1;
      unsigned char before = m.Ga().interrupt_counter_;
      while (m.Crtc().hcc_ != kR2 + 15)
      {
         m.Cycle();
         if (m.Ga().interrupt_counter_ == 0 && before != 0 && hcc_of_reset < 0) hcc_of_reset = m.Crtc().hcc_;
         before = m.Ga().interrupt_counter_;
      }
      if (hcc_of_reset == reset_hcc) return m.Ga().interrupt_counter_;
   }
   return -1;
}

TEST(Compendium_27, RmrResetOneMicrosecondBeforeTheLastOneOfTheHSync)
{
   EXPECT_EQ(1, R52AfterHSyncEndWithResetAt(kR2 + 14 - 2));
}

// DISABLED : the end of the HSYNC increments R52 after the reset (R52 = 1) : GATE ARRAY writes are
// applied without their T-state (CSig::Out -> GateArray::TickIO), unlike the CRTC ones.
TEST(Compendium_27, DISABLED_RmrResetOnTheLastMicrosecondOfTheHSyncWins)
{
   EXPECT_EQ(0, R52AfterHSyncEndWithResetAt(kR2 + 14 - 1));
}
