#pragma once

#include <string>
#include <filesystem>

class SugarboxInitialisation
{
public:
   // Element of configuration
   bool _debug_start;
   bool _no_show;
   std::string _hardware_configuration;
   std::filesystem::path _script_to_run;
   std::filesystem::path _cart_inserted;
   std::string _dbs_server;

};

