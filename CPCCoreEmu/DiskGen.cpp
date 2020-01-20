#include "stdafx.h"
#include "DiskGen.h"
//#include "rand.h"
#include <stdlib.h>
#include "simple_vector.hpp"
#include "MediaManager.h"

#define LOGFDC

#define SPEED_COUNTER 1
#define SPEED_CHANGE 20 /*20000*/

#ifdef LOGFDC
#define LOG(str) \
   if (log_) log_->WriteLog (str);
#define LOGEOL if (log_) log_->EndOfLine ();
#define LOGB(str) \
   if (log_) log_->WriteLogByte (str);
#else
#define LOG(str)
#define LOGB(str)
#define LOGEOL
#endif


static unsigned short SyncSeqShort1 = 0x4489;
static unsigned short SyncSeqShort2 = 0x5224;

#define RPM_MIN      298.5
#define RPM_MAX      299.5

DiskGen::DiskGen(void)
{
   byte_to_write_ = 0;
   sync_write_ = false;
   write_bit_ = 0;
   sync_found_ = false;
   current_mfm_byte_ = 0;
   byte_ready_ = false;
   time_for_one_bit_ = 3;
   speed_change_counter_ = 0;

   log_ = nullptr;
   fixed_speed_ = false;
   disk_present_ = false;
   disk_ = NULL;

   read_ = true;
   write_protection_on_ = false;

   current_side_ = 0;

   on_index_hole_ = false;

   data_bit_ = false;
   current_track_ = 0;
   current_speed_ = final_speed_ = SPEED_0;
   motor_on_ = false;
   new_bit_available_ = false;
   timer_count_ = 0;
   side_0_number_ = 0;

   encode_scheme_ = CODE_MFM;

   disk_type_manager_ = nullptr;
}


DiskGen::~DiskGen(void)
{
   Eject();
}

void DiskGen::Init(DskTypeManager* disk_type_manager)
{
   disk_type_manager_ = disk_type_manager;
}

IDisk::FaceSelection DiskGen::FilterSide(IDisk::FaceSelection faceSelection)
{
   if (disk_ == nullptr) return IDisk::NO_FACE;

   return disk_->FilterSide(faceSelection);
}


void DiskGen::FlipDisk()
{
   side_0_number_ ^= 1;
}

bool DiskGen::IsMotorOn()
{
   return (final_speed_ != SPEED_0);
}

void DiskGen::Eject()
{
   // No disk inside
   CleanDisk();

   disk_present_ = false;
}

bool DiskGen::IsFixedStatus()
{
   return (disk_ != NULL) ? disk_->IsFixedStatus() : false;
}


int DiskGen::GetNbTracks(int side)
{
   return (disk_present_) ? disk_->GetNumberOfTracks() : 0;
}

void DiskGen::SetCurrentTrack(int track)
{
   if (disk_ == nullptr) return; 
   if (disk_present_)
   {
      // Get next single bit
      disk_->ChangeTrack(current_side_, track);
      ComputeSpeed(0);
   }

   if (track <= disk_->side_[current_side_].nb_tracks)
   {
      current_track_ = track;
   }
   else
   {
      current_track_ = disk_->side_[current_side_].nb_tracks-1;
   }
   
}

void DiskGen::Write(unsigned char byte, bool bSync)
{
   read_ = false;

   // Convert to MFM Sync bytes
   if (bSync && byte == 0xA1)
   {
      sync_write_ = true;
      byte_to_write_ = SyncSeqShort1;
   }
   else if (bSync && byte == 0xC2)
   {
      sync_write_ = true;
      byte_to_write_ = SyncSeqShort2;
   }
   else
   {
      sync_write_ = false;
      byte_to_write_ = byte;
   }

   //LOGB(byte);

   write_bit_ = 0;
}

