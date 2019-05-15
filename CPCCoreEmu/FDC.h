#pragma once

#include <stdint.h>

#include "CRC.h"
#include "IComponent.h"

#define CPCCOREEMU_API

#include "ILog.h"
#include "DiskGen.h"
#include "Inotify.h"
#include "ILoadingProgress.h"

#define NB_DRIVES  2

class CPCCOREEMU_API FDC : public IComponent, public ILoadingProgress
{
friend class EmulatorEngine;
friend class CSnapshot;
public:
   FDC(void);
   ~FDC(void);

   void Init (DskTypeManager * disk_type_manager);
   void Reset ();
   void SetLog ( ILog* log) ;
   void SetNotifier(IFdcNotify * notifier) { notifier_ = notifier; }
   void Out (unsigned short addr, unsigned char data);
   unsigned char In ( unsigned short addr );

   virtual void SetProgress(int progress) {load_progress_ = progress;};
   virtual int GetCurrentProgress() { return load_progress_; }

   int LoadDisk ( unsigned int drive_number, const char* file_path, bool delayed = true);
   int LoadDisk ( unsigned int drive_number, DataContainer* container, bool delayed = true);
   int CompareDriveAandB();

   const char* GetDiskPath (unsigned int drive_number);
   int LoadDiskDelayed ( );
   void WriteDisk ( unsigned int drive_number );
   void WriteDiskAs ( unsigned int drive_number, const char* file_path, FormatType* format);
   void EjectDisk (unsigned int drive_number);

   IDisk::AutorunType GetAutorun(unsigned int drive_number, char * buffer, unsigned int size_of_buffer);

   void InsertBlankDisk ( unsigned int drive_number, IDisk::DiskType );

   unsigned int Tick ( /*unsigned int nbTicks = 1*/); // Add 1 ms to the timer

   // 0 : Not present;  1 : No; 2 : Read; 3 : Write
   int IsRunning (int drive );
   bool IsDiskModified (unsigned int drive_number){return disk_[drive_number].IsDiskModified();}
   bool IsDiskPresent ( int drive );

   void SetCurrentTrack ( unsigned int drive_number, int track){disk_[drive_number].SetCurrentTrack (track);};
   int GetCurrentDrive () {return hu_;};
   int GetCurrentSector (unsigned int drive_number) { return (drive_number<NB_DRIVES)?sc_array_[drive_number]:0;};
   int GetCurrentTrack (unsigned int drive_number) { return (drive_number<NB_DRIVES)?disk_[drive_number].GetCurrentTrack ():0; };
   int GetCurrentSide (unsigned int drive_number) { return (drive_number<NB_DRIVES)?disk_[drive_number].GetCurrentSide ():0;};
   void FlipDisk (unsigned int drive_number);

   unsigned int GetNbDrives () { return NB_DRIVES;};

   // Protection status
   bool IsDiskWriteProtected ( int drive_number ) { return (drive_number<NB_DRIVES)?disk_[drive_number].IsWriteProtected():false;}
   void SetWriteProtection ( bool bOn, int drive_number ) { if (drive_number<NB_DRIVES) disk_[drive_number].SetWriteProtect ( bOn);}

   void SetMotor ( bool on);
   bool IsMotorOn () { return disk_[0].IsMotorOn ();}

protected:

   int load_progress_;
   void HandleResult (int result_needed );

   void ResetStatus0();
   void ResetStatus1();
   void ResetStatus2();

   unsigned char GetStatus0 ();
   unsigned char GetStatus1();
   unsigned char GetStatus2();
   unsigned char GetStatus3 ();

   IFdcNotify* notifier_;
   ILog* log_ ;
   enum TransfertType
   {
      WRITE = 0,
      READ = 1,
      NONE = 2
   };

   enum Seek
   {
      NOCHANGE,
      READWRITE,
      SEEK,
      RECAL,
      SENSEINTSTATE
   };


   typedef struct {
      unsigned char param_number;
      unsigned char result_number;
      TransfertType transfer_type;
      void (FDC::* func)();
      unsigned int timing_before;
      unsigned int timing_after;
      Seek seek_cmd;
   } FDCCommand;


