#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

// Tests written from "The Amstrad CPC CRTC Compendium" v1.11 (Logon System, CC BY-NC-ND 4.0),
// chapters 8 (RAM written by the Z80A and read by the GATE ARRAY) and 9 (GATE ARRAY : pixels, inks,
// graphic mode).
// A failing test is DISABLED_ : it documents a behaviour the emulation does not implement yet.

#include "CompendiumMachine.h"

#include "gtest/gtest.h"

using namespace compendium;

namespace
{
const unsigned short kScreen = 0xC000;   // R12/R13 = &3000 (firmware)
const int kR1 = 40;
const int kLine = 50;                     // a displayed line (C4 = 6, C9 = 2)
const unsigned char kBorderColour = 0x58;
// Hardware colours of the pens 0 to 15 : all different, all different from the border.
const unsigned char kPenColours[16] = { 0x54, 0x44, 0x55, 0x5C, 0x4C, 0x4E, 0x4A, 0x52,
                                        0x4B, 0x46, 0x57, 0x5E, 0x40, 0x5F, 0x53, 0x5A };

std::vector<unsigned char> RmrMode(int mode) { return z80::OutGateArray((unsigned char)(z80::kRmrRamOnly - 1 + mode)); }

// Screen filled with 'byte', mode 'mode', the 16 inks and the border set ; one frame later.
void Prepare(Machine& m, int mode, unsigned char byte)
{
   for (unsigned int a = kScreen; a <= 0xFFFF; ++a) m.Ram()[a] = byte;
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, RmrMode(mode));
   for (int pen = 0; pen < 16; ++pen)
   {
      z80::Append(p, z80::OutGateArray((unsigned char)pen));
      z80::Append(p, z80::OutGateArray(kPenColours[pen]));
   }
   z80::Append(p, z80::OutGateArray(0x10));
   z80::Append(p, z80::OutGateArray(kBorderColour));
   z80::Append(p, z80::kJrSelf);
   m.Run(p);
   for (int i = 0; i < 4 * 19968 * 2; ++i) m.Cycle();
}

const int kBorder = -2;
const int kBlack = -3;
// Pen of a pixel (from the inks of the GATE ARRAY), kBorder, kBlack or -1.
int PenOf(Machine& m, int pixel)
{
   if (pixel == (int)0xFF000000) return kBlack;
   if (pixel == (int)m.Ga().video_border_[0]) return kBorder;
   for (int pen = 0; pen < 16; ++pen)
      if (pixel == (int)m.Ga().ink_list_[pen]) return pen;
   return -1;
}

// Pixels of 'line' drawn during the microseconds c0_first .. c0_first + count - 1.
std::vector<int> LinePixels(Machine& m, int line, int c0_first, int count)
{
   std::vector<int> pixels;
   for (int c0 = c0_first; c0 < c0_first + count; ++c0)
   {
      const Machine::Block* b = m.FindBlock(line, c0);
      for (int i = 0; i < 16; ++i) pixels.push_back(b != nullptr ? b->pixels[i] : 0x12345678);
   }
   return pixels;
}

// Records the blocks of one frame.
void RecordFrame(Machine& m)
{
   m.RecordBlocks(true);
   for (int i = 0; i < 4 * 19968; ++i) m.Cycle();
   m.RecordBlocks(false);
}
}

/////////////////////////////////////////////////////////////
// 9.1 : pixelisation

// 9.1 : bits of a byte for each pixel. Mode 0 : A0 B0 A2 B2 A1 B1 A3 B3 (2 pixels of 4 Mode 2
// pixels) ; mode 1 : A0 B0 C0 D0 A1 B1 C1 D1 (4 pixels of 2) ; mode 2 : one pixel per bit, bit 7
// first ; mode 3 : A0 B0 x x A1 B1 x x (2 pixels of 4, 4 colours).
TEST(Compendium_9, PixelBitsOfEachMode)
{
   const unsigned char byte = 0x9C;   // 1001 1100
   const int expected[4][8] = {
      { 3, 3, 3, 3, 6, 6, 6, 6 },     // A = b1 b5 b3 b7 = 0011, B = b0 b4 b2 b6 = 0110
      { 3, 3, 2, 2, 0, 0, 1, 1 },     // A = b3 b7, B = b2 b6, C = b1 b5, D = b0 b4
      { 1, 0, 0, 1, 1, 1, 0, 0 },
      { 3, 3, 3, 3, 2, 2, 2, 2 },     // A = b3 b7, B = b2 b6
   };
   for (int mode = 0; mode < 4; ++mode)
   {
      SCOPED_TRACE(testing::Message() << "mode " << mode);
      Machine m(CRTC::HD6845S);
      Prepare(m, mode, byte);
      RecordFrame(m);
      const Machine::Block* b = m.FindBlock(kLine, 20);
      ASSERT_NE(nullptr, b);
      for (int i = 0; i < 16; ++i)
         EXPECT_EQ(expected[mode][i % 8], PenOf(m, b->pixels[i])) << "pixel " << i;
   }
}