void DiskGen::Tick()
{
   if (!motor_on_)
   {
      // Check if Final speed is different from 0
      if (final_speed_ != SPEED_0)
      {
         motor_on_ = true;
         speed_change_counter_ = 0;
      }
   }

   if (disk_present_ && motor_on_)
   {
      // Do we have to change speed ?
      if (speed_change_counter_ == 0)
      {
         // Change every 100000 ticks (20 ms)
         speed_change_counter_ = SPEED_COUNTER; // 10000;

         if (current_speed_ < final_speed_)
         {
            current_speed_ = current_speed_ + SPEED_CHANGE/*m_FinalSpeed / NB_ROUND_SPEED_MAX*/;
            if (current_speed_ > final_speed_) current_speed_ = final_speed_;

            if (current_speed_ == SPEED_0) motor_on_ = false;
         }
         else
         {
            current_speed_ = current_speed_ - SPEED_CHANGE/*m_FinalSpeed / NB_ROUND_SPEED_MAX*/;
            if (current_speed_ < final_speed_) current_speed_ = final_speed_;

            // 0 ?
         }
         // Compute the "time for one bit"
         if (motor_on_)
            time_for_one_bit_ = ((float)current_speed_ / (float)disk_->GetSizeOfTrack(current_side_, current_track_));
      }
      else
      {
         --speed_change_counter_;
      }

      // Default value : if m_TimeForOneBit is too low, it's not used
      if (time_for_one_bit_ > 3) time_for_one_bit_ = 3;

      // Can we handle the next bit ?
      // Add the real time value
      timer_count_ += 2.0;
      // int nb_bit_read = m_bDataBit?2:1;
      int nb_bit_read = 1;

      // Remove the speed value
      while (timer_count_ - time_for_one_bit_ > 0 && nb_bit_read > 0)
      {
         --nb_bit_read;

         // To adjust !
         timer_count_ -= time_for_one_bit_;

         new_bit_available_ = true;
         if (read_)
         {
            // Get next single bit
            unsigned char b = disk_->GetNextBit(current_side_, current_track_);
            if (encode_scheme_ == CODE_FM)
            {
               // TODO : Not supported now
               b = ( rand() & 0x1) | (b & BIT_INDEX);
            }


            // Index hole ?
            on_index_hole_ = ((b & BIT_INDEX) == BIT_INDEX);

            // Complete current byte
            current_mfm_byte_ <<= 1;
            current_mfm_byte_ |= (b & 0x1);

            if (((current_mfm_byte_ & 0xF) == 0)
               || ((current_mfm_byte_ & 0x3) == 3)
            )
            {
               
            }

            // If sync search on : compare
            sync_found_ = ((current_mfm_byte_ == SyncSeqShort1)
               || (current_mfm_byte_ == SyncSeqShort2)
            );
         }
         else
         {
            // Write !
            unsigned char b;
            if (!sync_write_)
            {
               b = ((byte_to_write_ >> (7 - (write_bit_ >> 1))) & 0x01);
               // If m_bWriteBit odd, write next data bit
               if ((write_bit_ & 0x1) == 0x0)
               {
                  // Otherwise, write clock
                  b = (b == 1) ? 0 : (((current_mfm_byte_ & 0x1) == 0x1) ? 0 : 1);
               }
            }
            else
            {
               b = byte_to_write_ >> (15 - write_bit_) & 0x01;
            }

            disk_->WriteBit(current_side_, current_track_, b);

            current_mfm_byte_ <<= 1;
            current_mfm_byte_ |= (b & 0x1);


            // Increase bit to write
            ++write_bit_;
            if (write_bit_ == 16)
            {
               write_bit_ = 0;
               read_ = true;
            }
         }

         data_bit_ = !data_bit_;
      }
   }
}

unsigned char DiskGen::GetCurrentBit()
{
   new_bit_available_ = false;
   return (current_mfm_byte_ & 0x1);
}


void DiskGen::Recalib()
{
   if (current_track_ > 77)
   {
      current_track_ -= 77;
   }
   else
   {
      current_track_ = 0;
   }
   if (disk_present_)
   {
      // Get next single bit
      disk_->ChangeTrack(current_side_, current_track_);
      ComputeSpeed(0);
   }
}


