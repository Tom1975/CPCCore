#pragma once

#include <string.h>

#include "simple_string.h"
#include "IExpansion.h"
#include "DMAStop.h"


class Monitor;


class Memory : public IDmaSTOP
{
   friend class EmulatorEngine;
   friend class CSnapshot;

public:
   // RAM bank
   typedef unsigned char RamBank [0x4000];

   enum {
      N = 0,
      R = 1,
      W = 2,
      RW= 3
   } asic_rw_;

public:
   Memory(Monitor* monitor);
   ~Memory(void);

   void InitMemory();
   void Initialisation  ();
   void SetPlus(bool plus) { plus_ = plus; }

   bool ExtendedPAL () { return extended_pal_;}
   void RomDis (bool set);
   void RamDis (bool set);

   void ClearRom(int rom_number);
   bool LoadROM (unsigned char rom_number, unsigned char* rom_from, unsigned int size);
   bool LoadLowerROM (unsigned char* rom_from, unsigned int size);

   unsigned char* GetAsicRegisters() {return asic_io_;}
   unsigned char ReadAsicRegister(unsigned short i);
   inline unsigned char Get(unsigned short i) {

      // plus feature ?
      if (asic_io_enabled_ && i >= 0x4000 && i <= 0x7FFF /* ((i & 0xC000 ) == 0x4000)*/)
      {
         return last_value_read_ = ReadAsicRegister(i);
      }
      last_address_read_[(index_addr_read_++)] = i;
      index_addr_read_=index_addr_read_%4;

      return last_value_read_ = (*ram_read_[i>>14]) [ i & 0x3FFF];
   };
   inline unsigned short GetWord(unsigned short i)
   {
      if (i!=0x3FFF)
      {
         unsigned char * p= &((*ram_read_[i>>14]) [ i & 0x3FFF]);return p[0]|(p[1]<<8);
      }
      else return ( (Get ( i+1 )<<8)|Get(i)  );
   };


   inline unsigned char GetWrite(unsigned short i)  {return (*ram_write_[i>>14]) [ i & 0x3FFF];};
   inline unsigned short GetWordWrite(unsigned short i) {return ((GetWrite ( i+1 )<<8) | (GetWrite(i)));};

   void WriteAsicRegister(unsigned short addr, unsigned char data);
   void UpdateAsicPalette(unsigned char color_index, unsigned char hardware_color );

   inline void Set ( unsigned short addr, unsigned char data ) {
      last_value_read_ = data;
      if (asic_io_enabled_ && addr >= 0x4000 && addr <= 0x7FFF /* (addr & 0xC000) == 0x4000)*/)
      {
         WriteAsicRegister(addr, data);
         return;
      }

      last_address_write_[(index_addr_write_++)&0x3] = addr;
      if ( !ram_dis_ || !expansion_ || expansion_->CanWrite (addr))
         (*ram_write_[addr>>14])[addr & 0x3FFF] = data;
   };
   inline void SetWord ( unsigned short addr, unsigned short data )
   {   Set( addr, data&0xff);Set( addr+1, data>>8);};


   // Debug methods :
   inline unsigned char GetDbg(unsigned short i, unsigned int ram_bank)  {return (*ram_read_[ram_bank]) [ i & 0x3FFF];};
   inline unsigned short GetWordDbg(unsigned short i, unsigned int ram_bank) {return ((GetDbg ( i+1 , ram_bank)<<8) + (GetDbg(i, ram_bank)));};
   inline unsigned char GetWriteDbg(unsigned short i, unsigned int ram_bank)  {return (*ram_write_[ram_bank]) [ i & 0x3FFF];};
   inline unsigned short GetWWriteDbg(unsigned short i, unsigned int ram_bank) {return ((GetWriteDbg ( i+1 , ram_bank)<<8) + (GetWriteDbg(i, ram_bank)));};
   inline void SetDbg ( unsigned short addr, unsigned char data , unsigned int ram_bank) {(*ram_write_[ram_bank])[addr & 0x3FFF] = data;};
   inline void SetWordDbg ( unsigned short addr, unsigned short data , unsigned int ram_bank){   SetDbg( addr, data&0xff, ram_bank);SetDbg( addr+1, data>>8, ram_bank);};
   inline unsigned char *GetRamRead(unsigned int ram_bank) { return *ram_read_[ram_bank]; }
   //
   inline unsigned char* GetRomBuffer() {return rom_[0];};
   inline unsigned char* GetCartridge(int index) {
      cart_available_[index] = true; return cartridge_[index];
};
   void EjectCartridge() {
      memset (cartridge_, 0, sizeof(cartridge_)); memset(cart_available_, 0, sizeof(cart_available_)); SetMemoryMap();
   }

