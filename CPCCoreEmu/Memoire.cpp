#include "stdafx.h"
#include "Memoire.h"
#include "Monitor.h"
#include "VGA.h"
//#include "Shlwapi.h"


#define DEFAUT_VALUE 0
extern unsigned int ListeColorsIndex[0x100];



Memory::Memory(Monitor* monitor) : 
   lower_rom_available_(false), 
   expansion_(nullptr),
   ram_dis_(false),
   rom_dis_(false),
   connected_bank_(false),
   inf_rom_connected_(false),
   sup_rom_connected_(false),
   extended_pal_(false),
   rom_number_(0),
   plus_(false),
   asic_io_enabled_(false),
   rmr2_(0), 
   last_value_read_(0), 
   monitor_(monitor)
{
   memset(cartridge_, 0, sizeof(cartridge_));
   memset(cart_available_, 0, sizeof(cart_available_));
}


Memory::~Memory(void)
{
}


void Memory::InitMemory()
{
   int i;
   connected_bank_ = 0;

   memset(lower_rom_, DEFAUT_VALUE, sizeof(lower_rom_));
   lower_rom_available_ = false;
   for (i = 0; i < 4; i++)
   {
      memset(ram_buffer_[i], DEFAUT_VALUE, sizeof(RamBank));
   }

   memset(extended_ram_buffer_, DEFAUT_VALUE, sizeof(extended_ram_buffer_));

   for (i = 0; i < 256; i++)
   {
      rom_available_[i] = false;
   }
   memset(rom_, DEFAUT_VALUE, sizeof rom_);

   for (i = 0; i < 8; i++)
   {
      extended_ram_available_[i] = false;
   }

   // Default is : 6128 config
   extended_ram_available_[0] = true;
   extended_pal_ = true;
   expansion_ = nullptr;

   memset(asic_io_, 0, sizeof(asic_io_));
   memset(asic_io_rw_, N, sizeof(asic_io_rw_)); // Default is unused

   memset(asic_mask_w_, 0xFF, sizeof(asic_mask_w_)); // Default is "no mask"
   for (int i = 0; i < 0x1000; i++)  // Writing to any byte in the range &4000-&5000 will be masked with &0F
   {
      asic_mask_w_[i] = 0xF;
   }
   // Palette :When a byte is written to offset 1 and then read back it's value is returned & 0x0f. When writing to offset 0, and then read back, it's value is the same as written.
   for (int i = 0x2400; i < 0x2440; i++)  // Writing to any byte in the range &4000-&5000 will be masked with &0F
   {
      if ( i & 0x1)
         asic_mask_w_[i] = 0xF;
   }
                                                   // ASIC IO
   memset(&asic_io_rw_[0], RW, 0x1000);         // 0x4000 - 0x4FFF : Sprite 0-15 image data

                                                // For each sprite :
   for (int sprinte_index = 0; sprinte_index < 16; sprinte_index++)
   {
      asic_io_rw_[0x2000 + 8 * sprinte_index] = RW;   // X
      asic_io_rw_[0x2001 + 8 * sprinte_index] = RW;   // X
      asic_io_rw_[0x2002 + 8 * sprinte_index] = RW;   // Y
      asic_io_rw_[0x2003 + 8 * sprinte_index] = RW;   // Y
      asic_io_rw_[0x2004 + 8 * sprinte_index] = RW;   // magnification
      asic_io_rw_[0x2005 + 8 * sprinte_index] = RW;   // magnification
      asic_io_rw_[0x2006 + 8 * sprinte_index] = RW;   // magnification
      asic_io_rw_[0x2007 + 8 * sprinte_index] = RW;   // magnification
   }

   // For each color palette
   for (int color_palette_index = 0; color_palette_index < 16; color_palette_index++)
   {
      memset(&asic_io_rw_[0x2400 + 2 * color_palette_index], RW, 2);   // Colour for pen 'color_palette_index'
   }
   memset(&asic_io_rw_[0x2420], RW, 2);   // Colour for border

                                          // Sprite palette
   for (int color_palette_index = 0; color_palette_index < 15; color_palette_index++)
   {
      memset(&asic_io_rw_[0x2422 + 2 * color_palette_index], RW, 2);   // Colour for pen 'color_palette_index'
   }

   // PRI
   memset(&asic_io_rw_[0x2800], W, 1);
   // SPLT
   memset(&asic_io_rw_[0x2801], W, 1);
   // SSA
   memset(&asic_io_rw_[0x2802], W, 2);
   // SSCR
   // at most times, the split value is captured when HCC=R1 (i.e. border starts on horizontal).
   // BUT when VCC=R4 and RCC=R9 (i.e. last scanline of frame) the split value is captured at HCC=R0 and is then used when RCC=1, VCC=0
   memset(&asic_io_rw_[0x2804], W, 1);

   // IVR
   memset(&asic_io_rw_[0x2805], W, 1);

   // Analog input channel : ADC0 - 7
   for (int input_chan_index = 0; input_chan_index < 8; input_chan_index++)
   {
      memset(&asic_io_rw_[0x2808 + input_chan_index], R, 1);
   }

   // DMA
   memset(&asic_io_rw_[0x2C00], W, 2); // SAR0
   memset(&asic_io_rw_[0x2C02], W, 1); // PPR0

   memset(&asic_io_rw_[0x2C04], W, 2); // SAR1
   memset(&asic_io_rw_[0x2C06], W, 1); // PPR1

   memset(&asic_io_rw_[0x2C08], W, 2); // SAR2
   memset(&asic_io_rw_[0x2C0A], W, 1); // PPR2

   memset(&asic_io_rw_[0x2C0F], RW, 1); // DCSR

   asic_io_enabled_ = false;

   // Initial values :
   // IVR bit 1 = 1
   asic_io_[0x2805] = 0x1;

   // sure ???
   rom_number_ = 1;
   sup_rom_connected_ = true;
   SetMemoryMap();
}

