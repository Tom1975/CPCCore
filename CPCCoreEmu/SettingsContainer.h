#pragma once

#include "CRTC.h"
#include "ISound.h"
#include "IConfiguration.h"

#ifdef _WIN32
#include <experimental\filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

class EmulatorEngine;

/*#ifdef CPCCOREEMU_EXPORTS
#define CPCCOREEMU_API __declspec(dllexport)
#else
#define CPCCOREEMU_API __declspec(dllimport)
#endif
*/
#define CPCCOREEMU_API


class CPCCOREEMU_API SettingsContainer
{
public:
   SettingsContainer(const char* path);
   virtual ~SettingsContainer();

   virtual void Init (EmulatorEngine* machine, ISoundFactory* sound_factory, IConfiguration * configuration_manager);
   virtual void Load (char* path);
   virtual void SaveAs (char* path );
   virtual void Save ();
   virtual void UpdateComputer(bool no_cart_reload = false);
   virtual std::string GetPath() { return conf_path_.string();};

   ////////////////////////////////////////////////////////////////
   // Conf description
   const char* GetDescription (){return description_.c_str();};
   void SetDescription ( char* txt );
   const char* GetShortName(){return description_short_.c_str();};
   void SetShortName ( char* txt );

   ////////////////////////////////////////////////////////////////
   // Hardware
   CRTC::TypeCRTC GetCRTCType (){ return type_crtc_;}
   void SetCRTCType ( CRTC::TypeCRTC  typeCRTC ) {type_crtc_ = typeCRTC;}

   bool FDCPlugged () {return fdc_present_;}
   void SetFDCPlugged ( bool bFdc ) { fdc_present_ = bFdc; };

   ////////////////////////////////////////////////////////////////
   // Memory conf
   typedef enum {
      M64_K = 0,
      M128_K = 1,
      M512_K = 0xFF
   } RAMCfg;

   RAMCfg GetRAMCfg () { return ram_cfg_;} ;
   void SetRAMCfg ( RAMCfg ramCfg) {ram_cfg_ = ramCfg;};

   typedef enum
   {
      OLD_464,
      OLD_664,
      OLD_6128,
      PLUS_464,
      PLUS_6128,
      GX400
   } HardwareType;

   virtual HardwareType GetHardwareType() { return hardware_type_; };
   virtual void SetHardwareType(HardwareType new_type) { hardware_type_ = new_type; };

   std::string GetLowerRom() { return lower_rom_.string();} ;
   void SetLowerRom (char* rom);

   std::string GetDefaultCartridge() { return default_cartridge_.string(); };
   void SetDefaultCartridge(char* cartridge);

   std::string GetUpperRom( unsigned int num) {return upper_roms_list_[num].string();};
   void SetUpperRom (unsigned int num, char* rom);

   const char* GetAvailableRom  ( unsigned int num );
   unsigned int GetNumberOfAvailableRoms ( ) { return available_rom_list_.size();};

   ////////////////////////////////////////////////////////////////
   // Sound description
   ISound* GetSound (){return sound_;};
   void SetSound (ISound* sound){sound_ = sound;};

   ////////////////////////////////////////////////////////////////
   // Keyboard & joystick
   const char* GetKeyboardConfig () { return keyboard_config_.c_str(); }
   void SetKeyboardConfig ( const char* key_config ){ keyboard_config_ = key_config;}
   const std::string GetROMPath () { return rom_path_.string();}

   void SetMultiface2(bool set) { multiface_2_ = set; }
   bool GetMultiface2() {return multiface_2_;}
   void SetPlayCity(bool set) { play_city_ = set; }
   bool GetPlayCity() { return play_city_; }

protected:
   void ComputeAvailableRoms();

   IConfiguration * configuration_manager_;

   #define MAX_ROMS 100
   std::vector<std::string> available_rom_list_;
   std::vector<std::string> available_path_rom_list_;

   // Configuration
   EmulatorEngine* machine_;
   std::string description_;
   std::string description_short_;

   //////////////////////////
   // Type of hardware
   HardwareType hardware_type_;

   //////////////////////////
   // RAM
   RAMCfg ram_cfg_;

   //////////////////////////
   // default Cartridge
   fs::path default_cartridge_;

   //////////////////////////
   // ROM
   fs::path rom_path_;
   fs::path conf_path_;
   fs::path base_conf_path_;
   fs::path lower_rom_;
   fs::path upper_roms_list_[256];


   //////////////////////////
   // Sound
   ISound* sound_;
   ISoundFactory* sound_factory_ ;

   //////////////////////////
   // Keyboard
   std::string keyboard_config_;

   //////////////////////////
   // Hardware
   CRTC::TypeCRTC type_crtc_;
   bool fdc_present_;
   bool pal_present_;

   //////////////////////////
   // External Devices
   bool multiface_2_;
   bool play_city_;
};