namespace
{
// Position, in Mode 2 pixels from the start of the block drawn on C0 = 0, of the first displayed
// pixel of 'kLine' ; and from the block drawn on C0 = R1, of the first border pixel.
void DisplayEdges(Machine& m, int mode, int* start, int* end)
{
   Prepare(m, mode, 0xFF);
   RecordFrame(m);
   std::vector<int> left = LinePixels(m, kLine, 0, 3);
   std::vector<int> right = LinePixels(m, kLine, kR1, 3);
   *start = *end = -100;
   for (int p = 0; p < (int)left.size(); ++p)
      if (PenOf(m, left[p]) >= 0) { *start = p; break; }
   for (int p = 0; p < (int)right.size(); ++p)
      if (PenOf(m, right[p]) == kBorder) { *end = p; break; }
}
}

// 9.1 : on the GATE ARRAY 40007, 40008, 40010 (and the ASIC 40226), the Mode 2 pixels are drawn
// 1/16 us early : the border stops one pixel earlier and starts one pixel earlier on C0 = R1.
// DISABLED : not emulated (the type of GATE ARRAY, GateArray::type_gate_array_, is never used).
TEST(Compendium_9, DISABLED_Mode2PixelsAreOnePixelEarlyOnTheGateArray40010)
{
   Machine m1(CRTC::HD6845S), m2(CRTC::HD6845S);
   int start1, end1, start2, end2;
   DisplayEdges(m1, 1, &start1, &end1);
   DisplayEdges(m2, 2, &start2, &end2);
   ASSERT_GE(start1, 0);
   ASSERT_GE(end1, 0);
   EXPECT_EQ(start1 - 1, start2);
   EXPECT_EQ(end1 - 1, end2);
}

// 9.1 : the ASIC 40489 of the CPC+ aligns the pixels of every mode.
TEST(Compendium_9, PixelsOfEveryModeAlignedOnTheCpcPlus)
{
   Machine m1(CRTC::AMS40489, "6128PLUS"), m2(CRTC::AMS40489, "6128PLUS");
   int start1, end1, start2, end2;
   DisplayEdges(m1, 1, &start1, &end1);
   DisplayEdges(m2, 2, &start2, &end2);
   ASSERT_GE(start1, 0);
   ASSERT_GE(end1, 0);
   EXPECT_EQ(start1, start2);
   EXPECT_EQ(end1, end2);
}

/////////////////////////////////////////////////////////////
// 9.2.2 : ink update

namespace
{
enum class InkInstruction { kOuti, kOutCR8, kOutNA };

// Pen 0 (screen of 0) gets a new colour by 'instruction' on kLine. Returns the position of the
// first pixel with the new colour, in Mode 2 pixels from the start of the block drawn during the
// microsecond of the I/O (3rd microsecond of OUT (C),r8 and OUT (n),A, 5th of OUTI).
int FirstPixelWithTheNewInk(CRTC::TypeCRTC type, const char* config, int mode, InkInstruction instruction)
{
   Machine m(type, config);
   Prepare(m, mode, 0x00);
   if (!m.CyclesUntil([&]() { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_ == kLine && m.Crtc().hcc_ == 5; }, 4 * 20000)) return -100;
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::OutGateArray(0x00));   // PENR : pen 0
   int io_microsecond = 3;
   switch (instruction)
   {
   case InkInstruction::kOutCR8:
      z80::Append(p, { 0x3E, 0x4C });          // LD A,&4C
      break;
   case InkInstruction::kOutNA:
      z80::Append(p, { 0x3E, 0x7F });          // LD A,&7F : port &7Fxx and INKR (colour &1F)
      break;
   case InkInstruction::kOuti:
      m.Ram()[0x3000] = 0x4C;
      z80::Append(p, { 0x01, 0x00, 0x80, 0x21, 0x00, 0x30 });   // LD BC,&8000 : LD HL,&3000
      io_microsecond = 5;
      break;
   }
   const unsigned short at = (unsigned short)(kProgram + p.size());
   switch (instruction)
   {
   case InkInstruction::kOutCR8: z80::Append(p, { 0xED, 0x79 }); break;   // OUT (C),A
   case InkInstruction::kOutNA: z80::Append(p, { 0xD3, 0x00 }); break;    // OUT (&00),A
   case InkInstruction::kOuti: z80::Append(p, { 0xED, 0xA3 }); break;     // OUTI
   }
   z80::Append(p, z80::kJrSelf);
   m.RecordBlocks(true);
   m.Run(p);
   int line = -1;
   const int start = m.InstructionStart(at, &line);
   for (int i = 0; i < 4 * 30; ++i) m.Cycle();
   m.RecordBlocks(false);
   const int io = start + io_microsecond - 1;
   const std::vector<int> pixels = LinePixels(m, line, io - 3, 7);
   const int old_colour = pixels[0];
   const int new_colour = pixels[pixels.size() - 1];
   if (old_colour == new_colour) return -101;
   for (int i = 0; i < (int)pixels.size(); ++i)
      if (pixels[i] == new_colour) return i - 3 * 16;
   return -102;
}
}

