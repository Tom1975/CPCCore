#include "stdafx.h"
#include "Machine.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "PrinterDefault.h"
#include "MediaManager.h"

// MACRO de profilage
// MACRO

#ifdef _DEBUG
//#define PROF
#endif

#ifdef PROF
#define START_CHRONO  QueryPerformanceFrequency((LARGE_INTEGER*)&freq);;QueryPerformanceCounter ((LARGE_INTEGER*)&s1);
#define STOP_CHRONO   QueryPerformanceCounter ((LARGE_INTEGER*)&s2);t=(DWORD)(((s2 - s1) * 1000000) / freq);
#define PROF_DISPLAY _stprintf(s, _T("Duree displays Frame: %d us\n"), t);OutputDebugString (s);

static __int64 s1, s2, freq;
static DWORD t;
static char s [1024];
#else
   #define START_CHRONO
   #define STOP_CHRONO
   #define PROF_DISPLAY
#endif

extern unsigned int ListeColorsIndex[0x100];
extern unsigned int ListeColorsIndexConvert[32];
extern unsigned int ListeColors[0x100];

const char* ROMPath = "\\ROM\\";
const char* CartPath = "\\CART\\";


#define MAX_SIZE_BUFFER 256

EmulatorEngine::EmulatorEngine() :
   address_bus_(16), data_bus_(8), paste_size_(0), paste_count_(0), media_inserted_(&disk_type_manager_),
   netlist_int_(&signals_), netlist_nmi_(&signals_.nmi_), plus_(false), do_snapshot_(false), current_settings_(nullptr),
   head_component_(nullptr), directories_ (nullptr), display_(nullptr), motherboard_(&sound_mixer_)
{

   fdc_present_ = true;
   pal_present_ = true;
   quick_sna_available_ = false;
   supervisor_ = NULL;
   breakpoint_index_ = 0;
   memset ( breakpoint_list_ , 0, sizeof(breakpoint_list_));
   log_ = NULL;

   time_slice_ = 20;
   time_elapsed_ = std::chrono::steady_clock::now();
   time_computed_ = 0;

   speed_limit_ = E_NONE;
   memset (paste_buffer_, 0, sizeof(paste_buffer_));
   paste_wait_time_ = 3;
   paste_vkey_ = 0;
   speed_percent_ = 0;
   memory_ = new Memory (&monitor_);
   fdc_ = new FDC;
   psg_ = new Ay8912 (&sound_mixer_);
   play_city_ = new PlayCity(&netlist_int_, &netlist_nmi_, &sound_mixer_);
   cursor_line_ = new  CursorLine(play_city_);

   generic_breakpoint_ = NULL;

   speed_ = 100;
   printer_ = NULL;
   notifier_ = NULL;
   stop_pc_ = 0;
   remember_step_ = false;
   bin_to_load_ = false;
   sna_to_load_ = false;
}


EmulatorEngine::~EmulatorEngine(void)
{
   ClearComponents();

   delete memory_ ;
   delete fdc_;
   delete psg_;
   delete play_city_;
   delete cursor_line_;

   /*if (generated_loop_.y != NULL)
   {
      VirtualFreeEx(GetCurrentProcess(), generated_loop_.y, 0, MEM_RELEASE);
   }*/

}

bool EmulatorEngine::Init (IDisplay* display, ISoundFactory* sound)
{
   DataContainer::Init();

   display_ = display;
   counter_ = 0;
   paste_available_ = false;


   run_ = false;
   step_in_ = false;
   step_ = false;

   // Initialisation des donnes interne
   z80_.Init (memory_, &signals_, log_);

   signals_.address_bus_ = &address_bus_;
   signals_.data_bus_ = &data_bus_;
   signals_.ppi_ = &ppi_;
   signals_.fdc_ = fdc_;
   signals_.crtc_ = &crtc_;
   signals_.vga_ = &vga_;
   signals_.z80_ =  &z80_;
   signals_.memory_ = memory_;
   signals_.asic_ = &asic_;

   // CRTC
   //crtc_.SetBus ( &address_bus_, &data_bus_ );
   crtc_.SetSig ( &signals_ );
   crtc_.SetGateArray ( &vga_ );
   crtc_.SetPPI ( &ppi_ );
   crtc_.SetPlayback ( &sna_handler_  );

   // Gate array
   monitor_.SetCRTC ( & crtc_ );
   monitor_.SetVGA (&vga_ );
   monitor_.SetScreen (display_);
   monitor_.SetPlayback ( &sna_handler_ );


   vga_.SetMonitor ( &monitor_ );
   vga_.SetDMA(dma_);

   dma_[0].Init(0, memory_, memory_, memory_->GetDMARegister(0), psg_, &signals_);
   dma_[1].Init(1, memory_, memory_, memory_->GetDMARegister(1), psg_, &signals_);
   dma_[2].Init(2, memory_, memory_, memory_->GetDMARegister(2), psg_, &signals_);

   vga_.SetCRTC ( &crtc_ );
   vga_.SetBus ( &address_bus_, &data_bus_ );
   vga_.SetSig ( &signals_ );
   vga_.SetMemory (memory_);
   //vga_.SetScreen ( Display_P );

   asic_.Init(&vga_, &crtc_, log_);

   // Printer
   printer_ = &default_printer_;
   signals_.printer_port_ = printer_ ;


   // FDC ?

   fdc_->SetLog (log_);
   fdc_->Init (&disk_type_manager_);
   fdc_->SetNotifier(notifier_);
   tape_.SetLog (log_);

   sna_handler_.SetMachine ( this );
   tape_.SetVGA (&vga_);
   crtc_.SetLog (log_);

   // PPI et cfg standard
   // Type :
   typedef enum
   {
      ISP = 0x0,
      TRIUMPH = 0x2,
      SAIPHO = 0x4,
      SOLAVOX = 0x6,
      AWA = 0x8,
      SCHNEIDER = 0xA,
      ORION = 0xC,
      AMSTRAD = 0xE,
   } Amstrad_Type;

   ppi_.SetAmstradType (AMSTRAD);
   ppi_.SetPsg(psg_);
   ppi_.SetSig (&signals_);
   ppi_.SetTape ( & tape_ );
   tape_.Init (directories_, &ppi_, configuration_manager_);

   paste_wait_time_ = 3;

   psg_->SetDirectories(directories_);
   psg_->LoadKeyboardMap ( "FRENCH");

   //UpdateComputer ();

   // Expansion
   multiface2_.Init ( &signals_ );

   crtc_.SetCursorLine(cursor_line_);


   // Plug it to the signal
   UpdateExternalDevices();

   if (plus_)
   {
      InitStartOptimizedPlus();
   }
   else
   {
      InitStartOptimized();
   }


   BuildMachine ();
   //motherboard_.InitMotherbard(log_, &sna_handler_, display_, notifier_, directories_, configuration_manager_);

   return true;
}

void EmulatorEngine::UpdateExternalDevices()
{
   signals_.nb_expansion_ = 0;
   /*
   if (current_settings_->GetMultiface2())
   {
      m_Sig.m_ExpList[m_Sig.m_NbExpansionPlugged++] = &multiface2_;
   }
   if (current_settings_->GetPlayCity())
   {
      m_Sig.m_ExpList[m_Sig.m_NbExpansionPlugged++] = play_city;
   }*/
}

bool EmulatorEngine::InitSound ( ISound* sound)
{
   psg_->InitSound (sound);
   sound_mixer_.Init(sound, &tape_);
   sound_player_ = sound;
   return true;
}

ISoundMixer* EmulatorEngine::GetMixer()
{
   return &sound_mixer_;
}


EmulatorEngine::SpeedLimit EmulatorEngine::IsSpeedLimited()
{
   return speed_limit_;
}

// 0 : Not present;  1 : No; 2 : Read; 3 : Write
int EmulatorEngine::IsDiskRunning (int drive )
{
   return fdc_->IsRunning(drive );
}

void EmulatorEngine::InsertBlankDisk ( unsigned int DriveNumber, IDisk::DiskType type )
{
   fdc_->InsertBlankDisk ( DriveNumber, type );
}

