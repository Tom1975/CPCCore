
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>

#include "gtest/gtest.h"
#include "DiskContainer.h"

#include "TestUtils.h"

//#define  SCR_COMPARE false

//[CPC] 1001 BC A Mediterranean Odyssey (1986)(Ere Informatique)(Fr)[RAW].raw
#ifdef NO_INIT_SCREENSHOT
   #define SCREENSHOT_COMPARE(path,cycle_count) cmd_list.AddCommand(new CommandRunToScreenshot( &test_dump.display, path, cycle_count+50 ));
#else
   #define SCREENSHOT_COMPARE(path,cycle_count) cmd_list.AddCommand(new CommandRunCycles(cycle_count));cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, path, false ));
#endif

TEST(Dumps_Disk, 1001BCAD)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandKeyboard(" "));

   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/[CPC] 1001 BC A Mediterranean Odyssey (1986)(Ere Informatique)(Fr)[RAW].raw.bmp", SCR_COMPARE));

   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/[CPC] 1001 BC A Mediterranean Odyssey (1986)(Ere Informatique)(Fr)[RAW].raw", "run\"ere.bas\r", &cmd_list, true));
}

TEST(Dumps_Disk, AceOfAce)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1202));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 0));
   cmd_list.AddCommand(new CommandAddBreakpoint(0x2E30));
   auto t = [](EmulatorEngine* machine) -> bool {  return ( machine->GetProc()->GetPC() == 0x2E30 && 
                                                      machine->GetProc()->hl_.w == 0x156B &&
                                                      machine->GetProc()->af_.w == 0x3162 &&
                                                      machine->GetProc()->bc_.w == 0x167F &&  
                                                      machine->GetProc()->de_.w == 0x156C ); };
   cmd_list.AddCommand(new CommandRunCyclesCondition(1500, t));

   ASSERT_EQ(true, test_dump.Test ("6128", "./TestConf.ini", "./res/DSK/Ace Of Aces (UK) (1985) [Original] (Weak Sectors).dsk", "run\"Ace\r", &cmd_list));
}

TEST(Dumps_Disk, AfterBurner)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1350));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/After Burner (UK) (1988) (UK retail version) (CPM) [Original] (Weak Sectors).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test ("6128", "./TestConf.ini", "./res/DSK/After Burner (UK) (1988) (UK retail version) (CPM) [Original] (Weak Sectors).dsk", "|cpm\r", &cmd_list));
}

TEST(Dumps_Disk, AnglaisCollege)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1058));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Anglais College 4e-3e(UK) (Face A) (1988)[Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Anglais College 4e-3e (UK) (Face A) (1988) [Original].dsk", "run\"depart\r", &cmd_list));
}

TEST(Dumps_Disk, APB)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(3958));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/APB - All Points Bulletin (UK) (1989) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/APB - All Points Bulletin (UK) (1989) [Original].dsk", "run\"APB\r", &cmd_list));
}

//Asterix Et La Potion Magique_KBI-10.raw
TEST(Dumps_Disk, AsterixEtLaPotionMagique)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandKeyboard("2"));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Asterix Et La Potion Magique_KBI-10.raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Asterix Et La Potion Magique_KBI-10.raw", "run\"loader\r", &cmd_list, true));
}

TEST(Dumps_Disk, CaptainBlood_IPF)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandAddBreakpoint(0x5BAE));

   auto t = [](EmulatorEngine* machine) -> bool { return (machine->GetProc()->GetPC() == 0x5BAE); };
   cmd_list.AddCommand(new CommandRunCyclesCondition(1300, t) );
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/ArcheCapitaineBlood.ipf", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk6{ 6128 } {disk} {Arkanoid - Revenge of Doh(1988)(Imagine).dsk} {run"Arkanoid} {612} {Arkanoid}]
TEST(Dumps_Disk, Arkanoid_II)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1600));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 0));
   cmd_list.AddCommand(new CommandRunCycles(612));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Arkanoid - Revenge of Doh (1988)(Imagine).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Arkanoid - Revenge of Doh (1988)(Imagine).dsk", "run\"Arkanoid\r", &cmd_list));
}

// lappend testlist[SingleTest dsk7{ 6128 } {disk} {Bad Cat(UK) (Face A) (1987)[Original](Weak Sectors) (GAPS).dsk} {run"badcat} {500} {BadCat}]
TEST(Dumps_Disk, BadCat)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(663));
   cmd_list.AddCommand(new CommandKeyboard("\r"));
   cmd_list.AddCommand(new CommandRunCycles(300));
   cmd_list.AddCommand(new CommandKeyboard("\r"));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandKeyboard("\r"));
   cmd_list.AddCommand(new CommandRunCycles(150));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x2A, 1));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x02, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x2A, 0));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x02, 0));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandKeyboard("\r"));
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandJoystick(0, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Bad Cat (UK) (Face A) (1987) [Original] (Weak Sectors) (GAPS).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Bad Cat (UK) (Face A) (1987) [Original] (Weak Sectors) (GAPS).dsk", "run\"badcat\r", &cmd_list));
}

TEST(Dumps_Disk, BaladeOutreRhin)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(325));
   cmd_list.AddCommand(new CommandKeyboard("balade\r"));
   cmd_list.AddCommand(new CommandRunCycles(444));

   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Balade Outre - Rhin(F, G) (1986) (CPM)[Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Balade Outre-Rhin (F,G) (1986) (CPM) [Original].dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk8{ 6128 } {disk} {Barbarian (1987)(Palace Software)(Disk 1 of 2)[a].dsk} {run"barb1a} {1140} {defaut}]
TEST(Dumps_Disk, Barbarian)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1250));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Barbarian (1987)(Palace Software)(Disk 1 of 2)[a].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Barbarian (1987)(Palace Software)(Disk 1 of 2)[a].dsk", "run\"barb1a\r", &cmd_list));
}

