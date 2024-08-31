#pragma once

//////////////////////////////////////////////////////////
// SettingsList.h
//
// This class handle a list of settings
//
// (c) T.Guillemin 2018
//
//
//////////////////////////////////////////////////////////

#include "FileAccess.h"
#include "MachineSettings.h"
#include "IConfiguration.h"

#include <string>
#include <vector>

class SettingsList
{
public:
   //////////////////////////////////////////////////////////
   // Constructor / Destructor
   SettingsList();
   virtual ~SettingsList();

   virtual void InitSettingsList(IConfiguration * configuration_manager, const char* base_conf_directory);

   //////////////////////////////////////////////////////////
   // Build settings
   virtual void BuildList();

   //////////////////////////////////////////////////////////
   // Iterate through settings
   unsigned int GetNumberOfConfigurations() const ;
   MachineSettings* GetConfiguration(unsigned int index);

protected:
   
   std::vector<MachineSettings*> settings_array_;
   IConfiguration * configuration_manager_;
   std::string base_conf_directory_;

};