int DiskGen::LoadDisk(IDisk* new_disk)
{
   CleanDisk();
   disk_ = new_disk;

   disk_->SetFixedSpeed(fixed_speed_);
   disk_->SetLog(log_);
   current_side_ = 0;

   // Juste check there is enough tracks...
   if (disk_->GetNumberOfTracks(0) <= current_track_)
      current_track_ = 0;

   disk_present_ = true;


   return 0;
}

int DiskGen::LoadDisk(DataContainer* container, ILoadingProgress* loading_progress)
{
   // Get proper IDisk
   std::vector<IDisk*> disk_list = CreateDisk(container, loading_progress, log_);

   // Load
   if (disk_list.size() == 0) return -1;

   CleanDisk();
   disk_ = disk_list[0];

   disk_->SetFixedSpeed(fixed_speed_);
   disk_->SetLog(log_);
   current_side_ = 0;

   // Juste check there is enough tracks...
   if (disk_->GetNumberOfTracks(0) <= current_track_)
      current_track_ = 0;

   disk_present_ = true;


   return 0;
}

IDisk* DiskGen::CreateDisk(const char* path, ILoadingProgress* loading_progress, ILog* log)
{
   IDisk* new_disk = nullptr;
   FormatType* format;
   if (disk_builder_.CanLoad(path, format))
   {
      if (disk_builder_.LoadDisk(path, new_disk, loading_progress) == 0)
      {
         current_disk_format_ = format;
      }
   }
   return new_disk;
}

int DiskGen::GetTypeFromBuffer(unsigned char* buffer, int size)
{
   return (disk_type_manager_ == nullptr) ? 0 : disk_type_manager_->GetTypeFromBuffer(buffer, size);
}

int DiskGen::GetTypeFromFile(const char* str)
{
   return (disk_type_manager_ == nullptr) ? 0 : disk_type_manager_->GetTypeFromFile(str);
}

int DiskGen::GetTypeFromMultipleTypes(int* type_list, int nb_types)
{
   return 0;
}


std::vector<IDisk*> DiskGen::CreateDisk(DataContainer* container, ILoadingProgress* loading_progress, ILog* log)
{
   MediaManager media_mgr(container);
   return media_mgr.GetDisk(loading_progress, log);
}

int DiskGen::GetCurrentLoadProgress()
{
   if (disk_ != nullptr)
   {
      return disk_->GetCurrentLoadProgress();
   }
   else
   {
      return -1;
   }
}

int DiskGen::LoadDisk(const char* filepath)
{
   // Get proper IDisk
   IDisk* disk = CreateDisk(filepath, nullptr, log_);

   // Load
   if (disk == nullptr) return -1;

   CleanDisk();
   disk_ = disk;

   disk_->SetFixedSpeed(fixed_speed_);
   disk_->SetName(filepath);
   disk_->SetLog(log_);
   current_side_ = 0;

   // Juste check there is enough tracks...
   if (disk_->GetNumberOfTracks(0) <= current_track_)
      current_track_ = 0;

   disk_present_ = true;

   return 0;
}


void DiskGen::WriteDisk()
{
   if (disk_ != NULL)
   {
      current_disk_format_->SaveDisk(disk_->GetCurrentLoadedDisk(), disk_);
   }
}

void DiskGen::WriteDisk(const char* filepath, FormatType* format)
{
   if (disk_ != NULL)
   {
      format->SaveDisk(filepath, disk_);
   }
}


void DiskGen::CleanDisk()
{
   if (disk_ != NULL)
   {
      disk_->CleanDisk();
   }

   delete disk_;
   disk_ = NULL;
}

void DiskGen::InsertBlankDisk(IDisk::DiskType diskType)
{
   // Eject existing
   Eject();

   // Insert new RAW blank disk
   disk_ = new IDisk(diskType);

   disk_->SetLog(log_);
   disk_present_ = true;

   disk_->SetName("");
}

