#pragma once

#include <list>
#include <thread>

#include "Z80_Full.h"
#include "Breakpoint.h"
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
#include "SettingsContainer.h"
#include "Monitor.h"
#include "IDirectories.h"
#include "IConfiguration.h"
#include "MultifaceII.h"
#include "Tape.h"
#include "IComponent.h"
#include "ISound.h"
#include "PrinterDefault.h"
#include "IPlayback.h"
#include "ITapeAccelerator.h"
#include "Snapshot.h"
#include "DiskContainer.h"
#include "MediaManager.h"
#include "SoundMixer.h"
#include "PlayCity.h"
#include "DMA.h"
#include "MachineSettings.h"
#include "EmulatorSettings.h"

/*#ifdef CPCCOREEMU_EXPORTS
#define CPCCOREEMU_API __declspec(dllexport)
#else
#define CPCCOREEMU_API __declspec(dllimport)
#endif
*/
#define CPCCOREEMU_API

#define NB_BP_MAX    10


///////////////////////////////
// Definition de la machine elle-meme.


class CursorLine : public IClockable
{
public:
   CursorLine (PlayCity* playcity) : playcity_(playcity)
   {}
   virtual unsigned int Tick() { playcity_->GetCtc()->channel_[1].Trg(true); playcity_->GetCtc()->channel_[1].Trg(false); return 1; }
protected:
   PlayCity* playcity_;
};


class NetList : public IClockable
{
public:
   NetList( bool* signal) :signal_(signal) {};
   virtual unsigned int Tick() { if (signal_)*signal_ = true; return 1; }

private:
   bool* signal_;
};

class NetListINT : public IClockable
{
public:
   NetListINT(CSig* z80_int) :z80_int_(z80_int) {};
   virtual unsigned int Tick() { if (z80_int_)z80_int_->req_int_ = true; return 1; }

private:
   CSig* z80_int_;
};


class NetListNMI : public IClockable
{
public:
   NetListNMI(CSig* z80_int) :z80_int_(z80_int) {};
   virtual unsigned int Tick() { if(z80_int_)z80_int_->nmi_ = true; return 1; }

private:
   CSig* z80_int_;
};



class ISupervisor
{
public:
   virtual void EmulationStopped () = 0;
   virtual void SetEfficience (double p) = 0;
   virtual void RefreshRunningData  () = 0;
};


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