void EmulatorEngine::Eject (unsigned int DriveNumber )
{
   if (DriveNumber == 1)
   {
      fd1_path_.clear();
   }
   else if (DriveNumber == 2)
   {
      fd2_path_.clear();
   }

   fdc_->EjectDisk (DriveNumber);
}

void EmulatorEngine::FlipDisk ( unsigned int DriveNumber )
{
   fdc_->FlipDisk (DriveNumber);
}

void EmulatorEngine::ReleaseContainer(DataContainer* container)
{
   media_inserted_.Clear();
}

DataContainer* EmulatorEngine::CanLoad (char* file, std::vector<MediaManager::MediaType>list_of_types)
{
   media_inserted_.Clear ();
   media_inserted_.AddSourceFile (file);
   MediaManager media_mgr (&media_inserted_);

   media_mgr.GetType ( list_of_types );

   return &media_inserted_;
}

int EmulatorEngine::LoadTape(IContainedElement* container )
{
   int ret = tape_.InsertTape(container);
   switch (ret)
   {
   case 0:
      break;
   case -1:
      // TODO
      //MessageBox(NULL, "File not found", "Tape error", MB_OK);
      break;
   case -2:
      // TODO
      //MessageBox(NULL, "Unknown tape type", "Tape error", MB_OK);
      break;
   }
   return ret;
}

int EmulatorEngine::LoadTape ( const char* file_path)
{
   //
   int ret = tape_.InsertTape ( file_path );
   switch (ret)
   {
   case 0:
      break;
   case -1:
      // TODO
      //MessageBox ( NULL, "File not found", "Tape error", MB_OK );
      break;
   case -2:
      // TODO
      //MessageBox ( NULL, "Unknown tape type", "Tape error", MB_OK );
      break;
   }
   return ret;
}

int EmulatorEngine::CompareDriveAandB()
{
   return fdc_->CompareDriveAandB();
}

int EmulatorEngine::LoadCprFromBuffer(unsigned char* buffer, int size)
{

   // Check RIFF chunk
   int index = 0;
   if (  size >= 12
      && (memcmp(&buffer[0], "RIFF", 4) == 0)
      && (memcmp(&buffer[8], "AMS!", 4) == 0)
      )
   {
      // Reinit Cartridge
      memory_->EjectCartridge();

      // Ok, it's correct.
      index += 4;
      // Check the whole size

      int chunk_size = buffer[index]
         + (buffer[index+1] << 8)
         + (buffer[index+2] << 16)
         + (buffer[index+3] << 24);

      index += 8;

      // 'fmt ' chunk ? skip it
      if (index + 8 < size && (memcmp(&buffer[index], "fmt ", 4) == 0))
      {
         index += 8;
      }

      // Good.
      // Now we are at the first cbxx
      while (index + 8 < size)
      {
         if (buffer[index] == 'c' && buffer[index + 1] == 'b')
         {
            index += 2;
            char buffer_block_number[3] = { 0 };
            memcpy(buffer_block_number, &buffer[index], 2);
            int block_number = atoi(buffer_block_number);
            index += 2;

            // Read size
            int block_size = buffer[index]
               + (buffer[index + 1] << 8)
               + (buffer[index + 2] << 16)
               + (buffer[index + 3] << 24);
            index += 4;

            if (block_size <= size && block_number < 256)
            {
               // Copy datas to proper ROM
               unsigned char* rom = memory_->GetCartridge(block_number);
               memset(rom, 0, 0x1000);
               memcpy(rom, &buffer[index], block_size);
               index += block_size;
            }
            else
            {
               return -1;
            }
         }
         else
         {
            return -1;
         }
      }
   }
   else
   {
      // Incorrect headers
      return -1;
   }

   // Insertion ok : Reset to 0
   ResetPlus();

   return 0;
}

int EmulatorEngine::LoadCpr( const char* file_path)
{
   FILE * f;

   if (fopen_s(&f, file_path, "rb") == 0)
   {
      fseek(f, 0, SEEK_END);
      int buffer_size = ftell(f);
      rewind(f);
      unsigned char* buffer = new unsigned char[buffer_size];

      fread(buffer, buffer_size, 1, f);
      fclose(f);
      int ret = LoadCprFromBuffer(buffer, buffer_size);
      delete[]buffer;
      return ret;
   }
   return -1;
}

int EmulatorEngine::LoadCpr(IContainedElement* container)
{
   // todo
   for (int i = 0; i < container->GetNumberOfElements(); i++)
   {
      unsigned char* buffer = container->GetBuffer(i);
      const int size = container->GetSize(i);

      if (LoadCprFromBuffer(buffer, size) == 0)
         return 0;
   }
   return -1;
}

int EmulatorEngine::GetCurrentProgress()
{
   if (fdc_->GetCurrentProgress() != -1)
      return fdc_->GetCurrentProgress ();
   else return tape_.GetCurrentProgress();
}

int EmulatorEngine::LoadMedia(DataContainer* container)
{
   //return fdc_->LoadDisk(DriveNumber, container, bDifferential);

   MediaManager media_mgr(container);
   std::vector<MediaManager::MediaType> list_of_types;
   list_of_types.push_back(MediaManager::MEDIA_DISK);
   list_of_types.push_back(MediaManager::MEDIA_SNA);
   list_of_types.push_back(MediaManager::MEDIA_SNR);
   list_of_types.push_back(MediaManager::MEDIA_TAPE);
   list_of_types.push_back(MediaManager::MEDIA_BIN);
   list_of_types.push_back(MediaManager::MEDIA_CPR);

   int bRet = media_mgr.GetType(list_of_types);

   switch (bRet)
   {
      // Test : Is it SNA?
   case 1:
      //LoadSnapshot(container);
      break;
   case 2:
      // Set ROM : TODO
      break;
   case 3:
      LoadDisk(container, 0);

      break;
      // Tape - TODO
   case 4:
      // TODO : Ask for tape saving ?

      // Load first element of the container
      //m_pMachine->LoadTape(m_DragFiles[0]);
   {
      MediaManager media_mgr(container);
      std::vector<MediaManager::MediaType> list_of_types;
      list_of_types.push_back(MediaManager::MEDIA_TAPE);
      auto list = container->GetFileList();
      LoadTape(list[0]);
      break;
   }
   case 5:
      //LoadSnr(container);
      break;
   case 6:
      //LoadBin(m_DragFiles);
      break;
   case 8:  // CPR : TODO
      {
      MediaManager media_mgr(container);
      std::vector<MediaManager::MediaType> list_of_types;
      list_of_types.push_back(MediaManager::MEDIA_CPR);
      auto list = container->GetFileList();
      LoadCpr (list[0]);
      break;
      }
   }

   return -1;
}
int EmulatorEngine::LoadDisk(DataContainer* container, unsigned int DriveNumber , bool bDifferential )
{
   return fdc_->LoadDisk(DriveNumber, container, bDifferential);
}

int EmulatorEngine::LoadDisk (const char* file_path, unsigned int DriveNumber, bool bDifferential)
{
   if (DriveNumber == 0)
   {
      fd1_path_ = file_path;
   }
   else if (DriveNumber == 1)
   {
      fd2_path_ = file_path;
   }

   int ret = fdc_->LoadDisk ( DriveNumber, file_path, bDifferential );

   switch (ret)
   {
   case 0:
      // Ok
      break;
   case -1:
      // TODO
      //MessageBox ( NULL, _T("File not found"), _T("Disk error"), MB_OK );
      break;
   case -2:
      // TODO
      //MessageBox ( NULL, _T("Unknown disk type"), _T("Disk error"), MB_OK );
      break;
   }

   return ret;
}

void EmulatorEngine::BeginRecord()
{
   sound_mixer_.BeginRecord();
}

void EmulatorEngine::EndRecord ()
{
   sound_mixer_.EndRecord();
}

bool EmulatorEngine::IsRecording ()
{
   return sound_mixer_.IsRecording();
}

void EmulatorEngine::SetDefaultConfiguration ()
{
   speed_limit_ = E_FULL;
   fd1_path_.clear();
   fd2_path_.clear();
}