//lappend testlist[SingleTest dsk9{ 6128 } {disk} {Barbarian 2 (1989)(Palace Software).dsk} {run"disc} {121} {Barbarian2}]
TEST(Dumps_Disk, Barbarian2)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(800));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x0B, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x0B, 0));
   cmd_list.AddCommand(new CommandRunCycles(861));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x0B, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x0B, 0));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Barbarian 2 (1989)(Palace Software).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Barbarian 2 (1989)(Palace Software).dsk", "run\"disc\r", &cmd_list));
}

//lappend testlist[SingleTest dsk10{ 6128 } {disk} {Basun(UK) (CPM)[Copia](no funciona).dsk} { | cpm} {150} {Basun}]
TEST(Dumps_Disk, Basun)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(3000));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1C, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1C, 0));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x24, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x24, 0));
   cmd_list.AddCommand(new CommandRunCycles(150));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Basun (UK) (CPM) [Copia] (no funciona).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Basun (UK) (CPM) [Copia] (no funciona).dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk74{ 6128 } {disk} {Bobby Bearing (UK) (1986) (CPM) [Original] (GAPS).dsk} { | cpm} {732} {defaut}]
TEST(Dumps_Disk, BobbyBearingGaps)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(732));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Bobby Bearing (UK) (1986) (CPM) [Original] (GAPS).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Bobby Bearing (UK) (1986) (CPM) [Original] (GAPS).dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk71{ 6128 } {disk} {Bobby Bearing (UK) (1986) (CPM) [Original].dsk} { | cpm} {686} {defaut}]
TEST(Dumps_Disk, BobbyBearing)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(732));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Bobby Bearing (UK) (1986) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Bobby Bearing (UK) (1986) (CPM) [Original].dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk11{ 6128 } {disk} {Castle Master(1990)(Domark)(Sp).dsk} {run"castle} {500} {Castle}]
TEST(Dumps_Disk, CastleMaster)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1098));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1C, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1C, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Castle Master (1990)(Domark)(Sp).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Castle Master (1990)(Domark)(Sp).dsk", "run\"castle\r", &cmd_list));
}

//lappend testlist[SingleTest dsk12{ 6128 } {disk} {Catch 23 (UK)(1987) (CPM)[Original].dsk} { | cpm} {35} {Catch23}]
TEST(Dumps_Disk, Catch23)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(890));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(35));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Catch 23 (UK) (1987) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Catch 23 (UK) (1987) (CPM) [Original].dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk13{ 6128 } {disk} {Chicago 90[MAXIT][SAMdisk37][Original][GAPS].dsk} {run"ch90} {2684} {defaut}]
TEST(Dumps_Disk, Chicago90)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(3000));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Chicago 90 [MAXIT][SAMdisk37][Original][GAPS].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Chicago 90 [MAXIT][SAMdisk37][Original][GAPS].dsk", "run\"ch90\r", &cmd_list));
}

//[CPC] Contamination (1985)(Ere Informatique)(Fr)[RAW].raw
TEST(Dumps_Disk, Contamination)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2300));
   cmd_list.AddCommand(new CommandKeyboard("1"));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1c, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1c, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/[CPC] Contamination (1985)(Ere Informatique)(Fr)[RAW].raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/[CPC] Contamination (1985)(Ere Informatique)(Fr)[RAW].raw", "run\"ere\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk87{ 6128 } {disk} {Corsarios(UK) (1988) (CPM)[Original].dsk} { | cpm} {2684} {Corsarios}]
TEST(Dumps_Disk, CorsariosUK)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(2684));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Corsarios (UK) (1988) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Corsarios (UK) (1988) (CPM) [Original].dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk73{ 6128 } {disk} {Cosa Nostra(F) (1986) (CPM)[Original].dsk} { | cpm} {665} {defaut}]
TEST(Dumps_Disk, CosaNostra)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(665));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Cosa Nostra (F) (1986) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Cosa Nostra (F) (1986) (CPM) [Original].dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk14{ 6128 } {disk} {Crazy Cars[MAXIT][SAMDisk383][Original].dsk} {run"crazy} {569} {Crazy}]
TEST(Dumps_Disk, CrazyCars)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(395));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(600));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Crazy Cars [MAXIT][SAMDisk383][Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Crazy Cars [MAXIT][SAMDisk383][Original].dsk", "run\"crazy\r", &cmd_list));
}

//lappend testlist[SingleTest dsk72{ 6128 } {disk} {lance crazyii.dsk} {run"crazyii} {870} {crazyii}]
TEST(Dumps_Disk, CrazyCarsII)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(450));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(300));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(300));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(900));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/lance crazyii.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/lance crazyii.dsk", "run\"crazyii\r", &cmd_list));
}

//lappend testlist[SingleTest dsk16{ 6128 } {disk} {DaleyThompson'sOC.raw} {run"disc} {1732} {defaut}]
TEST(Dumps_Disk, DaleyThompson_RAW)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1732));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/DaleyThompson'sOC.raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/DaleyThompson'sOC.raw", "run\"disc\r", &cmd_list));
}

TEST(Dumps_Disk, DaleyThompson_IPF)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1732));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/DaleyThompson'sOC.ipf.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/DaleyThompson'sOC.ipf", "run\"disc\r", &cmd_list));
}

