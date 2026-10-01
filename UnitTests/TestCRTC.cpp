#include "gtest/gtest.h"
#include <string>

#include "CRTC.h"
#include "Sig.h"

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
// touches CRTC::signals_ and the CRTC's own registers/counters. That works
// cleanly for CRTC 0/1/2. CRTC 3/4's ClockTick34() also dereferences
// gate_array_->memory_ for the CPC+ split-screen/soft-scroll registers, so
// ticking those two types needs the full Motherboard/EmulatorEngine wiring
// and is out of scope for this isolated harness -- register-write tests
// (which never tick) still cover all 5 types.

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
void MakeCrtc(CRTC& crtc, CSig& sig, CRTC::TypeCRTC type)
{
   crtc.SetSig(&sig);
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

// Compendium chapitre 14.2: R3's high nibble is a 4-bit VSYNC line count on
// CRTC 0, 3 and 4 (0 meaning 16 lines), exactly like the low nibble is a
// 4-bit HSYNC width. CRTC 1 and 2 ignore it and always use 16 lines. The R3
// handler in CRTC::Out() (case 3) implements this correctly for CRTC 3/4
// (vertical_sync_width_ = registers_list_[3] >> 4) but collapses CRTC 0 to a
// binary choice driven by bit 7 alone (16 if set, else a fixed 8), ignoring
// bits 4-6 entirely. KNOWN DIVERGENCE: with the same R3 nibble, CRTC0 and
// CRTC3/4 compute different (and, for CRTC0, wrong) VSYNC widths.
TEST(CRTC_VerticalSyncWidth, Crtc0IgnoresBits4To6OfR3_KNOWN_DIVERGENCE)
{
   CRTC crtc0; CSig sig0;
   MakeCrtc(crtc0, sig0, CRTC::HD6845S);
   WriteRegister(crtc0, 3, 0x30);  // nibble = 3 -> Compendium says 3 lines.

   CRTC crtc3; CSig sig3;
   MakeCrtc(crtc3, sig3, CRTC::AMS40489);
   WriteRegister(crtc3, 3, 0x30);  // Same R3 value, CRTC3 this time.

   EXPECT_EQ(3, crtc3.vertical_sync_width_)
      << "CRTC3 reads the R3 nibble as a count, matching the Compendium.";

   // Compendium-documented hardware would also give 3 here. The code gives 8
   // because HD6845S's branch only tests bit 0x80 and picks between 8 or 16.
   EXPECT_EQ(8, crtc0.vertical_sync_width_)
      << "CRTC0's VSYNC width computation in CRTC::Out() case 3 only looks "
         "at bit 7 of R3 (8 or 16 lines); the Compendium (14.2) documents "
         "the full nibble as a 1-15 count (0 meaning 16), same as CRTC3/4. "
         "If this now reads 3, that branch has been corrected -- update "
         "this assertion and the one above accordingly.";
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
// With R0 = R2 = 0, C0 equals R2 on every tick, so C0 == R2 also holds at the
// position C0 = R2 + R3l where the HSYNC should end:
// - CRTC 0 is protected (15.3.1, 15.3.2): the HSYNC ends and cannot restart
//   on that same position; it restarts on the next one. With R3l = 4 the
//   HSYNC pin reads 1111 0 1111 0 ...
// - CRTC 1, 2, 3 and 4 have the bug (15.3.1, 15.3.2): the HSYNC does not end,
//   C3l overflows (15, 0, ...) and the pin stays high ("HSYNC infinie"). On
//   CRTC 1 the internal off/on transition is shorter than 1 us (15.3.4),
//   so at 1 us resolution the pin reads 1 constantly.
// The emulation currently swaps CRTC 0 and CRTC 1, and CRTC 2 drops HSYNC for
// good once the first pulse is over. These three tests pin down today's exact
// sequence so that the fix is a deliberate, visible change.
// ---------------------------------------------------------------------------

namespace
{
std::string HSyncTrace(CRTC::TypeCRTC type, int ticks)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, type);
   WriteRegister(crtc, 0, 0);     // R0 = 0
   WriteRegister(crtc, 2, 0);     // R2 = 0
   WriteRegister(crtc, 3, 0x84);  // R3l = 4

   std::string trace;
   for (int i = 0; i < ticks; ++i)
   {
      Advance(crtc);
      trace += sig.h_sync_ ? '1' : '0';
   }
   return trace;
}
}  // namespace

// Compendium expects "111101111011110111101111" (protected).
TEST(CRTC_HSyncReentrancy, Crtc0IsNotProtected_KNOWN_DIVERGENCE)
{
   EXPECT_EQ("011111111111111111111111", HSyncTrace(CRTC::HD6845S, 24));
}

// Compendium expects "111111111111111111111111" (C3l overflow).
TEST(CRTC_HSyncReentrancy, Crtc1RestartsInsteadOfOverflowing_KNOWN_DIVERGENCE)
{
   EXPECT_EQ("111101111011110111101111", HSyncTrace(CRTC::UM6845R, 24));
}

// Compendium expects "111111111111111111111111" (C3l overflow).
TEST(CRTC_HSyncReentrancy, Crtc2StopsAfterFirstPulse_KNOWN_DIVERGENCE)
{
   EXPECT_EQ("111100000000000000000000", HSyncTrace(CRTC::MC6845, 24));
}
