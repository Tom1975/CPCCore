#include "gtest/gtest.h"
#include <string>

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
// instead they invoke the per-type ClockTickN() function directly through
// the public TickFunction member-pointer (Advance() below), which only
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

// Calls the CRTC's own per-type tick function directly (bypasses
// CRTC::Tick(), see file header), then replicates the one line of
// CRTC::Tick() that is safe to run standalone: signals_->v_sync_ = ff4_.
// (The rest of Tick() after the ClockTickN() call is gate_array_->Tick()
// [unsafe, see file header], cursor-line handling [no-op: cursor_line_ is
// nullptr by default], and lightpen bookkeeping [no-op: gun_button_ is 0 by
// default] -- ff4_ propagation is the only part that matters and is safe.)
// One call = one microsecond of CRTC time.
void Advance(CRTC& crtc)
{
   (crtc.*(crtc.TickFunction))();
   crtc.signals_->v_sync_ = crtc.ff4_;
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
   WriteRegister(crtc, 2, 0);     // R2 = 0
   WriteRegister(crtc, 3, 0x82);  // R3l = 2

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
// cancels it; CRTC 0 and 2 treat 0 as a value to reach (C3l overflows to 16).
// SAFETY NET.
TEST(CRTC_HSyncReentrancy, WritingR3lZeroDuringHSync)
{
   struct { CRTC::TypeCRTC type; int length; } const cases[] = {
      { CRTC::HD6845S, 16 }, { CRTC::UM6845R, 3 }, { CRTC::MC6845, 16 },
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
// R0 is written while C0 = 0, so C0 does not overflow (13.6).
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

   WriteRegister(crtc, 0, 0);   // first C0 = 0 with R0 = 0
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
   Advance(crtc);
   EXPECT_EQ(6, crtc.vcc_);
   EXPECT_EQ(0, crtc.vlc_);
   AdvanceMicroseconds(crtc, 200);
   EXPECT_EQ(6, crtc.vcc_);
   EXPECT_EQ(0, crtc.vlc_);

   // C9 was not reset : it now counts from its frozen value
   WriteRegister(crtc, 0, 3);
   AdvanceUntilHccEquals(crtc, 0);
   EXPECT_EQ(6, crtc.vcc_);
   EXPECT_EQ(1, crtc.vlc_);
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
