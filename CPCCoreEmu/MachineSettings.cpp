#include "stdafx.h"
#include "MachineSettings.h"
#include <sstream>

MachineSettings::MachineSettings() :configuration_manager_(nullptr), loaded_(false)
{
   // Default configuration:
   // The default conf is "6128 PLUS FR"
   hardware_type_ = PLUS_6128;
   ram_cfg_ = M128_K;
   cartridge_path_ = "CART\\plus_en.cpr";
   fdc_present_ = true;
   pal_present_ = true;
   type_crtc_ = CRTC::AMS40226;
}

MachineSettings::~MachineSettings()
{
   
}

bool MachineSettings::operator== (MachineSettings& other) const 
{
   // From filepath : compare filename
   fs::path p1 = file_path_;
   p1 = p1.filename();
   fs::path p2 = other.file_path_;
   p2 = p2.filename();

   return (p1 == p2);
   //return (strcmp(other.GetFilePath(), GetFilePath()) == 0);
}

MachineSettings *MachineSettings::CreateSettings(IConfiguration * configuration_manager, const char* path)
{
   MachineSettings *settings = new MachineSettings;
   settings->Init(configuration_manager);
   if (!settings->SetFile(path))
   {
      delete settings;
      settings = nullptr;
   }
   return settings;
}

void MachineSettings::Init(IConfiguration * configuration_manager)
{
   configuration_manager_ = configuration_manager;
}

bool MachineSettings::SetFile(const char* path)
{
   file_path_ = path;

   // Do nothing if not initialised
   if (configuration_manager_ == nullptr) return false;

   // File is correct if a CRTC conf exist
   if (configuration_manager_->GetConfigurationInt("Hardware", "Type_CRTC", -1, path) == -1)
      return false;

   // A path is set : Try to load standard elements :
   char tmp_buffer[128];
   // Desc
   configuration_manager_->GetConfiguration("Generic", "Desc", "", tmp_buffer, sizeof(tmp_buffer), path);
   description_ = tmp_buffer;

   // ShortDesc
   configuration_manager_->GetConfiguration("Generic", "ShortDesc", "", tmp_buffer, sizeof(tmp_buffer), path);
   short_description_ = tmp_buffer;

   // If none of those are present, assume that the path is not valid
   return true;// !short_description_.empty() || !description_.empty();
}

bool MachineSettings::Load(bool force_reload)
{
   if (loaded_ && !force_reload) return true;

   if (file_path_.empty() || configuration_manager_ == nullptr) return false;

   char buffer[1024];
   configuration_manager_->GetConfiguration("Generic", "Desc", "", buffer, sizeof buffer, file_path_.c_str());
   description_ = buffer;
   configuration_manager_->GetConfiguration("Generic", "ShortDesc", "", buffer, sizeof buffer, file_path_.c_str());
   short_description_ = buffer;

   // Hardware type
   hardware_type_ = static_cast<HardwareType>(configuration_manager_->GetConfigurationInt("Hardware", "Type", OLD_6128, file_path_.c_str()));

   // Hardware
   type_crtc_ = static_cast<CRTC::TypeCRTC>(configuration_manager_->GetConfigurationInt("Hardware", "Type_CRTC", 0, file_path_.c_str()));
   pal_present_ = (configuration_manager_->GetConfigurationInt("Hardware", "PAL", 1, file_path_.c_str()) == 1);
   fdc_present_ = (configuration_manager_->GetConfigurationInt("Hardware", "FDC", 1, file_path_.c_str()) == 1);

   ram_cfg_ = static_cast<RamCfg>(configuration_manager_->GetConfigurationInt("Memory", "RAM", M128_K, file_path_.c_str()));

   // Default Cartridge
   configuration_manager_->GetConfiguration("Memory", "DefaultCartridge", "", buffer, MAX_PATH, file_path_.c_str());
   cartridge_path_ = buffer;

   // - Lower ROM
   configuration_manager_->GetConfiguration("Memory", "LowerROM", "", buffer, MAX_PATH, file_path_.c_str());
   lower_rom_ = buffer;

   // - Upper ROMs
   upper_rom_.clear();
   char key[16];
   for (int i = 0; i < 256; i++)
   {
      sprintf(key, "%s_%i", "UpperROM", i);
      configuration_manager_->GetConfiguration("Memory", key, "", buffer, MAX_PATH, file_path_.c_str());
      upper_rom_[i] = buffer;
   }

   // Keyboard  todo
   configuration_manager_->GetConfiguration("Keyboard", "Type", "", buffer, 1024, file_path_.c_str());
   keyboard_config_ = buffer;

   return true;
}

bool MachineSettings::Save(const char* new_name)
{
   if (new_name != nullptr) file_path_ = new_name;
   if (file_path_.empty() || configuration_manager_ == nullptr) return false;

   configuration_manager_->SetConfiguration("Generic", "Desc", description_.c_str(), file_path_.c_str());
   configuration_manager_->SetConfiguration("Generic", "ShortDesc", short_description_.c_str(), file_path_.c_str());


   // Hardware
   std::ostringstream hard_type_str;

   hard_type_str << hardware_type_;
   configuration_manager_->SetConfiguration("Hardware", "Type", hard_type_str.str().c_str(), file_path_.c_str());
   
   std::ostringstream crtc_type_str;
   crtc_type_str << type_crtc_;
   configuration_manager_->SetConfiguration("Hardware", "Type_CRTC", crtc_type_str.str().c_str(), file_path_.c_str());
   configuration_manager_->SetConfiguration("Hardware", "PAL", pal_present_ ? "1" : "0", file_path_.c_str());
   configuration_manager_->SetConfiguration("Hardware", "FDC", fdc_present_ ? "1" : "0", file_path_.c_str());

   // Keyboard
   configuration_manager_->SetConfiguration("Keyboard", "Type", keyboard_config_.c_str(), file_path_.c_str());

   return true;
}

MachineSettings::RamCfg MachineSettings::GetRamCfg() const
{
   return ram_cfg_;
}
