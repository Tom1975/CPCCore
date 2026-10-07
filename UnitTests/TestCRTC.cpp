#include "gtest/gtest.h"
#include <string>
#include <utility>

#include "Bus.h"
#include "CRTC.h"
#include "Memoire.h"
#include "Sig.h"
#include "VGA.h"

// Non-regression / characterisation tests for CPCCore/CPCCoreEmu/CRTC.* ,
// checked against "The Amstrad CPC CRTC Compendium" v1.11 (Serge Querne /
// Logon System, CC BY-NC-ND 4.0 -- the document's own licence asks that code
// generated from it carry a mention; this file cites it as its reference
// throughout instead of repeating a boilerplate comment on every test).
//
// Two different kinds of assertion live here, and every TEST says which:
//
// - SAFETY NET: behaviour that already matches the Compendium. These must
//   never fail; a failure here is a genuine regression.
//
// - KNOWN DIVERGENCE: behaviour identified as NOT matching the Compendium.
//   These pin down what the code does TODAY, so an unrelated change cannot
//   silently drift it further, and so that fixing the divergence later is a
//   deliberate, reviewable edit to this file rather than a change nobody
//   notices. When the fix lands, the matching assertion is expected to flip.
//
// Isolation: CRTC::Tick() unconditionally calls gate_array_->Tick(), which is
// not safe to call on an unwired GateArray*. These tests never call Tick();
// instead they call CRTC::ClockCharacter() (Advance() below), which only
// touches CRTC::signals_ and the CRTC's own registers/counters. CRTC 3/4's
// ClockTick34() also reads the CPC+ split-screen/soft-scroll registers
// through gate_array_, so every CRTC is wired to a neutral, never-ticked
// GateArray (see NeutralGateArray() below): all 5 types can be ticked.

namespace
{

// CRTC port addresses (chapitre 4.4.1 du Compendium).
const unsigned short kSelectRegister = 0xBC00;
const unsigned short kWriteRegister = 0xBD00;

const CRTC::TypeCRTC kAllTypes[] = {
   CRTC::HD6845S,   // CRTC 0
   CRTC::UM6845R,   // CRTC 1
   CRTC::MC6845,    // CRTC 2
   CRTC::AMS40489,  // CRTC 3
   CRTC::AMS40226,  // CRTC 4
};

// Types for which the isolated tick harness is safe (see file header).
const CRTC::TypeCRTC kTickableTypes[] = {
   CRTC::HD6845S,   // CRTC 0
   CRTC::UM6845R,   // CRTC 1
   CRTC::MC6845,    // CRTC 2
   CRTC::AMS40489,  // CRTC 3
   CRTC::AMS40226,  // CRTC 4
};

const char* TypeName(CRTC::TypeCRTC type)
{
   switch (type)
   {
   case CRTC::HD6845S: return "CRTC0 (HD6845S/UM6845)";
   case CRTC::UM6845R: return "CRTC1 (UM6845R)";
   case CRTC::MC6845: return "CRTC2 (MC6845)";
   case CRTC::AMS40489: return "CRTC3 (ASIC 40489)";
   case CRTC::AMS40226: return "CRTC4 (ASIC 40226)";
   default: return "?";
   }
}

// Builds a CRTC wired to its own CSig only (no GateArray, no PPI). sig must
// outlive crtc; CRTC keeps a raw pointer to it (CRTC::SetSig).
// ClockTick34() (CRTC 3/4) reads the CPC+ split/soft-scroll registers through
// gate_array_->memory_ (GetSPLT, GetSSCR) and gate_array_->GetSSA(). A bare
// GateArray wired to a Memory whose ASIC registers are all zero (no split, no
// soft scroll) is enough for that; neither is ever ticked here.
GateArray* NeutralGateArray()
{
   static Memory* memory = nullptr;
   static GateArray* gate_array = nullptr;
   if (gate_array == nullptr)
   {
      memory = new Memory(nullptr);
      memset(memory->GetAsicRegisters(), 0, 0x4000);
      gate_array = new GateArray();
      gate_array->memory_ = memory;
   }
   return gate_array;
}

// What an IN reads when nothing drives the data bus.
const unsigned char kFloatingBus = 0xA5;

Bus* FloatingDataBus()
{
   static Bus* bus = nullptr;
   if (bus == nullptr)
   {
      bus = new Bus(8);
      bus->SetBus(kFloatingBus);
   }
   return bus;
}

void MakeCrtc(CRTC& crtc, CSig& sig, CRTC::TypeCRTC type)
{
   sig.data_bus_ = FloatingDataBus();
   crtc.SetSig(&sig);
   crtc.SetGateArray(NeutralGateArray());
   crtc.DefinirTypeCRTC(type);
   crtc.Reset();
}

void WriteRegister(CRTC& crtc, unsigned char reg, unsigned char value)
{
   crtc.Out(kSelectRegister, reg);
   crtc.Out(kWriteRegister, value);
}

// Programs the "standard European" table from the CPC low ROM (Compendium
// chapitre 4.1, table ROM address &5C5): lines of 64 chars (40 displayed),
// 312 raster lines as 39 character rows of 8 lines, no vertical adjustment.
// Written through Out(), as the boot ROM does (Compendium chapitre 4.1,
// note 2), rather than relying on CRTC::Reset()'s defaults.
void ProgramStandardEuropeanScreen(CRTC& crtc)
{
   WriteRegister(crtc, 0, 0x3F);  // R0 = 63  (64 char/line)
   WriteRegister(crtc, 1, 0x28);  // R1 = 40  (40 displayed)
   WriteRegister(crtc, 2, 0x2E);  // R2 = 46  (HSYNC position)
   WriteRegister(crtc, 3, 0x8E);  // R3 = HSYNC width 14, VSYNC width nibble 8
   WriteRegister(crtc, 4, 0x26);  // R4 = 38  (39 character rows)
   WriteRegister(crtc, 5, 0x00);  // R5 = 0   (no vertical adjustment)
   WriteRegister(crtc, 6, 0x19);  // R6 = 25  (25 rows displayed)
   WriteRegister(crtc, 7, 0x1E);  // R7 = 30  (VSYNC position)
   WriteRegister(crtc, 9, 0x07);  // R9 = 7   (8 lines/character row)
}

// Runs CRTC::ClockCharacter() : the CRTC's own per-type tick function, the
// bus interface (pending I/O) and the VSYNC pin -- everything CRTC::Tick()
// does except gate_array_->Tick() [unsafe, see file header], cursor-line
// handling [no-op: cursor_line_ is nullptr by default] and lightpen
// bookkeeping [no-op: gun_button_ is 0 by default].
// One call = one microsecond of CRTC time.
void Advance(CRTC& crtc)
{
   crtc.ClockCharacter();
}

void AdvanceMicroseconds(CRTC& crtc, int n)
{
   for (int i = 0; i < n; ++i) Advance(crtc);
}

// Ticks at least once, then until hcc_ == target. Ticking at least once
// first guarantees forward progress even when hcc_ already equals target
// (e.g. target 0 right after Reset()), so callers reliably observe a full
// lap of the counter rather than a zero-tick no-op.
void AdvanceUntilHccEquals(CRTC& crtc, unsigned char target)
{
   Advance(crtc);
   while (crtc.hcc_ != target) Advance(crtc);
}

// Ticks until signals_->v_sync_ (rising edge) is seen, starting from
// whatever edge state the CRTC is currently in. Returns the number of ticks
// consumed, or -1 if the edge was not seen within cap ticks.
int TicksUntilVSyncRisingEdge(CRTC& crtc, CSig& sig, int cap)
{
   bool was_set = sig.v_sync_;
   for (int i = 0; i < cap; ++i)
   {
      Advance(crtc);
      if (sig.v_sync_ && !was_set) return i + 1;
      was_set = sig.v_sync_;
   }
   return -1;
}

}  // namespace

// ---------------------------------------------------------------------------
// Group A: register masking, Out() only, no ticking -- valid for all 5 types.
// ---------------------------------------------------------------------------

// Compendium chapitre 10.1: "La definition de R9 est de 5 bits." A write of
// 0xFF must be truncated to 0x1F, as on real hardware. SAFETY NET.
TEST(CRTC_RegisterMasks, R9KeepsOnlyFiveBits)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);

      WriteRegister(crtc, 9, 0xFF);

      EXPECT_EQ(0x1F, crtc.registers_list_[9]);
   }
}

// Compendium chapitre 19.1 (register table) and 19.2 (SKEW-DISPTMG): bits 0-1
// of R8 (interlace) exist on every type; bits 4-5 (BORDER ON/OFF, BORDER
// DELAI +1/+2) exist on CRTC 0/3/4 only; bits 6-7 (cursor skew) on CRTC0 only.
// SAFETY NET for the stored value. NOTE: no ClockTickN() reads bits 4-7 yet,
// so the SKEW-DISPTMG behaviour itself is still to be implemented.
TEST(CRTC_RegisterMasks, R8KeepsTheBitsEachTypeImplements)
{
   struct { CRTC::TypeCRTC type; unsigned char expected; } const cases[] = {
      { CRTC::HD6845S,  0xF3 },
      { CRTC::UM6845R,  0x03 },
      { CRTC::MC6845,   0x03 },
      { CRTC::AMS40489, 0x33 },
      { CRTC::AMS40226, 0x33 },
   };
   for (const auto& c : cases)
   {
      SCOPED_TRACE(TypeName(c.type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, c.type);

      WriteRegister(crtc, 8, 0xFF);

      EXPECT_EQ(c.expected, crtc.registers_list_[8]);
   }
}

// The CRTC type can change without a Reset() (snapshot load, settings
// change): the R8 mask must follow the new type. SAFETY NET.
TEST(CRTC_RegisterMasks, R8MaskFollowsATypeChangeWithoutReset)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::UM6845R);

   crtc.DefinirTypeCRTC(CRTC::HD6845S);
   WriteRegister(crtc, 8, 0xFF);
   EXPECT_EQ(0xF3, crtc.registers_list_[8]);

   crtc.DefinirTypeCRTC(CRTC::MC6845);
   WriteRegister(crtc, 8, 0xFF);
   EXPECT_EQ(0x03, crtc.registers_list_[8]);
}

// Compendium chapitre 4.1: the CPC low ROM programs R0=0x3F, R1=0x28,
// R2=0x2E, R3=0x8E, R4=0x26, R5=0x00, R6=0x19, R7=0x1E, R9=0x07 (the
// "standard European" 312-line, 39-character-row table at ROM address &5C5).
// CRTC::Reset() hard-codes exactly this table as the post-Reset register
// state (not fetched from an actual ROM image), independently of
// registers_mask_[]. SAFETY NET: this is the baseline every other test in
// this file assumes; if it breaks, the room they were tested in has moved.
TEST(CRTC_RegisterDefaults, MatchTheEuropeanRomTableAfterReset)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);

      EXPECT_EQ(0x3F, crtc.registers_list_[0]);
      EXPECT_EQ(0x28, crtc.registers_list_[1]);
      EXPECT_EQ(0x2E, crtc.registers_list_[2]);
      EXPECT_EQ(0x8E, crtc.registers_list_[3]);
      EXPECT_EQ(0x26, crtc.registers_list_[4]);
      EXPECT_EQ(0x00, crtc.registers_list_[5]);
      EXPECT_EQ(0x19, crtc.registers_list_[6]);
      EXPECT_EQ(0x1E, crtc.registers_list_[7]);
      EXPECT_EQ(0x07, crtc.registers_list_[9]);
   }
}

// Real hardware's RESET forces DISPEN and VSYNC inactive. CRTC::Reset()
// must clear the flip-flops behind them (ff1_/ff3_: DE, ff4_: VSYNC), as it
// already does for signals_->h_sync_/v_sync_. SAFETY NET.
TEST(CRTC_Reset, ClearsDisplayEnableAndVSyncFlipFlops)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::UM6845R);

   crtc.ff1_ = true;
   crtc.ff3_ = true;
   crtc.ff4_ = true;

   crtc.Reset();

   EXPECT_FALSE(crtc.ff1_);
   EXPECT_FALSE(crtc.ff3_);
   EXPECT_FALSE(crtc.ff4_);
}

// The HSYNC/VSYNC widths derived from R3 must match the R3 value Reset()
// installs, exactly as if R3 had been written through Out(). SAFETY NET.
TEST(CRTC_Reset, DerivesSyncWidthsFromTheDefaultR3)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC reset_only; CSig sig1;
      MakeCrtc(reset_only, sig1, type);

      CRTC written; CSig sig2;
      MakeCrtc(written, sig2, type);
      WriteRegister(written, 3, reset_only.registers_list_[3]);

      EXPECT_EQ(written.horizontal_sync_width_, reset_only.horizontal_sync_width_);
      EXPECT_EQ(written.vertical_sync_width_, reset_only.vertical_sync_width_);
      EXPECT_EQ(14, reset_only.horizontal_sync_width_);
   }
}

// Compendium chapitre 14.1/14.2: R3 = vvvvhhhh on CRTC 0, 3 and 4 (VSYNC
// lines, 0 meaning 16) and xxxxhhhh on CRTC 1 and 2 (VSYNC always 16 lines).
// SAFETY NET.
TEST(CRTC_SyncWidths, VSyncWidthFollowsR3HighNibbleOnCrtc034Only)
{
   struct { unsigned char r3; int crtc034; } const cases[] = {
      { 0x0E, 16 }, { 0x1E, 1 }, { 0x3E, 3 }, { 0x4E, 4 }, { 0x8E, 8 }, { 0xFE, 15 },
   };
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      const bool programmable = (type == CRTC::HD6845S || type == CRTC::AMS40489 || type == CRTC::AMS40226);
      for (const auto& c : cases)
      {
         CRTC crtc; CSig sig;
         MakeCrtc(crtc, sig, type);
         WriteRegister(crtc, 3, c.r3);
         EXPECT_EQ(programmable ? c.crtc034 : 16, crtc.vertical_sync_width_) << "R3=" << (int)c.r3;
      }
   }
}