//lappend testlist[SingleTest dsk77{ 6128 } {disk} {Dark Sceptre(UK) (1988) (CPM)[Original].dsk} { | cpm} {871} {defaut}]
TEST(Dumps_Disk, DarkSceptre)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(871));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Dark Sceptre (UK) (1988) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Dark Sceptre (UK) (1988) (CPM) [Original].dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk89{ 6128 } {disk} {Despotik Design[MAXIT][SAMdisk388][Original][ALLGAPS].dsk} {run"ere"} {1500} {defaut}]
TEST(Dumps_Disk, DespotikDesign)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1600));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Despotik Design [MAXIT][SAMdisk388][Original][ALLGAPS].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Despotik Design [MAXIT][SAMdisk388][Original][ALLGAPS].dsk", "run\"ere\r", &cmd_list));
}

//lappend testlist[SingleTest dsk17{ 6128 } {disk} {Discology 6.0 Plus.scp} { | cpm} {700} {defaut}]
TEST(Dumps_Disk, Discologie_SCP)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Discology 6.0 Plus.scp.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Discology 6.0 Plus.scp", "|cpm\r", &cmd_list));
}

TEST(Dumps_Disk, Discologie_DSK)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Discology60PlusA1 [MAXIT][SAMdisk36B19][Original].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Discology60PlusA1 [MAXIT][SAMdisk36B19][Original].dsk", "|cpm\r", &cmd_list));
}

//Dogfight 2187_Ariolasoft.raw
TEST(Dumps_Disk, Dogfight)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2300));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(770));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Dogfight 2187_Ariolasoft.raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128Uk", "./TestConf.ini", "./res/DSK/Dogfight 2187_Ariolasoft.raw", "run\"as\r", &cmd_list, true));
}

//Dizzy II - Treasure Island Dizzy (UK) (1989) [Original].raw
TEST(Dumps_Disk, Dizzy2TreasureIsland)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Dizzy II - Treasure Island Dizzy (UK) (1989) [Original].raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Dizzy II - Treasure Island Dizzy (UK) (1989) [Original].raw", "run\"disk\r", &cmd_list, true));
}

// Emlyn Hughes Arcade Quiz(UK) (1991) (CPM)[Original].dsk
TEST(Dumps_Disk, EmlynHughesArcadeQuiz)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1350));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Emlyn Hughes Arcade Quiz (UK) (1991) (CPM) [Original].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Emlyn Hughes Arcade Quiz (UK) (1991) (CPM) [Original].dsk", "run\"ehaq\r", &cmd_list));
}


//lappend testlist[SingleTest dsk19{ 6128 } {disk} {E.X.I.T(F) (Face A) (1988)[Original].dsk} {run"ubi} {93} {Exit}]
TEST(Dumps_Disk, Exit)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(400));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   // Insert disk B
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/E.X.I.T (F) (Face B) (1988) [Original].dsk"));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(165));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/E.X.I.T (F) (Face A) (1988) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/E.X.I.T (F) (Face A) (1988) [Original].dsk", "run\"ubi\r", &cmd_list));
}

//lappend testlist[SingleTest dsk78{ 6128 } {disk} {Every Second Counts(UK) (1988) (CPM)[Original].dsk} { | cpm} {640} {defaut}]
TEST(Dumps_Disk, EverySecondCount)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Every Second Counts (UK) (1988) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Every Second Counts (UK) (1988) (CPM) [Original].dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk21{ 6128 } {disk} {Fer & Flamme(F) (Face 1) (1986)[Original](Weak Sectors).dsk} {run"f&f} {2000} {F_et_F}]
TEST(Dumps_Disk, FerEtFlamme)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(550));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x51, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x51, 0));
   cmd_list.AddCommand(new CommandRunCycles(2200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Fer & Flamme (F) (Face 1) (1986) [Original] (Weak Sectors).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Fer & Flamme (F) (Face 1) (1986) [Original] (Weak Sectors).dsk", "run\"f&f\r", &cmd_list));
}

//lappend testlist[SingleTest dsk67{ 6128 } {disk} {Zombi(1986)(Ubi Soft)(Fr)(Disk 1 of 2).dsk} {run"zomb} {500} {Zomb}]
TEST(Dumps_Disk, Fire)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(800));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(1500));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(1500));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(1500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Fire !.raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Fire !.raw", "|cpm\r", &cmd_list, true));
}

TEST(Dumps_Disk, Fugitif)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2000));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(1000));
   // Insert disk B
   cmd_list.AddCommand(new CommandEjectDisk());
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Fugitif [1B][MAXIT][SAMdisk38][Original].dsk"));
   cmd_list.AddCommand(new CommandRunCycles(400));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Fugitif [1A][MAXIT][SAMdisk38][Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Fugitif [1A][MAXIT][SAMdisk38][Original].dsk", "|cpm\r", &cmd_list));
}

// Mission Genocide (UK) (1987).dsk
TEST(Dumps_Disk, Genocide)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandKeyboard("n"));
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Mission Genocide (UK) (1987).dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Mission Genocide (UK) (1987).dsk", "run\"genocide\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk22{ 6128 } {disk} {Gryzor(UK) (1987).dsk} {run"gryzor} {170} {Gryzor}]
TEST(Dumps_Disk, Gryzor)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(4300));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Gryzor (UK) (1987).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Gryzor (UK) (1987).dsk", "run\"gryzor\r", &cmd_list));
}

//lappend testlist[SingleTest dsk23{ 6128 } {disk} {Guadalcanal(UK) (1987) (CPM)[Original](Weak Sectors).dsk} {run"disc} {397} {Guadalcanal}]
TEST(Dumps_Disk, Guadalcanal)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 0));
   cmd_list.AddCommand(new CommandRunCycles(420));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Guadalcanal (UK) (1987) (CPM) [Original] (Weak Sectors).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Guadalcanal (UK) (1987) (CPM) [Original] (Weak Sectors).dsk", "run\"disc\r", &cmd_list));
}

