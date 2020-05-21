#pragma once

#include <list>
#include <thread>

#include "Motherboard.h"
#include "Z80_Full.h"
#include "BreakpointHandler.h"
#include "Bus.h"
#include "Memoire.h"
#include "Bus.h"
#include "Asic.h"
#include "VGA.h"
#include "CRTC.h"
#include "Sig.h"
#include "PPI.h"
#include "PSG.h"
#include "FDC.h"
#include "Screen.h"
#include "ILog.h"
#include "Inotify.h"
#include "Monitor.h"
#include "IDirectories.h"
#include "IConfiguration.h"
#include "MultifaceII.h"
#include "Tape.h"
#include "IComponent.h"
#include "ISound.h"
#include "PrinterDefault.h"
#include "IPlayback.h"
#include "Snapshot.h"
#include "DiskContainer.h"
#include "MediaManager.h"
#include "SoundMixer.h"
#include "PlayCity.h"
#include "DMA.h"
#include "MachineSettings.h"
#include "EmulatorSettings.h"
#include "KeyboardHandler.h"

#define CPCCOREEMU_API


///////////////////////////////
// Definition de la machine elle-meme.




////////////////////////////////////
// Definition of strategy pattern for media loading
class ILoadingMedia
{
public :
   virtual int LoadMedia(DataContainer* container) = 0;
};

class ILoadingProgree
{
public :
   virtual int GetCurrentProgress() = 0;
};

class CPCCOREEMU_API EmulatorEngine : public ILoadingProgree
{
public:
   typedef enum {
      E_NONE,
      E_FULL,
      E_VBL
   } SpeedLimit;


   EmulatorEngine();
   virtual ~EmulatorEngine(void);

   // Snapshots
   BreakpointHandler* GetBreakpointHandler() {return &breakpoint_handler_;}
   virtual void StopPlayback (){sna_handler_.StopPlayback ();}
   virtual void StartRecord (const char* path_file) { sna_handler_.StartRecord (path_file) ;}
   virtual void StopRecord () { sna_handler_.StopRecord () ;}
   virtual bool IsSnrRecording() { return sna_handler_.IsRecording(); }
   virtual bool IsSnrReplaying() { return sna_handler_.IsReplaying(); }

   bool LoadSnr (const char* path_file) {return sna_handler_.LoadSnr (path_file);}
   bool LoadBin(const char* path_file);
   bool LoadSnapshot (const char* path_file);
   bool LoadSnapshotDelayed();
   bool SaveSnapshot (const char* path_file);
   bool IsQuickSnapAvailable ();
   bool QuickLoadsnapshot ();
   bool QuickSavesnapshot ();
   virtual void Accelerate ( bool speed_on);

   virtual int GetCurrentProgress();

   void SetSettings(EmulatorSettings& emulator_settings);
   void UpdateFromSettings();
   void LoadRom(int rom_number, const char* path);

   MachineSettings* GetSettings(){return current_settings_ ;}
   void ChangeSettings(MachineSettings* new_settings) 
   {
      current_settings_ = new_settings;
      UpdateComputer();
   }


   KeyboardHandler* GetKeyboardHandler () { return &keyboardhandler_;}
   virtual void SetSupervisor (ISupervisor* supervisor) {motherboard_.SetSupervisor( supervisor);}
   virtual void SetDirectories ( IDirectories * dir ){ directories_ = dir;
      motherboard_.SetDirectories(dir);   }
   virtual void SetConfigurationManager(IConfiguration * configuration_manager) {
      configuration_manager_ = configuration_manager; motherboard_.SetConfigurationManager(configuration_manager_);
   }

   void UpdateComputer(bool no_cart_reload = false);
   void UpdateEmulator();

   virtual bool Init (IDisplay* display, ISoundFactory* sound);
   virtual bool InitSound ( ISound* sound);
   virtual ISoundMixer* GetMixer();
   virtual void Paste (const char* command);
   virtual bool PasteBufferIsEmpty() {
      return paste_size_ == 0;}
   virtual SpeedLimit IsSpeedLimited();

   virtual void SetDefaultConfiguration ();
   virtual void SaveConfiguration (const char* config_name, const char* ini_file);
   virtual void LoadConfiguration (const char* config_name, const char* ini_file);

   void SetLog(ILog* log) { log_ = log; motherboard_.SetLog(log_);sound_mixer_.SetLog(log_);}
   void SetNotifier(IFdcNotify* notifier) { notifier_ = notifier; sna_handler_.SetNotifier(notifier);motherboard_.SetNotifier(notifier);}

