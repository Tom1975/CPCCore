
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>

#include "tchar.h"
#include "windows.h"
#include "gtest/gtest.h"

#include "TestUtils.h"

#define BUILD 1
#define EXECUTE 0

////////////////////////////////////
// PROTECTIONS
////////////////////////////////////

////////////////////////////////////
// Protection Alkatraz
// E-motion (UK) (1990) (UK retail version) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Alkatraz_Emotion_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\E-motion (UK) (1990) (UK retail version) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\E-motion (UK) (1990) (UK retail version) [Original] [TAPE].cdt_1.txt", 0x9D7F, 0x500, "A", 57000, EXECUTE));
}

////////////////////////////////////
// Protection  Ariolasoft K7
//  - TODO
TEST(Dumps_Tape_Protections, DISABLED_Ariolasoft_TODO)
{
   // KO ! Bride of Frankenstein & Werner - Mach hin
}

////////////////////////////////////
// Protection  Bleepload v1
// Thrust (UK) (1986) (v2) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Bleepload_v1_Thrust_cdt)
{
   
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Thrust (UK) (1986) (v2) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Thrust (UK) (1986) (v2) [Original] [TAPE].cdt_1.txt", 0x3EC2, 0x7000, "A", 100000, EXECUTE));
   //// 28bc, 38b1, 3ec2
}

////////////////////////////////////
// Protection  Bleepload v2
// Flying Shark (UK) (1987) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Bleepload_v2_Flying_shark_cdt)
{

   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Flying Shark (UK) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Flying Shark (UK) (1987) [Original] [TAPE].cdt_1.txt", 0x07E3, 0xC000, "A", 53000, EXECUTE));
   //// 28bc, 38b1, 3ec2
}

////////////////////////////////////
// Protection  Bleepload v3
// Booty (UK) (1986) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Bleepload_v3_Booty_cdt)
{

   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Booty (UK) (1986) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Booty (UK) (1986) [Original] [TAPE].cdt_1.txt", 0x1C65, 0x6001, "D", 43000, EXECUTE));
}

////////////////////////////////////
// Protection  Cassys
// 007 The Living Daylights (UK) (1987) (UK retail version) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Cassys_Living_daylights_cdt)
{

   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\007 The Living Daylights (UK) (1987) (UK retail version) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\007 The Living Daylights (UK) (1987) (UK retail version) [Original] [TAPE].cdt_1.txt", 0x28BC, 0x78D, "A", 61000, EXECUTE));
}

////////////////////////////////////
// Protection Codemaster
// Little Puff In Dragonland (UK) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Codemaster_Little_puff_in_dragonland_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Little Puff In Dragonland (UK) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Little Puff In Dragonland (UK) (1989) [Original] [TAPE].cdt_1.txt", 0xBC18, 0x2718, "L", 57000, EXECUTE));
}

////////////////////////////////////
// Custom loader 
// Cauldron II (UK) (1988) (Silverbird) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Custom_Cauldron_II_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Cauldron II (UK) (1988) (Silverbird) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Cauldron II (UK) (1988) (Silverbird) [Original] [TAPE].cdt_1.txt", 0x28BC, 0xC000, "A", 77000, EXECUTE));
}

////////////////////////////////////
// Custom musical
// The Red Arrows (UK) (1985) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, DISABLED_Custom_musical_The_red_arrow_cdt)
{
   // TODO
}

////////////////////////////////////
// Digital Integration
// TODO KO !!
TEST(Dumps_Tape_Protections, DISABLED_Digital_Integration_TODO)
{
   // TODO
}

////////////////////////////////////
// Dinamic poliload
// After The War  (UK) (Face 1) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Dinamic_Poliload_After_The_War_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\After The War  (UK) (Face 1) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\After The War  (UK) (Face 1) (1989) [Original] [TAPE].cdt_1.txt", 0xA4C0, 0x7FBC, "A", 80000, EXECUTE));
}

// El Capitan Trueno (S) (Face A) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Dinamic_Poliload_ElCapitanTrueno_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\El Capitan Trueno (S) (Face A) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\El Capitan Trueno (S) (Face A) (1989) [Original] [TAPE].cdt_1.txt",
      0xA563, 0x3B1, "L", 68000, EXECUTE));
}

////////////////////////////////////
// Elite MFM K7 Encoding
// Frank Brunos Boxing(UK) (Face 1) (1985) (v1)[Original][TAPE].cdt
TEST(Dumps_Tape_Protections, Elite_MFM_Frank_Brunos_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Frank Brunos Boxing (UK) (Face 1) (1985) (v1) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Frank Brunos Boxing (UK) (Face 1) (1985) (v1) [Original] [TAPE].cdt_1.txt", 0xBD4C, 0x55A, "C", 49000, EXECUTE));
}