   inline unsigned char* GetRomBuffer(int rom_index) { return rom_[rom_index]; };
   inline unsigned char* GetRamBuffer() {return ram_buffer_[0];};

   bool* GetAvailableCartridgeSlot() { return cart_available_; }
   bool* GetAvailableROM() {
      return rom_available_;
   }
   bool IsLowerRomLoaded() {
      return lower_rom_available_;
   }
   bool * GetAvailableRam() {
      return extended_ram_available_;
   }

   typedef enum
   {
      MEM_READ,
      MEM_WRITE,
      MEM_RAM_LOWER_BANK,
      MEM_RAM_BANK,
      MEM_LOWER_ROM,
      MEM_ROM_BANK,
      MEM_CART_SLOT
   } DbgMemAccess   ;

   unsigned int GetDebugValue(unsigned char * address_buffer, unsigned short adress_start, unsigned int size_of_buffer, DbgMemAccess acces, unsigned int data = 0);
   unsigned short GetDebugMaxAdress(DbgMemAccess acces);

   // RAM
   RamBank rom_[256];
   RamBank ram_buffer_[4];

   RamBank cartridge_[32]; // Plugged cartridge
   bool cart_available_[32];
   bool lower_rom_available_;

   void SetInfROMConnected (bool set );
   void SetSupROMConnected (bool set );
   void SetRmr2(unsigned char rmr2) {
      rmr2_ = rmr2; SetMemoryMap();
      asic_io_enabled_ = ((rmr2_ & 0x18) == 0x18);
   }

   void SetRam ( unsigned char ram_available );

   void SetLogicalROM ( unsigned char num );
   void ConnectBank  (unsigned char page, unsigned s, unsigned char bank);

   void SetMemoryMap();

   unsigned short last_address_read_[4];
   unsigned short last_address_write_[4];
   void ResetStockAddress (){index_addr_write_ = index_addr_read_ = 0;memset ( last_address_read_, 0, sizeof last_address_read_);memset ( last_address_write_, 0, sizeof last_address_write_);};
   unsigned int index_addr_read_;
   unsigned int index_addr_write_;

   // Asic registers
   unsigned char GetDCSR() { return asic_io_[0x2C0F]; }
   void SetDCSR(unsigned char dcsr) { asic_io_[0x2C0F] = dcsr; }
   unsigned char GetPRI() {return asic_io_[0x2800];}
   unsigned char GetIVR() { return asic_io_[0x2805]; }
   unsigned char GetSPLT() { return asic_io_[0x2801]; }

   unsigned char GetSSCR() { return asic_io_[0x2804]; }
   unsigned char * GetSprite(int i) { return &asic_io_[0x0000 + i * 0x100]; }

   struct TSpriteInfo
   {
      bool displayed;
      short x;
      short y;
      int zoomx;
      int zoomy;
      int sizex;
      int sizey;
   };

   TSpriteInfo* GetSpriteInfo(int i) { return &sprite_info_[i]; }

   unsigned char * GetSpritePalette() { return &asic_io_[0x2422]; }

   // DMA
   unsigned char* GetDMARegister(int channel) { return &asic_io_[0x2C00+channel*4]; }
   unsigned short GetDMAAdress(int channel) { return (asic_io_[0x2C00+channel*4] & 0xFFFE) + (asic_io_[0x2C01 + channel * 4]<<8); }
   unsigned short GetDMAPrescaler(int channel) { return (asic_io_[0x2C02 + channel * 4] ) ; }

   virtual void DMAStop(int channel);

   unsigned char GetLastValueRead() { return last_value_read_; };

   IExpansion* expansion_;

protected:
   // Memoires disponibles :
   bool ram_dis_ ;
   bool rom_dis_ ;

   unsigned char connected_bank_;
   bool LoadROM(unsigned char* rom, unsigned char* rom_from, unsigned int size);
   bool inf_rom_connected_;
   bool sup_rom_connected_;

   // RAM
   RamBank* ram_read_ [4];
   RamBank* ram_write_ [4];

   RamBank asic_io_;
   unsigned char asic_mask_w_[0x4000];
   unsigned char asic_io_rw_[0x4000];



   // Extension
   bool extended_pal_;
   bool extended_ram_available_ [8];
   RamBank extended_ram_buffer_[8][4];
   // ROMs
   unsigned char rom_number_;
   RamBank lower_rom_;
   bool rom_available_[256];
   bool plus_;
   bool asic_io_enabled_;          // true if ASIC io is paged
   TSpriteInfo sprite_info_[16];
   unsigned char rmr2_;
   unsigned char last_value_read_;  // Last value read (ofr unmapped register)
   Monitor* monitor_;
};

