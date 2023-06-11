
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>

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

   enum SpecialAction
   {
      None,
      Wait_a_sec,
      Hit_Space,
      Wait_more_than_a_sec
   };

   ShakerTest(std::string name, std::string test_prime, unsigned char test_second, unsigned int nb_subtest, unsigned char mask = 0x1F, SpecialAction wait = None, unsigned char more_wait = 1) :
      name_(name),
      test_prime_(test_prime),
      test_second_(test_second),
      nb_subtest_(nb_subtest),
      mask_(mask),
      wait_(wait),
      more_wait_(more_wait)
   {

   }

   std::string name_;
   std::string test_prime_;
   unsigned char test_second_;
   unsigned int nb_subtest_;

   unsigned char mask_;
   unsigned int wait_;
   unsigned int more_wait_;
};

std::vector<ShakerTest*> shaker_full_test =
{
   // A test
   new ShakerTest("A1", "a", 2, 5, 0x1F),
   new ShakerTest("A2", "a", 3, 4, 0x1F),
   new ShakerTest("A3", "a", 4, 22, 0x1F),
   new ShakerTest("A4", "a", 5, 1, 0x1F),
   new ShakerTest("A5", "a", 6, 5, 0x1F),
   new ShakerTest("A6", "a", 7, 5, 0x1F),
   new ShakerTest("A7", "a", 8, 5, 0x1F),
   new ShakerTest("A8", "a", 9, 1, 0x1F),
   new ShakerTest("A9", "a", 10, 3, 0x1F),
   new ShakerTest("ACOPY", "a", 56, 2, 0x4),
   new ShakerTest("AE", "a", 18, 1, 0x1F),
   new ShakerTest("AI", "a", 23, 5, 0x1F),
   new ShakerTest("AO", "a", 24, 9, 0x1F),
   new ShakerTest("AP", "a", 25, 13, 0x1F),
   new ShakerTest("AR", "a", 19, 6, 0x1F),
   new ShakerTest("AT", "a", 20, 6, 0x1F),
   new ShakerTest("AU", "a", 22, 1, 0x1F),
   new ShakerTest("AY", "a", 21, 8, 0x1F),

   // B test
   new ShakerTest("B0", "b", 11, 4, 0x1F),
   new ShakerTest("B1", "b", 2, 11, 0x1F, ShakerTest::Hit_Space),      // Base 06 todo (but how ?)
   new ShakerTest("B2", "b", 3, 3, 4),
   new ShakerTest("B3", "b", 4, 1),
   new ShakerTest("B4", "b", 5, 8, 0x1F, ShakerTest::Wait_a_sec),
   new ShakerTest("B5", "b", 6, 2),
   new ShakerTest("B6", "b", 7, 5),
   new ShakerTest("B7", "b", 8, 15, 1, ShakerTest::Wait_a_sec),
   new ShakerTest("B9", "b", 10, 4),
   new ShakerTest("BCAPS", "b", 58, 2, 0x1F, ShakerTest::Wait_more_than_a_sec, 4),
   new ShakerTest("BCOPY", "b", 56, 5, 0x1F, ShakerTest::Wait_a_sec),
   new ShakerTest("BCTRL", "b", 29, 3),
   new ShakerTest("BE", "b", 18, 1, 0x18),   // CRTC 3/4
   new ShakerTest("BF0", "b", 82, 1),
   new ShakerTest("BI", "b", 23, 8),
   new ShakerTest("BO", "b", 24, 2, 0x2),   // 1A / 1B : todo => use different way of differenciation !
   new ShakerTest("BP", "b", 25, 4, 0x1F, ShakerTest::Wait_a_sec),
   new ShakerTest("BR", "b", 19, 6),
   new ShakerTest("BRETURN", "b", 28, 1, 0x1F, ShakerTest::Wait_more_than_a_sec, 19),
   new ShakerTest("BS", "b", 31, 1, 2), 
   new ShakerTest("BU", "b", 22, 1, 19),

   // C
   // D


};


void main (void)
{
   for (int crtc = 0; crtc <= 4; crtc++)
   {
      // Create output dir
      std::filesystem::path outdir = "./Shaker_020202/CRTC" + extend_crtc[crtc];
      std::filesystem::create_directory("./Shaker_020202");
      std::filesystem::create_directory(outdir);

      std::filesystem::path ini_file = "./TestConf" + extend_crtc[crtc] + ".ini";

      for (auto it : shaker_full_test)
      {
         if (it->mask_ & (1 << crtc))
         {
            TestDump test_dump;
            CommandList cmd_list;

            // Select module A
            cmd_list.AddCommand(new CommandRunCycles(500));
            cmd_list.AddCommand(new CommandKeyboard(it->test_prime_.c_str()));
            cmd_list.AddCommand(new CommandRunCycles(500));
            cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), it->test_second_, 1)); ;
            cmd_list.AddCommand(new CommandRunCycles(50));
            cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), it->test_second_, 0)); ;
            cmd_list.AddCommand(new CommandRunCycles(450));

            for (int i = 0; i < it->nb_subtest_; i++)
            {
               char subtest_ext[2] = "A";
               subtest_ext[0] = 'A' + i;
               std::string name = "Shaker24_" + it->name_ + "_" + subtest_ext + ".bmp";
               std::filesystem::path outname = outdir / name;
               cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, outname, SCR_CREATE));

               if(it->wait_ != ShakerTest::None)
               {
                  switch (it->wait_)
                  {
                     case ShakerTest::Hit_Space:
                        cmd_list.AddCommand(new CommandKeyboard(" "));
                        cmd_list.AddCommand(new CommandRunCycles(500));
                        name = "Shaker24_" + it->name_ + "_" + subtest_ext + "_" + std::to_string(i + 1) + ".bmp";
                        outname = outdir / name;
                        cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, outname, SCR_CREATE));
                        break;
                     case ShakerTest::Wait_a_sec:
                        cmd_list.AddCommand(new CommandRunCycles(1000));
                        name = "Shaker24_" + it->name_ + "_" + subtest_ext + "_" + std::to_string(i + 1) + ".bmp";
                        outname = outdir / name;
                        cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, outname, SCR_CREATE));
                        break;
                     case ShakerTest::Wait_more_than_a_sec:
                        for (int j = 0; j < it->more_wait_; j++)
                        {
                           cmd_list.AddCommand(new CommandRunCycles(1000));
                           name = "Shaker24_" + it->name_ + "_" + subtest_ext + "_" + std::to_string(i + 1) " _" + std::to_string(j + 1) + ".bmp";
                           outname = outdir / name;
                           cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, outname, SCR_CREATE));
                        }
                        break;
                  }

               }
               // Go to next screen
               cmd_list.AddCommand(new CommandKeyboard(" "));
               cmd_list.AddCommand(new CommandRunCycles(500));

            }
            test_dump.Test("6128", ini_file, "./res/Shaker/shaker24.dsk", "run \"shaker24.bas\"\r", &cmd_list);
         }
      }
   }
}

