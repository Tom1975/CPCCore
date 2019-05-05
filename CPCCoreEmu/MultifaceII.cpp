#include "stdafx.h"
#include "MultifaceII.h"
#include "Sig.h"

#include "Multiface Two (Romantic Robot) (8D) (AmsDOS v05)_rom.h"


MultifaceII::MultifaceII(void) : nmi_enabled_(false)
{
   sig_ = NULL;
   visible_ = false;
   ram_zone_ = &rom_ram_[0];
   memset(rom_ram_, 0, sizeof(rom_ram_));
}


MultifaceII::~MultifaceII(void)
{
}

void MultifaceII::Init (CSig* sig)
{
   sig_ = sig;
   sig_ ->memory_->expansion_ = this;

   // Init m_ROMRAM
   memcpy(rom_ram_, MultiROMV05, sizeof(MultiROMV05));
}

unsigned int MultifaceII::Tick ()
{
   return 1000000;
}

   // Out
void MultifaceII::Out (unsigned short Addr_P, unsigned char Data_P )
{
   if (visible_)
   {
      if ( (Addr_P & 0xFFFE) ==  0xFEE8)
      {
         // Enable ROM/RAM
         sig_->memory_->RomDis(true);
         sig_->memory_->RamDis(true);

      }
      else if ( (Addr_P & 0xFFFE) ==  0xFEEA)
      {
         // Disable ROM/RAM
         sig_->memory_->RomDis(false);
         sig_->memory_->RamDis(false);
      }
   }

   // Get the IO for every component when it occurs
   // 0xF7xx - PPI
   if ((Addr_P & 0xFF00) == 0xF700)
   {
      ram_zone_[0x37FF] = Data_P;
   }
   // 0xBCxx - CRTC
   if ((Addr_P & 0xFF00) == 0xBC00)
   {
      ram_zone_[0x3CFF] = Data_P;
   }

   // Register of CRTC
   if ((Addr_P & 0xFF00) == 0xBD00)
   {
      ram_zone_[0x3DB0 +(ram_zone_[0x3CFF]&0xF)] = Data_P;
   }

   // Gate Array
   if ((Addr_P & 0xFF00) == 0x7F00)
   {
      // Color palette
      if ((Data_P & 0xC0) == 0x40)
      {
         if (ram_zone_[0x3FCF] < 0x10)
         {
            ram_zone_[0x3F10 + ram_zone_[0x3FCF]] = Data_P&0x1F;
            ram_zone_[0x3F90 + ram_zone_[0x3FCF]] = Data_P&0x1F;
         }
         else
         {
            // Border
            ram_zone_[0x3FD0 ] = Data_P&0x1F;
         }
      }

      // PENR 0x7F
      if ((Data_P & 0xC0) == 0x00)
      {
         ram_zone_[0x3FCF] = Data_P;
      }

      // ROM from GA (RMR)
      if ((Data_P & 0xC0) == 0x80)
      {
         ram_zone_[0x3FEF] = Data_P/*&0x3F*/;
      }
      // RAM from GA (MMR)
      if ((Data_P & 0xC0) == 0xC0)
      {
         ram_zone_[0x3FFF] = Data_P/*&0x3F*/;
      }
   }




}

   // In
void MultifaceII::In (unsigned char* data, unsigned short address)
{
}

unsigned char*  MultifaceII::GetROM()
{
   return rom_ram_;
}

void MultifaceII::Reset ()
{
   if ( sig_ != NULL)
   {
      sig_ ->Reset ();
   }
}

void MultifaceII::M1 ()
{
   // NMI ACK => ROMDIS
   if (nmi_enabled_ )
   {
      sig_->memory_->RomDis(true);
      sig_->memory_->RamDis(true);
      nmi_enabled_ = false;
   }

}

void MultifaceII::Stop ()
{
   // NMI
   // Sure ???
   //if (m_bVisible)
   {
      sig_->nmi_ = true;
      nmi_enabled_ = true;
   }
}