   FDCCommand FillStructFDCCommand(unsigned char ParamNumber, unsigned char ResultNumber, TransfertType TransfertType, void (FDC::* func)(), unsigned int timingBefore, unsigned int timingAfter, Seek bSeek = NOCHANGE)
   {
      FDCCommand op;
      op.param_number = ParamNumber;
      op.result_number = ResultNumber;
      op.transfer_type = TransfertType;
      op.func = func;
      op.timing_before = timingBefore;
      op.timing_after = timingAfter;
      op.seek_cmd = bSeek;
      return op;
   };

#define NB_COMMAND_MAX 0xff
   FDCCommand fdc_command_ [NB_COMMAND_MAX];

   enum Phase
   {
      INSTRUCTION,
      PARAMETER,
      TIMING_BEFORE,
      EXEC,
      TIMING_AFTER,
      RESULT,
      EXEC_PRECISE
   };

   Phase state_;
   unsigned char instruction_;
   bool mt_;
   bool mf_;
   bool sk_;

   bool motor_on_;

   bool first_sector_found_;
   int sector_count_;
   unsigned int nb_data_in_buffer_;
   unsigned int nb_data_offset_;
   unsigned char data_buffer_ [1/*32*1024*/];
   bool recalibrate_;

   unsigned char parameters_[9];
   unsigned char results_[7];
   unsigned char parameters_count_;
   unsigned char results_count_;
   unsigned int index_fdc_command_;
   unsigned int step_rate_;
   unsigned char track_seeded_;
   unsigned char step_count_;    // Number of pulse for changing track
   int move_size_;
   bool dma_disable_;

   // Status
   bool ma_;
   bool nw_;
   bool nd_;
   bool or_;
   bool de_;
   bool en_;

   unsigned char status_0_;
   unsigned char status_1_;
   unsigned char status_2_;
   unsigned char status_3_;

   unsigned char hu_;
   unsigned char tp_;
   unsigned char tr_;
   unsigned char hd_;

   unsigned char sc_;
   unsigned char init_r_;
   unsigned char sz_;
   unsigned char ls_;
   unsigned char gp_;
   unsigned char sl_;
   unsigned char fb_;
   unsigned char nm_;

   // Internal signals
   bool seek_cmd_;
   unsigned char interrupt_code_; // 2 bits only.
   bool seek_end_;

   bool force_nd_;
   unsigned char last_interrupt_result_;
   //


   // Disk buffer
   void CleanDisk ();


   //Disk m_CurrentDisk;
   bool interrupt_occured_;
   bool ready_line_changed_;
   unsigned char busy_;

   // Command functions
   void TimingBeforePhase ();
   void TimingAfterPhase ();
   void ExecPhase ();
   void ResultPhase ();

   void ReadTrack ();
   void SpecifySpdDma();
   void SenseDriveState ();
   void WriteSector ();
   void ReadSector ();
   void ReadSectorOLD ();
   void Recalib ();
   void SenseIntState ();
   void WrDeletedSec ();
   void ReadId();
   void RdDeletedSec();
   void FormatTrack();
   void SeekTrackN();
   void ScanEqual();
   void ScanLowOrEqual();
   void ScanHighOrEqual();


   // Registers
   unsigned char main_status_;
   unsigned char data_register_;

   // Inner values
   bool cb_;
   // Request for master : 1 = ready for next byte
   //
   bool rqm_;
   bool dio_;
   bool exm_;

   // Timing : Counter
   unsigned int delay_for_instruction_;
#if defined (__unix) || (RASPPI)
   uint64_t  time_, time_for_bad_instruction_;
#else
   unsigned __int64 time_;
   unsigned __int64 time_for_bad_instruction_;
#endif
   bool rw_command_;

   ////////////////////////////////////
   //
   // Extended FDC : Precise timings.

   int cnt_exec_;
   Phase old_state_;

   // Definition of commands
   typedef enum  {
            C_NONE,
            C_READ_TRACK,
            C_SPECIFY_SPDDMA,
            C_SENSE_DRIVE_STATE,
            C_WRITE_SECTOR,
            C_READ_SECTOR,
            C_RECALIB,
            C_SENSE_INT_STATE,
            C_WR_DELETED_SEC,
            C_READ_ID,
            C_DELETED_SEC,
            C_FORMAT_TRACK,
            C_SCAN_EQUAL,
            C_SCAN_LOW_OR_EQUAL,
            C_SCAN_HIGH_OR_EQUAL,
            C_SEEK_TRACK
            } Commands;
   Commands current_command_;

   bool CheckOverrun (bool read);
   bool SectorAlreadyExists ( unsigned int cpt, unsigned int side, unsigned int i, unsigned int s, unsigned int *covering, unsigned int last_valid_data);