//[CPC] Harricana - Raid International Motoneige (1990)(Loriciels)(Fr)[RAW].raw
TEST(Dumps_Disk, Harricana)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2500));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/[CPC] Harricana - Raid International Motoneige (1990)(Loriciels)(Fr)[RAW].raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/[CPC] Harricana - Raid International Motoneige (1990)(Loriciels)(Fr)[RAW].raw", "run\"harri\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk24{ 6128 } {disk} {Hercule2.scp} {run"esat} {1070} {Hercule}]
TEST(Dumps_Disk, Hercule)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(1100));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Hercule2.scp.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Hercule2.scp", "run\"esat\r", &cmd_list));
}

//lappend testlist[SingleTest dsk25{ 6128 } {disk} {International Karate Plus(UK) (1988) (CPM)[Original](Weak Sectors).dsk} {run"ik} {678} {defaut}]
TEST(Dumps_Disk, IK_plus)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/International Karate Plus (UK) (1988) (CPM) [Original] (Weak Sectors).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/International Karate Plus (UK) (1988) (CPM) [Original] (Weak Sectors).dsk", "run\"ik\r", &cmd_list));
}

//lappend testlist[SingleTest dsk79{ 6128 } {disk} {Krypton Factor(UK) (Face A) (1987) (CPM)[Original].dsk} { | cpm} {928} {defaut}]
TEST(Dumps_Disk, KryptonFactor)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(950));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Krypton Factor (UK) (Face A) (1987) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Krypton Factor (UK) (Face A) (1987) (CPM) [Original].dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk75{ 6128 } {disk} {La Bosse Des Maths 3eme(F) (128K) (Face A) (1987) (CPM)[Original].dsk} { | cpm} {1977} {bosse}]
TEST(Dumps_Disk, LaBosseDesMath3e)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(420));
   cmd_list.AddCommand(new CommandKeyboard("loader\r"));
   cmd_list.AddCommand(new CommandRunCycles(2050));

   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/La Bosse Des Maths 3eme (F) (128K) (Face A) (1987) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/La Bosse Des Maths 3eme (F) (128K) (Face A) (1987) (CPM) [Original].dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk26{ 6128 } {disk} {La Chose De Grotemburg[A][MAXIT][SAMdisk37][Original].dsk} { | cpm} {600} {LaChose}]
TEST(Dumps_Disk, LaChoseDeGrotemburg)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(1740));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(600));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/La Chose De Grotemburg [A][MAXIT][SAMdisk37][Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/La Chose De Grotemburg [A][MAXIT][SAMdisk37][Original].dsk", "|cpm\r", &cmd_list));
}

//[CPC] La Chose De Grotemburg (1987)(Ubisoft)(Fr)(Face A)[RAW].raw
TEST(Dumps_Disk, LaChoseDeGrotemburg_RAW)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(1740));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(600));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/[CPC] La Chose De Grotemburg (1987)(Ubisoft)(Fr)(Face A)[RAW].raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/[CPC] La Chose De Grotemburg (1987)(Ubisoft)(Fr)(Face A)[RAW].raw", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk27{ 6128 } {disk} {LArche Du Captain Blood(UK, F, G, S, I) (1988) (CPM)[Original].dsk} { | cpm} {1200} {CaptainBlood}]
TEST(Dumps_Disk, LArcheDuCaptainBlood)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1100));
   cmd_list.AddCommand(new CommandKeyboard("\r"));
   cmd_list.AddCommand(new CommandRunCycles(1200));

   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/LArche Du Captain Blood (UK,F,G,S,I) (1988) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/LArche Du Captain Blood (UK,F,G,S,I) (1988) (CPM) [Original].dsk", "|cpm\r", &cmd_list));
}

//lappend testlist[SingleTest dsk86{ 6128 } {disk} {Le Necromancien(F) (Face A) (1987)[Original](GAPS) - patched.dsk} {run"disc} {200} {Necro}]
TEST(Dumps_Disk, LeNecromancien_Patched_dsk)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1300));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Le Necromancien (F) (Face A) (1987) [Original] (GAPS) - patched.dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Le Necromancien (F) (Face A) (1987) [Original] (GAPS) - patched.dsk", "run\"disc\r", &cmd_list));
}

//lappend testlist[SingleTest dsk29{ 6128 } {disk} {Le Necromancien[A][MAXIT][SAMdisk37][Original][ALLGAPS].dsk} {run"disc} {200} {Necro}]
TEST(Dumps_Disk, LeNecromancien_1Adsk)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1300));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Necromancien (F), Le - Disk 1A.dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Necromancien (F), Le - Disk 1A.dsk", "run\"disc\r", &cmd_list));
}

//lappend testlist[SingleTest dsk35{ 6128 } {disk} {Necromancien(F), Le - Disk 1A.dsk} {run"disc} {200} {Necro}]
TEST(Dumps_Disk, LeNecromancien_dsk_ALLGAP)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1300));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Le Necromancien [A][MAXIT][SAMdisk37][Original][ALLGAPS].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Le Necromancien [A][MAXIT][SAMdisk37][Original][ALLGAPS].dsk", "run\"disc\r", &cmd_list));
}

//lappend testlist[SingleTest dsk83{ 6128 } {disk} {Le Maitre Absolu(F) (Face 1) (1989) (CPM) (Modified).raw} {run"ubi} {1271} {Maitre_Abso}]
TEST(Dumps_Disk, LeMaitreAbsolu)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2800));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandRunCycles(450));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandRunCycles(400));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandRunCycles(1271));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Le Maitre Absolu (F) (Face 1) (1989) (CPM) (Modified).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Le Maitre Absolu (F) (Face 1) (1989) (CPM) (Modified).raw", "run\"ubi\r", &cmd_list));
}

