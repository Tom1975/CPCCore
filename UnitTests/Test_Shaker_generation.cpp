
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>

#include "gtest/gtest.h"
#include "DiskContainer.h"

#include "TestUtils.h"

// Extension to automate most of test :
// CRTC
std::string extend_crtc[5] =
{ "_0", "_1", "_2", "_3", "_4"
};

// Main tests
class ShakerTest
{
public:
   std::string name;
   std::string test_prime;
   std::string test_second;
   unsigned int nb_subtest;
};

std::vector<ShakerTest> shaker_full_test =
{
   {"A1", "a", "&", 5},
};


TEST(Shaker_generation_A1, Test_1)
{
   for (int crtc = 0; crtc <= 4; crtc++)
   {
      std::filesystem::path ini_file = "./TestConf" + extend_crtc[crtc] + ".ini";

      for (auto it : shaker_full_test)
      {
         TestDump test_dump;
         CommandList cmd_list;

         // Select module A
         cmd_list.AddCommand(new CommandRunCycles(500));
         cmd_list.AddCommand(new CommandKeyboard(it.test_prime.c_str()));
         cmd_list.AddCommand(new CommandRunCycles(500));
         cmd_list.AddCommand(new CommandKeyboard(it.test_second.c_str()));
         cmd_list.AddCommand(new CommandRunCycles(500));

         for (int i = 0; i < it.nb_subtest; i++)
         {

            char subtest_ext [2] = "A";
            subtest_ext[0] = 'A' + i;
            std::string name = "Shaker24_" + it.name + "_" + subtest_ext + ".bmp";
            std::filesystem::path outname = "./res/Record/Shaker/CRTC" + extend_crtc[crtc] + "/" + name;
            cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, outname, SCR_CREATE));

            // Go to next screen
            cmd_list.AddCommand(new CommandKeyboard(" "));
            cmd_list.AddCommand(new CommandRunCycles(500));
         }
         test_dump.Test("6128", ini_file, "./res/Shaker/shaker24.dsk", "run \"shaker24.bas\"\r", &cmd_list);

      }
   }
}