void EmulatorEngine::SaveConfiguration (const char* config_name, const char* ini_file)
{
   if (configuration_manager_ == nullptr) return;

   // Save configuration
   // Speed limit
   char tmp_buffer[MAX_SIZE_BUFFER];
   switch (speed_limit_)
   {
      case E_FULL:
         configuration_manager_->SetConfiguration(config_name, "LimitSpeed", "F", ini_file );
         break;
      case E_VBL:
         configuration_manager_->SetConfiguration(config_name, "LimitSpeed", "V", ini_file);
         break;
      case E_NONE:
      default:
         configuration_manager_->SetConfiguration(config_name, "LimitSpeed", "N", ini_file);
   }

   // Disk in drive
   configuration_manager_->SetConfiguration(config_name, "FD1_Path", fd1_path_.c_str(), ini_file);
   configuration_manager_->SetConfiguration(config_name, "FD2_Path", fd2_path_.c_str(), ini_file);

   // Configuration
   configuration_manager_->SetConfiguration(config_name, "Machine_Settings", current_settings_->GetFilePath(), ini_file);
   current_settings_->Save();

   // CRTC Type
   sprintf ( tmp_buffer,  "%i", crtc_.type_crtc_);
   configuration_manager_->SetConfiguration(config_name, "CRTC", tmp_buffer, ini_file);


   // Display
   int scanlines = vga_.GetScanlines ( );
   sprintf ( tmp_buffer,  "%i", scanlines);
   configuration_manager_->SetConfiguration(config_name, "SCANLINES", tmp_buffer, ini_file);

   // Speed
   sprintf( tmp_buffer,  "%i", speed_);
   configuration_manager_->SetConfiguration(config_name, "Speed", tmp_buffer, ini_file);

   // Snapshot quick load/save
   configuration_manager_->SetConfiguration(config_name, "QuickSnap", tmp_buffer, ini_file);

   emulator_settings_.Save(ini_file);

   // Other features...

}

void EmulatorEngine::LoadConfiguration (const char* config_name, const char* ini_file)
{
   if (configuration_manager_ == nullptr) return;
   char tmp_buffer [MAX_SIZE_BUFFER ];

   configuration_manager_->GetConfiguration(config_name, "LimitSpeed", "Y", tmp_buffer, MAX_SIZE_BUFFER, ini_file);
   if ( tmp_buffer[0] == 'F') speed_limit_ = E_FULL;
   else if ( tmp_buffer[0] == 'V') speed_limit_ = E_VBL ;
   else speed_limit_ = E_NONE ;

   configuration_manager_->GetConfiguration(config_name, "FD1_Path", "", tmp_buffer, MAX_SIZE_BUFFER, ini_file);
   if ( strlen(tmp_buffer) > 0)
   {
      fd1_path_ = tmp_buffer;
      LoadDisk (fd1_path_.c_str(), 0);
   }
   configuration_manager_->GetConfiguration(config_name, "FD2_Path", "", tmp_buffer, MAX_SIZE_BUFFER, ini_file);
   if ( strlen(tmp_buffer) > 0)
   {
      fd2_path_ = tmp_buffer;
      LoadDisk (fd2_path_.c_str(), 1);
   }

   // Configuration
   configuration_manager_->GetConfiguration(config_name, "Machine_Settings", "CONF\\CPC6128PLUSEN.cfg", tmp_buffer, MAX_SIZE_BUFFER, ini_file);
   
   current_settings_ = MachineSettings::CreateSettings(configuration_manager_, tmp_buffer);
   if (current_settings_ == nullptr)
   {
      current_settings_ = new MachineSettings();
   }

   current_settings_->Load();
   ChangeConfig(current_settings_);

   emulator_settings_.Load(ini_file);
   UpdateFromSettings();

   // CRTC
   /*GetPrivateProfileString (config_name, _T("CRTC"), _T("1"), tmp_buffer, MAX_SIZE_BUFFER , ini_file);
   _stscanf_s (tmp_buffer, _T("%i"), &crtc_.m_TypeCRTC );
   */
   // Display
   int scanlines;
   configuration_manager_->GetConfiguration(config_name, "SCANLINES", "1", tmp_buffer, MAX_SIZE_BUFFER, ini_file);
   sscanf (tmp_buffer, "%i", &scanlines );
   vga_.SetScanlines(scanlines);

   // Speed
   configuration_manager_->GetConfiguration(config_name, "Speed", "100", tmp_buffer, MAX_SIZE_BUFFER, ini_file);
   sscanf (tmp_buffer, "%i", &speed_ );

   // Snapshot quick load/save
   configuration_manager_->GetConfiguration(config_name, "QuickSnap", "", tmp_buffer, MAX_SIZE_BUFFER, ini_file);
   quick_sna_path_ = tmp_buffer;
   quick_sna_available_ = quick_sna_path_.size()>0;

   SetSpeed(speed_);
}

///////////////////////////////////////////////////
// Demarrage et boucle principale
///////////////////////////////////////////////////

void EmulatorEngine::Reinit ()
{
   //UpdateComputer ();
   // todo : update computer
   OnOff ();
}

void EmulatorEngine::OnOff ()
{
   multiface_stop_ = false;

   asic_.HardReset();

   GetMem()->Initialisation ();
   GetMem()->SetRmr2(0);

   UpdateComputer(true);
   // todo : update computer

   psg_->Reset ();
   signals_.Reset ();

   display_->Reset();
   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      signals_.exp_list_[i]->Reset();
   }

   time_elapsed_ = std::chrono::steady_clock::now();
   time_computed_ = 0;

   paste_available_ = false;
}

void EmulatorEngine::ResetPlus()
{
   // todo
 /*  multiface_stop_ = false;

   GetMem()->Initialisation();
   psg_->Reset();

   m_Sig.Reset();

   display_->Reset();
   for (int i = 0; i < m_Sig.m_NbExpansionPlugged; i++)
   {
      m_Sig.m_ExpList[i]->Reset();
   }

   TimeElapsed = 0;
   paste_available_ = false;*/
   OnOff();

}

void EmulatorEngine::Reset ()
{
   GetMem()->Initialisation ();
   psg_->Reset ();

   signals_.Reset ();

   display_->Reset();

   time_elapsed_ = std::chrono::steady_clock::now();
   time_computed_ = 0;

   paste_available_ = false;
}




void EmulatorEngine::SetBreakpointHandler ( IBreakpoint* bp )
{
   generic_breakpoint_ = bp;
}


void EmulatorEngine::AddBreakpoint ( unsigned short addr)
{
   // Add breakpoint to list
   if ( breakpoint_index_ < NB_BP_MAX )
   {
      breakpoint_list_ [breakpoint_index_++] = addr;
   }
}

void EmulatorEngine::ChangeBreakpoint ( unsigned short  oldBP, unsigned short newBP )
{
   for (unsigned int i = 0; i < breakpoint_index_; i++)
   {
      if ( breakpoint_list_ [i] == oldBP)
      {
         breakpoint_list_ [i] = newBP;
         return;
      }
   }
}

void EmulatorEngine::RemoveBreakpoint ( unsigned short addr)
{
   for (unsigned int i = 0; i < breakpoint_index_; i++)
   {
      if ( breakpoint_list_ [i] == addr)
      {
         breakpoint_list_ [i] = 0;
         for (unsigned int j = i; j < breakpoint_index_-1; j++)
         {
            breakpoint_list_ [j]  = breakpoint_list_ [j+1];
         }
         breakpoint_index_--;
         return;
      }
   }
}

void EmulatorEngine::CleanBreakpoints ()
{
   breakpoint_index_ = 0;
}

void EmulatorEngine::SetSpeed ( int speedLimit )
{
   if ( speedLimit == -1)
   {
      display_->SetSyncWithVbl (-1);
   }
   else
   {
      display_->SetSyncWithVbl (speedLimit / 2);
   }
   speed_ = speedLimit;
}

#define TARGET_RESOLUTION 1         // 1-millisecond target resolution

