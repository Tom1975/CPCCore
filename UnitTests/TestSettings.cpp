
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>

#include "gtest/gtest.h"

#include "TestWorkspace.h"
#include "TestUtils.h"

#include "SettingsList.h"

#include <map>
#include <string>

// Configuration kept in memory : what Save writes, and whether it was written to the file
class MemoryConfiguration : public IConfiguration
{
public:
   void OpenFile(const char* config_file) override { current_ = config_file; }
   void CloseFile() override { written_[current_] = values_[current_]; }

   void SetConfiguration(const char* section, const char* key, const char* value, const char* file) override
   {
      OpenFile(file);
      SetConfiguration(section, key, value);
   }
   void SetConfiguration(const char* section, const char* key, const char* value) override
   {
      values_[current_][std::string(section) + "/" + key] = value;
   }

   unsigned int GetConfiguration(const char* section, const char* key, const char* default_value, char* out_buffer, unsigned int buffer_size, const char* file) override
   {
      OpenFile(file);
      return GetConfiguration(section, key, default_value, out_buffer, buffer_size);
   }
   unsigned int GetConfiguration(const char* section, const char* key, const char* default_value, char* out_buffer, unsigned int buffer_size) override
   {
      // Read from what was written to the file only
      auto& file = written_[current_];
      auto it = file.find(std::string(section) + "/" + key);
      const std::string value = (it == file.end()) ? default_value : it->second;
      snprintf(out_buffer, buffer_size, "%s", value.c_str());
      return static_cast<unsigned int>(strlen(out_buffer));
   }

   unsigned int GetConfigurationInt(const char* section, const char* key, unsigned int default_value, const char* file) override
   {
      OpenFile(file);
      return GetConfigurationInt(section, key, default_value);
   }
   unsigned int GetConfigurationInt(const char* section, const char* key, unsigned int default_value) override
   {
      auto& file = written_[current_];
      auto it = file.find(std::string(section) + "/" + key);
      return (it == file.end()) ? default_value : static_cast<unsigned int>(std::stoul(it->second));
   }

   const char* GetFirstSection() override { return nullptr; }
   const char* GetNextSection() override { return nullptr; }
   const char* GetFirstKey(const char* section) override { return nullptr; }
   const char* GetNextKey() override { return nullptr; }

private:
   std::string current_;
   std::map<std::string, std::map<std::string, std::string>> values_;
   std::map<std::string, std::map<std::string, std::string>> written_;
};

/////////////////////////////////////////////////////////////
/// Tests 
TEST(Settings, basic_settinglist_test)
{
   // Load settings
   SettingsList settings_list;
   ConfigurationManager conf_manager;
   
   settings_list.InitSettingsList (&conf_manager, TestWorkspace::Fixture("./CONF").c_str());
   settings_list.BuildList();

   // Read and check settings list
   ASSERT_EQ(36, settings_list.GetNumberOfConfigurations()) << "Wrong configuration number !";
   ASSERT_NE(nullptr, settings_list.GetConfiguration(0)) << "Configuration 0 shouldn't be nullptr !";
}

TEST(Settings, defaultc_settinglist_test)
{
   // Load settings
   SettingsList settings_list;
   ConfigurationManager conf_manager;
   settings_list.InitSettingsList(&conf_manager, "./CONF_NOT_EXISTING");
   settings_list.BuildList();

   // Read and check settings list
   ASSERT_EQ(1, settings_list.GetNumberOfConfigurations()) << "Wrong configuration number !";
   ASSERT_NE(nullptr, settings_list.GetConfiguration(0)) << "Configuration 0 shouldn't be nullptr !";
}

// Everything a configuration holds is written to its file, and read back
TEST(Settings, save_then_load_gives_the_same_configuration)
{
   MemoryConfiguration configuration;

   MachineSettings saved;
   saved.Init(&configuration);
   saved.SetFile("CONF/Test.cfg");
   saved.SetDescription("Description");
   saved.SetShortDescription("Test machine");
   saved.SetHardwareType(MachineSettings::OLD_464);
   saved.SetCRTCType(CRTC::MC6845);
   saved.SetRamCfg(MachineSettings::M512_K);
   saved.SetFDCPlugged(false);
   saved.SetTapePlugged(true);
   saved.SetPALPlugged(false);
   saved.SetLowerRom("Lower.rom");
   saved.SetUpperRom(0, "Basic.rom");
   saved.SetUpperRom(7, "Amsdos.rom");
   saved.SetUpperRom(255, "Last.rom");
   saved.SetDefaultCartridge("CART/test.cpr");
   saved.SetKeyboardConfig("SPANISH");
   ASSERT_TRUE(saved.Save());

   MachineSettings loaded;
   loaded.Init(&configuration);
   ASSERT_TRUE(loaded.SetFile("CONF/Test.cfg"));
   ASSERT_TRUE(loaded.Load(true));

   EXPECT_STREQ("Description", loaded.GetDescription());
   EXPECT_STREQ("Test machine", loaded.GetShortDescription());
   EXPECT_EQ(MachineSettings::OLD_464, loaded.GetHardwareType());
   EXPECT_EQ(CRTC::MC6845, loaded.GetCRTCType());
   EXPECT_EQ(MachineSettings::M512_K, loaded.GetRamCfg());
   EXPECT_FALSE(loaded.FDCPlugged());
   EXPECT_TRUE(loaded.TapePlugged());
   EXPECT_FALSE(loaded.PALPlugged());
   EXPECT_STREQ("Lower.rom", loaded.GetLowerRom());
   EXPECT_STREQ("Basic.rom", loaded.GetUpperRom(0));
   EXPECT_STREQ("Amsdos.rom", loaded.GetUpperRom(7));
   EXPECT_STREQ("", loaded.GetUpperRom(1));
   EXPECT_STREQ("Last.rom", loaded.GetUpperRom(255));
   EXPECT_STREQ("CART/test.cpr", loaded.GetDefaultCartridge());
   EXPECT_STREQ("SPANISH", loaded.GetKeyboardConfig());
}
