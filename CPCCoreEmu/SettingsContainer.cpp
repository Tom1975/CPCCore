#include "stdafx.h"
#include "SettingsContainer.h"
#include "Machine.h"

#include <regex>


#define MAX_SIZE_BUFFER MAX_PATH

extern const char* ROMPath;
extern const char* CartPath;

//#define CONF_PATH _T("\\CONF\\")


SettingsContainer::SettingsContainer(const char* path) : play_city_(false), multiface_2_(false), hardware_type_ (OLD_6128), configuration_manager_(nullptr)
{
   sound_factory_ = NULL;
   machine_ = NULL;

   rom_path_ = path;
   base_conf_path_ = path;
   conf_path_ = path;

   rom_path_ /= ROMPath;

   // Default cartridge
   default_cartridge_ = path;
   default_cartridge_ /= CartPath;
   default_cartridge_ /= "AmstradPlus.f4.cpr";

   // Default initialisation : We assume French ROM, 128k machine
   for (auto t : upper_roms_list_)
   {
      t = "";
   }

   memset (upper_roms_list_, 0, sizeof(upper_roms_list_));

   // Standard 6128 - FR
   lower_rom_ = "Rom CPC 6128 (FR) (Rom A) (1985) (OS v3) [UTILITAIRE] [ROM].rom";

   upper_roms_list_[0] = "Rom CPC 6128 (FR) (Rom B) (1985) (Basic 1.1) [UTILITAIRE] [ROM].rom";
   upper_roms_list_[7] = "Cpc 664 and 6128 Amsdos (1985)(Amstrad)(AMSDOS.ROM).rom";

   // Default sound
   sound_ = 0;

   ram_cfg_ = M512_K;
   pal_present_ = true;
   fdc_present_ = true;
   type_crtc_ = CRTC::UM6845R;

   keyboard_config_ = "Default";
}


SettingsContainer::~SettingsContainer(void)
{
}

void SettingsContainer::Init (EmulatorEngine* machine, ISoundFactory* sound_factory, IConfiguration * configuration_manager)
{
   configuration_manager_ = configuration_manager;
   sound_factory_ = sound_factory;
   machine_ = machine;
   ComputeAvailableRoms();

   sound_ = sound_factory_->GetSound (NULL);
}

void SettingsContainer::SetDescription ( char* txt )
{
   description_ = txt;
}

void SettingsContainer::SetShortName ( char* txt )
{
   description_short_ = txt;
}

void SettingsContainer::UpdateComputer (bool no_cart_reload )
{
   fs::path path;

   machine_->GetMem()->InitMemory();

   // Update RAM
   machine_->GetMem ()->SetRam(ram_cfg_);

   // Update ROM
   path = rom_path_;
   path /= lower_rom_;

   machine_->GetMem ()->LoadLowerROM(path.string().c_str());
   machine_->GetPSG()->LoadKeyboardMap(keyboard_config_.c_str());

   for (int i = 0; i < 256; i++)
   {
      if (  upper_roms_list_[i].empty() )
      {
         machine_->GetMem ()->LoadROM (i, NULL);
      }
      else
      {
         path = rom_path_;
         path /= upper_roms_list_[i];
         machine_->GetMem ()->LoadROM (i, path.string().c_str());
      }
   }

   // Sound
   machine_->InitSound (GetSound ());

   // Hardware
   machine_->GetCRTC()->DefinirTypeCRTC (type_crtc_);
   machine_->SetPAL (pal_present_);
   machine_->SetFDCPlugged ( fdc_present_ );

   // External devices
   machine_->UpdateExternalDevices();


   // PLUS Machine ?
   if (  hardware_type_ == PLUS_6128
      || hardware_type_ == PLUS_464)
   {
      machine_->SetMachineType(hardware_type_);
      machine_->SetPlus(true);
      if (no_cart_reload == false)
      {
         machine_->LoadCpr(default_cartridge_.string().c_str());
      }
   }
   else
   {
      machine_->SetPlus(false);
   }


}

void SettingsContainer::SaveAs (char* path )
{
   conf_path_ = path;
   Save ();
}

void SettingsContainer::Save ()
{
   char tmp_buffer [MAX_SIZE_BUFFER ];
   if (configuration_manager_ != nullptr)
   {
      configuration_manager_->SetConfiguration("Generic", "Desc", description_.c_str(), conf_path_.string().c_str());
      configuration_manager_->SetConfiguration("Generic", "ShortDesc", description_short_.c_str(), conf_path_.string().c_str());

      // Hardware type
      sprintf (tmp_buffer, "%i", hardware_type_);
      configuration_manager_->SetConfiguration("Hardware", "Type", tmp_buffer, conf_path_.string().c_str());


      // Load configuration
      // RAM
      sprintf(tmp_buffer, "%i", ram_cfg_);
      configuration_manager_->SetConfiguration("Memory", "RAM", tmp_buffer, conf_path_.string().c_str());

      // default cartridge
      configuration_manager_->SetConfiguration("Memory", "DefaultCartridge",  default_cartridge_.string().c_str(),
                                                                              conf_path_.string().c_str());

      // - Lower ROM
      sprintf(tmp_buffer, "%s", "LowerROM");
      configuration_manager_->SetConfiguration("Memory", tmp_buffer, lower_rom_.string().c_str(), conf_path_.string().c_str());

      // - Upper ROMs
      for (int i = 0; i < 256; i++)
      {
         sprintf(tmp_buffer, "%s_%i", "UpperROM", i);
         configuration_manager_->SetConfiguration("Memory", tmp_buffer, upper_roms_list_[i].string().c_str(), conf_path_.string().c_str());
      }

      // Sound
      const char * name = sound_factory_->GetSoundName(sound_);
      configuration_manager_->SetConfiguration("Sound", "SoundType", name, conf_path_.string().c_str());
      sound_->SaveConfiguration("Sound", conf_path_.string().c_str());

      // Keyboard
      configuration_manager_->SetConfiguration("Keyboard", "Type", keyboard_config_.c_str(), conf_path_.string().c_str());

      // Hardware
      sprintf(tmp_buffer, "%i", type_crtc_);
      configuration_manager_->SetConfiguration("Hardware", "Type_CRTC", tmp_buffer, conf_path_.string().c_str());

      // PAL is present ?
      configuration_manager_->SetConfiguration("Hardware", "PAL", pal_present_ ? "1" : "0", conf_path_.string().c_str());

      configuration_manager_->SetConfiguration("Hardware", "FDC", fdc_present_ ? "1" : "0", conf_path_.string().c_str());

      // External Devices
      configuration_manager_->SetConfiguration("Devices", "Multiface2", multiface_2_ ? "1" : "0", conf_path_.string().c_str());
      configuration_manager_->SetConfiguration("Devices", "PlayCity", play_city_ ? "1" : "0", conf_path_.string().c_str());
   }
}

