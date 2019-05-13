#include "stdafx.h"
#include "FDC.h"

//#include <algorithm>
#include "simple_stdio.h"
#include "simple_vector.hpp"
#include "simple_math.h"

// Parameter definition for FDC commands

// NOT READY
#define NR 0x8


#define LOGFDC
#define LOG_SENSE_INT
#define LOG_EXEC
#define LOG_MAX

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

#define FDC_OUT_BUFFER_SIZE    1

#define SYNC_SIZE 0

#define MOVE_ONE_TRACK_LENGTH    512

#define SYNC_COUNT (12*8)

#define DELAY_INSTR 30

////////////////////////////////////////////////////////////////////////////////////////
// Result type
////////////////////////////////////////////////////////////////////////////////////////
#define R_S0     0x001
#define R_S1     0x002
#define R_S2     0x004
#define R_S3     0x008
#define R_TR     0x010
#define R_HD     0x020
#define R_NM     0x040
#define R_SZ     0x080
#define R_LS     0x100
#define R_TP     0x200

////////////////////////////////////////////////////////////////////////////////////////
// MACRO helper
////////////////////////////////////////////////////////////////////////////////////////

// **********
#define READ_END(EOT)\
   status_0_ |= (tc_?0:0x40);\
   interrupt_code_ = (tc_)?0:1;interrupt_occured_ = true;\
   if(EOT)en_ = true;\
   HandleResult(R_S0|R_S1|R_S2|R_TR|R_HD|R_LS|R_SZ);\
   dio_ = true;exm_=false;rqm_=true;\
   state_ = RESULT;\
   break;

#define READ_END_TRACK(EOT)\
   status_0_ |= (tc_?0:0x40);\
   interrupt_code_ = (tc_)?0:1;interrupt_occured_ = true;\
   if(EOT)en_ = true;/*m_Status1 |= 0x80;\*/\
   HandleResult(R_S0|R_S1|R_S2|R_TR|R_HD|R_NM|R_SZ);\
   /*results_[0] = m_Status0;*/\
   /*results_[1] = m_Status1;*/\
   /*results_[2] = m_Status2;*/\
   state_ = RESULT;\
   break;

// ********** Seek sector

#define DECODE_SIDE\
   disk_[parameters_[0]&0x1].SetSide((parameters_[0]&0x04)>>2); \
   disk_[parameters_[0] & 0x1].SetMfm(mf_ ? DiskGen::CODE_MFM : DiskGen::CODE_FM);\
   if ( !disk_[parameters_[0]&0x1].SideValid ())\
   {sz_= 0;status_0_= 0x44;int i =0;results_[i++] = status_0_;\
   results_[i++] = 0x80;\
   results_[i++] = status_2_;\
   results_[i++] = disk_[parameters_[0]&0x1].GetCurrentTrack();\
   results_[i++] = hd_;\
   results_[i++] = ls_;\
   results_[i++] = sz_;\
   interrupt_occured_ = true;\
   state_ = RESULT;return;\
   }


////////////////////////////////////////////////////////////////////////////////////////

FDC::FDC(void) : load_progress_(-1)
{
   delayed_load_ = false;
   delayed_load_container_ = false;
   notifier_ = NULL;
   log_ = NULL;
   seek_cmd_ = false;

   cnt_exec_ = 0;
   old_state_ = INSTRUCTION;

   for (int i = 0; i < NB_COMMAND_MAX; i++)
   {
      fdc_command_ [i].func = NULL;
      fdc_command_ [i].param_number = 0;
   }

   fdc_command_ [0x0] = FillStructFDCCommand (  0, 1, READ, NULL,                  0,    0);
   fdc_command_ [0x2] = FillStructFDCCommand (  8, 7, READ ,&FDC::ReadTrack,      0,    0, READWRITE );
   fdc_command_ [0x3] = FillStructFDCCommand (  2, 0, NONE ,&FDC::SpecifySpdDma,  0,    0 );
   fdc_command_ [0x4] = FillStructFDCCommand (  1, 1, NONE ,&FDC::SenseDriveState,0,    0 );
   fdc_command_ [0x5] = FillStructFDCCommand (  8, 7, WRITE,&FDC::WriteSector,    0,    0, READWRITE );
   fdc_command_ [0x6] = FillStructFDCCommand (  8, 7, READ ,&FDC::ReadSector,     0,    0, READWRITE );
   fdc_command_ [0x7] = FillStructFDCCommand (  1, 0, NONE ,&FDC::Recalib,        0,    0, RECAL );
   fdc_command_ [0x8] = FillStructFDCCommand (  0, 2, NONE ,&FDC::SenseIntState,  0,    0, SENSEINTSTATE );
   fdc_command_ [0x9] = FillStructFDCCommand (  8, 7, WRITE,&FDC::WrDeletedSec,   0,    0, READWRITE );
   fdc_command_ [0xA] = FillStructFDCCommand (  1, 7, NONE ,&FDC::ReadId,         0,    0, READWRITE );
   fdc_command_ [0xC] = FillStructFDCCommand (  8, 7, READ,&FDC::RdDeletedSec,    0,    0, READWRITE );
   fdc_command_ [0xD] = FillStructFDCCommand (  5, 7, WRITE,&FDC::FormatTrack,    0,    0, READWRITE );
   fdc_command_ [0xF] = FillStructFDCCommand (  2, 0, NONE ,&FDC::SeekTrackN,     0,    0, SEEK);
   fdc_command_ [0x11] = FillStructFDCCommand ( 8, 7, WRITE,&FDC::ScanEqual,      0,    0, READWRITE );
   fdc_command_ [0x19] = FillStructFDCCommand ( 8, 7, WRITE,&FDC::ScanLowOrEqual, 0,    0, READWRITE );
   fdc_command_ [0x1D] = FillStructFDCCommand ( 8, 7, WRITE,&FDC::ScanHighOrEqual,0,    0, READWRITE );

   for (int i = 0; i < NB_DRIVES; i++)
   {
      //disk_[i].Init ();
      disk_[i].Eject ();
   }

   current_drive_ = 0;
   Reset ();
}
void FDC::Init (DskTypeManager * disk_type_manager)
{
   for (int i = 0; i < NB_DRIVES; i++)
   {
      disk_[i].Init (disk_type_manager);
   }
}

void FDC::SetLog(ILog* log)
{
   log_ = log;
   for (int i = 0; i < NB_DRIVES; i++)
   {
      disk_[i].SetLog(log);
   }
}


void FDC::Reset ()
{
   seek_track_ = false;
   seek_count_ = 0;
   delay_for_instruction_ = 0;
   rqm_ = true;
   dio_ = false;
   exm_ = false;
   cb_ = false;

   data_register_ = 0;
   state_ = INSTRUCTION;
   nb_data_in_buffer_ = 0;
   instruction_ = 0;
   busy_ = 0;

   status_0_ = 0x48; // No disk
   ResetStatus1();
   status_2_ = 0;
   status_3_ = 0;

   hu_ = 0;
   tp_ = 0;
   tr_ = 0;
   hd_ = 0;


   sc_ = 0;

   for (int i =0; i < NB_DRIVES; i++)
      sc_array_[i] = 0;

   sz_ = 0;
   ls_ = 0;
   gp_ = 0;
   sl_ = 0;
   fb_ = 0;
   nm_ = 0;

   time_ = 0;
   time_for_bad_instruction_ = 0;
   interrupt_occured_ = false;

   current_command_ = C_NONE;
}


FDC::~FDC(void)
{
   for (int i = 0; i < NB_DRIVES; i++)
   {
      disk_[i].CleanDisk ();
   }
}

void FDC::SetMotor ( bool bOn)
{
   for (int i = 0; i < NB_DRIVES; i++)
   {
      disk_[i].StartMotor(bOn);
   }
}

void FDC::FlipDisk (unsigned int drive_number)
{
   disk_[drive_number].FlipDisk ();
}

void FDC::InsertBlankDisk ( unsigned int drive_number, IDisk::DiskType type)
{
   // If disk persent : Interrupt
   if ( disk_[drive_number].IsDiskPresent())
   {
      ready_line_changed_ = true;
   }

   disk_[drive_number].InsertBlankDisk(type);
}

void FDC::EjectDisk (unsigned int drive_number )
{
   // If disk persent : Interrupt
   if ( disk_[drive_number].IsDiskPresent())
   {
      if (notifier_ != NULL) notifier_->DiskEject();

      ready_line_changed_ = true;
   }

   disk_[drive_number].Eject();
}

// 0 : Not present;  1 : No; 2 : Read; 3 : Write
int FDC::IsRunning (int drive )
{
   // Exist ?
   if ( !disk_[drive].IsDiskPresent ())
      return 0;

   if (state_ != INSTRUCTION && current_drive_ == drive )
   {
      if (current_command_ == C_WRITE_SECTOR
         ||current_command_ == C_FORMAT_TRACK)
      {
         return 3;
      }
      else
      {
         return 2;
      }
   }


   return 1;
}



void FDC::Out (unsigned short addr, unsigned char data)
{
   // 0xFA7E
   if ( (addr &0x0580) ==0x0000 )
   {
      bool motor_on = ((data & 0x01) == 0x01);
      if (motor_on^motor_on_)
      {
         if (log_)
         {
            if (motor_on)
            {
               LOG("Motor ON");
               LOGEOL
            }
            else
            {
               LOG("Motor OFF");
               LOGEOL
            }
         }
         motor_on_ = motor_on;
      }

      for (int i = 0; i < NB_DRIVES; i++)
      {
         disk_[i].StartMotor(motor_on);
      }
   }
   //if (Addr_P == 0xFB7F)
   else if (( (addr &0x0581) ==0x0101 )
      ||((addr & 0x0581) == 0x0100))
   {
      // Data register
      if ((state_ == INSTRUCTION )
         &&
         // If bits 0-4 are set, do not accept read/write commands
         (  (!seek_track_)
         || (   (fdc_command_ [data&0x1F].transfer_type != WRITE)
         && (fdc_command_ [data&0x1F].transfer_type != READ)
         )
         )
         )
      {
         delay_for_instruction_ = DELAY_INSTR;
         rqm_ = false;
         time_ = 0;

         switch (fdc_command_[data&0x1F].seek_cmd)
         {
         case RECAL:
            seek_cmd_ = true;
            recalibrate_ = true;
            rw_command_ = true;
            last_interrupt_result_ = -1;
            LOG ("Instruction : ");
            LOGB (data);
            break;
         case READWRITE:
            recalibrate_ = false;
            seek_cmd_ = false;
            rw_command_ = true;
            LOG ("Instruction : ");
            LOGB (data);
            last_interrupt_result_ = -1;
            break;
         case SENSEINTSTATE:
            LOG("Instruction : SENSEINTSTATE ");
            break;
         default:
            LOG("Instruction : ");
            LOGB(data);
            last_interrupt_result_ = -1;
            rw_command_ = true;
         }

         // Is this command valid ?
         if ( (fdc_command_ [data&0x1F].func == NULL)
            // BUSY
            // SEEK TRACK
            || (   (seek_track_)
            && (   (fdc_command_ [data&0x1F].transfer_type == WRITE)
            || (fdc_command_ [data&0x1F].transfer_type == READ)
            )
            )
            )
         {
            // WTF ??
            results_count_ = 1;
            LOG ("FONCTION INCONNUE ! ");
            index_fdc_command_ = 0;
            status_0_ = 0x80;
            results_count_ = 1;
            results_[0] = status_0_ ;
            state_ = RESULT;

            LOGB (index_fdc_command_);
            LOGEOL
               LOG ("Result : ");

            time_for_bad_instruction_ = 32;
            //m_DIO = true;
            //m_RQM = true;
            exm_ = false;
            cb_ = true;
         }
         else
         {

            // Increment the weak cycle count
            tc_ = false;

            // Set main register
            exm_ = false;
            dio_ = false;
            //m_RQM = true;
            cb_ = true;

            // Command ?
            // Parameters :
            mt_ = (data & 0x80)==0x80;
            mf_ = (data & 0x40)==0x40;
            sk_ = (data & 0x20)==0x20;

            data &= 0x1F;

            instruction_ = data;
            parameters_count_ = 0;

            index_fdc_command_ = data;

            if ( fdc_command_[index_fdc_command_].param_number > 0)
            {
               state_ = PARAMETER;
               LOG ("Param : ");
            }
            else
            {
               TimingBeforePhase ();
            }
         }
      }
      else if (state_ == PARAMETER)
      {
         LOGB ( data );
         delay_for_instruction_ = DELAY_INSTR;
         rqm_ = false;
         parameters_[parameters_count_++] = data;

         if (parameters_count_ == fdc_command_[index_fdc_command_].param_number )
         {
            delay_for_instruction_ = 0;
            // Nothing to do as now
            LOGEOL
               TimingBeforePhase ();

         }
      }

      // Everything undesrstood ?
      else if (state_ == EXEC)
      {
         if (fdc_command_[index_fdc_command_].transfer_type == WRITE)
         {
#ifdef LOG_EXEC
            LOGB ( data );
#endif
            // Exec phase
            data_buffer_[nb_data_in_buffer_++] = data;
            rqm_ = false;
            if (nb_data_offset_ == nb_data_in_buffer_)
            {
#ifdef LOG_EXEC
               LOGEOL
#endif
                  TimingAfterPhase ();
            }
         }
         else
            if (fdc_command_[index_fdc_command_].transfer_type == READ)
            {
               // ACQ the reading... (test 0x10 from fdctest)
               if (nb_data_in_buffer_>0)
                  nb_data_in_buffer_--;
            }

      }
      else if ( state_ == EXEC_PRECISE)
      {
         if (fdc_command_[index_fdc_command_].transfer_type == WRITE)
         {
            data_buffer_[nb_data_in_buffer_++] = data;
#ifdef LOG_EXEC
            LOGB(data);
#endif
            rqm_ = false;
         }
         else
         {
            if (nb_data_in_buffer_>0)
               nb_data_in_buffer_--;
#ifdef LOG_EXEC
            LOGB(data);
#endif
            rqm_ = false;
         }
      }
      else
      {
         LOG("Unknown state : m_State = ");
         LOGB(state_)
            LOG(" - Mainstatus = ");
         LOGB(main_status_);
         LOG (" Data = ");
         LOGB ( data );
         LOGEOL

            // Set to result phase, with a '80' result
            exm_ = false;
         rqm_ = true;

         // Something to do ?
         results_count_ = 1;
         index_fdc_command_ = 0;
         results_[0] = 0x80;
         state_ = RESULT;
         LOGEOL
            LOG ("Result : ");
         dio_ = true;
      }
   }
}