int EmulatorEngine::RunTimeSlice (bool bNotDbg )
{
   //unsigned long long TimeElapsedEnd;
   std::chrono::time_point<std::chrono::steady_clock> time_elapsed_end;
   int ret = 0;

   // Begin ?
   /*if (TimeElapsed == 0)
   {
      TimeElapsed = std::chrono::steady_clock::now();

         //std::chrono::time_point<std::chrono::steady_clock>::now ()
      TimeComputed = 0;
   }*/

   // Anything to load ?
   // SNR
   // BIN
   if (bin_to_load_)
   {
      bin_to_load_ = false;
      LoadBinInt (bin_to_load_path_.c_str());
   }

   // Expansion action ?
   if ( multiface_stop_ )
   {
      multiface_stop_ = false;
      // Multiface 2 action
      multiface2_.Stop ();
   }

   // Anything to paste ? - Only after keyboard and basic is up to date....
   if ( (paste_size_ > 0) && (paste_available_) && (paste_wait_time_ == 0) && (paste_vkey_ == NULL))
   {
      // Press the key
      paste_vkey_ = paste_buffer_[paste_count_];
      if (paste_vkey_ != NULL)
      {
         psg_->CharPressed(paste_vkey_);
         paste_wait_time_ = 160000;
      }
      else
      {
         --paste_size_;
         ++paste_count_;
      }
   }

   int run_time = 0;
   // 1ms is 4 000 tick
   if(bNotDbg)
   {
      run_ = true;

      run_time = time_slice_ * 4000;
      if (plus_)
         StartOptimizedPlus(run_time);
      else
      {
         /*unsigned int next_cycle = runTime;

         component_list* element = head_component_;
         while (element != nullptr)
         {
            if (element->elapsed_time < next_cycle)
               next_cycle = element->elapsed_time;
            element = element->next;
         }

         while (next_cycle < runTime)
         {
            component_list* element = head_component_;
            while (element != nullptr)
            {
               if (element->elapsed_time == next_cycle)
               {
                  element->elapsed_time += element->component->Tick();
               }
               element = element->next;
            }

            // Propagate SIG
            m_Sig.Propagate();

            ++next_cycle;
         }

         element = head_component_;
         while (element != nullptr)
         {
            element->elapsed_time -= next_cycle;
            element = element->next;
         }
         */
         StartOptimized(run_time);
      }
         

      //StartPrecise ( TimeSlice * 4000);

   }
      //Debug ( TimeSlice * 500);
   else
   {

      run_time = DebugNew(time_slice_ * 4000);

      if (!run_)
      {
         display_->VSync(true);
         ret = 1;
      }
   }

   if (do_snapshot_)
   {
      if (z80_.machine_cycle_ == Z80::M_FETCH && z80_.t_ == 1)
      {
         z80_.stop_on_fetch_ = false;
         if (sna_handler_.SaveSnapshot(snapshot_file_.c_str()))
         {
            // We have to stop on :
            // - FETCH, first T cycle.
            quick_sna_available_ = true;
            quick_sna_path_ = snapshot_file_;
            do_snapshot_ = false;
         }
      }
   }

   if (sna_to_load_)
   {
      LoadSnapshotDelayed();
      sna_to_load_ = false;
   }

   if (( paste_size_ > 0) && (paste_available_) && (paste_wait_time_ == 0) && (paste_vkey_ != NULL))
   {
      // Release the key, and move on
      psg_->CharReleased(paste_buffer_[paste_count_++]);
      paste_vkey_ = NULL;
      --paste_size_;
      paste_wait_time_ = 200000;
   }



   // Increase total time
   time_computed_ += run_time / 4000;

   // Wait if needed
   //TimeElapsedEnd = GetTickCount ();
   time_elapsed_end = std::chrono::steady_clock::now();

   std::chrono::milliseconds real_time = std::chrono::duration_cast<std::chrono::milliseconds> (time_elapsed_end - time_elapsed_);
   std::chrono::milliseconds base_realtime = real_time;
   //unsigned long long real_time = time_elapsed_end - TimeElapsed;
   //unsigned long long baserealTime_L = real_time ;

   if ( !display_->IsWaitHandled ())
   {
      if ( speed_ != 0)
      {
         real_time = real_time * speed_/ 100;
         speed_limit_=E_FULL;
      }
      else
      {
         speed_limit_ = E_NONE;
      }

      if (std::chrono::milliseconds(time_computed_) > real_time)
      {
         // Depends on the type of wait :
         // based on sound
         /*if (speed_limit_ == E_Full && speed_ == 100)
         {
            //sound_player_->SyncWithSound();
         }
         else*/
         // Standard
         if (speed_limit_==E_FULL && (std::chrono::milliseconds(time_computed_) - real_time) > std::chrono::milliseconds(10))
         {
            std::this_thread::sleep_for(std::chrono::microseconds(( std::chrono::milliseconds(time_computed_) - real_time)));
            //Sleep( (DWORD)(TimeComputed-real_time) );
            //Speed = 100;
            if (real_time != std::chrono::milliseconds(0))
            {
               speed_percent_ = (unsigned int)(time_computed_* 100  / base_realtime.count());
            }
         }
         else
         {
            /*
            if (speed_limit_==E_Vbl )
            {
               display_->WaitVbl ();
            }

            */
            if (base_realtime.count() != 0)
            {
               speed_percent_ = (unsigned int)(time_computed_* 100  / base_realtime.count());
            }
         }
      }
      else
      {
         if (base_realtime.count() != 0)
         {
            speed_percent_ = (unsigned int)(time_computed_* 100  / base_realtime.count());
         }
      }
   }
   else
   {
      if (base_realtime.count() != 0)
      {
         speed_percent_ = (unsigned int)(time_computed_* 100  / base_realtime.count());
      }
      else
      {
         speed_percent_ = 1000;
      }
   }
   /*if (TimeComputed > 1000)
   {
      paste_available_ = true;
   }
   */
   // Renew the total elapsed time (every 5 seconds)
   if ( paste_wait_time_ > 0)
   {

      paste_wait_time_ -= run_time;
      if (paste_wait_time_ < 0)  paste_wait_time_ = 0;
   }

   if (time_computed_ > 2000)
   {
      //--paste_wait_time_;
      paste_available_ = true;
      //TimeElapsed = 0;
      time_elapsed_ = std::chrono::steady_clock::now();
      time_computed_ = 0;


      /*if (paste_wait_time_ > 1000)
         paste_wait_time_-=1000;*/
   }
   return ret;
}

