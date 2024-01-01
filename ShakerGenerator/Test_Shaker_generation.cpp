
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>
#include <fstream>

#include "DiskContainer.h"

#include "TestUtils.h"

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
   // Handle generic values
   if (argument_list.find("-help") != argument_list.end())
   {
      // Display help
      std::cout << "A Sugarbox text to generate Shaker tests." << std::endl;
      std::cout << "Usage : ShakerGenerator [-help] [-show] [-out][-crtc crtc_num] [-module module_name]" << std::endl;
      std::cout << "    -help   : display this message" << std::endl;
      std::cout << "    -crtc   : list the CRTC type to generate, from 0 to 4. It can be a list (ShakerGenerator -crtc 0 2 4)" << std::endl;
      std::cout << "    -module : module name to filter : can be A, B, C, D. Can also be a list." << std::endl;
      std::cout << "    -show   : show the executed test in a window while generated" << std::endl;
      std::cout << "  Do not use -module with -test : Only the wanted test will be executed !" << std::endl;
   }

   bool show = argument_list.find("-show") != argument_list.end();

   std::filesystem::path script_path = "C:/Thierry/Amstrad/Dev/Shakerland/Shaker_CSL/CSL/MODULE_A/SHAKE25A-0.CSL";

   std::filesystem::path ini_file = "./TestConf.ini";

   TestDump test_dump(show);
   test_dump.Test("6128", ini_file, script_path);
}