void SettingsContainer::Load (char* path)
{
   conf_path_ = base_conf_path_;
   conf_path_ /= path;

   char tmp_buffer [MAX_SIZE_BUFFER ];
   char path_buffer [MAX_PATH ];
   char description[1024];

   configuration_manager_->GetConfiguration ("Generic", "Desc", "", description, 1024 , conf_path_.string().c_str());
   description_ = description;
   configuration_manager_->GetConfiguration ("Generic", "ShortDesc", "", description, 128, conf_path_.string().c_str());
   description_short_ = description;

   // Hardware type
   hardware_type_ = (HardwareType)configuration_manager_->GetConfigurationInt("Hardware", "Type", OLD_6128, conf_path_.string().c_str());

   // Load configuration
   // RAM
   ram_cfg_ = (RAMCfg)configuration_manager_->GetConfigurationInt ("Memory", "RAM", M128_K, conf_path_.string().c_str());

   // Default Cartridge
   configuration_manager_->GetConfiguration("Memory", "DefaultCartridge", "", path_buffer, MAX_PATH, conf_path_.string().c_str());
   default_cartridge_ = path_buffer;

   // - Lower ROM
   sprintf ( tmp_buffer, "%s", "LowerROM");
   configuration_manager_->GetConfiguration ("Memory", tmp_buffer, "", path_buffer, MAX_PATH , conf_path_.string().c_str());
   lower_rom_ = path_buffer;

   // - Upper ROMs
   for (int i =0; i < 256; i++)
   {
      sprintf ( tmp_buffer, "%s_%i", "UpperROM", i);
      configuration_manager_->GetConfiguration ("Memory", tmp_buffer, "", path_buffer, MAX_PATH , conf_path_.string().c_str());
      upper_roms_list_[i] = path_buffer;
   }

   // Sound : What's the name of the sound ?
   configuration_manager_->GetConfiguration ("Sound", "SoundType", "", path_buffer, MAX_PATH , conf_path_.string().c_str());
   if ( sound_factory_!=NULL)
   {
      sound_ = sound_factory_->GetSound ( path_buffer );
      if (sound_ != NULL)
         sound_->LoadConfiguration ( "Sound", conf_path_.string().c_str());
   }

   // Keyboard 
   configuration_manager_->GetConfiguration ("Keyboard", "Type", "", tmp_buffer, sizeof(tmp_buffer), conf_path_.string().c_str());
   keyboard_config_ = tmp_buffer;

   // Hardware
   type_crtc_ = (CRTC::TypeCRTC)configuration_manager_->GetConfigurationInt ("Hardware", "Type_CRTC", 0, conf_path_.string().c_str());
   if (type_crtc_ >= CRTC::MAX_CRTC)
	   type_crtc_ = CRTC::HD6845S;


   pal_present_ = (configuration_manager_->GetConfigurationInt ("Hardware", "PAL", 1, conf_path_.string().c_str()) == 1);

   fdc_present_ = (configuration_manager_->GetConfigurationInt ("Hardware", "FDC", 1, conf_path_.string().c_str()) == 1);

   multiface_2_ = (configuration_manager_->GetConfigurationInt("Devices", "Multiface2", 0, conf_path_.string().c_str()) == 1);
   play_city_ = (configuration_manager_->GetConfigurationInt("Devices", "PlayCity", 0, conf_path_.string().c_str()) == 1);
}

void SettingsContainer::SetLowerRom (char* rom)
{
   lower_rom_ = rom;
}

void SettingsContainer::SetDefaultCartridge(char* cartridge)
{
   default_cartridge_ = cartridge;
}

void SettingsContainer::SetUpperRom (unsigned int num, char* rom)
{
   if (num < 256)
      upper_roms_list_[num] = rom;
}


const char* SettingsContainer::GetAvailableRom  ( unsigned int num)
{
   if ( num < available_rom_list_.size())
   {
      return available_rom_list_[num].c_str();
   }
   return NULL;
}

void SettingsContainer::ComputeAvailableRoms()
{
   fs::path path = conf_path_;
   path /= "ROM\\*\\.*\\";

   const std::regex my_filter(path.generic_string().c_str());

   available_rom_list_.clear();
   available_path_rom_list_.clear();

   for (auto& p : fs::directory_iterator(conf_path_))
   {
      if (fs::is_regular_file(p.status()))
      {
         if (regex_match(p.path().filename().generic_string().c_str(), my_filter))
         {
            available_rom_list_.push_back(p.path().filename().string());
            available_path_rom_list_.push_back(p.path().string());
         }
      }
   }

}