// 9.2.2 : CRTC 0, 1, 2 (GATE ARRAY 40010) : the new colour is drawn from the 2nd byte of the
// character displayed during the microsecond of the I/O, whatever the mode (Mode 2 pixel 120 of the
// schematics for OUT (C),r8 and OUT (n),A started on C0vs = 6, 152 for OUTI).
// DISABLED : the new colour comes one microsecond late (pixel 24) : the GATE ARRAY draws the block
// of the microsecond k + 1 at the end of the microsecond k, an ink written during k + 1 changes the
// following block.
TEST(Compendium_9, DISABLED_InkTakenOnTheSecondByteOfTheIoMicrosecond)
{
   for (int mode : { 0, 1, 2 })
   {
      SCOPED_TRACE(testing::Message() << "mode " << mode);
      EXPECT_EQ(8, FirstPixelWithTheNewInk(CRTC::HD6845S, "6128", mode, InkInstruction::kOutCR8));
      EXPECT_EQ(8, FirstPixelWithTheNewInk(CRTC::HD6845S, "6128", mode, InkInstruction::kOutNA));
      EXPECT_EQ(8, FirstPixelWithTheNewInk(CRTC::HD6845S, "6128", mode, InkInstruction::kOuti));
   }
}

// 9.2.2 : CRTC 3 (ASIC 40489) : same position, whatever the mode.
// DISABLED : one microsecond late, and the position depends on the mode (pixel 16 in mode 0, 20 in
// modes 1 and 2).
TEST(Compendium_9, DISABLED_InkTakenOnTheSecondByteOfTheIoMicrosecondOnTheCpcPlus)
{
   for (int mode : { 0, 1, 2 })
   {
      SCOPED_TRACE(testing::Message() << "mode " << mode);
      EXPECT_EQ(8, FirstPixelWithTheNewInk(CRTC::AMS40489, "6128PLUS", mode, InkInstruction::kOutCR8));
      EXPECT_EQ(8, FirstPixelWithTheNewInk(CRTC::AMS40489, "6128PLUS", mode, InkInstruction::kOuti));
   }
}

// 9.2.2 : CRTC 4 (ASIC 40226) : the colour changes one Mode 2 pixel earlier in mode 2.
// DISABLED : not emulated, and the colour comes one microsecond late (see above).
TEST(Compendium_9, DISABLED_InkOnePixelEarlierInMode2OnTheAsic40226)
{
   EXPECT_EQ(8, FirstPixelWithTheNewInk(CRTC::AMS40226, "6128", 1, InkInstruction::kOutCR8));
   EXPECT_EQ(7, FirstPixelWithTheNewInk(CRTC::AMS40226, "6128", 2, InkInstruction::kOutCR8));
}

/////////////////////////////////////////////////////////////
// 9.3 : graphic mode

