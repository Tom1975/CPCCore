#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include "TestUtils.h"
#include "Machine.h"
#include "Display.h"

#include "gtest/gtest.h"

#include <iostream>

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