unsigned short Memory::GetDebugMaxAdress(DbgMemAccess acces)
{
   switch (acces)
   {
   case MEM_READ:
   case MEM_WRITE:
   case MEM_RAM_LOWER_BANK:
   case MEM_RAM_BANK:
      return 0xFFFF;
   default:
      return 0x3FFF;
   }
}

unsigned int Memory::GetDebugValue(unsigned char * address_buffer, unsigned short adress_start, unsigned int size_of_buffer, DbgMemAccess acces, unsigned int data)
{
   // Fill the buffer with values from needed accès
   unsigned short max_size = 0;
   switch (acces)
   {
   case MEM_READ:
   {
      for (unsigned int i = 0; i < size_of_buffer; i++)
      {
         unsigned short mem_addr = i + adress_start;
         if (asic_io_enabled_ && mem_addr >= 0x4000 && mem_addr <= 0x7FFF)
         {
            address_buffer[i] = ReadAsicRegister(mem_addr);
         }
         else
         {
            address_buffer[i] = (*ram_read_[mem_addr >> 14])[mem_addr & 0x3FFF];
         }
      }
      break;
   }
   case MEM_WRITE:
   {
      unsigned short mem_addr = adress_start;
      for (unsigned int i = 0; i < size_of_buffer; i++)
      {
         address_buffer[i] = (*ram_write_[mem_addr >> 14])[mem_addr & 0x3FFF];
         mem_addr++;
      }
      break;

   }
   case MEM_RAM_LOWER_BANK:
   {
      unsigned short mem_addr = adress_start;
      for (unsigned int i = 0; i < size_of_buffer; i++)
      {
         address_buffer[i] = ram_buffer_[0][mem_addr & 0x3FFF];
         mem_addr++;
      }

      break;
   }
   case MEM_RAM_BANK:
   {
      unsigned short mem_addr = adress_start;

      for (unsigned int i = 0; i < size_of_buffer; i++)
      {
         address_buffer[i] = (extended_ram_buffer_[data][0])[mem_addr & 0x3FFF];
         mem_addr++;
      }
      break;
   }
   case MEM_LOWER_ROM:
   {
      unsigned short mem_addr = adress_start;
      for (unsigned int i = 0; i < size_of_buffer; i++)
      {
         address_buffer[i] = lower_rom_[mem_addr & 0x3FFF];
         mem_addr++;
      }
      break;
   }
   case MEM_ROM_BANK:
   {
      RamBank* rom_bank;
      unsigned short mem_addr = adress_start;
      // Check ??
      rom_bank = &rom_[data];

      for (unsigned int i = 0; i < size_of_buffer; i++)
      {
         address_buffer[i] = (*rom_bank)[mem_addr & 0x3FFF];
         mem_addr++;
      }
      break;
   }
   case MEM_CART_SLOT:
   {
      RamBank* crt_bank = &cartridge_[data&0x1F];
      unsigned short mem_addr = adress_start;
      for (unsigned int i = 0; i < size_of_buffer; i++)
      {
         address_buffer[i] = (*crt_bank)[mem_addr & 0x3FFF];
         mem_addr++;
      }
      break;
   }
   }
   return max_size;
}