namespace
{
// Mode used by the GATE ARRAY on C0 = 10 of the line after the one where OUT (C),r8 asks mode 0
// with its 3rd microsecond (the write) on C0vs = 'write_c0' ; the screen was in mode 2. -1 : the
// write could not be placed there.
int ModeAfterAWriteOn(CRTC::TypeCRTC type, int write_c0, unsigned char r3 = 0x8E)
{
   for (int attempt = 0; attempt < 16; ++attempt)
   {
      Machine m(type);
      Prepare(m, 2, 0x00);
      std::vector<unsigned char> p = { z80::kDi };
      z80::Append(p, z80::WriteCrtc(3, r3));
      if (!m.CyclesUntil([&]() { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_ == kLine - 1 && m.Crtc().hcc_ == 60; }, 4 * 20000)) return -1;
      m.Run(p);
      if (!m.CyclesUntil([&]() { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_ == kLine && m.Crtc().hcc_ == write_c0 - 8 + attempt / 2; }, 4 * 200)) return -1;
      p = { z80::kDi };
      p.insert(p.end(), attempt & 1, z80::kNop);
      const unsigned short at = (unsigned short)(kProgram + p.size() + 3);
      z80::Append(p, RmrMode(0));
      z80::Append(p, z80::kJrSelf);
      m.Run(p);
      int line = -1;
      const int start = m.InstructionStart(at, &line);
      if (line != kLine || start + 2 != write_c0) continue;
      if (!m.CyclesUntil([&]() { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_ == kLine + 1 && m.Crtc().hcc_ == 10; }, 4 * 200)) return -1;
      return m.Ga().buffered_screen_mode_;
   }
   return -1;
}
}

// 9.3.1 : the mode is changed during the HSYNC of the GATE ARRAY, which must last 2 us at least.
// DISABLED : the emulated GATE ARRAY changes the mode with a 1 us HSYNC.
TEST(Compendium_9, DISABLED_ModeChangeNeedsAHSyncOfTwoMicroseconds)
{
   EXPECT_EQ(2, ModeAfterAWriteOn(CRTC::HD6845S, 40, 0x81));
   EXPECT_EQ(0, ModeAfterAWriteOn(CRTC::HD6845S, 40, 0x82));
}

// 9.3.2 : CRTC 0, 1, 2, R2 = 46, R3 = 14 : the new mode is taken for the next line when the write
// (3rd microsecond of OUT (C),r8) is on C0vs <= 51 (the 6th microsecond of the HSYNC of the GATE
// ARRAY), not on 52.
TEST(Compendium_9, ModeChangeWindowCrtc012)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::UM6845R, CRTC::MC6845 })
   {
      SCOPED_TRACE(testing::Message() << "CRTC type " << (int)type);
      EXPECT_EQ(0, ModeAfterAWriteOn(type, 45));
      EXPECT_EQ(0, ModeAfterAWriteOn(type, 51));
      EXPECT_EQ(2, ModeAfterAWriteOn(type, 52));
   }
}

// 9.3.3 : CRTC 3, 4 : the HSYNC of the ASIC is 1 us later : written on C0vs <= 52, not on 53.
TEST(Compendium_9, ModeChangeWindowCrtc34)
{
   for (CRTC::TypeCRTC type : { CRTC::AMS40489, CRTC::AMS40226 })
   {
      SCOPED_TRACE(testing::Message() << "CRTC type " << (int)type);
      EXPECT_EQ(0, ModeAfterAWriteOn(type, 52));
      EXPECT_EQ(2, ModeAfterAWriteOn(type, 53));
   }
}

/////////////////////////////////////////////////////////////
// 8 : RAM written by the Z80A, read by the GATE ARRAY

