#include "gtest/gtest.h"

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
//
// CRTC::Reset() never touches ff1_/ff3_/ff4_ (see CRTC_Reset.
// LeavesFf1Ff3Ff4AtWhateverValueTheyHadBefore_KNOWN_DIVERGENCE below), so on
// a freshly-constructed CRTC they hold indeterminate memory rather than a
// defined post-reset value. Real hardware's RESET pin forces DE and VSYNC to
// their inactive state, so the three lines below emulate that correct
// behaviour at the harness level -- without them, every tick-driven test in
// this file would be at the mercy of whatever garbage happened to be on the
// stack.
void MakeCrtc(CRTC& crtc, CSig& sig, CRTC::TypeCRTC type)
{
   crtc.SetSig(&sig);
   crtc.DefinirTypeCRTC(type);
   crtc.Reset();
   crtc.ff1_ = false;
   crtc.ff3_ = false;
   crtc.ff4_ = false;
}

void WriteRegister(CRTC& crtc, unsigned char reg, unsigned char value)
{
   crtc.Out(kSelectRegister, reg);
   crtc.Out(kWriteRegister, value);
}

// Programs the "standard European" table from the CPC low ROM (Compendium
// chapitre 4.1, table ROM address &5C5): lines of 64 chars (40 displayed),
// 312 raster lines as 39 character rows of 8 lines, no vertical adjustment.
// Written through Out(), not by relying on CRTC::Reset()'s raw defaults,
// because Out()'s R3 handler is what actually computes
// horizontal_sync_width_/vertical_sync_width_ -- CRTC::Reset() only pokes
// registers_list_[3] directly and leaves those two derived fields whatever
// they were before, exactly as real hardware leaves them undefined until the
// boot ROM's first OUT to R3 (Compendium chapitre 4.1, note 2).
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

// A real CRTC's RESET input forces its outputs to their inactive state (the
// datasheets describe DE and the sync outputs as cleared on reset), and
// CRTC::Reset() does clear signals_->h_sync_/v_sync_ accordingly (when
// signals_ is wired). But ff1_ (drives DE together with ff3_, see
// GateArray.cpp's DISPEN_TEST) and ff3_/ff4_ (DISPEN gate / VSYNC) are never
// assigned anywhere in CRTC::Reset(): whatever value they held the instant
// before Reset() was called is exactly what they still hold afterwards. On a
// freshly-constructed CRTC (as in every other test in this file) that is
// indeterminate memory rather than a defined value -- MakeCrtc() works
// around it by clearing the three fields itself post-Reset(), which is a
// test-harness compensation, not evidence that CRTC::Reset() is correct.
// KNOWN DIVERGENCE, demonstrated here without relying on actual
// indeterminate memory (which would be undefined behaviour to read): poison
// the flags to a known value, Reset(), and show Reset() left them untouched.
TEST(CRTC_Reset, LeavesFf1Ff3Ff4AtWhateverValueTheyHadBefore_KNOWN_DIVERGENCE)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::UM6845R);

   crtc.ff1_ = true;
   crtc.ff3_ = true;
   crtc.ff4_ = true;

   crtc.Reset();

   // Compendium-documented hardware would clear all three here, exactly as
   // CRTC::Reset() already does for signals_->h_sync_/v_sync_ a few lines
   // above. If these now read false, ff1_/ff3_/ff4_ have been added to
   // CRTC::Reset() -- update this assertion (and drop the workaround in
   // MakeCrtc() above) accordingly.
   EXPECT_TRUE(crtc.ff1_) << "ff1_ (DE) survived Reset() unchanged";
   EXPECT_TRUE(crtc.ff3_) << "ff3_ (DE gate) survived Reset() unchanged";
   EXPECT_TRUE(crtc.ff4_) << "ff4_ (VSYNC) survived Reset() unchanged";
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
// "Sur le CRTC 0, deux HSYNC ne peuvent pas etre collees si la position
// C0=R2 est rencontree lorsque C3l atteint R3l" (15.3.1) -- CRTC0 is
// protected. "Sur les CRTC 1, 2, 3 et 4, il y a un bug de gestion si C0=R2
// sur C0=R2+R3" (15.3.1), demonstrated in 15.3.2 with R0=R2=0: C0 equals R2
// on every single tick, so the HSYNC start condition re-arms before the
// previous pulse's width has elapsed, and the CRTC's HSYNC signal never
// falls again ("HSYNC infinie").
//
// CRTC_0.cpp and CRTC_1.cpp raise the HSYNC start flip-flop unconditionally
// whenever hcc_ == R2 (no guard). CRTC_2.cpp (and CRTC_3_4.cpp) guard that
// same test with "if (h_no_sync_)". With R0 = R2 = 0, h_no_sync_ is only
// ever cleared by the "hcc_ != R2" branch, which never runs -- so CRTC2's
// guard, once armed, blocks all further re-triggers and its HSYNC pulse
// completes normally. That is the exact inverse of the Compendium: CRTC0
// unprotected, CRTC2 protected.
// ---------------------------------------------------------------------------

TEST(CRTC_HSyncReentrancy, Crtc0RemainsAssertedForever_KNOWN_DIVERGENCE)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::HD6845S);
   WriteRegister(crtc, 0, 0);  // R0 = 0
   WriteRegister(crtc, 2, 0);  // R2 = 0
   WriteRegister(crtc, 3, 0x84);  // HSYNC width 4 chars

   AdvanceMicroseconds(crtc, 40);  // Far past the 4-character pulse width.

   // Compendium (15.3.1): CRTC0 should be protected and h_sync_ should have
   // fallen again after 4 ticks. If this now reads false, the guard has been
   // added -- update this assertion (and its title) to reflect the fix.
   EXPECT_TRUE(sig.h_sync_)
      << "CRTC0's HSYNC never fell -- current (unprotected) behaviour. The "
         "Compendium documents CRTC0 as the one type NOT affected by this "
         "bug (15.3.1); expected to become false once ClockTick0() gains "
         "CRTC2's h_no_sync_-style guard.";
}

TEST(CRTC_HSyncReentrancy, Crtc1RemainsAssertedForever)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::UM6845R);
   WriteRegister(crtc, 0, 0);
   WriteRegister(crtc, 2, 0);
   WriteRegister(crtc, 3, 0x84);

   AdvanceMicroseconds(crtc, 40);

   // SAFETY NET: the Compendium documents this exact bug on CRTC1 (15.3.1,
   // 15.3.2 worked example). This must stay true.
   EXPECT_TRUE(sig.h_sync_) << "CRTC1's HSYNC infinie bug regressed";
}

TEST(CRTC_HSyncReentrancy, Crtc2RecoversAfterPulseWidth_KNOWN_DIVERGENCE)
{
   CRTC crtc; CSig sig;
   MakeCrtc(crtc, sig, CRTC::MC6845);
   WriteRegister(crtc, 0, 0);
   WriteRegister(crtc, 2, 0);
   WriteRegister(crtc, 3, 0x84);

   AdvanceMicroseconds(crtc, 40);

   // Compendium-documented hardware would also read true here (CRTC2 is
   // explicitly listed alongside 1/3/4 in 15.3.1). If this now reads true,
   // the h_no_sync_ guard specific to CRTC2 has been removed -- update this
   // assertion (and its title) accordingly.
   EXPECT_FALSE(sig.h_sync_)
      << "CRTC2 recovered cleanly -- current behaviour, caused by the "
         "h_no_sync_ guard in ClockTick2() that the Compendium does not "
         "document for this type.";
}