////////////////////////////////////
// Elmar Krieger
// Prehistorik II (UK,F,G) (1993) [Original] [TAPE].cdt
// Super Cauldron (UK,F,G) (1993) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Elmar_Krieger_Prehistorik_2_cdt)
{
   // Chgt
   TestTape test;
   // Press space
   CommandKeyboard cmd_space(" ");
   CommandRunCycles run_cycles(350);

   // First test : To the white screen, with '0' countdown
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Prehistorik II (UK,F,G) (1993) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Prehistorik II (UK,F,G) (1993) [Original] [TAPE].cdt_1.txt", 0x28BC, 0x6000, "A", 63000, EXECUTE));

   run_cycles.Action(test.machine_);
   // "Espace"
   cmd_space.Action(test.machine_);
   run_cycles.Action(test.machine_);
   // "Espace"
   cmd_space.Action(test.machine_);
   // 2nd part
   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Prehistorik II (UK,F,G) (1993) [Original] [TAPE].cdt_2.txt", 0x28BC, 0x274, "A", 49000, EXECUTE));

   // "Espace"
   run_cycles.Action(test.machine_);
   cmd_space.Action(test.machine_);
   run_cycles.Action(test.machine_);

   cmd_space.Action(test.machine_);

   // 3rd part
   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Prehistorik II (UK,F,G) (1993) [Original] [TAPE].cdt_3.txt", 0x28BC, 0x1097, "A", 19000, EXECUTE));

}

////////////////////////////////////
// Protection Gremlin loader 2
// Basil The Great Mouse Detective (UK) (1987) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Gremlin_loader_2_Basil_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Basil The Great Mouse Detective (UK) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Basil The Great Mouse Detective (UK) (1987) [Original] [TAPE].cdt_1.txt", 0x0275, 0x8000, "A", 63000, EXECUTE));
}


////////////////////////////////////
// Protection Hexagon
//Eswat - Cyber Police (UK) (64K) (Face A) (1990) [Original] [TAPE].cdt
// Eswat - Cyber Police (UK) (64K) (Face B) (1990) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Hexagon_Eswat_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Eswat - Cyber Police (UK) (64K) (Face A) (1990) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Eswat - Cyber Police(UK) (64K) (Face A) (1990)[Original][TAPE].cdt_1.txt", 0x81BE, 0x2AEB, "A", 40000, EXECUTE));

   CommandKeyboard cmd_space(" ");
   CommandRunCycles run_cycles(350);

   // Press space
   cmd_space.Action(test.machine_);
   run_cycles.Action(test.machine_);
   // Insert tape 2
   test.machine_->LoadTape(".\\res\\Tape\\Eswat - Cyber Police (UK) (64K) (Face B) (1990) [Original] [TAPE].cdt");

   // Press space
   run_cycles.Action(test.machine_);
   cmd_space.Action(test.machine_);
   run_cycles.Action(test.machine_);
   cmd_space.Action(test.machine_);

   // Next
   ASSERT_EQ(true, test.MoreTest ( ".\\res\\Tape\\Record\\Eswat - Cyber Police (UK) (64K) (Face B) (1990) [Original] [TAPE].cdt_1.txt", 0x3238, 0x14EF, "A", 19000, EXECUTE));

}

////////////////////////////////////
// Protection Laser Load
// The Krypton Factor (UK) (1987) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, LaserLoad_Krypton_Factor_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\The Krypton Factor (UK) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\The Krypton Factor (UK) (1987) [Original] [TAPE].cdt_1.txt", 0x30DF, 0x1ABC, "L", 51000, EXECUTE));
}

////////////////////////////////////
// Protection Lenslok
// Tomahawk (UK) (1986) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, DISABLED_Lenslok_Tomahawk_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Tomahawk (UK) (1986) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Tomahawk (UK) (1986) [Original] [TAPE].cdt_1.txt", 0x30DF, 0x1ABC, "L", 51000, EXECUTE));
}

////////////////////////////////////
// Protection Lockout
// Le Necromancien (F) (1987) (-Code Programme) (Version Split) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Lockout_Necromancien_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Le Necromancien (F) (1987) (-Code Programme) (Version Split) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Le Necromancien (F) (1987) (-Code Programme) (Version Split) [Original] [TAPE].cdt_1.txt", 0x28BC, 0xCF65, "A", 24000, EXECUTE));
}