namespace
{
// Address of the byte 'n' read by the GATE ARRAY from the microsecond C0 = c0 of 'line' (byte
// 2 * c0 + n of the line, standard screen).
unsigned short ScreenByte(int line, int c0, int n)
{
   const int ma = (line / 8) * kR1 + c0 + n / 2;
   return (unsigned short)(kScreen | ((line % 8) << 11) | ((ma * 2) & 0x7FF) | (n & 1));
}

// Byte displayed for the byte n of the microsecond C0 = c0 of 'line' (mode 2, pen 1 = bit set).
int DisplayedByte(Machine& m, int line, int c0, int n)
{
   const Machine::Block* b = m.FindBlock(line, c0 + n / 2 + 1);
   if (b == nullptr) return -1;
   int byte = 0;
   for (int i = 0; i < 8; ++i)
   {
      const int pen = PenOf(m, b->pixels[(n & 1) * 8 + i]);
      if (pen < 0) return -1;
      byte = (byte << 1) | pen;
   }
   return byte;
}

// Runs program(c0) : its setup, then the measured instruction (its last 'length' bytes), started on
// the microsecond C0 = c0 of kLine ; returns the bytes displayed for the bytes 'from' and
// 'from' + 1 of the microsecond c0. The start is found by a first run ; the second, identical
// except for the addresses, builds the program for it.
typedef std::function<std::vector<unsigned char>(int c0)> Program;
std::pair<int, int> Displayed(const Program& program, size_t length, int from)
{
   int c0 = 12;
   for (int pass = 0; pass < 2; ++pass)
   {
      Machine m(CRTC::HD6845S);
      Prepare(m, 2, 0x00);
      if (!m.CyclesUntil([&]() { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_ == kLine && m.Crtc().hcc_ == 2; }, 4 * 20000)) break;
      std::vector<unsigned char> p = program(c0);
      const unsigned short at = (unsigned short)(kProgram + p.size() - length);
      z80::Append(p, z80::kJrSelf);
      m.RecordBlocks(true);
      m.Run(p);
      int line = -1;
      const int start = m.InstructionStart(at, &line);
      if (line != kLine) break;
      if (pass == 0) { c0 = start; continue; }
      if (start != c0) break;
      for (int i = 0; i < 4 * 30; ++i) m.Cycle();
      return { DisplayedByte(m, kLine, c0, from), DisplayedByte(m, kLine, c0, from + 1) };
   }
   return { -1, -1 };
}

std::vector<unsigned char> Ld(unsigned char opcode, unsigned short value)
{
   return { opcode, (unsigned char)(value & 0xFF), (unsigned char)(value >> 8) };
}
}

// 8.1 : LD (HL),r8 (2 us) started on C0vs = c0 : the byte is written on its 2nd microsecond. The
// GATE ARRAY reads the bytes 0, 1 on c0 (too early : 00), the bytes 2, 3 on c0 + 1 (FF).
// DISABLED : the GATE ARRAY reads the RAM one microsecond early (at the end of the previous
// microsecond) : a byte written on the microsecond of its read is not displayed.
TEST(Compendium_8, DISABLED_LdHlR8)
{
   const int expected[4] = { 0x00, 0x00, 0xFF, 0xFF };
   for (int n = 0; n < 4; ++n)
   {
      SCOPED_TRACE(testing::Message() << "HL = byte " << n);
      const auto d = Displayed([n](int c0) {
         std::vector<unsigned char> p = { z80::kDi, 0x3E, 0xFF };   // LD A,&FF
         z80::Append(p, Ld(0x21, ScreenByte(kLine, c0, n)));       // LD HL,byte n
         p.push_back(0x77);                                         // LD (HL),A
         return p; }, 1, n);
      EXPECT_EQ(expected[n], d.first);
   }
}

// 8.2 : LD (aaaa),HL (5 us), H = &FF, L = &55 : L is written on the 4th microsecond, H on the 5th.
// Bytes aaaa, aaaa + 1 displayed : aaaa = byte 4 : 00 00 ; 5 : 00 00 ; 6 : 55 00 ; 7 : 55 FF.
// DISABLED : the GATE ARRAY reads the RAM one microsecond early (at the end of the previous
// microsecond) : a byte written on the microsecond of its read is not displayed.
TEST(Compendium_8, DISABLED_LdAaaaHl)
{
   const std::pair<int, int> expected[4] = { { 0x00, 0x00 }, { 0x00, 0x00 }, { 0x55, 0x00 }, { 0x55, 0xFF } };
   for (int n = 4; n < 8; ++n)
   {
      SCOPED_TRACE(testing::Message() << "aaaa = byte " << n);
      const auto d = Displayed([n](int c0) {
         std::vector<unsigned char> p = { z80::kDi };
         z80::Append(p, Ld(0x21, 0xFF55));                          // LD HL,&FF55
         z80::Append(p, Ld(0x22, ScreenByte(kLine, c0, n)));       // LD (aaaa),HL
         return p; }, 3, n);
      EXPECT_EQ(expected[n - 4], d);
   }
}

