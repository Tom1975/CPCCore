
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include "TestUtils.h"
#include "Machine.h"
#include "Display.h"

#include <iostream>

#include "gtest/gtest.h"

/////////////////////////////////////////////////////////////
/// Helper functions

/////////////////////////////////////////////////////////////
//

//andy.bin
// TO CHECK
TEST(Plus, DISABLED_andy)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, ".\\res\\plus\\screesnhots\\andy.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\asic.dsk", "run\"andy\r", &cmd_list));
}

//asic_after_lock.bin
// ERROR ??
TEST(Plus, DISABLED_asic_after_lock)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, ".\\res\\plus\\screesnhots\\asic_after_lock.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\asic.dsk", "run\"afterlck\r", &cmd_list));
}

//asicfloat.bin
TEST(Plus, asicfloat)
{
   ASSERT_EQ(true, InitBinary("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\asicfloat.bin", 0x8B3A, 0x8c26 )); 
   
}

//asiclock.bin
TEST(Plus, asiclock)
{
   ASSERT_EQ(true, InitBinary("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\asiclock.bin", 0x8483, 0x8308));
}

//asicppi.bin
TEST(Plus, DISABLED_asicppi)
{
   ASSERT_EQ(true, InitBinary("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\asicppi.bin", 0x5042, 0x4Fb3));
}

//asicraster.bin
// NOT CORRECT : See screenshots to check
TEST(Plus, DISABLED_asicraster)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, ".\\res\\plus\\screesnhots\\asicraster.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\asic.dsk", "run\"raster\r", &cmd_list));
}

//asicrom.bin
TEST(Plus, asicrom)
{
   ASSERT_EQ(true, InitBinary("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\asicrom.bin", 0x910E, 0x9512));
}

//asictest.bin
TEST(Plus, asictest)
{
   ASSERT_EQ(true, InitBinary("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\asictest.bin", 0x9381, 0x92F1, 1));
}

//asic_external_ram.bin
TEST(Plus, asic_external_ram)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandKeyboard("y"));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandKeyboard("n"));
   cmd_list.AddCommand(new CommandAddBreakpoint(0xA286));
   auto t = [](EmulatorEngine* machine) -> bool {  return (machine->GetProc()->GetPC() == 0xA286 ); };
   cmd_list.AddCommand(new CommandRunCyclesCondition(500, t));
   ASSERT_EQ(true, test_dump.Test("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\asic.dsk", "run\"extnram\r", &cmd_list));
}

//dmatest.bin
// one test is missing
TEST(Plus, dmatest)
{
   ASSERT_EQ(true, InitBinary("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\dmatest.bin", 0x9C56, 0x9BC7, 1));
}

//dmatiming.bin
// TODO : All these are wrong !

//hscrl.bin
// to check with real hardware
//hscrl0.bin
// to check with real hardware
//hscrl0b.bin
// to check with real hardware
//hscrl1.bin
// to check with real hardware
//hscrl1b.bin
// to check with real hardware
//hscrl_mid.bin
// to check with real hardware
//lumasic.bin
// to check with real hardware
//lumasic2.bin
// to check with real hardware

//onlyin.bin
TEST(Plus, onlyin)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, ".\\res\\plus\\screesnhots\\onlyin.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128PLUSPARADOS", ".\\TestConf.ini", ".\\res\\plus\\asic.dsk", "run\"onlyin\r", &cmd_list));
}

//pluscol.bin
//pluscols.bin
//pluspen.bin
//plus_standard_hlace.bin
//pridelay.bin
//prihsyncw.bin

//pritest.bin
// vérifier les 3 tests ko, et les occurences

//pritrig.bin
//pri_ack_cpc_int.bin
//pri_delay_cpc_int.bin
//pri_hsync_pos.bin
//pri_hsync_width.bin
//pri_mix.bin
//pri_mix2.bin
//pri_mix_no_change.bin

//scrlr8.bin
//scrl_mid.bin
//split1.bin
//split3.bin
//split3d.bin
//splitmask.bin
//splits.bin
//splits2.bin
//splits3.bin
//splittrig.bin
//splittrig2.bin
//splittrig3.bin
//splittrig3b.bin
//splittrig3c.bin
//splittrig4.bin
//splt.bin
//splt2.bin
//splt3.bin
//splt3b.bin
//splt3c.bin
//spltr8.bin
//spr_mag_mirror.bin
//vscrl.bin
//vscrl2.bin
//vscrl2b.bin
//vscrl2c.bin
//vscrl_2b.bin
//vscrl_hdisp.bin
//vscrl_r9.bin
//vscrl_r9_16.bin
//vscrl_r9_4.bin
//vscroll_r9_31_ok.bin
//vscroll_r9_41_ok.bin