void EmulatorEngine::InitStartOptimized()
{
   // Set the components call file
   // Get from settings, all components
   // Place them in the file
   memset(component_elapsed_time_, 0, sizeof(component_elapsed_time_));
   nb_components_ = 0;

   // PSG
   component_elapsed_time_[nb_components_] = 15;
   component_list_[nb_components_] = psg_;
   component_list_[nb_components_]->this_tick_time_ = 15;
   nb_components_++;

   // VDU
   /*if ( display_->IsDisplayed ())
   {
   m_ElapsedTimeForcomponents[m_NbComponent ] = 0;
   m_ListOfComponents[m_NbComponent ] = &monitor_;
   m_ListOfComponents[m_NbComponent ]->ThisTickTime = 1;
   m_NbComponent ++;
   }
   else
   {
   m_ElapsedTimeForcomponents[m_NbComponent ] = -1;
   m_ListOfComponents[m_NbComponent ] = NULL;
   //m_ListOfComponents[m_NbComponent ]->ThisTickTime = 1;
   m_NbComponent ++;
   }*/

   // CPU : Z80
   z80_index_ = nb_components_;
   component_elapsed_time_[nb_components_] = 0;
   component_list_[nb_components_] = &z80_;
   component_list_[nb_components_]->this_tick_time_ = 0;
   component_list_[nb_components_]->elapsed_time_ = component_list_[nb_components_]->this_tick_time_;
   nb_components_++;
   // Tape ?
   // m_ListOfComponents[m_NbComponent ].ElapsedTime = m_ListOfComponents[m_NbComponent ].ThisTickTime;
   component_elapsed_time_[nb_components_] = 1;
   component_list_[nb_components_] = &tape_;
   component_list_[nb_components_]->this_tick_time_ = 1;
   nb_components_++;


   // CRTC
   //m_ListOfComponents[m_NbComponent ].ElapsedTime = m_ListOfComponents[m_NbComponent ].ThisTickTime;
   component_elapsed_time_[nb_components_] = 11;
   component_list_[nb_components_] = &crtc_;
   component_list_[nb_components_]->this_tick_time_ = 11;
   nb_components_++;

   // Gate Array
   // m_ListOfComponents[m_NbComponent ].ElapsedTime = m_ListOfComponents[m_NbComponent ].ThisTickTime;
   /*m_ElapsedTimeForcomponents[m_NbComponent ] = 2;
   m_ListOfComponents[m_NbComponent ] = &vga_;
   m_ListOfComponents[m_NbComponent ]->ThisTickTime = 2;

   m_NbComponent ++;*/


   // FDC ?
   component_elapsed_time_[nb_components_] = 8;
   component_list_[nb_components_] = fdc_;
   component_list_[nb_components_]->this_tick_time_ = 8;



   // SET sur 6128 ou si DDI ( dependant de la présenc du FDC, donc ?)
   ppi_.SetExpSignal(true);

   nb_components_++;

   for (int j = 0; j <nb_components_; j++)
   {
      if (component_list_[j] != NULL)
      {
         component_list_[j]->SetTickForcer(this);
      }
   }

   //





   // Create dynamic function :
   // - https://blogs.oracle.com/nike/entry/simple_jit_compiler_for_your
   // 1- Compute how many cycle we want to do in a single loop :
   //     - It has to be a multiple of the PPCM of the whole components - call this X time
   // 2 - Create the code
   /*if (generated_loop_.y != NULL)
   {
      VirtualFreeEx(GetCurrentProcess(), generated_loop_.y, 0, MEM_RELEASE);
   }

   byte* buf = (byte*)VirtualAllocEx(GetCurrentProcess(), 0, 1 << 16, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

   if (buf == 0) return;

   byte* p = buf;


   *p++ = 0xC3; // ret


   generated_loop_.y = buf;
   */
}

void EmulatorEngine::InitStartOptimizedPlus ()
{
   // Set the components call file
   // Get from settings, all components
   // Place them in the file
   memset(component_elapsed_time_, 0, sizeof(component_elapsed_time_));
   nb_components_ = 0;

   // PSG
   component_elapsed_time_[nb_components_ ] = 15;
   component_list_[nb_components_ ] = psg_;
   component_list_[nb_components_ ]->this_tick_time_ = 15;
   nb_components_ ++;


   // CPU : Z80
//#define Z80_COMP 2
   z80_index_ = nb_components_;
   component_elapsed_time_[nb_components_ ] = 0;
   component_list_[nb_components_ ] = &z80_;
   component_list_[nb_components_ ]->this_tick_time_ = 0;
   component_list_[nb_components_ ]->elapsed_time_ = component_list_[nb_components_ ]->this_tick_time_;
   nb_components_ ++;
   // Tape ?
  // m_ListOfComponents[m_NbComponent ].ElapsedTime = m_ListOfComponents[m_NbComponent ].ThisTickTime;
   component_elapsed_time_[nb_components_ ] = 1;
   component_list_[nb_components_ ] = &tape_;
   component_list_[nb_components_ ]->this_tick_time_ = 1;
   nb_components_ ++;

   // FDC ?
   component_elapsed_time_[nb_components_ ] = 8;
   component_list_[nb_components_ ] = fdc_;
   component_list_[nb_components_ ]->this_tick_time_ = 8;


   // ASIC
   //m_ListOfComponents[m_NbComponent ].ElapsedTime = m_ListOfComponents[m_NbComponent ].ThisTickTime;
   component_elapsed_time_[nb_components_] = 11;
   component_list_[nb_components_] = &asic_;
   component_list_[nb_components_]->this_tick_time_ = 11;
   nb_components_++;

   // SET sur 6128 ou si DDI ( dependant de la présenc du FDC, donc ?)
   ppi_.SetExpSignal ( true );

   for (int j = 0; j <nb_components_ ; j++)
   {
      if ( component_list_[j] != NULL)
      {
         component_list_[j]->SetTickForcer (this );
      }
   }

   //


   // Create dynamic function :
   // - https://blogs.oracle.com/nike/entry/simple_jit_compiler_for_your
   // 1- Compute how many cycle we want to do in a single loop :
   //     - It has to be a multiple of the PPCM of the whole components - call this X time
   // 2 - Create the code
   /*if (generated_loop_.y != NULL)
   {
      VirtualFreeEx(GetCurrentProcess(), generated_loop_.y, 0, MEM_RELEASE);
   }

   byte* buf = (byte*)VirtualAllocEx(GetCurrentProcess(), 0, 1 << 16, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

   if (buf == 0) return;

   byte* p = buf;


   *p++ = 0xC3; // ret


   generated_loop_.y = buf;*/
}


void EmulatorEngine::Resync ()
{
   // Set the components call file
   // Get from settings, all components
   // Place them in the file

   // PSG
   ForceTick ( psg_, 0) ;

   // VDU
   ForceTick ( &z80_, 0) ;

   // CRTC§Asic
   if (plus_)
   {
      ForceTick(&asic_, 11);

      ForceTick(&dma_[0], 11);
      ForceTick(&dma_[1], 11);
      ForceTick(&dma_[2], 11);
   }
   else
   {
      ForceTick(&crtc_, 8);

   }

   // Tape ?
   ForceTick ( &tape_, 1) ;

   // FDC ?
   ForceTick(fdc_, 8);

   // Gate Array
   ForceTick ( &vga_, 2) ;

}


void EmulatorEngine::ForceTick (IComponent* component, int ticks)
{
   // Look for this item
   for (int i = 0; i < nb_components_; i++)
   {
      if ( component_list_[i] == component)
      {
         // Found ? Just set the "current" value to 0
         if (ticks == 0)
         {
            //m_ListOfComponents[i].ThisTickTime -= (m_ListOfComponents[i].ElapsedTime-1);
            component_list_[i]->this_tick_time_ -= (component_elapsed_time_[i]-1);
            //m_ListOfComponents[i].ElapsedTime = 1;
            component_elapsed_time_[i] = 1;
         }
         else
         {
            component_list_[i]->this_tick_time_ = ticks;
            //m_ListOfComponents[i].ElapsedTime = ticks;
            component_elapsed_time_[i] = ticks;
         }

         return;
      }
   }

}

//#define  RUN_COMPOSANT_N(i) if (m_ElapsedTimeForcomponents[i] == value ) /*if (m_ListOfComponents[i] != NULL )*/ m_ElapsedTimeForcomponents[i] += m_ListOfComponents[i]->Tick ();\
//               if ( m_ElapsedTimeForcomponents[i] < next_cycle){next_cycle = m_ElapsedTimeForcomponents[i];}
#define  RUN_COMPOSANT_N(c,v) if (v <= next_cycle ) /*if (m_ListOfComponents[i] != NULL )*/ v += c.Tick ();
               //if ( m_ElapsedTimeForcomponents[i] < next_cycle){next_cycle = m_ElapsedTimeForcomponents[i];}

// New