// 8.3 : PUSH DE (4 us), D = &FF, E = &55 : D is written (at SP - 1) on the 3rd microsecond, E (at
// SP - 2) on the 4th. Bytes SP - 2, SP - 1 displayed : SP = byte 4 : 00 00 ; 5, 6, 7 : 00 FF ;
// 8 : 55 FF.
// DISABLED : the GATE ARRAY reads the RAM one microsecond early (at the end of the previous
// microsecond) : a byte written on the microsecond of its read is not displayed.
TEST(Compendium_8, DISABLED_PushR16)
{
   const std::pair<int, int> expected[5] = { { 0x00, 0x00 }, { 0x00, 0xFF }, { 0x00, 0xFF }, { 0x00, 0xFF }, { 0x55, 0xFF } };
   for (int n = 4; n < 9; ++n)
   {
      SCOPED_TRACE(testing::Message() << "SP = byte " << n);
      const auto d = Displayed([n](int c0) {
         std::vector<unsigned char> p = { z80::kDi };
         z80::Append(p, Ld(0x11, 0xFF55));                          // LD DE,&FF55
         z80::Append(p, Ld(0x31, ScreenByte(kLine, c0, n)));       // LD SP,byte n
         p.push_back(0xD5);                                         // PUSH DE
         return p; }, 1, n - 2);
      EXPECT_EQ(expected[n - 4], d);
   }
}


/////////////////////////////////////////////////////////////
// 9.3.4.3 : mode splitting with R3 = 2 (R2.NJIT), GATE ARRAY 40010, pixels read on the schematics
// p56-64. Positions in Mode 2 pixels from the start of the block drawn during C0 = R2 (it displays
// the 1st byte "VRAM" of the schematics). The display stops for 32 pixels (CRTC 0 : 4 .. 35 ;
// CRTC 2 : 33 pixels, 3 .. 35 ; CRTC 1 : 5 .. 36). On CRTC 0 and 2, pixel 36 is one Mode 2 pixel of
// the 5th byte in the old mode ; from pixel 37 the GATE ARRAY draws the end of this byte in the new
// mode, with bits already used for the old one :
// - from mode 2 : the bit counter goes on at 5 : b2 b1 b0 are taken as b7 b6 b5, the missing bits
//   are 0 on the 40010 : 4 new pixels (to mode 0 : pen 0 b0 0 b2 ; mode 1 : pens b2, b1 ; mode 3 :
//   pen b2) ;
// - from mode 0 and 3 : old pixel A ; new : mode 0 pen B (b0 b4 b2 b6), mode 1 pens B (b2 b6) and
//   C (b1 b5), mode 2 pixels b6 b5 b4 (3 pixels only), mode 3 pen B (b2 b6) ;
// - from mode 1 : old pixel B (b2 b6) ; new : mode 0 pen 0 b3 b1 b5, mode 2 pixels b5 b4 b3, mode 3
//   pen C (b1 b5).
// The CRTC 1 does not display pixel 36 (black) ; the new pixels are the same.

