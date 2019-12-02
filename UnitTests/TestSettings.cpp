
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>

#include "gtest/gtest.h"
#include "TestUtils.h"

#include "SettingsList.h"

/////////////////////////////////////////////////////////////
/// Tests 
TEST(Settings, basic_settinglist_test)
{
   // Load settings
   SettingsList settings_list;
   ConfigurationManager conf_manager;
   
   settings_list.InitSettingsList (&conf_manager, ".\\CONF");
   settings_list.BuildList();

   // Read and check settings list
   ASSERT_EQ(35, settings_list.GetNumberOfConfigurations()) << "Wrong configuration number !";
   ASSERT_NE(nullptr, settings_list.GetConfiguration(0)) << "Configuration 0 shouldn't be nullptr !";
}

TEST(Settings, defaultc_settinglist_test)
{
   // Load settings
   SettingsList settings_list;
   ConfigurationManager conf_manager;
   settings_list.InitSettingsList(&conf_manager, ".\\CONF_NOT_EXISTING");
   settings_list.BuildList();

   // Read and check settings list
   ASSERT_EQ(1, settings_list.GetNumberOfConfigurations()) << "Wrong configuration number !";
   ASSERT_NE(nullptr, settings_list.GetConfiguration(0)) << "Configuration 0 shouldn't be nullptr !";
}