   void HandleActions();
   void HandleSnapshots();
   void HandlePaste(bool before);
   void HandleSyncro(int run_time);
   virtual int RunTimeSlice(bool dbg = false);

   typedef enum
   {
      DBG_OPCODE_END,
      DBG_BREAKPOINT_FOUND,

   } DebugRunResult;
   virtual DebugRunResult RunDebugMode(unsigned int nb_opcode_to_run);
   virtual void RunFullSpeed();

   // Functions
   void RunUntilBReakpoint(int nb_opcode_to_run);

   void Resync ();
   void StartPrecise(unsigned int nb_cycles);
   void UpdateExternalDevices();
   void SetMachineType(int type) { type_machine_ = type; };
   int GetMachineType() { return type_machine_; }
   void SetPlus(bool plus);
   bool IsPLUS() { return motherboard_.IsPLUS(); }

   void SetStepIn(bool set) { motherboard_.step_in_ = set; }
   void SetRun(bool set) { motherboard_.run_ = set; }
   bool IsRunning() { return motherboard_.run_; }
   void Stop ();

   void AddBreakpoint ( unsigned short addr);
   void ChangeBreakpoint ( unsigned short  old_bp, unsigned short new_bp );

   void RemoveBreakpoint ( unsigned short addr);
   void SetBreakpointHandler ( IBreakpoint* bp );

   // Display
   void SetScanlines (int type){GetVGA()->SetScanlines (type);};
   int GetScanlines (){return GetVGA()->GetScanlines ();};

   // Record
   void BeginRecord();
   void EndRecord ();
   bool IsRecording ();

   void SetFixedSpeed(bool bFixed) { if (bFixed) SetRandomSeed(0x1337);  GetFDC()->SetFixedSpeed(bFixed); }
   void SetRandomSeed(unsigned int seed) { srand(seed); };

   // Control
   void Reinit ();
   void Reset ();
   void ResetPlus();
   void OnOff ();


   void GunSet(int x, int y, int button) { GetCRTC()->GunSet ( x, y, button); };

   // Disk and tape
   void InsertBlankDisk ( unsigned int drive_number, IDisk::DiskType );
   void Eject ( unsigned int drive_number = 0);

#ifndef MINIMUM_DEPENDENCIES
   DataContainer* CanLoad(const char* file, std::vector<MediaManager::MediaType>list_of_types = { MediaManager::MEDIA_DISK, MediaManager::MEDIA_SNA, MediaManager::MEDIA_SNR, MediaManager::MEDIA_TAPE, MediaManager::MEDIA_BIN,MediaManager::MEDIA_CPR });
   void ReleaseContainer(DataContainer* container);

   int LoadMedia(DataContainer* container);
   int LoadDisk ( DataContainer* container, unsigned int drive_number = 0, bool differential_load = true);
#endif
   int LoadDisk ( const char* file_path, unsigned int drive_number = 0, bool differential_load = true);
   void FlipDisk ( unsigned int drive_number = 0);
// 0 : Not present;  1 : No; 2 : Read; 3 : Write
   int IsDiskRunning (int drive );
   bool IsDiskModified (int drive){return GetFDC()->IsDiskModified(drive); };
   void SaveDisk (int drive);
   void SaveDiskAs (int drive, const char* file_path, const FormatType* format_type);
   bool IsDiskPresent ( int drive );

   int CompareDriveAandB();

   int GetDiskTrack ( int drive){return GetFDC()->GetCurrentTrack(drive);};
   int GetDiskSector ( int drive){return GetFDC()->GetCurrentSector(drive);};
   int GetDiskSide ( int drive){return GetFDC()->GetCurrentSide(drive);};
   int GetCurrentDrive ( ) {return GetFDC()->GetCurrentDrive ();};

   // Tape
   int LoadTape ( const char* file_path);
   int LoadTape(IContainedElement* container );
   bool IsTapeChanged() { return motherboard_.GetTape()->IsTapeChanged();};

   // Cartridge
   int LoadCprFromBuffer(unsigned char* buffer, int size);
   int LoadCpr(const char* file_path);
   int LoadCpr(IContainedElement* container);