   unsigned int index_hole_encountered_;

   bool read_deleted_;
   bool write_deleted_  ;
   bool tc_;

   unsigned short read_crc_;

   ////////////////////////////////////////
   // Precise Command timing
   // Seek track
   bool seek_track_;

   // Read ID
   void ReadIdTick ();
   
   // Read Track
   void ReadTrackTick ();

   // scan
   // comparison functions
   // Scan tick
   typedef bool (*ScanCompareFunc)(unsigned char dfdd, unsigned char dproc);
   unsigned char stp_;
   ScanCompareFunc scan_func_;
   void Scan();
   void ScanTick();

   // Read Sector
   void ReadSectorTick ();

   using SectorPhase = enum  {
      R_LOAD_HEAD,
      R_FIND_SECTOR,
      R_FIND_INDEX_HOLE,
      R_WAIT_FOR_BYTES,
      R_READ_BYTE,
      R_WRITE_BYTE,
      R_FORMAT,
      R_WAIT_FOR_NEXT_SECTOR,
      R_WAIT_GAP3,
      R_FIND_SYNC_FIELD,
      R_FIND_IDAM,
      R_FIND_ADRESS_MARK,
      R_READ_CHRN,
      R_CHECK_CRC,
      R_FIND_DAM,
      R_READ_DATA,
      R_PRE_DATA_WRITE,
      R_WRITE_DATA,
      R_WRITE_CRC
   };

   typedef enum
   {
      MFM_GAP4_A,
      MFM_SYNC1,
      MFM_IAM,
      MFM_IAM2,
      MFM_GAP1,
      MFM_SYNC2,
      MFM_IDAM,
      MFM_IDAM2,
      MFM_CHRN,
      MFM_CRC,
      MFM_GAP2,
      MFM_SYNC3,
      MFM_DAM,
      MFM_DAM2,
      MFM_DATA,
      MFM_CRC2,
      MFM_GAP3,
      MFM_GAB4_B,
      MFM_END
   } MFMPhase;

   // Write sector
   void WriteSectorTick ();

   // Format track
   void FormatTrackTick ();
   MFMPhase format_phase_;
   unsigned int format_sector_count_ ;

   unsigned int seek_count_;  // In 2us ticks
   void SeekTrackNTick();

   void SetFixedSpeed ( bool fixed_speed) { for (int i =0; i < NB_DRIVES; i++) disk_[i].SetFixedSpeed(fixed_speed);};

   /////////////////////////////////////////////////////////////////////
   // Disks
   DiskGen disk_ [NB_DRIVES];
   unsigned char sc_array_[NB_DRIVES];

   int current_drive_;

   CRC crc_;

   SectorPhase current_command_phase_;

   unsigned char previous_bit_;
   unsigned char sync_count_;
   unsigned char current_data_byte_;
   unsigned char bit_count_;
   unsigned int out_index_;
   unsigned int data_to_read_;
   unsigned int data_to_return_;
   bool first_non_0_ ;
   int read_sector_state_;
   bool read_track_ ;

   // Write data :
   int write_byte_counter_;

   void InitHandleSyncField ();
   bool HandleSyncField ();

   void InitHandleIDAM ();
   int HandleIDAM (unsigned char *next_byte);

   void InitHandleCHRN ();
   int HandleCHRN ( unsigned char* out_buffer );

   void InitHandleCRC ();
   int HandleCRC ( bool data = false );

   void InitHandleReadData (int size);
   int HandleReadData (unsigned char* out_buffer);

   void InitHandleLoadHead ();
   int HandleLoadHead ();

   void InitHandleFindIndexHole ();

   void InitHandlePreDataWrite ();
   int HandlePreDataWrite ();

   void InitHandleWriteData ();
   int HandleWriteData ();

   void InitHandleWriteCrc ();
   int HandleWriteCrc ();

   void InitHandleFormat (MFMPhase format_phase );
   int HandleFormat ();

   unsigned char byte_to_write_;
   bool byte_is_sync_;
   MFMPhase next_phase_;
   bool crc_on_;

   int chrn_byte_count_ ;
   int crc_cyte_;

   // Delayed loading
   bool delayed_load_;
   IDisk* disk_to_load_;
   int delayed_load_count_ ;
   unsigned int delayed_load_drive_ ;

   bool delayed_load_container_;
   std::string delayed_load_filepath_; // [MAX_PATH];
   
   bool on_index_;
   bool read_data_done_;
};