// ---------------------------------------------------------------------------
// Group B: tick-driven safety net for CRTC 0/1/2, standard European screen.
// ---------------------------------------------------------------------------

// Compendium chapitre 4.1: (R0+1) lines of 64 microseconds/(R4+1)*(R9+1)+R5
// character rows -- 64 * (39*8) = 19968 microseconds for the table used here.
TEST(CRTC_FrameTiming, StandardEuropeanFrameIs19968Microseconds)
{
   for (CRTC::TypeCRTC type : kTickableTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      ProgramStandardEuropeanScreen(crtc);

      const int cap = 3 * 19968;
      const int first = TicksUntilVSyncRisingEdge(crtc, sig, cap);
      EXPECT_NE(-1, first) << "no VSYNC seen within " << cap << " ticks";
      if (first == -1) continue;

      const int second = TicksUntilVSyncRisingEdge(crtc, sig, cap);
      EXPECT_NE(-1, second) << "no second VSYNC seen within " << cap << " ticks";
      if (second == -1) continue;

      EXPECT_EQ(19968, second);
   }
}

// Compendium chapitre 13.1: C0 counts 0..R0 then wraps to 0.
TEST(CRTC_HorizontalCounter, WrapsFromR0BackToZero)
{
   for (CRTC::TypeCRTC type : kTickableTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      ProgramStandardEuropeanScreen(crtc);

      // Land on hcc_ == 0 first so the wrap we observe is unambiguous.
      AdvanceUntilHccEquals(crtc, 0);

      bool mismatch = false;
      for (int expected = 1; !mismatch && expected <= 63; ++expected)
      {
         Advance(crtc);
         EXPECT_EQ(expected, crtc.hcc_) << "at expected hcc_ == " << expected;
         mismatch = (crtc.hcc_ != expected);
      }
      if (mismatch) continue;

      Advance(crtc);
      EXPECT_EQ(0, crtc.hcc_) << "hcc_ did not wrap after R0 (63)";
   }
}

// Compendium chapitre 17.1: DISPEN goes low (BORDER) when C0 == R1, and high
// again at the start of the next line (C0 == 0). ff1_ is the CRTC's own
// DISPEN/BORDER-from-R1 flip-flop.
TEST(CRTC_DisplayEnable, BorderAssertedAtR1AndClearedAtNewLine)
{
   for (CRTC::TypeCRTC type : kTickableTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      ProgramStandardEuropeanScreen(crtc);

      AdvanceUntilHccEquals(crtc, 0);
      EXPECT_TRUE(crtc.ff1_) << "display should be enabled right after C0 == 0";

      AdvanceUntilHccEquals(crtc, 0x28);  // R1 == 40
      EXPECT_FALSE(crtc.ff1_) << "display should be disabled once C0 == R1";

      AdvanceUntilHccEquals(crtc, 0);
      EXPECT_TRUE(crtc.ff1_) << "display should re-enable at the next C0 == 0";
   }
}

// Compendium chapitre 16.1: VSYNC is generated when C4 (vcc_) reaches R7.
TEST(CRTC_VerticalSync, AssertedWhenC4ReachesR7)
{
   for (CRTC::TypeCRTC type : kTickableTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      ProgramStandardEuropeanScreen(crtc);

      const int cap = 19968 + 1000;
      int i = 0;
      for (; i < cap; ++i)
      {
         Advance(crtc);
         if (sig.v_sync_) break;
      }
      EXPECT_LT(i, cap) << "VSYNC never asserted";
      if (i >= cap) continue;
      EXPECT_EQ(0x1E, crtc.vcc_) << "VSYNC asserted with vcc_ != R7 (30)";
   }
}

// ---------------------------------------------------------------------------
// Group C: HSYNC re-entrancy (Compendium chapitre 15.3).
//
// With R0 = 1 and R2 = 0, C0 alternates 0/1 and equals R2 every other tick;
// with R3l = 2, C0 == R2 also holds at the position C0 = R2 + R3l where the
// HSYNC should end. (R0 = 0, the Compendium's own 15.3.2 example, is covered
// for CRTC 0 in group F.)
// - CRTC 0 is protected (15.3.1, 15.3.2): the HSYNC ends and cannot restart
//   on that same position; it restarts on the next C0 == R2. The HSYNC pin
//   reads 11 00 11 00 ...
// - CRTC 1, 2, 3 and 4 have the bug (15.3.1, 15.3.2): the HSYNC does not end,
//   C3l overflows (15, 0, ...) and the pin stays high ("HSYNC infinie"). On
//   CRTC 1 the internal off/on transition is shorter than 1 us (15.3.4),
//   so at 1 us resolution the pin reads 1 constantly.
// ---------------------------------------------------------------------------

namespace
{
std::string HSyncTrace(CRTC::TypeCRTC type, int ticks, int* falls = nullptr)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, type);
   WriteRegister(crtc, 0, 1);     // R0 = 1
   WriteRegister(crtc, 3, 0x82);  // R3l = 2
   // R2 = 0, written at the end of the character C0 = 0 (T-state 2) : taken by the next
   // window, on C0 = 1, so that it does not start an HSYNC on the current C0 = 0 (R2.JIT)
   crtc.Out(kSelectRegister, 2);
   crtc.Out(kWriteRegister, 0, 2);

   std::string trace;
   if (falls) *falls = 0;
   for (int i = 0; i < ticks; ++i)
   {
      Advance(crtc);
      trace += sig.h_sync_ ? '1' : '0';
      if (falls && sig.hsync_fall_) ++*falls;
      sig.hsync_fall_ = sig.hsync_raise_ = false;  // consumed by the GATE ARRAY
   }
   return trace;
}
}  // namespace

// SAFETY NET.
TEST(CRTC_HSyncReentrancy, Crtc0IsProtected)
{
   // The first tick takes C0 from 0 to 1: no HSYNC yet.
   EXPECT_EQ("011001100110011001100110", HSyncTrace(CRTC::HD6845S, 24));
}

// SAFETY NET. CRTC 1 also signals its invisible restart to the GATE ARRAY.
TEST(CRTC_HSyncReentrancy, Crtc1OverflowsWithAnInvisibleRestart)
{
   int falls = 0;
   EXPECT_EQ("011111111111111111111111", HSyncTrace(CRTC::UM6845R, 24, &falls));
   EXPECT_GT(falls, 0);
}

// SAFETY NET. No HSYNC end at all is signalled on CRTC 2, 3 and 4.
TEST(CRTC_HSyncReentrancy, Crtc234Overflow)
{
   for (CRTC::TypeCRTC type : { CRTC::MC6845, CRTC::AMS40489, CRTC::AMS40226 })
   {
      SCOPED_TRACE(TypeName(type));
      int falls = 0;
      EXPECT_EQ("011111111111111111111111", HSyncTrace(type, 24, &falls));
      EXPECT_EQ(0, falls);
   }
}

// Compendium 14.6 / 27.6.3: with R3l=0, CRTC 0 and 1 produce no HSYNC, so the
// GATE ARRAY must not see any HSYNC end either (no interrupt). SAFETY NET.
TEST(CRTC_HSyncReentrancy, NoHSyncEndWithR3lZeroOnCrtc01)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::UM6845R })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      ProgramStandardEuropeanScreen(crtc);
      WriteRegister(crtc, 3, 0x80);
      int events = 0;
      for (int i = 0; i < 19968; ++i)
      {
         Advance(crtc);
         if (sig.hsync_fall_ || sig.hsync_raise_ || sig.h_sync_) ++events;
         sig.hsync_fall_ = sig.hsync_raise_ = false;
      }
      EXPECT_EQ(0, events);
   }
}

// Compendium 14.5.2: CRTC 1 keeps handling R3l=0 during the HSYNC, which
// cancels it on the T-state of the write (here T-state 0 of the 3rd us : 2 us);
// CRTC 0 and 2 treat 0 as a value to reach (C3l overflows to 16). SAFETY NET.
TEST(CRTC_HSyncReentrancy, WritingR3lZeroDuringHSync)
{
   struct { CRTC::TypeCRTC type; int length; } const cases[] = {
      { CRTC::HD6845S, 16 }, { CRTC::UM6845R, 2 }, { CRTC::MC6845, 16 },
   };
   for (const auto& c : cases)
   {
      SCOPED_TRACE(TypeName(c.type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, c.type);
      ProgramStandardEuropeanScreen(crtc);   // R2 = 46, R3l = 14
      while (!sig.h_sync_) Advance(crtc);
      int length = 1;
      Advance(crtc); ++length;
      Advance(crtc); ++length;                // 3rd us of the HSYNC
      WriteRegister(crtc, 3, 0x80);           // R3l = 0
      --length;
      while (sig.h_sync_ && length < 64) { Advance(crtc); ++length; }
      EXPECT_EQ(c.length, length);
   }
}

// ---------------------------------------------------------------------------
// Group D: sync widths observed on the pins, CRTC 0/1/2 (Compendium 14).
// ---------------------------------------------------------------------------

namespace
{
// Longest HSYNC pulse and number of HSYNC pulses, in microseconds, over one
// standard frame programmed with the given R3.
void MeasureHSync(CRTC::TypeCRTC type, unsigned char r3, int& longest, int& count)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, type);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 3, r3);
   longest = 0; count = 0;
   int current = 0;
   for (int i = 0; i < 19968; ++i)
   {
      Advance(crtc);
      if (sig.h_sync_) { if (current++ == 0) ++count; } else current = 0;
      if (current > longest) longest = current;
   }
}

// Length in microseconds of the first full VSYNC pulse. If rewrite_line > 0,
// R3 is rewritten with rewrite_r3 on that VSYNC line (1-based), at C0 = 10.
int MeasureVSync(CRTC::TypeCRTC type, unsigned char r3, int rewrite_line = 0, unsigned char rewrite_r3 = 0)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, type);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 3, r3);
   while (!sig.v_sync_) Advance(crtc);
   int length = 1;
   while (length < 3 * 1024)
   {
      if (rewrite_line > 0 && length == (rewrite_line - 1) * 64 + 10)
         WriteRegister(crtc, 3, rewrite_r3);
      Advance(crtc);
      if (!sig.v_sync_) break;
      ++length;
   }
   return length;
}
}  // namespace

// Compendium 14.2: VSYNC lasts R3h lines on CRTC 0, 3 and 4 (0 = 16), 16
// lines on CRTC 1 and 2. SAFETY NET.
TEST(CRTC_SyncWidths, VSyncLinesOnThePin)
{
   struct { unsigned char r3; int crtc034_lines; } const cases[] = {
      { 0x0E, 16 }, { 0x1E, 1 }, { 0x4E, 4 }, { 0x8E, 8 }, { 0xFE, 15 },
   };
   for (CRTC::TypeCRTC type : kTickableTypes)
   {
      SCOPED_TRACE(TypeName(type));
      for (const auto& c : cases)
      {
         const bool programmable = (type == CRTC::HD6845S || type == CRTC::AMS40489 || type == CRTC::AMS40226);
         const int lines = programmable ? c.crtc034_lines : 16;
         EXPECT_EQ(lines * 64, MeasureVSync(type, c.r3)) << "R3=" << (int)c.r3;
      }
   }
}

// Compendium 14.2: with R3h=9, rewriting R3h=8 during the 8th VSYNC line
// ends the VSYNC after 8 lines; doing it during the 9th line (C3h already
// past 8) makes the 4-bit counter wrap: 16 lines, then 8 more. SAFETY NET.
TEST(CRTC_SyncWidths, Crtc0ShorteningR3hDuringVSync)
{
   EXPECT_EQ(9 * 64, MeasureVSync(CRTC::HD6845S, 0x9E));
   EXPECT_EQ(8 * 64, MeasureVSync(CRTC::HD6845S, 0x9E, 8, 0x8E));
   EXPECT_EQ(24 * 64, MeasureVSync(CRTC::HD6845S, 0x9E, 9, 0x8E));
}

// Compendium 14.6: R3l=0 means no HSYNC at all on CRTC 0 and 1, but a 16 us
// HSYNC on CRTC 2, 3 and 4. Other values give R3l us. SAFETY NET.
TEST(CRTC_SyncWidths, HSyncWidthOnThePin)
{
   for (CRTC::TypeCRTC type : kTickableTypes)
   {
      SCOPED_TRACE(TypeName(type));
      int longest, count;

      MeasureHSync(type, 0x84, longest, count);
      EXPECT_EQ(4, longest);
      EXPECT_EQ(312, count);

      MeasureHSync(type, 0x8E, longest, count);
      EXPECT_EQ(14, longest);
      EXPECT_EQ(312, count);

      MeasureHSync(type, 0x80, longest, count);
      if (type == CRTC::MC6845 || type == CRTC::AMS40489 || type == CRTC::AMS40226)
      {
         EXPECT_EQ(16, longest);
         EXPECT_EQ(312, count);
      }
      else
      {
         EXPECT_EQ(0, count) << "R3l=0 must not produce any HSYNC on CRTC 0/1";
      }
   }
}

// ---------------------------------------------------------------------------
// Group E: register reads (Compendium chapitre 21).
// ---------------------------------------------------------------------------

namespace
{
const unsigned short kStatusPort = 0xBE00;
const unsigned short kReadPort = 0xBF00;

// R12..R15 written through Out(), light pen R16/R17 set directly (read-only).
void FillReadableRegisters(CRTC& crtc)
{
   WriteRegister(crtc, 12, 0x2A);
   WriteRegister(crtc, 13, 0x5B);
   WriteRegister(crtc, 14, 0x13);
   WriteRegister(crtc, 15, 0x9C);
   crtc.registers_list_[16] = 0x15;
   crtc.registers_list_[17] = 0x67;
}

unsigned char ReadRegister(CRTC& crtc, unsigned short port, unsigned char reg)
{
   crtc.Out(kSelectRegister, reg);
   return crtc.In(port);
}
}  // namespace