//lappend testlist[SingleTest dsk80{ 6128 } {disk} {Lee Enfield - The Tournament Of Death(UK) (1988) (CPM)[Original].dsk} { | cpm} {5560} {defaut}]
TEST(Dumps_Disk, LeeEnfieldTournamentOfDeath)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(7500));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Lee Enfield - The Tournament Of Death (UK) (1988) (CPM) [Original].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Lee Enfield - The Tournament Of Death (UK) (1988) (CPM) [Original].dsk", "|cpm\r", &cmd_list, false, 0x12345678 ));
}


//[CPC] Les 4 Saisons de L'ecrit 6ème - 3ème (1989)(Generation 5)(Fr)(Face A)[RAW]
//[CPC] Les 4 Saisons de L'ecrit 6ème - 3ème (1989)(Generation 5)(Fr)(Face B)[RAW]
TEST(Dumps_Disk, Les4SaisonsDeLecrit6e3e)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2500));
   cmd_list.AddCommand(new CommandKeyboard("2"));
   cmd_list.AddCommand(new CommandRunCycles(400));
   cmd_list.AddCommand(new CommandKeyboard("3"));
   cmd_list.AddCommand(new CommandRunCycles(600));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x03, 1));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x2A, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x03, 0));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x2A, 0));
   cmd_list.AddCommand(new CommandRunCycles(300));
   cmd_list.AddCommand(new CommandKeyboard("toto"));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1C, 1));
   cmd_list.AddCommand(new CommandRunCycles(100)); 
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1C, 0));
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x02, 1));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x2A, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x02, 0));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x2A, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandKeyboard("6"));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/[CPC] Les 4 Saisons de Lecrit 6eme - 3eme (1989)(Generation 5)(Fr)(Face B)[RAW].raw"));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(1500));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(400));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(400));
   cmd_list.AddCommand(new CommandKeyboard(" "));
   cmd_list.AddCommand(new CommandRunCycles(400));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/[CPC] Les 4 Saisons de Lecrit 6eme - 3eme (1989)(Generation 5)(Fr)(Face A)[RAW].raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/[CPC] Les 4 Saisons de Lecrit 6eme - 3eme (1989)(Generation 5)(Fr)(Face A)[RAW].raw", "run\"s\r", &cmd_list, true));
}


//[CPC] Little Puff In Dragonland(UK) (1990)[EDOS - Codemasters].raw
TEST(Dumps_Disk, LittlePuffInDragonland)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2500));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/[CPC] Little Puff In Dragonland (UK) (1990) [EDOS - Codemasters].raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/[CPC] Little Puff In Dragonland (UK) (1990) [EDOS - Codemasters].raw", "run\"disk\r", &cmd_list));
}

//lappend testlist[SingleTest dsk31{ 6128 } {disk} {Marauder(UK) (1988)[Original].dsk} {run"disc} {50} {Marauder}]
TEST(Dumps_Disk, Marauder)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x22, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x22, 0));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Marauder (UK) (1988) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Marauder (UK) (1988) [Original].dsk", "run\"disc\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk32{ 6128 } {disk} {microCLUB 9[B][MAXIT][SAMdisk37][Original].dsk} {run"disc} {1200} {defaut}]
TEST(Dumps_Disk, MicroClub)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1300));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/microCLUB 9 [B][MAXIT][SAMdisk37][Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/microCLUB 9 [B][MAXIT][SAMdisk37][Original].dsk", "run\"disc\r", &cmd_list, true));
}

//MikeEtMoko_FaceA.raw
TEST(Dumps_Disk, MikeEtMoko)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(3000));
   cmd_list.AddCommand(new CommandKeyboard("j"));
   cmd_list.AddCommand(new CommandRunCycles(1500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/MikeEtMoko_FaceA.raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/MikeEtMoko_FaceA.raw", "run\"MBC\r", &cmd_list, true));
}

//Monty Python's Flying Circus.raw
TEST(Dumps_Disk, MontyPythonFlyingCircus)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Monty Python's Flying Circus.raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Monty Python's Flying Circus.raw", "run\"disc\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk34{ 6128 } {disk} {Mot(S) (Face 1) (1989) (CPM)[Original].dsk} { | cpm} {1200} {Mot}]
TEST(Dumps_Disk, Mot)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(800));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Mot (S) (Face 1) (1989) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Mot (S) (Face 1) (1989) (CPM) [Original].dsk", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk36{ 6128 } {disk} {Nemesis(UK) (1987) (CPM)[Original].dsk} { | cpm} {147} {Nemesis}]
TEST(Dumps_Disk, Nemesis)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(800));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Nemesis (UK) (1987) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Nemesis (UK) (1987) (CPM) [Original].dsk", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk82{ 6128 } {disk} {Nigel Mansell's Grand Prix [MAXIT][SAMdisk387][Original][ALLGAPS].dsk} {run"mansell} {951} {defaut}]
TEST(Dumps_Disk, NigelMansell)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1100));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Nigel Mansell's Grand Prix [MAXIT][SAMdisk387][Original][ALLGAPS].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Nigel Mansell's Grand Prix [MAXIT][SAMdisk387][Original][ALLGAPS].dsk", "run\"mansell\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk37{ 6128 } {disk} {Orion Prime(F) (128K) (2009) (PD) (Version double size).ipf} {run"orion} {512} {defaut}]
TEST(Dumps_Disk, OrionPrime)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(800));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Orion Prime (F) (128K) (2009) (PD) (Version double size).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Orion Prime (F) (128K) (2009) (PD) (Version double size).ipf", "run\"orion\r", &cmd_list, true));
}

