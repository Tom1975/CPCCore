
#ifdef _WIN32
   #define _CRT_SECURE_NO_WARNINGS
   #define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include "TestUtils.h"
#include <iostream>

#include "gtest/gtest.h"
#include "Machine.h"
#include "Display.h"

/////////////////////////////////////////////////////////////
/// Helper functions



bool TestAutorun(char* dump_to_load, const char* command_to_run)
{
   DiskGen disk_gen;
   std::string log_file = dump_to_load;
   log_file += ".log";
   FileLog log(log_file.c_str());

   disk_gen.SetLog(&log);
   disk_gen.SetFixedSpeed(true);


   // Load disk
   if (disk_gen.LoadDisk(dump_to_load) != 0)
   {
      char buffer[128] = { 0 };
      sprintf(buffer, "Loading error !");
      return false;
   }

   char command[16] = { 0 };
   IDisk::AutorunType autorun_type = disk_gen.GetAutorun(command, 16);
   if (autorun_type == IDisk::AUTO_CPM)
   {
      strcpy(command, "|CPM");
   }

   int ret = strcmp(command_to_run, command);
   if (ret != 0)
   {
      disk_gen.DumpTrack(0, 2);
   }

   return (ret == 0);
   
}

/////////////////////////////////////////////////////////////
// Test functions
TEST(Autorun, fdctest)
{
   ASSERT_EQ(true, TestAutorun ( "res/FDC/fdctest.dsk", "FDCTEST" ) );
}

TEST(Autorun, 4SaisonDeLEcrit)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/[CPC] Les 4 Saisons de L'ecrit 6ème - 3ème (1989)(Generation 5)(Fr)(Face A)[RAW].raw", "S"));
}

TEST(Autorun, AnglaisCollege4e3eUK)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Anglais College 4e-3e (UK) (Face A) (1988) [Original].dsk", "DEPART"));
}

TEST(Autorun, DISABLED_BaladeOutreRhin)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Balade Outre-Rhin (F,G) (1986) (CPM) [Original].dsk", "|CPM"));
}

TEST(Autorun, Barbarian2)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Barbarian 2 (1989)(Palace Software).dsk", "DISC"));
}

TEST(Autorun, DISABLED_Basun)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Basun (UK) (CPM) [Copia] (no funciona).dsk", "|CPM"));
}

TEST(Autorun, Catch23)
 {
   ASSERT_EQ(true, TestAutorun("res/DSK/Catch 23 (UK) (1987) (CPM) [Original].dsk", "CATCH"));
}

TEST(Autorun, Dogfight)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Dogfight 2187_Ariolasoft.raw", "AS"));
}

TEST(Autorun, FetEtFlammes)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Fer & Flamme (F) (Face 1) (1986) [Original] (Weak Sectors).dsk", "F&F"));
}

TEST(Autorun, Hercule2)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Hercule2.scp", "ESAT"));
}

TEST(Autorun, DISABLED_LaBosseDesMath)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/La Bosse Des Maths 3eme (F) (128K) (Face A) (1987) (CPM) [Original].dsk", "|CPM"));
}

TEST(Autorun, MissionGenocide)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Mission Genocide (UK) (1987).dsk", "GENOCIDE"));
}

TEST(Autorun, Nemesis)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Nemesis (UK) (1987) (CPM) [Original].dsk", "NEM"));
}

TEST(Autorun, Orphee)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Orphee (1985)(Loriciels)(Fr)(Disk 1 of 2).dsk", "ORPHEE"));
}

TEST(Autorun, ProfessionDetective)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Profession Detective (F) (Face 1A) (1987) [Original] (GAPS).dsk", "UBI"));
}

TEST(Autorun, QuestionReponse)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Questions - Reponses (F) (1987) (CPM) [Original] (GAPS).dsk", "|CPM"));
}

TEST(Autorun, SRAM)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Sram (1986)(Ere Software)(M3)(Disk 1 of 2).dsk", "SRAM"));
}

TEST(Autorun, TonyTruand)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Tony Truand (1985)(Loriciels)(Fr).dsk", "TONY"));
}

TEST(Autorun, TurboCup)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Turbo Cup (F) (1988) [Original].dsk", "TURBO"));
}

TEST(Autorun, Ulysse)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Ulysse (F) (1988) [Original] [UTILITAIRE].dsk", "ESAT"));
}

TEST(Autorun, DISABLED_UniqueMegademo)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Unique Megademo (UK) (128K) (Face A) (1997) (CPM) [CPC CPC+] [Original] [DEMO].dsk", "|CPM"));
}

TEST(Autorun, WildStreet)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Wild Streets [MAXIT][SAMdisk38][Original][GAPS].dsk", "WILD"));
}

TEST(Autorun, DISABLED_Zombi)
{
   ASSERT_EQ(true, TestAutorun("res/DSK/Zombi (1986)(Ubi Soft)(Fr)(Disk 1 of 2).dsk", "ZOMB"));
}