////////////////////////////////////
// Protection Loriciel
// Bactron (F) (1986) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Loriciel_Bactron_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Bactron (F) (1986) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Bactron (F) (1986) [Original] [TAPE].cdt_1.txt", 0x28BC, 0x7E0B, "A", 61000, EXECUTE));
}

////////////////////////////////////
// Protection Microkey
// Despotik Design(F, UK, G) (1987)[Original][TAPE].cdt
TEST(Dumps_Tape_Protections, Microkey_DespotikDesign_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Despotik Design (F,UK,G) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Despotik Design (F,UK,G) (1987) [Original] [TAPE].cdt_1.txt", 0x28BC, 0x5563, "A", 72000, EXECUTE));
}

////////////////////////////////////
// Protection Opera soft
// Sol Negro - Soleil Noir(S) (Face A) (1988)[Original][TAPE].cdt
TEST(Dumps_Tape_Protections, OperaSoft_SolNegro_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Sol Negro - Soleil Noir (S) (Face A) (1988) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Sol Negro - Soleil Noir (S) (Face A) (1988) [Original] [TAPE].cdt_1.txt", 0x0137, 0x400, "H", 66000, EXECUTE));
}

////////////////////////////////////
// Protection Ricochet
// Stunt Car Racer - Normal Version (UK) (1990) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Ricochet_StuntCarRacer_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Stunt Car Racer - Normal Version (UK) (1990) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Stunt Car Racer - Normal Version (UK) (1990) [Original] [TAPE].cdt_1.txt", 0x42ED, 0xA684, "L", 63000, EXECUTE));
}

////////////////////////////////////
// Protection RubiK7
// Skate Ball(F) (1989)[Original][TAPE].cdt
TEST(Dumps_Tape_Protections, RubiK7_SkateBall_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Skate Ball (F) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Skate Ball (F) (1989) [Original] [TAPE].cdt_1.txt", 0xBC96, 0x6F50, "L", 57000, EXECUTE));
}

////////////////////////////////////
// Protection Spectrum
// North Star (UK) (1988) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Spectrum_NorthStar_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\North Star (UK) (1988) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\North Star (UK) (1988) [Original] [TAPE].cdt_1.txt", 0x01FE, 0x2E32, "L", 68000, EXECUTE));
}

////////////////////////////////////
// Protection Spectrum Multiload
// Deflektor (UK) (1987) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpectrumMultiload_Deflektor_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Deflektor (UK) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Deflektor (UK) (1987) [Original] [TAPE].cdt_1.txt", 0x0183, 0x0AAD, "L", 51000, EXECUTE));
}

////////////////////////////////////
// Protection Spectrum Pur
// Chart Attack (UK) (Face 1A) (1991) (Lotus Turbo Challenge - Data - HandBook) [Original] [TAPE] [COMPILATION].cdt
TEST(Dumps_Tape_Protections, SpectrumPur_Lotus_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Chart Attack (UK) (Face 1A) (1991) (Lotus Turbo Challenge - Data - HandBook) [Original] [TAPE] [COMPILATION].cdt",
      ".\\res\\Tape\\Record\\Chart Attack (UK) (Face 1A) (1991) (Lotus Turbo Challenge - Data - HandBook) [Original] [TAPE] [COMPILATION].cdt_1.txt", 0x20E2, 0x7F11, "L", 46000, EXECUTE));
}

////////////////////////////////////
// Protection Spectrum Variant 3
// Chicagos 30 (S) (1988) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpectrumVariant3_Chicagos30_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Chicagos 30 (S) (1988) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Chicagos 30 (S) (1988) [Original] [TAPE].cdt_1.txt", 0xA585, 0x2B90, "L", 65000, EXECUTE));
}

////////////////////////////////////
// Protection Spectrum Variant 4
// Spherical (UK) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpectrumVariant4_Spherical_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Spherical (UK) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Spherical (UK) (1989) [Original] [TAPE].cdt_1.txt", 0x28BC, 0xF21B, "A", 2000, EXECUTE));
   // Press space
   CommandKeyboard cmd_space(" ");
   CommandRunCycles run_cycles(100);
   run_cycles.Action(test.machine_);
   cmd_space.Action(test.machine_);

   // Second part
   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Spherical (UK) (1989) [Original] [TAPE].cdt_2.txt",
      0x9F1C, 0x89EB, "L", 74000, EXECUTE));
}

////////////////////////////////////
// Protection Spectrum Zydrloload
// The Light Corridor (UK) (1990) [Original] [TAPE]
TEST(Dumps_Tape_Protections, Spectrumzydroload_LightCorridor_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\The Light Corridor (UK) (1990) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\The Light Corridor (UK) (1990) [Original] [TAPE].cdt_1.txt", 0xD143, 0x2100, "L", 75000, EXECUTE));
}

