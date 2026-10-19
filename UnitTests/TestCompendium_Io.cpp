#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

// Tests written from "The Amstrad CPC CRTC Compendium" v1.11 (Logon System, CC BY-NC-ND 4.0),
// chapters 5 (I/O decoding, CPC+ unlocking), 7 (VSYNC seen by the PPI, FAKE VSYNC) and 29 (CPC
// identification : ASIC, PPI).
// A failing test is DISABLED_ : it documents a behaviour the emulation does not implement yet.

#include "CompendiumMachine.h"

#include "gtest/gtest.h"

using namespace compendium;

namespace
{
const unsigned short kResult = 0x3000;   // where the programs store what they read
const int kVSyncLine = 30 * 8;           // standard screen : VSYNC on C4 = R7 = 30, C9 = 0

int Line(Machine& m) { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_; }

bool UntilPosition(Machine& m, int line, int hcc)
{
   return m.CyclesUntil([&]() { return Line(m) == line && m.Crtc().hcc_ == hcc; }, 4 * 20000) >= 0;
}

// Runs 'program' (then JR $) to its end : 'cycles' 4 MHz cycles.
void RunProgram(Machine& m, std::vector<unsigned char> program, int cycles = 4 * 400)
{
   z80::Append(program, z80::kJrSelf);
   m.Run(program);
   for (int i = 0; i < cycles; ++i) m.Cycle();
}

// One byte sent to the CRTC select port &BC00 by OUT (C),C.
std::vector<unsigned char> SelectCrtc(const std::vector<unsigned char>& bytes)
{
   std::vector<unsigned char> p = { z80::kDi };
   for (unsigned char b : bytes) z80::Append(p, { 0x01, b, 0xBC, 0xED, 0x49 });
   return p;
}

// The CPC+ unlocking sequence (5.2) : RQ00, 0, 255, 119, 179, 81, 168, 212, 98, 57, 156, 70, 43, 21,
// 138, STATE (205 = unlock), <ACQ>.
std::vector<unsigned char> AsicSequence(unsigned char rq00, unsigned char state, bool acq)
{
   std::vector<unsigned char> s = { rq00, 0, 255, 119, 179, 81, 168, 212, 98, 57, 156, 70, 43, 21, 138, state };
   if (acq) s.push_back(0x00);
   return s;
}
}

/////////////////////////////////////////////////////////////
// 5.1 : I/O decoding

// 5.1 : the CRTC is selected by A14 = 0, A9-A8 give the function ; the other address bits are not
// decoded. The register number and the value are truncated to their useful bits : &29 selects R9,
// &27 writes 7 in it.
TEST(Compendium_5, CrtcDecodesA14AndA9A8Only)
{
   Machine m(CRTC::HD6845S);
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::Out(0x3C00, 0x29));   // A15 A13..A10 = 0 1111 : select
   z80::Append(p, z80::Out(0x3DFF, 0x27));   // write
   RunProgram(m, p);
   EXPECT_EQ(7, m.Crtc().registers_list_[9]);
}

// 5.1 : one I/O reaches every chip whose address bits are set : &3400 selects the CRTC (A14 = 0,
// A9-A8 = 00) and writes PPI port A (A11 = 0, A9-A8 = 00) with the same value.
TEST(Compendium_5, OneOutReachesEveryDecodedChip)
{
   Machine m(CRTC::HD6845S);
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::Out(0x3400, 0x0E));
   RunProgram(m, p);
   EXPECT_EQ(0x0E, m.Crtc().adddress_register_);
   EXPECT_EQ(0x0E, m.Engine().GetPPI()->port_a_);
}

// 5.1 (*) : on the CPC with an ASIC (CRTC 3), the GATE ARRAY (PAL) is selected whatever A14 :
// &3Fxx writes the border colour. On the CPC "old", A14 must be 1.
TEST(Compendium_5, DISABLED_GateArraySelectedWithA14ZeroOnTheCpcPlus)
{
   Machine m(CRTC::AMS40489, "6128PLUS");
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::Out(0x3F00, 0x10));   // PENR : border
   z80::Append(p, z80::Out(0x3F00, 0x4C));   // INKR : colour 12
   RunProgram(m, p);
   EXPECT_EQ(0x4C, m.Ga().border_reg_);
}

