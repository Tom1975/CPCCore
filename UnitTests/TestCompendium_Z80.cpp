#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

// Tests written from "The Amstrad CPC CRTC Compendium" v1.11 (Logon System, CC BY-NC-ND 4.0),
// chapter 26 : duration of the Z80A instructions on CPC, in microseconds (the GATE ARRAY holds WAIT
// low 3 cycles out of 4, which aligns every memory or I/O access on the microsecond).
// A failing test is DISABLED_ : it documents a behaviour the emulation does not implement yet.

#include "CompendiumMachine.h"

#include "gtest/gtest.h"

#include <string>

using namespace compendium;

namespace
{
const int kNext = -1;   // the next instruction follows the measured one
const int kSelf = -2;   // the measured instruction repeats (LDIR, ...)
const unsigned short kJumpTarget = 0x5000;
const unsigned short kData = 0x3000;      // HL, IX, IY, (aa)
const unsigned short kStack = 0x3800;     // SP ; (SP) = kJumpTarget

struct Instruction
{
   const char* name;
   std::vector<unsigned char> setup;   // run before the measured instruction
   std::vector<unsigned char> code;
   int us;
   int target;                         // kNext, kSelf, or the address of the next instruction
};

// Common setup : DI, ROM off, HL = IX = IY = kData, DE = kData + &100, BC = &FF01 (port &FF01 :
// no device), SP = kStack, A = 1.
std::vector<unsigned char> CommonSetup()
{
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::OutGateArray(z80::kRmrRamOnly));
   z80::Append(p, { 0x21, 0x00, 0x30, 0xDD, 0x21, 0x00, 0x30, 0xFD, 0x21, 0x00, 0x30,
                    0x11, 0x00, 0x31, 0x01, 0x01, 0xFF, 0x31, 0x00, 0x38, 0x3E, 0x01 });
   return p;
}

// Microseconds from the fetch of the instruction to the fetch of the next one. -1 : not seen.
// The emulated Z80 may start a fetch (PC moves) before the microsecond boundary and insert the
// WAIT cycles inside it : each fetch is aligned on the next boundary. The boundaries come from the
// CRTC, which moves C0 on the last cycle of each microsecond.
int Measure(Machine& m, const Instruction& i)
{
   // Every possible next instruction is a DI (1 us, like a NOP, and no interrupt after an EI).
   for (unsigned int a = 0; a < 0x40; ++a) m.Ram()[a] = z80::kDi;
   for (unsigned int a = kJumpTarget; a < kJumpTarget + 0x10; ++a) m.Ram()[a] = z80::kDi;
   for (unsigned int a = kData; a < kData + 0x200; ++a) m.Ram()[a] = 0;
   m.Ram()[kStack] = kJumpTarget & 0xFF;
   m.Ram()[kStack + 1] = kJumpTarget >> 8;

   std::vector<unsigned char> p = CommonSetup();
   z80::Append(p, i.setup);
   const unsigned short at = (unsigned short)(kProgram + p.size());
   z80::Append(p, i.code);
   for (int n = 0; n < 8; ++n) p.push_back(z80::kDi);
   m.Run(p);

   const unsigned short next = (unsigned short)(i.target == kNext ? at + i.code.size()
                                              : i.target == kSelf ? at : i.target);
   const unsigned char hcc = m.Crtc().hcc_;
   if (m.CyclesUntil([&]() { return m.Crtc().hcc_ != hcc; }, 8) < 0) return -1;
   const long long boundary = (long long)m.Cycles() + 1;
   auto microsecond = [&](long long cycle) { return (cycle - boundary + 4 * 100000 + 3) / 4; };

   if (m.CyclesUntil([&]() { return m.Cpu().pc_ == at + 1; }, 4 * 200) < 0) return -1;
   const long long start = microsecond((long long)m.Cycles());
   unsigned short previous = m.Cpu().pc_;
   for (int cycles = 1; cycles < 4 * 20; ++cycles)
   {
      m.Cycle();
      const unsigned short pc = m.Cpu().pc_;
      if (pc == (unsigned short)(next + 1) && previous != pc) return (int)(microsecond((long long)m.Cycles()) - start);
      previous = pc;
   }
   return -1;
}