// 21.2.1: CRTC 0 reads R12..R17 on &BF00 (register number on 5 bits), every
// other register reads 0. SAFETY NET.
TEST(CRTC_RegisterRead, Crtc0)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::HD6845S);
   FillReadableRegisters(crtc);
   for (int reg = 0; reg < 32; ++reg)
   {
      SCOPED_TRACE(reg);
      unsigned char expected = 0;
      switch (reg)
      {
      case 12: expected = 0x2A; break;
      case 13: expected = 0x5B; break;
      case 14: expected = 0x13; break;
      case 15: expected = 0x9C; break;
      case 16: expected = 0x15; break;
      case 17: expected = 0x67; break;
      }
      EXPECT_EQ(expected, ReadRegister(crtc, kReadPort, reg));
   }
   EXPECT_EQ(0x2A, ReadRegister(crtc, kReadPort, 12 + 0x60)) << "register number is truncated to 5 bits";
}

// 21.2.2: CRTC 1 reads R14..R17 on &BF00; R31 reads a non-zero value; every
// other register reads 0. SAFETY NET.
TEST(CRTC_RegisterRead, Crtc1)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::UM6845R);
   FillReadableRegisters(crtc);
   for (int reg = 0; reg < 31; ++reg)
   {
      SCOPED_TRACE(reg);
      unsigned char expected = 0;
      switch (reg)
      {
      case 14: expected = 0x13; break;
      case 15: expected = 0x9C; break;
      case 16: expected = 0x15; break;
      case 17: expected = 0x67; break;
      }
      EXPECT_EQ(expected, ReadRegister(crtc, kReadPort, reg));
   }
   EXPECT_NE(0, ReadRegister(crtc, kReadPort, 31));
}

// 21.2.2: CRTC 2 reads R14..R17 on &BF00, every other register reads 0.
// (28.1.9 only lists R16/R17 for CRTC 2; 21.2.2, the detailed chapter, is
// followed here.) SAFETY NET.
TEST(CRTC_RegisterRead, Crtc2)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::MC6845);
   FillReadableRegisters(crtc);
   for (int reg = 0; reg < 32; ++reg)
   {
      SCOPED_TRACE(reg);
      unsigned char expected = 0;
      switch (reg)
      {
      case 14: expected = 0x13; break;
      case 15: expected = 0x9C; break;
      case 16: expected = 0x15; break;
      case 17: expected = 0x67; break;
      }
      EXPECT_EQ(expected, ReadRegister(crtc, kReadPort, reg));
   }
}

// 21.2.3: CRTC 3/4 only use the 3 low bits of the register number for reads:
// R16, R17, STATUS1 (R10), STATUS2 (R11), R12, R13, R14, R15. SAFETY NET
// (status values are covered elsewhere).
TEST(CRTC_RegisterRead, Crtc34UseAThreeBitTable)
{
   const unsigned char table[8] = { 0x15, 0x67, 0, 0, 0x2A, 0x5B, 0x13, 0x9C };
   for (CRTC::TypeCRTC type : { CRTC::AMS40489, CRTC::AMS40226 })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      FillReadableRegisters(crtc);
      for (int reg = 0; reg < 32; ++reg)
      {
         if ((reg & 7) == 2 || (reg & 7) == 3) continue;
         SCOPED_TRACE(reg);
         EXPECT_EQ(table[reg & 7], ReadRegister(crtc, kReadPort, reg));
      }
   }
}

// 21.3: &BE00 is a status register on CRTC 1 only (bits 0-4 and 7 read 0),
// a mirror of &BF00 on CRTC 3/4, and nothing on CRTC 0/2 (open bus, 0xFF
// observed on a CRTC 2). SAFETY NET.
TEST(CRTC_RegisterRead, StatusPort)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      FillReadableRegisters(crtc);
      for (int reg = 0; reg < 32; ++reg)
      {
         SCOPED_TRACE(reg);
         const unsigned char status = ReadRegister(crtc, kStatusPort, reg);
         switch (type)
         {
         case CRTC::HD6845S:
         case CRTC::MC6845:
            EXPECT_EQ(0xFF, status);
            break;
         case CRTC::UM6845R:
            EXPECT_EQ(0, status & 0x9F);
            break;
         default:
            EXPECT_EQ(ReadRegister(crtc, kReadPort, reg), status);
            break;
         }
      }
   }
}

// ---------------------------------------------------------------------------
// Group F: R0 = 0 on CRTC 0 (Compendium 13.2.3, 13.2.4, 13.2.6).
//
// C0 never reaches 1, so C9 is no longer handled: it stays frozen, and R4,
// R5 and R9 are ignored while R0 = 0. A C4 increment armed on the first
// C0 = 0 (C9 == R9) still happens once, on the second C0 = 0.
// R0 is written while C0 = 0, so C0 does not overflow (13.6). The character it
// is written on was started with the old R0 : the line end registered at its
// start is not seen, and the first C0 = 0 "for which R0 = 0" is the next one.
// ---------------------------------------------------------------------------

namespace
{
void Crtc0WithShortLines(CRTC& crtc, CSig& sig, unsigned char r4, unsigned char r9)
{
   MakeCrtc(crtc, sig, CRTC::HD6845S);
   WriteRegister(crtc, 0, 3);
   WriteRegister(crtc, 4, r4);
   WriteRegister(crtc, 5, 0);
   WriteRegister(crtc, 9, r9);
}
}  // namespace

// SAFETY NET.
TEST(CRTC_R0Zero, Crtc0KeepsR0ZeroAndC0StaysAtZero)
{
   CRTC crtc; CSig sig;
   Crtc0WithShortLines(crtc, sig, 38, 7);
   AdvanceUntilHccEquals(crtc, 0);
   WriteRegister(crtc, 0, 0);
   EXPECT_EQ(0, crtc.registers_list_[0]);
   for (int i = 0; i < 100; ++i)
   {
      Advance(crtc);
      ASSERT_EQ(0, crtc.hcc_) << "after " << i + 1 << " us";
   }
}

// 13.2.6, example 1: C0 = R0 = C4 = R4 = C9 = R9 = R5 = 0. SAFETY NET.
TEST(CRTC_R0Zero, Crtc0AdditionalManagementOnTheLastLine)
{
   CRTC crtc; CSig sig;
   Crtc0WithShortLines(crtc, sig, 0, 0);
   AdvanceMicroseconds(crtc, 128 * 4);   // let C4 wrap past its reset value
   AdvanceUntilHccEquals(crtc, 0);
   ASSERT_EQ(0, crtc.vcc_);
   ASSERT_EQ(0, crtc.vlc_);

   WriteRegister(crtc, 0, 0);
   Advance(crtc);               // first C0 = 0 for which R0 = 0 : last line, additional management
   EXPECT_EQ(0, crtc.vcc_);
   Advance(crtc);               // second C0 = 0 : C4 is incremented once
   EXPECT_EQ(1, crtc.vcc_);
   EXPECT_EQ(0, crtc.vlc_);
   AdvanceMicroseconds(crtc, 200);
   EXPECT_EQ(1, crtc.vcc_);
   EXPECT_EQ(0, crtc.vlc_);

   // R0 > 2 again : the additional management goes on, C9 + 1 != R5 so C9 counts
   WriteRegister(crtc, 0, 3);
   AdvanceUntilHccEquals(crtc, 0);
   EXPECT_EQ(1, crtc.vcc_);
   EXPECT_EQ(1, crtc.vlc_);
}

// 13.2.6, "dernier hoquet": C9 == R9 but C4 != R4. SAFETY NET.
TEST(CRTC_R0Zero, Crtc0LastC4Hiccup)
{
   CRTC crtc; CSig sig;
   Crtc0WithShortLines(crtc, sig, 20, 0);   // one line per character row
   do { AdvanceUntilHccEquals(crtc, 0); } while (crtc.vcc_ != 5);

   WriteRegister(crtc, 0, 0);
   AdvanceMicroseconds(crtc, 2);
   EXPECT_EQ(6, crtc.vcc_);
   EXPECT_EQ(0, crtc.vlc_);
   AdvanceMicroseconds(crtc, 200);
   EXPECT_EQ(6, crtc.vcc_);
   EXPECT_EQ(0, crtc.vlc_);

   // C9 was not reset and still equals R9 : the next line end is a plain C9 == R9 match
   WriteRegister(crtc, 0, 3);
   AdvanceUntilHccEquals(crtc, 0);
   EXPECT_EQ(7, crtc.vcc_);
   EXPECT_EQ(0, crtc.vlc_);
}

// 13.2.3, 13.2.4: C9 != R9, every counter is frozen and R9 is ignored. SAFETY NET.
TEST(CRTC_R0Zero, Crtc0FreezesC9AndIgnoresR9)
{
   CRTC crtc; CSig sig;
   Crtc0WithShortLines(crtc, sig, 38, 7);
   do { AdvanceUntilHccEquals(crtc, 0); } while (crtc.vlc_ != 3);
   const unsigned char c4 = crtc.vcc_;

   WriteRegister(crtc, 0, 0);
   AdvanceMicroseconds(crtc, 100);
   WriteRegister(crtc, 9, 3);   // C9 == R9 now, but C9 is not handled
   AdvanceMicroseconds(crtc, 100);
   EXPECT_EQ(c4, crtc.vcc_);
   EXPECT_EQ(3, crtc.vlc_);

   WriteRegister(crtc, 9, 7);
   WriteRegister(crtc, 0, 3);
   AdvanceUntilHccEquals(crtc, 0);
   EXPECT_EQ(c4, crtc.vcc_);
   EXPECT_EQ(4, crtc.vlc_);
}

// 15.3.2: R0 = 0, R2 = 0, R3l = 1 : CRTC 0 is protected, the HSYNC appears on
// the 1st C0 = 0, not on the 2nd, again on the 3rd. SAFETY NET.
TEST(CRTC_R0Zero, Crtc0HSyncIsProtected)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::HD6845S);
   WriteRegister(crtc, 3, 0x81);   // R3l = 1
   WriteRegister(crtc, 2, 0);
   WriteRegister(crtc, 0, 1);
   AdvanceUntilHccEquals(crtc, 0);
   WriteRegister(crtc, 0, 0);
   std::string trace;
   for (int i = 0; i < 12; ++i)
   {
      Advance(crtc);
      trace += sig.h_sync_ ? '1' : '0';
   }
   // The 1st C0 = 0 (HSYNC) is the one R0 is written on : the trace starts on the 2nd.
   EXPECT_EQ("010101010101", trace);
}

// ---------------------------------------------------------------------------
// Group G: SKEW-DISPTMG, R8 bits 5-4 (Compendium 19.2). CRTC 0, 3 and 4 only;
// CRTC 1 and 2 mask these bits (group A), so their DISPEN is never delayed.
// ---------------------------------------------------------------------------

namespace
{
// Puts the CRTC on C0 = 0 of a displayed line (C4 < R6) of a full frame
// (ff3_ is only set when a new frame starts after Reset()).
void ReachDisplayedLine(CRTC& crtc)
{
   AdvanceMicroseconds(crtc, 19968);
   do { AdvanceUntilHccEquals(crtc, 0); } while (crtc.vcc_ != 2);
}

// DISPEN for C0 = 0 .. 63 of one line ('1' = character, '0' = border).
std::string DispEnLine(CRTC& crtc)
{
   std::string line;
   for (int c0 = 0; c0 < 64; ++c0)
   {
      if (c0 > 0) Advance(crtc);
      line += crtc.DispEn() ? '1' : '0';
   }
   return line;
}

std::string Expected(int first, int last)
{
   std::string line;
   for (int c0 = 0; c0 < 64; ++c0) line += (c0 >= first && c0 <= last) ? '1' : '0';
   return line;
}
}  // namespace

// 19.2.1 / 19.2.3: DELAI +1 / +2 shift both border edges, 11 = BORDER ON. SAFETY NET.
TEST(CRTC_SkewDispTmg, DelaysTheBorderOnCrtc034)
{
   struct { unsigned char r8; int first; int last; } const cases[] = {
      { 0x00, 0, 39 }, { 0x10, 1, 40 }, { 0x20, 2, 41 }, { 0x30, -1, -1 },
   };
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::AMS40489, CRTC::AMS40226 })
   {
      for (const auto& c : cases)
      {
         SCOPED_TRACE(TypeName(type));
         SCOPED_TRACE(c.r8);
         CRTC crtc; CSig sig;
         MakeCrtc(crtc, sig, type);
         ProgramStandardEuropeanScreen(crtc);
         WriteRegister(crtc, 8, c.r8);
         ReachDisplayedLine(crtc);
         EXPECT_EQ(Expected(c.first, c.last), DispEnLine(crtc));
      }
   }
}

// SAFETY NET.
TEST(CRTC_SkewDispTmg, NoSkewOnCrtc12)
{
   for (CRTC::TypeCRTC type : { CRTC::UM6845R, CRTC::MC6845 })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      ProgramStandardEuropeanScreen(crtc);
      WriteRegister(crtc, 8, 0x30);
      ReachDisplayedLine(crtc);
      EXPECT_EQ(Expected(0, 39), DispEnLine(crtc));
   }
}

// 19.2.3: with R1 = R0, DELAI +1 moves the border byte to C0 = 0. SAFETY NET.
TEST(CRTC_SkewDispTmg, R1EqualsR0)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::HD6845S);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 1, 63);
   ReachDisplayedLine(crtc);
   EXPECT_EQ(Expected(0, 62), DispEnLine(crtc));
   WriteRegister(crtc, 8, 0x10);
   Advance(crtc);
   EXPECT_EQ(Expected(1, 63), DispEnLine(crtc));
}

