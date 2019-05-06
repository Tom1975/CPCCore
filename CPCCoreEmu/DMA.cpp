#include "stdafx.h"
#include "DMA.h"
#include "Memoire.h"

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

DMA::DMA() : 
   dma_cycle_(NONE),
   enable_next_(false),
   dma_register_(nullptr),
   memory_(nullptr),
   dma_stop_(nullptr),
   channel_(0),
   curent_instr_(0),
   psg_(nullptr),
   pause_counter_(0),
   repeat_counter_(0),
   repeat_addr_(0),
   interrupt_on_(false), 
   prescalar_(0)
{

}


DMA::~DMA(void)
{
}


void DMA::HardReset()
{
   dma_cycle_ = NONE;
   pause_counter_ = 0;
   dma_cycle_ = NONE;
   repeat_addr_ = 0;
   repeat_counter_ = 0;
   interrupt_on_ = false;
   prescalar_ = 0;
}

// HBL occurs
void DMA::Hbl()
{
   // Anything to do ?
   switch (dma_cycle_)
   {
   case NONE:
   {
      // Fetch next Instruction
      unsigned short dma_addr = dma_register_[0]&0xFE;
      dma_addr |= (dma_register_[1]<<8);

      curent_instr_ = memory_->GetRamBuffer()[dma_addr++]; // lsb
      curent_instr_ |= ((memory_->GetRamBuffer()[dma_addr++]) << 8); // msb
      dma_register_[0] = dma_addr & 0xFF;
      dma_register_[1] = dma_addr >>8;


      // Decode it
      if ((curent_instr_ & 0x7000) == 0)
      {
         // RUN : TODO => Handle this wih correct timings
         unsigned char r = (curent_instr_ >> 8) & 0xF;
         unsigned char dd = (curent_instr_ ) & 0xFF;

         // Read state
         unsigned char old_reg = psg_->GetRegisterAdress();
         // Select register
         psg_->Access ( &r, 3, 0);
         // Write it
         psg_->Access(&dd, 2, 0);
         // Set as previous
         psg_->Access(&old_reg, 3, 0);
         break;
      }
      else
      {
         //case 0x1000: // PAUSE NNNN
         if ((curent_instr_ & 0x1000) == 0x1000)
         {
            pause_counter_ = (curent_instr_ & 0xFFF) /** (prescalar_+1)*/;
            prescalar_counter_ = 0; // (prescalar_ + 1);
            dma_cycle_ = PAUSE;

            if ( pause_counter_ == 0 ||
                (prescalar_ == 0 && pause_counter_ == 1))
            {
               dma_cycle_ = NONE;
            }
            else
            {
               prescalar_counter_++;
               if (prescalar_counter_ == prescalar_ + 1)
               {
                  pause_counter_--;
                  prescalar_counter_ = 0;// (prescalar_ + 1);
               }
            }
         }
         if ((curent_instr_ & 0x2000) == 0x2000)
         {
            repeat_counter_ = curent_instr_ & 0xFFF;
            repeat_addr_ = dma_addr;
         }
         if ((curent_instr_ & 0x4000) == 0x4000)
         {
            if ((curent_instr_ & 0x4000) == 0x4000)
            {
               // Nothing to do
            }
            if ((curent_instr_ & 0x4001) == 0x4001)
            {
               // Go to previous repeat
               if (repeat_counter_ > 1 /*!= 0*/ )
               {
                  --repeat_counter_;
                  dma_register_[0] = repeat_addr_ & 0xFF;
                  dma_register_[1] = repeat_addr_ >> 8;
               }

            }
            if ((curent_instr_ & 0x4010) == 0x4010)
            {
               interrupt_on_ = (memory_->GetIVR() & 0x01);
               sig_->interrupt_io_data_ = (memory_->GetIVR() & 0xF8) | ((2 - channel_) << 1);
               sig_->req_int_ = true;
               unsigned char dcsr = memory_->GetDCSR();
               dcsr |= (0x40 >> channel_);
               memory_->SetDCSR(dcsr);


            }
            if ((curent_instr_ & 0x4020) == 0x4020)
            {
               dma_stop_->DMAStop(channel_);

            }
         }
      }

      break;
   }
   case PAUSE:
   {
      prescalar_counter_++;
      //if (prescalar_counter_ >= prescalar_ + 1)
      while (prescalar_counter_ >= prescalar_ + 1)
      {
         prescalar_counter_ -= (prescalar_ + 1);
         // Reload prescalar counter
         if (--pause_counter_ == 0)
         {
            dma_cycle_ = NONE;
         }
/*         else
         {
            prescalar_counter_ = 0;// prescalar_ + 1;
         }*/
      }

      break;
   }

   }
}

void DMA::SetPrescalar(unsigned char prescalar)
{
   // Set prescalar seems to trigger the pause counter
   prescalar_ = prescalar;
   // Check currently prescalar run ?
   //while (prescalar_counter_ >= prescalar_ + 1)
   {
      //prescalar_counter_ -= (prescalar_ + 1);
      // Reload prescalar counter
      if (--pause_counter_ == 0)
      {
         dma_cycle_ = NONE;
      }
      prescalar_counter_ = 0;
   }
}

unsigned int DMA::Tick()
{

   // todo
   return 4;
}

void DMA::M1()
{
   // Arbitrate the CTC internal priorities
   // Set interrupt vector
}