namespace
{
const int kAny = -100;   // not checked

int Bit(unsigned char byte, int n) { return (byte >> n) & 1; }

// Pens expected on the Mode 2 pixels 36 .. 40.
std::vector<int> ExpectedSplit(int from, int to, unsigned char b, bool crtc1)
{
   int old_pen = 0;
   switch (from)
   {
   case 2: old_pen = Bit(b, 3); break;
   case 0: old_pen = (Bit(b, 1) << 3) | (Bit(b, 5) << 2) | (Bit(b, 3) << 1) | Bit(b, 7); break;
   case 1: old_pen = (Bit(b, 2) << 1) | Bit(b, 6); break;
   case 3: old_pen = (Bit(b, 3) << 1) | Bit(b, 7); break;
   }
   std::vector<int> e = { crtc1 ? kBlack : old_pen };
   auto four = [&](int pen) { for (int i = 0; i < 4; ++i) e.push_back(pen); };
   auto two_two = [&](int p, int q) { e.push_back(p); e.push_back(p); e.push_back(q); e.push_back(q); };
   auto three = [&](int p, int q, int r) { e.push_back(p); e.push_back(q); e.push_back(r); e.push_back(kAny); };
   if (from == 2)
   {
      if (to == 0) four((Bit(b, 0) << 2) | Bit(b, 2));
      if (to == 1) two_two(Bit(b, 2), Bit(b, 1));
      if (to == 3) four(Bit(b, 2));
   }
   else if (from == 0 || from == 3)
   {
      if (to == 0) four((Bit(b, 0) << 3) | (Bit(b, 4) << 2) | (Bit(b, 2) << 1) | Bit(b, 6));
      if (to == 1) two_two((Bit(b, 2) << 1) | Bit(b, 6), (Bit(b, 1) << 1) | Bit(b, 5));
      if (to == 2) three(Bit(b, 6), Bit(b, 5), Bit(b, 4));
      if (to == 3) four((Bit(b, 2) << 1) | Bit(b, 6));
   }
   else
   {
      if (to == 0) four((Bit(b, 3) << 2) | (Bit(b, 1) << 1) | Bit(b, 5));
      if (to == 2) three(Bit(b, 5), Bit(b, 4), Bit(b, 3));
      if (to == 3) four((Bit(b, 1) << 1) | Bit(b, 5));
   }
   return e;
}

// Pens on the Mode 2 pixels 36 .. 40 of kLine : mode 'from', mode 'to' asked during the line, R3 = 2.
std::vector<int> MeasuredSplit(CRTC::TypeCRTC type, int from, int to, unsigned char byte)
{
   Machine m(type);
   Prepare(m, from, byte);
   // R1 = 63 : the bytes under the HSYNC are displayed.
   std::vector<unsigned char> p = { z80::kDi };
   z80::Append(p, z80::WriteCrtc(3, 0x82));
   z80::Append(p, z80::WriteCrtc(1, 63));
   z80::Append(p, z80::kJrSelf);
   m.Run(p);
   for (int i = 0; i < 4 * 19968; ++i) m.Cycle();
   if (!m.CyclesUntil([&]() { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_ == kLine && m.Crtc().hcc_ == 10; }, 4 * 20000)) return {};
   p = { z80::kDi };
   z80::Append(p, RmrMode(to));
   z80::Append(p, z80::kJrSelf);
   m.RecordBlocks(true);
   m.Run(p);
   m.CyclesUntil([&]() { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_ == kLine + 1 && m.Crtc().hcc_ == 10; }, 4 * 200);
   m.RecordBlocks(false);
   const std::vector<int> pixels = LinePixels(m, kLine, 46, 3);
   std::vector<int> pens;
   for (int i = 36; i <= 40; ++i) pens.push_back(PenOf(m, pixels[i]));
   return pens;
}

void CheckSplits(CRTC::TypeCRTC type)
{
   const unsigned char byte = 0xB4;   // 1011 0100
   for (int from = 0; from < 4; ++from)
      for (int to = 0; to < 4; ++to)
      {
         if (from == to) continue;
         SCOPED_TRACE(testing::Message() << "mode " << from << " -> " << to);
         const std::vector<int> expected = ExpectedSplit(from, to, byte, type == CRTC::UM6845R);
         const std::vector<int> measured = MeasuredSplit(type, from, to, byte);
         ASSERT_EQ(expected.size(), measured.size());
         for (size_t i = 0; i < expected.size(); ++i)
            if (expected[i] != kAny) EXPECT_EQ(expected[i], measured[i]) << "pixel " << 36 + i;
      }
}
}

// DISABLED : the emulated GATE ARRAY changes the mode on a whole block and draws no mixed pixel.
TEST(Compendium_9, DISABLED_ModeSplittingPixelsCrtc0)
{
   CheckSplits(CRTC::HD6845S);
}

TEST(Compendium_9, DISABLED_ModeSplittingPixelsCrtc1)
{
   CheckSplits(CRTC::UM6845R);
}

TEST(Compendium_9, DISABLED_ModeSplittingPixelsCrtc2)
{
   CheckSplits(CRTC::MC6845);
}