////////////////////////////////////
// Protection SpecVar
// Gryzor.cdt
TEST(Dumps_Tape_Protections, SpecVar_Gryzor_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Gryzor.cdt",
      ".\\res\\Tape\\Record\\Gryzor.cdt_1.txt", 0x80EE, 0x0700, "L", 31000, EXECUTE));

   // Press space
   CommandKeyboard cmd_space(" ");
   CommandRunCycles run_cycles(100);
   run_cycles.Action(test.machine_);
   cmd_space.Action(test.machine_);

   // Second part
   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Gryzor.cdt_2.txt",
      0x1F58, 0x5D4  , "L", 26000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock
// Alien Highway (UK) (1986) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Speedlock_AlienHighway_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Alien Highway (UK) (1986) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Alien Highway (UK) (1986) [Original] [TAPE].cdt_1.txt", 0x28BC, 0x0E1D, "A", 13000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock DATA TYPE_1
// Barbarian II(UK) (1989)[Original][TAPE].cdt
TEST(Dumps_Tape_Protections, Speedlock_DataType1_BarbarianII_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Barbarian II (UK) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Barbarian II (UK) (1989) [Original] [TAPE].cdt_1.txt", 0xA85B, 0xB533, "E", 8500, EXECUTE));

   // Press '0'
   CommandScanCode cmd_keyboard(test.machine_->GetKeyboardHandler(), 0x0b, 1);
   CommandScanCode cmd_keyboard_up(test.machine_->GetKeyboardHandler(), 0x0b, 0);
   CommandRunCycles run_cycles(100);
   run_cycles.Action(test.machine_);
   cmd_keyboard.Action(test.machine_);
   run_cycles.Action(test.machine_);
   cmd_keyboard_up.Action(test.machine_);

   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Barbarian II (UK) (1989) [Original] [TAPE].cdt_2.txt",
      0xB860, 0x2BA3, "A", 64000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V1
// 10th_Frame__RERELEASE_KIXX.cdt
TEST(Dumps_Tape_Protections, SpeedlockV1_10thFrame_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\10th_Frame__RERELEASE_KIXX.cdt",
      ".\\res\\Tape\\Record\\10th_Frame__RERELEASE_KIXX.cdt_1.txt", 0xBC18, 0x0400, "L", 60000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V1 Chaine
// 3D Starfighter (UK) (1987) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV1Chaine_3DStarfighter_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\3D Starfighter (UK) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\3D Starfighter (UK) (1987) [Original] [TAPE].cdt_1.txt", 0xBC18, 0x1FA7, "L", 61100, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V2
// Frankenstein Junior(UK) (1987)[Codemasters Software][Original][TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV2_FrankensteinJr_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Frankenstein Junior (UK) (1987) [Codemasters Software] [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Frankenstein Junior (UK) (1987) [Codemasters Software] [Original] [TAPE].cdt_1.txt", 0xBC18, 0x610B, "L", 70000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V2 chaine
// Arkanoid (UK) (1987) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV2Chaine_Arkanoid_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Arkanoid (UK) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Arkanoid (UK) (1987) [Original] [TAPE].cdt_1.txt", 0xBC18, 0x22F, "L", 51000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V2 chaine Type 1
// Combat School (UK) (1987) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV2ChaineType1_CombatSchool_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Combat School (UK) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Combat School (UK) (1987) [Original] [TAPE].cdt_1.txt", 0xA527, 0x3E8, "A", 52000, EXECUTE));

   CommandKeyboard cmd_space(" ");
   CommandRunCycles run_cycles(100);
   run_cycles.Action(test.machine_);
   cmd_space.Action(test.machine_);

   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Combat School (UK) (1987) [Original] [TAPE].cdt_2.txt",
      0xA0D1, 0xF5F, "L", 18000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V2 chaine Type 2 
// Amstrad Gold Hits 3 (UK) (Face 2B) (1988) (6. World Class Leaderboard - Terrain B) [Original] [TAPE] [COMPILATION].cdt
TEST(Dumps_Tape_Protections, SpeedlockV2ChaineType2_Leaderboard_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Amstrad Gold Hits 3 (UK) (Face 2B) (1988) (6. World Class Leaderboard - Terrain B) [Original] [TAPE] [COMPILATION].cdt",
      ".\\res\\Tape\\Record\\Amstrad Gold Hits 3 (UK) (Face 2B) (1988) (6. World Class Leaderboard - Terrain B) [Original] [TAPE] [COMPILATION].cdt_1.txt"
      , 0xBAA4, 0x5B2A, "A", 60000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V2 Data Type 1
// Arkanoid Revenge Of Doh (UK) (1988) (UK retail version) [Original] [TAPE].cdt
// TODO : NOT WORKING !

////////////////////////////////////
// Protection Speedlock V3
// 750cc Grand Prix (UK) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV3_750cc_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\750cc Grand Prix (UK) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\750cc Grand Prix (UK) (1989) [Original] [TAPE].cdt_1.txt"
      , 0xBC18, 0x32AD, "L", 56000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V3 DATA TYPE 1
// Mega Mix (UK) (Face 2) (1989) (2. Operation Wolf) [Original] [TAPE] [COMPILATION].cdt
TEST(Dumps_Tape_Protections, SpeedlockV3DataTye1_OperationWolf_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Mega Mix (UK) (Face 2) (1989) (2. Operation Wolf) [Original] [TAPE] [COMPILATION].cdt",
      ".\\res\\Tape\\Record\\Mega Mix (UK) (Face 2) (1989) (2. Operation Wolf) [Original] [TAPE] [COMPILATION].cdt_1.txt"
      , 0xA677, 0x2E8E, "A", 52000, EXECUTE));
   // Press 1
   CommandScanCode cmd_keyboard(test.machine_->GetKeyboardHandler(), 2, 1);
   CommandScanCode cmd_keyboard_up(test.machine_->GetKeyboardHandler(), 2, 0);
   CommandRunCycles run_cycles(200);
   run_cycles.Action(test.machine_);
   cmd_keyboard.Action(test.machine_);
   run_cycles.Action(test.machine_);
   cmd_keyboard_up.Action(test.machine_);

   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Mega Mix (UK) (Face 2) (1989) (2. Operation Wolf) [Original] [TAPE] [COMPILATION].cdt_2.txt",
      0x3E80, 0x23DF, "E", 18000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V3 DATA TYPE 3
// Batman The Movie(UK) (1989)[Original][TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV3DataType3_BatmanTheMovie_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464UK", ".\\TestConf.ini",
      ".\\res\\Tape\\Batman The Movie (UK) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Batman The Movie (UK) (1989) [Original] [TAPE].cdt_1.txt"
      , 0xF703, 0x6F7F, "E", 45000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V4
// Batman The Caped Crusader (UK) (Face A) (1988) (Version Hit Squad 06) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV4_BatmanCapedCrusader_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Batman The Caped Crusader (UK) (Face A) (1988) (Version Hit Squad 06) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Batman The Caped Crusader (UK) (Face A) (1988) (Version Hit Squad 06) [Original] [TAPE].cdt_1.txt"
      , 0xA77F, 0x9424, "A", 64000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V4 DATA TYPE 1 
//Mega Mix (UK) (Face 1) (1989) (1. Dragon Ninja) [Original] [TAPE] [COMPILATION].cdt
TEST(Dumps_Tape_Protections, SpeedlockV4DataType1_DragonNinja_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Mega Mix (UK) (Face 1) (1989) (1. Dragon Ninja) [Original] [TAPE] [COMPILATION].cdt",
      ".\\res\\Tape\\Record\\Mega Mix (UK) (Face 1) (1989) (1. Dragon Ninja) [Original] [TAPE] [COMPILATION].cdt_1.txt"
      , 0xA788, 0x19A2, "A", 55000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V4 DATA TYPE 3
//Cabal (UK) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV4DataType3_Cabal_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Cabal (UK) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Cabal (UK) (1989) [Original] [TAPE].cdt_1.txt"
      , 0xA786, 0x34C, "A", 59000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V4 Spectrum
//Mega Mix (UK) (Face 4) (1989) (4. The Real Ghostbusters) [Original] [TAPE] [COMPILATION].cdt
TEST(Dumps_Tape_Protections, SpeedlockV4Spectrum_Ghostbuster_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Mega Mix (UK) (Face 4) (1989) (4. The Real Ghostbusters) [Original] [TAPE] [COMPILATION].cdt",
      ".\\res\\Tape\\Record\\Mega Mix (UK) (Face 4) (1989) (4. The Real Ghostbusters) [Original] [TAPE] [COMPILATION].cdt_1.txt"
      , 0x308B, 0x51A5, "A", 45000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V5
// Adidas Championship Tie Break (UK) (1990) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV5_AdidasChampionshipTieBreak_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Adidas Championship Tie Break (UK) (1990) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Adidas Championship Tie Break (UK) (1990) [Original] [TAPE].cdt_1.txt"
      , 0x15F, 0x415, "E", 57000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V5 DATA TYPE 3
// Operation Thunderbolt (UK) (Face A) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV5DataType3_OperationThunderbolt_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Operation Thunderbolt (UK) (Face A) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Operation Thunderbolt (UK) (Face A) (1989) [Original] [TAPE].cdt_1.txt"
      , 0xA5C3, 0x3852, "A", 58000, EXECUTE));
   // Press 4
   CommandScanCode cmd_keyboard(test.machine_->GetKeyboardHandler(), 5, 1);
   CommandScanCode cmd_keyboard_up(test.machine_->GetKeyboardHandler(), 5, 0);
   CommandRunCycles run_cycles(200);
   run_cycles.Action(test.machine_);
   cmd_keyboard.Action(test.machine_);
   run_cycles.Action(test.machine_);
   cmd_keyboard_up.Action(test.machine_);

   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Operation Thunderbolt (UK) (Face A) (1989) [Original] [TAPE].cdt_2.txt",
      0x3EC1, 0x5138, "E", 16426, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V6
// Puzznic (UK) (1990) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV6_Puzzinc_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Puzznic (UK) (1990) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Puzznic (UK) (1990) [Original] [TAPE].cdt_1.txt", 
      0xA594, 0x5FEC, "A", 51000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V6 DATA TYPE 1
// The Untouchables (UK) (Face A) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV6DataType1_Untouchable_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\The Untouchables (UK) (Face A) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\The Untouchables (UK) (Face A) (1989) [Original] [TAPE].cdt_1.txt",
      0xA594, 0x8000, "A", 56000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V6 Spectrum
// 10 Great Games II (UK) (Face 1A) (1988) (01. Death Wish 3) [Original] [TAPE] [COMPILATION].cdt
TEST(Dumps_Tape_Protections, SpeedlockV6Spectrum_DeathWish3_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\10 Great Games II (UK) (Face 1A) (1988) (01. Death Wish 3) [Original] [TAPE] [COMPILATION].cdt",
      ".\\res\\Tape\\Record\\10 Great Games II (UK) (Face 1A) (1988) (01. Death Wish 3) [Original] [TAPE] [COMPILATION].cdt_1.txt",
      0x1FE, 0x41E2, "L", 67000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V6 Spectrum variant 3
// After The War (S) (Face A) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV6SpectrumVariant3_AfterTheWar_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\After The War (S) (Face A) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\After The War (S) (Face A) (1989) [Original] [TAPE].cdt_1.txt",
      0xA523, 0x9C97, "L", 67000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V7
// Captain Planet And The Planeteers(UK) (1990)[Original][TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV7_CaptainPlanet_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Captain Planet And The Planeteers (UK) (1990) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Captain Planet And The Planeteers (UK) (1990) [Original] [TAPE].cdt_1.txt",
      0xA0FD, 0x2007, "E", 13000, EXECUTE));

   CommandKeyboard cmd_space(" ");
   CommandRunCycles run_cycles(100);

   run_cycles.Action(test.machine_);
   cmd_space.Action(test.machine_);

   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Captain Planet And The Planeteers (UK) (1990) [Original] [TAPE].cdt_2.txt",
      0xA292, 0xA0D1, "E", 47000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V7 TYPE 1
// Cisco Heat (UK) (1991) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV7Type1_CiscoHeat_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Cisco Heat (UK) (1991) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Cisco Heat (UK) (1991) [Original] [TAPE].cdt_1.txt",
      0x31C, 0x411E, "E", 53000, EXECUTE));
}