// 19.2.5.2: R0 = R1 = 63, R8 = #10 before C0 = 63 then R8 = #00 on C0 = 0 :
// the change is immediate, both conditions are cancelled and the border byte
// between the two lines disappears. SAFETY NET.
TEST(CRTC_SkewDispTmg, Crtc0BorderDisintegration)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::HD6845S);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 1, 63);
   ReachDisplayedLine(crtc);
   AdvanceUntilHccEquals(crtc, 61);
   WriteRegister(crtc, 8, 0x10);
   std::string trace;
   for (int i = 0; i < 4; ++i)   // C0 = 62, 63, 0, 1
   {
      Advance(crtc);
      if (crtc.hcc_ == 0) WriteRegister(crtc, 8, 0x00);
      trace += crtc.DispEn() ? '1' : '0';
   }
   EXPECT_EQ("1111", trace);
}

// ---------------------------------------------------------------------------
// Group H: CRTC 0 vertical logic (Compendium 10.3.1, 11.2.2, 11.3.1, 12.2,
// 13.2, 13.7.2, 16.4.1). A register written after AdvanceUntilHccEquals(k)
// lands during the character C0 = k.
// ---------------------------------------------------------------------------

namespace
{
// Standard screen on CRTC 0, then the given R4 / R5 / R9, settled for 2 frames.
void Crtc0Screen(CRTC& crtc, CSig& sig, unsigned char r4, unsigned char r5, unsigned char r9)
{
   MakeCrtc(crtc, sig, CRTC::HD6845S);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 4, r4);
   WriteRegister(crtc, 5, r5);
   WriteRegister(crtc, 9, r9);
   AdvanceMicroseconds(crtc, 2 * 128 * 64);
}

// Puts the CRTC on C0 = 0 of the line C4 = c4, C9 = c9.
bool ReachLine(CRTC& crtc, int c4, int c9)
{
   for (int i = 0; i < 4 * 128 * 32; ++i)
   {
      AdvanceUntilHccEquals(crtc, 0);
      if (crtc.vcc_ == c4 && crtc.vlc_ == c9) return true;
   }
   return false;
}

// C4/C9 of the next line.
std::pair<int, int> NextLine(CRTC& crtc)
{
   AdvanceUntilHccEquals(crtc, 0);
   return { crtc.vcc_, crtc.vlc_ };
}

typedef std::pair<int, int> Line;
}  // namespace

// 11.2.2, example p82 : R4 = 10, R5 = 16, R9 = 3. C4 is incremented once
// (R4 + 1) and kept, C9 counts 0..15 instead of R9, then a new frame. SAFETY NET.
TEST(CRTC_Crtc0Vertical, AdjustmentCountsC9UpToR5)
{
   CRTC crtc; CSig sig;
   Crtc0Screen(crtc, sig, 10, 16, 3);
   ASSERT_TRUE(ReachLine(crtc, 10, 3));
   for (int c9 = 0; c9 < 16; ++c9)
      EXPECT_EQ(Line(11, c9), NextLine(crtc));
   EXPECT_EQ(Line(0, 0), NextLine(crtc));
}

// 11.2.2, 11.4.2 : R5 > 0 written on the last line is taken into account up to
// C0 = 2, not after. SAFETY NET.
TEST(CRTC_Crtc0Vertical, R5IsSampledUntilC0Equals2)
{
   {
      CRTC crtc; CSig sig;
      Crtc0Screen(crtc, sig, 10, 0, 3);
      ASSERT_TRUE(ReachLine(crtc, 10, 3));
      AdvanceUntilHccEquals(crtc, 2);
      WriteRegister(crtc, 5, 2);
      EXPECT_EQ(Line(11, 0), NextLine(crtc));
      EXPECT_EQ(Line(11, 1), NextLine(crtc));
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
   }
   {
      CRTC crtc; CSig sig;
      Crtc0Screen(crtc, sig, 10, 0, 3);
      ASSERT_TRUE(ReachLine(crtc, 10, 3));
      AdvanceUntilHccEquals(crtc, 3);
      WriteRegister(crtc, 5, 2);
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
   }
}

// 12.2 : on the last line, R4 written on C0 = 0 cancels the last line ; written
// on C0 = 1 it starts the additional management (the line becomes the first
// additional line : C4 kept, C9 compared with R5). SAFETY NET.
TEST(CRTC_Crtc0Vertical, R4WrittenOnC0Equals0Or1OfTheLastLine)
{
   {
      CRTC crtc; CSig sig;
      Crtc0Screen(crtc, sig, 10, 0, 3);
      ASSERT_TRUE(ReachLine(crtc, 10, 3));
      WriteRegister(crtc, 4, 20);
      EXPECT_EQ(Line(11, 0), NextLine(crtc));
   }
   {
      CRTC crtc; CSig sig;
      Crtc0Screen(crtc, sig, 10, 0, 3);
      ASSERT_TRUE(ReachLine(crtc, 10, 3));
      AdvanceUntilHccEquals(crtc, 1);
      WriteRegister(crtc, 4, 20);
      EXPECT_EQ(Line(10, 4), NextLine(crtc));
   }
   {
      // Written after C0 = 1 : the last line stays true
      CRTC crtc; CSig sig;
      Crtc0Screen(crtc, sig, 10, 0, 3);
      ASSERT_TRUE(ReachLine(crtc, 10, 3));
      AdvanceUntilHccEquals(crtc, 2);
      WriteRegister(crtc, 4, 20);
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
   }
}

// 10.3.1 : R9 written with C9 during the line resets C9 on the next line ; R9
// moved away from C9 == R9 lets C9 count on with C4 unchanged ; R9 written on
// C0 == R0 while C9 == R9 increments both C4 and C9. SAFETY NET.
TEST(CRTC_Crtc0Vertical, R9WrittenDuringTheLine)
{
   {
      CRTC crtc; CSig sig;
      Crtc0Screen(crtc, sig, 38, 0, 7);
      ASSERT_TRUE(ReachLine(crtc, 5, 3));
      AdvanceUntilHccEquals(crtc, 30);
      WriteRegister(crtc, 9, 3);
      EXPECT_EQ(Line(6, 0), NextLine(crtc));
   }
   {
      CRTC crtc; CSig sig;
      Crtc0Screen(crtc, sig, 38, 0, 0);
      ASSERT_TRUE(ReachLine(crtc, 5, 0));
      AdvanceUntilHccEquals(crtc, 30);
      WriteRegister(crtc, 9, 7);
      EXPECT_EQ(Line(5, 1), NextLine(crtc));
   }
   {
      CRTC crtc; CSig sig;
      Crtc0Screen(crtc, sig, 38, 0, 7);
      ASSERT_TRUE(ReachLine(crtc, 5, 7));
      AdvanceUntilHccEquals(crtc, 63);
      WriteRegister(crtc, 9, 3);
      EXPECT_EQ(Line(6, 8), NextLine(crtc));
   }
}

// 12.2.1 R.L.A.L. : R4 = R9 = 0 written on the last line after C0 = 1 makes
// every following line a last line (C4 = C9 = 0). SAFETY NET.
TEST(CRTC_Crtc0Vertical, LineToLineRupture)
{
   CRTC crtc; CSig sig;
   Crtc0Screen(crtc, sig, 38, 0, 7);
   ASSERT_TRUE(ReachLine(crtc, 38, 7));
   AdvanceUntilHccEquals(crtc, 10);
   WriteRegister(crtc, 9, 0);
   WriteRegister(crtc, 4, 0);
   for (int i = 0; i < 5; ++i)
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
}

// 13.2.5 : R0 = 1, R4 = R9 = R5 = 0. C0 never reaches 2, the additional
// management is never cancelled : each 2 us line (C4 = 0) is followed by an
// additional 2 us line (C4 = 1), the frame (and R12/R13) restarts every 4 us. SAFETY NET.
TEST(CRTC_Crtc0Vertical, R0EqualsOneAlternatesC4)
{
   CRTC crtc; CSig sig;
   Crtc0Screen(crtc, sig, 0, 0, 0);
   AdvanceUntilHccEquals(crtc, 0);
   WriteRegister(crtc, 0, 1);
   AdvanceMicroseconds(crtc, 8);
   std::string c4;
   for (int i = 0; i < 8; ++i)
   {
      const Line line = NextLine(crtc);
      EXPECT_EQ(0, line.second);
      c4 += char('0' + line.first);
   }
   EXPECT_TRUE(c4 == "01010101" || c4 == "10101010") << c4;
}

// 13.7.2.2 : R0 = 1 on a last line (C4 = R4, C9 = R9), then R0 enlarged on
// C0 = 1 : the line end was registered with R0 = 1, C4 is incremented at C0 = 2
// without C0 and C9 returning to 0, and the additional management stays active :
// C9 counts 1..31 with C4 = R4 + 1, then C4 = C9 = 0. SAFETY NET.
TEST(CRTC_Crtc0Vertical, R0EnlargedOnC0Equals1OfALastLine)
{
   CRTC crtc; CSig sig;
   Crtc0Screen(crtc, sig, 0, 0, 0);
   AdvanceUntilHccEquals(crtc, 0);
   WriteRegister(crtc, 0, 1);
   do { AdvanceUntilHccEquals(crtc, 0); } while (crtc.vcc_ != 0);
   AdvanceUntilHccEquals(crtc, 1);
   WriteRegister(crtc, 0, 63);
   Advance(crtc);
   EXPECT_EQ(2, crtc.hcc_);
   EXPECT_EQ(1, crtc.vcc_);
   EXPECT_EQ(0, crtc.vlc_);
   for (int c9 = 1; c9 < 32; ++c9)
      EXPECT_EQ(Line(1, c9), NextLine(crtc));
   EXPECT_EQ(Line(0, 0), NextLine(crtc));
}

// 13.2.2, 16.4.1.2 : C0 must reach 2 on the line before C4 = R7. With R0 = 1 on
// that line the VSYNC does not happen and stays blocked for this frame. SAFETY NET.
TEST(CRTC_Crtc0Vertical, VSyncFrozenByAShortLine)
{
   CRTC crtc; CSig sig;
   Crtc0Screen(crtc, sig, 38, 0, 7);
   ASSERT_TRUE(ReachLine(crtc, 29, 7));
   WriteRegister(crtc, 0, 1);
   AdvanceUntilHccEquals(crtc, 0);
   ASSERT_EQ(30, crtc.vcc_);
   WriteRegister(crtc, 0, 63);
   bool vsync = false;
   for (int i = 0; i < 64 * 40 && crtc.vcc_ != 0; ++i) { Advance(crtc); vsync |= sig.v_sync_; }
   EXPECT_FALSE(vsync);
   EXPECT_NE(-1, TicksUntilVSyncRisingEdge(crtc, sig, 19968));
}

// 16.4.1.1 : R7 written with C4 on C0 < 2 blocks the VSYNC ; written later it
// starts it at once, and the VSYNC lasts R3h lines + (R0 - C0) us. SAFETY NET.
TEST(CRTC_Crtc0Vertical, R7WrittenDuringALine)
{
   {
      CRTC crtc; CSig sig;
      Crtc0Screen(crtc, sig, 38, 0, 7);
      ASSERT_TRUE(ReachLine(crtc, 10, 0));
      AdvanceUntilHccEquals(crtc, 1);
      WriteRegister(crtc, 7, 10);
      bool vsync = false;
      for (int i = 0; i < 64 * 8; ++i) { Advance(crtc); vsync |= sig.v_sync_; }
      EXPECT_FALSE(vsync);
   }
   {
      CRTC crtc; CSig sig;
      Crtc0Screen(crtc, sig, 38, 0, 7);   // R3h = 8
      ASSERT_TRUE(ReachLine(crtc, 10, 0));
      AdvanceUntilHccEquals(crtc, 5);
      WriteRegister(crtc, 7, 10);
      Advance(crtc);
      EXPECT_TRUE(sig.v_sync_);
      int length = 1;
      while (length < 2000) { Advance(crtc); if (!sig.v_sync_) break; ++length; }
      EXPECT_EQ(8 * 64 + (63 - 5), length);
   }
}

// 16.4.1.2 : R0 set to 0 on C0 = 0 of the first VSYNC line : the VSYNC starts
// but C3h is frozen, it does not end with R3h = 1. SAFETY NET.
TEST(CRTC_Crtc0Vertical, VSyncCounterFrozenWithR0Zero)
{
   CRTC crtc; CSig sig;
   Crtc0Screen(crtc, sig, 38, 0, 7);
   WriteRegister(crtc, 3, 0x1E);   // R3h = 1
   AdvanceMicroseconds(crtc, 19968);
   while (!sig.v_sync_) Advance(crtc);
   ASSERT_EQ(0, crtc.hcc_);
   WriteRegister(crtc, 0, 0);
   AdvanceMicroseconds(crtc, 1000);
   EXPECT_TRUE(sig.v_sync_);
}

// ---------------------------------------------------------------------------
// Group I: CRTC 3 and 4 vertical logic (Compendium 10.3.4, 11.2.6, 11.3.3,
// 12.5, 16.4.4, 18.2.4).
// ---------------------------------------------------------------------------

namespace
{
const CRTC::TypeCRTC kAsicTypes[] = { CRTC::AMS40489, CRTC::AMS40226 };

void AsicScreen(CRTC& crtc, CSig& sig, CRTC::TypeCRTC type, unsigned char r4, unsigned char r5, unsigned char r9)
{
   MakeCrtc(crtc, sig, type);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 4, r4);
   WriteRegister(crtc, 5, r5);
   WriteRegister(crtc, 9, r9);
   AdvanceMicroseconds(crtc, 2 * 128 * 64);
}
}  // namespace

// 10.3.4 : C9 >= R9 ends the character, C9 cannot overflow. SAFETY NET.
TEST(CRTC_AsicVertical, C9CannotOverflow)
{
   for (CRTC::TypeCRTC type : kAsicTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      AsicScreen(crtc, sig, type, 38, 0, 7);
      ASSERT_TRUE(ReachLine(crtc, 5, 4));
      AdvanceUntilHccEquals(crtc, 20);
      WriteRegister(crtc, 9, 1);
      EXPECT_EQ(Line(6, 0), NextLine(crtc));
      EXPECT_EQ(Line(6, 1), NextLine(crtc));
      EXPECT_EQ(Line(7, 0), NextLine(crtc));
   }
}

