
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>
#include <fstream>

#include "DiskContainer.h"

#include "TestUtils.h"

#define SHAKER_VERSION "2.4"

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

// Complete shaker list
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
   new ShakerTest("AR", "a", 19, 3, 0x1F, ShakerTest::Hit_Space),
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
   new ShakerTest("C2", "c", 3, 9, 2, ShakerTest::Wait_a_sec),
   new ShakerTest("C3", "c", 4, 1, 2),
   new ShakerTest("C4", "c", 5, 30, 2),
   new ShakerTest("C5", "c", 6, 1, 5),
   new ShakerTest("C6", "c", 7, 16, 4),
   new ShakerTest("C7", "c", 8, 1, 4),
   new ShakerTest("C8", "c", 9, 7, 4),
   new ShakerTest("C9", "c", 10, 2, 4),
   new ShakerTest("CE", "c", 18, 1),
   new ShakerTest("CP", "c", 25, 1),
   new ShakerTest("CR", "c", 19, 1),
   new ShakerTest("CRETURN", "c", 28, 1),

   // D
   // todo ? 


};

std::map<std::string, std::function<ICommand*(std::vector<std::string>&)>> function_map_ = {
    { "csl_version" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "reset" , [](std::vector<std::string>& args)->ICommand* { return new CommandReset(); }},
    { "crtc_select" , [](std::vector<std::string>& args)->ICommand* { return new CommandSelectCRTC(args[1]); }},
    { "disk_insert" , [](std::vector<std::string>& args)->ICommand* { return new CommandInsertDisk(args[1].c_str()); }},
    { "disk_dir" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "tape_insert" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "tape_dir" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "tape_play" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "tape_stop" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "tape_rewind" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "snapshot_load" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "snapshot_dir" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "snapshot_name" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "key_delay" , [](std::vector<std::string>& args)->ICommand* { return new CommandKeyDelay(args); }},
    { "key_output" , [](std::vector<std::string>& args)->ICommand* { return new CommandKeyOutput(args[1].c_str()); }},
    { "key_from_file" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "wait" , [](std::vector<std::string>& args)->ICommand* { return new CommandWait(strtol(args[1].c_str(), NULL, 10) * 4); }},
    { "wait_driveonoff" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "wait_vsyncoffon" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "screenshot_name" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "screenshot_dir" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "screenshot" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "snapshot" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "csl_load" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }}
};

ICommand* GetCommand(std::vector<std::string>& args)
{
    ICommand* current_command = nullptr;
    if (function_map_.find(args[0]) != function_map_.end())
    {
        current_command = function_map_[args[0]](args);
    }
    return current_command;
}

void LoadScript(std::filesystem::path& script_path, CommandList* cmd_list)
{
    std::ifstream f(script_path);
    std::string line;

    while (std::getline(f, line))
    {
        // Handle line
        std::string::size_type begin = line.find_first_not_of(" \f\t\v");

        // Skip blank lines
        if (begin == std::string::npos) continue;
        // Skip commentary
        std::string::size_type end = line.find_first_of(";");
        if (end != std::string::npos)
            line = line.substr(begin, end);

        if (line.empty()) continue;

        // Get command
        std::vector<std::string> command_parameters;

        std::string current_parameter;
        char current_delim = ' ';
        for (auto& c : line)
        {
            if (c == ';')
            {
                break;
            }
            if (c == current_delim)
            {
                // New 
                if (current_parameter.size() > 0)
                    command_parameters.push_back(current_parameter);
                current_parameter.clear();
                current_delim = ' ';
            }
            else if (c == '\'' && current_parameter.size() == 0)
            {
                current_delim = '\'';
            }
            else if (c == '\"' && current_parameter.size() == 0)
            {
                current_delim = '\"';
            }
            else
            {
                current_parameter += c;
            }
        }
        // Comment : no more parameter to handle
        if (current_parameter.size() > 0)
            command_parameters.push_back(current_parameter);

        // Look for command
        ICommand* command = GetCommand(command_parameters);

        if (command != nullptr)
        {
            cmd_list->AddCommand(command);
        }
        else
        {
            std::cout << "unknow command : " << command_parameters[0] << std::endl;
        }
        //IScriptCommand* command = ScriptCommandFactory::GetCommand(command_parameters[0]);
        //if (command != nullptr)
        //{
        //    script_.push(Script(command, command_parameters));
        //}
    }
}