////////////////////////////////////
// Protection Speedlock V7 TYPE 3
// Lemmings (UK) (1991) (00. Code Program) (Version Split) [Original] [TAPE].cdt
// Lemmings (UK) (1991) (01. Level 01 FUN - JUST DIG!) (Version Split) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, SpeedlockV7Type3_Lemmings_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Lemmings (UK) (1991) (00. Code Program) (Version Split) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Lemmings (UK) (1991) (00. Code Program) (Version Split) [Original] [TAPE].cdt_1.txt",
      0x2D6, 0x8EEB, "E", 69000, EXECUTE));

   // Press 1
   CommandScanCode cmd_keyboard(test.machine_->GetKeyboardHandler(), 2, 1);
   CommandScanCode cmd_keyboard_up(test.machine_->GetKeyboardHandler(), 2, 0);
   CommandRunCycles run_cycles(100);
   run_cycles.Action(test.machine_);
   cmd_keyboard.Action(test.machine_);
   run_cycles.Action(test.machine_);
   cmd_keyboard_up.Action(test.machine_);

   // Press space 
   CommandKeyboard cmd_space(" ");
   cmd_space.Action(test.machine_);

   // Insert tape 2
   test.machine_->LoadTape(".\\res\\Tape\\Lemmings (UK) (1991) (01. Level 01 FUN - JUST DIG!) (Version Split) [Original] [TAPE].cdt");

   // Load Lemmings (UK) (1991) (01. Level 01 FUN - JUST DIG!) (Version Split) [Original] [TAPE].cdt
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Lemmings (UK) (1991) (01. Level 01 FUN - JUST DIG!) (Version Split) [Original] [TAPE].cdt_1.txt",
      0xF62F, 0xB646, "E", 5000, EXECUTE));

}

