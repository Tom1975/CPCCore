#include "stdafx.h"
#include "Asic.h"

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

const unsigned char Asic::locking_sequence []= {0xFF, 0x77, 0xB3, 0X51, 0xA8, 0xD4, 0x62, 0x39, 0X9c, 0x46, 0x2B, 0x15, 0x8A};

Asic::Asic() : crtc_(nullptr), vga_(nullptr), index_verification_(0), log_(nullptr)
{

}


Asic::~Asic()
{
}

void Asic::Init(GateArray* vga, CRTC* crtc, ILog* log)
{
   log_ = log;
   vga_ = vga;
   crtc_ = crtc;
}

void Asic::HardReset()
{
   vga_->Unlock(false);
   index_verification_ = 0;
}


void Asic::M1()
{
   // Arbitrate the CTC internal priorities
   // Set interrupt vector
}


void Asic::Out(const unsigned short address, const unsigned char data)
{
   if ((address & 0x4300) == 0x0000
      || (address & 0x4300) == 0x0100
      )
   {
      // LOCK / UNLOCK ??
      switch (index_verification_)
      {
      case 0:        // Any value != 0 is good
         if (data) index_verification_++;
         break;
      case 1:        // only 0
         if (data==0) index_verification_++;
         break;
      case 15:       // cd : Wait another value, then unlock
         if (data == 0xCD) index_verification_++;
         else        // Other : Lock is set
         {
            LOG("[ASIC] : Locked");
            LOGEOL;
            vga_->Unlock(false);

            index_verification_ = 0;
         }
         break;

      case 16:       // Unlock
         vga_->Unlock(true);
         LOG("[ASIC ] : Unlocked");
         LOGEOL;

         index_verification_ = 0;
         break;
      default :      // 2-> 14 : Check in the array
         if (data == locking_sequence[index_verification_ - 2])
            index_verification_++;
         else
         {
            // Check : if ( index_verification_ > 2, Previous was not 0)
            if (/*index_verification_ > 2 && */data == 0)
            {
               // Sync is done !
               index_verification_ = 2;
            }
            else
            {
               index_verification_ = 1;
            }

         }
         break;

      }

      // Others features
      if (crtc_ != nullptr)
      {
         crtc_->Out(address, data);
      }
   }

   if ((address & 0xC000) == 0x4000
      || (address & 0x8000) == 0x0000
      || (address & 0x2000) == 0
      )
   {
      if (vga_ != nullptr)
      {
         vga_->TickIO();
      }
   }
}

void Asic::In(unsigned char* a, unsigned short address)
{
   if ((address & 0xC000) == 0x4000
      || (address & 0x8000) == 0x0000
      || (address & 0x2000) == 0
      )
   {
      if (vga_ != nullptr)
      {
         vga_->TickIO();
      }
   }
}
