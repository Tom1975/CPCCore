#include "stdafx.h"
#include "Machine.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "PrinterDefault.h"
#include "MediaManager.h"

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

const char* ROMPath = "ROM";
const char* CartPath = "CART";

#define MAX_SIZE_BUFFER 256

EmulatorEngine::EmulatorEngine() :
   log_(nullptr), paste_size_(0), paste_count_(0), sna_handler_(log_), media_inserted_(&disk_type_manager_),
   do_snapshot_(false), current_settings_(nullptr),
   directories_ (nullptr), display_(nullptr), motherboard_(&sound_mixer_, &keyboardhandler_)
{
   breakpoint_handler_.Init(this);
   fdc_present_ = true;
   pal_present_ = true;
   quick_sna_available_ = false;

   time_slice_ = 20;
   time_elapsed_ = std::chrono::steady_clock::now();
   time_computed_ = 0;

   speed_limit_ = E_NONE;
   memset (paste_buffer_, 0, sizeof(paste_buffer_));
   paste_wait_time_ = 3;
   paste_vkey_ = 0;
   speed_percent_ = 0;

   speed_ = 100;
   notifier_ = NULL;
   stop_pc_ = 0;
   remember_step_ = false;
   bin_to_load_ = false;
   sna_to_load_ = false;
}

EmulatorEngine::~EmulatorEngine(void)
{
}

bool EmulatorEngine::Init (IDisplay* display, ISoundFactory* sound)
{
   DataContainer::Init();

   display_ = display;
   counter_ = 0;
   paste_available_ = false;
   paste_wait_time_ = 3;

   keyboardhandler_.SetDirectories(directories_);
   keyboardhandler_.SetConfigurationManager(configuration_manager_);
   keyboardhandler_.LoadKeyboardMap("FRENCH");

   motherboard_.InitMotherbard(log_, &sna_handler_, display_, notifier_, directories_, configuration_manager_);
   sna_handler_.SetMachine(&motherboard_);

   return true;
}

void EmulatorEngine::UpdateExternalDevices()
{
   GetSig()->nb_expansion_ = 0;
   
   /*if (current_settings_->GetMultiface2())
   {
      m_Sig.m_ExpList[m_Sig.m_NbExpansionPlugged++] = &multiface2_;
   }
   if (current_settings_->GetPlayCity())
   {
      m_Sig.m_ExpList[m_Sig.m_NbExpansionPlugged++] = play_city;
   }*/
   GetSig()->exp_list_[GetSig()->nb_expansion_++] = GetMotherboard()->GetPlayCity();
}

bool EmulatorEngine::InitSound ( ISound* sound)
{
   GetPSG()->InitSound (sound);
   sound_mixer_.Init(sound, GetTape());
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
   return GetFDC()->IsRunning(drive );
}

void EmulatorEngine::InsertBlankDisk ( unsigned int DriveNumber, IDisk::DiskType type )
{
   GetFDC()->InsertBlankDisk ( DriveNumber, type );
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

   GetFDC()->EjectDisk (DriveNumber);
}

void EmulatorEngine::FlipDisk ( unsigned int DriveNumber )
{
   GetFDC()->FlipDisk (DriveNumber);
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
   int ret = GetTape()->InsertTape(container);
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
   int ret = GetTape()->InsertTape ( file_path );
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
   return GetFDC()->CompareDriveAandB();
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
      motherboard_.EjectCartridge();

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
               unsigned char* rom = motherboard_.GetCartridge(block_number);
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
   if (GetFDC()->GetCurrentProgress() != -1)
      return GetFDC()->GetCurrentProgress ();
   else return GetTape()->GetCurrentProgress();
}