TEST(Compendium_5, GateArrayNeedsA14OneOnTheCpcOld)
{
   Machine m(CRTC::HD6845S);
   const unsigned char border = m.Ga().border_reg_;
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::Out(0x3F00, 0x10));
   z80::Append(p, z80::Out(0x3F00, border == 0x4C ? 0x4D : 0x4C));
   RunProgram(m, p);
   EXPECT_EQ(border, m.Ga().border_reg_);
}

/////////////////////////////////////////////////////////////
// 5.2, 29.1.1 : CPC+ extended functions, unlocked by a 17 bytes sequence on &BC00

TEST(Compendium_5, AsicUnlockSequence)
{
   Machine m(CRTC::AMS40489, "6128PLUS");
   RunProgram(m, SelectCrtc(AsicSequence(1, 205, true)));
   EXPECT_FALSE(m.Ga().IsAsicLocked());
}

// STATE = 205 unlocks once <ACQ>, any value, is sent.
TEST(Compendium_5, AsicUnlockWaitsForTheAcknowledgeByte)
{
   Machine m(CRTC::AMS40489, "6128PLUS");
   RunProgram(m, SelectCrtc(AsicSequence(1, 205, false)));
   EXPECT_TRUE(m.Ga().IsAsicLocked());
}

// STATE other than 205 locks.
TEST(Compendium_5, AsicSequenceWithAnotherStateLocks)
{
   Machine m(CRTC::AMS40489, "6128PLUS");
   std::vector<unsigned char> s = AsicSequence(1, 205, true);
   z80::Append(s, AsicSequence(1, 0, false));
   RunProgram(m, SelectCrtc(s), 4 * 600);
   EXPECT_TRUE(m.Ga().IsAsicLocked());
}

// RQ00 must not be 0. The sequence starts after a lock sequence (known state of the ASIC).
TEST(Compendium_5, AsicSequenceNeedsANonZeroRq00)
{
   Machine m(CRTC::AMS40489, "6128PLUS");
   std::vector<unsigned char> s = AsicSequence(1, 0, false);
   z80::Append(s, AsicSequence(0, 205, true));
   RunProgram(m, SelectCrtc(s), 4 * 600);
   EXPECT_TRUE(m.Ga().IsAsicLocked());
}

// A wrong byte in the sequence restarts it.
TEST(Compendium_5, AsicSequenceBrokenByAWrongByte)
{
   Machine m(CRTC::AMS40489, "6128PLUS");
   std::vector<unsigned char> s = AsicSequence(1, 205, true);
   s[6] = 0x12;
   RunProgram(m, SelectCrtc(s));
   EXPECT_TRUE(m.Ga().IsAsicLocked());
}

// The sequence does nothing on a CPC with a CRTC 4 (ASIC 40226, other sequence, not documented).
TEST(Compendium_5, AsicSequenceIgnoredWithACrtc4)
{
   Machine m(CRTC::AMS40226);
   RunProgram(m, SelectCrtc(AsicSequence(1, 205, true)));
   EXPECT_TRUE(m.Ga().IsAsicLocked());
}

/////////////////////////////////////////////////////////////
// 7.2 : the VSYNC pin of the CRTC is wired to bit 0 of PPI port B

namespace
{
int PortBAtLine(CRTC::TypeCRTC type, int line)
{
   Machine m(type);
   if (!UntilPosition(m, line, 0)) return -1;
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::In(0xF500, kResult));
   RunProgram(m, p, 4 * 20);
   return m.Ram()[kResult];
}
}

// Bit 0 of port B is the CRTC VSYNC pin (R3h = 8 lines on CRTC 0, 16 lines on CRTC 1), not the
// 26 lines VSYNC of the GATE ARRAY.
TEST(Compendium_7, PpiPortBBit0IsTheCrtcVSyncPin)
{
   EXPECT_EQ(0, PortBAtLine(CRTC::HD6845S, kVSyncLine - 2) & 1);
   EXPECT_EQ(1, PortBAtLine(CRTC::HD6845S, kVSyncLine + 2) & 1);
   EXPECT_EQ(0, PortBAtLine(CRTC::HD6845S, kVSyncLine + 10) & 1);
   EXPECT_EQ(1, PortBAtLine(CRTC::UM6845R, kVSyncLine + 10) & 1);
   EXPECT_EQ(0, PortBAtLine(CRTC::UM6845R, kVSyncLine + 20) & 1);
}

