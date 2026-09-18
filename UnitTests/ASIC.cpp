
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#include <iostream>


#include "gtest/gtest.h"
#include "Asic.h"
#include "Memoire.h"

#include "TestUtils.h"

/////////////////////////////////////////////////////////////
/// Helper functions

/////////////////////////////////////////////////////////////
//
// TEST : Lock / Unlock

TEST(ASIC, Unlock)
{
   // Create engine
   Asic asic;
   GateArray vga;

   asic.Init(&vga, nullptr, nullptr);
   // Default is : No unlock
   ASSERT_EQ(asic.IsAsicLocked(), true);

   // Out the magic sequence
   // Not zero
   asic.Out(0xBC00, 0xFF);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   // zero
   asic.Out(0xBC00, 0x00);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   // sequence 
   asic.Out(0xBC00, 0xFF);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0x77);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0xB3);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0x51);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0xA8);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0xD4);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0x62);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0x39);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0x9C);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0x46);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0x2B);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0x15);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0x8A);
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0xCD); // Unlock !
   ASSERT_EQ(asic.IsAsicLocked(), true);
   asic.Out(0xBC00, 0xEE);

   // Check if unlock is done
   ASSERT_EQ(asic.IsAsicLocked(), false);
}

TEST(ASIC, Lock)
{
   // Create engine
   Asic asic;
   GateArray vga;

   asic.Init(&vga, nullptr, nullptr);

   // Default is : No unlock
   ASSERT_EQ(asic.IsAsicLocked(), true);

   // Out the magic sequence
   // Not zero
   asic.Out(0xBC00, 0xFF);
   // zero
   asic.Out(0xBC00, 0x00);
   // sequence 
   asic.Out(0xBC00, 0xFF);
   asic.Out(0xBC00, 0x77);
   asic.Out(0xBC00, 0xB3);
   asic.Out(0xBC00, 0x51);
   asic.Out(0xBC00, 0xA8);
   asic.Out(0xBC00, 0xD4);
   asic.Out(0xBC00, 0x62);
   asic.Out(0xBC00, 0x39);
   asic.Out(0xBC00, 0x9C);
   asic.Out(0xBC00, 0x46);
   asic.Out(0xBC00, 0x2B);
   asic.Out(0xBC00, 0x15);
   asic.Out(0xBC00, 0x8A);
   asic.Out(0xBC00, 0xCD); // Unlock !
   asic.Out(0xBC00, 0xEE);

   // Check if unlock is done
   ASSERT_EQ(asic.IsAsicLocked(), false);

   // Out the magic sequence
   // Not zero
   asic.Out(0xBC00, 0xFF);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   // zero
   asic.Out(0xBC00, 0x00);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   // sequence 
   asic.Out(0xBC00, 0xFF);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0x77);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0xB3);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0x51);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0xA8);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0xD4);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0x62);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0x39);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0x9C);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0x46);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0x2B);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0x15);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0x8A);
   ASSERT_EQ(asic.IsAsicLocked(), false);
   asic.Out(0xBC00, 0x00); // Unlock !

   // Check if unlock is done
   ASSERT_EQ(asic.IsAsicLocked(), true);
}

// With nothing plugged into the analogue port, a real Plus reads 0x3F on
// ADC0-4 and ADC6, and 0x00 on ADC5 and ADC7. Measured values from Kevin
// Thacker's notes, https://cpctech.cpcwiki.de/docs/cpcplus.html, section
// "Analogue inputs"; MAME and AMSpiriT return the same.
TEST(ASIC, AnalogueInputsWithNothingPlugged)
{
   // sizeof(Memory) is 4.6 MB, more than a default Windows stack holds.
   Memory* memory = new Memory(nullptr);
   memory->InitMemory();

   const unsigned char expected[8] = { 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x3F, 0x00 };
   for (unsigned short channel = 0; channel < 8; channel++)
   {
      EXPECT_EQ(expected[channel], memory->ReadAsicRegister(0x6808 + channel)) << "ADC" << channel;
   }

   delete memory;
}

////////////////////////////
// Arnoldemu asic test

TEST(ASIC, asiclock)
{
   TestDump * test_dump = new TestDump();
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(250));
   cmd_list.AddCommand(new CommandKeyboard("run\"lock"));
   cmd_list.AddCommand(new CommandRunCycles(80));
   cmd_list.AddCommand(new CommandKeyboard("\r"));

   // KO BP
   cmd_list.AddCommand(new CommandAddBreakpoint(0x84F9));
   // OK BP
   cmd_list.AddCommand(new CommandAddBreakpoint(0x84F5));

   auto t = [](EmulatorEngine* machine) -> bool { return (machine->GetProc()->GetPC() == 0x84F5); };
   cmd_list.AddCommand(new CommandRunCyclesCondition(300, t));

   ASSERT_EQ(true, test_dump->Test("6128PLUS", "./TestConf.ini", "./res/TestSuite/asic.dsk", "1", &cmd_list, true));
}

TEST(ASIC, ROM)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(250));
   cmd_list.AddCommand(new CommandKeyboard("run\"ROM"));
   cmd_list.AddCommand(new CommandRunCycles(80)); 
   cmd_list.AddCommand(new CommandKeyboard("\r"));

   // KO BP
   cmd_list.AddCommand(new CommandAddBreakpoint(0x9691));
   // OK BP
   cmd_list.AddCommand(new CommandAddBreakpoint(0x968e));

   auto t = [](EmulatorEngine* machine) -> bool { return (machine->GetProc()->GetPC() == 0x968e); };
   cmd_list.AddCommand(new CommandRunCyclesCondition(350, t));

   ASSERT_EQ(true, test_dump.Test("6128PLUS", "./TestConf.ini", "./res/TestSuite/asic.dsk", "1", &cmd_list, true));
}