unsigned char Memory::ReadAsicRegister(unsigned short i)
{
   // Ok access here ?
   if ((asic_io_rw_[i - 0x4000] & R) == R)
   {
      // Specific values :
      // Sprite magnification
      if (  ( i >= 0x6000 && i < 0x6080)
         )
      {
         switch (i & 0x3)
         {
         case 0:
         case 2:
            return asic_io_[(i & 0xFFFB) - 0x4000];
         case 1:
            if ((asic_io_[(i & 0xFFFB) - 0x4000] & 0x3) == 0x3)
            {
               return 0xFF;
            }
            else
            {
               return (asic_io_[(i & 0xFFFB) - 0x4000] & 0x3);
            }
         case 3:
            {
               return (asic_io_[(i & 0xFFFB) - 0x4000] & 0x1) ? 0xFF : 0;
            }
         }
      }
      else
         // Analog
         if (i >= 0x6808 && i <= 0x680F)
         {
            return 0x3F; // TODO : Nothing is plugged. Handled plugged devices
         }
         else
         return asic_io_[i - 0x4000];
   }
   else
   {
      // It depends :
      // http://cpctech.cpc-live.com/docs/cpcplus.html : See "Sprite Position and magnification information " section
      // Sprite magnification : Read X-Y coord instead
      if ((i >= 0x6000 && i < 0x6080)
         )
      {
         switch (i & 0x3)
         {
         case 0:
         case 2:
            return asic_io_[(i & 0xFFF3) - 0x4000];
         case 1:
            if ((asic_io_[(i & 0xFFF3) - 0x4000] & 0x3) == 0x3)
            {
               return 0xFF;
            }
            else
            {
               return (asic_io_[(i & 0xFFF3) - 0x4000] & 0x3);
            }
         case 3:
         {
            return (asic_io_[(i & 0xFFF3) - 0x4000] & 0x1) ? 0xFF : 00;
         }
         }
      }
      else
      // DCSR
      if (i >= 0x6c00 && i < 0x6C0F)
      {
         return asic_io_[0x2C0F];
      }

      // todo : check behaviour for thoses values !
      // the last byte of the instruction used to do the read is return. e.g. if LD A,(&5000) is done, the last byte of this instruction is
   }
   return last_value_read_;
}