void DiskGen::ComputeSpeed(int typeOfSpeed)
{
   switch (typeOfSpeed)
   {
   case 0: // Track changing
      // todo
      break;
   case 1: // Start
      {
         // Change final speed; Curent speed will be adjusted each turn;
         // Example :
         // 00: 19830-19840 (302.4 RPM)
         // 05: 19825-19835 (302.5 RPM)
         // 10: 19815-19825 (302.7 RPM)
         // 15: 19800-19810 (302.9 RPM)
         // 20: 19795-19805 (303.0 RPM)
         // 25: 19785-19795 (303.2 RPM)
         // 30: 19775-19785 (303.3 RPM)
         // 35: 19770-19780 (303.4 RPM)
         // 40: 19755-19770 (303.6 RPM)

         // Adjust  between 297 and 300 RPM
         // 297 RPM = 297/60 RPS = VS
         // Speed = 60/297 * 1000 * 100 ( 10us / rota)


         // Use linear computation : 0 = 19840; 40 = 19755; X = 19830 + X*(19840-19755)/41
         //float slope = (198300.0-197550.0)/41.0;
         float slope;
         float vsMax = static_cast<float>(60.0f * 1000000.0f / RPM_MIN);
         float vsMin = static_cast<float>(60.0f * 1000000.0f / RPM_MAX);
         slope = (vsMax - vsMin) / 41.0f;

         final_speed_ = static_cast<int>(vsMax - current_track_ * slope);
         if (!fixed_speed_)
         {
            final_speed_ += rand() % 100;
         }

         break;
      }
   case 2: // Stop
      final_speed_ = SPEED_0;
      break;
   }
}


bool CompareStringFromCat(const char* track, const char* searched_string, const char* search_ext)
{
   // Compare string
   char file_ext[4] = {0x0};
   memcpy(file_ext, &track[8], 3);
   file_ext[0] = file_ext[0] & 0x7F;
   file_ext[1] = file_ext[1] & 0x7F;
   char filename[9] = {0};
   for (int i = 0; i < 8 && track[i] != 0x20; i++)
      filename[i] = track[i];

   return (stricmp(filename, searched_string) == 0
      && strnicmp(file_ext, search_ext, strlen(search_ext)) == 0);
}

int NbIdenticalCar(const char* track, const char* searched_string, int size_max)
{
   int i = 0;
   int lg_max = strlen(searched_string);
   int nb_car_found = 0;

   for (int j = 0; j < lg_max; j++)
   {
      bool correct = true;
      for (i = 0; i < size_max && i < 8 && i < lg_max - j && correct; i++)
      {
         if (track[i] != searched_string[j + i]
            && track[i] + 0x20 != searched_string[j + i]
            && track[i] != searched_string[j + i] + 0x20)
         {
            correct = false;
         }
         else
         {
            if (i >= nb_car_found) nb_car_found = i + 1;
         }
      }
   }

   return nb_car_found;
}


bool FindName(std::vector<std::string>& filename_vector, const char* name, const char* ext)
{
   bool ret = false;
   for (auto it = filename_vector.begin(); it != filename_vector.end(); it++)
   {
      if (CompareStringFromCat(it->c_str(), name, ext))
      {
         return true;
      }
   }
   return ret;
}