   // Multiface II
   void MultifaceStop (){multiface_stop_ = true;}
   void MultifaceToggleVisible (){multiface2_.Visible(multiface2_.IsVisible()?false:true);};
   bool IsMultifaceIIVisible () { return multiface2_.IsVisible();};
   // Données
   IPrinterPort* GetPrinter () { return motherboard_.GetPrinter();};
   Memory* GetMem () { return motherboard_.GetMem();};
   Asic* GetAsic() { return motherboard_.GetAsic(); }

   Z80* GetProc () { return motherboard_.GetProc();};

   Ay8912* GetPSG() {return motherboard_.GetPSG();};
   CRTC* GetCRTC () { return motherboard_.GetCRTC();}
   GateArray* GetVGA() { return motherboard_.GetVGA(); }
   FDC* GetFDC () { return motherboard_.GetFDC();};
   CTape* GetTape () { return motherboard_.GetTape();};
   Monitor* GetMonitor () { return motherboard_.GetMonitor();}
   PPI8255* GetPPI () { return motherboard_.GetPPI();}
   CSig* GetSig(){return motherboard_.GetSig();}
   bool GetPAL () { return pal_present_;}
   void SetPAL ( bool bPAL) { pal_present_ = bPAL; GetVGA()->SetPAL( bPAL);};
   void SetFDCPlugged ( bool bFDCPlugged) { fdc_present_ = bFDCPlugged;GetSig()->fdc_present_ = fdc_present_;GetPPI()->SetExpSignal ( fdc_present_ );};
   void ChangeConfig (MachineSettings* current_settings);
   void LimitSpeed (SpeedLimit bLimit){speed_limit_ = bLimit;};

   void SetSpeed ( int speed_limit );
   unsigned int GetSpeedLimit (){return speed_;}
   unsigned int GetSpeed (){return speed_percent_;}

   int speed_;

   bool bin_to_load_;
   std::string bin_to_load_path_;

   unsigned int stop_pc_;
   bool remember_step_;

   bool old_stop_on_fetch_;

   unsigned int counter_ ;

   // Expansion available
   MultifaceII multiface2_;
   KeyboardHandler keyboardhandler_;

   bool multiface_stop_;

   bool LoadBinInt(unsigned char* buffer, unsigned int size);
   bool LoadBinInt(const char* path_file);

   DMA* GetDMA(int i) { return motherboard_.GetDMA(i); }
   Motherboard * GetMotherboard() {return &motherboard_;}

protected:

   ///////////////////////////////////////
   // What's in this machine ?
   ///////////////////////////////////////
   Motherboard motherboard_;

   bool pal_present_;
   bool fdc_present_;

   /////////////////////////////
   // Donnees interne
   bool quick_sna_available_;
   std::string quick_sna_path_;

   bool sna_to_load_;
   std::string sna_path_to_load_;

   SpeedLimit speed_limit_;

   unsigned int speed_percent_;
   unsigned int time_slice_;
   unsigned long long time_computed_;

   std::chrono::time_point<std::chrono::steady_clock> time_elapsed_;

   ILog* log_;
   IFdcNotify* notifier_;

   IDirectories * directories_;
   IConfiguration * configuration_manager_;

   // Breakpoint generic
   IBreakpoint* generic_breakpoint_;

   std::string fd1_path_;
   std::string fd2_path_;
   
   // Paste Command
   char paste_buffer_ [4096+1];
   unsigned int paste_count_;
   unsigned int paste_size_;
   bool paste_available_;
   int paste_wait_time_;
   unsigned int paste_vkey_ ;

   // Settings handler
   MachineSettings * current_settings_;
   EmulatorSettings emulator_settings_;

   // Display handler
   IDisplay* display_;

   // Snapshot handler
   CSnapshot sna_handler_;
   DataContainer media_inserted_;

   DskTypeManager disk_type_manager_;

   /////////////////////////////////
   // Netlists

   int type_machine_;
   bool do_snapshot_;
   std::string snapshot_file_;

   ISound* sound_player_;
   SoundMixer sound_mixer_;

   BreakpointHandler breakpoint_handler_;

};

class LoadingDisk : public ILoadingMedia
{
public:
   LoadingDisk(EmulatorEngine* machine, unsigned int drive_number, bool differential) :machine_(machine), drive_number_(drive_number), differential_(differential) {}
   virtual int LoadMedia(DataContainer* container)
   {
      machine_->LoadDisk(container, drive_number_, differential_);
   };

protected:
   EmulatorEngine* machine_;
   unsigned int drive_number_;
   bool differential_;
};