void Memory::UpdateAsicPalette(unsigned char color_index, unsigned char hardware_color)
{
   unsigned char r, g, b;
   switch (hardware_color & 0x1F)
   {
   case 0: r = g = b = 6; break;
   case 1: r = g = b = 6; break;
   case 2: r = 0;  g = 15; b = 6 ; break;
   case 3: r = 15; g = 15; b = 6 ; break;
   case 4: r = 0 ; g = 0 ; b = 6; break;
   case 5: r = 15; g = 0; b = 6 ; break;
   case 6: r = 0 ; g = 6; b = 6; break;
   case 7: r = 15; g = 6; b = 6; break;
   case 8: r = 15; g = 0; b = 6; break;
   case 9: r = 15; g = 15; b = 6; break;
   case 10: r = 15; g = 15; b = 0; break;
   case 11: r = 15; g = 15; b = 15; break;
   case 12: r = 15; g = 0 ; b = 0 ; break;
   case 13: r = 15; g = 0 ; b = 15; break;
   case 14: r = 15; g = 6; b = 0 ; break;
   case 15: r = 15; g = 6; b = 15; break;
   case 16: r = 0 ; g = 0 ; b = 6 ; break;
   case 17: r = 0; g = 15; b = 6 ; break;
   case 18: r = 0 ; g = 15; b = 0; break;
   case 19: r = 0 ; g = 15; b = 15; break;
   case 20: r = 0; g = 0; b = 0; break;
   case 21: r = 0; g = 0; b = 15; break;
   case 22: r = 0; g = 6; b = 0; break;
   case 23: r = 0; g = 6; b = 15; break;
   case 24: r = 6; g = 0; b = 6 ; break;
   case 25: r = 6; g = 15; b = 6; break;
   case 26: r = 6; g = 15; b = 0 ; break;
   case 27: r = 6; g = 15; b = 15; break;
   case 28: r = 6; g = 0 ; b = 0 ; break;
   case 29: r = 6; g = 0; b = 15; break;
   case 30: r = 6; g = 6; b = 0; break;
   case 31:
   default:
      r = 6; g = 6; b = 15; break;
   }
   // Pen or border ?
   if (color_index >= 0 && color_index < 16)
   {
      // Pen
      asic_io_[0x2400+ color_index*2] = (b | (r << 4));
      asic_io_[0x2401+ color_index * 2] = (g);

      r <<= 4;
      g <<= 4;
      b <<= 4;
      monitor_->gate_array_->ink_list_[color_index] = (r << 16) + (g << 8) + (b);
   }
   else
   {
      // Border
      asic_io_[0x2420] = (b | (r << 4));
      asic_io_[0x2421] = (g);
      for (int i = 0; i < NB_BYTE_BORDER; i++)monitor_->gate_array_->video_border_[i] = ListeColorsIndex[hardware_color | 0x40];
   }
}

