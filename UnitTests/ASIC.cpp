
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#include <iostream>

#include "tchar.h"
#include "windows.h"
#include "gtest/gtest.h"
#include "Asic.h"

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

////////////////////////////
// Arnoldemu asic test

TEST(ASIC, asiclock)
{
   TestDump test_dump;
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

   ASSERT_EQ(true, test_dump.Test("6128PLUS", ".\\TestConf.ini", ".\\res\\TestSuite\\asic.dsk", "1", &cmd_list, true));
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

   ASSERT_EQ(true, test_dump.Test("6128PLUS", ".\\TestConf.ini", ".\\res\\TestSuite\\asic.dsk", "1", &cmd_list, true));
}