// 11.2.6, example p82 : C4 stays at R4 (10), C9 counts 0..15, then a new frame. SAFETY NET.
TEST(CRTC_AsicVertical, AdjustmentKeepsC4AtR4)
{
   for (CRTC::TypeCRTC type : kAsicTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      AsicScreen(crtc, sig, type, 10, 16, 3);
      ASSERT_TRUE(ReachLine(crtc, 10, 3));
      for (int c9 = 0; c9 < 16; ++c9)
         EXPECT_EQ(Line(10, c9), NextLine(crtc));
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
   }
}

// 11.3.3 : R5 written below C9 + 1 during the adjustment ends it at once. SAFETY NET.
TEST(CRTC_AsicVertical, R5LoweredEndsTheAdjustment)
{
   for (CRTC::TypeCRTC type : kAsicTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      AsicScreen(crtc, sig, type, 10, 16, 3);
      ASSERT_TRUE(ReachLine(crtc, 10, 3));
      EXPECT_EQ(Line(10, 0), NextLine(crtc));
      EXPECT_EQ(Line(10, 1), NextLine(crtc));
      EXPECT_EQ(Line(10, 2), NextLine(crtc));
      AdvanceUntilHccEquals(crtc, 30);
      WriteRegister(crtc, 5, 1);
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
   }
}

// 12.5 : R4 lower than C4 makes C4 overflow (unlike C9). SAFETY NET.
TEST(CRTC_AsicVertical, C4Overflows)
{
   for (CRTC::TypeCRTC type : kAsicTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      AsicScreen(crtc, sig, type, 38, 0, 0);
      ASSERT_TRUE(ReachLine(crtc, 10, 0));
      WriteRegister(crtc, 4, 5);
      EXPECT_EQ(Line(11, 0), NextLine(crtc));
   }
}

// 16.4.4 : R7 written with C4 during a character does not start a VSYNC. SAFETY NET.
TEST(CRTC_AsicVertical, R7WrittenWithC4StartsNoVSync)
{
   for (CRTC::TypeCRTC type : kAsicTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      AsicScreen(crtc, sig, type, 38, 0, 7);
      ASSERT_TRUE(ReachLine(crtc, 10, 0));
      AdvanceUntilHccEquals(crtc, 5);
      WriteRegister(crtc, 7, 10);
      bool vsync = false;
      for (int i = 0; i < 7 * 64; ++i) { Advance(crtc); vsync |= sig.v_sync_; }
      EXPECT_FALSE(vsync);
   }
}

// 16.3, 16.4.4 : no re-entrance protection. With R4 = R7 = 0 and R9 = 0 (every
// line is C4 = C9 = 0), the VSYNC starts again as soon as it ends : infinite
// VSYNC. CRTC 0 protects it : a single VSYNC. SAFETY NET.
TEST(CRTC_AsicVertical, InfiniteVSyncWithoutProtection)
{
   for (CRTC::TypeCRTC type : { CRTC::AMS40489, CRTC::AMS40226, CRTC::HD6845S })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      ProgramStandardEuropeanScreen(crtc);
      WriteRegister(crtc, 7, 0);
      WriteRegister(crtc, 4, 0);
      WriteRegister(crtc, 9, 0);
      AdvanceMicroseconds(crtc, 2 * 128 * 64);
      int high = 0;
      for (int i = 0; i < 64 * 64; ++i) { Advance(crtc); if (sig.v_sync_) ++high; }
      if (type == CRTC::HD6845S)
         EXPECT_EQ(0, high);
      else
         EXPECT_EQ(64 * 64, high);
   }
}

// 18.2.4 : R6 is only tested when C0 restarts at 0. SAFETY NET.
TEST(CRTC_AsicVertical, R6TestedAtTheLineStartOnly)
{
   for (CRTC::TypeCRTC type : kAsicTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      AsicScreen(crtc, sig, type, 38, 0, 7);
      ASSERT_TRUE(ReachLine(crtc, 5, 2));
      AdvanceUntilHccEquals(crtc, 10);
      WriteRegister(crtc, 6, 5);
      AdvanceUntilHccEquals(crtc, 63);
      EXPECT_TRUE(crtc.ff3_) << "R6 written during the line must not set the border";
      Advance(crtc);
      EXPECT_FALSE(crtc.ff3_) << "border expected when C0 restarts at 0 with C4 == R6";
   }
}

// ---------------------------------------------------------------------------
// Group J: CRTC 1 (Compendium 10.3.2, 11.2.4, 11.3.2, 11.6, 12.3, 16.3,
// 17.4.2, 18.3.3, 21.3.3).
// ---------------------------------------------------------------------------

namespace
{
void Crtc1Screen(CRTC& crtc, CSig& sig, unsigned char r4, unsigned char r5, unsigned char r9)
{
   MakeCrtc(crtc, sig, CRTC::UM6845R);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 4, r4);
   WriteRegister(crtc, 5, r5);
   WriteRegister(crtc, 9, r9);
   AdvanceMicroseconds(crtc, 2 * 128 * 64);
}

const unsigned short kOffset = 0x1234 & 0x3FFF;

void SetOffset(CRTC& crtc)
{
   WriteRegister(crtc, 12, kOffset >> 8);
   WriteRegister(crtc, 13, kOffset & 0xFF);
}
}  // namespace

// 10.3.2 : pure logic, R9 is compared at the line end. SAFETY NET.
TEST(CRTC_Crtc1, R9WrittenDuringTheLine)
{
   struct { int c9; unsigned char r9_before; unsigned char r9_after; Line next; } const cases[] = {
      { 3, 7, 3, Line(6, 0) },   // R9 = C9 : C9 returns to 0, C4 is incremented
      { 4, 7, 1, Line(5, 5) },   // R9 < C9 : C9 overflows, C4 unchanged
      { 0, 0, 7, Line(5, 1) },   // R9 > C9 : C9 + 1, C4 unchanged
   };
   for (const auto& c : cases)
   {
      CRTC crtc; CSig sig;
      Crtc1Screen(crtc, sig, 38, 0, c.r9_before);
      ASSERT_TRUE(ReachLine(crtc, 5, c.c9));
      AdvanceUntilHccEquals(crtc, 30);
      WriteRegister(crtc, 9, c.r9_after);
      EXPECT_EQ(c.next, NextLine(crtc));
   }
}

// 11.2.1 / 11.2.3, example p82 : C5 counts the 16 lines, C9 keeps counting with
// R9 and increments C4 whatever R4 : C4 = 11..14. SAFETY NET.
TEST(CRTC_Crtc1, AdjustmentUsesC5)
{
   CRTC crtc; CSig sig;
   Crtc1Screen(crtc, sig, 10, 16, 3);
   ASSERT_TRUE(ReachLine(crtc, 10, 3));
   for (int line = 0; line < 16; ++line)
      EXPECT_EQ(Line(11 + line / 4, line % 4), NextLine(crtc));
   EXPECT_EQ(Line(0, 0), NextLine(crtc));
}

// 12.3 : R4 = 0 written on the last line makes C4 overflow ; written when C0
// reached R0 the frame end is already decided (11.2.4). SAFETY NET.
TEST(CRTC_Crtc1, R4WrittenOnTheLastLine)
{
   {
      CRTC crtc; CSig sig;
      Crtc1Screen(crtc, sig, 38, 0, 7);
      ASSERT_TRUE(ReachLine(crtc, 38, 7));
      AdvanceUntilHccEquals(crtc, 30);
      WriteRegister(crtc, 4, 0);
      EXPECT_EQ(Line(39, 0), NextLine(crtc));
   }
   {
      CRTC crtc; CSig sig;
      Crtc1Screen(crtc, sig, 38, 0, 7);
      ASSERT_TRUE(ReachLine(crtc, 38, 7));
      AdvanceUntilHccEquals(crtc, 63);
      WriteRegister(crtc, 4, 0);
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
   }
}

// 11.3.2 : R5 set to 0 during the adjustment does not end it : C4 does not
// return to 0 and C5 loops. SAFETY NET.
TEST(CRTC_Crtc1, R5ZeroDuringTheAdjustment)
{
   CRTC crtc; CSig sig;
   Crtc1Screen(crtc, sig, 10, 4, 3);
   ASSERT_TRUE(ReachLine(crtc, 10, 3));
   EXPECT_EQ(Line(11, 0), NextLine(crtc));
   WriteRegister(crtc, 5, 0);
   for (int line = 1; line < 40; ++line)
      ASSERT_NE(Line(0, 0), NextLine(crtc)) << "line " << line;
}

// 17.4.2 : VMA is loaded from R12/R13 at each line start while C4 = 0, then
// from VMA'. SAFETY NET.
TEST(CRTC_Crtc1, OffsetOnEachLineOfTheFirstCharacter)
{
   CRTC crtc; CSig sig;
   Crtc1Screen(crtc, sig, 38, 0, 7);
   SetOffset(crtc);
   ASSERT_TRUE(ReachLine(crtc, 0, 0));
   EXPECT_EQ(kOffset, crtc.ma_);
   NextLine(crtc);
   WriteRegister(crtc, 13, 0x80);   // taken on the next line of C4 = 0
   NextLine(crtc);
   EXPECT_EQ((kOffset & 0x3F00) | 0x80, crtc.ma_);
   SetOffset(crtc);
   ASSERT_TRUE(ReachLine(crtc, 1, 0));
   EXPECT_EQ(kOffset + 40, crtc.ma_);   // VMA' = VMA when C0 reached R1 on C9 = R9
}

// 11.6 R.F.D. : R5 0 -> 1 -> 0 on C0 = R0 of a line C9 != R9 : the next line
// starts from R12/R13 whatever C4. SAFETY NET.
TEST(CRTC_Crtc1, RuptureForDummies)
{
   CRTC crtc; CSig sig;
   Crtc1Screen(crtc, sig, 38, 0, 7);
   SetOffset(crtc);
   ASSERT_TRUE(ReachLine(crtc, 5, 3));
   const unsigned short vma_prime = crtc.bu_;
   AdvanceUntilHccEquals(crtc, 63);
   WriteRegister(crtc, 5, 1);
   WriteRegister(crtc, 5, 0);
   NextLine(crtc);
   EXPECT_EQ(kOffset, crtc.ma_);
   EXPECT_NE(vma_prime, crtc.ma_);
}

// 18.3.3 : R6 = 0 forces the border as long as it stays 0 ; written while
// C4 = R6 = 0 the border lasts until the next frame. SAFETY NET.
TEST(CRTC_Crtc1, R6ZeroBorder)
{
   {
      CRTC crtc; CSig sig;
      Crtc1Screen(crtc, sig, 38, 0, 7);
      ASSERT_TRUE(ReachLine(crtc, 10, 2));
      AdvanceUntilHccEquals(crtc, 5);
      WriteRegister(crtc, 6, 0);
      Advance(crtc);
      EXPECT_FALSE(crtc.DispEn());
      WriteRegister(crtc, 6, 25);
      Advance(crtc);
      EXPECT_TRUE(crtc.DispEn());
   }
   {
      CRTC crtc; CSig sig;
      Crtc1Screen(crtc, sig, 38, 0, 7);
      ASSERT_TRUE(ReachLine(crtc, 0, 1));
      AdvanceUntilHccEquals(crtc, 5);
      WriteRegister(crtc, 6, 0);
      Advance(crtc);
      WriteRegister(crtc, 6, 25);
      ASSERT_TRUE(ReachLine(crtc, 3, 0));
      Advance(crtc);
      EXPECT_FALSE(crtc.DispEn());
   }
}

// 21.3.3 : &BE00 bit 5 is the BORDER R6 state, updated at the line end. SAFETY NET.
TEST(CRTC_Crtc1, StatusBorderR6)
{
   CRTC crtc; CSig sig;
   Crtc1Screen(crtc, sig, 38, 0, 7);
   ASSERT_TRUE(ReachLine(crtc, 24, 7));
   EXPECT_EQ(0, crtc.In(0xBE00) & 0x20);
   NextLine(crtc);   // C4 = R6 = 25, C9 = 0
   EXPECT_EQ(0x20, crtc.In(0xBE00) & 0x20);
   ASSERT_TRUE(ReachLine(crtc, 0, 0));
   EXPECT_EQ(0, crtc.In(0xBE00) & 0x20);
}

// 16.3 : once a VSYNC happened for C4 == R7, writing R7 again with the same
// value does not start a new one : the equality did not change. SAFETY NET.
TEST(CRTC_VerticalSync, RewritingR7DoesNotRestartTheVSync)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::UM6845R, CRTC::MC6845 })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      ProgramStandardEuropeanScreen(crtc);
      WriteRegister(crtc, 9, 31);           // 32 lines per character : C4 stays at R7 after the VSYNC
      WriteRegister(crtc, 4, 9);
      WriteRegister(crtc, 6, 8);
      WriteRegister(crtc, 7, 5);
      AdvanceMicroseconds(crtc, 2 * 10 * 32 * 64);
      ASSERT_TRUE(ReachLine(crtc, 5, 20));
      ASSERT_FALSE(sig.v_sync_);
      AdvanceUntilHccEquals(crtc, 10);
      WriteRegister(crtc, 7, 5);
      bool vsync = false;
      for (int i = 0; i < 64 * 4; ++i) { Advance(crtc); vsync |= sig.v_sync_; }
      EXPECT_FALSE(vsync);
   }
}

// ===========================================================================
// Suite CRTC_Compendium : behaviours described by the Compendium that the
// emulation does not implement yet. Each test states the expected behaviour ;
// those that fail today are DISABLED_ and form the to-do list :
//   unitTests --gtest_also_run_disabled_tests --gtest_filter='CRTC_Compendium.*'
// When a behaviour is implemented, remove the DISABLED_ prefix of its test.
// ===========================================================================