void EmulatorEngine::StartOptimizedPlus(unsigned int nb_cycles)
{
   unsigned int next_cycle = nb_cycles;
   unsigned int index = 0;
   unsigned int elapsed_time_psg = component_elapsed_time_[index++];

   unsigned int elapsed_time_z80 = component_elapsed_time_[index++];
   unsigned int elapsed_time_tape = component_elapsed_time_[index++];
   unsigned int elapsed_time_fdc = component_elapsed_time_[index++];
   //int elapsedTimeMixer = m_ElapsedTimeForcomponents[index++];
   //unsigned int elapsedTimeMixerHub = m_ElapsedTimeForcomponents[index++];
   unsigned int elapsed_time_asic = component_elapsed_time_[index++];
   unsigned int elapsed_time_dma = component_elapsed_time_[index++];

   unsigned int elapsed_components[16];
   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      elapsed_components[i] = component_elapsed_time_[index++];
   }

   unsigned int* elapsed = component_elapsed_time_;
   for (unsigned int i = 0; i < index; ++i)
   {
      if (*elapsed < next_cycle)
      {
         next_cycle = *elapsed;
      }
      ++elapsed;
   }

   //if (generated_loop_.y != NULL)
   {
      //generated_loop_.x(NbCycles, next_cycle);

      while (next_cycle < nb_cycles)
      {
         if (elapsed_time_psg == next_cycle)
         {
            elapsed_time_psg += (*psg_).Tick();
         }
         RUN_COMPOSANT_N(tape_, elapsed_time_tape);
         RUN_COMPOSANT_N(asic_, elapsed_time_asic);
         RUN_COMPOSANT_N((*fdc_), elapsed_time_fdc);
         RUN_COMPOSANT_N((z80_), elapsed_time_z80);
         //RUN_COMPOSANT_N(sound_mixer_, elapsedTimeMixerHub);
         RUN_COMPOSANT_N(dma_[0], elapsed_time_dma);
         RUN_COMPOSANT_N(dma_[1], elapsed_time_dma);
         RUN_COMPOSANT_N(dma_[2], elapsed_time_dma);

         for (int i = 0; i < signals_.nb_expansion_; i++)
         {
            if (elapsed_components[i] <= next_cycle) /*if (m_ListOfComponents[i] != NULL )*/ elapsed_components[i] += signals_.exp_list_[i]->Tick();
         }

         // Propagate SIG
         signals_.Propagate();
         ++next_cycle;
      }
      index = 0;
      component_elapsed_time_[index++] = elapsed_time_psg - nb_cycles;
      component_elapsed_time_[index++] = elapsed_time_z80 - nb_cycles;
      component_elapsed_time_[index++] = elapsed_time_tape - nb_cycles;
      component_elapsed_time_[index++] = elapsed_time_fdc - nb_cycles;
      //m_ElapsedTimeForcomponents[index++] = elapsedTimeMixer - NbCycles;
      //m_ElapsedTimeForcomponents[index++] = elapsedTimeMixerHub - NbCycles;
      component_elapsed_time_[index++] = elapsed_time_asic - nb_cycles;
      component_elapsed_time_[index++] = elapsed_time_dma - nb_cycles;

      for (int i = 0; i < signals_.nb_expansion_; i++)
      {
         component_elapsed_time_[index++] = elapsed_components[i] - nb_cycles;
      }



   }
/*   else
   {
      while (next_cycle < NbCycles)
      {
         for (int i = 0; i < m_NbComponent; ++i)
         {
            //if ( *elapsed == value )
            if (m_ElapsedTimeForcomponents[i] == next_cycle)
            {
               // Update time
               m_ElapsedTimeForcomponents[i] += m_ListOfComponents[i]->Tick();
            }
         }

         // Propagate SIG
         m_Sig.Propagate();

         ++next_cycle;
      }

      // No more
      for (int i = 0; i < m_NbComponent; ++i)
      {
         m_ElapsedTimeForcomponents[i] -= NbCycles;
      }
   }*/
}

void EmulatorEngine::StartOptimized(unsigned int NbCycles)
{

   //
   //int i;
   //
   unsigned int next_cycle = NbCycles;
   unsigned int index = 0;
   unsigned int elapsed_time_psg = component_elapsed_time_[index++];
   //unsigned int elapsedTimeMon = m_ElapsedTimeForcomponents[1];
   unsigned int elapsed_time_z80 = component_elapsed_time_[index++];
   unsigned int elapsed_time_tape = component_elapsed_time_[index++];
   unsigned int elapsed_time_crtc = component_elapsed_time_[index++];
   //unsigned int elapsedTimeVGA = m_ElapsedTimeForcomponents[5];
   unsigned int elapsed_time_fdc = component_elapsed_time_[index++];
   //unsigned int elapsedTimeMixer = m_ElapsedTimeForcomponents[index++];
   //unsigned int elapsedTimeMixerHub = m_ElapsedTimeForcomponents[index++];

   unsigned int elapsed_components[16];
   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      elapsed_components[i] = component_elapsed_time_[index++];
   }

   unsigned int* elapsed = component_elapsed_time_;
   for (unsigned int i = 0 ; i < index; ++i )
   {
      if ( *elapsed < next_cycle)
      {
         next_cycle = *elapsed;
      }
      ++elapsed;
   }

   //if (generated_loop_.y != NULL)
   {
      //generated_loop_.x(NbCycles, next_cycle);
      
      while (next_cycle < NbCycles)
      {
         /*
         component_list* element = head_component_;
         while (element != nullptr)
         {
            if (element->elapsed_time == next_cycle)
            {
               element->elapsed_time += element->component->Tick();
            }
            element = element->next;
         }*/

         
         if (elapsed_time_psg == next_cycle)
         {
            elapsed_time_psg += (*psg_).Tick();              // 32us, placed
         }
         RUN_COMPOSANT_N(tape_, elapsed_time_tape);          // Depends on next flux reversal.... 
         RUN_COMPOSANT_N(crtc_, elapsed_time_crtc);          // 4us
         RUN_COMPOSANT_N((*fdc_), elapsed_time_fdc);         // 8 us
         RUN_COMPOSANT_N((z80_), elapsed_time_z80);      // Depends on instructions timings....
         //RUN_COMPOSANT_N(sound_mixer_, elapsedTimeMixerHub);// 32 us 


         // ASIC ?

         for (int i = 0; i < signals_.nb_expansion_; i++)
         {
            if (elapsed_components[i] <= next_cycle) elapsed_components[i] += signals_.exp_list_[i]->Tick();
         }

         
         // Propagate SIG
         signals_.Propagate();

         ++next_cycle;
      }
   /*
      component_list* element = head_component_;
      while (element != nullptr)
      {
         element->elapsed_time -= next_cycle;
         element = element->next;
      }*/
      
      index = 0;
      component_elapsed_time_[index++] = elapsed_time_psg - NbCycles;
      component_elapsed_time_[index++] = elapsed_time_z80 - NbCycles;
      component_elapsed_time_[index++] = elapsed_time_tape - NbCycles;
      component_elapsed_time_[index++] = elapsed_time_crtc - NbCycles;
      component_elapsed_time_[index++] = elapsed_time_fdc - NbCycles;
      //m_ElapsedTimeForcomponents[index++] = elapsedTimeMixer - NbCycles;
      //m_ElapsedTimeForcomponents[index++] = elapsedTimeMixerHub - NbCycles;
      for (int i = 0; i < signals_.nb_expansion_; i++)
      {
         component_elapsed_time_[index++] = elapsed_components[i] - NbCycles;
      }
      


   }
   /*else
   {
      while (next_cycle < NbCycles)
      {

         for (int i = 0; i < m_NbComponent; ++i)
         {
            //if ( *elapsed == value )
            if (m_ElapsedTimeForcomponents[i] == next_cycle)
            {
               // Update time
               m_ElapsedTimeForcomponents[i] += m_ListOfComponents[i]->Tick();
            }
         }

         z80_.PreciseTick();
         ++next_cycle;
      }

         // No more
      for (int i = 0; i < m_NbComponent; ++i)
      {
         m_ElapsedTimeForcomponents[i] -= NbCycles;
      }
   }*/
}