// OperationThunderbolt
TEST(Dumps_Disk, OperationThunderbolt)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2500));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Operation Thunderbolt_B.raw"));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(350));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x05, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x05, 0));

   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Operation Thunderbolt_A.raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Operation Thunderbolt_A.raw", "run\"disc\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk37{ 6128 } {disk} {Orphee(1985)(Loriciels)(Fr)(Disk 1 of 2).dsk} {run"orphee} {512} {Orphee}]
TEST(Dumps_Disk, Orphee)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(350));
   cmd_list.AddCommand(new CommandKeyboard("J"));
   cmd_list.AddCommand(new CommandRunCycles(800));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Orphee (1985)(Loriciels)(Fr)(Disk 2 of 2).dsk"));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(600));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(600));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Orphee (1985)(Loriciels)(Fr)(Disk 1 of 2).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Orphee (1985)(Loriciels)(Fr)(Disk 1 of 2).dsk", "run\"orphee\r", &cmd_list, true));
}
//lappend testlist[SingleTest dsk39{ 6128 } {disk} {Prehistorik(UK) (1991)[Original].dsk} {run"Disc} {1943} {defaut}]
TEST(Dumps_Disk, Prehistorik)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2000));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Prehistorik (UK) (1991) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Prehistorik (UK) (1991) [Original].dsk", "run\"disc\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk40{ 6128 } {disk} {Profession Detective(F) (Face 1A) (1987)[Original](GAPS).dsk} {run"ubi} {349} {Profdetec}]
TEST(Dumps_Disk, Prof_Detective)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(350));
   // move joystick upn "play"
   cmd_list.AddCommand(new CommandJoystick(0, 2));
   cmd_list.AddCommand(new CommandRunCycles(517));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 0));
   cmd_list.AddCommand(new CommandRunCycles(4000));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Profession Detective (F) (Face 1B) (1987) [Original].dsk"));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(400));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Profession Detective (F) (Face 1A) (1987) [Original] (GAPS).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Profession Detective (F) (Face 1A) (1987) [Original] (GAPS).dsk", "run\"ubi\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk42{ 6128 } {disk} {Prohibition[A].raw} { | cpm} {140} {Prohibition}]
TEST(Dumps_Disk, Prohibition)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1400));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(150));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Prohibition [A].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Prohibition [A].raw", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk68{ 6128 } {disk} {Puffys Saga(UK) (Face A) (1989) (CPM)[Original](Weak Sectors).dsk} { | cpm} {1589} {defaut}]
TEST(Dumps_Disk, PuffysSaga)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1700));
   cmd_list.AddCommand(new CommandEjectDisk());
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Puffy's Saga [B].ipf"));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Puffys Saga (UK) (Face A) (1989) (CPM) [Original] (Weak Sectors).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Puffys Saga (UK) (Face A) (1989) (CPM) [Original] (Weak Sectors).dsk", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk43{ 6128 } {disk} {Qin(F) (Face 1A) (1987) (v2)[Original](GAPS).dsk} {run"ere} {148} {Qin}]
TEST(Dumps_Disk, QinV2)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2200));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x01, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x01, 0));
   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Qin (F) (Face 1B) (1987) (v2) [Original].dsk"));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(150));
   cmd_list.AddCommand(new CommandKeyboard("N"));
   cmd_list.AddCommand(new CommandRunCycles(150));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Qin (F) (Face 1A) (1987) (v2) [Original] (GAPS).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Qin (F) (Face 1A) (1987) (v2) [Original] (GAPS).dsk", "run\"ere\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk44{ 6128 } {disk} {Qin(F) (Face 1A) (1987)[Original].dsk} {run"ere} {148} {Qin2}]
TEST(Dumps_Disk, Qin)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2200));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x01, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x01, 0));
   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Qin (F) (Face 1B) (1987) [Original].dsk"));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(150));
   cmd_list.AddCommand(new CommandKeyboard("N"));
   cmd_list.AddCommand(new CommandRunCycles(150));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Qin (F) (Face 1A) (1987) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Qin (F) (Face 1A) (1987) [Original].dsk", "run\"ere\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk81{ 6128 } {disk} {Questions - Reponses(F) (1987) (CPM)[Original](GAPS).dsk} { | cpm} {1081} {defaut}]
TEST(Dumps_Disk, QuestionReponse)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Questions - Reponses (F) (1987) (CPM) [Original] (GAPS).bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Questions - Reponses (F) (1987) (CPM) [Original] (GAPS).dsk", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk88{ 6128 } {disk} {Reussir Conjugaison[A][MAXIT][SAMdisk38][Original][OVERLAP].dsk} {run"r"} {1081} {defaut}]
TEST(Dumps_Disk, ReussirConjugaison)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Reussir Conjugaison [A][MAXIT][SAMdisk38][Original][OVERLAP].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Reussir Conjugaison [A][MAXIT][SAMdisk38][Original][OVERLAP].dsk", "run\"r\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk88{ 6128 } {disk} {Reussir Conjugaison[A][MAXIT][SAMdisk38][Original][OVERLAP].dsk} {run"r"} {1081} {defaut}]
TEST(Dumps_Disk, Reussir4Operations)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Reussir Les 4 Operations [A][MAXIT][SAMdisk387][Original][GAPS].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Reussir Les 4 Operations [A][MAXIT][SAMdisk387][Original][GAPS].dsk", "run\"r\r", &cmd_list, true));
}

TEST(Dumps_Disk, ReussirMathematiques)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Reussir Mathematiques CM [A][MAXIT][SAMdisk387][Original][GAPS].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Reussir Mathematiques CM [A][MAXIT][SAMdisk387][Original][GAPS].dsk", "run\"r\r", &cmd_list, true));
}