void Memory::WriteAsicRegister(unsigned short addr, unsigned char data)
{
   // Ok access here ?
   if ((asic_io_rw_[addr - 0x4000] & W) == W)
   {
      if (addr >= 0x6000 && addr < 0x6080)
      {
         asic_io_[addr - 0x4000] = data & asic_mask_w_[addr - 0x4000];

         // Sprites
         int num_sprite = (addr & 0x78 ) >> 3;
         unsigned char* sprite = &asic_io_[0x2000 + (addr & 0x78)];

         if ((addr & 0x6) == 0)
         {
            sprite_info_[num_sprite].x = (sprite[0] + ((sprite[1] & 0x3) << 8));
            if ((sprite_info_[num_sprite].x & 0x300) == 0x300)
            {
               sprite_info_[num_sprite].x = (-1 * ((~sprite_info_[num_sprite].x) & 0xFF)) - 1;
            }
         }
         else if ((addr & 0x6) == 2)
         {
            sprite_info_[num_sprite].y = (sprite[2] + ((sprite[3] & 0x1) << 8));
            if (sprite_info_[num_sprite].y & 0x100)
            {
               sprite_info_[num_sprite].y = (-1 * ((~sprite_info_[num_sprite].y) & 0xFF))-1;
            }
         }
         else if ((addr & 0x7) == 4)
         {
            sprite_info_[num_sprite].zoomx = ((sprite[4] & 0xC) >> 2);
            sprite_info_[num_sprite].zoomy = (sprite[4] & 0x3);
            sprite_info_[num_sprite].displayed = (sprite_info_[num_sprite].zoomx > 0 && sprite_info_[num_sprite].zoomy > 0);
            if (sprite_info_[num_sprite].displayed)
            {
               sprite_info_[num_sprite].sizex = 16 << (sprite_info_[num_sprite].zoomx - 1);
               sprite_info_[num_sprite].sizey = 16 << (sprite_info_[num_sprite].zoomy - 1);
            }
         }

      }

      // DMA
      else if (addr == 0x6C02)
      {
         monitor_->gate_array_->GetDMAChannel(0)->SetPrescalar(data);
         asic_io_[addr - 0x4000] = data & asic_mask_w_[addr - 0x4000];
      }
      else if (addr == 0x6C06)
      {
         monitor_->gate_array_->GetDMAChannel(1)->SetPrescalar(data);
         asic_io_[addr - 0x4000] = data & asic_mask_w_[addr - 0x4000];
      }
      else if (addr == 0x6C0A)
      {
         monitor_->gate_array_->GetDMAChannel(2)->SetPrescalar(data);
         asic_io_[addr - 0x4000] = data & asic_mask_w_[addr - 0x4000];
      }
      else if (addr == 0x6C0F)
      {
         // bit 1 = acknoledge interrupt
         if (data & 0x80)
         {
            // Ack Raster

         }
         if (data & 0x40)
         {
            // Ack DMA 0
            monitor_->gate_array_->GetDMAChannel(0)->AcqInt();
            data &= ~0x40;
         }
         if (data & 0x20)
         {
            // Ack DMA 1
            monitor_->gate_array_->GetDMAChannel(1)->AcqInt();
            data &= ~0x20;
         }
         if (data & 0x10)
         {
            // Ack DMA 2
            monitor_->gate_array_->GetDMAChannel(2)->AcqInt();
            data &= ~0x10;
         }
         asic_io_[0x2C0F] = data & 0x7;
      }
      else
      {
         if (addr == 0x6800)
         {
            // If not 0 : Reset raster interrupt (to be tested !)
            if (data != 0 && asic_io_[0x2800] != data)
            {
               if (monitor_->gate_array_->sig_handler_->int_is_gate_array_)
               {
                  monitor_->gate_array_->sig_handler_->int_is_gate_array_ = false;
                  monitor_->gate_array_->sig_handler_->int_ = 0;
               }
               // Can trigger interrupt if FF2 is on
               monitor_->gate_array_->sig_handler_->pri_changed_ = true;
            }
         }
         asic_io_[addr - 0x4000] = data & asic_mask_w_[addr - 0x4000];

         // Palette ?
         if (addr >= 0x6400 && addr < 0x6420)
         {
            int p = (addr - 0x6400) >> 1;
            // Get r, g, b
            unsigned char r = (asic_io_[(addr - 0x4000) & 0xFFFE] >> 4);
            unsigned char b = asic_io_[(addr - 0x4000) & 0xFFFE] & 0x0F;
            unsigned char g = asic_io_[((addr - 0x4000) & 0xFFFE) + 1] & 0xF;

            r <<= 4;
            g <<= 4;
            b <<= 4;

            monitor_->gate_array_->ink_list_[p] = (r << 16) + (g << 8) + (b);

            // Recompute pen/border
            //monitor_->RecomputeAllColors();
         }
         else if (addr == 0x6420 || addr == 0x6421)
         {
            // Border
            unsigned char r = (asic_io_[(addr - 0x4000) & 0xFFFE] >> 4);
            unsigned char b = asic_io_[(addr - 0x4000) & 0xFFFE] & 0x0F;
            unsigned char g = asic_io_[((addr - 0x4000) & 0xFFFE) + 1] & 0xF;

            r <<= 4;
            g <<= 4;
            b <<= 4;
            for (int i = 0; i < NB_BYTE_BORDER; i++)monitor_->gate_array_->video_border_[i] = (r << 16) + (g << 8) + (b);
         }
         // Sprite palette
         else if (addr >= 0x6422 && addr < 0x6440)
         {
            int p = (addr - 0x6420) >> 1;
            // Get r, g, b
            unsigned char r = (asic_io_[(addr - 0x4000) & 0xFFFE] >> 4);
            unsigned char b = asic_io_[(addr - 0x4000) & 0xFFFE] & 0x0F;
            unsigned char g = asic_io_[((addr - 0x4000) & 0xFFFE) + 1] & 0xF;

            r <<= 4;
            g <<= 4;
            b <<= 4;

            monitor_->gate_array_->sprite_ink_list_[p] = (r << 16) + (g << 8) + (b);

         }
         // SSA
         else if (addr == 0x6802 || addr == 0x6803)
         {
            // Delay the timing by 2 us
            monitor_->gate_array_->ssa_new_counter_ = 1;
            monitor_->gate_array_->ssa_new_ = asic_io_[0x2803] + ((asic_io_[0x2802] & 0x3F) << 8);
         }
         // DCSR
         else if (addr == 0x6C0F)
         {
            // Acq DMA channels
            if (data & 0x40)
               monitor_->gate_array_->GetDMAChannel(0)->AcqInt();
            if (data & 0x20)
               monitor_->gate_array_->GetDMAChannel(1)->AcqInt();
            if (data & 0x10)
               monitor_->gate_array_->GetDMAChannel(2)->AcqInt();
         }
      }
   }
   else
   {
      // It depends :
      // http://cpctech.cpc-live.com/docs/cpcplus.html : See "Sprite Position and magnification information " section
      // Sprite magnification : Read X-Y coord instead
      // todo
      
      // Palette : Same
   }
}

