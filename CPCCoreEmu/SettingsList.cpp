#include "stdafx.h"
#include "SettingsList.h"

SettingsList::SettingsList() : configuration_manager_(nullptr), base_conf_directory_(nullptr)
{
   
}

SettingsList::~SettingsList()
{
   for (auto& it : settings_array_)
   {
      delete it;
   }
}

void SettingsList::InitSettingsList(IConfiguration * configuration_manager, const char* base_conf_directory)
{
   base_conf_directory_ = base_conf_directory;
   configuration_manager_ = configuration_manager;
}


void SettingsList::BuildList()
{
   // Build list :
   // Read configurations from directory
   std::vector<std::string> file_list;
   GetDirectoryContent(base_conf_directory_, file_list);

   // For each file : Try to load it.
   for (auto& it : file_list)
   {
      MachineSettings *settings = MachineSettings::CreateSettings(configuration_manager_, it.c_str());
      if ( settings != nullptr )
      {
         settings_array_.push_back(settings);
      }
   }

   // If no configurations are available : Create a default one
   if (settings_array_.empty())
   {
      // Create a dummy conf
      auto *settings = new MachineSettings;

      // Init : TODO

      // Add it to list
      settings_array_.push_back(settings);

   }
}

unsigned int SettingsList::GetNumberOfConfigurations() const
{
   return settings_array_.size();
}

MachineSettings* SettingsList::GetConfiguration(unsigned int index)
{
   return settings_array_.at(index);
}