int EmulatorEngine::DebugNew(unsigned int nb_cycles)
{

   //
   //int i;
   //
   unsigned int next_cycle = nb_cycles;
   unsigned int index = 0;
   unsigned int elapsed_time_psg = component_elapsed_time_[index++];
   //unsigned int elapsedTimeMon = m_ElapsedTimeForcomponents[1];
   unsigned int elapsed_time_z80 = component_elapsed_time_[index++];
   unsigned int elapsed_time_tape = component_elapsed_time_[index++];
   unsigned int elapsed_time_crtc = component_elapsed_time_[index++];
   //unsigned int elapsedTimeVGA = m_ElapsedTimeForcomponents[5];
   unsigned int elapsed_time_fdc = component_elapsed_time_[index++];
   //unsigned int elapsedTimeMixer = m_ElapsedTimeForcomponents[index++];
   //unsigned int elapsedTimeMixerHub = m_ElapsedTimeForcomponents[index++];

   unsigned int elapsed_components[16];
   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      elapsed_components[i] = component_elapsed_time_[index++];
   }

   unsigned int* elapsed = component_elapsed_time_;
   for (unsigned int i = 0; i < index; ++i)
   {
      if (*elapsed < next_cycle)
      {
         next_cycle = *elapsed;
      }
      ++elapsed;
   }


   unsigned short old_pc = z80_.GetPC();

   // Verifier que c'est un call, sinon, comme stepin
   if (step_)
   {
      // Compute next PC to stop
      if (z80_.IsCallInstruction(z80_.GetPC()))
      {
         stop_pc_ = z80_.GetPC() + z80_.GetOpcodeSize(z80_.GetPC());
      }
      else
      {
         if (!remember_step_)
         {
            // Step in !
            step_ = false;
            step_in_ = true;
         }
      }
   }
   run_ = true;

   /*int* elapsed = m_ElapsedTimeForcomponents;
   for (int i = 0; i < m_NbComponent; ++i)
   {
      if (*elapsed < next_cycle)
      {
         next_cycle = *elapsed;
      }
      ++elapsed;
   }
   */
   next_cycle = 0;

   int old_counter = next_cycle;

   while (run_ && next_cycle < nb_cycles)
   {
      /*
      component_list* element = head_component_;
      while (element != nullptr)
      {
         if (element->elapsed_time == next_cycle)
         {
            element->elapsed_time += element->component->Tick();
         }        
         element = element->next;
      }*/

      if (elapsed_time_psg == next_cycle)
      {
         elapsed_time_psg += (*psg_).Tick();
      }
      RUN_COMPOSANT_N(tape_, elapsed_time_tape);
      RUN_COMPOSANT_N(crtc_, elapsed_time_crtc);
      RUN_COMPOSANT_N((*fdc_), elapsed_time_fdc);
      RUN_COMPOSANT_N((z80_), elapsed_time_z80);
      //RUN_COMPOSANT_N(sound_mixer_, elapsedTimeMixerHub);

      for (int i = 0; i < signals_.nb_expansion_; i++)
      {
         if (elapsed_components[i] <= next_cycle) elapsed_components[i] += signals_.exp_list_[i]->Tick();
      }

      signals_.Propagate();
      ++next_cycle;

      // Now, apply all changes on the buses
      //m_Sig.Propagate ();

      //if (endZ80instr-1 <= value)
      if (  z80_.t_ == 1 &&
            (  z80_.machine_cycle_ == Z80::M_M1_NMI
            || z80_.machine_cycle_ == Z80::M_M1_INT
            )
         || (z80_.machine_cycle_ == Z80::M_FETCH && z80_.t_ == 4 && ((z80_.current_opcode_&0xFF00) == 0) && elapsed_time_z80 == next_cycle)
         )
      {
         counter_ += (component_elapsed_time_[z80_index_] - old_counter + 1);
         old_counter = component_elapsed_time_[z80_index_];
         {
            old_pc = z80_.GetPC();
            if (generic_breakpoint_)
            {
               run_ = generic_breakpoint_->IsBreak() ? false : true;
               memory_->ResetStockAddress();
            }
            for (unsigned int i = 0; i < breakpoint_index_; i++)
            {
               if (breakpoint_list_[i] == z80_.GetPC())
               {
                  // Break !
                  run_ = false;
               }
            }
            if (step_)
            {
               if (z80_.GetPC() == stop_pc_)
               {
                  step_ = false;
                  run_ = false;
                  remember_step_ = false;
               }
            }
            if (step_in_)
            {
               step_in_ = false;
               run_ = false;
            }
         }
      }
   }
   /*
   component_list* element = head_component_;
   while (element != nullptr)
   {
      element->elapsed_time -= next_cycle;
      element = element->next;
   }*/

   index = 0;
   component_elapsed_time_[index++] = elapsed_time_psg - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_z80 - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_tape - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_crtc - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_fdc - next_cycle;
   //m_ElapsedTimeForcomponents[index++] = elapsedTimeMixer - next_cycle;
   //m_ElapsedTimeForcomponents[index++] = elapsedTimeMixerHub - next_cycle;

   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      component_elapsed_time_[index++] = elapsed_components[i] - next_cycle;
   }

   if (run_ &&  step_)
   {
      remember_step_ = true;
   }

   if (supervisor_ != NULL)
   {
      if (!run_) supervisor_->EmulationStopped();
      if (counter_ >= 4000000)
      {
         supervisor_->SetEfficience(GetSpeed());
         supervisor_->RefreshRunningData();
         // Refresh ?
      }
   }
   return next_cycle;
}

void EmulatorEngine::Paste (const char* command)
{
   // Copy the remaining buffer to begining of buffer
   char tmp_buffer[4096];

   strncpy ( tmp_buffer, &paste_buffer_[paste_count_], paste_size_);

   memcpy (paste_buffer_, tmp_buffer, paste_size_);
   paste_count_ = 0;

   const unsigned int paste_command = strlen(command);
   for (unsigned int i = 0; i < paste_command && paste_size_ < 4096; i++)
   {
      if ( command[i] != '\0')
         paste_buffer_[paste_size_++] = command[i];
   }

}

void EmulatorEngine::SetPlus(bool plus)
{
   plus_ = plus;

   // Set component to "plus"
   signals_.SetPlus(plus_);
   ppi_.SetPlus(plus_);
   vga_.SetPlus(plus_);
   memory_->SetPlus(plus_);
}

void EmulatorEngine::ChangeConfig(MachineSettings* current_settings)
{
   // todo : update computer
   current_settings_ = current_settings;
   UpdateComputer();

   // Build list of components to tick :
   RebuildComponents();
}

void EmulatorEngine::ClearComponents()
{
   while (head_component_ != nullptr)
   {
      ComponentList* next = head_component_->next;
      delete head_component_;
      head_component_ = next;
   }
   head_component_ = nullptr;

}
void EmulatorEngine::RebuildComponents()
{
   // clear existing
   ClearComponents();

   // Creat new list
   head_component_ = new ComponentList;
   ComponentList* element = head_component_;

   element->component = psg_;
   element->elapsed_time = 15;
   element->next = nullptr;

   // Tape ? 
   if (true) // todo
   {
      element = element->QueueComponent(&tape_, 1);
   }

   element = element->QueueComponent(&crtc_, 11);
   if (true)
   {
      element = element->QueueComponent(fdc_, 8);
   }
   element = element->QueueComponent(&z80_, 0);
   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      // Do something !
      //element = element->QueueComponent(m_Sig.m_ExpList[i]);
   }

}
bool EmulatorEngine::IsQuickSnapAvailable ()
{
   return quick_sna_available_;
}

bool EmulatorEngine::LoadSnapshot(const char* path_file)
{
   sna_path_to_load_ = path_file;
   sna_to_load_ = true;
   return true;
}

bool EmulatorEngine::LoadSnapshotDelayed()
{
   if ( sna_handler_.LoadSnapshot (sna_path_to_load_.c_str()))
   {
      quick_sna_available_ = true;
      quick_sna_path_ = sna_path_to_load_;
      monitor_.RecomputeAllColors ();

      return true;
   }
   else
   {
      return false;
   }
}

bool EmulatorEngine::SaveSnapshot (const char* path_file)
{
   do_snapshot_ = true;
   snapshot_file_ = path_file;
   z80_.stop_on_fetch_ = true;
   return true;
}


bool EmulatorEngine::QuickLoadsnapshot ()
{
   if (quick_sna_available_)
   {
      if ( LoadSnapshot (quick_sna_path_.c_str()) )
      {
         return true;
      }
      else
      {
         quick_sna_available_ = false;
      }
   }
   return false;
}

bool EmulatorEngine::QuickSavesnapshot ()
{
   if (quick_sna_available_)
   {
      if ( SaveSnapshot (quick_sna_path_.c_str()) )
      {
         return true;
      }
      else
      {
         quick_sna_available_ = false;
      }
   }
   return false;
}

void EmulatorEngine::SaveDisk (int drive)
{
   fdc_->WriteDisk ( drive );
}