void Memory::SetRam ( unsigned char RAMavailable )
{
   extended_pal_ = false;
   for (int i = 0; i < 8; i++)
   {
      extended_ram_available_[i] = ((RAMavailable>>i)&0x01);
      extended_pal_ |= extended_ram_available_[i];
   }
}

void Memory::Initialisation  ()
{

   rom_dis_ = ram_dis_ = false;

   memset(sprite_info_, 0, sizeof(sprite_info_));

   // Initial configuraiton
   for (int i = 0; i < 4; i++)
   {
      memset (ram_buffer_[i], DEFAUT_VALUE, sizeof(RamBank));
   }

   memset(extended_ram_buffer_, DEFAUT_VALUE, sizeof( extended_ram_buffer_) );
   for (int i = 0; i < 4; i++)
   {
      ram_read_[i] = &ram_buffer_[i];
      ram_write_[i] = &ram_buffer_[i];
   }

   inf_rom_connected_ = true;
   //  #0000 à #003F : réservé (copie de la ROM inférieure) (64 octets)
   memset(asic_io_, DEFAUT_VALUE, sizeof(asic_io_));

   //
   //m_InfROMConnected = false;
   sup_rom_connected_ = true;
   rom_number_ = 1;
   ResetStockAddress();
   SetMemoryMap();
}

void Memory::ClearRom(int rom_number)
{
   if (rom_number < -1 || rom_number > 256) return;
   unsigned char* rom = (rom_number==-1)?lower_rom_: rom_[rom_number];
   memset(rom, 0, sizeof(RamBank));
}

bool Memory::LoadLowerROM (unsigned char* rom_from, unsigned int size)
{

   unsigned char* rom = lower_rom_;
   memcpy(rom, rom_from, (sizeof(RamBank) < size) ? sizeof(RamBank) : size);
   lower_rom_available_ = true;
   return true;
}

bool Memory::LoadROM (unsigned char rom_number, unsigned char* rom_from, unsigned int size)
{
   unsigned char* rom = rom_[rom_number];
   memcpy(rom, rom_from, (sizeof(RamBank) < size) ? sizeof(RamBank) : size);
   rom_available_[rom_number] = true;
   return true;
}


void Memory::RomDis (bool set)
{
   rom_dis_ = set;
   SetMemoryMap();
}

void Memory::RamDis (bool set)
{
   ram_dis_ = set;
   SetMemoryMap();
}


void Memory::SetInfROMConnected (bool set)
{

   inf_rom_connected_ = set;
   SetMemoryMap();
}
void Memory::SetSupROMConnected (bool set)
{

   sup_rom_connected_ = set;

   SetMemoryMap();
}