void CheckDurations(const std::vector<Instruction>& instructions)
{
   Machine m(CRTC::HD6845S);
   for (const Instruction& i : instructions)
   {
      SCOPED_TRACE(i.name);
      EXPECT_EQ(i.us, Measure(m, i));
   }
}

// Flags for the conditional instructions : XOR A -> Z = 1, C = 0 ; SCF -> C = 1.
const std::vector<unsigned char> kZ = { 0xAF };
const std::vector<unsigned char> kC = { 0x37 };
const std::vector<unsigned char> kNone = {};
// The repeated I/O instructions with B = 2 / 1 address &01xx-&02xx : CRTC (A14 = 0) and GATE ARRAY
// (A15 = 0) are written. R15 (cursor) is selected first : the CRTC writes are harmless.
const std::vector<unsigned char> kR15 = { 0x01, 0x0F, 0xBC, 0xED, 0x49 };
std::vector<unsigned char> operator+(std::vector<unsigned char> a, const std::vector<unsigned char>& b)
{
   a.insert(a.end(), b.begin(), b.end());
   return a;
}

// Table of the compendium, pages 281-282. Each row is checked on one or more encodings.
const std::vector<Instruction> kTable = {
   // 8 bits arithmetic and logic
   { "ADD A,(HL)", kNone, { 0x86 }, 2, kNext },       { "ADD A,(IX+d)", kNone, { 0xDD, 0x86, 5 }, 5, kNext },
   { "ADD A,(IY+d)", kNone, { 0xFD, 0x86, 5 }, 5, kNext },
   { "ADD A,B", kNone, { 0x80 }, 1, kNext },          { "ADD A,A", kNone, { 0x87 }, 1, kNext },
   { "ADD A,HX", kNone, { 0xDD, 0x84 }, 2, kNext },   { "ADD A,LY", kNone, { 0xFD, 0x85 }, 2, kNext },
   { "ADD A,d", kNone, { 0xC6, 1 }, 2, kNext },
   { "ADC A,(HL)", kNone, { 0x8E }, 2, kNext },       { "ADC A,(IX+d)", kNone, { 0xDD, 0x8E, 5 }, 5, kNext },
   { "ADC A,C", kNone, { 0x89 }, 1, kNext },          { "ADC A,LX", kNone, { 0xDD, 0x8D }, 2, kNext },
   { "ADC A,d", kNone, { 0xCE, 1 }, 2, kNext },
   { "SUB (HL)", kNone, { 0x96 }, 2, kNext },         { "SUB (IY+d)", kNone, { 0xFD, 0x96, 5 }, 5, kNext },
   { "SUB D", kNone, { 0x92 }, 1, kNext },            { "SUB HY", kNone, { 0xFD, 0x94 }, 2, kNext },
   { "SUB d", kNone, { 0xD6, 1 }, 2, kNext },
   { "SBC A,(HL)", kNone, { 0x9E }, 2, kNext },       { "SBC A,(IX+d)", kNone, { 0xDD, 0x9E, 5 }, 5, kNext },
   { "SBC A,E", kNone, { 0x9B }, 1, kNext },          { "SBC A,HX", kNone, { 0xDD, 0x9C }, 2, kNext },
   { "SBC A,d", kNone, { 0xDE, 1 }, 2, kNext },
   { "AND (HL)", kNone, { 0xA6 }, 2, kNext },         { "AND (IX+d)", kNone, { 0xDD, 0xA6, 5 }, 5, kNext },
   { "AND H", kNone, { 0xA4 }, 1, kNext },            { "AND LX", kNone, { 0xDD, 0xA5 }, 2, kNext },
   { "AND d", kNone, { 0xE6, 1 }, 2, kNext },
   { "XOR (HL)", kNone, { 0xAE }, 2, kNext },         { "XOR (IY+d)", kNone, { 0xFD, 0xAE, 5 }, 5, kNext },
   { "XOR L", kNone, { 0xAD }, 1, kNext },            { "XOR HY", kNone, { 0xFD, 0xAC }, 2, kNext },
   { "XOR d", kNone, { 0xEE, 1 }, 2, kNext },
   { "OR (HL)", kNone, { 0xB6 }, 2, kNext },          { "OR (IX+d)", kNone, { 0xDD, 0xB6, 5 }, 5, kNext },
   { "OR B", kNone, { 0xB0 }, 1, kNext },             { "OR LY", kNone, { 0xFD, 0xB5 }, 2, kNext },
   { "OR d", kNone, { 0xF6, 1 }, 2, kNext },
   { "CP (HL)", kNone, { 0xBE }, 2, kNext },          { "CP (IX+d)", kNone, { 0xDD, 0xBE, 5 }, 5, kNext },
   { "CP C", kNone, { 0xB9 }, 1, kNext },             { "CP HX", kNone, { 0xDD, 0xBC }, 2, kNext },
   { "CP d", kNone, { 0xFE, 1 }, 2, kNext },
   { "INC A", kNone, { 0x3C }, 1, kNext },            { "INC (HL)", kNone, { 0x34 }, 3, kNext },
   { "INC (IX+d)", kNone, { 0xDD, 0x34, 5 }, 6, kNext }, { "INC HX", kNone, { 0xDD, 0x24 }, 2, kNext },
   { "DEC E", kNone, { 0x1D }, 1, kNext },            { "DEC (HL)", kNone, { 0x35 }, 3, kNext },
   { "DEC (IY+d)", kNone, { 0xFD, 0x35, 5 }, 6, kNext }, { "DEC LY", kNone, { 0xFD, 0x2D }, 2, kNext },
   // 16 bits arithmetic
   { "ADD HL,BC", kNone, { 0x09 }, 3, kNext },        { "ADD HL,SP", kNone, { 0x39 }, 3, kNext },
   { "ADD IX,DE", kNone, { 0xDD, 0x19 }, 4, kNext },  { "ADD IY,IY", kNone, { 0xFD, 0x29 }, 4, kNext },
   { "ADC HL,DE", kNone, { 0xED, 0x5A }, 4, kNext },  { "SBC HL,BC", kNone, { 0xED, 0x42 }, 4, kNext },
   { "INC BC", kNone, { 0x03 }, 2, kNext },           { "INC SP", kNone, { 0x33 }, 2, kNext },
   { "DEC DE", kNone, { 0x1B }, 2, kNext },           { "INC IX", kNone, { 0xDD, 0x23 }, 3, kNext },
   { "DEC IY", kNone, { 0xFD, 0x2B }, 3, kNext },
   // General purpose
   { "CCF", kNone, { 0x3F }, 1, kNext },              { "SCF", kNone, { 0x37 }, 1, kNext },
   { "CPL", kNone, { 0x2F }, 1, kNext },              { "DAA", kNone, { 0x27 }, 1, kNext },
   { "NEG", kNone, { 0xED, 0x44 }, 2, kNext },        { "NOP", kNone, { 0x00 }, 1, kNext },
   { "DI", kNone, { 0xF3 }, 1, kNext },               { "EI", kNone, { 0xFB }, 1, kNext },
   { "IM 0", kNone, { 0xED, 0x46 }, 2, kNext },       { "IM 1", kNone, { 0xED, 0x56 }, 2, kNext },
   { "IM 2", kNone, { 0xED, 0x5E }, 2, kNext },
   { "EX AF,AF'", kNone, { 0x08 }, 1, kNext },        { "EX DE,HL", kNone, { 0xEB }, 1, kNext },
   { "EXX", kNone, { 0xD9 }, 1, kNext },              { "EX (SP),HL", kNone, { 0xE3 }, 6, kNext },
   { "EX (SP),IX", kNone, { 0xDD, 0xE3 }, 7, kNext },
   // Rotations and shifts
   { "RLCA", kNone, { 0x07 }, 1, kNext },             { "RRCA", kNone, { 0x0F }, 1, kNext },
   { "RLA", kNone, { 0x17 }, 1, kNext },              { "RRA", kNone, { 0x1F }, 1, kNext },
   { "RLD", kNone, { 0xED, 0x6F }, 5, kNext },        { "RRD", kNone, { 0xED, 0x67 }, 5, kNext },
   { "RLC B", kNone, { 0xCB, 0x00 }, 2, kNext },      { "RLC (HL)", kNone, { 0xCB, 0x06 }, 4, kNext },
   { "RLC (IX+d)", kNone, { 0xDD, 0xCB, 5, 0x06 }, 7, kNext }, { "RLC (IX+d),B", kNone, { 0xDD, 0xCB, 5, 0x00 }, 7, kNext },
   { "RRC C", kNone, { 0xCB, 0x09 }, 2, kNext },      { "RRC (HL)", kNone, { 0xCB, 0x0E }, 4, kNext },
   { "RRC (IY+d)", kNone, { 0xFD, 0xCB, 5, 0x0E }, 7, kNext }, { "RRC (IY+d),C", kNone, { 0xFD, 0xCB, 5, 0x09 }, 7, kNext },
   { "RL D", kNone, { 0xCB, 0x12 }, 2, kNext },       { "RL (HL)", kNone, { 0xCB, 0x16 }, 4, kNext },
   { "RL (IX+d)", kNone, { 0xDD, 0xCB, 5, 0x16 }, 7, kNext }, { "RL (IX+d),D", kNone, { 0xDD, 0xCB, 5, 0x12 }, 7, kNext },
   { "RR E", kNone, { 0xCB, 0x1B }, 2, kNext },       { "RR (HL)", kNone, { 0xCB, 0x1E }, 4, kNext },
   { "RR (IX+d)", kNone, { 0xDD, 0xCB, 5, 0x1E }, 7, kNext }, { "RR (IX+d),E", kNone, { 0xDD, 0xCB, 5, 0x1B }, 7, kNext },
   { "SLA H", kNone, { 0xCB, 0x24 }, 2, kNext },      { "SLA (HL)", kNone, { 0xCB, 0x26 }, 4, kNext },
   { "SLA (IX+d)", kNone, { 0xDD, 0xCB, 5, 0x26 }, 7, kNext }, { "SLA (IX+d),H", kNone, { 0xDD, 0xCB, 5, 0x24 }, 7, kNext },
   { "SRA L", kNone, { 0xCB, 0x2D }, 2, kNext },      { "SRA (HL)", kNone, { 0xCB, 0x2E }, 4, kNext },
   { "SRA (IX+d)", kNone, { 0xDD, 0xCB, 5, 0x2E }, 7, kNext },
   { "SLL A", kNone, { 0xCB, 0x37 }, 2, kNext },      { "SLL (HL)", kNone, { 0xCB, 0x36 }, 4, kNext },
   { "SLL (IX+d)", kNone, { 0xDD, 0xCB, 5, 0x36 }, 7, kNext }, { "SLL (IX+d),A", kNone, { 0xDD, 0xCB, 5, 0x37 }, 7, kNext },
   { "SRL B", kNone, { 0xCB, 0x38 }, 2, kNext },      { "SRL (HL)", kNone, { 0xCB, 0x3E }, 4, kNext },
   { "SRL (IY+d)", kNone, { 0xFD, 0xCB, 5, 0x3E }, 7, kNext }, { "SRL (IY+d),B", kNone, { 0xFD, 0xCB, 5, 0x38 }, 7, kNext },
   // Bits
   { "BIT 0,B", kNone, { 0xCB, 0x40 }, 2, kNext },    { "BIT 7,(HL)", kNone, { 0xCB, 0x7E }, 3, kNext },
   { "BIT 3,(IX+d)", kNone, { 0xDD, 0xCB, 5, 0x5E }, 6, kNext },
   { "SET 1,C", kNone, { 0xCB, 0xC9 }, 2, kNext },    { "SET 2,(HL)", kNone, { 0xCB, 0xD6 }, 4, kNext },
   { "SET 4,(IY+d)", kNone, { 0xFD, 0xCB, 5, 0xE6 }, 7, kNext }, { "SET 4,(IY+d),A", kNone, { 0xFD, 0xCB, 5, 0xE7 }, 7, kNext },
   { "RES 5,D", kNone, { 0xCB, 0xAA }, 2, kNext },    { "RES 6,(HL)", kNone, { 0xCB, 0xB6 }, 4, kNext },
   { "RES 0,(IX+d)", kNone, { 0xDD, 0xCB, 5, 0x86 }, 7, kNext }, { "RES 0,(IX+d),E", kNone, { 0xDD, 0xCB, 5, 0x83 }, 7, kNext },
   // 8 bits loads
   { "LD B,C", kNone, { 0x41 }, 1, kNext },           { "LD A,A", kNone, { 0x7F }, 1, kNext },
   { "LD D,(HL)", kNone, { 0x56 }, 2, kNext },        { "LD E,(IX+d)", kNone, { 0xDD, 0x5E, 5 }, 5, kNext },
   { "LD (HL),B", kNone, { 0x70 }, 2, kNext },        { "LD (HL),d", kNone, { 0x36, 1 }, 3, kNext },
   { "LD (IX+d),C", kNone, { 0xDD, 0x71, 5 }, 5, kNext }, { "LD (IY+d),d", kNone, { 0xFD, 0x36, 5, 1 }, 6, kNext },
   { "LD (BC),A", { 0x01, 0x00, 0x30 }, { 0x02 }, 2, kNext }, { "LD (DE),A", kNone, { 0x12 }, 2, kNext },
   { "LD A,(BC)", { 0x01, 0x00, 0x30 }, { 0x0A }, 2, kNext }, { "LD A,(DE)", kNone, { 0x1A }, 2, kNext },
   { "LD (aa),A", kNone, { 0x32, 0x00, 0x30 }, 4, kNext }, { "LD A,(aa)", kNone, { 0x3A, 0x00, 0x30 }, 4, kNext },
   { "LD H,d", kNone, { 0x26, 0x30 }, 2, kNext },     { "LD HX,A", kNone, { 0xDD, 0x67 }, 2, kNext },
   { "LD LY,B", kNone, { 0xFD, 0x68 }, 2, kNext },    { "LD HX,d", kNone, { 0xDD, 0x26, 0x30 }, 3, kNext },
   { "LD I,A", kNone, { 0xED, 0x47 }, 3, kNext },     { "LD A,I", kNone, { 0xED, 0x57 }, 3, kNext },
   { "LD R,A", kNone, { 0xED, 0x4F }, 3, kNext },     { "LD A,R", kNone, { 0xED, 0x5F }, 3, kNext },
   // 16 bits loads
   { "LD BC,dd", kNone, { 0x01, 0x01, 0xFF }, 3, kNext }, { "LD SP,dd", kNone, { 0x31, 0x00, 0x38 }, 3, kNext },
   { "LD IX,dd", kNone, { 0xDD, 0x21, 0x00, 0x30 }, 4, kNext },
   { "LD (aa),HL", kNone, { 0x22, 0x00, 0x30 }, 5, kNext }, { "LD (aa),DE", kNone, { 0xED, 0x53, 0x00, 0x30 }, 6, kNext },
   { "LD (aa),IY", kNone, { 0xFD, 0x22, 0x00, 0x30 }, 6, kNext },
   { "LD HL,(aa)", kNone, { 0x2A, 0x00, 0x30 }, 5, kNext }, { "LD BC,(aa)", { 0x01, 0x01, 0xFF }, { 0xED, 0x4B, 0x10, 0x30 }, 6, kNext },
   { "LD IX,(aa)", kNone, { 0xDD, 0x2A, 0x00, 0x30 }, 6, kNext },
   { "LD SP,HL", { 0x21, 0x00, 0x38 }, { 0xF9 }, 2, kNext }, { "LD SP,IX", { 0xDD, 0x21, 0x00, 0x38 }, { 0xDD, 0xF9 }, 3, kNext },
   { "PUSH BC", kNone, { 0xC5 }, 4, kNext },          { "PUSH IX", kNone, { 0xDD, 0xE5 }, 5, kNext },
   { "POP DE", kNone, { 0xD1 }, 3, kNext },           { "POP IY", kNone, { 0xFD, 0xE1 }, 4, kNext },
   // Blocks (BC = 1 : last iteration ; BC = 2 : repeated)
   { "LDI", kNone, { 0xED, 0xA0 }, 5, kNext },        { "LDD", kNone, { 0xED, 0xA8 }, 5, kNext },
   { "LDIR (BC=2)", { 0x01, 0x02, 0x00 }, { 0xED, 0xB0 }, 6, kSelf }, { "LDIR (BC=1)", { 0x01, 0x01, 0x00 }, { 0xED, 0xB0 }, 5, kNext },
   { "LDDR (BC=2)", { 0x01, 0x02, 0x00 }, { 0xED, 0xB8 }, 6, kSelf }, { "LDDR (BC=1)", { 0x01, 0x01, 0x00 }, { 0xED, 0xB8 }, 5, kNext },
   { "CPI", kNone, { 0xED, 0xA1 }, 4, kNext },        { "CPD", kNone, { 0xED, 0xA9 }, 4, kNext },
   { "CPIR (BC=2)", { 0x01, 0x02, 0x00 }, { 0xED, 0xB1 }, 6, kSelf }, { "CPIR (BC=1)", { 0x01, 0x01, 0x00 }, { 0xED, 0xB1 }, 4, kNext },
   { "CPDR (BC=2)", { 0x01, 0x02, 0x00 }, { 0xED, 0xB9 }, 6, kSelf }, { "CPDR (BC=1)", { 0x01, 0x01, 0x00 }, { 0xED, 0xB9 }, 4, kNext },
   // Inputs / outputs (port &FFxx : no device ; repeated ones : B = 2 / 1, C = &FF, see kR15)
   { "IN A,(C)", kNone, { 0xED, 0x78 }, 4, kNext },   { "IN D,(C)", kNone, { 0xED, 0x50 }, 4, kNext },
   { "IN F,(C)", kNone, { 0xED, 0x70 }, 4, kNext },   { "IN A,(d)", { 0x3E, 0xFF }, { 0xDB, 0xFF }, 3, kNext },
   { "OUT (C),A", kNone, { 0xED, 0x79 }, 4, kNext },  { "OUT (C),E", kNone, { 0xED, 0x59 }, 4, kNext },
   { "OUT (C),0", kNone, { 0xED, 0x71 }, 4, kNext },  { "OUT (d),A", { 0x3E, 0xFF }, { 0xD3, 0xFF }, 3, kNext },
   { "INI", kNone, { 0xED, 0xA2 }, 5, kNext },        { "IND", kNone, { 0xED, 0xAA }, 5, kNext },
   { "OUTI", kNone, { 0xED, 0xA3 }, 5, kNext },       { "OUTD", kNone, { 0xED, 0xAB }, 5, kNext },
   { "INIR (B=2)", kR15 + std::vector<unsigned char>{ 0x01, 0xFF, 0x02 }, { 0xED, 0xB2 }, 6, kSelf }, { "INIR (B=1)", kR15 + std::vector<unsigned char>{ 0x01, 0xFF, 0x01 }, { 0xED, 0xB2 }, 5, kNext },
   { "INDR (B=2)", kR15 + std::vector<unsigned char>{ 0x01, 0xFF, 0x02 }, { 0xED, 0xBA }, 6, kSelf }, { "INDR (B=1)", kR15 + std::vector<unsigned char>{ 0x01, 0xFF, 0x01 }, { 0xED, 0xBA }, 5, kNext },
   { "OTIR (B=2)", kR15 + std::vector<unsigned char>{ 0x01, 0xFF, 0x02 }, { 0xED, 0xB3 }, 6, kSelf }, { "OTIR (B=1)", kR15 + std::vector<unsigned char>{ 0x01, 0xFF, 0x01 }, { 0xED, 0xB3 }, 5, kNext },
   { "OTDR (B=2)", kR15 + std::vector<unsigned char>{ 0x01, 0xFF, 0x02 }, { 0xED, 0xBB }, 6, kSelf }, { "OTDR (B=1)", kR15 + std::vector<unsigned char>{ 0x01, 0xFF, 0x01 }, { 0xED, 0xBB }, 5, kNext },
   // Jumps, calls, returns (taken / not taken)
   { "JP aa", kNone, { 0xC3, 0x00, 0x50 }, 3, kJumpTarget },
   { "JP Z,aa (taken)", kZ, { 0xCA, 0x00, 0x50 }, 3, kJumpTarget }, { "JP NZ,aa (not taken)", kZ, { 0xC2, 0x00, 0x50 }, 3, kNext },
   { "JP (HL)", { 0x21, 0x00, 0x50 }, { 0xE9 }, 1, kJumpTarget }, { "JP (IX)", { 0xDD, 0x21, 0x00, 0x50 }, { 0xDD, 0xE9 }, 2, kJumpTarget },
   { "JR a", kNone, { 0x18, 0x00 }, 3, kNext },
   { "JR Z,a (taken)", kZ, { 0x28, 0x00 }, 3, kNext }, { "JR NZ,a (not taken)", kZ, { 0x20, 0x00 }, 2, kNext },
   { "JR C,a (taken)", kC, { 0x38, 0x00 }, 3, kNext }, { "JR NC,a (not taken)", kC, { 0x30, 0x00 }, 2, kNext },
   { "DJNZ (taken)", { 0x06, 0x02 }, { 0x10, 0x00 }, 4, kNext }, { "DJNZ (not taken)", { 0x06, 0x01 }, { 0x10, 0x00 }, 3, kNext },
   { "CALL aa", kNone, { 0xCD, 0x00, 0x50 }, 5, kJumpTarget },
   { "CALL Z,aa (taken)", kZ, { 0xCC, 0x00, 0x50 }, 5, kJumpTarget }, { "CALL NZ,aa (not taken)", kZ, { 0xC4, 0x00, 0x50 }, 3, kNext },
   { "RET", kNone, { 0xC9 }, 3, kJumpTarget },
   { "RET Z (taken)", kZ, { 0xC8 }, 4, kJumpTarget }, { "RET NZ (not taken)", kZ, { 0xC0 }, 2, kNext },
   { "RETI", kNone, { 0xED, 0x4D }, 4, kJumpTarget }, { "RETN", kNone, { 0xED, 0x45 }, 4, kJumpTarget },
   { "RST 38h", kNone, { 0xFF }, 4, 0x38 },           { "RST 08h", kNone, { 0xCF }, 4, 0x08 },
};
}

// 26 : durations of the table (pages 281-282).
TEST(Compendium_26, InstructionDurations)
{
   CheckDurations(kTable);
}