////////////////////////////////////
// Protection Titus K7 FM
// Knight Force (UK) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, TitusK7FM_KnightForce_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Knight Force (UK) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Knight Force (UK) (1989) [Original] [TAPE].cdt_1.txt",
      0x6506, 0x7B61, "A", 48000, EXECUTE));
}

////////////////////////////////////
// Protection Uniload
// Gazza II (UK) (1990) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, Uniload_GazzaII_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Gazza II (UK) (1990) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Gazza II (UK) (1990) [Original] [TAPE].cdt_1.txt",
      0x70A8, 0x0C00, "C", 43000, EXECUTE));
}

////////////////////////////////////
// Protection  USGOld
// Gauntlet II (UK) (Face A) (1986) [Original] [TAPE].cdt
TEST(Dumps_Tape_Protections, USGold_GauntletII_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Gauntlet II (UK) (Face A) (1986) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Gauntlet II (UK) (Face A) (1986) [Original] [TAPE].cdt_1.txt",
      0x4BD, 0xC00, "A", 38000, EXECUTE));
}

////////////////////////////////////
// Protection Zydroload
// Not working ! Todo


////////////////////////////////////
// Specific Tape blocks
////////////////////////////////////

////////////////////////////////////
// ID10-11 : Speed data
//1942 + Batty (UK) (1989) (Spain retail version) [Original] [TAPE] [COMPILATION].cdt
TEST(Dumps_Tape_Format_CDT, ID10_11_1942_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\1942 + Batty (UK) (1989) (Spain retail version) [Original] [TAPE] [COMPILATION].cdt",
      ".\\res\\Tape\\Record\\1942 + Batty (UK) (1989) (Spain retail version) [Original] [TAPE] [COMPILATION].cdt_1.txt",
      0xA7FE, 0x2B0A, "L", 61000, EXECUTE));
}