void Memory::SetLogicalROM ( unsigned char num )
{
   rom_number_ = num;
   // Actuellement, : uniquement ROM 0 et 7
   if (plus_)
   {
      // ROM available ?
      if (rom_number_ & 0x80)
      {
         // Physical ROM
         rom_number_ = num & 0x9F;
      }
      else
      {
         // todo : Check this !!!
         //if (!rom_available_[rom_number_])
         //   rom_number_ = 0;
      }
   }
   else
   {
      if (!rom_available_[rom_number_])
         rom_number_ = 0;
   }
   SetMemoryMap();
}

void Memory::ConnectBank(unsigned char p, unsigned s, unsigned char b)
{
   connected_bank_ = (p << 3) | b | (s << 2);
   SetMemoryMap();
}

void Memory::SetMemoryMap ()
{
   unsigned char p = connected_bank_ & 0x38;
   p = p >> 3;
   unsigned char b = connected_bank_ & 0x3;
   unsigned char s = connected_bank_ & 0x4;
   s = s >> 2;

   if (!extended_ram_available_[p])
   {
      p = 0;
   }

   for (int i = 0; i < 4; i++)
   {
      ram_read_[i] = &ram_buffer_[i];
      ram_write_[i] = &ram_buffer_[i];
   }

   unsigned char* rom_exp = ((expansion_==nullptr)? nullptr :expansion_->GetROM());

   if ( s == 0)
   {
      // depending on bank :
      switch (b)
      {
      case 0: // RAM centrale lineaire bank deconnectées
         {
         }
         break;
      case 1: // Bank 3 de la premiere page en &c000 - &ffff
         ram_read_[3] = &extended_ram_buffer_[p][3];
         ram_write_[3] = &extended_ram_buffer_[p][3];
         break;

      case 2:  // Premiere page en &0000-&ffff
         {
            for (int i = 0; i < 4; i++)
            {
               if (i != 0 || (rom_exp == NULL) || (!ram_dis_ && !rom_dis_) )
                  ram_read_[i] = &extended_ram_buffer_[p][i];

               if (i != 0 || (rom_exp == NULL) || (!ram_dis_ ))
                  ram_write_[i] = &extended_ram_buffer_[p][i];
            }
         }
         break;

      case 3:  // Bank 3 de la premiere page en &c000-&ffff et RAM centrale &c000-&ffff en &4000-&7fff
         ram_read_[3] = &extended_ram_buffer_[p][3];
         ram_write_[3] = &extended_ram_buffer_[p][3];

         ram_read_[1] = &ram_buffer_[3];
         ram_write_[1] = &ram_buffer_[3];

         break;
      default:
         // ERROR !
         break;
      }
   }
   else
   {
      // Connect page p bank b to &4000-&7FFF
      ram_read_[1] = &extended_ram_buffer_[p][b];
      ram_write_[1] = &extended_ram_buffer_[p][b];

   }
   if (ram_dis_ && (rom_exp != NULL))
   {
      ram_write_[0] = (RamBank*)rom_exp;
   }
   if ((rom_dis_) && (rom_exp != NULL))
   {
      ram_read_[0] = (RamBank*)rom_exp;
   }

   if (inf_rom_connected_)
   {
      if (plus_)
      {
         // RMR2
         // 0-2 : ROM low+
         unsigned int p = (rmr2_ & 0x7);

         // 3-4 : conf ROM/ASIC
         switch (rmr2_ & 0x18)
         {
         case 0x00:
            // ROM p 0x0000-0x3fff
            ram_read_[0] = &cartridge_[p];
            // ASIC IO unpaged
            asic_io_enabled_ = false;
            break;
         case 0x08:
            // ROM p 0x4000-0x7FFF
            ram_read_[1] = &cartridge_[p];
            // ASIC IO unpaged
            asic_io_enabled_ = false;
            break;
         case 0x10:
            // ROM p 0x8000-0xBFFF
            ram_read_[2] = &cartridge_[p];
            // ASIC IO unpaged
            asic_io_enabled_ = false;
            break;
         case 0x18:
            // ROM p 0x0000-0x3fff
            ram_read_[0] = &cartridge_[p];
            // ASIC IO paged
            asic_io_enabled_ = true;
            break;
         default:
            asic_io_enabled_ = true;
            break;
         }
      }
      else
      {
         ram_read_[0] = &lower_rom_;
      }
   }
   if (sup_rom_connected_)
   {
      if (plus_)
      {
         if ((rom_number_ & 0x80) == 0x80)
         {
            // Physical ROM from cartridge
            ram_read_[3] = &cartridge_[rom_number_ & 0x1F];
         }
         else
         {
            // Logical ROM :
            // 0 => Cartridge Physical ROM 1
            // 7 => Cartridge Physical ROM 3

            if (rom_number_ == 0)
               ram_read_[3] = &cartridge_[1];
            else if (rom_number_ == 7)
               ram_read_[3] = &cartridge_[3];
            else
            {
               if (rom_available_[rom_number_])
               {
                  ram_read_[3] = &rom_[rom_number_];
               }
               else
               {
                  ram_read_[3] = &cartridge_[1];
               }
            }
         }
      }
      else
      {
         if (!rom_available_[rom_number_])
            rom_number_ = 0;

         ram_read_[3] = &rom_[rom_number_];
      }
   }
}