unsigned char FDC::In ( unsigned short Addr_P )
{ 
   // %xxxxx0x1 0xxxxxx0
   //if (Addr_P == 0xFB7E )
   if ((Addr_P & 0x0581) == 0x0100)
   {
      // Build m_MainStatus:
      // todo : Use real values from diskgen

      //FDD3-0 Busy
      main_status_ =    busy_;
      // FDC Busy
      if (cb_) main_status_ |= 0x10;
      // EXecution Mode
      if (exm_) main_status_ |= 0x20;
      // Data Input/Output
      if (dio_) main_status_ |= 0x40;
      // ReQuest for Master
      if (rqm_) main_status_ |= 0x80;

      return main_status_;// |m_Busy;
   }
   //%xxxxx0x1 0xxxxxx1
   //else if (Addr_P == 0xFB7F )
   else if ((Addr_P & 0x0581) == 0x0101)
   {
      // Data register
      if (state_ == RESULT)
      {
         exm_ = false;
         if ( results_count_ > 0)
         {
            data_register_ = results_[fdc_command_ [index_fdc_command_].result_number - (results_count_--)];
            if ( current_command_ != C_SENSE_INT_STATE)
               LOGB ( data_register_ );
         }
         if ( results_count_ == 0)
         {
            if (current_command_ != C_SENSE_INT_STATE)
               LOGEOL

               // special case : If Seekend but no seek command, dont do this
               // After a while, reset m_bSeekEnd
               if ( time_ == 0) //!m_bSeekEnd  || (m_bSeekCmd))
                  cb_ = false;
            // if (!m_bSeekEnd) // FAUX !
            rqm_ = true;
            dio_ = false;
            //interrupt_occured_ = false;
            state_ = INSTRUCTION;
         }
      }
      else if ((state_ == EXEC) && (fdc_command_[index_fdc_command_].transfer_type == READ)
         &&  (current_command_ != C_READ_ID && current_command_ != C_READ_SECTOR) )
      {
         // Execute
         data_register_ = data_buffer_[nb_data_offset_++];
#ifdef LOG_EXEC
         if (current_command_ != C_SENSE_INT_STATE)
            LOGB ( data_register_ );
#endif

         // End of execution ?
         if (nb_data_offset_ == nb_data_in_buffer_)
         {
            results_count_ = fdc_command_ [index_fdc_command_].result_number;

#ifdef LOG_EXEC
            LOGEOL
#endif
               if ( dma_disable_)
                  exm_ = true;
               else
                  exm_ = false;
            // Go to result phase !
            TimingAfterPhase ();
         }
      }
      else if ( state_ == EXEC_PRECISE)
      {
         if (nb_data_in_buffer_ > FDC_OUT_BUFFER_SIZE)
         {
            // Wtf ?!
            int err = 1;
         }
         if ( dma_disable_)
            exm_ = true;
         else
            exm_ = false;
         data_register_ = data_buffer_[0];
#ifdef LOG_EXEC
         LOGB ( data_register_ );
#endif
         if (nb_data_in_buffer_>0)
            nb_data_in_buffer_--;
         rqm_ = false;
      }
      else
      {
         // TODO ??!!!
         data_register_ = GetStatus3 ();
      }

      return data_register_;
   }

   return 0; // ??

}

void FDC::TimingBeforePhase ()
{
   if ( current_command_ == C_READ_ID
      ||current_command_ == C_READ_SECTOR
      ||current_command_ == C_READ_TRACK
      ||current_command_ == C_WRITE_SECTOR
      ||current_command_ == C_FORMAT_TRACK
      /*||m_CurrentCommand == cSeekTrack*/)
   {
      return;
   }
   // Set timings
   /*m_Time = FdcCommand [m_IndexFDCCommand].TimingBefore ;
   if (m_Time == 0)*/
   {
      ExecPhase ();
   }
   /*   else
   {
   m_State = TimingBefore;
   }*/
}

void FDC::TimingAfterPhase ()
{
   // Set timings
   if (current_command_ == C_READ_ID ||current_command_ == C_READ_SECTOR ||current_command_ == C_READ_TRACK||current_command_ == C_WRITE_SECTOR
      ||current_command_ == C_FORMAT_TRACK/*||m_CurrentCommand == cSeekTrack*/) return;


   //m_Time = FdcCommand [m_IndexFDCCommand].TimingAfter ;
   //if (m_Time == 0)
   {
      ResultPhase ();
   }
   /*else
   {
   m_State = TimingAfter;
   }*/
}

void FDC::ExecPhase ()
{
   results_count_ = fdc_command_ [index_fdc_command_].result_number;
   state_ = EXEC;

   if (current_command_ != C_SENSE_INT_STATE)
      LOG ("Exec : ");

   nb_data_offset_ = nb_data_in_buffer_ = 0;
   if ( fdc_command_[index_fdc_command_].transfer_type == READ
      ||fdc_command_[index_fdc_command_].transfer_type == NONE)
   {
      if ( fdc_command_ [index_fdc_command_].func != NULL)
      {
         ((*this).*fdc_command_ [index_fdc_command_].func) ();
      }

      // Go to result phase !
      if (nb_data_offset_ == nb_data_in_buffer_)
      {
         TimingAfterPhase ();
      }
   }
   else
   {
      // Write
      if ( fdc_command_ [index_fdc_command_].func != NULL)
      {
         ((*this).*fdc_command_ [index_fdc_command_].func) ();
      }

   }
}

void FDC::ResultPhase ()
{
   exm_ = false;


   // Something to do ?
   results_count_ = fdc_command_ [index_fdc_command_].result_number;
   /*if (FdcCommand[m_IndexFDCCommand].TransferType == WRITE)
   {
   if ( FdcCommand [m_IndexFDCCommand].func != NULL)
   {
   ((*this).*FdcCommand [m_IndexFDCCommand].func) ();
   }
   }*/

   state_ = RESULT;
   if (current_command_ != C_SENSE_INT_STATE)
   {
      LOGEOL
         LOG("Result : ");
   }
   if ( results_count_ == 0)
   {
      // Finish ? Reset the status to available;
      if (current_command_ != C_SENSE_INT_STATE)
         LOGEOL

         // TO CHECK ! What about bit 1-5 ????
         //if ( !m_bSeekEnd  || (m_bSeekCmd))
         if (time_ == 0)
            cb_ = false;
      exm_ = false;
      dio_ = false;
      //if (!m_bSeekEnd)
      rqm_ = true;

      state_ = INSTRUCTION;
   }
   else
   {
      dio_ = true;
      rqm_ = true;
   }
}


int FDC::LoadDisk ( unsigned int drive_number, const char* file_path, bool delayed)
{
   delayed_load_drive_ = drive_number;
   if ( disk_[delayed_load_drive_].IsDiskPresent())
   {
      ready_line_changed_ = true;
      // Disk present : Eject, then wait a bit...
      EjectDisk (delayed_load_drive_);
   }

   delayed_load_filepath_ = file_path;
   if (delayed)
   {
      // Set delayed load :
      delayed_load_container_ = false;

      delayed_load_ = true;
      delayed_load_count_ = 5000;
   }
   else
   {
      return disk_[drive_number].LoadDisk (file_path);
   }

   return 0;
}

int FDC::LoadDisk ( unsigned int drive_number, DataContainer* containter, bool delayed)
{
   delayed_load_drive_ = drive_number;
   if ( disk_[delayed_load_drive_].IsDiskPresent())
   {
      ready_line_changed_ = true;
      // Disk present : Eject, then wait a bit...
      EjectDisk (delayed_load_drive_);
   }
   delayed_load_filepath_ = containter->GetCurrentPath();
   if (delayed)
   {
      // Set differential load :
      delayed_load_container_ = true;

      std::vector<IDisk*> disk_list = disk_[drive_number].CreateDisk(containter, this, log_);
      // Load
      if (disk_list.size() == 0) return -1;
      disk_to_load_ = disk_list[0];

      delayed_load_ = true;
      delayed_load_count_ = 5000;
   }
   else
   {
      return disk_[drive_number].LoadDisk (containter);
   }

   return 0;
}

int FDC::LoadDiskDelayed ( )
{
   ready_line_changed_ = true;

   int ret =  (delayed_load_container_)?disk_[delayed_load_drive_].LoadDisk (disk_to_load_):disk_[delayed_load_drive_].LoadDisk (delayed_load_filepath_.c_str());
   delayed_load_ = false;
   load_progress_ = -1;
   return ret;
}

int FDC::CompareDriveAandB()
{
   return disk_[0].CompareToDisk ( &disk_[1] );
}


const char* FDC::GetDiskPath (unsigned int drive_number)
{
   // Disk path
   return disk_[drive_number].GetCurrentLoadedDisk ();
}

bool FDC::IsDiskPresent ( int drive )
{
   return disk_[drive].IsDiskPresent ();
}

void FDC::WriteDisk ( unsigned int drive_number)
{
   disk_[drive_number].WriteDisk ();
}

void FDC::WriteDiskAs( unsigned int drive_number, const char* file_path, FormatType* format)
{
   disk_[drive_number].WriteDisk (file_path, format);
}

///////////////////////////////////////////
// Main ticker
unsigned int FDC::Tick( )
{
   // Delayed load ?
   if ( delayed_load_ )
   {
      --delayed_load_count_;
      if ( delayed_load_count_ == 0)
      {
         // Load the disk
         int ret = LoadDiskDelayed();

         // Notify the disk loaded
         if (notifier_ != NULL) notifier_->ItemLoaded( delayed_load_filepath_.c_str(), ret, delayed_load_drive_);
      }


   }

   /*if (m_bReady != disk_[current_drive_].IsDiskReady())
   {
   m_bReadyLineChanged = true;
   m_bReady = disk_[current_drive_].IsDiskReady();
   }*/


   // Compute the head.
   // Theory : If the motor is on, one impulse = 2 us
   // MFM type : One bit is 2 impulse
   // One byte is 16 impulse, so 32 us.
   // On track is run 300 time per second.
   // So : One track is DEFAULT_TRACK_SIZE byte max.

   // Here : Handle 1 MFM impulse

   for (int i = 0; i < NB_DRIVES; i++)
   {
      disk_[i].Tick ();
   }

   // Seek track ?
   if (seek_track_)
   {
      // TODO
      SeekTrackNTick ();
   }
   if (time_>0)
   {
      --time_;
      if (time_ == 0 && state_==INSTRUCTION) cb_ = false;
   }

   if (time_for_bad_instruction_ > 0)
   {
      --time_for_bad_instruction_;
      if (time_for_bad_instruction_ == 0)
      {
         dio_ = true;
         rqm_ = true;
      }
   }

   if (delay_for_instruction_>0)
   {
      --delay_for_instruction_;
      if (delay_for_instruction_ == 0 ) rqm_ = true;
   }

   // Something to do ??
   if ((state_ == EXEC
      || state_ == EXEC_PRECISE)
      && (current_command_ == C_READ_ID
      ||current_command_ == C_READ_SECTOR
      ||current_command_ == C_READ_TRACK
      ||current_command_ == C_WRITE_SECTOR
      ||current_command_ == C_FORMAT_TRACK
      ||current_command_ == C_SCAN_EQUAL
      ||current_command_ == C_SCAN_LOW_OR_EQUAL
      ||current_command_ == C_SCAN_HIGH_OR_EQUAL))
   {
      // Fix cosa nostra TODO : To check...
      // On the WRITE, it's not correct to wait this long (sphaira and unique dont work anymore)
      if (old_state_ != EXEC && old_state_ != EXEC_PRECISE)
      {
         old_state_ = state_;
         cnt_exec_ = 0;
      }

      if (cnt_exec_ == 32 || current_command_ == C_WRITE_SECTOR)
      {
         if ( dma_disable_)
            exm_ = true;
         else
            exm_ = false;
      }
      cnt_exec_++;


      if (disk_[current_drive_].NewBitAvailable ())
      {

         switch (current_command_)
         {
         case C_READ_ID:        ReadIdTick ();       break;
         case C_READ_SECTOR :   ReadSectorTick ();   break;
         case C_READ_TRACK:     ReadTrackTick ();    break;
         case C_WRITE_SECTOR:   WriteSectorTick();   break;
         case C_FORMAT_TRACK:   FormatTrackTick ();  break;
         case C_SCAN_EQUAL:
         case C_SCAN_LOW_OR_EQUAL:
         case C_SCAN_HIGH_OR_EQUAL:   ScanTick ();  break;

            //            case cSeekTrack:     SeekTrackNTick ();   break;
            // Nothing to do !
         case C_NONE:
         default:
            break;
         }
      }
   }
   else if (state_ == RESULT &&  ( current_command_ == C_READ_ID
      ||current_command_ == C_READ_SECTOR
      ||current_command_ == C_READ_TRACK
      ||current_command_ == C_WRITE_SECTOR
      ||current_command_ == C_FORMAT_TRACK
      ))
   {
      ResultPhase ();
      current_command_ = C_NONE;
   }
   old_state_ = state_;

   return this_tick_time_ = 8;
}