namespace
{
// The C9 the GATE ARRAY uses to build the address (C9.VMA in Interlace Video Mode).
int AddressC9(const CRTC& crtc)
{
   return crtc.AddressC9() & 0x07;
}

void Screen(CRTC& crtc, CSig& sig, CRTC::TypeCRTC type)
{
   MakeCrtc(crtc, sig, type);
   ProgramStandardEuropeanScreen(crtc);
   AdvanceMicroseconds(crtc, 2 * 19968);
}

// Number of lines of the next complete frame (C4 = C9 = 0 to C4 = C9 = 0).
int NextFrameLines(CRTC& crtc)
{
   // Consecutive calls measure consecutive frames : do not skip a frame start we are already on
   const bool on_frame_start = (crtc.hcc_ == 0 && crtc.vcc_ == 0 && crtc.vlc_ == 0);
   if (!on_frame_start && !ReachLine(crtc, 0, 0)) return -1;
   int lines = 0;
   do { NextLine(crtc); ++lines; } while (!(crtc.vcc_ == 0 && crtc.vlc_ == 0) && lines < 1000);
   return lines;
}

// Address C9 values seen at the line starts of the character row C4 = c4.
std::string RowAddressC9s(CRTC& crtc, int c4)
{
   for (int i = 0; i < 400 && crtc.vcc_ != c4; ++i) AdvanceUntilHccEquals(crtc, 0);
   std::string s;
   while (crtc.vcc_ == c4 && s.size() < 32) { s += char('0' + AddressC9(crtc)); AdvanceUntilHccEquals(crtc, 0); }
   return s;
}

bool VSyncSeen(CRTC& crtc, CSig& sig, int us)
{
   bool seen = false;
   for (int i = 0; i < us; ++i) { Advance(crtc); seen |= sig.v_sync_; }
   return seen;
}

unsigned char ReadStatus(CRTC& crtc, unsigned char reg)
{
   crtc.Out(kSelectRegister, reg);
   return crtc.In(0xBF00);
}
}  // namespace

// --- CRTC 2 ----------------------------------------------------------------

// 11.2.3 : like CRTC 1, a dedicated C5 counts the R5 lines, C9 keeps counting
// with R9 and increments C4 whatever R4.
TEST(CRTC_Compendium, Crtc2AdjustmentUsesC5)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::MC6845);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 4, 10);
   WriteRegister(crtc, 5, 16);
   WriteRegister(crtc, 9, 3);
   AdvanceMicroseconds(crtc, 2 * 128 * 64);
   ASSERT_TRUE(ReachLine(crtc, 10, 3));
   for (int line = 0; line < 16; ++line)
      EXPECT_EQ(Line(11 + line / 4, line % 4), NextLine(crtc));
   EXPECT_EQ(Line(0, 0), NextLine(crtc));
}

// 15.4.4 : a VSYNC condition evaluated during the HSYNC (from C0 = R2 to the
// character after R3l + 1) starts a GHOST VSYNC : the pin is not raised. With
// R0 = 63, R2 = 50 : R3l = 14 reaches C0 = 0 of the C4 = R7 line, no VSYNC ;
// R3l = 13 does not, the VSYNC happens. CRTC 0 and 1 are not concerned.
TEST(CRTC_Compendium, Crtc2GhostVSync)
{
   struct { CRTC::TypeCRTC type; unsigned char r3; bool vsync; } const cases[] = {
      { CRTC::MC6845, 0x8E, false }, { CRTC::MC6845, 0x8D, true },
      { CRTC::HD6845S, 0x8E, true }, { CRTC::UM6845R, 0x8E, true },
   };
   for (const auto& c : cases)
   {
      SCOPED_TRACE(TypeName(c.type));
      SCOPED_TRACE(c.r3);
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, c.type);
      ProgramStandardEuropeanScreen(crtc);
      WriteRegister(crtc, 2, 50);
      WriteRegister(crtc, 3, c.r3);
      AdvanceMicroseconds(crtc, 19968);
      EXPECT_EQ(c.vsync, VSyncSeen(crtc, sig, 2 * 19968));
   }
}

// 15.4.4 : with R2 = 0 the HSYNC starts on C0 = 0, but a VSYNC condition met
// on C0 = 0 is processed first : normal VSYNC. (With R2 = 0 the last line is
// never reached on CRTC 2, 15.6 : C4 = R7 is met here by the counting.)
TEST(CRTC_Compendium, Crtc2NoGhostVSyncWithR2Zero)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::MC6845);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 2, 0);
   WriteRegister(crtc, 7, 100);
   AdvanceMicroseconds(crtc, 2 * 19968);
   ASSERT_TRUE(ReachLine(crtc, 10, 7));
   AdvanceUntilHccEquals(crtc, 30);
   WriteRegister(crtc, 7, 11);
   AdvanceUntilHccEquals(crtc, 0);
   ASSERT_EQ(11, crtc.vcc_);
   EXPECT_TRUE(sig.v_sync_);
}

// 15.5 : CRTC 2 does not lift the border on C0 = 0 during a HSYNC : with a
// HSYNC spanning C0 = 0 (R2 = 62, R3l = 6) the displayed lines stay border.
TEST(CRTC_Compendium, Crtc2BorderNotLiftedDuringHSync)
{
   struct { CRTC::TypeCRTC type; bool displayed; } const cases[] = {
      { CRTC::MC6845, false }, { CRTC::HD6845S, true }, { CRTC::UM6845R, true },
   };
   for (const auto& c : cases)
   {
      SCOPED_TRACE(TypeName(c.type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, c.type);
      ProgramStandardEuropeanScreen(crtc);
      WriteRegister(crtc, 2, 62);
      WriteRegister(crtc, 3, 0x86);
      AdvanceMicroseconds(crtc, 2 * 19968);
      ASSERT_TRUE(ReachLine(crtc, 10, 3));
      EXPECT_EQ(c.displayed, DispEnLine(crtc).find('1') != std::string::npos);
   }
}

// 17.4.3, 20 : CRTC 2 loads VMA' with R12/R13 when C0 reaches R1 on the last
// line of the frame : R12/R13 written after that are too late for the next
// frame. CRTC 0 loads R12/R13 when the frame starts.
TEST(CRTC_Compendium, Crtc2OffsetTakenAtR1OfTheLastLine)
{
   struct { CRTC::TypeCRTC type; unsigned short expected; } const cases[] = {
      { CRTC::MC6845, 0x0100 }, { CRTC::HD6845S, 0x0200 },
   };
   for (const auto& c : cases)
   {
      SCOPED_TRACE(TypeName(c.type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, c.type);
      WriteRegister(crtc, 12, 0x01); WriteRegister(crtc, 13, 0x00);
      ASSERT_TRUE(ReachLine(crtc, 38, 7));
      AdvanceUntilHccEquals(crtc, 50);
      WriteRegister(crtc, 12, 0x02);
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
      EXPECT_EQ(c.expected, crtc.ma_);
   }
}

// 12.4.1, 15.6 : on CRTC 2 a HSYNC starting on C0 = 0 prevents the last line
// state : C4 is incremented instead of returning to 0.
TEST(CRTC_Compendium, Crtc2HSyncOnC0ZeroCancelsTheLastLine)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::MC6845);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 2, 0);
   AdvanceMicroseconds(crtc, 2 * 19968);
   ASSERT_TRUE(ReachLine(crtc, 38, 7));
   EXPECT_EQ(Line(39, 0), NextLine(crtc));
}

// 12.4.1 : a line that follows a last line (same C4/C9 equality on the last
// HSYNC character) cannot be a last line : with R4 = R9 = 0 written on the last
// line, C4 = C9 = 0 twice, then C4 is incremented. CRTC 0 keeps C4 = C9 = 0.
TEST(CRTC_Compendium, Crtc2PreviousLastLine)
{
   struct { CRTC::TypeCRTC type; Line third; } const cases[] = {
      { CRTC::MC6845, Line(1, 0) }, { CRTC::HD6845S, Line(0, 0) },
   };
   for (const auto& c : cases)
   {
      SCOPED_TRACE(TypeName(c.type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, c.type);
      ASSERT_TRUE(ReachLine(crtc, 38, 7));
      AdvanceUntilHccEquals(crtc, 10);
      WriteRegister(crtc, 4, 0);
      WriteRegister(crtc, 9, 0);
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
      EXPECT_EQ(c.third, NextLine(crtc));
   }
}

// 12.4.2 R.L.A.L. on CRTC 2 : R2 = 1, R3 = 6, R4 = R9 = 0 ; on each line R9 = 1
// during the HSYNC then R9 = 0 after it keeps every line at C4 = C9 = 0.
TEST(CRTC_Compendium, Crtc2LineToLineRupture)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::MC6845);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 2, 1);
   WriteRegister(crtc, 3, 0x86);
   AdvanceMicroseconds(crtc, 2 * 19968);
   ASSERT_TRUE(ReachLine(crtc, 38, 7));
   AdvanceUntilHccEquals(crtc, 10);
   WriteRegister(crtc, 4, 0);
   WriteRegister(crtc, 9, 0);
   for (int i = 0; i < 5; ++i)
   {
      EXPECT_EQ(Line(0, 0), NextLine(crtc));
      AdvanceUntilHccEquals(crtc, 3);
      WriteRegister(crtc, 9, 1);
      AdvanceUntilHccEquals(crtc, 7);
      WriteRegister(crtc, 9, 0);
   }
}

// 18.2.2 : R6 written with C4 sets the border at once and until the next frame.
TEST(CRTC_Compendium, Crtc02R6BorderIsDefinitive)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::MC6845 })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, type);
      ASSERT_TRUE(ReachLine(crtc, 10, 2));
      AdvanceUntilHccEquals(crtc, 5);
      WriteRegister(crtc, 6, 10);
      Advance(crtc);
      EXPECT_FALSE(crtc.DispEn());
      WriteRegister(crtc, 6, 25);
      ASSERT_TRUE(ReachLine(crtc, 12, 0));
      EXPECT_EQ(std::string::npos, DispEnLine(crtc).find('1'));
   }
}

// --- CRTC 1 ----------------------------------------------------------------

// 11.6.1 : after a R.F.D. the C9 == R9 test on C0 == R1 takes the frame parity
// into account : on one frame out of two VMA' is not updated and VMA keeps
// being loaded from R12/R13 (the next character row starts from R12/R13).
TEST(CRTC_Compendium, Crtc1RfdParity)
{
   CRTC crtc; CSig sig;
   Screen(crtc, sig, CRTC::UM6845R);
   WriteRegister(crtc, 12, 0x12); WriteRegister(crtc, 13, 0x34);
   int repeated = 0;
   for (int frame = 0; frame < 2; ++frame)
   {
      ASSERT_TRUE(ReachLine(crtc, 5, 3));
      AdvanceUntilHccEquals(crtc, 63);
      WriteRegister(crtc, 5, 1);
      WriteRegister(crtc, 5, 0);
      ASSERT_TRUE(ReachLine(crtc, 6, 0));
      if (crtc.ma_ == 0x1234) ++repeated;
   }
   EXPECT_EQ(1, repeated);
}

// 11.6.2 : IVM ON/OFF (R8 = 3 then 0 on an even C9, R9 odd) before the R.F.D.
// fixes the (even) parity of the frame : done on each frame, every frame keeps
// loading VMA from R12/R13.
TEST(CRTC_Compendium, Crtc1IvmOnOffFixesTheParity)
{
   CRTC crtc; CSig sig;
   Screen(crtc, sig, CRTC::UM6845R);
   WriteRegister(crtc, 12, 0x12); WriteRegister(crtc, 13, 0x34);
   int repeated = 0;
   for (int frame = 0; frame < 2; ++frame)
   {
      // The parity toggles on each frame : as in the 11.6.3 recipe, IVM ON/OFF is done on each frame
      ASSERT_TRUE(ReachLine(crtc, 2, 2));
      AdvanceUntilHccEquals(crtc, 10);
      WriteRegister(crtc, 8, 3);
      WriteRegister(crtc, 8, 0);
      ASSERT_TRUE(ReachLine(crtc, 5, 3));
      AdvanceUntilHccEquals(crtc, 63);
      WriteRegister(crtc, 5, 1);
      WriteRegister(crtc, 5, 0);
      ASSERT_TRUE(ReachLine(crtc, 6, 0));
      if (crtc.ma_ == 0x1234) ++repeated;
   }
   EXPECT_EQ(2, repeated);
}

// 16.4.2 : R7 written with C4 during a line starts the VSYNC at once, counted
// as started on C0 = 0 : it ends at the end of the 16th line.
TEST(CRTC_Compendium, Crtc1VSyncStartedDuringALine)
{
   CRTC crtc; CSig sig;
   Screen(crtc, sig, CRTC::UM6845R);
   ASSERT_TRUE(ReachLine(crtc, 10, 0));
   AdvanceUntilHccEquals(crtc, 5);
   WriteRegister(crtc, 7, 10);
   Advance(crtc);
   ASSERT_TRUE(sig.v_sync_);
   int length = 1;
   while (length < 2000) { Advance(crtc); if (!sig.v_sync_) break; ++length; }
   EXPECT_EQ(16 * 64 - 6, length);
}

// --- CRTC 3, 4 status registers (21.3.4) ------------------------------------

// STATUS 1 : bit 0 = 1 on C0 = R0 ; bit 1 = 0 on C0 = R0/2 ; bit 2 = 0 on
// C0 = R1 - 1 ; bit 3 = 0 on C0 = R2 ; bit 6 always 1.
TEST(CRTC_Compendium, AsicStatus1)
{
   for (CRTC::TypeCRTC type : kAsicTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, type);
      ASSERT_TRUE(ReachLine(crtc, 5, 2));
      for (int c0 = 1; c0 < 64; ++c0)
      {
         Advance(crtc);
         SCOPED_TRACE(c0);
         const unsigned char s = ReadStatus(crtc, 10);
         EXPECT_EQ(c0 == 63, (s & 0x01) != 0);
         EXPECT_EQ(c0 == 31, (s & 0x02) == 0);
         EXPECT_EQ(c0 == 39, (s & 0x04) == 0);
         EXPECT_EQ(c0 == 46, (s & 0x08) == 0);
         EXPECT_NE(0, s & 0x40);
      }
   }
}