IDisk::AutorunType DiskGen::GetAutorun(char* buffer, unsigned int size_of_buffer)
{
   IDisk::AutorunType ret;
   bool end = false;
   int nb_correct_char = 0;
   bool correct_hidden = 1;

   std::vector<std::string> filename_vector = disk_->GetCat(ret);

   // Get disk name
   std::string disk_filename = GetCurrentLoadedDisk();

   // Specific rules : for specific disks !
   //todo

   // Generic Rules
   class Rule
   {
   public:
      Rule(const char* name, const char* ext, const char* autofile) : name_(name), ext_(ext), autofile_(autofile)
      {
      }
      Rule(const Rule& rule) 
      {
         name_ = rule.name_;
         ext_ = rule.ext_;
         autofile_ = rule.autofile_;
      }

      const char* name_;
      const char* ext_;
      const char* autofile_;
   };

   std::vector<Rule> rule_list;
   rule_list.push_back(Rule("disc", "bin", "DISC"));
   rule_list.push_back(Rule("disk", "bin", "DISK"));
   rule_list.push_back(Rule("disc", "bas", "DISC"));
   rule_list.push_back(Rule("disk", "bas", "DISK"));
   rule_list.push_back(Rule("elite", "bin", "ELITE"));
   rule_list.push_back(Rule("elite", "bas", "ELITE"));
   rule_list.push_back(Rule("ubi", "bin", "UBI"));
   rule_list.push_back(Rule("ubi", "bas", "UBI"));
   rule_list.push_back(Rule("ubi", "", "UBI"));
   rule_list.push_back(Rule("ere", "bin", "ERE.BAS"));
   rule_list.push_back(Rule("ere", "bas", "ERE.BAS"));
   rule_list.push_back(Rule("mbc", "bin", "MBC"));
   rule_list.push_back(Rule("mbc", "bas", "MBC"));
   rule_list.push_back(Rule("esat", "bas", "ESAT"));
   rule_list.push_back(Rule("esat", "bin", "ESAT"));
   rule_list.push_back(Rule("esat", "", "ESAT"));
   rule_list.push_back(Rule("jeu", "bin", "JEU"));
   rule_list.push_back(Rule("jeu", "bas", "JEU"));

   for (auto it = rule_list.begin(); it != rule_list.end() && (end == false); it++)
   {
      if (FindName(filename_vector, it->name_, it->ext_))
      {
         strcpy(buffer, it->autofile_);
         end = true;
      }
   }

   // Other rules
   bool ext_binbas = false;

   if (end == false)
   {
      for (auto it = filename_vector.begin(); it != filename_vector.end(); it++)
      {
         unsigned char filename[16] = {0};
         memcpy(filename, it->c_str(), 13);
         filename[9] = filename[9] & 0x7F; // READ ONLY
         filename[8] = filename[8] & 0x7F; // HIDDEN / SYSTEM
         bool bnewbinbas = false;
         if (strnicmp((char*)&filename[8], "bin", 3) == 0
            || strnicmp((char*)&filename[8], "bas", 3) == 0
            || strnicmp((char*)&filename[8], "   ", 3) == 0)
         {
            // Priority : If it's call "sav?????.b??", it's less priority thant any other
            int nbcar = NbIdenticalCar(it->c_str(), disk_filename.c_str(), disk_filename.size());
            if (strncmp(&it->c_str()[8], "BIN", 3) == 0
               || strncmp(&it->c_str()[8], "BAS", 3) == 0
            )
               bnewbinbas = true;
            if (nbcar > nb_correct_char || nb_correct_char == 0 || (correct_hidden == true && ((it->c_str()[9] & 0x80)
               == 0)) || (bnewbinbas == true && ext_binbas == false))
            {
               if (correct_hidden == false && (it->c_str()[9] & 0x80) == 0x80)
               {
               }
               else if (bnewbinbas == false && ext_binbas == true)
               {
               }
               else
               {
                  // Update return;
                  nb_correct_char = nbcar;
                  ext_binbas = bnewbinbas;

                  correct_hidden = it->c_str()[9] & 0x80;
                  memcpy(buffer, it->c_str(), 8);
                  for (int i = 7; i >= 0; i--)
                  {
                     if (buffer[i] == 0x20) buffer[i] = 0x00;
                     else break;
                  }
               }
            }
         }
      }
   }

   if (end || strlen(buffer) > 0)
      ret = IDisk::AUTO_FILE;

   return ret;
}


int DiskGen::CompareToDisk(DiskGen* other_disk)
{
   if (disk_ == NULL || other_disk->disk_ == NULL) return -1;
   return disk_->CompareToDisk(other_disk->disk_);
}
