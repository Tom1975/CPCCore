#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include "TestUtils.h"
#include "Machine.h"
#include "Display.h"

#include "gtest/gtest.h"

#include <iostream>
#include <memory>
#include <vector>

/////////////////////////////////////////////////////////////
/// Helper functions

/////////////////////////////////////////////////////////////
// Test functions
TEST(Z80, z80ccf)
{
   ASSERT_EQ(true, InitBinary("6128", "./TestConf.ini", "./res/z80/z80ccf.bin", 0x8079, 0x8064));
}

TEST(Z80, z80doc)
{
   ASSERT_EQ(true, InitBinary("6128", "./TestConf.ini", "./res/z80/z80doc.bin", 0x8079, 0x8064));
}

TEST(Z80, z80docflags)
{
   ASSERT_EQ(true, InitBinary("6128", "./TestConf.ini", "./res/z80/z80docflags.bin", 0x807F, 0x806A));
}

TEST(Z80, z80flags)
{
   ASSERT_EQ(true, InitBinary("6128", "./TestConf.ini", "./res/z80/z80flags.bin", 0x807B, 0x8066));
}

TEST(Z80, z80full)
{
   ASSERT_EQ(true, InitBinary("6128", "./TestConf.ini", "./res/z80/z80full.bin", 0x807A, 0x8065));
}

TEST(Z80, z80memptr)
{
   ASSERT_EQ(true, InitBinary("6128", "./TestConf.ini", "./res/z80/z80memptr.bin", 0x807C, 0x8067));
}


TEST(Z80, cpu)
{
   ASSERT_EQ(true, InitBinary("6128", "./TestConf.ini", "./res/z80/cpu.bin", 0x88F9, 0x89E2));
}

TEST(Z80, inout)
{
   ASSERT_EQ(true, InitBinary("6128", "./TestConf.ini", "./res/z80/inout.bin", 0x8571, 0x8398));
}

TEST(Z80, rtestopcodes)
{
   // TODO : 
   // - Press a key
   // - Wait for 
   ASSERT_EQ(true, InitBinary("6128", "./TestConf.ini", "./res/z80/rtestopcodes.bin", 0x2015, 0x531F, 1)); // 1 error is ok : My 6128 fail on 1 test (ld r,a : got 0, expected 2)
}


/////////////////////////////////////////////////////////////
// I/O timing (CRTC Compendium 4.4.3, 4.4.4) : position of the I/O in the microsecond.
// OUT(C),r8 asserts IORQ one T-state later than OUTI / OUT(n),A. The Gate Array
// CRTC (0, 1, 2) takes both in the same microsecond, the ASIC CRTC (3, 4) misses
// OUT(C),r8 and takes it one microsecond later.
namespace
{
// Runs 'program' at &4000 and returns the number of 4 MHz cycles until the first CRTC
// clock (C0 moves) that evaluates the character with the new R12.
int CyclesUntilR12Changes(CRTC::TypeCRTC type, const std::vector<unsigned char>& program)
{
   DirectoriesImp dirImp;
   ConfigurationManager conf_manager;
   CDisplay display;
   display.Init(false);
   display.Show(false);
   SoundFactory soundFactory;
   std::unique_ptr<EmulatorEngine> engine(new EmulatorEngine());
   EmulatorEngine& machine = *engine;
   machine.SetDirectories(&dirImp);
   machine.SetConfigurationManager(&conf_manager);
   machine.SetLog(NULL);
   machine.Init(&display, &soundFactory);
   machine.GetMem()->Initialisation();
   machine.SetFixedSpeed(true);
   machine.LoadConfiguration("6128", "./TestConf.ini");
   machine.Reinit();
   for (int i = 0; i < 50; i++) machine.RunTimeSlice();

   CRTC* crtc = machine.GetCRTC();
   crtc->DefinirTypeCRTC(type);
   crtc->registers_list_[12] = 0x30;
   unsigned char* ram = machine.GetMotherboard()->GetRamBuffer();
   for (size_t i = 0; i < program.size(); i++) ram[0x4000 + i] = program[i];
   ram[0x5000] = 0x15;   // byte sent by OUTI
   machine.GetProc()->PrepareForFetch(0x4000);

   bool written = false;
   unsigned char c0 = crtc->hcc_;
   for (int cycle = 0; cycle < 400; ++cycle)
   {
      machine.GetMotherboard()->DebugNew(1);
      if (written && crtc->hcc_ != c0) return cycle;
      written = written || (crtc->registers_list_[12] != 0x30);
      c0 = crtc->hcc_;
   }
   return -1;
}

const std::vector<unsigned char> kOutCR8 = {
   0xF3,                // DI
   0x01, 0x0C, 0xBC,    // LD BC,&BC0C
   0xED, 0x49,          // OUT (C),C   : select R12
   0x01, 0x00, 0xBD,    // LD BC,&BD00
   0x26, 0x15,          // LD H,&15
   0xED, 0x61,          // OUT (C),H   : R12 = &15
   0x18, 0xFE };        // JR $
const std::vector<unsigned char> kOuti = {
   0xF3,                // DI
   0x01, 0x0C, 0xBC,    // LD BC,&BC0C
   0xED, 0x49,          // OUT (C),C   : select R12
   0x01, 0x00, 0xBE,    // LD BC,&BE00 (OUTI decrements B first)
   0x21, 0x00, 0x50,    // LD HL,&5000
   0x00, 0x00,          // NOP x2 : same length as kOutCR8 before the I/O instruction
   0xED, 0xA3,          // OUTI        : R12 = (&5000)
   0x18, 0xFE };        // JR $
}

