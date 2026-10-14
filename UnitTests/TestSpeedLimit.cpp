#include "gtest/gtest.h"

#include "TestUtils.h"

#include <filesystem>
#include <fstream>
#include <string>

// Speed limit read from Sugarbox.ini.
//
// "LimitSpeed=C" (custom) runs at the "Speed" percentage. That key was never read
// and the engine started with a speed of 0: HandleSyncro then waited for all the
// emulated time, longer at each slice, so the emulation crawled and a command
// sent to it waited up to 2 seconds. A missing or empty file gives the custom
// limit too.

namespace
{

class SpeedLimitTest : public ::testing::Test
{
protected:
   EmulatorEngine* Load(const char* ini_content)
   {
      ini_path_ = (std::filesystem::temp_directory_path() / "sugarbox_speed_test.ini").string();
      std::ofstream(ini_path_) << ini_content;

      machine_ = new EmulatorEngine();
      display_.Init(false);
      display_.Show(false);
      machine_->SetDirectories(&dirImp_);
      machine_->SetLog(&log_);
      machine_->SetConfigurationManager(&conf_manager_);
      machine_->Init(&display_, &soundFactory_);
      machine_->LoadConfiguration("Current", ini_path_.c_str());
      return machine_;
   }

   void TearDown() override
   {
      delete machine_;
      std::error_code error;
      std::filesystem::remove(ini_path_, error);
   }

   DirectoriesImp dirImp_; CDisplay display_; Log log_;
   SoundFactory soundFactory_; ConfigurationManager conf_manager_;
   EmulatorEngine* machine_ = nullptr;
   std::string ini_path_;
};

const char* kMachine = "Machine_Settings=CONF/CPC6128FR.cfg\n";

}  // namespace

TEST_F(SpeedLimitTest, CustomSpeedIsRead)
{
   EmulatorEngine* machine = Load((std::string("[Current]\nLimitSpeed=C\nSpeed=50\n") + kMachine).c_str());
   EXPECT_EQ(EmulatorEngine::E_CUSTOM, machine->GetSpeedLimit());
   EXPECT_EQ(50u, machine->GetSpeed());
}

TEST_F(SpeedLimitTest, NoSpeedLimitGivesARealTimeCustomSpeed)
{
   EmulatorEngine* machine = Load((std::string("[Current]\n") + kMachine).c_str());
   EXPECT_EQ(EmulatorEngine::E_CUSTOM, machine->GetSpeedLimit());
   EXPECT_EQ(100u, machine->GetSpeed());
}

TEST_F(SpeedLimitTest, OtherLimitsRunAtRealTime)
{
   EmulatorEngine* machine = Load((std::string("[Current]\nLimitSpeed=S\nSpeed=50\n") + kMachine).c_str());
   EXPECT_EQ(EmulatorEngine::E_SOUND, machine->GetSpeedLimit());
   EXPECT_EQ(100u, machine->GetSpeed());
}

TEST_F(SpeedLimitTest, NullSpeedIsRefused)
{
   EmulatorEngine* machine = Load((std::string("[Current]\nLimitSpeed=C\nSpeed=0\n") + kMachine).c_str());
   EXPECT_EQ(100u, machine->GetSpeed());
   machine->SetSpeed(-5);
   EXPECT_EQ(100u, machine->GetSpeed());
}