void Memory::DMAStop(int channel)
{
   // Set DCSR bit to 0
   asic_io_[0x2C0F] &= ~( 1<<channel);
}

Memory* Memory::CopyMe()
{
   Memory* new_memory = new Memory(monitor_);
   *new_memory = *this;

   return new_memory;
}

void Memory::DeleteCopy(Memory* memory)
{
   delete memory;
}

bool Memory::CompareToCopy(Memory*other)
{
   // Compare two memory
   if (memcmp(rom_, other->rom_, sizeof(rom_)) != 0) return false;
   if (memcmp(ram_buffer_, other->ram_buffer_, sizeof(ram_buffer_)) != 0) return false;

   // This one is not mandatory : Remember that cartridge is NOT part of the snapshot
   if (memcmp(cartridge_, other->cartridge_, sizeof(cartridge_)) != 0) return false;
   if (memcmp(cart_available_, other->cart_available_, sizeof(cart_available_)) != 0) return false;
   if (ram_dis_ != other->ram_dis_) return false;
   if (rom_dis_ != other->rom_dis_) return false;

   if (connected_bank_ != other->connected_bank_) return false;
   if (inf_rom_connected_ != other->inf_rom_connected_) return false;
   if (sup_rom_connected_ != other->sup_rom_connected_) return false;

   for (int i = 0; i < 4; i++)
   {
      if (memcmp(ram_read_[i], other->ram_read_[i], sizeof(RamBank)) != 0) return false;
      if (memcmp(ram_write_[i], other->ram_write_[i], sizeof(RamBank)) != 0) return false;
   }

   if (memcmp(asic_io_, other->asic_io_, sizeof(asic_io_)) != 0) return false;
   if (memcmp(asic_mask_w_, other->asic_mask_w_, sizeof(asic_mask_w_)) != 0) return false;
   if (memcmp(asic_io_rw_, other->asic_io_rw_, sizeof(asic_io_rw_)) != 0) return false;

   if (extended_pal_ != other->extended_pal_) return false;

   if (memcmp(extended_ram_available_, other->extended_ram_available_, sizeof(extended_ram_available_)) != 0) return false;

   for (int i = 0; i < 8; i++)
   {
      for (int j = 0; j < 4; j++)
      {
         if (memcmp(extended_ram_buffer_[i][j], other->extended_ram_buffer_[i][j], sizeof(RamBank)) != 0) return false;
      }
   }

   if (rom_number_ != other->rom_number_) return false;
   if (memcmp(lower_rom_, other->lower_rom_, sizeof(lower_rom_)) != 0) return false;
   if (memcmp(rom_available_, other->rom_available_, sizeof(rom_available_)) != 0) return false;

   if (plus_ != other->plus_) return false;
   if (asic_io_enabled_ != other->asic_io_enabled_) return false;

   if (rmr2_ != other->rmr2_) return false;
   if (last_value_read_ != other->last_value_read_) return false;

   return true;
}