////////////////////////////////////
// ID12 : Pure tone
//Bubble Bobble (UK) (1987) [Original] [TAPE].cdt
TEST(Dumps_Tape_Format_CDT, ID12_BubbleBobble_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Bubble Bobble (UK) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Bubble Bobble (UK) (1987) [Original] [TAPE].cdt_1.txt",
      0x3CD1, 0x1C3, "A", 61000, EXECUTE));
}

////////////////////////////////////
// ID15 : Direct Recording block
// One (UK) (1986) [Original] [TAPE].cdt
TEST(Dumps_Tape_Format_CDT, ID15_One_cdt)
{

   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\One (UK) (1986) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\One (UK) (1986) [Original] [TAPE].cdt_1.txt",
      0xF8C, 0x116A, "A", 67000, EXECUTE));

   // Space
   CommandKeyboard cmd_space(" ");
   cmd_space.Action(test.machine_);

   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\One (UK) (1986) [Original] [TAPE].cdt_2.txt",
      0xF8C, 0x49AC, "A", 66000, EXECUTE));

}

////////////////////////////////////
// ID32 : Archive information block
// 007 The Living Daylights (UK) (1987) (Spain retail version) [Original] [TAPE].cdt
TEST(Dumps_Tape_Format_CDT, ID32_Living_daylights_cdt)
{

   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\007 The Living Daylights (UK) (1987) (Spain retail version) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\007 The Living Daylights (UK) (1987) (Spain retail version) [Original] [TAPE].cdt_1.txt", 0x28BC, 0x78D, "A", 61000, EXECUTE));
}



////////////////////////////////////
// NON REGRESSION / Specific 
////////////////////////////////////