class CPCCOREEMU_API EmulatorEngine : public IMachine, public ITapeAccelerator, public ILoadingProgree, public ISynchro
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
   virtual void StartRecord (char* path_file) { sna_handler_.StartRecord (path_file) ;}
   virtual void StopRecord () { sna_handler_.StopRecord () ;}
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

   MachineSettings* GetSettings(){return current_settings_ ;}
   void ChangeSettings(MachineSettings* new_settings) 
   {
      current_settings_ = new_settings;
      UpdateComputer();
   }


   IKeyboard* GetKeyboardHandler () { return psg_;}
   virtual void SetSupervisor (ISupervisor* supervisor) {supervisor_ = supervisor;}
   virtual void SetDirectories ( IDirectories * dir ){ directories_ = dir;
      default_printer_.SetDirectories(dir);   }
   virtual void SetConfigurationManager(IConfiguration * configuration_manager) {
      configuration_manager_ = configuration_manager; psg_->SetConfigurationManager(configuration_manager_);
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

   void SetLog(ILog* log) { log_ = log; fdc_->SetLog(log); crtc_.SetLog(log_); signals_.SetLog(log_); ppi_.SetLog(log_); }
   void SetNotifier(INotify* notifier) { notifier_ = notifier; sna_handler_.SetNotifier(notifier);if(fdc_!=NULL)fdc_->SetNotifier(notifier); tape_.SetNotifier (notifier);}

   virtual void Demarrer ();
   virtual int RunTimeSlice(bool dbg = false);
   virtual void DoSynchroVbl();

   void BuildEngine();


   virtual void ForceTick (IComponent* component, int ticks=0);

   void InitStartOptimized ();
   void InitStartOptimizedPlus ();

   void Resync ();
   void StartOptimized(unsigned int nb_cycles);
   void StartOptimizedPlus(unsigned int nb_cycles);
   void StartPrecise(unsigned int nb_cycles);
   void BuildMachine ();
   void BuildMachine464 ();
   void BuildMachine664 ();
   void BuildMachine6128 ();
   void BuildMachineCustom ();
   void UpdateExternalDevices();
   void SetMachineType(int type) { type_machine_ = type; };
   int GetMachineType() { return type_machine_; }
   void SetPlus(bool plus);
   bool IsPLUS() { return plus_; }

   void Stop ();

   virtual int DebugNew(unsigned int nb_cycles);

   void AddBreakpoint ( unsigned short addr);
   void ChangeBreakpoint ( unsigned short  old_bp, unsigned short new_bp );

   void RemoveBreakpoint ( unsigned short addr);
   void CleanBreakpoints ();

   void SetBreakpointHandler ( IBreakpoint* bp );

   // Display
   void SetScanlines (int type){vga_.SetScanlines (type);};
   int GetScanlines (){return vga_.GetScanlines ();};

   // Record
   void BeginRecord();
   void EndRecord ();
   bool IsRecording ();

   void SetFixedSpeed(bool bFixed) { if (bFixed) SetRandomSeed(0x1337);  fdc_->SetFixedSpeed(bFixed); }
   void SetRandomSeed(unsigned int seed) { srand(seed); };

   // Control
   void Reinit ();
   void Reset ();
   void ResetPlus();
   void OnOff ();


   void GunSet(int x, int y, int button) { crtc_.GunSet ( x, y, button); };

   // Disk and tape
   void InsertBlankDisk ( unsigned int drive_number, IDisk::DiskType );
   void Eject ( unsigned int drive_number = 0);

   DataContainer* CanLoad(char* file, std::vector<MediaManager::MediaType>list_of_types = { MediaManager::MEDIA_DISK, MediaManager::MEDIA_SNA, MediaManager::MEDIA_SNR, MediaManager::MEDIA_TAPE, MediaManager::MEDIA_BIN,MediaManager::MEDIA_CPR });
   void ReleaseContainer(DataContainer* container);

   int LoadMedia(DataContainer* container);
   int LoadDisk ( const char* file_path, unsigned int drive_number = 0, bool differential_load = true);
   int LoadDisk ( DataContainer* container, unsigned int drive_number = 0, bool differential_load = true);
   void FlipDisk ( unsigned int drive_number = 0);
// 0 : Not present;  1 : No; 2 : Read; 3 : Write
   int IsDiskRunning (int drive );
   bool IsDiskModified (int drive){return fdc_->IsDiskModified(drive); };
   void SaveDisk (int drive);
   void SaveDiskAs (int drive, char* file_path, FormatType* format_type);
   bool IsDiskPresent ( int drive );

   int CompareDriveAandB();

   int GetDiskTrack ( int drive){return fdc_->GetCurrentTrack(drive);};
   int GetDiskSector ( int drive){return fdc_->GetCurrentSector(drive);};
   int GetDiskSide ( int drive){return fdc_->GetCurrentSide(drive);};
   int GetCurrentDrive ( ) {return fdc_->GetCurrentDrive ();};

   // Tape
   int LoadTape ( const char* file_path);
   int LoadTape(IContainedElement* container );

   // Cartridge
   int LoadCprFromBuffer(unsigned char* buffer, int size);
   int LoadCpr(const char* file_path);
   int LoadCpr(IContainedElement* container);

   // Multiface II
   void MultifaceStop (){multiface_stop_ = true;}
   void MultifaceToggleVisible (){multiface2_.Visible(multiface2_.IsVisible()?false:true);};
   bool IsMultifaceIIVisible () { return multiface2_.IsVisible();};
   // Données
   IPrinterPort* GetPrinter () { return printer_;};
   Memory* GetMem () { return memory_;};
   Asic* GetAsic() { return &asic_; }

   IZ80* GetProc () { return &z80_;};
   Z80* GetProcFull() { return &z80_; };

   Ay8912* GetPSG() {return psg_;};
   CRTC* GetCRTC () { return &crtc_;}
   GateArray* GetVGA() { return &vga_; }
   FDC* GetFDC () { return fdc_;};
   CTape* GetTape () { return &tape_;};
   CMonitor* GetMonitor () { return &monitor_;}
   PPI8255* GetPPI () { return &ppi_;}
   CSig* GetSig(){return &signals_;}
   bool GetPAL () { return pal_present_;}
   void SetPAL ( bool bPAL) { pal_present_ = bPAL; vga_.SetPAL( bPAL);};
   void SetFDCPlugged ( bool bFDCPlugged) { fdc_present_ = bFDCPlugged;signals_.fdc_present_ = fdc_present_;ppi_.SetExpSignal ( fdc_present_ );};
   void ChangeConfig (MachineSettings* current_settings);
   void LimitSpeed (SpeedLimit bLimit){speed_limit_ = bLimit;};

   void SetSpeed ( int speed_limit );
   unsigned int GetSpeedLimit (){return speed_;}
   unsigned int GetSpeed (){return speed_percent_;}

   int speed_;

   bool run_;
   bool step_in_;
   bool step_;

   bool bin_to_load_;
   std::string bin_to_load_path_;

   unsigned int stop_pc_;
   bool remember_step_;

   unsigned int counter_ ;

   // Expansion available
   CMultifaceII multiface2_;
   PlayCity * play_city_;

   bool multiface_stop_;

   IClockable* GetNMILine() {return &netlist_nmi_;};
   IClockable* GetINTLine() { return &netlist_int_; };

   bool LoadBinInt(unsigned char* buffer, unsigned int size);
   bool LoadBinInt(const char* path_file);

   DMA* GetDMA(int i) { return &dma_[i]; }

protected:

   ///////////////////////////////////////
   // What's in this machine ?
   ///////////////////////////////////////
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
   INotify* notifier_;

   ISupervisor* supervisor_;
   void DemarrerThread ();
   static void Run ( void* param);

   IDirectories * directories_;
   IConfiguration * configuration_manager_;

   // Max 10 breakpoints
   unsigned short breakpoint_list_ [NB_BP_MAX];
   unsigned int breakpoint_index_;

   // Breakpoint generic
   IBreakpoint* generic_breakpoint_;


   // Signaux divers
   CSig signals_;

   // Z80
   Z80 z80_;

   bool plus_;

   // Video Gate Array (video)
   Asic asic_;
   DMA dma_[3];
   GateArray vga_;

   // 6845 CRTC
   CRTC crtc_;

   // Son
   // AY-3-8912 (PSG)
   Ay8912 *psg_;

   // I/O
   // PPI (interface paralelle) 8255
   PPI8255 ppi_;

   // CRT Monitor
   CMonitor monitor_;

   // Tape
   CTape tape_;

   // Floppy - FDC 765
   FDC *fdc_;
   std::string fd1_path_;
   std::string fd2_path_;
   
   // Paste Command
   char paste_buffer_ [4096+1];
   unsigned int paste_count_;
   unsigned int paste_size_;
   bool paste_available_;
   int paste_wait_time_;
   unsigned int paste_vkey_ ;

   // Bus : Adresse (16b) et donnes (8b)
   Bus address_bus_;
   Bus data_bus_;

   // Overall Memory Map
   Memory *memory_;

   // Settings handler
   MachineSettings * current_settings_;
   EmulatorSettings emulator_settings_;

   // Display handler
   IDisplay* display_;

   IPrinterPort* printer_;
   CPrinterDefault default_printer_;

   // Internal component list
   IComponent * component_list_[10]; // Used to avoid allocation
   unsigned int component_elapsed_time_ [32];

   int nb_components_ ;
   int z80_index_;

   // Snapshot handler
   CSnapshot sna_handler_;
   DataContainer media_inserted_;

   CDskTypeManager disk_type_manager_;

   /////////////////////////////////
   // Netlists
   NetListINT netlist_int_;
   NetList netlist_nmi_;

   CursorLine* cursor_line_;
   int type_machine_;
   bool do_snapshot_;
   std::string snapshot_file_;

   ISound* sound_player_;
   SoundMixer sound_mixer_;

   BreakpointHandler breakpoint_handler_;

   std::thread emulation_thread_;

   void RebuildComponents();
   void ClearComponents();

   struct ComponentList
   {
      ComponentList* QueueComponent(IComponent* comp, unsigned int init_time) {
         next = new ComponentList;
         next->next = nullptr; 
         next->component = comp;
         next->elapsed_time = init_time;
         elapsed_time = 0;
         return next;
      }
      IComponent * component;
      ComponentList* next;
      unsigned int elapsed_time;
   };
   ComponentList *head_component_;
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