// 7.2 : IN A,(C) lasts 4 us ; A gets bit 0 of port B on its 4th microsecond. The IN starts
// k us before the VSYNC (C0 = 0 of the line C4 = R7) : bit 0 = 1 if the VSYNC starts on one of
// its 4 microseconds.
// DISABLED : the emulated IN reads port B on its 3rd microsecond (a VSYNC starting on the 4th one
// is not seen).
TEST(Compendium_7, DISABLED_InReadsTheVSyncOnItsFourthMicrosecond)
{
   for (int start = 52; start < 64; ++start)
   {
      Machine m(CRTC::HD6845S);
      ASSERT_TRUE(UntilPosition(m, kVSyncLine - 1, start));
      std::vector<unsigned char> p = { z80::kDi, 0x01, 0x00, 0xF5 };   // LD BC,&F500
      const unsigned short in = (unsigned short)(kProgram + p.size());
      z80::Append(p, { 0xED, 0x78, 0x32, (unsigned char)(kResult & 0xFF), (unsigned char)(kResult >> 8) });
      z80::Append(p, z80::kJrSelf);
      m.Run(p);
      // Microsecond of the IN (1 to 4) on which the VSYNC pin rises (0 : before the IN).
      ASSERT_GE(m.CyclesUntil([&]() { return m.Cpu().pc_ == in + 1; }, 400), 0);
      const int line_of_in = Line(m);
      int us = 0;
      if (!m.Sig().v_sync_)
      {
         const int cycles = m.CyclesUntil([&]() { return m.Sig().v_sync_; }, 4 * 100);
         ASSERT_GE(cycles, 0);
         // The CRTC moves C0 on the last cycle of each Z80 microsecond : the character starting on
         // cycle 4k+3 after the fetch is the one of the microsecond k+2.
         us = (cycles + 1) / 4 + 1;
      }
      for (int i = 0; i < 4 * 20; ++i) m.Cycle();
      SCOPED_TRACE(testing::Message() << "IN started on line " << line_of_in << ", VSYNC on its microsecond " << us);
      EXPECT_EQ(us <= 4 ? 1 : 0, m.Ram()[kResult] & 1);
   }
}

// 7.3 : port B programmed as an output, bit 0 = 1 drives the VSYNC line seen by the GATE ARRAY
// (FAKE VSYNC) : R52 is reset at the end of the 2nd HSYNC, as for a CRTC VSYNC. (It does not work
// on every CPC ; this is the behaviour of the CPC on which it works.)
TEST(Compendium_7, DISABLED_FakeVSyncIsSeenByTheGateArray)
{
   Machine m(CRTC::HD6845S);
   ASSERT_TRUE(UntilPosition(m, 100, 0));
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::Out(0xF700, 0x80));   // port B as an output
   z80::Append(p, z80::Out(0xF500, 0x01));   // VSYNC line high
   z80::Append(p, z80::kJrSelf);
   m.Run(p);
   ASSERT_TRUE(UntilPosition(m, 102, 46 + 16));
   EXPECT_EQ(0, m.Ga().interrupt_counter_);
}

/////////////////////////////////////////////////////////////
// 29.1 : CPC identification

// 29.1.2 : writing the PPI control register resets port C on a CPC with a real 8255 ; the ASIC of
// the CPC+ does not.
TEST(Compendium_29, PpiControlWordResetsPortCExceptOnTheCpcPlus)
{
   for (bool plus : { false, true })
   {
      SCOPED_TRACE(plus ? "CPC+" : "CPC old");
      Machine m(plus ? CRTC::AMS40489 : CRTC::HD6845S, plus ? "6128PLUS" : "6128");
      std::vector<unsigned char> p = { z80::kDi };
      z80::Append(p, z80::Out(0xF600, 0x0A));
      z80::Append(p, z80::Out(0xF700, 0x82));
      RunProgram(m, p);
      EXPECT_EQ(plus ? 0x0A : 0x00, m.Engine().GetPPI()->port_c_);
   }
}

// 29.1.3 : on a CPC with a real 8255, port B programmed as an output reads back the value written.
TEST(Compendium_29, PpiPortBOutputReadsBackOnTheCpcOld)
{
   Machine m(CRTC::HD6845S);
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::Out(0xF700, 0x80));
   z80::Append(p, z80::Out(0xF500, 0x5A));
   z80::Append(p, z80::In(0xF500, kResult));
   z80::Append(p, z80::Out(0xF700, 0x82));
   RunProgram(m, p);
   EXPECT_EQ(0x5A, m.Ram()[kResult]);
}