// Bad Cat (UK) (1987) [Original] [TAPE].cdt
TEST(Dumps_Tape_Other, BadCat_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Bad Cat (UK) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Bad Cat (UK) (1987) [Original] [TAPE].cdt_1.txt",
      0x28BC, 0x542F, "A", 28000, EXECUTE));

   //test.machine_->CleanBreakpoints();

   CommandRunCycles run_cycles(250);
   run_cycles.Action(test.machine_);

   // Space
   CommandKeyboard cmd_space(" ");
   cmd_space.Action(test.machine_);

   run_cycles.Action(test.machine_);

   // Space
   cmd_space.Action(test.machine_);

   // Wait a bit
   run_cycles.Action(test.machine_);

   // Name + enter
   CommandKeyboard cmd_name("Test\r");
   cmd_name.Action(test.machine_);

   // 2nd load part
   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\Bad Cat (UK) (1987) [Original] [TAPE].cdt_2.txt",
      0x28BC, 0x31A3, "A", 37000, EXECUTE));
}


// Enterprise (UK) (1987) [Original] [TAPE].cdt
TEST(Dumps_Tape_Other, Enterprise_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Enterprise (UK) (1987) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Enterprise (UK) (1987) [Original] [TAPE].cdt_1.txt", 0xBDF5, 0xBD5A, "L", 100000, EXECUTE));
}

// Footballer Of The Year 2 (UK) (1989) [Original] [TAPE].cdt
TEST(Dumps_Tape_Other, FOTY2_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Footballer Of The Year 2 (UK) (1989) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Footballer Of The Year 2 (UK) (1989) [Original] [TAPE].cdt_1.txt", 0x9D0C, 0x9C8E, "A", 54000, EXECUTE));
}

// Kikekankoi Face Programme Principal.cdt
TEST(Dumps_Tape_Other, Kikekankoi_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Kikekankoi Face Programme Principal.cdt",
      ".\\res\\Tape\\Record\\Kikekankoi Face Programme Principal.cdt_1.txt",
      0x28BC, 0xFA2A, "A", 48000, EXECUTE));
}

// Marmelade (F) (1987) (Version Basic 1.0) [Original] [TAPE]
TEST(Dumps_Tape_Other, Marmelade_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\Marmelade (F) (1987) (Version Basic 1.0) [Original] [TAPE].cdt",
      ".\\res\\Tape\\Record\\Marmelade (F) (1987) (Version Basic 1.0) [Original] [TAPE].cdt_1.txt", 0xBC2A, 0xBF00, "D", 60000, EXECUTE));
}

// 12 Jeux Exceptionnels (UK) (Face 2B) (1988) (12. Mask) [Original] [TAPE] [COMPILATION].cdt
TEST(Dumps_Tape_Other, Mask_12_jeux_exceptionnles_cdt)
{
   TestTape test;
   // First test : To the white screen, with '0' countdown
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini",
      ".\\res\\Tape\\12 Jeux Exceptionnels (UK) (Face 2B) (1988) (12. Mask) [Original] [TAPE] [COMPILATION].cdt",
      ".\\res\\Tape\\Record\\12 Jeux Exceptionnels (UK) (Face 2B) (1988) (12. Mask) [Original] [TAPE] [COMPILATION].cdt_1.txt", 0x40F, 0x47D, "A", 63000, EXECUTE));

   // Press space
   CommandKeyboard cmd_space(" ");
   CommandRunCycles run_cycles(100);
   CommandJoystick cmd1(0, 64);
   CommandJoystick cmd2(0, 0);

   run_cycles.Action(test.machine_);
   cmd_space.Action(test.machine_);
   run_cycles.Action(test.machine_);
   cmd_space.Action(test.machine_);
   run_cycles.Action(test.machine_);

   // Joystick button x2
   cmd1.Action(test.machine_);
   run_cycles.Action(test.machine_);
   cmd2.Action(test.machine_);
   run_cycles.Action(test.machine_);
   cmd1.Action(test.machine_);
   run_cycles.Action(test.machine_);
   cmd2.Action(test.machine_);

   // Another test, until game
   ASSERT_EQ(true, test.MoreTest(".\\res\\Tape\\Record\\12 Jeux Exceptionnels (UK) (Face 2B) (1988) (12. Mask) [Original] [TAPE] [COMPILATION].cdt_2.txt",
      0x465, 0x828F, "A", 20000, EXECUTE));
}

// Skate Crazy (UK) (Face A) (1988) [Original] [TAPE].cdt
TEST(Dumps_Tape_Other, Skate_Crazy_cdt)
{
   TestTape test;
   ASSERT_EQ(true, test.Test("464", ".\\TestConf.ini", 
                           ".\\res\\Tape\\SkateCrazy (UK) (Face A) (1988) (Part 1) [Original] TAPE].cdt", 
                           ".\\res\\Tape\\Record\\SkateCrazy (UK) (Face A) (1988) (Part 1) [Original] TAPE].cdt_1.txt", 0x0AE7, 0xC36, "A", 60000, EXECUTE)) ;
}

