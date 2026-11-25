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
   prescalar_(0),
   prescalar_counter_(0),
   fetch_in_(0),
   exec_in_(0),
   prescalar_write_(false)
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
   prescalar_counter_ = 0;
   fetch_in_ = 0;
   exec_in_ = 0;
   prescalar_write_ = false;
}

// Start of the HSYNC, the logic of the channel is synchronous to it (PROVISIONAL MODEL, RealCpc README P12).
// A channel in PAUSE clocks its prescaler counter : the counter clears when it reaches the prescaler, and this
// clear clocks the pause counter down. A write of the prescaler register (SetPrescalar) is latched and taken
// on the next HSYNC, in place of the count : the prescaler counter clears and clocks the pause counter. When
// the pause counter reaches 0, the channel asks for the RAM on this same HSYNC : PAUSE N lets
// N * (prescaler + 1) - 1 HSYNC go by without a fetch, PAUSE 0 and PAUSE 1 (prescaler 0) do not pause.
// Measured, plusdma section C (real 6128 Plus, PAUSE 16 with the prescaler p1, p2 written after k lines) :
// the 8 x 16 values (4>4 : 81 76 76 76 76 76 71...), test AsicDma.PauseAndPrescalerWritesGiveTheTableOfTheRealPlus.
// A channel that does not pause asks for the RAM.
bool DMA::HsyncTick()
{
   const bool write = prescalar_write_;
   prescalar_write_ = false;
   if (dma_cycle_ != PAUSE)
   {
      return true;
   }
   if (write || ++prescalar_counter_ >= prescalar_ + 1)
   {
      prescalar_counter_ = 0;
      --pause_counter_;
   }
   if (pause_counter_ <= 0)
   {
      dma_cycle_ = NONE;
      return true;
   }
   return false;
}

void DMA::Clock()
{
   if (fetch_in_ != 0 && --fetch_in_ == 0)
   {
      Fetch();
   }
   if (exec_in_ != 0 && --exec_in_ == 0)
   {
      Execute();
   }
}

// The 2 bytes of the instruction, read on the RAM slot the sequencer gives the channel
void DMA::Fetch()
{
   unsigned short dma_addr = dma_register_[0] & 0xFE;
   dma_addr |= (dma_register_[1] << 8);

   curent_instr_ = memory_->GetRamBuffer()[dma_addr++]; // lsb
   curent_instr_ |= ((memory_->GetRamBuffer()[dma_addr++]) << 8); // msb
   dma_register_[0] = dma_addr & 0xFF;
   dma_register_[1] = dma_addr >> 8;
}

void DMA::Execute()
{
   // Decode it
   if ((curent_instr_ & 0x7000) == 0)
   {
      // LOAD R,DD : the register of the PSG
      unsigned char r = (curent_instr_ >> 8) & 0xF;
      unsigned char dd = (curent_instr_) & 0xFF;

      // Read state
      unsigned char old_reg = psg_->GetRegisterAdress();
      // Select register
      psg_->Access(&r, 3, 0);
      // Write it
      psg_->Access(&dd, 2, 0);
      // Set as previous
      psg_->Access(&old_reg, 3, 0);
      return;
   }

   // PAUSE NNNN : the pause counter is loaded, the prescaler counter cleared (see HsyncTick). PAUSE 0 does
   // not pause.
   if ((curent_instr_ & 0x1000) == 0x1000)
   {
      pause_counter_ = (curent_instr_ & 0xFFF);
      prescalar_counter_ = 0;
      dma_cycle_ = (pause_counter_ == 0) ? NONE : PAUSE;
   }
   if ((curent_instr_ & 0x2000) == 0x2000)
   {
      unsigned short dma_addr = dma_register_[0] | (dma_register_[1] << 8);
      repeat_counter_ = curent_instr_ & 0xFFF;
      repeat_addr_ = dma_addr;
   }
   if ((curent_instr_ & 0x4000) == 0x4000)
   {
      if ((curent_instr_ & 0x4001) == 0x4001)
      {
         // Go to previous repeat
         if (repeat_counter_ > 1 /*!= 0*/)
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

// Write of the prescaler register : the strobe of the Z80 is latched, the channel takes it on the next HSYNC
// (see HsyncTick)
void DMA::SetPrescalar(unsigned char prescalar)
{
   prescalar_ = prescalar;
   prescalar_write_ = true;
}

void DMA::ClearPrescalarWrite()
{
   prescalar_write_ = false;
}

unsigned int DMA::Tick()
{
   // This is not handled it by now...
   return 4000000;
}

void DMA::M1()
{
   // Arbitrate the CTC internal priorities
   // Set interrupt vector
}