bool FDC::CheckOverrun (bool bRead)
{
   if ( ((bRead==false) && (nb_data_in_buffer_ == 0))
      ||((bRead==true) && (nb_data_in_buffer_ != 0))
      )
   {
      // Overrun
      int i = 0;
      status_1_ |= 0x10;
      results_[i++] = status_0_;
      results_[i++] = status_1_;
      results_[i++] = status_2_;
      results_[i++] = tr_;
      results_[i++] = hd_;
      results_[i++] = ls_;
      results_[i++] = sz_;
      state_ = RESULT;
      return true;
   }
   return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// COMMANDS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
// Read ID
void FDC::ReadId() // 4a
{
   seek_end_ = false;
   current_command_ = C_READ_ID;

   // No Index Hole encounter
   index_hole_encountered_ = 0;
   status_0_ = status_2_ = 0;
   ResetStatus1();
   ma_ = true;

   // US1 not connected ( -> Sphaira)
   hu_ = parameters_[0]&0x3; // Drive number
   current_drive_ = hu_&0x1; // Only one drive connected !
   status_0_ = parameters_[0] & 0x07;
   rqm_ = false;
   DECODE_SIDE

      if (!disk_[current_drive_].IsDiskReady())
      {
         // No disk
         status_0_ = 0x48;
         int i =0;
         results_[i++] = status_0_; // m_Status0;
         results_[i++] = status_1_;
         results_[i++] = status_2_;
         results_[i++] = tr_;// disk_[current_drive_].GetCurrentTrack() /*m_CurrentTrack*/; //TR;
         results_[i++] = hd_;
         results_[i++] = ls_;
         results_[i++] = sz_;

         interrupt_occured_ = true;
         sz_ = 0;
         dio_ = true; exm_ = false; rqm_ = true;
         state_ = RESULT;
         return;
      }

      status_1_ = 0x01;   // MA

      // Begin the command, by finding IDAM
      InitHandleSyncField ();
      on_index_ = false;

      if ( disk_[current_drive_].NewBitAvailable ())
         ReadIdTick ();
}

void FDC::ReadIdTick ()
{
   unsigned char car_buffer;
   // One more tick on the Read ID command
   bool bOnIndex = disk_[current_drive_].IsHeadOnIndex ();
   if ( bOnIndex && (!on_index_)/*disk_[current_drive_].IsHeadOnIndex ()*/ )
   {
      // Yes, increment counter.
      ++index_hole_encountered_;

      // Encountered twice ?
      if (index_hole_encountered_ == 2)
      {
         // Missing Adress Mark, end of command.
         //m_Status1 = 0x01;   // MA

         status_0_ &= 0x7F;
         status_0_ |= 0x40;

         results_[0] = status_0_; // m_Status0;
         results_[1] = status_1_;
         results_[2] = status_2_;
         results_[3] = tr_;// disk_[current_drive_].GetCurrentTrack() /*m_CurrentTrack*/; //TR;
         // Result phase
         interrupt_occured_ = true;
         dio_ = true; exm_ = false; rqm_ = true;
         state_ = RESULT;
         return;
      }
   }
   if (!disk_[current_drive_].IsDiskReady())
   {
      // todo
      // No disk
      status_0_ = 0x48;
      status_1_ = 0x00;
      int i = 0;
      results_[i++] = status_0_; // m_Status0;
      results_[i++] = status_1_;
      results_[i++] = status_2_;
      results_[i++] = disk_[current_drive_].GetCurrentTrack() /*m_CurrentTrack*/; //TR;
      results_[i++] = hd_;
      results_[i++] = ls_;
      results_[i++] = sz_;
      interrupt_occured_ = true;
      dio_ = true; exm_ = false; rqm_ = true;
      state_ = RESULT;
      sz_ = 0;
      return;
   }
   //else
   {
      switch (current_command_phase_)
      {
      case R_FIND_SYNC_FIELD:
         if ( HandleSyncField ())
         {
            InitHandleIDAM ();
         }
         break;
      case R_FIND_IDAM:
      case R_FIND_ADRESS_MARK:
         switch ( HandleIDAM (&car_buffer) )
         {
         case 0: // Nothing to do
            break;
         case 1:
            if ( car_buffer == 0xFE)
            {
               ma_ = false;
               status_1_ &= (~0x1);    // At least one is found
               InitHandleCHRN ();
            }
            else
            {
               InitHandleSyncField ();
            }
            break;
         case 2:
            InitHandleSyncField ();
            break;
         };
         break;
      case R_READ_CHRN:
         {
            switch (HandleCHRN ( &results_[3] ))
            {
            case 0:
            case 1: // Nothing
               break;
            case 2: // Check CRC
               // Update TR, HD, LS, Sz
               tr_ = results_[3];
               hd_ = results_[4];
               sc_ = results_[5];
               sz_ = results_[6];
               InitHandleCRC ();
               break;
            case 3: // Error !? TODO ??
               break;
            }
         }
         break;
      case R_CHECK_CRC:
         {
            unsigned int retCrc = HandleCRC ();
            if (retCrc  != 0)
            {
               if (retCrc == 2)
               {
                  status_1_ |= 0x4;
                  nd_ = true;
               }
               // Let's have a return !
               results_[0] = status_0_; // m_Status0;
               results_[1] = status_1_;
               results_[2] = status_2_;

               interrupt_code_ = 0x0;
               interrupt_occured_ = true;
               HandleResult(R_S0 | R_S1 | R_S2 | R_TR | R_HD | R_LS | R_SZ);
               state_ = RESULT;

            }
         }
         break;
      default:
         // ERROR !
         return;
      }
   }
   on_index_ = bOnIndex;
}

///////////////////////////////////////////
// cReadSector
void FDC::ReadSector ()   // 6
{
   seek_end_ = false;
   read_deleted_  = false;
   current_command_ = C_READ_SECTOR;
   nb_data_in_buffer_ = 0;
   status_0_ = status_2_ = 0;
   ResetStatus0();
   ResetStatus1();

   rqm_ = false;
   dio_ = true;
   //m_Status1 = 0x4;  // ND by default - No IDAM Found by default
   nd_ = true;

   // TODO : sanity check
   DECODE_SIDE
   status_0_ = parameters_[0] & 0x07;

   hu_ = parameters_[0]&0x3; // Drive number
   current_drive_ = hu_&0x1;
   tr_ = parameters_[1] ;  // TR
   hd_ = parameters_[2] ;  // HD
   init_r_ = sc_ = parameters_[3] ;  // Sz
   sc_array_[current_drive_&1] = sc_;

   sz_ = parameters_[4] ;  // Sz
   ls_ = parameters_[5] ;  // LS - EOT
   gp_ = parameters_[6] ;  // NM
   sl_ = parameters_[7] ;  // SL

   if (!disk_[current_drive_].IsDiskReady())
   {
      interrupt_code_ = 0x1;
      nd_ = false;
      HandleResult (R_S0|R_S1|R_S2|R_TR|R_HD|R_LS|R_SZ);
      sz_ = 0;
      state_ = RESULT;
      return;

   }
   read_track_ = false;
   read_sector_state_ = 0;
   index_hole_encountered_ = 0;
   on_index_ = false;
   InitHandleLoadHead ();
}

void FDC::ReadSectorTick ()   // 6
{
   // One more tick on the Read ID command
   bool bOnIndex = disk_[current_drive_].IsHeadOnIndex ();
   if ((bOnIndex && (!on_index_)) && current_command_phase_ == R_READ_DATA)
   {
      LOG("[[ ** INDEX ** ]]")
      LOGEOL
   }

   if ( ( bOnIndex && (!on_index_)) && ( current_command_phase_ != R_READ_DATA/*read_sector_state_ == 0*/) )
   {

      // Yes, increment counter.
      ++index_hole_encountered_;
      // Encountered twice ?
      if (index_hole_encountered_ >= 2)
      {
         if (!nd_)
         {
            status_2_ &= ~0x2;
         }

         interrupt_code_ = 0x1;


         interrupt_occured_ = true;
         HandleResult (R_S0|R_S1|R_S2|R_TR|R_HD|R_LS|R_SZ);
         sz_ = 0;

         // Missing Adress Mark, end of command.
         //m_Status0 &= 0x7F;
         //if (!read_track_) // TODO :check this ??
         //m_Status0 |= 0x40;
         /*
         results_[0] = m_Status0; // m_Status0;
         results_[1] = m_Status1;
         results_[2] = m_Status2;
         results_[3] = disk_[current_drive_].GetCurrentTrack () ; //TR;
         //results_[3] = TR;
         results_[4] = HD;
         results_[5] = SC;
         results_[6] = Sz;

         */
         // Result phase
         on_index_ = bOnIndex;
         state_ = RESULT;
         return;
      }
   }
   on_index_ = bOnIndex;
   unsigned char car_buffer;
   switch (current_command_phase_)
   {
   case R_FIND_INDEX_HOLE:
      if (bOnIndex)
      {
         /*if (read_track_)
         {
         ++index_hole_encountered_;
         }*/
         InitHandleSyncField ();
      }
      break;
      // Load the head
   case R_LOAD_HEAD:
      if (HandleLoadHead () == 1)
      {
         // Arrived : Check now the sector
         index_hole_encountered_ = 0;
         if (read_track_)
         {
            // Waiting for index hole
            InitHandleFindIndexHole ();
         }
         else
         {
            InitHandleSyncField ();
         }
      }
      break;

      //case rFindSector:
   case R_FIND_SYNC_FIELD:
      if ( HandleSyncField ())
      {
         InitHandleIDAM ();
      }
      break;
   case R_FIND_IDAM:
   case R_FIND_ADRESS_MARK:
      switch ( HandleIDAM (&car_buffer) )
      {
      case 0: break;
      case 1:

         if (read_sector_state_ == 0 && car_buffer == 0xFE)
         {
            InitHandleCHRN ();
         }
         else if (read_sector_state_ == 1 )
         {
            if (car_buffer == 0xF8 || car_buffer == 0xFB /*|| read_track_*/)
            {
               if (car_buffer == 0xF8 || car_buffer == 0xFB)
               {
                  status_2_ &= (~0x1);
                  //m_Status1 &= (~0x1);
                  ma_ = false;

               }
               // TODO : Rework this
               // It seems the following behaviour is used :

               // - If Read normal and F8, or Read deleted and Fb, same behaviour occurs :
               if (((!read_deleted_ && car_buffer == 0xF8)
                  || (read_deleted_ && car_buffer == 0xFB)
                  )
                  )
               {
                  status_2_ |= 0x40;      // Set control mark
                  //  - If Sk = 1 : skip.
                  if (sk_)
                  {
                     //
                     if ( results_[5] == ls_)
                     {
                        //results_[5] = parameters_[3]; //
                        READ_END(true);
                     }
                     else
                     {
                        // Seek next sector
                        sc_++;
                        sc_array_[current_drive_&1] = sc_;
                        index_hole_encountered_ = 0;
                        read_sector_state_ = 0;
                        InitHandleSyncField ();
                     }
                  }
                  else
                  {
                     //  - Otherwise, dont skip.
                     status_2_ |= 0x40;      // Set control mark
                     tc_ = true;
                     //int size = 0x80<<(( Sz == 0 )?SL:Sz);
                     int size = (0x80 << (std::min(sz_, (unsigned char)0x8)));
                     data_to_return_ = (sz_ == 0) ? (((sl_ & 1) == 0) ? 0x50 : 0x28) : (0x80 << (std::min(sz_, (unsigned char)0x8)));
                     InitHandleReadData ( size );
                  }
               }
               else
               {
                  // Normal behaviour
                  //int size = 0x80<<(( Sz == 0 )?SL:Sz);
                  int size = (0x80 << (std::min(sz_, (unsigned char)0x8)));
                  data_to_return_ = (sz_ == 0) ? (((sl_ & 1) == 0) ? 0x50 : 0x28) : (0x80 << (std::min(sz_, (unsigned char)0x8)));
                  //int size = (Sz == 0) ? SL : (0x80 << Sz);
                  InitHandleReadData ( size );
               }
            }
            else
            {
               // No adress mark here...
               status_2_ |= 0x1;
               READ_END(false);
            }
         }
         else
         {
            read_sector_state_ = 0;
            InitHandleSyncField ();

         }
         break;
      case 2:
         read_sector_state_ = 0;
         InitHandleSyncField ();
         break;

      };
      break;
   case R_READ_CHRN:
      {
         switch (HandleCHRN ( &results_[3] ))
         {
         case 0:
         case 1: // Nothing
            break;
         case 2: // Check CRC
            // Check WC and BC flags of status 2
            // Set the next phase, if CHRN are ok :
            if (
               results_[4] == hd_
               && results_[5] == sc_
               && results_[6] == sz_
               )
            {
               if (results_[3] == tr_)
               {
                  //m_Status1 &= (~0x4); // Remove the ND flag
                  nd_ = false;
                  //m_Status1 |= 0x1;
                  ma_ = true;

                  InitHandleCRC ();
               }
               else
               {
                  if (results_[3] == 0xFF)
                  {
                     // BC set !
                     status_2_ |= 0x2;
                  }
                  // WC is set
                  status_2_ |= 0x10;

                  InitHandleSyncField ();
               }
            }
            else // Otherwise, back to search of sync bytes
            {
               InitHandleSyncField ();
            }

            break;
         case 3: // Error !? TODO ??
            break;
         }
      }
      break;
   case R_CHECK_CRC:
      {
         unsigned int retCrc = HandleCRC (read_sector_state_==1);
         if (retCrc != 0)
         {
            // Let's have a return !
            results_[0] = status_0_; // m_Status0;
            results_[1] = status_1_;
            results_[2] = status_2_;

            if (read_sector_state_ == 0)
            {
               read_sector_state_ = 1; // DATA phase
               InitHandleSyncField ();
            }
            else
            {
               // How to handle end of reading ?
               //if ( (m_Status1 & 0x20) == 0x20 && !read_track_)
               if (de_ && !read_track_)
               {
                  //results_[5] = parameters_[3]; //

                  READ_END(false);
               }
               else
                  if ((status_2_ & 0x40) == 0x40 && !sk_)
                  {
                     ls_ = sc_;
                     READ_END(false);
                  }
                  else if ( results_[5] == ls_ && !read_track_)
                  {
                     // MT ?
                     //if (m_MT && disk_[current_drive_].GetSide () == 0)
                     if (mt_ && ((hd_ &1)== 0))
                     {
                        mt_ = false;
                        nd_ = true;
                        // Invert head
                        // Read sector 0 to LS-first
                        hd_ ++;
                        // Invert side

                        disk_[current_drive_].SetSide (disk_[current_drive_].GetSide()==0?1:0);

                        if (init_r_ != 1 )force_nd_ = true;
                        sc_ = 1;
                        //LS = LS - m_InitR+SC;

                        if (!read_track_ )
                        {
                           index_hole_encountered_ = 0;
                        }
                        read_sector_state_ = 0;
                        InitHandleSyncField ();
                     }
                     else
                     {
                        if (or_)
                        {
                           READ_END(false);
                        }
                        else
                        {
                           READ_END(true);
                        }
                     }
                  }
                  else
                  {
                     // Seek next sector
                     sc_++;
                     nd_ = true;
                     sc_array_[current_drive_&1] = sc_;
                     if (!read_track_ )
                     {
                        index_hole_encountered_ = 0;
                     }
                     read_sector_state_ = 0;
                     InitHandleSyncField ();
                  }

            }
         }
      }
      break;


   case R_READ_DATA:
      // Test : Are-we on index hole ?
      /*if ( read_track_ == true && disk_[current_drive_].IsHeadOnIndex () )
      {
      // Stop sending information.
      READ_END(false)
      }
      else*/
      {
         switch ( HandleReadData (data_buffer_) )
         {
         case 0:
            break;
         case 1:

            // One data is available
            if ( nb_data_in_buffer_ > 0 )
            {
               // Overrun
               //m_Status1 |= 0x10;
               or_ = true;
               /*                  m_Status0 |= 0x40;
               m_Status0 &= ~0x80;*/

               /*                  if ( true )
               {
               interrupt_occured_ = true;
               READ_END(false);
               }*/

               /*
               LOG(_T("("));
               LOGB(m_DataBuffer[0]);
               LOG(_T(")"));*/
            }
            if ( tr_ == 03)
            {
               int bdg = 1;
            }
            nb_data_in_buffer_ = 1;

            state_ = EXEC_PRECISE;
            rqm_ = true;

            // TO REMOVE ???
            // m_EXM = true;

            break;
         case 2:
            // One data, then go to CRC check
            nb_data_in_buffer_ = 1;
            state_ = EXEC_PRECISE;
            rqm_ = true;
            // Check for overrun - TODO
            // CRC Check
            InitHandleCRC ();
            break;
         }
         break;
   default:
      // ERROR !
      break;
      }
   }
}

///////////////////////////////////////////
// cSpecifySPDDMA
void FDC::SpecifySpdDma()
{
   // b 4..7 : steprate (16-n)*2ms
   unsigned char n = parameters_[0] >> 4;
   step_rate_ = (16-n);
   dma_disable_ = parameters_[1]&0x1;

   dma_disable_  = true;
}

///////////////////////////////////////////
// cSenseDriveState

void FDC::SenseIntState () // 08
{
   int i = 0;

   status_0_ = 0;
   current_command_ = C_SENSE_INT_STATE;
   if (!disk_[current_drive_].IsDiskReady(true) && (recalibrate_ ))
   {
      if (recalibrate_)
      {
         // Not ready during seek :
         // -
         tp_ = 0;
         time_ = 0;
         interrupt_code_ = 0x1;
         seek_end_ = true;
         rw_command_ = true;
         HandleResult(R_S0 | R_TP);
         seek_end_ = false;
         seek_track_ = false;
         recalibrate_ = false;

         busy_ = 0;

#ifdef LOG_SENSE_INT
         if (last_interrupt_result_ != results_[0])
         {
            LOGEOL
               LOG("Instruction : 08 - Result : ");
            last_interrupt_result_ = results_[0];
            LOGB(last_interrupt_result_);

         }
#endif

         return;
      }
   }

   if (!interrupt_occured_)
   {
      //m_Status0 |= 0x80;
      results_count_ = 1;

      index_fdc_command_ = 0;

      if (!disk_[current_drive_].IsDiskReady((!seek_track_)) && !seek_track_)
      {
         interrupt_code_ = 0x2;
      }
      else
      {
         interrupt_code_ = 0x2;
      }
      state_ = RESULT;

      dio_ = true;
   }
   else
   {
      busy_ = 0;
      //interrupt_code_ = 0x0;
      interrupt_occured_ = false;
      state_ = RESULT;

      dio_ = true;
      recalibrate_ = false;
   }

   if (seek_end_)
   {
      main_status_ &= ~0x0F;
   }

   if (ready_line_changed_)
   {
      status_0_ &= (~0xE0);
      status_0_ |= 0x0C0;
      ready_line_changed_ = false;
      //interrupt_code_ = 0x3;

   }

   HandleResult( R_S0|R_TP);
   seek_end_ = false;


#ifdef LOG_SENSE_INT
   if (last_interrupt_result_ != results_[0])
   {
      LOGEOL
         LOG("Instruction : 08 - Result : ");
      last_interrupt_result_ = results_[0];
      for (int i = 0; i < results_count_; i++)
         LOGB(results_[i]);

   }
#endif

}
///////////////////////////////////////////
// cWriteSector
void FDC::WriteSector ()
{
   seek_end_ = false;
   write_deleted_  = false;
   current_command_ = C_WRITE_SECTOR;
   current_command_phase_ = R_LOAD_HEAD;
   nb_data_in_buffer_ = 0;
   status_0_ = status_2_ = 0;
   ResetStatus1();

   dio_ = false;
   rqm_ = false;
   nd_ = true;

   state_ = EXEC_PRECISE;
   // TODO : sanity check
   DECODE_SIDE

      status_1_ |= disk_[current_drive_].IsWriteProtected()?0x02:0;
   status_0_ = parameters_[0] & 0x07;
   hu_ = parameters_[0]&0x3; // Drive number
   current_drive_ = hu_&0x1;
   tr_ = parameters_[1] ;  // TR
   hd_ = parameters_[2] ;  // HD
   sc_ = parameters_[3] ;  // Sz
   sc_array_[current_drive_&1] = sc_;
   sz_ = parameters_[4] ;  // Sz
   ls_ = parameters_[5] ;  // LS
   gp_ = parameters_[6] ;  // NM
   sl_ = parameters_[7] ;  // SL

   if (!disk_[current_drive_].IsDiskReady())
   {
      // No disk
      status_0_ = 0x48;
      status_1_ = 0x00;
      int i =0;
      results_[i++] = status_0_; // m_Status0;
      results_[i++] = status_1_;
      results_[i++] = status_2_;
      results_[i++] = disk_[current_drive_].GetCurrentTrack () /*m_CurrentTrack*/; //TR;
      results_[i++] = hd_;
      results_[i++] = ls_;
      results_[i++] = sz_;
      interrupt_occured_ = true;
      dio_ = true; exm_ = false; rqm_ = true;
      state_ = RESULT;
      exm_ = false;
      sz_ = 0;

   }
   else
      if (disk_[current_drive_].IsWriteProtected())
      {
         // No disk
         status_0_ = 0x80;
         status_1_ = 0x02;
         int i =0;
         results_[i++] = status_0_; // m_Status0;
         results_[i++] = status_1_;
         results_[i++] = status_2_;
         results_[i++] = disk_[current_drive_].GetCurrentTrack () /*m_CurrentTrack*/; //TR;
         results_[i++] = hd_;
         results_[i++] = ls_;
         results_[i++] = sz_;
         interrupt_occured_ = true;
         dio_ = true; exm_ = false; rqm_ = true;
         state_ = RESULT;
         sz_ = 0;

      }
      /*if ( TR == 0x03  && current_drive_ == 1)
      {
      disk_[current_drive_].DumpTrack ( 0,3);
      }*/

}


void FDC::WriteSectorTick ()
{
   // 1/ Look for proper sector
   // 2/ Get ready to write something
   // 3/ Write it
   // 4/ Add CRC
   // 5/ Finish command


   // One more tick on the Write Sector command
   bool bOnIndex = disk_[current_drive_].IsHeadOnIndex ();
   if ( ( bOnIndex && (!on_index_))  )
   {
      // Yes, increment counter.
      LOG( " Pos = 0 ->");
      LOGB( disk_[current_drive_].GetPos ())
         LOG( " ");
      LOG( "index_hole_encountered_++ ");
      ++index_hole_encountered_;
      // Encountered twice ?
      if (index_hole_encountered_ == 2)
      {
         LOG( "index_hole_encountered_ == 2");
         // Missing Adress Mark, end of command.
         status_1_ |= 0x04;   // No Data
         nd_ = true;

         status_0_ &= 0x7F;
         status_0_ |= 0x40;

         results_[0] = status_0_; // m_Status0;
         results_[1] = status_1_;
         results_[2] = status_2_;

         if (!nd_)
         {
            status_2_ &= ~0x2;
         }
         interrupt_code_ = 0x1;
         interrupt_occured_ = true;
         HandleResult(R_S0 | R_S1 | R_S2 | R_TR | R_HD | R_LS | R_SZ);

         // Result phase
         state_ = RESULT;
         return;
      }
   }
   if (!disk_[current_drive_].IsDiskReady())
   {
      // todo
      // No disk
      status_0_ = 0x48;
      status_1_ = 0x00;
      int i = 0;
      results_[i++] = status_0_; // m_Status0;
      results_[i++] = status_1_;
      results_[i++] = status_2_;
      results_[i++] = disk_[current_drive_].GetCurrentTrack() /*m_CurrentTrack*/; //TR;
      results_[i++] = hd_;
      results_[i++] = ls_;
      results_[i++] = sz_;
      interrupt_occured_ = true;
      dio_ = true; exm_ = false; rqm_ = true;
      state_ = RESULT;
      sz_ = 0;
      return;
   }
   on_index_ = bOnIndex;

   unsigned char car_buffer;
   switch (current_command_phase_)
   {
      // Load the head
   case R_LOAD_HEAD:
      if (HandleLoadHead () == 1)
      {
         LOG( "HandleLoadHead");
         // Arrived : Check now the sector
         index_hole_encountered_ = 0;
         InitHandleSyncField ();
      }
      break;

      //case rFindSector:
   case R_FIND_SYNC_FIELD:
      if ( HandleSyncField ())
      {
         LOG( "Sync");
         InitHandleIDAM ();
      }
      break;
   case R_FIND_IDAM:
   case R_FIND_ADRESS_MARK:
      switch ( HandleIDAM (&car_buffer) )
      {
      case 0: break;
      case 1:
         if (car_buffer == 0xFE)
         {
            LOG( "IDAM FE");
            InitHandleCHRN ();
         }
         else
         {
            InitHandleSyncField ();
         }
         break;
      case 2:
         InitHandleSyncField ();
         break;
      };
      break;
   case R_READ_CHRN:
      {
         switch (HandleCHRN ( &results_[3] ))
         {
         case 0:
         case 1: // Nothing
            break;
         case 2: // Check CRC
            LOG( "CHRN");
            LOGB (results_[3]);
            LOGB (results_[4]);
            LOGB (results_[5]);
            LOGB (results_[6]);
            LOGEOL
               // Set the next phase, if CHRN are ok :
               if (
                  /*results_[3] == TR
                  &&*/ results_[4] == hd_
                  && results_[5] == sc_
                  && results_[6] == sz_
                  )
               {
                  if (results_[3] == tr_)
                  {
                     rqm_ = true;
                     nd_ = false;
                     ma_ = true;
                     LOG("Trouve");
                     status_1_ &= (~0x4); // Remove the ND flag
                     InitHandleCRC();
                  }
                  else
                  {
                     if (results_[3] == 0xFF)
                     {
                        // BC set !
                        status_2_ |= 0x2;
                     }
                     // WC is set
                     status_2_ |= 0x10;
                     InitHandleSyncField();
                  }
               }
               else // Otherwise, back to search of sync bytes
               {
                  LOG( "Next");
                  InitHandleSyncField ();
               }

               break;
         case 3: // Error !? TODO ??
            break;
         }
      }
      break;
   case R_CHECK_CRC:
      {
         unsigned int retCrc = HandleCRC (false);
         if (retCrc != 0)
         {
            // Let's have a return !
            results_[0] = status_0_; // m_Status0;
            results_[1] = status_1_;
            results_[2] = status_2_;

            InitHandlePreDataWrite ();
         }
      }
      break;
   case R_PRE_DATA_WRITE:
      {
         // Write sync data and DAM before real datas
         switch ( HandlePreDataWrite ())
         {
         case 0: break;
         case 1: // Next state
            InitHandleWriteData ();
            break;
         }

      }
      break;
   case R_WRITE_DATA:
      {
         // Write sync data and DAM before real datas
         switch ( HandleWriteData ())
         {
         case 0: break;
         case 1: // Next byte
            break;

         case 2: // Write CRC
            InitHandleWriteCrc ();
            break;
         case 3:
            // Error !
            // Overrun
            status_1_ |= 0x10;
            or_ = true;
            /*
            int i = 0;
            m_Status1 |= 0x10;
            results_[i++] = m_Status0|0x40;
            results_[i++] = m_Status1;
            results_[i++] = m_Status2;
            results_[i++] = TR;
            results_[i++] = HD;
            results_[i++] = LS;
            results_[i++] = Sz;
            interrupt_occured_ = true;
            m_DIO = true; m_EXM = false; m_RQM = true;
            state_ = RESULT;
            */
            break;
         }
      }
      break;
   case R_WRITE_CRC:
      {
         switch ( HandleWriteCrc () )
         {
         case 0: break;
         case 1: // end of CRC : Next track ?
            if ( (status_1_ & 0x20) == 0x20)
            {
               READ_END(false);
            }
            else
               if ( results_[5] == ls_)
               {

                  if (mt_ && ((hd_ & 1) == 0))
                  {
                     mt_ = false;
                     nd_ = true;
                     // Invert head
                     // Read sector 0 to LS-first
                     hd_++;
                     // Invert side

                     disk_[current_drive_].SetSide(disk_[current_drive_].GetSide() == 0 ? 1 : 0);

                     if (init_r_ != 1)force_nd_ = true;
                     sc_ = 1;
                     //LS = LS - m_InitR+SC;

                     index_hole_encountered_ = 0;
                     read_sector_state_ = 0;
                     rqm_ = false;
                     InitHandleSyncField();
                  }
                  else
                  {
                     // Yes : EOT encountered

                     // S0 S1 S2 TR HD LS Sz
                     int i = 0;

                     // TEST : At the end of a successful read/write command, the program should send a Terminal Count (TC) signal to the FDC.
                     // However, in the CPC the TC pin isn't connected to the I/O bus, making it impossible for the program to confirm a correct operation.
                     // For that reason, the FDC will assume that the command has failed, and it'll return both Bit 6 in Status Register 0 and Bit 7 in Status Register 1 set.
                     // The program should ignore this error message.
                     status_0_ |= 0x40; // Instruction achevée - TA : No TC on CPC
                     if( !or_)
                        status_1_ |= 0x80;
                     results_[i++] = status_0_;
                     results_[i++] = status_1_;
                     results_[i++] = status_2_;

                     results_[i++] = tr_;
                     results_[i++] = hd_;
                     results_[i++] = sc_; // parameters_[3]; //LS;
                     results_[i++] = sz_;

                     // TODO : Add a question mark about the writability of disk ?
                     interrupt_occured_ = true;
                     dio_ = true; exm_ = false; rqm_ = true;
                     state_ = RESULT;
                  }
               }
               else
               {
                  // Seek next sector
                  sc_++;
                  sc_array_[current_drive_&1] = sc_;
                  index_hole_encountered_ = 0;
                  rqm_ = false;
                  InitHandleSyncField ();
               }

               break;
         case 2: // Error....
            break;
         }
      }
      break;
   }
}

///////////////////////////////////////////
// cRecalib
void FDC::Recalib () // 07
{
   status_0_ = parameters_[0] & 0x07;
   hu_ = parameters_[0]&0x3; // Drive number
   current_drive_ = hu_&0x1;
   rqm_ = false;
   DECODE_SIDE

      recalibrate_ = true;
   //if (notifier_ != NULL) notifier_->TrackChanged(disk_[current_drive_].GetCurrentTrack());

   if (disk_[current_drive_].IsDiskReady(true))
   {
      //disk_[current_drive_].Recalib ();
   }

   tp_ = disk_[current_drive_].GetCurrentTrack();
   /*TP = 0; // Force it !*/
   track_seeded_ = 0;
   time_ = 200; // to check

   /*m_Status3 = (TP == 0)?0x10:0; // track 0 we are
   m_Status3 |= disk_[current_drive_].IsWriteProtected()?0x40:0;

   m_Status0  &= ~(0xC0);
   m_Status0  |= 0x20;
   if (!disk_[current_drive_].IsDiskReady(true))
   {
      m_bRecal = false;
      interrupt_code_ = 0x1;
      //m_Status0  |= 0x40 | NR;
      //interrupt_code_ = 0x1;
      //m_bSeekEnd = false;

   }
   else
   {
      m_bRecal = true;
      m_CB = false;
      m_Status3 |= 0x20; // ready
      interrupt_code_ = 0x00;
   }
   m_Time = 200; // to check
   m_CB = true;

   m_bSeekEnd = true;
   interrupt_occured_ = true;

   */
   //m_Busy = (1<<current_drive_);

   if (tp_ == track_seeded_)
   {
      disk_[current_drive_].SetCurrentTrack(track_seeded_);

      status_0_ |= 0x20; // Seek end
      interrupt_code_ = 0x00;
      interrupt_occured_ = true;
      seek_end_ = true;
      seek_track_ = false;

   }
   else
   {
      // Count to new track :
      // Step speed * nb tracks
      busy_ = ((0x1) << current_drive_); // Seek end
                                          //m_MainStatus |= ((0x1)<<current_drive_); // Seek end


                                          // TROP LONG : Pas ok pour blood !
                                          // "If the time to write 3 bytes of seek command exceed 150us, the timing between the first two step pulses may be shorter
                                          // than set  in the specify command by as much as 1 ms"
      move_size_ = (track_seeded_ > tp_) ? 1 : -1;
      step_count_ = abs(track_seeded_ - tp_);
      seek_count_ = step_rate_ * 500; // steprate in ms ; tick is in 2us;

      if (true) // CHEAT HERE ! TODO : Compute real value
      {
         seek_count_ -= 500;
         seek_count_ += 1;
      }

      if (notifier_ != NULL) notifier_->TrackChanged(track_seeded_ - tp_);

      if (!disk_[current_drive_].IsDiskReady())
      {
         cb_ = true;
         time_ = 200; // to check !
                       //m_Status0  |= 0x40 | NR;
                       //interrupt_code_ = 0x1;
                       //m_bSeekEnd = false;
         interrupt_code_ = 0x1;
         seek_end_ = true;
         interrupt_occured_ = true;
         seek_cmd_ = false;

      }
      else
      {
         cb_ = false;
         time_ = 200;
         status_3_ |= 0x20; // ready
         interrupt_occured_ = false;
         seek_end_ = false;
         seek_track_ = true;
         rw_command_ = false;
         seek_cmd_ = true;
         interrupt_code_ = 0x2; // Not interrupted !
      }
   }
   cb_ = true;
   current_command_ = C_NONE;
   state_ = RESULT;
}


///////////////////////////////////////////
// cSenseIntState
void FDC::SenseDriveState () // 4
{
   //DECODE_SIDE
   HandleResult ( R_S3 );
   /*
   // Unit select, Head adress
   m_Status3 = parameters_[0]&0x7;
   HU = parameters_[0]&0x1;
   current_drive_ = HU&0x3;
   m_Status3 |= disk_[current_drive_].IsWriteProtected()?0x40:0;

   // 2 side (0 = yes, 1 = no)

   // Track 0
   m_Status3 |= (TP == 0)?0x10:0;

   // Ready : only if a disk is present - NOT : don't try this, otherwise empty disk will not be recognized
   m_Status3 |= (disk_[current_drive_].IsDiskPresent())?0x20:0;
   //m_Status3 |= 0x20;   // Set to 0 if not drive B is present. - TODO :Handle existence or not of drive B.

   m_Status3 |= (!disk_[current_drive_].IsDiskPresent())?0x80:0;
   //m_Status3 = 0x10;

   // Write protected
   // Fault

   results_[0] = m_Status3;*/
}

///////////////////////////////////////////
// cWrDeletedSec
void FDC::WrDeletedSec ()
{
   WriteSector ();
   write_deleted_ = true;
}

///////////////////////////////////////////
// cRdDeletedSec
void FDC::RdDeletedSec()
{
   ReadSector ();
   read_deleted_ = true;
}

///////////////////////////////////////////
// cFormatTrack
void FDC::FormatTrack()
{

   current_command_ = C_FORMAT_TRACK;
   current_command_phase_ = R_LOAD_HEAD;
   nb_data_in_buffer_ = 0;
   status_0_ = status_2_ = 0;
   ResetStatus1();
   dio_ = false;
   rqm_ = false;
   exm_ = true;
   // HU Sz NM GP FB

   status_0_ = parameters_[0] & 0x07;
   hu_ = parameters_[0]&0x3; // Drive number
   current_drive_ = hu_&1;
   DECODE_SIDE
      sz_ = parameters_[1] ;  // Sz
   nm_ = parameters_[2] ;  // NM
   gp_ = parameters_[3] ;  // GP
   fb_ = parameters_[4] ;  // FB

   format_sector_count_ = 0;

   status_1_ |= disk_[current_drive_].IsWriteProtected()?0x02:0;

   disk_[current_drive_].SetWrite( tp_);

   if (!disk_[current_drive_].IsDiskReady())
   {
      // No disk
      status_0_ = 0x48;
      status_1_ = 0x00;
      int i =0;
      results_[i++] = status_0_; // m_Status0;
      results_[i++] = status_1_;
      results_[i++] = status_2_;
      results_[i++] = disk_[current_drive_].GetCurrentTrack () /*m_CurrentTrack*/; //TR;
      results_[i++] = hd_;
      results_[i++] = format_sector_count_;
      results_[i++] = sz_;
      interrupt_occured_ = true;
      dio_ = true; exm_ = false; rqm_ = true;
      state_ = RESULT;
      sz_ = 0;
   }
}

void FDC::FormatTrackTick ()
{
   if (!disk_[current_drive_].IsDiskReady())
   {
      // No disk
      status_0_ = 0x48;
      status_1_ = 0x00;
      int i = 0;
      results_[i++] = status_0_; // m_Status0;
      results_[i++] = status_1_;
      results_[i++] = status_2_;
      results_[i++] = disk_[current_drive_].GetCurrentTrack() /*m_CurrentTrack*/; //TR;
      results_[i++] = hd_;
      results_[i++] = format_sector_count_;
      results_[i++] = sz_;
      interrupt_occured_ = true;
      dio_ = true; exm_ = false; rqm_ = true;
      state_ = RESULT;
      sz_ = 0;
      return;
   }

   bool bOnIndex = disk_[current_drive_].IsHeadOnIndex ();
   switch (current_command_phase_)
   {
      // Load the head
   case R_LOAD_HEAD:
      if (HandleLoadHead () == 1)
      {
         // Arrived : Check now the sector
         index_hole_encountered_ = 0;
         InitHandleFindIndexHole ();
      }
      break;
   case R_FIND_INDEX_HOLE:
      if (bOnIndex)
      {
         // Begin formating
         InitHandleFormat (MFM_GAP4_A);
      }
      break;
   case R_FORMAT:
      switch (HandleFormat ())
      {
      case 0 :break;
      case 1: // Next
         if (next_phase_ == MFM_END)
         {
            int i = 0;
            status_0_ |= 0;//0x40; // Instruction achevée
            results_[i++] = status_0_;
            results_[i++] = status_1_;
            results_[i++] = status_2_;

            results_[i++] = tr_;
            results_[i++] = hd_;
            results_[i++] = format_sector_count_;
            results_[i++] = sz_;

            // Write disk !
            interrupt_occured_ = true;
            dio_ = true; exm_ = false; rqm_ = true;
            state_ = RESULT;
            break;
         }
         else
         {
            InitHandleFormat (next_phase_);
         }
         break;
      case 2: // Error ?
         // TODO
         break;
      }

      break;
   }
}

///////////////////////////////////////////
// cSeekTrackN
void FDC::SeekTrackN()   // 0F
{
   //unsigned char track;
   //m_CurrentCommand = cSeekTrack;
   recalibrate_ = false;
   status_0_ = parameters_[0] & 0x07;
   rqm_ = false;
   recalibrate_ = false;
   hu_ = parameters_[0]&0x3;
   current_drive_ = hu_&1;
   DECODE_SIDE
   busy_ |= (1 << current_drive_);

      if ( seek_track_ && track_seeded_ == parameters_[1])
      {
         // Already seeking this track !
         return;
      }

      track_seeded_ = parameters_[1]; // TP
      unsigned char head_number = ((parameters_[0] & 0x04) >> 2);
      if ( track_seeded_ >= disk_[current_drive_].GetNbTracks(head_number))
      {
         track_seeded_ = (disk_[current_drive_].GetNbTracks(head_number) != 0)?disk_[current_drive_].GetNbTracks(head_number)-1:0;
      }

      tp_ = disk_[current_drive_].GetCurrentTrack ();
      if ( tp_ == track_seeded_)
      {
         disk_[current_drive_].SetCurrentTrack( track_seeded_);

         status_0_ |= 0x20; // Seek end
         interrupt_code_ = 0x00;
         interrupt_occured_ = true;
         seek_end_ = true;
         seek_track_ = false;

      }
      else
      {
         // Count to new track :
         // Step speed * nb tracks
         busy_ = ((0x1)<<current_drive_); // Seek end
         //m_MainStatus |= ((0x1)<<current_drive_); // Seek end


         // TROP LONG : Pas ok pour blood !
         // "If the time to write 3 bytes of seek command exceed 150us, the timing between the first two step pulses may be shorter
         // than set  in the specify command by as much as 1 ms"
         move_size_ = (track_seeded_ > tp_) ? 1 : -1;
         step_count_ = abs(track_seeded_ - tp_);
         seek_count_ = /*m_StepCount * */ step_rate_ * 500 ; // steprate in ms ; tick is in 2us;

         if (true) // CHEAT HERE ! TODO : Compute real value
         {
            seek_count_ -= 500;
            seek_count_ += 1;
         }

         if (notifier_ != NULL) notifier_->TrackChanged(track_seeded_ - tp_);

         if (!disk_[current_drive_].IsDiskReady(/*true*/))
         {
            cb_ = true;
            time_ = 200; // to check !
            //m_Status0  |= 0x40 | NR;
            interrupt_code_ = 0x1;
            //m_bSeekEnd = false;
            interrupt_code_ = 0x1;
            seek_end_ = true;
            interrupt_occured_ = true;
            seek_cmd_ = false;
            return;
         }
         else
         {
            status_3_ |= 0x20; // ready
            interrupt_occured_ = false;
            seek_end_ = false;
            seek_track_ = true;
            rw_command_ = false;
            seek_cmd_ = true;
            interrupt_code_ = 0x2; // Not interrupted !
         }
      }
      cb_ = true;
      current_command_ = C_NONE;
}

void FDC::SeekTrackNTick()   // 0F
{
   if (--seek_count_ == 0)
   {
      tp_ += move_size_;
      disk_[current_drive_].SetCurrentTrack(tp_);
      if (track_seeded_ == tp_)
      {
         //m_CB = false;
         // End
         //disk_[current_drive_].SetCurrentTrack(m_TrackSeeked);
         tp_ = track_seeded_;
         interrupt_occured_ = true;
         seek_end_ = true;
         seek_track_ = false;
         rqm_ = true;
         interrupt_code_ = 0x00;
      }
      else
      {
         if (!disk_[current_drive_].IsDiskReady())
         {
            ready_line_changed_ = true;
            //interrupt_occured_ = true;
            //seek_track_ = false;
            rqm_ = true;
            interrupt_code_ = 0x02;
            tp_ = track_seeded_; // Force it !
         }
         else
         {
            seek_count_ = step_rate_ * 500;
         }
      }
   }
   else
   {
   }
}



bool CompareEqual (unsigned char Dfdd, unsigned char Dproc){ return Dfdd == Dproc;}
bool CompareInfEqual (unsigned char Dfdd, unsigned char Dproc){ return Dfdd <= Dproc;}
bool CompareSupEqual (unsigned char Dfdd, unsigned char Dproc){ return Dfdd >= Dproc;}

///////////////////////////////////////////
// cScanEqual
void FDC::ScanEqual()
{
   scan_func_ = CompareEqual;
   current_command_ = C_SCAN_EQUAL;

   status_0_ = parameters_[0] & 0x07;
   Scan ();
}

///////////////////////////////////////////
// cScanLowOrEqual
void FDC::ScanLowOrEqual()
{
   scan_func_ = CompareInfEqual;
   current_command_ = C_SCAN_LOW_OR_EQUAL;
   Scan ();
}
///////////////////////////////////////////
// cScanHighOrEqual
void FDC::ScanHighOrEqual()
{
   scan_func_ = CompareSupEqual;
   current_command_ = C_SCAN_HIGH_OR_EQUAL;
   Scan ();

}

void FDC::Scan()
{
   seek_end_ = false;
   read_deleted_  = false;
   
   nb_data_in_buffer_ = 0;
   status_0_ = status_2_ = 0;
   ResetStatus0();
   ResetStatus1();

   rqm_ = false;
   dio_ = false;
   nd_ = true;

   DECODE_SIDE
   status_0_ = parameters_[0] & 0x07;

   hu_ = parameters_[0]&0x3; // Drive number
   current_drive_ = hu_&0x1;
   tr_ = parameters_[1] ;  // TR
   hd_ = parameters_[2] ;  // HD
   init_r_ = sc_ = parameters_[3] ;  // Sz
   sc_array_[current_drive_&1] = sc_;

   sz_ = parameters_[4] ;  // Sz
   ls_ = parameters_[5] ;  // EOT
   gp_ = parameters_[6] ;  // NM
   stp_ = parameters_[7] ;  // STP !

   if (!disk_[current_drive_].IsDiskReady())
   {
      interrupt_code_ = 0x1;
      nd_ = false;
      HandleResult (R_S0|R_S1|R_S2|R_TR|R_HD|R_LS|R_SZ);
      sz_ = 0;
      state_ = RESULT;
      return;

   }
   tc_ = true;
   read_sector_state_ = 0;
   index_hole_encountered_ = 0;
   InitHandleLoadHead ();
}

void FDC::ScanTick()
{
   bool bOnIndex = disk_[current_drive_].IsHeadOnIndex ();
   if ( ( bOnIndex && (!on_index_))  )
   {
      if (read_sector_state_ == 0)
      {
         // Otherwise
         // Yes, increment counter.
         ++index_hole_encountered_;

         // Handle EOT in case we have already found the first sector (at least)
         // TODO

         // Encountered twice ?
         if (index_hole_encountered_ >= 2)
         {
            if (!nd_)
            {
               status_2_ &= ~0x2;
            }

            interrupt_code_ = 0x1;

            interrupt_occured_ = true;
            HandleResult (R_S0|R_S1|R_S2|R_TR|R_HD|R_LS|R_SZ);
            sz_ = 0;

            // Result phase
            on_index_ = bOnIndex;
            state_ = RESULT;
            return;
         }
      }
      else
      {
         tc_ = false;
      }
   }


   on_index_ = bOnIndex;
   unsigned char car_buffer;
   switch (current_command_phase_)
   {
   case R_FIND_INDEX_HOLE:
      if (bOnIndex)
      {
         /*if (read_track_)
         {
         ++index_hole_encountered_;
         }*/
         InitHandleSyncField ();
      }
      break;
      // Load the head
   case R_LOAD_HEAD:
      if (HandleLoadHead () == 1)
      {
         // Arrived : Check now the sector
         index_hole_encountered_ = 0;
         if (read_track_)
         {
            // Waiting for index hole
            InitHandleFindIndexHole ();
         }
         else
         {
            InitHandleSyncField ();
         }
      }
      break;

      //case rFindSector:
   case R_FIND_SYNC_FIELD:
      if ( HandleSyncField ())
      {
         InitHandleIDAM ();
      }
      break;
   case R_FIND_IDAM:
   case R_FIND_ADRESS_MARK:
      switch ( HandleIDAM (&car_buffer) )
      {
      case 0: break;
      case 1:

         if (read_sector_state_ == 0 && car_buffer == 0xFE)
         {
            InitHandleCHRN ();
         }
         else if (read_sector_state_ == 1 )
         {
            if (car_buffer == 0xF8 || car_buffer == 0xFB /*|| read_track_*/)
            {
               if (car_buffer == 0xF8 || car_buffer == 0xFB)
               {
                  status_2_ &= (~0x1);
                  //m_Status1 &= (~0x1);
                  ma_ = false;

               }
               // TODO : Rework this
               // It seems the following behaviour is used :

               // - If Read normal and F8, or Read deleted and Fb, same behaviour occurs :
               if (((!read_deleted_ && car_buffer == 0xF8)
                  || (read_deleted_ && car_buffer == 0xFB)
                  )
                  )
               {
                  status_2_ |= 0x40;      // Set control mark
                  //  - If Sk = 1 : skip.
                  if (sk_)
                  {
                     //
                     if ( results_[5] == ls_)
                     {
                        //results_[5] = parameters_[3]; //
                        READ_END(true);
                     }
                     else
                     {
                        // Seek next sector
                        sc_++;
                        sc_array_[current_drive_&1] = sc_;
                        index_hole_encountered_ = 0;
                        read_sector_state_ = 0;
                        InitHandleSyncField ();
                     }
                  }
                  else
                  {
                     //  - Otherwise, dont skip.
                     status_2_ |= 0x40;      // Set control mark
                     tc_ = true;
                     //int size = 0x80<<(( Sz == 0 )?SL:Sz);
                     int size = (0x80 << (std::min(sz_, (unsigned char)0x8 )));
                     data_to_return_ = (sz_ == 0) ? (((sl_ & 1) == 0) ? 0x50 : 0x28) : (0x80 << (std::min(sz_, (unsigned char)0x8)));
                     InitHandleReadData ( size );
                  }
               }
               else
               {
                  // Normal behaviour
                  //int size = 0x80<<(( Sz == 0 )?SL:Sz);
                  int size = (0x80 << (std::min(sz_, (unsigned char)0x8)));
                  data_to_return_ = (sz_ == 0) ? (((sl_ & 1) == 0) ? 0x50 : 0x28) : (0x80 << (std::min(sz_, (unsigned char)0x8)));
                  //int size = (Sz == 0) ? SL : (0x80 << Sz);
                  InitHandleReadData ( size );
               }
            }
            else
            {
               // No adress mark here...
               status_2_ |= 0x1;
               READ_END(false);
            }
         }
         else
         {
            read_sector_state_ = 0;
            InitHandleSyncField ();

         }
         break;
      case 2:
         read_sector_state_ = 0;
         InitHandleSyncField ();
         break;

      };
      break;
   case R_READ_CHRN:
      {
         switch (HandleCHRN ( &results_[3] ))
         {
         case 0:
         case 1: // Nothing
            break;
         case 2: // Check CRC
            // Check WC and BC flags of status 2
            // Set the next phase, if CHRN are ok :
            if (
               results_[4] == hd_
               && results_[5] == sc_
               && results_[6] == sz_
               )
            {
               if (results_[3] == tr_)
               {
                  //m_Status1 &= (~0x4); // Remove the ND flag
                  nd_ = false;
                  //m_Status1 |= 0x1;
                  ma_ = true;

                  rqm_ = true;
                  status_2_ |= 0x08;
                  InitHandleCRC ();
               }
               else
               {
                  if (results_[3] == 0xFF)
                  {
                     // BC set !
                     status_2_ |= 0x2;
                  }
                  // WC is set
                  status_2_ |= 0x10;

                  InitHandleSyncField ();
               }
            }
            else // Otherwise, back to search of sync bytes
            {
               InitHandleSyncField ();
            }

            break;
         case 3: // Error !? TODO ??
            break;
         }
      }
      break;
   case R_CHECK_CRC:
      {
         unsigned int retCrc = HandleCRC (read_sector_state_==1);
         if (retCrc != 0)
         {
            // Let's have a return !
            results_[0] = status_0_; // m_Status0;
            results_[1] = status_1_;
            results_[2] = status_2_;

            if (read_sector_state_ == 0)
            {
               read_sector_state_ = 1; // DATA phase
               InitHandleSyncField ();
            }
            else
            {
               // How to handle end of reading ?
               //if ( (m_Status1 & 0x20) == 0x20 && !read_track_)
               if (de_)
               {
                  //results_[5] = parameters_[3]; //

                  READ_END(false);
               }
               else
                  if ((status_2_ & 0x40) == 0x40 && !sk_)
                  {
                     ls_ = sc_;
                     READ_END(false);
                  }
                  else if ( results_[5] == ls_)
                  {
                     // MT ?
                     //if (m_MT && disk_[current_drive_].GetSide () == 0)
                     if (mt_ && ((hd_ &1)== 0))
                     {
                        mt_ = false;
                        nd_ = true;
                        // Invert head
                        // Read sector 0 to LS-first
                        hd_ ++;
                        // Invert side

                        disk_[current_drive_].SetSide (disk_[current_drive_].GetSide()==0?1:0);

                        if (init_r_ != 1 )force_nd_ = true;
                        sc_ = 1;
                        
                        if (!read_track_ )
                        {
                           index_hole_encountered_ = 0;
                        }
                        read_sector_state_ = 0;
                        InitHandleSyncField ();
                     }
                     else
                     {
                        if (or_)
                        {
                           READ_END(false);
                        }
                        else
                        {

                           READ_END(false);
                        }
                     }
                  }
                  else
                  {
                     // Seek next sector
                     sc_ += stp_;
                     nd_ = true;
                     sc_array_[current_drive_&1] = sc_;
                     if (!read_track_ )
                     {
                        index_hole_encountered_ = 0;
                     }
                     read_sector_state_ = 0;
                     InitHandleSyncField ();
                  }

            }
         }
      }
      break;


   case R_READ_DATA:
      // Test : Are-we on index hole ?
      /*if ( read_track_ == true && disk_[current_drive_].IsHeadOnIndex () )
      {
      // Stop sending information.
      READ_END(false)
      }
      else*/
      {
         unsigned char data_from_fdc;
         switch ( HandleReadData (&data_from_fdc) )
         {
         case 0:
            break;
         case 1:

            // One data is available ?
            if ( nb_data_in_buffer_ == 0 )
            {
               // Overrun
               or_ = true;
               // TOCHECK : What aer we supposed to do ?
            }
            else
            {
               if (data_from_fdc != data_buffer_[0])
               {
                  // Remove the Scan hit bit
                  status_2_ &= ~0x8;

                  // Set the SN if condition is not met
                  if ( scan_func_ (data_from_fdc, data_buffer_[0]) == false)
                  {
                     status_2_ |= 4;
                  }
               }
               // Scan not hit :
            }

            // Check the scan

            if (nb_data_in_buffer_>0)
               nb_data_in_buffer_--;

            state_ = EXEC_PRECISE;
            rqm_ = true;

            // TO REMOVE ???
            // m_EXM = true;

            break;
         case 2:
            // One data, then go to CRC check
            if (nb_data_in_buffer_>0)
               nb_data_in_buffer_--;
            state_ = EXEC_PRECISE;
            rqm_ = false;
            // Check for overrun - TODO
            // CRC Check
            InitHandleCRC ();
            break;
         }
         break;
   default:
      // ERROR !
      break;
      }
   }
}


///////////////////////////////////////////
// cReadTrack
void FDC::ReadTrack ()
{
   seek_end_ = false;
   current_command_ = C_READ_TRACK;
   nb_data_in_buffer_ = 0;
   status_0_ = status_2_ = 0;
   ResetStatus0();
   ResetStatus1();
   ma_ = true;
   nd_ = false;
   status_1_ = 0x5; // ND by default
   // Default : Not found
   rqm_ = false;
   dio_ = true;

   // TODO : sanity check
   DECODE_SIDE
      hu_ = parameters_[0]&0x3; // Drive number
   current_drive_ = hu_&0x1;
   status_0_ = parameters_[0] & 0x07;
   tr_ = parameters_[1] ;  // TR
   hd_ = parameters_[2] ;  // HD
   ls_ = sc_ = parameters_[3] ;  // Sz
   sector_count_ = 0;
   //SC = 0; // Sector count
   //sc_array_[current_drive_&1] = SC;
   sz_ = parameters_[4] ;  // Sz
   nm_ = parameters_[5] ;  // LS
   gp_ = parameters_[6] ;  // NM
   sl_ = parameters_[7] ;  // SL

   read_data_done_ = false;
   if (!disk_[current_drive_].IsDiskReady())
   {
      interrupt_code_ = 0x1;
      nd_ = false;
      ma_ = false;
      HandleResult(R_S0 | R_S1 | R_S2 | R_TR | R_HD | R_LS | R_SZ);
      sz_ = 0;
      state_ = RESULT;
      return;
   }

   first_sector_found_ = false;
   read_track_ = true;
   index_hole_encountered_ = 0;
   read_sector_state_ = 0;
   InitHandleLoadHead ();

   if ( disk_[current_drive_].NewBitAvailable ())
      ReadTrackTick ();

}

void FDC::ReadTrackTick ()
{
   // One more tick on the Read ID command
   bool bOnIndex = disk_[current_drive_].IsHeadOnIndex ();
   //if ( disk_[current_drive_].IsHeadOnIndex () && (read_sector_state_ == 0) )
   if (  bOnIndex && (!on_index_) && (!first_sector_found_))
   {
      // Yes, increment counter.
      ++index_hole_encountered_;

      // Encountered twice ?
      if ( index_hole_encountered_ >= 2 )
      {
         if (current_command_phase_ != R_READ_DATA)
         {
            if (!nd_)
            {
               status_2_ &= ~0x2;
            }

            interrupt_code_ = 0x1;


            interrupt_occured_ = true;
            HandleResult(R_S0 | R_S1 | R_S2 | R_TR | R_HD | R_LS | R_SZ);
            sz_ = 0;


            /*// Missing Adress Mark, end of command.
            m_Status0 &= 0x7F;
            m_Status0 |= 0x40;

            // Remove for Hercule II
            // CHECK THIS : End of track if data are sent
            //if (read_data_done_)
            //   m_Status1 |= 0x80;

            // Do we have an error that ends the command ?
            // Yes : No 'TC' is needed :
            if ( (m_Status2 & 0x63) != 0)
            {
            m_Status1 &= ~0x80;
            }
            else
            {
            m_Status1 |= 0x80;
            }

            // No : TC is needed

            results_[0] = m_Status0; // m_Status0;
            results_[1] = m_Status1;
            results_[2] = m_Status2;
            */
            // Result phase
            interrupt_occured_ = true;
            on_index_ = bOnIndex;
            state_ = RESULT;
            return;
         }
      }
   }
   on_index_ = bOnIndex;
   unsigned char car_buffer;
   switch (current_command_phase_)
   {
   case R_FIND_INDEX_HOLE:
      if (bOnIndex )
      {
         InitHandleSyncField ();
      }
      break;
      // Load the head
   case R_LOAD_HEAD:
      if (HandleLoadHead () == 1)
      {
         // Arrived : Check now the sector
         index_hole_encountered_ = 0;
         // Waiting for index hole
         InitHandleFindIndexHole ();
      }
      break;

      //case rFindSector:
   case R_FIND_SYNC_FIELD:
      if ( HandleSyncField ())
      {
         InitHandleIDAM ();
      }
      break;
   case R_FIND_IDAM:
   case R_FIND_ADRESS_MARK:
      switch ( HandleIDAM (&car_buffer) )
      {
      case 0: break;
      case 1:
         status_1_ &= (~0x1);
         if (ma_)
         {
            ma_ = false;
            //m_ND = true;
         }
         if (read_sector_state_ == 0 && car_buffer == 0xFE)
         {
            InitHandleCHRN ();
         }
         else if (read_sector_state_ == 1 )
         {
            if (car_buffer == 0xF8 || car_buffer == 0xFB )
            {
               first_sector_found_ = true;

               status_2_ &= (~0x1);
               if (car_buffer == 0xF8)
               {
                  status_2_ |= 0x40;
                  if (sk_)
                  {
                     /*                        //
                     if (results_[5] == LS)
                     {
                     //results_[5] = parameters_[3]; //
                     READ_END(true);
                     }
                     else*/

                     {

                        sector_count_++;

                        if (nm_ == (sector_count_ & 0xFF))
                        {
                           // TC error !
                           if (read_data_done_)
                              status_1_ |= 0x80;

                           results_[5] = nm_; //
                           READ_END_TRACK(true);

                        }
                        else
                        {
                           // Seek next sector
                           /*SC++;
                           sc_array_[current_drive_ & 1] = SC;*/
                           index_hole_encountered_ = 0;
                           read_sector_state_ = 0;
                           InitHandleSyncField();
                        }
                     }
                  }
                  else
                  {
                     //  - Otherwise, dont skip.
                     //int size = 0x80<<(( Sz == 0 )?SL:Sz);
                     int size = (0x80 << (std::min(sz_, (unsigned char)0x8)));
                     data_to_return_ = (sz_ == 0) ? (((sl_ & 1) == 0) ? 0x50 : 0x28) : (0x80 << (std::min(sz_, (unsigned char)0x8)));
                     first_sector_found_ = true;
                     //m_ND = (results_[5] != SC);
                     InitHandleReadData(size);
                  }
               }
               else
               {
                  // Normal behaviour
                  //int size = 0x80<<(( Sz == 0 )?SL:(Sz&0x7));

                  int size = (0x80 << (std::min(sz_, (unsigned char)0x8)));
                  data_to_return_ = (sz_ == 0) ? (((sl_ & 1) == 0) ? 0x50 : 0x28) : (0x80 << (std::min(sz_, (unsigned char)0x8)));
                  // CHANGE : USE CHRN INSTEAD !
                  //int size = 0x80<<(( Sz == 0 )?SL:Sz); a a
                  first_sector_found_ = true;
                  nd_ = (results_[5] != sc_);
                  InitHandleReadData(size);
               }
            }
            else
            {
               // No adress mark here...
               status_1_ |= 0x1;
               status_2_ |= 0x1;
               nd_ = true;
               ma_ = true;
               READ_END_TRACK(false);
            }
         }
         else
         {
            read_sector_state_ = 0;
            InitHandleSyncField ();

         }
         break;
      case 2:
         read_sector_state_ = 0;
         InitHandleSyncField ();
         break;

      };
      break;
   case R_READ_CHRN:
      {
         switch (HandleCHRN ( &results_[3] ))
         {
         case 0:
         case 1: // Nothing
            break;
         case 2: // Check CRC
            // Set the next phase, if CHRN are ok :
            //Sz = results_[6]&0x7;
            /*if (results_[5] == SC)
            {
            //m_Status1 &= (~0x4); // Remove the ND flag
            m_ND = false;
            //m_Status1 |= 0x1;
            }
            else
            {

            }*/

            /*SC = results_[5];
            sc_array_[current_drive_&1] = SC;*/


            // Note : Dont use the Sz value as size ! Exit will not work anymore !
            InitHandleCRC ();

            break;
         case 3: // Error !? TODO ??
            break;
         }
      }
      break;
   case R_CHECK_CRC:
      {
         unsigned int retCrc = HandleCRC (read_sector_state_==1);
         if (retCrc != 0)
         {
            // Let's have a return !
            results_[0] = status_0_; // m_Status0;
            results_[1] = status_1_;
            results_[2] = status_2_;

            if (read_sector_state_ == 0)
            {
               if (retCrc==2)
               {
                  if (results_[5] == sc_)
                  {
                     nd_ = true;
                  }
               }
               read_sector_state_ = 1; // DATA phase
               InitHandleSyncField ();
            }
            else
            {
               if (retCrc == 2)
               {
                  if (results_[5] == sc_)
                  {
                     nd_ = true;
                  }
               }
               sector_count_++;

               if (or_)
               {
                  READ_END_TRACK(nm_ == (sector_count_ & 0xFF));
               }
               else
               if ( nm_ == (sector_count_&0xFF))
               {
                  if (mt_ && ((hd_ & 1) == 0))
                  {
                     mt_ = false;
                     nd_ = true;
                     // Invert head
                     // Read sector 0 to LS-first
                     hd_++;
                     // Invert side

                     disk_[current_drive_].SetSide(disk_[current_drive_].GetSide() == 0 ? 1 : 0);

                     if (init_r_ != 1)force_nd_ = true;
                     sc_ = 1;
                     sector_count_ = 0;
                     
                     index_hole_encountered_ = 0;
                     read_sector_state_ = 0;
                     InitHandleSyncField();
                  }
                  else
                  {
                     // TC error !
                     if (read_data_done_)
                        status_1_ |= 0x80;

                     results_[5] = nm_; //
                     READ_END_TRACK(true);
                  }
               }
               else
               {
                  //SC++;
                  //sc_array_[current_drive_&1] = SC;

                  // Seek next sector
                  if (!read_track_ )
                  {
                     index_hole_encountered_ = 0;
                  }
                  read_sector_state_ = 0;
                  InitHandleSyncField ();
               }

            }
         }
      }
      break;


   case R_READ_DATA:
      // Test : Are-we on index hole ?
      /*if ( read_track_ == true && disk_[current_drive_].IsHeadOnIndex () )
      {
      // Stop sending information.
      READ_END(false)
      }
      else*/
      {
         switch ( HandleReadData (data_buffer_) )
         {
         case 0:
            break;
         case 1:

            // One data is available
            if ( nb_data_in_buffer_ > 0 )
            {
               // Overrun
               status_1_ |= 0x10;
               or_ = true;
               if ( !read_track_ )
               {
                  interrupt_occured_ = true;
                  READ_END_TRACK(false);
               }
#ifdef LOG_EXEC
               LOG("(");
               LOGB(data_buffer_[0]);
               LOG(")");
#endif
            }
            if ( tr_ == 03)
            {
               int bdg = 1;
            }
            if ( data_to_return_ > 0)
            {
               nb_data_in_buffer_ = 1;

               state_ = EXEC_PRECISE;
               rqm_ = true;
               data_to_return_--;
            }

            break;
         case 2:
            // One data, then go to CRC check
            /*if (Sz == 0)
            {
            // TC error !
            if (read_data_done_)
            m_Status1 |= 0x80;

            results_[5] = NM; //
            READ_END_TRACK(true);
            }
            else*/
            {
            if (data_to_return_ > 0)
            {
               nb_data_in_buffer_ = 1;
               state_ = EXEC_PRECISE;
               rqm_ = true;
            }
               // Check for overrun - TODO
               // CRC Check
               InitHandleCRC();
            }
            break;
         }
         break;
   default:
      // ERROR !
      break;
      }
   }
}


void FDC::InitHandleCRC ()
{
#ifdef LOG_MAX
   LOG( "Begin of CRC reading...");LOGEOL;
#endif
   out_index_ = 0;
   bit_count_ = 0;
   current_command_phase_ = R_CHECK_CRC;
}

// Ret :
// 0 : Not finished
// 1 : Ok
// 2 : CRC KO
int FDC::HandleCRC ( bool bData )
{
   int ret = 0;
   // Get this bit
   if (disk_[current_drive_].NewBitAvailable () == false ) return 0;
   unsigned char c = disk_[current_drive_].GetCurrentBit ();

   // Is it a data byte ?
   if (disk_[current_drive_].IsDataBit ())
   {
      read_crc_ <<= 1;
      read_crc_ |= (c&0x01);
      /*current_data_byte_ <<= 1;
      current_data_byte_ |= (c&0x01);
      */
      ++bit_count_;

      if ( bit_count_ == 16 )
      {
#ifdef LOG_MAX
         LOG( "CRC : ");LOGB(read_crc_>>8); LOGB(read_crc_&0xFF);LOGEOL;
#endif

         if (read_crc_ == crc_.GetCRC () )
         {
            ret = 1;
         }
         else
         {
            de_ = true;
            //m_Status1 |= 0x20;
            if (bData)
               status_2_ |= 0x20;
            ret = 2;
         }
      }
   }

   return ret;

}

void FDC::InitHandleCHRN ()
{
#ifdef LOG_MAX
   LOG( "Begin CHRN Reading...");LOGEOL;
#endif
   current_command_phase_ = R_READ_CHRN;
   out_index_ = 0;
   bit_count_ = 0;
}

// 0 : Nothing
// 1 : Advance one byte
// 2 : 4 CHRN byte are read
// 3 : Error ?
int FDC::HandleCHRN ( unsigned char* out_buffer )
{
   int ret = 0;
   // Get this bit
   if (disk_[current_drive_].NewBitAvailable () == false ) return 0;
   unsigned char c = disk_[current_drive_].GetCurrentBit ();

   // Is it a data byte ?
   if (disk_[current_drive_].IsDataBit ())
   {
      current_data_byte_ <<= 1;
      current_data_byte_ |= (c&0x01);

      ++bit_count_;

      if ( bit_count_ == 8 )
      {
#ifdef LOG_MAX
         LOG( "One more byte : ");LOGB(current_data_byte_);LOGEOL;
#endif
         crc_.AddByteToCrc (current_data_byte_);
         out_buffer[out_index_ ++] = current_data_byte_;
         bit_count_ = 0;

         ret = (out_index_ == 4)?2:1;
      }
   }

   return ret;

}


void FDC::InitHandleIDAM ()
{
#ifdef LOG_MAX
   LOG( "Begin of IDAM detection\r\n");
#endif
   current_command_phase_ = R_FIND_IDAM;
   crc_.Reset ();
   sync_count_ = 0;
   bit_count_ = 0;
   first_non_0_ = false;
}

// 0 : Not finisehd yet
// 1 : Byte read
// 2 : No sync
int FDC::HandleIDAM (unsigned char *next_byte)
{
   int ret = 0;
   // Get this bit
   if (disk_[current_drive_].NewBitAvailable () == false ) return 0;
   unsigned char c = disk_[current_drive_].GetCurrentBit ();

   // Is it a data byte ?
   if (disk_[current_drive_].IsDataBit ())
   {
      if ( !first_non_0_  && c != 0)
      {
#ifdef LOG_MAX
         LOG( "First non bit detected\r\n");
#endif
         // First non 0 bit : Start to collect them
         bit_count_ = 0;
         first_non_0_ = true;
      }

      if (first_non_0_)
      {
         current_data_byte_ <<= 1;
         current_data_byte_ |= (c&0x01);

         ++bit_count_;

         // If waiting for Adress mark :
         if (current_command_phase_ == R_FIND_ADRESS_MARK)
         {
            // Adress mark ok : Next state
            if ( bit_count_ == 8 )
            {
               crc_.AddByteToCrc (current_data_byte_);

               *next_byte = current_data_byte_ ;
#ifdef LOG_MAX
               LOG( "Adress Mark = ");
               LOGB(current_data_byte_); LOGEOL;
#endif
               bit_count_ = 0;
               ret = 1;
               if (read_sector_state_ == 0 )
               {
                  //m_Status1 &= (~0x1); // Remove the ND flag
               }
            }
         }
         else
         {
            // Otherwise check for sync
            if ( disk_[current_drive_].IsSync ())
            {
#ifdef LOG_MAX
               LOG( "MFM Sync Byte = ");
               LOGB(current_data_byte_); LOGEOL;
#endif
               crc_.AddByteToCrc (current_data_byte_);
               // Sync : increase count
               ++sync_count_;
               bit_count_ = 0;
               // Enough ? Wait for specific byte
               if (sync_count_ == 3)
               {
                  // Yes, look for the adress mark
                  current_command_phase_ = R_FIND_ADRESS_MARK;

               }
            }
            else
            {
               if ( bit_count_ == 8 )
               {
                  // 8 more bits without any sync... Reset
#ifdef LOG_MAX
                  LOG( "Not a sync byte...");
#endif
                  bit_count_ = 0;
                  sync_count_ = 0;
                  ret = 2;
               }
            }
         }
      }
   }
   return ret;
}

void FDC::InitHandleSyncField ()
{
#ifdef LOG_MAX
   LOG( "Begin Sync Field\r\n");
#endif
   current_command_phase_ = R_FIND_SYNC_FIELD;
   sync_count_ = 0;
}

bool FDC::HandleSyncField ()
{
   bool next_step = false;
   // Wait for 12 0 ( 10 - try to sync on first found)
   if (!disk_[current_drive_].NewBitAvailable () ) return 0;
   const unsigned char c = disk_[current_drive_].GetCurrentBit ();

   if (disk_[current_drive_].IsDataBit ())
   {
      if ( c == 0 )
      {
         if ( previous_bit_ == 1 )
         {
            // Sync is ok. Count.
            ++sync_count_;
            // CHECK THIS !!!!!
            if (sync_count_ == SYNC_BIT_NUMBER/*SYNC_COUNT*/) // 72 min
            {
               // Next step
#ifdef LOG_MAX
               LOG( "Sync over (72 sync bits detected).\r\n");
#endif
               next_step = true;
            }
         }
         else
         {
            // Reset of sync count
            sync_count_ = 0;
         }
      }
      else
      {
         // Shift data : Current of 0 will never be ok like that
         disk_[current_drive_].ShiftDataBit ();

         // Reset of sync count
         sync_count_ = 0;
      }
   }
   previous_bit_ = c;

   return next_step;
}

void FDC::InitHandleReadData (int size)
{
#ifdef LOG_MAX
   LOG( "Begin Read Data\r\n");
#endif
   current_command_phase_ = R_READ_DATA;
   data_to_read_ = size;
   bit_count_ = 0;
   read_data_done_ = true;
}

// 0 : Nothing to process
// 1 : Data avilable
// 2 : Last data
int FDC::HandleReadData (unsigned char* out_buffer)
{
   int ret = 0;

   // Get this bit
   if (disk_[current_drive_].NewBitAvailable () == false ) return 0;

   unsigned char c = disk_[current_drive_].GetCurrentBit ();

   // Is it a data byte ?
   if (disk_[current_drive_].IsDataBit ())
   {
      current_data_byte_ <<= 1;
      current_data_byte_ |= (c&0x01);

      ++bit_count_;

      if ( bit_count_ == 8 )
      {
         -- data_to_read_;
         crc_.AddByteToCrc (current_data_byte_);
         *out_buffer = current_data_byte_;
         bit_count_ = 0;

         ret = (data_to_read_ == 0)?2:1;
      }
   }
   return ret;
}

void FDC::InitHandleLoadHead ()
{
#ifdef LOG_MAX
   LOG( "Head Loading\r\n");
#endif
   current_command_phase_ = R_LOAD_HEAD;
   index_hole_encountered_ = 0;
}

int FDC::HandleLoadHead ()
{
#ifdef LOG_MAX
   LOG( "Head Loading end\r\n");
#endif
   // Nothing here ! - Always ok
   return 1;
}

void FDC::InitHandleFindIndexHole ()
{
#ifdef LOG_MAX
   LOG( "Begin Find Index Hole\r\n");
#endif
   current_command_phase_ = R_FIND_INDEX_HOLE;
}

/*int FDC::HandleFindIndexHole ( bool bOnIndex )
{
#ifdef _LOG_MAX
LOG( "Find Index Hole\r\n");
#endif
// Nothing here ! - Always ok
return bOnIndex ?1:0;
}
*/

void FDC::InitHandlePreDataWrite ()
{
#ifdef LOG_MAX
   LOG( "Begin Predata Write\r\n");
#endif
   current_command_phase_ = R_PRE_DATA_WRITE;
   write_byte_counter_ = 0;
   disk_[current_drive_].Write (0x4E);
}

int FDC::HandlePreDataWrite ()
{
   int ret = 0;

   if ( disk_[current_drive_].IsWriteOver ())
   {
      ++write_byte_counter_;
      // If byte is under 22, no write : We're on the GAP
      if ( write_byte_counter_ < 22)
      {
         disk_[current_drive_].Write (0x4E);
      }
      // 22 -> 34 : 00 Sync bytes
      else if ( write_byte_counter_ < 34)
      {
         disk_[current_drive_].Write (0x00);
      }
      // 34 -> 37 : A1 sync byte
      else if ( write_byte_counter_ < 37)
      {
         if ( write_byte_counter_ == 34)
            crc_.Reset ();
         disk_[current_drive_].Write (0xA1, true);
         crc_.AddByteToCrc ( 0xA1 );
      }
      // 38 : FB/F8
      else if ( write_byte_counter_ < 38)
      {
         disk_[current_drive_].Write (write_deleted_?0xF8:0xFB);
         crc_.AddByteToCrc ( write_deleted_?0xF8:0xFB );
      }
      else
      {
         ret = 1;
      }
   }

   return ret;
}

void FDC::InitHandleWriteData ()
{
#ifdef LOG_MAX
   LOG( "Begin Data Write\r\n");
#endif
   current_command_phase_ = R_WRITE_DATA;
   write_byte_counter_ = 0;

   disk_[current_drive_].Write( data_buffer_[0] );
   crc_.AddByteToCrc ( data_buffer_[0] );
   if (nb_data_in_buffer_>0)
      nb_data_in_buffer_--;
   state_ = EXEC_PRECISE;

   // Data ready to be read
   rqm_ = true;
}

// 0 : Not finished
// 1 : Waiting for next byte
// 2 : Writing finished
// 3 : Overrun
int FDC::HandleWriteData ()
{
   int ret = 0;
   // Write next byte of data
   if ( disk_[current_drive_].IsWriteOver ())
   {
      ++write_byte_counter_;
      // End ?
      if ( write_byte_counter_ == (0x80<<sz_))
      {
         return 2;
      }
      else
      {
         // Next byte
         // Overrun ?
         /*if ( m_NbDataInBuffer <= 0)
         {
            if (Sz != 0 || (write_byte_counter_ < SL))
            {
               disk_[current_drive_].Write(m_DataBuffer[0]);
            }
            crc_.AddByteToCrc(m_DataBuffer[0]);
            m_RQM = true;
            return 3;
         }
         else*/
         {
            int ret = 0;
            if ( sz_ != 0 || (write_byte_counter_ < sl_))
            {
               disk_[current_drive_].Write( data_buffer_[0] );
            }
            crc_.AddByteToCrc ( data_buffer_[0] );
            // Get sector corresponding to CHRN
            if (nb_data_in_buffer_ <= 0)
            {
               ret = 3;
            }
            else
            {
               ret = 1;
               nb_data_in_buffer_--;
            }
            state_ = EXEC_PRECISE;

            // Data ready to be read
            if ( write_byte_counter_+1 == (0x80<<sz_))
            {
               int dbg = 1;
            }
            else
            {
               rqm_ = true;
            }
            return ret;
         }
      }
   }

   return ret;
}

void FDC::InitHandleWriteCrc ()
{
   current_command_phase_ = R_WRITE_CRC;
   LOG ("CRC = ");
   LOGB (crc_.GetCRC ()>>8);
   LOGB (crc_.GetCRC ()&0xFF);
   LOGEOL
      disk_[current_drive_].Write( (crc_.GetCRC ()>>8));
   write_byte_counter_ = 0;
}


int FDC::HandleWriteCrc ()
{
   int ret = 0;
   // Write next bit of computed CRC
   if ( disk_[current_drive_].IsWriteOver ())
   {
      ++write_byte_counter_;
      if ( write_byte_counter_ == 2)
      {
         // End
         ret = 1;
      }
      else
      {
         disk_[current_drive_].Write( (crc_.GetCRC ()&0xFF));
      }
   }

   return ret;
}
void FDC::InitHandleFormat ( MFMPhase formatPhase )
{
   format_phase_ = formatPhase;
   current_command_phase_ = R_FORMAT;

   write_byte_counter_ = 0;

   switch (format_phase_)
   {
   case MFM_GAP4_A:
      byte_is_sync_ = false;
      write_byte_counter_ = 80;
      byte_to_write_ = 0x4E;
      next_phase_ = MFM_SYNC1;
      break;
   case MFM_SYNC1:
      byte_is_sync_ = false;
      write_byte_counter_ = 12;
      byte_to_write_ = 0x00;
      next_phase_ = MFM_IAM;
      break;
   case MFM_IAM:
      byte_is_sync_ = true;
      write_byte_counter_ = 3;
      byte_to_write_ = 0xC2;
      next_phase_ = MFM_IAM2;
      break;
   case MFM_IAM2:
      byte_is_sync_ = false;
      write_byte_counter_ = 1;
      byte_to_write_ = 0xFC;
      next_phase_ = MFM_GAP1;
      break;
   case MFM_GAP1:
      byte_is_sync_ = false;
      write_byte_counter_ = 50;
      byte_to_write_ = 0x4E;
      next_phase_ = MFM_SYNC2;
      break;
   case MFM_SYNC2:
      byte_is_sync_ = false;
      write_byte_counter_ = 12;
      byte_to_write_ = 0x00;
      next_phase_ = MFM_IDAM;
      rqm_ = true;
      break;
   case MFM_IDAM:
      crc_on_ = true;
      crc_.Reset ();
      byte_is_sync_ = true;
      write_byte_counter_ = 3;
      byte_to_write_ = 0xA1;
      next_phase_ = MFM_IDAM2;
      break;
   case MFM_IDAM2:
      byte_is_sync_ = false;
      write_byte_counter_ = 1;
      byte_to_write_ = 0xFE;
      next_phase_ = MFM_CHRN;
      chrn_byte_count_ = 0;

      break;
   case MFM_CHRN:
      if (!CheckOverrun (false))
      {
         chrn_byte_count_ ++;
         byte_is_sync_ = false;
         // Get byte from data in
         // TODO : Overflow check
         byte_to_write_ = data_buffer_[0] ;
         nb_data_in_buffer_--;
         write_byte_counter_ = 1;

         // This should be checked...
         if (chrn_byte_count_ == 2)
         {
            hd_ = (byte_to_write_&1)<<2;
            rqm_ = true;
         }
         else
         if (chrn_byte_count_ == 3)
         {
            tr_ = byte_to_write_+1;
            rqm_ = true;
         }
         else  if (chrn_byte_count_ == 4)
         {
            //Sz = byte_to_write_;
            rqm_ = false;

         }
         else
         {
            rqm_ = true;
         }

         next_phase_ = (chrn_byte_count_  <4)?MFM_CHRN:MFM_CRC;
         crc_cyte_ = 0;
      }
      break;
   case MFM_CRC:
      crc_on_ = false;
      crc_cyte_++;
      byte_is_sync_ = false;
      write_byte_counter_ = 1;
      if ( crc_cyte_ == 1)
      {
         byte_to_write_ = (crc_.GetCRC()>>8);
         next_phase_ = MFM_CRC;
      }
      else
      {
         byte_to_write_ = (crc_.GetCRC()&0xFF);
         next_phase_ = MFM_GAP2;
      }
      break;
   case MFM_GAP2:
      byte_is_sync_ = false;
      write_byte_counter_ = 22;
      byte_to_write_ = 0x4E;
      next_phase_ = MFM_SYNC3;
      break;
   case MFM_SYNC3:
      byte_is_sync_ = false;
      write_byte_counter_ = 12;
      byte_to_write_ = 0x00;
      next_phase_ = MFM_DAM;
      break;
   case MFM_DAM:
      crc_on_ = true;
      crc_.Reset ();
      byte_is_sync_ = true;
      write_byte_counter_ = 3;
      byte_to_write_ = 0xA1;
      next_phase_ = MFM_DAM2;
      break;
   case MFM_DAM2:
      //if (!CheckOverrun (false))
      {
         byte_is_sync_ = false;
         write_byte_counter_ = 1;
         byte_to_write_ = 0xFB;
         next_phase_ = MFM_DATA;
      }
      break;
   case MFM_DATA:
      byte_is_sync_ = false;
      write_byte_counter_ = (unsigned int)(0x80<<sz_ ); // +1 for FB
      byte_to_write_ = fb_;
      next_phase_ = MFM_CRC2;
      crc_cyte_ = 0;
      break;
   case MFM_CRC2:
      crc_on_ = false;
      crc_cyte_++;
      byte_is_sync_ = false;
      write_byte_counter_ = 1;
      if ( crc_cyte_ == 1)
      {
         byte_to_write_ = (crc_.GetCRC()>>8);
         next_phase_ = MFM_CRC2;
      }
      else
      {
         byte_to_write_ = (crc_.GetCRC()&0xFF);
         next_phase_ = MFM_GAP3;
      }
      break;

   case MFM_GAP3:
      byte_is_sync_ = false;
      write_byte_counter_ = gp_;
      byte_to_write_ = 0x4E;
      if ( ++format_sector_count_ >= nm_)
      {
         // Track size
         //int sizeOfTrack = disk_[current_drive_].GetSizeOfTrack(HU, TP);

         if ( (disk_[current_drive_].GetPos()) + gp_ >= 6249)
            next_phase_ = MFM_END;
         else
            next_phase_ = MFM_GAB4_B;
      }
      else
      {
         next_phase_ = MFM_SYNC2;
      }
      break;
   case MFM_GAB4_B:
      byte_is_sync_ = false;
      if ( (disk_[current_drive_].GetPos()) >= DEFAULT_TRACK_SIZE)
         write_byte_counter_ = DEFAULT_TRACK_SIZE - disk_[current_drive_].GetPos();
      else
         write_byte_counter_ = 1;
      byte_to_write_ = 0x4E;
      next_phase_ = MFM_END;
      break;
   default:
      break;
   }

   //LOGB(byte_to_write_);
   if (state_ != RESULT)
   {
      if (crc_on_)
      {
         crc_.AddByteToCrc(byte_to_write_);
      }

      disk_[current_drive_].Write(byte_to_write_, byte_is_sync_);
   }
}

// Write x byte on the disk.
// 0 if not finished
// 1 if finished
// 2 if error
int FDC::HandleFormat ()
{
   int ret = 0;
   if ( disk_[current_drive_].IsWriteOver ())
   {
      --write_byte_counter_;
      if ( write_byte_counter_ == 0)
      {
         ret = 1;
      }
      else
      {
         if ( crc_on_ )
         {
            crc_.AddByteToCrc(byte_to_write_);
         }
         //LOGB(byte_to_write_);
         disk_[current_drive_].Write (byte_to_write_, byte_is_sync_);
      }
   }
   return ret;
}

IDisk::AutorunType FDC::GetAutorun(unsigned int driveNumber, char * buffer, unsigned int size_of_buffer)
{
   return disk_[driveNumber].GetAutorun (buffer, size_of_buffer);
}

void FDC::ResetStatus0()
{
}

unsigned char FDC::GetStatus0 ()
{
   unsigned char st0 = 0;
   // US0
   //if (current_drive_)st0|= 0x01;
   // US1 - Not connected
   st0 |= hu_;

   // HD
   if (disk_[current_drive_].GetSide() == 1)st0|= 0x04;
   // NR
   if (( !disk_[current_drive_].IsDiskReady() && rw_command_)
      ||(!disk_[current_drive_].IsDiskReady(true) && recalibrate_))
      st0|= 0x08;

   // Equipement check :
   // If recal, use TR0
   if (recalibrate_ && disk_[current_drive_].IsDiskReady(true) && seek_end_)
   {
      if ( disk_[current_drive_].GetCurrentTrack() != 0  ) st0 |= 0x10;
   }
   else
   {
      // FLT : todo
      //if ( !disk_[current_drive_].IsDiskReady() ) st0|= 0x10;
   }
   // Seek end
   if ( seek_end_) st0 |= 0x20;

   // Interrupt Code
   st0 |= (interrupt_code_<<6);
   interrupt_occured_ = false;

   return st0;
}

void FDC::ResetStatus1()
{
   status_1_ = 0;
   ma_ = false;
   nw_ = false;
   nd_ = false;
   force_nd_ = false;
   or_ = false;
   de_ = false;
   en_ = false;
}


unsigned char FDC::GetStatus1()
{
   unsigned char st1 = 0;

   // MA : Missing adress Mark
   if (ma_) st1|= 0x01;
   // NW : Not Writeable
   if (nw_) st1|= 0x02;
   // ND : No Data
   if (nd_ || force_nd_) st1|= 0x04;
   // Not used (0)
   // OR : Over run
   if (or_) st1|= 0x10;
   // DE : Data Error
   if (de_) st1|= 0x20;
   // Not used (0)
   // EN : End of Track
   if (en_) st1|= 0x80;

   return st1;
}

void FDC::ResetStatus2()
{
   status_2_ = 0;
}


unsigned char FDC::GetStatus2()
{
   unsigned char st2 = status_2_;

   return st2;
}
unsigned char FDC::GetStatus3 ()
{
   unsigned char st3 = 0;
   // US0
   if (current_drive_)st3|= 0x01;
   // US1 - Not connected
   // HD
   if (disk_[current_drive_].GetCurrentSide())st3|= 0x04;
   // TS
   if (disk_[current_drive_].DoubleSided () == false) st3|= 0x8;
   // T0
   if ( disk_[current_drive_].GetCurrentTrack() == 0) st3|= 0x10;
   // RY
   if ( disk_[current_drive_].IsDiskReady() ) st3|= 0x20;
   // WP
   if (disk_[current_drive_].IsWriteProtected()) st3|= 0x40;
   // FT

   // todo : check condition for FT signal !
   if (disk_[current_drive_].IsDiskPresent() == false) st3 |= 0x80;

   return st3;
}


void FDC::HandleResult ( int resultNeeded )
{
   int index_result = 0;

   if ( resultNeeded&R_S0)
   {
      results_[index_result++] = GetStatus0 ();
   }

   if ( resultNeeded&R_S1)
   {
      results_[index_result++] = GetStatus1();
   }

   if ( resultNeeded&R_S2)
   {
      results_[index_result++] = GetStatus2();
   }

   if ( resultNeeded&R_S3)
   {
      results_[index_result++] = GetStatus3 ();
   }

   if ( resultNeeded&R_TR)
   {
      results_[index_result++] = tr_;
   }

   if ( resultNeeded&R_HD)
   {
      results_[index_result++] = hd_;//disk_[current_drive_].GetSide();
   }

   if ( resultNeeded&R_NM)
   {
      results_[index_result++] = nm_;
   }

   if ( resultNeeded&R_LS)
   {
      results_[index_result++] = sc_;//LS;
   }

   if ( resultNeeded&R_SZ)
   {
      results_[index_result++] = sz_;
   }

   if ( resultNeeded&R_TP)
   {
      results_[index_result++] = tp_; //disk_[current_drive_].GetCurrentTrack();
   }

}