void EmulatorEngine::SaveDiskAs (int drive, char* file_path, FormatType* format_type)
{
   if (IsDiskPresent ( drive ))
   {
      fdc_->WriteDiskAs ( drive, file_path, format_type);
   }
}

bool EmulatorEngine::IsDiskPresent ( int drive )
{
   return fdc_->IsDiskPresent ( drive );
}

void EmulatorEngine::Accelerate ( bool speed_on)
{
   // TODO : Set it with a switch it can be disabled.
   /*if ( m_bFastTapeLoading )
      monitor_.Accelerate ( speed_on );*/
}

void EmulatorEngine::BuildMachine ()
{
   // TODO : Select proper machine
   BuildMachine6128 ();
}

// 464
void EmulatorEngine::BuildMachine464 ()
{
}

// 664
void EmulatorEngine::BuildMachine664 ()
{
}

// 6128
void EmulatorEngine::BuildMachine6128 ()
{
   // Clk 16 Mhz : Gate Array

   // 4 Mhz line from Gate array : Z80
   /*vga_.clock_4mhz_.AddComponent ( &z80_ );
   vga_.clock_4mhz_.AddComponent ( fdc_ );

   // 1 Mhz line
   vga_.clock_1mhz_.AddComponent ( &crtc_ );
   vga_.clock_1mhz_.AddComponent ( psg_ );*/
   // PAL  : TODO


   // This one is linked to the Gate Array
}
void EmulatorEngine::BuildMachineCustom ()
{
   // Custom
   // TODO
}

void EmulatorEngine::StartPrecise(unsigned int NbCycles)
{
   // Run precise cycle through the clock generator line (16 Mhz)
   for (unsigned int i = 0; i < NbCycles; i++)
   {
      // Gate array is the only component on the 16 Mhz line. Shunt this to be quicker
      vga_.PreciseTick ();

      // Monitor tick : Sample this on 16 Mhz, to be synched with Gate Array.
      monitor_.Tick ();

      // BUS tick : Handle bus datas
      //m_Sig.Tick ();
   }
}

bool EmulatorEngine::LoadBin(const char* path_file)
{
   bin_to_load_path_ = path_file;
   bin_to_load_ = true;
   return true;
}

bool EmulatorEngine::LoadBinInt(unsigned char* buffer, unsigned int size)
{
   // Header ? check the CRC
   if (size < 128) return false;

   unsigned int crc = 0;
   for (int i = 0; i < 67; i++)
   {
      crc += buffer[i];
   }
   if (((crc & 0xFF) == buffer[67])
      && (((crc >> 8) & 0xFF) == buffer[68])
      && (((buffer[18] >> 1) & 0x7) == 1)
      )
   {
      // AMSDOS Header:
      // Extract user
      // Extract file name
      // File type
      // Length
      unsigned int length = buffer[64] +
         (buffer[65] << 8) +
         (buffer[66] << 16);

      // Location
      // Entry adress
      if (length + 128 < size) return false;

      // Copy to memory ( RAM ) - TODO : Check proper RAM !
//      OnOff();
//      memory_->Initialisation();


      unsigned short addr = buffer[21] + (buffer[22] << 8);
      unsigned char *dest = memory_->GetRamBuffer();
      for (unsigned int i = 0; i < length; i++)
      {
         dest[addr + i] = buffer[128+i];
         //memory_->Set( i + addr, pBuffer[i] );
      }
      // Set run programm at bF00
      // LD C, &FF
      dest[0xBF00] = 0x0e;
      dest[0xBF01] = 0xFF;
      // LD, HL, base_program
      dest[0xBF02] = 0x21;
      dest[0xBF03] = addr&0xFF;
      dest[0xBF04] = (addr>>8)&0xFF;
      // MC_START_PROGRAM (JP &BD16)
      dest[0xBF05] = 0xC3;
      dest[0xBF06] = 0x16;
      dest[0xBF07] = 0xBD;

      z80_.pc_ = 0xBF00;
      z80_.ReinitProc();
      // TODO : Add some more controls / enable things
//      memory_->m_InfROMConnected = false;
//      memory_->m_SupROMConnected = false;
//      memory_->SetMemoryMap();
//      memory_->LowerROM[0x5E9] = buffer[26];
//      memory_->LowerROM[0x5EA] = buffer[27];
//      z80_.pc_ = 00;

//      z80_.ReinitProc();
      return true;
   }
   else
   {
      return false;
   }
}

bool EmulatorEngine::LoadBinInt (const char* path_file)
{
   // Load AMSDOS file
   bool return_value= false;
   FILE * file;

   if (fopen_s (&file, path_file, "rb") == 0)
   {
      fseek(file, 0, SEEK_END);
      long size = ftell(file);
      rewind(file);

      unsigned char * buffer = new unsigned char[size];

      if (fread(buffer, 1, size, file) == size)
      {
         return_value= LoadBinInt(buffer, size);
      }
      else
      {
         return_value= false;
      }
      delete[]buffer;

      fclose(file);
   }

   return return_value;
}


void EmulatorEngine::BuildEngine()
{
   // From configuration :
   /*if (generated_loop_.y == NULL)
   {
      generated_loop_.y = (byte*)VirtualAllocEx(GetCurrentProcess(), 0, 1 << 16, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
   }
   byte* buf = generated_loop_.y;

   if (buf == 0) return;

   byte* p = buf;

   // Create inner loop of x tick ( x/4 us)
   int x = 256; // 64us
   */
   // Add loop
   // NbCycles From the stack
   // Compute next_cycle : int next_cycle = NbCycles;
   // Then,
   /*
   int* elapsed = m_ElapsedTimeForcomponents;
   for (int i = 0; i < index; ++i)
   {
      if (*elapsed < next_cycle)
      {
         next_cycle = *elapsed;
      }
      ++elapsed;
   }

   // After all this preparative work,
   //while (next_cycle < NbCycles)

   for (int i = 0; i < x; i++)
   {
      // And, for each declared component,
         // RUN_COMPOSANT_N(tape_, elapsed_time_tape);
   }

   //
   *p++ = 0xC3; // ret
   */
}


void EmulatorEngine::UpdateEmulator()
{
   // Sound
   //InitSound(GetSound());


}

void EmulatorEngine::UpdateComputer(bool no_cart_reload)
{
   fs::path path;

   GetMem()->InitMemory();

   // Update RAM todo
   GetMem()->SetRam(current_settings_->GetRamCfg());

   // Update ROM todo
   if (directories_ != nullptr)
   {
      path = directories_->GetBaseDirectory();
   }
   path /= ROMPath;
   fs::path rom_path = path;
   path /= current_settings_->GetLowerRom();

   GetMem()->LoadLowerROM(path.string().c_str());
   GetPSG()->LoadKeyboardMap(current_settings_->GetKeyboardConfig() );
   

   for (int i = 0; i < 256; i++)
   {
      const char* rom_path_str = current_settings_->GetUpperRom(i);
      if (rom_path_str == nullptr)
      {
         GetMem()->LoadROM(i, NULL);
      }
      else
      {
         path = rom_path;
         path /= rom_path_str;
         GetMem()->LoadROM(i, path.string().c_str());
      }
   }
   
   // Hardware
   GetCRTC()->DefinirTypeCRTC(current_settings_->GetCRTCType());
   SetPAL(current_settings_->PALPlugged());
   SetFDCPlugged(current_settings_->FDCPlugged());

   // External devices
   motherboard_.UpdateExternalDevices();


   // PLUS Machine ?todo
   unsigned int hardware_type = current_settings_->GetHardwareType();
   if (hardware_type == MachineSettings::PLUS_6128
      || hardware_type == MachineSettings::PLUS_464)
   {
      SetMachineType(hardware_type);
      SetPlus(true);
      if (no_cart_reload == false)
      {
         LoadCpr(current_settings_->GetDefaultCartridge());
      }
   }
   else
   {
      SetPlus(false);
   }

}

void EmulatorEngine::SetSettings(EmulatorSettings& emulator_settings)
{
   emulator_settings_ = emulator_settings;
   UpdateFromSettings();
}

void EmulatorEngine::UpdateFromSettings()
{
   // Reinitialize settings
   InitSound(emulator_settings_.GetSound());
   // 
}