int EmulatorEngine::LoadMedia(DataContainer* container)
{
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
   return GetFDC()->LoadDisk(DriveNumber, container, bDifferential);
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

   int ret = GetFDC()->LoadDisk ( DriveNumber, file_path, bDifferential );

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
   sprintf ( tmp_buffer,  "%i", GetCRTC()->type_crtc_);
   configuration_manager_->SetConfiguration(config_name, "CRTC", tmp_buffer, ini_file);


   // Display
   int scanlines = GetVGA()->GetScanlines ( );
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
   fs::path default_path_cfg = "CONF";
   default_path_cfg /= "CPC6128PLUSEN.cfg";

   configuration_manager_->GetConfiguration(config_name, "Machine_Settings", default_path_cfg.string().c_str(), tmp_buffer, MAX_SIZE_BUFFER, ini_file);
   
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
   GetVGA()->SetScanlines(scanlines);

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

   motherboard_.OnOff();

   UpdateComputer(true);
   // todo : update computer

   GetPSG()->Reset ();
   GetSig()->Reset ();

   display_->Reset();
   for (int i = 0; i < GetSig()->nb_expansion_; i++)
   {
      GetSig()->exp_list_[i]->Reset();
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
   GetPSG()->Reset();

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
   GetPSG()->Reset ();

   GetSig()->Reset ();

   display_->Reset();

   time_elapsed_ = std::chrono::steady_clock::now();
   time_computed_ = 0;

   paste_available_ = false;
}

void EmulatorEngine::SetBreakpointHandler ( IBreakpoint* bp )
{
   motherboard_.SetGenericBreakpoint(bp);
}

void EmulatorEngine::AddBreakpoint ( unsigned short addr)
{
   motherboard_.AddBreakpoint(addr);
}

void EmulatorEngine::ChangeBreakpoint ( unsigned short  oldBP, unsigned short newBP )
{
   motherboard_.ChangeBreakpoint(oldBP, newBP);
}

void EmulatorEngine::RemoveBreakpoint ( unsigned short addr)
{
   motherboard_.RemoveBreakpoint(addr);
}

void EmulatorEngine::CleanBreakpoints ()
{
   motherboard_.CleanBreakpoints();
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
         GetKeyboardHandler()->CharPressed(paste_vkey_);
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
      motherboard_.run_ = true;

      run_time = time_slice_ * 4000;
      //if (motherboard_.IsPLUS())
      if(current_settings_->TapePlugged())
      {
         if (current_settings_->FDCPlugged())
         {
            if (motherboard_.GetSig()->nb_expansion_ == 0)
            {
               motherboard_.StartOptimizedPlus<true, true, false>(run_time);
            }
            else
            {
               motherboard_.StartOptimizedPlus<true, true, true>(run_time);
            }
            
         }
         else
         {
            if (motherboard_.GetSig()->nb_expansion_ == 0)
            {
               motherboard_.StartOptimizedPlus<true, false, false>(run_time);
            }
            else
            {
               motherboard_.StartOptimizedPlus<true, false, true>(run_time);
            }
            
         }
            
      }
      else
      {
         if (current_settings_->FDCPlugged())
         {
            if (motherboard_.GetSig()->nb_expansion_ == 0)
            {
               motherboard_.StartOptimizedPlus<false, true, false>(run_time);
            }
            else
            {
               motherboard_.StartOptimizedPlus<false, true, true>(run_time);
            }

         }
         else
         {
            if (motherboard_.GetSig()->nb_expansion_ == 0)
            {
               motherboard_.StartOptimizedPlus<false, false, false>(run_time);
            }
            else
            {
               motherboard_.StartOptimizedPlus<false, false, true>(run_time);
            }

         }
      }
   }
   else
   {

      run_time = motherboard_.DebugNew(time_slice_ * 4000);

      if (!motherboard_.run_)
      {
         display_->VSync(true);
         ret = 1;
      }
   }

   if (do_snapshot_)
   {
      if (GetProc()->machine_cycle_ == Z80::M_FETCH && GetProc()->t_ == 1)
      {
         GetProc()->stop_on_fetch_ = false;
         
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
      GetKeyboardHandler()->CharReleased(paste_buffer_[paste_count_++]);
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

   }
   return ret;
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
   motherboard_.SetPlus(plus);
}

void EmulatorEngine::ChangeConfig(MachineSettings* current_settings)
{
   // todo : update computer
   current_settings_ = current_settings;
   UpdateComputer();
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
      GetMonitor()->RecomputeAllColors ();

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
   GetProc()->stop_on_fetch_ = true;
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
   GetFDC()->WriteDisk ( drive );
}

void EmulatorEngine::SaveDiskAs (int drive, char* file_path, FormatType* format_type)
{
   if (IsDiskPresent ( drive ))
   {
      GetFDC()->WriteDiskAs ( drive, file_path, format_type);
   }
}

bool EmulatorEngine::IsDiskPresent ( int drive )
{
   return GetFDC()->IsDiskPresent ( drive );
}

void EmulatorEngine::Accelerate ( bool speed_on)
{
   // TODO : Set it with a switch it can be disabled.
   /*if ( m_bFastTapeLoading )
      monitor_.Accelerate ( speed_on );*/
}
void EmulatorEngine::Resync()
{
   motherboard_.Resync();
}
void EmulatorEngine::StartPrecise(unsigned int NbCycles)
{
   // Run precise cycle through the clock generator line (16 Mhz)
   for (unsigned int i = 0; i < NbCycles; i++)
   {
      // Gate array is the only component on the 16 Mhz line. Shunt this to be quicker
      GetVGA()->PreciseTick ();

      // Monitor tick : Sample this on 16 Mhz, to be synched with Gate Array.
      GetMonitor()->Tick ();

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

      unsigned short addr = buffer[21] + (buffer[22] << 8);
      unsigned char *dest = motherboard_.GetRamBuffer();
      for (unsigned int i = 0; i < length; i++)
      {
         dest[addr + i] = buffer[128+i];
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

      GetProc()->pc_ = 0xBF00;
      GetProc()->ReinitProc();

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

void EmulatorEngine::UpdateEmulator()
{
}

void EmulatorEngine::LoadRom(int rom_number, const char* path)
{
   FILE* file_rom = nullptr;
   if (fopen_s(&file_rom, path, "rb") == 0 && file_rom != nullptr)
   {
      fseek(file_rom, 0, SEEK_END);
      unsigned int lSize = ftell(file_rom);
      fseek(file_rom, 0, SEEK_SET);

      if (lSize > 0xffff)
      {
         Trace("ROM %s tros grosse pour un CPC ... KO !!\n", path);
         fclose(file_rom);
      }
      else
      {
         Memory::RamBank rom;
         fread(rom, 1, lSize, file_rom);
         fclose(file_rom);
         if (rom_number==-1)
            GetMem()->LoadLowerROM(rom, sizeof(rom));
         else
            GetMem()->LoadROM(rom_number, rom, sizeof(rom));
      }
   }
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
   LoadRom(-1, path.string().c_str());

   GetKeyboardHandler()->LoadKeyboardMap(current_settings_->GetKeyboardConfig() );
   
   for (int i = 0; i < 256; i++)
   {
      const char* rom_path_str = current_settings_->GetUpperRom(i);
      if (rom_path_str == nullptr)
      {
         GetMem()->ClearRom(i);
      }
      else
      {
         path = rom_path;
         path /= rom_path_str;
         LoadRom(i, path.string().c_str());
      }
   }
   
   // Hardware
   GetCRTC()->DefinirTypeCRTC(current_settings_->GetCRTCType());
   SetPAL(current_settings_->PALPlugged());
   SetFDCPlugged(current_settings_->FDCPlugged());

   // External devices
   UpdateExternalDevices();


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