// STATUS 2 : bit 0 = 0 on C4 = R4, C9 = R9, C0 = R0 ; bit 4 always 1 ; bit 5 =
// 0 while C9 = R9 ; bit 6 always 0.
TEST(CRTC_Compendium, AsicStatus2)
{
   for (CRTC::TypeCRTC type : kAsicTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, type);
      ASSERT_TRUE(ReachLine(crtc, 10, 2));
      AdvanceUntilHccEquals(crtc, 5);
      unsigned char s = ReadStatus(crtc, 11);
      EXPECT_NE(0, s & 0x01);
      EXPECT_NE(0, s & 0x10);
      EXPECT_NE(0, s & 0x20);
      EXPECT_EQ(0, s & 0x40);
      ASSERT_TRUE(ReachLine(crtc, 38, 7));
      AdvanceUntilHccEquals(crtc, 63);
      s = ReadStatus(crtc, 11);
      EXPECT_EQ(0, s & 0x01);
      EXPECT_EQ(0, s & 0x20);
      EXPECT_NE(0, s & 0x10);
      EXPECT_EQ(0, s & 0x40);
   }
}

// --- All CRTC ----------------------------------------------------------------

// 17.1 : DISPEN is enabled when C0 restarts at 0 after C0 = R0, not when C0
// overflows from 255 to 0 (at least on CRTC 0).
TEST(CRTC_Compendium, Crtc0NoDisplayAfterC0Overflow)
{
   CRTC crtc; CSig sig;
   Screen(crtc, sig, CRTC::HD6845S);
   ASSERT_TRUE(ReachLine(crtc, 5, 2));
   AdvanceUntilHccEquals(crtc, 50);
   WriteRegister(crtc, 0, 20);           // C0 = 50 > R0 : counts up to 255
   while (crtc.hcc_ != 255) Advance(crtc);
   Advance(crtc);
   ASSERT_EQ(0, crtc.hcc_);
   EXPECT_FALSE(crtc.DispEn());
}

// 17.2 : R1 > R0 : C0 never reaches R1, VMA' is never updated, every character
// row repeats the R12/R13 address (CRTC 0, 3, 4).
TEST(CRTC_Compendium, R1GreaterThanR0RepeatsTheRows)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::AMS40489, CRTC::AMS40226 })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      ProgramStandardEuropeanScreen(crtc);
      WriteRegister(crtc, 1, 70);
      WriteRegister(crtc, 12, 0x12); WriteRegister(crtc, 13, 0x34);
      AdvanceMicroseconds(crtc, 2 * 19968);
      ASSERT_TRUE(ReachLine(crtc, 5, 0));
      EXPECT_EQ(0x1234, crtc.ma_);
      ASSERT_TRUE(ReachLine(crtc, 6, 0));
      EXPECT_EQ(0x1234, crtc.ma_);
   }
}

// 19.3.1, 19.6 : Interlace Sync (R8 = 1) adds one line at the end of every even
// frame : frames of 312 and 313 lines alternate (R6 < R4, the parity runs).
TEST(CRTC_Compendium, InterlaceAddsALineEveryOtherFrame)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, type);
      WriteRegister(crtc, 8, 1);
      const int a = NextFrameLines(crtc);
      const int b = NextFrameLines(crtc);
      EXPECT_EQ(312 + 313, a + b) << a << " / " << b;
   }
}

// 19.6.1, 19.6.3 : on CRTC 0 and 2 the additional line depends on ParitéR6,
// toggled when C4 reaches R6 : with R6 > R4 the parity is frozen and every
// frame has the same length. CRTC 1, 3, 4 use ParitéFrame and keep alternating.
TEST(CRTC_Compendium, InterlaceParityFrozenByR6)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, type);
      WriteRegister(crtc, 8, 1);
      WriteRegister(crtc, 6, 50);
      NextFrameLines(crtc);
      const int a = NextFrameLines(crtc);
      const int b = NextFrameLines(crtc);
      if (type == CRTC::HD6845S || type == CRTC::MC6845)
         EXPECT_EQ(a, b);
      else
         EXPECT_EQ(312 + 313, a + b) << a << " / " << b;
   }
}

// 19.6.4 : on CRTC 3 and 4 the additional interlace line keeps C4 = R4 and
// C9 = 0 ; CRTC 0, 1, 2 increment C4 (R4 + 1).
TEST(CRTC_Compendium, InterlaceAdditionalLineCounters)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, type);
      WriteRegister(crtc, 8, 1);
      bool seen = false;
      for (int frame = 0; frame < 2 && !seen; ++frame)
      {
         ASSERT_TRUE(ReachLine(crtc, 38, 7));
         const Line next = NextLine(crtc);
         if (next == Line(0, 0)) continue;
         seen = true;
         const bool asic = (type == CRTC::AMS40489 || type == CRTC::AMS40226);
         EXPECT_EQ(asic ? Line(38, 0) : Line(39, 0), next);
      }
      EXPECT_TRUE(seen) << "no additional interlace line over 2 frames";
   }
}

// 19.7 : MID-VSYNC : on the even frames of an interlace mode, the VSYNC starts
// when C0 reaches R0/2 ; on the odd frames on C0 = 0.
TEST(CRTC_Compendium, InterlaceMidVSync)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, type);
      WriteRegister(crtc, 8, 1);
      std::string starts;
      for (int frame = 0; frame < 2; ++frame)
      {
         ASSERT_NE(-1, TicksUntilVSyncRisingEdge(crtc, sig, 2 * 19968));
         starts += (crtc.hcc_ == 0) ? 'L' : (crtc.hcc_ == 31) ? 'M' : '?';
      }
      EXPECT_TRUE(starts == "LM" || starts == "ML") << starts;
   }
}

// 19.4, 19.8 : Interlace Video Mode (R8 = 3), 8-line characters (R9 = 6 on
// CRTC 0, 3, 4 ; R9 = 7 on CRTC 1) : one frame displays the even lines of the
// characters, the other one the odd lines.
TEST(CRTC_Compendium, InterlaceVideoModeAlternatesTheLines)
{
   struct { CRTC::TypeCRTC type; unsigned char r9; } const cases[] = {
      { CRTC::HD6845S, 6 }, { CRTC::UM6845R, 7 }, { CRTC::AMS40489, 6 }, { CRTC::AMS40226, 6 },
   };
   for (const auto& c : cases)
   {
      SCOPED_TRACE(TypeName(c.type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, c.type);
      ProgramStandardEuropeanScreen(crtc);
      WriteRegister(crtc, 8, 3);
      WriteRegister(crtc, 9, c.r9);
      AdvanceMicroseconds(crtc, 3 * 19968);
      const std::string a = RowAddressC9s(crtc, 5);
      const std::string b = RowAddressC9s(crtc, 5);   // row 5 of the next frame
      EXPECT_TRUE((a == "0246" && b == "1357") || (a == "1357" && b == "0246")) << a << " / " << b;
   }
}

// --- Interlace Video Mode, details (second batch) ------------------------------

// 19.8.1 (table p222) : on CRTC 0 the IVM state set by R8 is taken when C0
// restarts at 0 : the line on which R8 = 3 is written keeps its non doubled
// address (C9.VMA = C9 = 1), the doubled C9.VMA starts on the next line.
// R9 = 6 : even frame 4, 6, then 0, 2 ; odd frame 5, 7, then 1, 3.
TEST(CRTC_Compendium, Crtc0IvmTakenAtTheNextLine)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::HD6845S);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 9, 6);
   AdvanceMicroseconds(crtc, 3 * 19968);
   std::string frames[2];
   for (int frame = 0; frame < 2; ++frame)
   {
      ASSERT_TRUE(ReachLine(crtc, 0, 1));
      AdvanceUntilHccEquals(crtc, 10);
      WriteRegister(crtc, 8, 3);
      EXPECT_EQ(1, AddressC9(crtc));
      for (int line = 0; line < 4; ++line)
      {
         NextLine(crtc);
         frames[frame] += char('0' + AddressC9(crtc));
      }
      ASSERT_TRUE(ReachLine(crtc, 3, 0));
      WriteRegister(crtc, 8, 0);
   }
   EXPECT_TRUE((frames[0] == "4602" && frames[1] == "5713") || (frames[0] == "5713" && frames[1] == "4602"))
      << frames[0] << " / " << frames[1];
}

// 19.8.1 (remark p221) : when R8 returns to 0, C9.VMA (with the parity) is
// compared with R9 without the parity : C9 = 3, R9 = 6 on an odd frame gives
// C9.VMA = 7 <> 6, C9 goes on to 4 ; on an even frame C9.VMA = 6 = R9, C9 = 0.
TEST(CRTC_Compendium, Crtc0IvmOffComparesC9VmaWithR9)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::HD6845S);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 9, 6);
   WriteRegister(crtc, 8, 3);
   AdvanceMicroseconds(crtc, 3 * 19968);
   bool seen[2] = { false, false };
   for (int frame = 0; frame < 2; ++frame)
   {
      ASSERT_TRUE(ReachLine(crtc, 5, 3));
      const int c9_vma = AddressC9(crtc);
      AdvanceUntilHccEquals(crtc, 10);
      WriteRegister(crtc, 8, 0);
      const Line next = NextLine(crtc);
      if (c9_vma == 7) { seen[1] = true; EXPECT_EQ(Line(5, 4), next); }
      else             { seen[0] = true; EXPECT_EQ(6, c9_vma); EXPECT_EQ(Line(6, 0), next); }
      ASSERT_TRUE(ReachLine(crtc, 0, 0));
      WriteRegister(crtc, 8, 3);
   }
   EXPECT_TRUE(seen[0] && seen[1]);
}

// 19.5.2, 19.5.5 : in IVM with R9 odd (an odd number of lines per character),
// CRTC 0, 3 and 4 delay the VSYNC by one line on an odd C4 of an odd frame. On
// the even frame it is a MID-VSYNC on the first line of C4 = R7. CRTC 1 has no
// such delay (19.5.3) : the VSYNC is on the first line on both frames.
TEST(CRTC_Compendium, IvmVSyncDelayedOnOddC4)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::AMS40489, CRTC::AMS40226, CRTC::UM6845R })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      ProgramStandardEuropeanScreen(crtc);
      WriteRegister(crtc, 8, 3);
      WriteRegister(crtc, 9, 7);
      WriteRegister(crtc, 7, 11);
      AdvanceMicroseconds(crtc, 3 * 19968);
      std::string starts;   // line of the character row C4 = R7 the VSYNC starts on
      for (int frame = 0; frame < 2; ++frame)
      {
         int line_in_row = -1;
         bool was = sig.v_sync_;
         for (int i = 0; i < 2 * 19968; ++i)
         {
            Advance(crtc);
            if (crtc.hcc_ == 0) line_in_row = (crtc.vcc_ == 11) ? line_in_row + 1 : -1;
            if (sig.v_sync_ && !was) break;
            was = sig.v_sync_;
         }
         starts += char('0' + line_in_row);
      }
      if (type == CRTC::UM6845R)
         EXPECT_EQ("00", starts);
      else
         EXPECT_TRUE(starts == "01" || starts == "10") << starts;
   }
}

// 19.8.3 : CRTC 2 counts C9 normally (0..R9) and uses another counter, C9.IVM,
// reset when C9 returns to 0 and when C9 reaches R9/2 : with R9 = 7 the address
// is 0, 2, 4, 6 twice per C4 on an even frame (1, 3, 5, 7 on an odd frame), and
// VMA' is loaded with VMA on C0 = R1 of the line C9 = R9/2 : the second half of
// the character starts R1 characters further.
TEST(CRTC_Compendium, Crtc2IvmCounter)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::MC6845);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 8, 3);
   AdvanceMicroseconds(crtc, 3 * 19968);
   const std::string a = RowAddressC9s(crtc, 5);
   const std::string b = RowAddressC9s(crtc, 5);
   EXPECT_TRUE((a == "02460246" && b == "13571357") || (a == "13571357" && b == "02460246")) << a << " / " << b;
   ASSERT_TRUE(ReachLine(crtc, 5, 0));
   const unsigned short first_half = crtc.ma_;
   ASSERT_TRUE(ReachLine(crtc, 5, 4));
   EXPECT_EQ(first_half + 40, crtc.ma_);
}

// 11.2.3 : with interlace, the additional interlace line is counted as one more
// R5 line on CRTC 1 : R4 = 37, R9 = 7, R5 = 8 : C4 = 38 on the R5 lines and 39
// on the interlace line (even frames).
TEST(CRTC_Compendium, Crtc1InterlaceLineAfterR5)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::UM6845R);
   ProgramStandardEuropeanScreen(crtc);
   WriteRegister(crtc, 4, 37);
   WriteRegister(crtc, 5, 8);
   WriteRegister(crtc, 8, 1);
   AdvanceMicroseconds(crtc, 3 * 19968);
   bool interlace_line = false;
   for (int frame = 0; frame < 2; ++frame)
   {
      ASSERT_TRUE(ReachLine(crtc, 37, 7));
      for (int c9 = 0; c9 < 8; ++c9)
         EXPECT_EQ(Line(38, c9), NextLine(crtc));
      const Line next = NextLine(crtc);
      if (next != Line(0, 0)) { interlace_line = true; EXPECT_EQ(Line(39, 0), next); }
   }
   EXPECT_TRUE(interlace_line);
}

// --- Half microsecond DISPEN (17.6, 18.3.2) -----------------------------------
// The GATE ARRAY fetches 2 bytes per CRTC character : DispEn(0) / DispEn(1) is
// DISPEN for the first / second byte (half microsecond) of the character.