// 9.3.4.5 : CRTC 4 (ASIC 40226), pixels read on the schematics p69-73. The display stops for 32
// pixels ; the first pixel after the black is one Mode 2 pixel of the 7th byte in the old mode,
// then 6 new pixels (5 to mode 2), computed with bits already used for the old mode. Positions
// from the first pixel after the HSYNC black.
namespace
{
std::vector<int> ExpectedSplitAsic40226(int from, int to, unsigned char b)
{
   auto pen0 = [&](int a, int c, int d, int e) { return (Bit(b, a) << 3) | (Bit(b, c) << 2) | (Bit(b, d) << 1) | Bit(b, e); };
   auto pen2 = [&](int a, int c) { return (Bit(b, a) << 1) | Bit(b, c); };
   auto run = [](int old, std::vector<std::pair<int, int>> pens) {
      std::vector<int> e = { old };
      for (const auto& p : pens) for (int i = 0; i < p.second; ++i) e.push_back(p.first);
      while (e.size() < 7) e.push_back(kAny);
      return e;
   };
   const int old0 = pen0(1, 5, 3, 7), old13 = pen2(3, 7);
   if (from == 2)
   {
      if (to == 0) return run(Bit(b, 5), { { (Bit(b, 2) << 2) | (Bit(b, 0) << 1) | Bit(b, 4), 2 }, { (Bit(b, 1) << 2) | Bit(b, 3), 4 } });
      if (to == 1) return run(Bit(b, 5), { { pen2(0, 4), 2 }, { Bit(b, 3), 2 }, { Bit(b, 2), 2 } });
      if (to == 3) return run(Bit(b, 5), { { pen2(0, 4), 2 }, { Bit(b, 3), 4 } });
   }
   if (from == 0)
   {
      if (to == 1) return run(old0, { { pen2(3, 7), 2 }, { pen2(2, 6), 2 }, { pen2(1, 5), 2 } });
      if (to == 2) return run(old0, { { Bit(b, 7), 1 }, { Bit(b, 6), 1 }, { Bit(b, 5), 1 }, { Bit(b, 4), 1 }, { Bit(b, 3), 1 } });
      if (to == 3) return run(kAny, { { kAny, 2 }, { pen2(2, 6), 4 } });
   }
   if (from == 1)
   {
      if (to == 0) return run(old13, { { pen0(0, 4, 2, 6), 2 }, { (Bit(b, 3) << 2) | (Bit(b, 1) << 1) | Bit(b, 5), 4 } });
      if (to == 2) return run(old13, { { Bit(b, 6), 1 }, { Bit(b, 5), 1 }, { Bit(b, 4), 1 }, { Bit(b, 3), 1 }, { Bit(b, 2), 1 } });
      if (to == 3) return run(old13, { { pen2(2, 6), 2 }, { pen2(1, 5), 4 } });
   }
   if (from == 3)
   {
      if (to == 0) return run(old13, { { pen0(1, 5, 3, 7), 2 }, { pen0(0, 4, 2, 6), 4 } });
      if (to == 1) return run(kAny, { { pen2(3, 7), 2 }, { pen2(2, 6), 2 }, { pen2(1, 5), 2 } });
      if (to == 2) return run(old13, { { Bit(b, 7), 1 }, { Bit(b, 6), 1 }, { Bit(b, 5), 1 }, { Bit(b, 4), 1 }, { Bit(b, 3), 1 } });
   }
   return {};
}
}

// DISABLED : the emulated GATE ARRAY changes the mode on a whole block and draws no mixed pixel.
TEST(Compendium_9, DISABLED_ModeSplittingPixelsCrtc4)
{
   const unsigned char byte = 0xB4;
   for (int from = 0; from < 4; ++from)
      for (int to = 0; to < 4; ++to)
      {
         if (from == to) continue;
         SCOPED_TRACE(testing::Message() << "mode " << from << " -> " << to);
         Machine m(CRTC::AMS40226);
         Prepare(m, from, byte);
         std::vector<unsigned char> p = { z80::kDi };
         z80::Append(p, z80::WriteCrtc(3, 0x82));
         z80::Append(p, z80::WriteCrtc(1, 63));
         z80::Append(p, z80::kJrSelf);
         m.Run(p);
         for (int i = 0; i < 4 * 19968; ++i) m.Cycle();
         ASSERT_TRUE(m.CyclesUntil([&]() { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_ == kLine && m.Crtc().hcc_ == 10; }, 4 * 20000));
         p = { z80::kDi };
         z80::Append(p, RmrMode(to));
         z80::Append(p, z80::kJrSelf);
         m.RecordBlocks(true);
         m.Run(p);
         m.CyclesUntil([&]() { return m.Crtc().vcc_ * 8 + m.Crtc().vlc_ == kLine + 1 && m.Crtc().hcc_ == 10; }, 4 * 200);
         m.RecordBlocks(false);
         const std::vector<int> pixels = LinePixels(m, kLine, 46, 8);
         int end = -1;
         for (int i = 0; i < (int)pixels.size(); ++i)
            if (PenOf(m, pixels[i]) == kBlack) end = i + 1;
            else if (end >= 0) break;
         ASSERT_GE(end, 0);
         const std::vector<int> expected = ExpectedSplitAsic40226(from, to, byte);
         for (size_t i = 0; i < expected.size(); ++i)
            if (expected[i] != kAny) EXPECT_EQ(expected[i], PenOf(m, pixels[end + i])) << "pixel " << i << " after the black";
      }
}
