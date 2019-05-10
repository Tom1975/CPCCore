#pragma once

#include "IDisk.h"
#include "ILog.h"
#include "DiskBuilder.h"
#include "DskTypeManager.h"
#include "ILoadingProgress.h"

#include "simple_string.h"

#define NB_ROUND_SPEED_MAX 50
#define SPEED_MAX_READY 203000
#define SPEED_0 1000000


class DiskGen : public ITypeManager
{
public:

   using CodageMfm = enum
   {
      CODE_MFM,
      CODE_FM
   };

   DiskGen(void);
   virtual ~DiskGen(void);

   //
   IDisk::FaceSelection FilterSide(IDisk::FaceSelection face_selection);

   //////////////////////////////////////////////////////////
   // Implementation of ITypeManager
   virtual int GetTypeFromFile(const char* str);
   virtual int GetTypeFromBuffer(unsigned char* buffer, int size);
   virtual int GetTypeFromMultipleTypes(int* type_list, int nb_types);
   virtual int GetCurrentLoadProgress();

   //////////////////////////////////////////////////////////

   virtual void DumpTrack(int side, int track)
   {
      if (disk_ != NULL)
         if (side < disk_->GetNumberOfSide())
         {
            if (side_0_number_ == 0)disk_->DumpTrack(side, track);
            else
            {
               if (current_side_ == 0 && disk_->GetNumberOfSide() == 2)
                  disk_->DumpTrack(1, track);
               else
                  disk_->DumpTrack(0, track);
            }
         }
   };

   virtual void SetWrite(int track) { if (disk_ != NULL)disk_->SetWrite(current_side_, track); };
   
   virtual void SetWrite(int side, unsigned int track)
   {
      if (disk_ != NULL)
         if (side < disk_->GetNumberOfSide())
         {
            if (side_0_number_ == 0)disk_->SetWrite(side, track);
            else
            {
               if (current_side_ == 0 && disk_->GetNumberOfSide() == 2)
                  disk_->SetWrite(1, track);
               else
                  disk_->SetWrite(0, track);
            }
         }
   };

   //
   void SetFixedSpeed(bool fixed_speed)
   {
      fixed_speed_ = fixed_speed;
      if (disk_)disk_->SetFixedSpeed(fixed_speed_);
   };
   void SetMfm(CodageMfm code) { encode_scheme_ = code; };

   void SetLog(ILog* log) { log_ = log; };
   virtual void Init(DskTypeManager* disk_type_manager);
   virtual void SetWriteProtect(bool on) { write_protection_on_ = on; };
   virtual void Eject();
   virtual void CleanDisk();
   virtual std::vector<IDisk*> CreateDisk(DataContainer* container, ILoadingProgress* loading_progress = nullptr,
                                          ILog* log = nullptr);
   virtual IDisk* CreateDisk(const char* path, ILoadingProgress* loading_progress = nullptr, ILog* log = nullptr);
   virtual int LoadDisk(const char* file_path);
   virtual int LoadDisk(DataContainer* container, ILoadingProgress* loading_progress = nullptr);
   virtual int LoadDisk(IDisk* new_disk);
   const char* GetCurrentLoadedDisk() { return (disk_ != NULL) ? disk_->GetCurrentLoadedDisk() : ""; };
   virtual void WriteDisk(const char* file_path, FormatType* format);
   virtual void WriteDisk();

   virtual int GetNbTracks(int side);
   void SetCurrentTrack(int track);
   int GetCurrentTrack() { return current_track_; }
   int GetCurrentSide() { return current_side_; }
   bool DoubleSided() { return (disk_ != NULL) ? ((disk_->GetNumberOfSide() == 2) ? true : false) : false; };

   virtual void Recalib();
   int GetPos() { return (disk_ != NULL) ? (disk_->GetPos() / 16) : 0; };

   virtual int GetSide()
   {
      return (side_0_number_ == 0) ? current_side_ : ((current_side_ == 0 && disk_->GetNumberOfSide() > 1) ? 1 : 0);
   };


   virtual void SetSide(int side)
   {
      if (disk_ != NULL)
      {
         if (side < disk_->GetNumberOfSide())
         {
            if (side_0_number_ == 0)current_side_ = side;
            else
            {
               if (side == 0 && disk_->GetNumberOfSide() > 1)
                  current_side_ = 1;
               else
                  current_side_ = 0;
            }
         }
         else
         {
            current_side_ = 0;
         }
      }
   };
   virtual bool SideValid() { return ((disk_ != NULL) ? current_side_ < disk_->GetNumberOfSide() : false); };
   // Advance
   void Tick();
   virtual bool NewBitAvailable() { return new_bit_available_; };