TEST(Z80_IoTiming, OutCR8IsOneMicrosecondLaterOnAsicCrtc)
{
   const int crtc0 = CyclesUntilR12Changes(CRTC::HD6845S, kOutCR8);
   const int crtc3 = CyclesUntilR12Changes(CRTC::AMS40489, kOutCR8);
   ASSERT_GE(crtc0, 0);
   EXPECT_EQ(crtc0 + 4, crtc3);
}

TEST(Z80_IoTiming, OutiIsTakenAtTheSameTimeOnEveryCrtc)
{
   const int crtc0 = CyclesUntilR12Changes(CRTC::HD6845S, kOuti);
   const int crtc3 = CyclesUntilR12Changes(CRTC::AMS40489, kOuti);
   ASSERT_GE(crtc0, 0);
   EXPECT_EQ(crtc0, crtc3);
}

// 13.6 / 24.1 : the shortest R13 then R12 update. B=&BC, A=12, C=13, H=R12, L=R13 :
// OUT (C),C / INC B / OUT (C),L / OUT (&FF),A / OUT (C),H (OUT (&FF),A selects R12 : port &0CFF,
// data 12). With the durations of 4.4.3 (OUT (C),r8 : 4 us, I/O on the 3rd ; OUT (n),A : 3 us),
// R13 is written on the 3rd us and R12 on the 10th : 7 us apart, R12 in the 8th us counted from
// the one of R13 -- the "8 us" of the compendium. Not a delay of the CRTC : only the instructions.
TEST(Z80_IoTiming, R13ThenR12AtTheEighthMicrosecond)
{
   for (CRTC::TypeCRTC type : { CRTC::HD6845S, CRTC::UM6845R, CRTC::MC6845, CRTC::AMS40489, CRTC::AMS40226 })
   {
      DirectoriesImp dirImp;
      ConfigurationManager conf_manager;
      CDisplay display;
      display.Init(false);
      display.Show(false);
      SoundFactory soundFactory;
      std::unique_ptr<EmulatorEngine> engine(new EmulatorEngine());
      EmulatorEngine& machine = *engine;
      machine.SetDirectories(&dirImp);
      machine.SetConfigurationManager(&conf_manager);
      machine.SetLog(NULL);
      machine.Init(&display, &soundFactory);
      machine.GetMem()->Initialisation();
      machine.SetFixedSpeed(true);
      machine.LoadConfiguration("6128", "./TestConf.ini");
      machine.Reinit();
      for (int i = 0; i < 50; i++) machine.RunTimeSlice();

      CRTC* crtc = machine.GetCRTC();
      crtc->DefinirTypeCRTC(type);
      crtc->registers_list_[12] = 0x30;
      crtc->registers_list_[13] = 0x00;
      const std::vector<unsigned char> program = {
         0xF3,                // DI
         0x01, 0x0D, 0xBC,    // LD BC,&BC0D
         0x3E, 0x0C,          // LD A,12
         0x21, 0x2A, 0x15,    // LD HL,&152A : R12 = &15, R13 = &2A
         0xED, 0x49,          // OUT (C),C   : select R13
         0x04,                // INC B
         0xED, 0x69,          // OUT (C),L   : R13 = L
         0xD3, 0xFF,          // OUT (&FF),A : select R12
         0xED, 0x61,          // OUT (C),H   : R12 = H
         0x18, 0xFE };        // JR $
      unsigned char* ram = machine.GetMotherboard()->GetRamBuffer();
      for (size_t i = 0; i < program.size(); i++) ram[0x4000 + i] = program[i];
      machine.GetProc()->PrepareForFetch(0x4000);

      int r13_at = -1, r12_at = -1;
      for (int cycle = 0; cycle < 400 && r12_at < 0; ++cycle)
      {
         machine.GetMotherboard()->DebugNew(1);
         if (r13_at < 0 && crtc->registers_list_[13] == 0x2A) r13_at = cycle;
         if (r12_at < 0 && crtc->registers_list_[12] == 0x15) r12_at = cycle;
      }
      SCOPED_TRACE((int)type);
      ASSERT_GE(r13_at, 0);
      ASSERT_GE(r12_at, 0);
      EXPECT_EQ(7 * 4, r12_at - r13_at);   // 4 MHz cycles
   }
}