namespace
{
// DISPEN of the C0 = 0 .. 63 characters of one line, 2 characters per C0
// ('1' = displayed byte, '0' = border byte).
std::string DispEnHalves(CRTC& crtc, int first_c0, int last_c0)
{
   std::string s;
   for (int c0 = 0; c0 <= last_c0; ++c0)
   {
      if (c0 > 0) Advance(crtc);
      if (c0 < first_c0) continue;
      s += crtc.DispEn(0) ? '1' : '0';
      s += crtc.DispEn(1) ? '1' : '0';
   }
   return s;
}
}  // namespace

// 17.6 : R1 > R0, C0 never reaches R1. On CRTC 0 and 2 the border is set 0.5 us
// after C0 = R0 and cleared on the next character : the second byte of the
// character C0 = R0 is border. CRTC 1, 3, 4 send no border.
TEST(CRTC_Compendium, BorderByteWhenR1GreaterThanR0)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, type);
      WriteRegister(crtc, 1, 70);
      ASSERT_TRUE(ReachLine(crtc, 5, 2));
      const bool border_byte = (type == CRTC::HD6845S || type == CRTC::MC6845);
      EXPECT_EQ(border_byte ? "111110" : "111111", DispEnHalves(crtc, 61, 63));
      Advance(crtc);   // C0 = 0 : displayed again
      EXPECT_TRUE(crtc.DispEn(0));
      EXPECT_TRUE(crtc.DispEn(1));
   }
}

// 17.6 : R0 = 0 with R1 > R0 on CRTC 0 : every character is C0 = R0, the bytes
// alternate between displayed and border.
TEST(CRTC_Compendium, Crtc0R0ZeroAlternatesBytes)
{
   CRTC crtc; CSig sig;
   Screen(crtc, sig, CRTC::HD6845S);
   ASSERT_TRUE(ReachLine(crtc, 5, 2));
   WriteRegister(crtc, 0, 0);
   std::string s;
   for (int i = 0; i < 4; ++i)
   {
      Advance(crtc);
      s += crtc.DispEn(0) ? '1' : '0';
      s += crtc.DispEn(1) ? '1' : '0';
   }
   EXPECT_EQ("10101010", s);
}

// 18.3.2 : R6 = 0 on the first line of a frame (C4 = C9 = 0), CRTC 0 and 2 : the
// R6 border is set (C4 = R6) and cleared (new frame) on each character : the
// first byte is displayed, the second one is border, as long as the R1 border is
// not active. From the second line the R6 border stays. CRTC 1 : R6 = 0 is a
// border (18.3.3). CRTC 3, 4 : no special case, border.
TEST(CRTC_Compendium, R6ZeroOnTheFirstLine)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      Screen(crtc, sig, type);
      WriteRegister(crtc, 6, 0);
      AdvanceMicroseconds(crtc, 19968);
      ASSERT_TRUE(ReachLine(crtc, 0, 0));
      const bool conflict = (type == CRTC::HD6845S || type == CRTC::MC6845);
      EXPECT_EQ(conflict ? "101010" : "000000", DispEnHalves(crtc, 0, 2));
      ASSERT_TRUE(ReachLine(crtc, 0, 1));
      EXPECT_EQ("000000", DispEnHalves(crtc, 0, 2));
   }
}

// 18.3.2 : on the first line, R6 set back to a value > 0 before C0 = R1 cancels
// the conflict : no R6 border on the next line. If R6 is still 0 when C0 = R1,
// the border is definitive, even if R6 changes afterwards.
TEST(CRTC_Compendium, R6ZeroConflictResolvedOnC0EqualsR1)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::MC6845 })
   {
      SCOPED_TRACE(TypeName(type));
      {
         CRTC crtc; CSig sig;
         Screen(crtc, sig, type);
         WriteRegister(crtc, 6, 0);
         AdvanceMicroseconds(crtc, 19968);
         ASSERT_TRUE(ReachLine(crtc, 0, 0));
         AdvanceUntilHccEquals(crtc, 20);
         WriteRegister(crtc, 6, 25);
         ASSERT_TRUE(ReachLine(crtc, 0, 1));
         EXPECT_EQ("111111", DispEnHalves(crtc, 0, 2));
      }
      {
         CRTC crtc; CSig sig;
         Screen(crtc, sig, type);
         WriteRegister(crtc, 6, 0);
         AdvanceMicroseconds(crtc, 19968);
         ASSERT_TRUE(ReachLine(crtc, 0, 0));
         AdvanceUntilHccEquals(crtc, 50);   // after C0 = R1 = 40
         WriteRegister(crtc, 6, 25);
         ASSERT_TRUE(ReachLine(crtc, 0, 1));
         EXPECT_EQ("000000", DispEnHalves(crtc, 0, 2));
      }
   }
}

/////////////////////////////////////////////////////////////
// K. Bus interface (4.4.3, 4.4.4) : the CRTC takes an I/O while its clock window
// is open. t_state = quarter of microsecond where the Z80 asserts IORQ.
// Gate Array (CRTC 0, 1, 2) : window on T-states 0 and 1. ASIC (CRTC 3, 4) : only
// T-state 0, a later I/O is taken by the next window (next character).

namespace
{
bool IsAsicCrtc(CRTC::TypeCRTC type)
{
   return type == CRTC::AMS40489 || type == CRTC::AMS40226;
}
}

// OUTI / OUT(n),A (T-state 0) : taken in the current character on every CRTC.
TEST(CRTC_BusInterface, TState0IsTakenNow)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      crtc.Out(kSelectRegister, 12, 0);
      crtc.Out(kWriteRegister, 0x15, 0);
      EXPECT_EQ(0x15, crtc.registers_list_[12]);
   }
}

// OUT(C),r8 (T-state 1) : 3rd microsecond on CRTC 0/1/2, 4th on CRTC 3/4 (4.4.3).
TEST(CRTC_BusInterface, TState1IsOneMicrosecondLaterOnAsic)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      const unsigned char before = crtc.registers_list_[12];
      crtc.Out(kSelectRegister, 12, 1);
      if (IsAsicCrtc(type)) Advance(crtc);
      crtc.Out(kWriteRegister, 0x15, 1);
      EXPECT_EQ(IsAsicCrtc(type) ? before : 0x15, crtc.registers_list_[12]);
      Advance(crtc);
      EXPECT_EQ(0x15, crtc.registers_list_[12]);
   }
}

// An I/O asserted after the window (T-state 2 or 3) waits for the next one.
TEST(CRTC_BusInterface, LateTStateWaitsForTheNextWindow)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      MakeCrtc(crtc, sig, type);
      const unsigned char before = crtc.registers_list_[12];
      crtc.Out(kSelectRegister, 12, 0);
      crtc.Out(kWriteRegister, 0x15, 2);
      EXPECT_EQ(before, crtc.registers_list_[12]);
      Advance(crtc);
      EXPECT_EQ(0x15, crtc.registers_list_[12]);
   }
}

/////////////////////////////////////////////////////////////
// L. R2.JIT / R3.JIT (14.5.4, 14.7.1) : on CRTC 0, 1, 2 the HSYNC pin follows the
// C0==R2 and C3l==R3l comparators inside the character. hsync_quarters_ : pin level on
// each T-state of the current character (bit 0 = T-state 0).
// Standard screen : R2 = 46, R3l = 14.

namespace
{
const unsigned int kOutCR8 = 1;   // OUT(C),r8 : IORQ on T-state 1
const unsigned int kOuti = 0;     // OUTI : IORQ on T-state 0

void WriteRegisterAt(CRTC& crtc, unsigned char reg, unsigned char value, unsigned int t_state)
{
   crtc.Out(kSelectRegister, reg, 0);
   crtc.Out(kWriteRegister, value, t_state);
}

// Screen with R2 = 50, on a visible line, C0 = 46 (the HSYNC has not started).
void OnC0Equals46WithoutHSync(CRTC& crtc, CSig& sig, CRTC::TypeCRTC type)
{
   Screen(crtc, sig, type);
   WriteRegister(crtc, 2, 50);
   ASSERT_TRUE(ReachLine(crtc, 5, 2));
   AdvanceUntilHccEquals(crtc, 46);
   ASSERT_FALSE(sig.h_sync_);
}

// HSYNC length in microseconds from now (the current character counts if the pin is high).
int HSyncLengthFromNow(CRTC& crtc, CSig& sig)
{
   int length = 0;
   while (sig.h_sync_ && length < 40)
   {
      ++length;
      Advance(crtc);
   }
   return length;
}

// Screen on a visible line, C0 = R2 + c3l : C3l = c3l.
void InsideHSync(CRTC& crtc, CSig& sig, CRTC::TypeCRTC type, int c3l)
{
   Screen(crtc, sig, type);
   ASSERT_TRUE(ReachLine(crtc, 5, 2));
   AdvanceUntilHccEquals(crtc, (unsigned char)(46 + c3l));
   ASSERT_TRUE(sig.h_sync_);
   ASSERT_EQ(c3l, crtc.horinzontal_pulse_);
}
}

// R2.JIT with OUT(C),r8 : the HSYNC starts on T-state 1 of C0 = R2, with its full length.
// CRTC 3, 4 take the I/O on the next character : C0 = R2 is missed.
TEST(CRTC_Jit, R2JitStartsTheHSyncOneTStateLate)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      OnC0Equals46WithoutHSync(crtc, sig, type);
      WriteRegisterAt(crtc, 2, 46, kOutCR8);
      if (IsAsicCrtc(type))
      {
         Advance(crtc);
         EXPECT_FALSE(sig.h_sync_);
         continue;
      }
      EXPECT_TRUE(sig.h_sync_);
      EXPECT_EQ(0x0E, crtc.hsync_quarters_);
      EXPECT_EQ(14, HSyncLengthFromNow(crtc, sig));
   }
}

// R2 written with the current C0 by OUTI : as if R2 had been written before (14.7.1).
TEST(CRTC_Jit, R2WrittenByOutiOnC0EqualsR2IsANormalHSync)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::UM6845R, CRTC::MC6845 })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      OnC0Equals46WithoutHSync(crtc, sig, type);
      WriteRegisterAt(crtc, 2, 46, kOuti);
      EXPECT_EQ(0x0F, crtc.hsync_quarters_);
      EXPECT_EQ(14, HSyncLengthFromNow(crtc, sig));
   }
}

// R3.JIT with OUT(C),r8 : R3l = C3l ends the HSYNC on T-state 1 (0.25 us after the end it
// would have had). Without JIT the CRTC 3, 4 C3l overflows : 16 + 5 us.
TEST(CRTC_Jit, R3JitEndsTheHSyncOneTStateLate)
{
   for (CRTC::TypeCRTC type : kAllTypes)
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      InsideHSync(crtc, sig, type, 5);
      sig.hsync_fall_ = false;
      WriteRegisterAt(crtc, 3, 0x85, kOutCR8);
      if (IsAsicCrtc(type))
      {
         Advance(crtc);
         EXPECT_EQ(16 - 1, HSyncLengthFromNow(crtc, sig));   // C3l = 6 .. 15, 0 .. 4
         continue;
      }
      EXPECT_FALSE(sig.h_sync_);
      EXPECT_TRUE(sig.hsync_fall_);
      EXPECT_EQ(0x01, crtc.hsync_quarters_);
   }
}

// R3l = C3l written by OUTI : the HSYNC ends on the character start, as if R3l had been
// programmed before (no R3.JIT with OUTI, 14.5.4).
TEST(CRTC_Jit, R3WrittenByOutiEndsOnTheCharacterStart)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::UM6845R, CRTC::MC6845 })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      InsideHSync(crtc, sig, type, 5);
      WriteRegisterAt(crtc, 3, 0x85, kOuti);
      EXPECT_FALSE(sig.h_sync_);
      EXPECT_EQ(0x00, crtc.hsync_quarters_);
   }
}

// First microsecond of the HSYNC, R3 = 0 (14.5.4) : CRTC 0, 1 cut the HSYNC (OUT(C),r8), or
// prevent it from starting (OUTI : no HSYNC, no falling edge for the interrupts). CRTC 2 :
// R3l = 0 is 16 us, the HSYNC goes on.
TEST(CRTC_Jit, R3ZeroOnTheFirstMicrosecond)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::UM6845R, CRTC::MC6845 })
   {
      SCOPED_TRACE(TypeName(type));
      const bool cut = (type != CRTC::MC6845);
      {
         CRTC crtc; CSig sig;
         InsideHSync(crtc, sig, type, 0);
         sig.hsync_fall_ = false;
         WriteRegisterAt(crtc, 3, 0x80, kOutCR8);
         EXPECT_EQ(!cut, sig.h_sync_);
         EXPECT_EQ(cut ? 0x01 : 0x0F, crtc.hsync_quarters_);
         EXPECT_EQ(cut, sig.hsync_fall_);
      }
      {
         CRTC crtc; CSig sig;
         InsideHSync(crtc, sig, type, 0);
         sig.hsync_fall_ = false;
         WriteRegisterAt(crtc, 3, 0x80, kOuti);
         EXPECT_EQ(!cut, sig.h_sync_);
         EXPECT_EQ(cut ? 0x00 : 0x0F, crtc.hsync_quarters_);
         EXPECT_FALSE(sig.hsync_fall_);
      }
   }
}

// R3 = 0 during the HSYNC (C3l = 5) : CRTC 1 cuts it (14.5.2) ; CRTC 0, 2 count up to 0
// (16 us : C3l = 6 .. 15, 0).
TEST(CRTC_Jit, R3ZeroDuringTheHSync)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::UM6845R, CRTC::MC6845 })
   {
      SCOPED_TRACE(TypeName(type));
      CRTC crtc; CSig sig;
      InsideHSync(crtc, sig, type, 5);
      WriteRegisterAt(crtc, 3, 0x80, kOutCR8);
      if (type == CRTC::UM6845R)
      {
         EXPECT_FALSE(sig.h_sync_);
         EXPECT_EQ(0x01, crtc.hsync_quarters_);
      }
      else
      {
         EXPECT_EQ(11, HSyncLengthFromNow(crtc, sig));
      }
   }
}
