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

// 5.1 (*) : the PAL (RAM configuration, &C0-&FF) is selected with A14 = 0 or 1 on the CPC+ (CRTC
// 3, 4) ; on the CPC "old" A14 must be 1 (only a few CPC select it with A14 = 0). The GATE ARRAY
// itself always needs A14 = 1.
TEST(Compendium_5, PalSelectedWithA14ZeroOnTheCpcPlus)
{
   Machine m(CRTC::AMS40489, "6128PLUS");
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::Out(0x3F00, 0xC1));   // RAM configuration 1
   RunProgram(m, p);
   EXPECT_EQ(1, m.Engine().GetMem()->GetRamBankConfig());
}

TEST(Compendium_5, PalNeedsA14OneOnTheCpcOld)
{
   Machine m(CRTC::HD6845S);
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::Out(0x3F00, 0xC1));
   RunProgram(m, p);
   EXPECT_EQ(0, m.Engine().GetMem()->GetRamBankConfig());
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
TEST(Compendium_7, InReadsTheVSyncOnItsFourthMicrosecond)
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

// 7.3 : port B programmed as an output, bit 0 = 1 puts the VSYNC line of the PPI high (FAKE
// VSYNC). The compendium found it does not work on some CPC and gives a wrong result on others :
// the PPI is not strong enough against the CRTC, which drives the line. The emulation follows the
// CRTC : a FAKE VSYNC does not reach the GATE ARRAY, R52 runs as without it.
TEST(Compendium_7, FakeVSyncDoesNotOverrideTheCrtc)
{
   int counter[2];
   for (int level = 0; level < 2; ++level)
   {
      Machine m(CRTC::HD6845S);
      ASSERT_TRUE(UntilPosition(m, 100, 0));
      std::vector<unsigned char> p = { z80::kDi };
      z80::Append(p, z80::Out(0xF700, 0x80));                              // port B as an output
      z80::Append(p, z80::Out(0xF500, static_cast<unsigned char>(level))); // VSYNC line of the PPI
      z80::Append(p, z80::kJrSelf);
      m.Run(p);
      ASSERT_TRUE(UntilPosition(m, 102, 46 + 16));
      counter[level] = m.Ga().interrupt_counter_;
   }
   EXPECT_EQ(counter[0], counter[1]);
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

/////////////////////////////////////////////////////////////
// 4.4.2, 4.4.3 : microsecond of each I/O instruction where the access is taken

namespace
{
// Writes : R12 is selected, 'setup' prepares the registers, 'io' writes R12. Returns the microsecond of
// the instruction (1 = its first) on whose end the CRTC first clocks with the new R12.
int WriteMicrosecond(CRTC::TypeCRTC type, const char* config, const std::vector<unsigned char>& setup,
                     const std::vector<unsigned char>& io)
{
   Machine m(type, config);
   std::vector<unsigned char> p = { z80::kDi, 0x01, 0x0C, 0xBC, 0xED, 0x49 };   // LD BC,&BC0C : OUT (C),C
   z80::Append(p, setup);
   // A NOP marks the start : InstructionStart follows PC, which an instruction with an operand only moves
   // once the operand is read
   const unsigned short at = (unsigned short)(kProgram + p.size());
   p.push_back(z80::kNop);
   z80::Append(p, io);
   z80::Append(p, z80::kJrSelf);
   m.Run(p);
   if (m.InstructionStart(at, nullptr) < 0) return -1;
   for (int c = 0; c < 4; ++c) m.Cycle();   // the NOP
   const unsigned char before = m.Crtc().registers_list_[12];
   for (int us = 1; us <= 8; ++us)
   {
      // Cycles 2, 3, 4 of the microsecond : sampled just before the clock edge (4th cycle)
      for (int c = 0; c < 2; ++c) m.Cycle();
      if (m.Crtc().registers_list_[12] != before) return us;
      for (int c = 0; c < 2; ++c) m.Cycle();
   }
   return -1;
}

// Reads : 'io' reads PPI port B (bit 0 : CRTC VSYNC) to kResult, 'setup' prepares the registers. The
// instruction starts k us before the VSYNC ; returns the last microsecond of the instruction (1 = its
// first) on which a rising VSYNC is still seen.
int ReadMicrosecond(const std::vector<unsigned char>& setup, const std::vector<unsigned char>& io)
{
   int last_seen = -1;
   for (int start = 52; start < 64; ++start)
   {
      Machine m(CRTC::HD6845S);
      if (!UntilPosition(m, kVSyncLine - 1, start)) return -2;
      std::vector<unsigned char> p = { z80::kDi };
      z80::Append(p, setup);
      const unsigned short at = (unsigned short)(kProgram + p.size());
      p.push_back(z80::kNop);   // marks the start (see WriteMicrosecond)
      z80::Append(p, io);
      z80::Append(p, z80::kJrSelf);
      m.Ram()[kResult] = 0;
      m.Run(p);
      if (m.InstructionStart(at, nullptr) < 0) return -3;
      for (int c = 0; c < 4; ++c) m.Cycle();   // the NOP
      // Microsecond of the instruction on which the VSYNC is high first (first cycle of each microsecond)
      int us = 1;
      while (!m.Sig().v_sync_ && us < 20)
      {
         for (int c = 0; c < 4; ++c) m.Cycle();
         ++us;
      }
      for (int i = 0; i < 4 * 20; ++i) m.Cycle();
      if ((m.Ram()[kResult] & 1) && us > last_seen) last_seen = us;
   }
   return last_seen;
}
}

// 4.4.3 : writes taken on the 3rd us of OUT (C),r8 / OUT (C),0 (4th on CRTC 3, 4 : the ASIC misses the
// I/O and takes it on the next window), the 3rd of OUT (n),A, the 5th of OUTI / OUTD, on every CRTC.
TEST(Compendium_4, WriteTakenOnTheMicrosecondOfEachInstruction)
{
   const std::vector<unsigned char> to_bd00 = { 0x01, 0x15, 0xBD };                                   // LD BC,&BD15
   const struct { CRTC::TypeCRTC type; const char* config; int out_c; } types[] = {
      { CRTC::HD6845S, "6128", 3 }, { CRTC::UM6845R, "6128", 3 }, { CRTC::MC6845, "6128", 3 },
      { CRTC::AMS40489, "6128PLUS", 4 }, { CRTC::AMS40226, "6128PLUS", 4 } };
   for (const auto& t : types)
   {
      SCOPED_TRACE(t.config);
      SCOPED_TRACE((int)t.type);
      EXPECT_EQ(t.out_c, WriteMicrosecond(t.type, t.config, to_bd00, { 0xED, 0x49 }));   // OUT (C),C
      EXPECT_EQ(t.out_c, WriteMicrosecond(t.type, t.config, to_bd00, { 0xED, 0x71 }));   // OUT (C),0
      EXPECT_EQ(3, WriteMicrosecond(t.type, t.config, { 0x3E, 0xBD }, { 0xD3, 0x00 })); // LD A,&BD : OUT (&00),A
      // LD HL,&5000 : LD (HL),&15 : LD BC,&BE00 (OUTI / OUTD decrement B first)
      const std::vector<unsigned char> hl = { 0x21, 0x00, 0x50, 0x36, 0x15, 0x01, 0x00, 0xBE };
      EXPECT_EQ(5, WriteMicrosecond(t.type, t.config, hl, { 0xED, 0xA3 }));   // OUTI
      EXPECT_EQ(5, WriteMicrosecond(t.type, t.config, hl, { 0xED, 0xAB }));   // OUTD
   }
}

// 4.4.3 : reads taken on the 4th us of IN r8,(C), INI, IND and the 3rd of IN A,(n).
TEST(Compendium_4, ReadTakenOnTheMicrosecondOfEachInstruction)
{
   const std::vector<unsigned char> store_a = { 0x32, (unsigned char)(kResult & 0xFF), (unsigned char)(kResult >> 8) };
   std::vector<unsigned char> in_c = { 0xED, 0x78 };   // IN A,(C)
   z80::Append(in_c, store_a);
   std::vector<unsigned char> in_n = { 0xDB, 0x00 };   // IN A,(&00) : port &F500
   z80::Append(in_n, store_a);
   const std::vector<unsigned char> hl = { 0x21, (unsigned char)(kResult & 0xFF), (unsigned char)(kResult >> 8) };
   std::vector<unsigned char> bc_hl = { 0x01, 0x00, 0xF5 };
   z80::Append(bc_hl, hl);
   EXPECT_EQ(4, ReadMicrosecond({ 0x01, 0x00, 0xF5 }, in_c));
   EXPECT_EQ(3, ReadMicrosecond({ 0x3E, 0xF5 }, in_n));
   EXPECT_EQ(4, ReadMicrosecond(bc_hl, { 0xED, 0xA2 }));   // INI
   EXPECT_EQ(4, ReadMicrosecond(bc_hl, { 0xED, 0xAA }));   // IND
}

// 12.5 : CRTC 3, an I/O on the CRTC with the ROM select active (A13 = 0 : OUT (C),0 with B = %00000001)
// is taken 1 us earlier, on the 3rd us of the OUT (C),r8 instead of the 4th ; not with B = %00100001
// (printer and PPI selected as well, without effect). The ASIC of the CRTC 4 is not concerned.
TEST(Compendium_12, Crtc3TakesAnIoWithTheRomSelectOneMicrosecondEarlier)
{
   const std::vector<unsigned char> rom_select = { 0x01, 0x00, 0x01 };   // LD BC,&0100
   const std::vector<unsigned char> no_rom_select = { 0x01, 0x00, 0x21 };   // LD BC,&2100
   const std::vector<unsigned char> out_c_0 = { 0xED, 0x71 };
   EXPECT_EQ(3, WriteMicrosecond(CRTC::AMS40489, "6128PLUS", rom_select, out_c_0));
   EXPECT_EQ(4, WriteMicrosecond(CRTC::AMS40489, "6128PLUS", no_rom_select, out_c_0));
   EXPECT_EQ(4, WriteMicrosecond(CRTC::AMS40226, "6128PLUS", rom_select, out_c_0));
   EXPECT_EQ(3, WriteMicrosecond(CRTC::HD6845S, "6128", rom_select, out_c_0));
}