TEST(Dumps_Disk, SecuTrack)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(600));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/secutrack.bmp", SCR_CREATE/*SCR_COMPARE*/));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/secutrack.hfe", "|cpm\r", &cmd_list, true));
}


//lappend testlist[SingleTest dsk47{ 6128 } {disk} {Shadow Of The Beast(UK) (Face 1) (1990) (CPM)[Original].dsk} { | cpm} {491} {SotB}]
TEST(Dumps_Disk, ShadowOfTheBeast)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(900));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Shadow Of The Beast (UK) (Face 2) (1990) (CPM) [Original].dsk"));
   cmd_list.AddCommand(new CommandRunCycles(10));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Shadow Of The Beast (UK) (Face 1) (1990) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Shadow Of The Beast (UK) (Face 1) (1990) (CPM) [Original].dsk", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk84{ 6128 } {disk} {Skyx(F) (Face A) (1988) (CPM) (GAPS) (!).raw} { | cpm} {948} {defaut}]
TEST(Dumps_Disk, Skyx)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Skyx (F) (Face A) (1988) (CPM) (GAPS) (!).raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Skyx (F) (Face A) (1988) (CPM) (GAPS) (!).raw", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk49{ 6128 } {disk} {Sol Negro - Soleil Noir(S) (1988) (CPM)[Original].dsk} { | cpm} {100} {SolNegro}]
TEST(Dumps_Disk, SolNegro)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x02, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x02, 0));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(150));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Sol Negro - Soleil Noir (S) (1988) (CPM) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Sol Negro - Soleil Noir (S) (1988) (CPM) [Original].dsk", "|cpm\r", &cmd_list, true));
}

//Space Ace [A]-atteindre le menu.raw
TEST(Dumps_Disk, SpaceAceCompil)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Space Ace [A]-atteindre le menu.raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Space Ace [A]-atteindre le menu.raw", "run\"disk\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk50{ 6128 } {disk} {Sphaira_FaceA.raw} { | cpm} {800} {Sphaira}]
TEST(Dumps_Disk, Sphaira)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1550));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(12));
   cmd_list.AddCommand(new CommandKeyboard("1"));
   cmd_list.AddCommand(new CommandRunCycles(576));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Sphaira_FaceB.raw"));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(800));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Sphaira_FaceA.raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Sphaira_FaceA.raw", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk52{ 6128 } {disk} {Sram(1986)(Ere Software)(M3)(Disk 1 of 2).dsk} {run"sram} {447} {Sram}]
TEST(Dumps_Disk, Sram)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1200));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(307));
   cmd_list.AddCommand(new CommandKeyboard("F"));
   cmd_list.AddCommand(new CommandRunCycles(840));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(40));
   cmd_list.AddCommand(new CommandKeyboard("N"));
   cmd_list.AddCommand(new CommandRunCycles(1250));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Sram (1986)(Ere Software)(M3)(Disk 2 of 2).dsk"));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(450));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Sram (1986)(Ere Software)(M3)(Disk 1 of 2).dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Sram (1986)(Ere Software)(M3)(Disk 1 of 2).dsk", "run\"sram\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk54{ 6128 } {disk} {Sram 2 (1986)(Ere Software)(Fr)(Disk 1 of 2).dsk} {run"ere} {479} {Sram2}]
TEST(Dumps_Disk, Sram2)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1750));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(2800));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(876));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Sram 2 (1986)(Ere Software)(Fr)(Disk 2 of 2).dsk"));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Sram 2 (1986)(Ere Software)(Fr)(Disk 1 of 2).dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Sram 2 (1986)(Ere Software)(Fr)(Disk 1 of 2).dsk", "run\"ere\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk56{ 6128 } {disk} {Starfox(UK) (1987) (CPM)[Original].dsk} { | cpm} {135} {Starfox}]
TEST(Dumps_Disk, Starfox)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(800));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1C, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1C, 0));
   cmd_list.AddCommand(new CommandRunCycles(150));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Starfox (UK) (1987) (CPM) [Original].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Starfox (UK) (1987) (CPM) [Original].dsk", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk57{ 6128 } {disk} {Strider.raw} {run"disk} {437} {Strider}]
TEST(Dumps_Disk, Strider)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1400));
   cmd_list.AddCommand(new CommandKeyboard("j"));
   cmd_list.AddCommand(new CommandRunCycles(450));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Strider.raw.dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Strider.raw", "run\"disk\r", &cmd_list, true));
}

//The Fury (UK) (1988) [Original].dsk
TEST(Dumps_Disk, TheFury)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1000));
   // move joystick upn "play"
   cmd_list.AddCommand(new CommandJoystick(0, 1 | 8));
   cmd_list.AddCommand(new CommandRunCycles(15));
   cmd_list.AddCommand(new CommandJoystick(0, 8));
   cmd_list.AddCommand(new CommandRunCycles(38));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 0));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/The Fury (UK) (1988) [Original].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/The Fury (UK) (1988) [Original].dsk", "run\"fury\r", &cmd_list, true));
}
//lappend testlist[SingleTest dsk60{ 6128 } {disk} {Titan(UK, F) (1988) (CPM)[Original].dsk} { | cpm} {331} {Titan}]
TEST(Dumps_Disk, Titan)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(750));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(213));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(331));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Titan (UK, F) (1988) (CPM) [Original].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Titan (UK, F) (1988) (CPM) [Original].dsk", "|cpm\r", &cmd_list, true));
}