   // Status -
   bool IsDiskPresent() { return disk_present_; };
   bool IsFixedStatus();

   // Information
   bool IsByteAvailable() { return byte_ready_; };
   bool IsSync() { return sync_found_; };

   bool IsDataBit() { return data_bit_; };
   unsigned char GetCurrentBit();

   // Set sync data
   void ShiftDataBit() { data_bit_ = !data_bit_; };
   void Write(unsigned char byte, bool sync = false);
   bool IsWriteOver() { return read_; };

   bool IsDiskModified() { return (disk_ != NULL) ? disk_->IsDiskModified() : false; };
   int GetSizeOfTrack(int side, int track) { return disk_->GetSizeOfTrack(side, track); }

   void InsertBlankDisk(IDisk::DiskType);


   void FlipDisk();

   void StartMotor(bool on) { ComputeSpeed(on ? 1 : 2); };
   bool IsMotorOn();

   IDisk::AutorunType GetAutorun(char* buffer, unsigned int size_of_buffer);

   //////////////////////////////////
   // Signal access
   // ---- IN

   // ---- OUT
   bool IsHeadOnIndex() { return on_index_hole_; };
   // Ready : Motor is on (with enough speed) AND disk is present
   bool IsDiskReady(bool final_trick = false)
   {
      if (final_trick)
      {
         return (disk_present_ && final_speed_ != SPEED_0);
      }
      else
      {
         return (disk_present_ && (current_speed_ < SPEED_MAX_READY) /*m_CurrentSpeed > ((60.0 * 1000000.0/300)*0.90)*/
         );
      }
   }
   ;
   bool IsTr0() { return current_track_ == 0; };
   bool IsWriteProtected() { return write_protection_on_; };
   bool IsTwoSide() { return disk_->GetNumberOfSide() == 2; }

   int CompareToDisk(DiskGen* other_disk);
   IDisk* GetDisk() { return disk_; }

protected:

   //
   DiskBuilder disk_builder_;
   FormatType* current_disk_format_;

   CodageMfm encode_scheme_;
   //////////////////////////////////
   // Signals from FDC to disk reader
   // HEAD SELECT (HD)
   // HEAD LOAD (HDL)
   // Write Enable (WE)
   // Flt Reset
   // Step
   // Low Current
   // DIR
   // WDA
   // PS0,1

   // Signals from disk reader to FDC
   // Index
   bool on_index_hole_;
   // FLT
   // TR0 - NA
   // WP
   bool write_protection_on_;
   // TS
   // RD
   // RDW

   bool fixed_speed_;

   bool disk_present_;

   bool read_;
   unsigned short byte_to_write_;
   bool sync_write_;
   int write_bit_;

   int current_side_;
   unsigned int current_track_;


   //
   unsigned int side_0_number_;

   ILog* log_;
   IDisk* disk_;

   bool sync_found_;
   unsigned short current_mfm_byte_;

   bool byte_ready_;

   bool data_bit_;

   ////////////////////////////////
   // Disk speed datas
   // - Speed is increased after motor is on
   // - Speed is decreased after motor stopped
   // - Speed depend from track, and also a random factor
   //
   // 00: 19830-19840 (302.4 RPM)
   // 05: 19825-19835 (302.5 RPM)
   // 10: 19815-19825 (302.7 RPM)
   // 15: 19800-19810 (302.9 RPM)
   // 20: 19795-19805 (303.0 RPM)
   // 25: 19785-19795 (303.2 RPM)
   // 30: 19775-19785 (303.3 RPM)
   // 35: 19770-19780 (303.4 RPM)
   // 40: 19755-19770 (303.6 RPM)
   //
   // Add up to 10 us per track (randomly)

   // Some computations :
   // X = nb of bit on this track
   // Y = nb of time for one bit.
   // S = Speed : total time for one revolution ( 19830 -> 19770 +- 10)
   // Y = S / X. This is a float.
   // Adjustement : Each bit time, see where we are, to adjust (and set bit as "ready" or "not ready".
   // Speed is computed at each revolution start.
   int current_speed_; // in 10 us / revolution
   int final_speed_; // Speed aimed to (for start / stop
   float time_for_one_bit_; // 1 = 1 us. Average value should be around 2
   bool motor_on_;
   float timer_count_;

   // Type of speed : 0 = change track; 1 = start; 2 = stop
   void ComputeSpeed(int type_of_speed);
   bool new_bit_available_;

   int speed_change_counter_;

   // Speed of motor : 3 seconds for full speed = 50 round ?
   DskTypeManager* disk_type_manager_;
};
