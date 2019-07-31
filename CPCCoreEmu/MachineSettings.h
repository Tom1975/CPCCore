#pragma once

//////////////////////////////////////////////////////////
// MachineSettings.h
//
// This class handle one settings of the emulated hardware.
//
// (c) T.Guillemin 2018
//
//
//////////////////////////////////////////////////////////

#include <map>

#include "IConfiguration.h"
#include "CRTC.h"

class MachineSettings
{
public:

	typedef enum
	{
		OLD_464,
		OLD_664,
		OLD_6128,
		PLUS_464,
		PLUS_6128,
		GX400
	} HardwareType;

	//////////////////////////////////////////////////////////
	// Constructor / Destructor
	MachineSettings();
	virtual ~MachineSettings();

	bool operator== (MachineSettings& other) const;

	//////////////////////////////////////////////////////////
	// Factory
	static MachineSettings * CreateSettings(IConfiguration * configuration_manager, const char* path);

	//////////////////////////////////////////////////////////
	// Initialisation
	void Init(IConfiguration * configuration_manager);

	//////////////////////////////////////////////////////////
	// Load / Save settings
	virtual bool SetFile(const char* path);
	virtual const char* GetFilePath() const { return file_path_.c_str(); }
	virtual bool Load(bool force_reload = false);
	virtual bool Save(const char* new_name = nullptr);

	//////////////////////////////////////////////////////////
	// Acces to settings elements
	const char* GetDescription() const { return description_.c_str(); }
	void SetDescription(const char* description) { description_ = description; }
	const char* GetShortDescription() const { return short_description_.c_str(); }
	void SetShortDescription(const char* description) { short_description_ = description; }

	////////////////////////////////////////////////////////////////
	// RAM, ROM, CARTRIDGE
	// Memory conf
	typedef enum {
		M64_K = 0,
		M128_K = 1,
		M512_K = 0xFF
	} RamCfg;

	RamCfg GetRamCfg() const;;
	void SetRamCfg(RamCfg ram_cfg) { ram_cfg_ = ram_cfg; };

	const char* GetLowerRom() const { return lower_rom_.c_str(); };
	void SetLowerRom(char* rom) { lower_rom_ = rom; };

	const char* GetUpperRom(unsigned int num) const { auto it = upper_rom_.find(num); return (it == upper_rom_.end()) ? nullptr : it->second.c_str(); };
	void SetUpperRom(unsigned int num, const char* rom) { upper_rom_[num] = rom; };

	virtual void SetDefaultCartridge(const char * cartridge_path) { cartridge_path_ = cartridge_path; }
	virtual const char* GetDefaultCartridge() { return cartridge_path_.c_str(); }

	// Hardware type
	virtual HardwareType GetHardwareType() { return hardware_type_; };
	virtual void SetHardwareType(HardwareType new_type) { hardware_type_ = new_type; };

	// CRTC
	CRTC::TypeCRTC GetCRTCType() const { return type_crtc_; }
	void SetCRTCType(CRTC::TypeCRTC  type_crtc) { type_crtc_ = type_crtc; }

	// FDC
	bool FDCPlugged() const { return fdc_present_; }
	void SetFDCPlugged(bool fdc_present) { fdc_present_ = fdc_present; };

	// PAL
	bool PALPlugged() const { return pal_present_; }
	void SetPALPlugged(bool fdc_present) { pal_present_ = fdc_present; };

	// Keyboard settings
	const char* GetKeyboardConfig() { return keyboard_config_.c_str(); };
	void SetKeyboardConfig(const char* key_config) { keyboard_config_ = key_config; };

protected:

	// Keep track of loaded settings
	bool loaded_;
	// Filename
	std::string file_path_;

	// Configuration manager : generic access
	IConfiguration * configuration_manager_;

	// Description
	std::string description_;
	// Short description
	std::string short_description_;

	// RAM configuration
	RamCfg ram_cfg_;
	std::string lower_rom_;

	// ROM configuration
	std::map<int, std::string> upper_rom_;

	// CARTRIDGE configuration (if any)
	std::string cartridge_path_;

	// Hardware type (if needed)
	HardwareType hardware_type_;

	//////////////////////////
	// Hardware
	CRTC::TypeCRTC type_crtc_;
	bool fdc_present_;
	bool pal_present_;

	//////////////////////////
	// Keyboard
	std::string keyboard_config_;

	// External device default plugged

};