//Titus Classiques Volume 1 (F) (1987) (CPM) (Modified).raw
TEST(Dumps_Disk, TitusClassiquesVol1)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1500));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(2000));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Titus Classiques Volume 1 (F) (1987) (CPM) (Modified).raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Titus Classiques Volume 1 (F) (1987) (CPM) (Modified).raw", "run\"jeu\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk61{ 6128 } {disk} {Tony Truand(1985)(Loriciels)(Fr).dsk} {run"Tony} {1667} {Tony}]
TEST(Dumps_Disk, TonyTruand)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(320));
   cmd_list.AddCommand(new CommandKeyboard("j"));
   cmd_list.AddCommand(new CommandRunCycles(2000));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1C, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1C, 0));
   cmd_list.AddCommand(new CommandRunCycles(1667));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Tony Truand (1985)(Loriciels)(Fr).dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Tony Truand (1985)(Loriciels)(Fr).dsk", "run\"tony\r", &cmd_list, true));
}

// Tour 91 (S) (1991) (!) (Not Duplicated).raw
TEST(Dumps_Disk, Tour91)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1500));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x0B, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x0B, 0));
   cmd_list.AddCommand(new CommandRunCycles(1000));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Tour 91 (S) (1991) (!) (Not Duplicated).raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Tour 91 (S) (1991) (!) (Not Duplicated).raw", "run\"disc\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk62{ 6128 } {disk} {Turbo Cup(F) (1988)[Original].dsk} {run"Turbo} {145} {Turbo}]
TEST(Dumps_Disk, TurboCup)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1250));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(150));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Turbo Cup (F) (1988) [Original].bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Turbo Cup (F) (1988) [Original].dsk", "run\"turbo\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk63{ 6128 } {disk} {Ulysse(F) (1988)[Original][UTILITAIRE].dsk} {run"esat} {1130} {Hercule}]
TEST(Dumps_Disk, Ulysse)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(50));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(1100));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Ulysse (F) (1988) [Original] [UTILITAIRE].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Ulysse (F) (1988) [Original] [UTILITAIRE].dsk", "run\"esat\r", &cmd_list));
}

//lappend testlist[SingleTest dsk64{ 6128 } {disk} {Unique Megademo(UK) (128K) (Face A) (1997) (CPM)[CPC CPC + ][Original][DEMO].dsk} { | cpm} {363} {Unique}]
TEST(Dumps_Disk, Unique)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(700));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(6276));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(370));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Unique Megademo (UK) (128K) (Face A) (1997) (CPM) [CPC CPC+] [Original] [DEMO].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Unique Megademo (UK) (128K) (Face A) (1997) (CPM) [CPC CPC+] [Original] [DEMO].dsk", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk85{ 6128 } {disk} {War In Middle Earth(UK) (1987) (CPM)[Original].dsk} { | cpm} {1500} {defaut}]
TEST(Dumps_Disk, WarInTheMiddleEarth)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(1500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/War In Middle Earth (UK) (1987) (CPM) [Original].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/War In Middle Earth (UK) (1987) (CPM) [Original].dsk", "|cpm\r", &cmd_list, true));
}

//lappend testlist[SingleTest dsk70{ 6128 } {disk} {Wild Streets[MAXIT][SAMdisk38][Original][GAPS].dsk} {run"wild} {2782} {defaut}]
TEST(Dumps_Disk, WildStreet)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2900));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Wild Streets [MAXIT][SAMdisk38][Original][GAPS].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Wild Streets [MAXIT][SAMdisk38][Original][GAPS].dsk", "run\"wild\r", &cmd_list, true));
}

// xMas2k17.DSK
TEST(Dumps_Disk, xMas2k17)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/xMas2k17.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/xMas2k17.DSK", "run\"XMAS2K17\r", &cmd_list));
}

//lappend testlist[SingleTest dsk69{ 6128 } {disk} {Zap Pak(UK) (1987)[Original][COMPILATION].dsk} {run"Disc} {200} {zap}]
TEST(Dumps_Disk, ZapPak)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandKeyboard("2"));
   cmd_list.AddCommand(new CommandRunCycles(1700));
   cmd_list.AddCommand(new CommandKeyboard("P"));
   cmd_list.AddCommand(new CommandRunCycles(200));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Zap Pak (UK) (1987) [Original] [COMPILATION].dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Zap Pak (UK) (1987) [Original] [COMPILATION].dsk", "run\"disc\r", &cmd_list, true));
}

//Zaxx.raw
TEST(Dumps_Disk, Zaxx)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2500));
   cmd_list.AddCommand(new CommandJoystick(0, 64));
   cmd_list.AddCommand(new CommandRunCycles(20));
   cmd_list.AddCommand(new CommandJoystick(0, 0));
   cmd_list.AddCommand(new CommandRunCycles(300));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Zaxx.raw.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Zaxx.raw", "run\"jeu\r", &cmd_list));
}

//lappend testlist[SingleTest dsk67{ 6128 } {disk} {Zombi(1986)(Ubi Soft)(Fr)(Disk 1 of 2).dsk} {run"zomb} {500} {Zomb}]
TEST(Dumps_Disk, Zombi)
{
   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(2300));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x39, 0));
   cmd_list.AddCommand(new CommandRunCycles(770));
   cmd_list.AddCommand(new CommandInsertDisk("./res/DSK/Zombi (1986)(Ubi Soft)(Fr)(Disk 2 of 2)(6128 version).dsk"));
   cmd_list.AddCommand(new CommandRunCycles(50)); 
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1c, 1));
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), 0x1c, 0));
   cmd_list.AddCommand(new CommandRunCycles(500));
   cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, "./res/Record/Zombi (1986)(Ubi Soft)(Fr)(Disk 1 of 2).dsk.bmp", SCR_COMPARE));
   ASSERT_EQ(true, test_dump.Test("6128", "./TestConf.ini", "./res/DSK/Zombi (1986)(Ubi Soft)(Fr)(Disk 1 of 2).dsk", "run\"zomb\r", &cmd_list, true));
}