int main(int argc, char* argv[])
{
   // Extract arguments
   int index = 1;
   std::map<std::string, std::vector<std::string>> argument_list;
   char* current_arg = nullptr;
   while (index < argc)
   {
      // Extract arg 'index'
      if ( argv[index][0] == '-')   // New argument
      {
         current_arg = argv[index];
         argument_list[current_arg];
         
      }
      else
      {
         if (current_arg != nullptr)
         {
            argument_list[current_arg].push_back(argv[index]);
         }
      }

      ++index;
   }

   std::cout << "ShakerGenerator" << std::endl << std::endl;
   std::cout << "Version of shaker : " << SHAKER_VERSION << std::endl;
   // Handle generic values
   if (argument_list.find("-help") != argument_list.end())
   {
      // Display help
      std::cout << "A Sugarbox text to generate Shaker tests." << std::endl;
      std::cout << "Usage : ShakerGenerator [-help] [-show] [-out][-crtc crtc_num] [-module module_name] [-test test_name]" << std::endl;
      std::cout << "    -help   : display this message" << std::endl;
      std::cout << "    -crtc   : list the CRTC type to generate, from 0 to 4. It can be a list (ShakerGenerator -crtc 0 2 4)" << std::endl;
      std::cout << "    -module : module name to filter : can be A, B, C, D. Can also be a list." << std::endl;
      std::cout << "    -test   : tests to execute, by its name. Ex : -test A7 A8 BRETURN CR" << std::endl;
      std::cout << "    -show   : show the executed test in a window while generated" << std::endl;
      std::cout << "  Do not use -module with -test : Only the wanted test will be executed !" << std::endl;
   }

   bool show = argument_list.find("-show") != argument_list.end();

   std::filesystem::path path = "C:/Thierry/Amstrad/Dev/git/SugarboxV2/out/build/x64-Debug/install/Shaker/Shaker_CSL/CSL/MODULE_A/SHAKE25A-0.CSL";

   CommandList cmd_list;
   LoadScript(path, &cmd_list);

   std::filesystem::path ini_file = "./TestConf_0.ini";

   TestDump test_dump(show);
   test_dump.Test("6128", ini_file, &cmd_list);

   /*
   for (int crtc = 0; crtc <= 4; crtc++)
   {
      // Check filter
      bool to_run = false;
      if (argument_list["-crtc"].size() > 0 )
      {
         for (auto it_crtc: argument_list["-crtc"])
         {
            char* endstr;
            int crtc_num = strtol(it_crtc.c_str(), &endstr, 10);
            if ( endstr != it_crtc.c_str() && crtc_num == crtc)
            {
               to_run = true;
               break;
            }
         }
      }
      else
      {
         to_run = true;
      }

      // Create output dir
      std::filesystem::path base_dir = (argument_list["-out"].size() > 0) ? argument_list["-out"][0] : ".";
      std::filesystem::path outdir = base_dir / "Shaker_" SHAKER_VERSION;
      std::filesystem::create_directory(outdir);
      outdir /= "CRTC" + extend_crtc[crtc];
      std::filesystem::create_directory(outdir);

      std::filesystem::path ini_file = "./TestConf" + extend_crtc[crtc] + ".ini";

      std::cout << "Running CRTC" << crtc << std::endl;

      // Filter on CRTC type
      if (to_run)
      {
         for (auto it : shaker_full_test)
         {
            if (it->mask_ & (1 << crtc))
            {
               TestDump test_dump (show);
               CommandList cmd_list;

               // Select module
               bool to_run_test = false;

               if (argument_list["-test"].size() > 0)
               {
                  for (auto it_module : argument_list["-test"])
                  {
                     if (it_module == it->name_)
                     {
                        to_run_test = true;
                        break;
                     }
                  }
               }
               else if (argument_list["-module"].size() > 0)
               {
                  for (auto it_module : argument_list["-module"])
                  {

                     if (stricmp(it_module.c_str(), it->test_prime_.c_str()) == 0)
                     {
                        to_run_test = true;
                        break;
                     }
                  }
               }
               else
               {
                  to_run_test = true;
               }

               if (to_run_test)
               {
                  std::cout << "Running test " << it->name_ << std::endl;

                  cmd_list.AddCommand(new CommandRunCycles(500));
                  cmd_list.AddCommand(new CommandKeyboard(it->test_prime_.c_str()));


                  cmd_list.AddCommand(new CommandRunCycles(500));
                  cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), it->test_second_, 1)); ;
                  cmd_list.AddCommand(new CommandRunCycles(50));
                  cmd_list.AddCommand(new CommandScanCode(test_dump.machine_->GetKeyboardHandler(), it->test_second_, 0)); ;
                  cmd_list.AddCommand(new CommandRunCycles(450));

                  for (int i = 0; i < it->nb_subtest_; i++)
                  {
                     char subtest_ext[3] = "A";
                     if (i < 26)
                     {
                        subtest_ext[0] = 'A' + i;
                     }
                     else
                     {
                        subtest_ext[0] = 'A' + i / 26;
                        subtest_ext[1] = 'A' + i % 26;
                        subtest_ext[2] = 0;
                     }

                     std::string name = "Shaker24_" + it->name_ + "_" + subtest_ext + ".jpg";
                     std::filesystem::path outname = outdir / name;
                     cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, outname, SCR_CREATE));

                     if (it->wait_ != ShakerTest::None)
                     {
                        switch (it->wait_)
                        {
                        case ShakerTest::Hit_Space:
                           cmd_list.AddCommand(new CommandKeyboard(" "));
                           cmd_list.AddCommand(new CommandRunCycles(500));
                           name = "Shaker24_" + it->name_ + "_" + subtest_ext + "_" + std::to_string(i + 1) + ".jpg";
                           outname = outdir / name;
                           cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, outname, SCR_CREATE));
                           break;
                        case ShakerTest::Wait_a_sec:
                           cmd_list.AddCommand(new CommandRunCycles(1000));
                           name = "Shaker24_" + it->name_ + "_" + subtest_ext + "_" + std::to_string(i + 1) + ".jpg";
                           outname = outdir / name;
                           cmd_list.AddCommand(new CommandSaveScreenshot(&test_dump.display, outname, SCR_CREATE));
                           break;
                        case ShakerTest::Wait_more_than_a_sec:
                           for (int j = 0; j < it->more_wait_; j++)
                           {
                              cmd_list.AddCommand(new CommandRunCycles(1000));
                              name = "Shaker24_" + it->name_ + "_" + subtest_ext + "_" + std::to_string(i + 1) + " _" + std::to_string(j + 1) + ".jpg";
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
                  test_dump.Test("6128", ini_file, "./Shaker/shaker24.dsk", "run \"shaker24.bas\"\r", &cmd_list);
               }
            }
         }
      }
   */
   }
