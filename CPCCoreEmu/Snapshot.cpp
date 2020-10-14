#include "stdafx.h"
#include "Snapshot.h"
#include "Motherboard.h"
#include "simple_stdio.h"

extern unsigned int ListeColorsIndex[0x100];
extern unsigned int ListeColorsIndexConvert[32];
extern unsigned int ListeColors[0x100];

CSnapshot::CSnapshot(ILog* log) : log_(log)
{
   machine_ = NULL;
   notifier_ = NULL;
   replay_buffer_ = NULL;
   replay_ = false;
   start_replay_ = false;
   start_record_ = false;
   record_ = false;

   record_buffer_size_ = record_buffer_offset_ = 0;
   record_buffer_ = NULL;
}


CSnapshot::~CSnapshot(void)
{
   delete []record_buffer_;
   delete []replay_buffer_;
}

// Helper function
void CSnapshot::CheckBufferSize ( int nSizeToAdd )
{
   if ( record_buffer_offset_ + nSizeToAdd >= record_buffer_size_ )
   {
      unsigned char *new_buff = new unsigned char [record_buffer_size_*2];
      memcpy (new_buff , record_buffer_, record_buffer_offset_);
      delete []record_buffer_;
      record_buffer_ = new_buff;
      record_buffer_size_ *= 2;
   }
}



void CSnapshot::Playback ()
{
   if (machine_ == NULL) return;

   // One more frame !
   if (start_replay_)
   {
      InitReplay ();
   }
   else if (replay_)
   {
      current_frame_to_wait_--;
      if (current_frame_to_wait_ == 0)
      {
         // read BLS
         unsigned char BLS = replay_buffer_[replay_offset_++];

         // Sync ?
         if (( BLS & 0x80) == 0x80)
         {
            // Sync : Read 4 bytes
            lsb_pc_ = replay_buffer_[replay_offset_++];          // Z80 PC lower byte
            crtc_sync_v_count_ = replay_buffer_[replay_offset_++]; // VLC
            crtc_sync_h_count_ = replay_buffer_[replay_offset_++]; // HCC
            lsb_total_cycles_ = replay_buffer_[replay_offset_++]; //
            // Compare this !

         }

         // Nb key changed :
         int nbKeychanged = (BLS & 0x7F);
         if (nbKeychanged != 0x7F)
         {
            last_keystroke_size_ = 0;
            for (int i = 0; i < nbKeychanged; i++)
            {
               // Next key
               last_keystroke_[last_keystroke_size_++] = replay_buffer_[replay_offset_++];
            }
         }

         // Replay Lastkeystroke
         for (int i = 0; i < last_keystroke_size_; i++)
         {
            // Update key
            /*   if ((keyBytes[(keynum / 8)] & (1 << (keynum & 0x07))) == 0) {
                  keyBytes[(keynum / 8)] |= (1 << (keynum & 0x07));
               } else {
                  keyBytes[(keynum / 8)] &= ~(1 << (keynum & 0x07));
               }*/

            unsigned char c = last_keystroke_[i];
            unsigned char x = (1<< (c & 0x07));
            key_buffer_ [c/8] ^= x;

            //if ( m_KeyBuffer [c/8] & (1<< (c & 0x07))0)

            // RTC : TODO
            // Mouse : TODO
         }
         machine_->GetKeyboardHandler()->ForceKeyboardState ( key_buffer_ );

         // Start new frame count
         if (replay_offset_ < replay_size_)
         {
            current_frame_to_wait_ = replay_buffer_[replay_offset_++];
            if ( current_frame_to_wait_ == 0)
            {
               current_frame_to_wait_ = (replay_buffer_[replay_offset_+1]<<8) + replay_buffer_[replay_offset_];
               replay_offset_ += 2;
            }
         }
         else
         {
            replay_ = false;
         }
      }
   }
   else if (start_record_)
   {
       InitRecord ();
   }
   else if ( record_ )
   {

      current_frame_to_wait_++;
      // Record the current key state
      
      // Compare with the one already in cache
      last_keystroke_size_ = 0;
      for (int i = 0; i < 10; i++)
      {
         unsigned char key_line = machine_->GetKeyboardHandler()->GetKeyboardMap(i);
         if (key_line != key_buffer_[i] )
         {
            // Compute the key pressed
            unsigned char key = key_line ^ key_buffer_[i];
            for (int j = 0; j < 8; j++)
            {
               if ( key & 0x1 )
               {
                  unsigned char c = i * 8;
                  c |= j;
                  last_keystroke_[last_keystroke_size_++] = c;
               }
               key >>= 1;
            }
            machine_->GetKeyboardHandler();
         }
      }
      // Same ? Just increase frame count
      if ( last_keystroke_size_ != 0)
      /*{
         //m_CurrentFrameToWait++;
      }
      else*/
      {
         // Otherwise, record a new entry

         // Nb frame elapsed
         if ( current_frame_to_wait_ < 0x100)
         {
            CheckBufferSize ();
            record_buffer_[record_buffer_offset_++] = (unsigned char)(current_frame_to_wait_&0xFF);
         }
         else
         {
            CheckBufferSize (3);
            record_buffer_[record_buffer_offset_++] = 0;
            record_buffer_[record_buffer_offset_++] = (unsigned char)(current_frame_to_wait_&0xFF);
            record_buffer_[record_buffer_offset_++] = (unsigned char)((current_frame_to_wait_>>8)&0xFF);
         }

         // BRS : m_LastKeystrokeSize
         CheckBufferSize ();
         record_buffer_[record_buffer_offset_++] = last_keystroke_size_;

         // Keystroke recorded
         CheckBufferSize (last_keystroke_size_);

         for (int i = 0; i < last_keystroke_size_; i++)
         {
            record_buffer_[record_buffer_offset_++] = last_keystroke_[i];
         }

         // Reset everything
         machine_->GetKeyboardHandler()->ForceKeyboardState(key_buffer_);
         //memcpy ( key_buffer_, current_state, 10);
         current_frame_to_wait_ = 0;
      }
   }

}


void CSnapshot::LoadStdSna ( unsigned char * header, FILE* f)
{
   if (machine_ == NULL) return;

   // Type of snapshot :
   unsigned int snaType = header[0x10];

   // read Version 1
   // Z80 Registers
   machine_->GetProc()->af_.b.l = header[0x11];
   //machine_->GetProc()->af_.b.l = header[0x11];
   machine_->GetProc()->af_.b.h = header[0x12];
   //machine_->GetProc()->af_.b.h = header[0x12];
   machine_->GetProc()->bc_.b.l = header[0x13];
   ////machine_->GetProc()->bc_.b.l = header[0x13];
   machine_->GetProc()->bc_.b.h = header[0x14];
   //machine_->GetProc()->bc_.b.h = header[0x14];
   machine_->GetProc()->de_.b.l = header[0x15];
   //machine_->GetProc()->de_.b.l = header[0x15];
   machine_->GetProc()->de_.b.h = header[0x16];
   //machine_->GetProc()->de_.b.h = header[0x16];
   machine_->GetProc()->hl_.b.l = header[0x17];
   //machine_->GetProc()->hl_.b.l = header[0x17];
   machine_->GetProc()->hl_.b.h = header[0x18];
   //machine_->GetProc()->hl_.b.h = header[0x18];
   machine_->GetProc()->ir_.b.l = header[0x19];
   //machine_->GetProc()->ir_.b.l = header[0x19];
   machine_->GetProc()->ir_.b.h = header[0x1A];
   //machine_->GetProc()->ir_.b.h = header[0x1A];

   machine_->GetProc()->iff1_ = (header[0x1B]==1);
   //machine_->GetProc()->iff1_ = (header[0x1B] == 1);
   machine_->GetProc()->iff2_ = (header[0x1C]==1);
   //machine_->GetProc()->iff2_ = (header[0x1C] == 1);

   //m_pMachine->GetProc()->m_InterruptAuthorized = (m_pMachine->GetProc()->iff1_ == true);

   machine_->GetProc()->ix_.w  = (header[0x1D]| ((header[0x1E])<<8));
   machine_->GetProc()->iy_.w = (header[0x1F]|((header[0x20])<<8));
   machine_->GetProc()->sp_ = (header[0x21]|((header[0x22])<<8));
   machine_->GetProc()->pc_ = (header[0x23]|((header[0x24])<<8));
   machine_->GetProc()->interrupt_mode_ = ((header[0x25]));

   //machine_->GetProc()->ix_.w = (header[0x1D] | ((header[0x1E]) << 8));
   //machine_->GetProc()->iy_.w = (header[0x1F] | ((header[0x20]) << 8));
   //machine_->GetProc()->sp_ = (header[0x21] | ((header[0x22]) << 8));
   //machine_->GetProc()->pc_ = (header[0x23] | ((header[0x24]) << 8));
   //machine_->GetProc()->interrupt_mode_ = ((header[0x25]));

   machine_->GetProc()->af_p_.b.l = header[0x26];
   machine_->GetProc()->af_p_.b.h = header[0x27];
   machine_->GetProc()->bc_p_.b.l = header[0x28];
   machine_->GetProc()->bc_p_.b.h = header[0x29];
   machine_->GetProc()->de_p_.b.l = header[0x2A];
   machine_->GetProc()->de_p_.b.h = header[0x2B];
   machine_->GetProc()->hl_p_.b.l = header[0x2C];
   machine_->GetProc()->hl_p_.b.h = header[0x2D];

   //machine_->GetProc()->af_p_.b.l = header[0x26];
   //machine_->GetProc()->af_p_.b.h = header[0x27];
   //machine_->GetProc()->bc_p_.b.l = header[0x28];
   //machine_->GetProc()->bc_p_.b.h = header[0x29];
   //machine_->GetProc()->de_p_.b.l = header[0x2A];
   //machine_->GetProc()->de_p_.b.h = header[0x2B];
   //machine_->GetProc()->hl_p_.b.l = header[0x2C];
   //machine_->GetProc()->hl_p_.b.h = header[0x2D];

   // GA
   machine_->GetVGA()->pen_r_ = header[0x2E] & 0x1F;
   for (int i =0; i < 0x10; i++)
   {
      // TODO : check this !!
      machine_->GetVGA()->ink_list_[i] = ListeColorsIndex[(header[0x2F+i] & 0x1F)|0x40];
      machine_->GetMem()->UpdateAsicPalette(i, (header[0x2F+i] & 0x1F));

      //m_pMachine->GetVGA()->m_InkList[i] = ListeColorsIndex[ListeColorsIndexConvert[header[0x2F+i] & 0x1F]];
      //m_pMachine->GetVGA()->m_InkList[i] = ListeColorsIndex[ListeColorsIndexConvert[header[0x2F+i]]];
      //m_pMachine->GetVGA()->m_InkList[i] = ListeColorsIndex[header[0x2F+i]];
      //m_pMachine->GetVGA()->m_InkList[i] = ListeColors[header[0x2F+i]&0x1F];
   }
   //m_pMachine->GetVGA()->m_Border = ListeColorsIndex[/*ListeColorsIndexConvert[*/header[0x3F/*]*/]];
   //m_pMachine->GetVGA()->m_Border = ListeColorsIndex[header[0x3F] & 0xFF];
   //m_pMachine->GetVGA()->m_Border = ListeColorsIndex[(header[0x3F] & 0x1F)|0x40];
   for (int i = 0; i < NB_BYTE_BORDER; i++)machine_->GetVGA()->video_border_[i] = ListeColorsIndex[(header[0x3F] & 0x1F) | 0x40];
   machine_->GetMem()->UpdateAsicPalette(0x10, (header[0x3F] & 0x1F));

   //m_pMachine->GetMonitor()->SetBorder ( ListeColorsIndex[(header[0x3F] & 0x1F)|0x40] );
   //m_pMachine->GetVGA()->m_Border = ListeColors[header[0x3F]&0x1F];

   unsigned char multiconfg = header[0x40];
   machine_->GetVGA()->buffered_screen_mode_ = multiconfg & 0x3;

   machine_->GetMem()->SetInfROMConnected ( (multiconfg & 0x4)?false:true);
   machine_->GetMem()->SetSupROMConnected ( (multiconfg & 0x8)?false:true);

   // RAM Conf
   unsigned char ramConf = header[0x41];
   // Decode 64k page
   unsigned char p = ramConf&0x38;
   p = p>>3;
   unsigned char b = ramConf & 0x3;
   unsigned char s = ramConf & 0x4;
   s = s >>2;
   machine_->GetMem()->ConnectBank ( p, s, b);

   // CRTC
   machine_->GetCRTC()->adddress_register_ = header[0x42];

   for (int i = 0; i < 18; i++)
   {
      machine_->GetCRTC()->registers_list_[i] = header[0x43+i];
   }
   machine_->GetCRTC()->horizontal_sync_width_ = (machine_->GetCRTC()->registers_list_ [3] & 0x0F);

   // ROM Selection
   machine_->GetMem()->SetLogicalROM ( header[0x55] );

   // PPI
   machine_->GetPPI()->port_a_ = header[0x56];
   machine_->GetPPI()->port_b_ = header[0x57] & 0xFE; // Remove VBL status from snapshot
   // Force 50hz/Amstrad - TODO : Use config instead
   machine_->GetPPI()->port_b_ |= 0x1E;

   machine_->GetSig()->v_sync_ = header[0x57]&0x1;

   machine_->GetPPI()->port_c_ = header[0x58];

   // PPI Control register
   unsigned char data = header[0x59];

   machine_->GetPPI()->control_word_.byte = data;
   // Mode : bit 5&6
   /*if (( data & 0x60) == 0x00 )
   {
      // Mode 0
      m_pMachine->GetPPI()->m_ModeA = 0;
   }
   else if (( data & 0x60) == 0x20 )
   {
      // Mode 1
      m_pMachine->GetPPI()->m_ModeA = 1;
   }
   else
   {
      // Mode 2
      m_pMachine->GetPPI()->m_ModeA = 2;
   }
   // Port A : Entree ou Sortie
   m_pMachine->GetPPI()->m_EntreeA = (( data & 0x10) == 0x10 );

   // Port B
   m_pMachine->GetPPI()->m_EntreeB = (( data & 0x02) == 0x02 );
   m_pMachine->GetPPI()->m_ModeB = (( data & 0x04) == 0x04 );

   // Port C
   m_pMachine->GetPPI()->m_EntreeCLow = (( data & 0x01) == 0x01 );
   m_pMachine->GetPPI()->m_EntreeCHigh = (( data & 0x08) == 0x08 );
   */
   // PSG
   machine_->GetPSG()->register_address_ = header[0x5A];

   for (int i = 0; i < 16; i++)
   {
      unsigned char data = i;
      machine_->GetPSG()->Access(&data, 3, 0);
      machine_->GetPSG()->Access( &header[0x5B+i], 2, 0);
   }

   // Patch with Version 2 if necessary
   if (snaType == 1)
   {
      // CPC Type : To do ?
      /*switch (header[0x6D])
      {
         
      case 0:machine_->SetMachineType(0); break;  // 464
      case 1:machine_->SetMachineType(1); break;  // 664
      case 2:machine_->SetMachineType(2); break;  // 6128
      
      }*/
      machine_->SetPlus(false);

      // Other values are unused
   }
   else if (snaType > 2)
   {
      // CPC Type : To do ?
      switch (header[0x6D])
      {
      case 0:/*machine_->SetMachineType(0); */machine_->SetPlus(false); break;  // 464
      case 1:/*machine_->SetMachineType(1); */machine_->SetPlus(false); break;  // 664
      case 2:/*machine_->SetMachineType(2); */machine_->SetPlus(false); break;  // 6128
      case 4:/*machine_->SetMachineType(4); */machine_->SetPlus(true); break;  // 6128+
      case 5:/*machine_->SetMachineType(3); */machine_->SetPlus(true); break;  // 464+
      case 6:/*machine_->SetMachineType(5); */machine_->SetPlus(true); break;  // GX400
      }


      // MemEnable - Bit 7 set if used. Bit 0 = Bank C4..C7, Bit 1 = Banks C4..DF
      // Bit 2 = Banks E4..FF, Bit 3 = Full 4M expansion
      // TODO
      header[0x9A] ;

      //   9C	 1	 FDD motor drive state (0=off, 1=on)
      machine_->GetFDC()->SetMotor( (header[0x9c] == 1) );

      //   9D-A0	 1	 FDD current physical track (note 15)
      machine_->GetFDC()->SetCurrentTrack ( 0,header[0x9D]);
      machine_->GetFDC()->SetCurrentTrack ( 1,header[0x9E]);
      //m_pMachine->GetFDC()->SetCurrentTrack ( 2,header[0x9F]);
      //m_pMachine->GetFDC()->SetCurrentTrack ( 3,header[0xA0]);
      // TODO : FDD 1-3

      //   A1	 1	 Printer Data/Strobe Register (note 1)
      // TODO

      // Monitor line short
      machine_->GetMonitor()->y_ = header[0xA2] + (header[0xA3] <<8);

      //   A4	 1	CRTC type:
      //   CRTC type:
      //   0 = HD6845S/UM6845
      //   1 = UM6845R
      //   2 = MC6845
      //   3 = 6845 in CPC+ ASIC
      //   4 = 6845 in Pre-ASIC
      switch (header[0xA4])
      {
      case 0 : machine_->GetCRTC()->DefinirTypeCRTC(CRTC::HD6845S);break;
         case 1 : machine_->GetCRTC()->DefinirTypeCRTC(CRTC::UM6845R);break;
         case 2 : machine_->GetCRTC()->DefinirTypeCRTC(CRTC::MC6845);break;
         case 3 : machine_->GetCRTC()->DefinirTypeCRTC(CRTC::AMS40489);break;
         case 4 : machine_->GetCRTC()->DefinirTypeCRTC(CRTC::AMS40226);break;
         default: machine_->GetCRTC()->DefinirTypeCRTC(CRTC::UM6845R);break; // defautl is 1
      }

      //   A9	 1	 CRTC horizontal character counter register (note 11)
      machine_->GetCRTC()->hcc_ = header[0xA9];
      //   AA	 1	 unused (0)
      //   AB	 1	 CRTC character-line counter register (note 2)
      machine_->GetCRTC()->vcc_ = header[0xAB];
      //   AC	 1	 CRTC raster-line counter register (note 3)
      machine_->GetCRTC()->vlc_ = header[0xAC];
      //   AD	 1	 CRTC vertical total adjust counter register (note 4)
      machine_->GetCRTC()->vertical_adjust_counter_= header[0xAD];
      //   AE	 1	 CRTC horizontal sync width counter (note 5)
      machine_->GetCRTC()->horinzontal_pulse_ = header[0xAE];
      //   AF	 1	 CRTC vertical sync width counter (note 6)
      machine_->GetCRTC()->scanline_vbl_ = header[0xAF];

      //   B0-B1	 2
      //   CRTC state flags. (note 7)
      //   Bit	 Function
      //   0	 if "1" VSYNC is active, if "0" VSYNC is inactive (note 8)
      machine_->GetCRTC()->ff4_ = ((header[0xB0]&0x01)==0x01);
      //   1	 if "1" HSYNC is active, if "0" HSYNC is inactive (note 9)
      machine_->GetSig()->h_sync_ = ((header[0xB0]&0x02)==0x02);
      //   2-7	 reserved
      //   7	 if "1" Vertical Total Adjust is active, if "0" Vertical Total Adjust is inactive (note 10)
      machine_->GetCRTC()->r4_reached_ = ((header[0xB0]&0x80)==0x80);

      //   8-15	 Reserved (0)
      //   B2	 1	 GA vsync delay counter (note 14)
      machine_->GetVGA()->wait_for_hsync_ = header[0xB2];
      if (machine_->GetVGA()->wait_for_hsync_ > 0)
      {
         machine_->GetVGA()->wait_for_hsync_ = 3-machine_->GetVGA()->wait_for_hsync_;
      }
      else
      {
         if ( machine_->GetCRTC()->ff4_)
            machine_->GetVGA()->v_old_sync_ = false;
      }
      //   B3	 1	 GA interrupt scanline counter (note 12)*
      machine_->GetVGA()->interrupt_counter_ = header[0xB3] - 1; // TODO Check
      //   B4	 1	 interrupt request flag (0=no interrupt requested, 1=interrupt requested) (note 13)
      machine_->GetVGA()->sig_handler_->int_ = (header[0xB4] == 1);
      machine_->GetVGA()->interrupt_raised_ = (header[0xB4] == 1);

      //   B5-FF	 75	 unused (0)

   }

   switch (machine_->GetCRTC()->type_crtc_)
   {
   case 0: //
      machine_->GetCRTC()->vertical_sync_width_ = ((machine_->GetCRTC()->registers_list_ [3]&0x80 )== 0x80)?8:16;
      if (machine_->GetCRTC()->vertical_sync_width_ == 0)machine_->GetCRTC()->vertical_sync_width_ = 16;
      break;
   case 1:
   case 2:
      machine_->GetCRTC()->vertical_sync_width_ = 16;
      break;
   case 3:
   case 4:
      machine_->GetCRTC()->vertical_sync_width_ = machine_->GetCRTC()->registers_list_ [3] >> 4;
      break;
   }

   // Memory

   // Memory dump size

   bool bUpRAM = (header[0x6B] == 128);
   bool bLowRAM = (header[0x6B] >= 64);

   // Memory dump
   // Lower RAM
   if (bLowRAM)
	   {
	   //if (!feof(f))
	   {
		  int count = 0;
		  for (int i = 0; i < 4; i++)
		  {
			 count = fread ( machine_->GetMem()->ram_buffer_[i], 1, 0x4000, f);
		  }
	   }
   }
   // Upper RAM
   if (bUpRAM)
   {
      //if (!feof(f))
      {
         for (int i = 0; i < 4; i++)
         {
            fread ( machine_->GetMem()->extended_ram_buffer_[0][i], 1, 0x4000, f);
         }
      }
         //fread_s( &m_pMachine->GetMem()->ExtendedRamBuffer[0][0] , sizeof(CMemoire::tRamBank) * 4, sizeof(CMemoire::tRamBank) * 4, 1, f);
   }
}

void CSnapshot::HandleChunkBRKC(unsigned char* chunk, unsigned char* in_buffer, int size)
{
   // ACE's breakpoints
   int nb_breakpoints = size / 216;

   for (int i = 0; i < nb_breakpoints; i++)
   {
      unsigned char * buffer = &in_buffer[i * 216];
      // Add new breakpoint
      switch (buffer[0])
      {
      case 0:
      {
         // Exec breakpoint
         //BreakpointPC * break_pc = new BreakpointPC(machine_, (buffer[04] << 8) | buffer[5]);
         machine_->AddBreakpoint((buffer[04] << 8) | buffer[5]);
         break;
      }
      case 1:
         // Memory breakpoint
         break;
      case 2:
         // IO breakpoint
         break;
      default:
         // Not supported !
         break;
      }
   }
}

void CSnapshot::HandleChunkBRKS(unsigned char* chunk, unsigned char* in_buffer, int size)
{
   int nb_breakpoints = size / 5;
  
   for (int i = 0; i < nb_breakpoints; i++)
   {
      unsigned char * buffer = &in_buffer[i * 5];
      // base, or extended ram ?
      // Currently not handled by sbx...
      if (buffer[2] == 1)
      {
         // extended ram

      }
      // condition
      machine_->AddBreakpoint((buffer[01] << 8) | buffer[0]);

   }
}

void CSnapshot::HandleChunkSYMB(unsigned char* chunk, unsigned char* in_buffer, int size)
{
   // ACE's Symbols

   // 1 octet -> taille du symbole (0 est une valeur invalide)
   // n octets->le nom du symbole(sans 0 à la fin puisqu'on connait la taille)
   // 6 octets->réservé(ça sera utilisé plus tard pour des symbols contextuels)
   // 2 octets->l'adresse du symbole (en big endian)
}

void CSnapshot::HandleChunkROMS ( unsigned char* chunk, unsigned char* in_buffer, int size )
{
   int off = 0;
   // First string... Or first two bytes ?
   // Have to find out... => Language ??

   off += 2;

   // Read lower rom
   char rom_name [128];
   strcpy ( rom_name, (char*)&in_buffer[off] );
   off += strlen (rom_name);
   off ++;
   // Load it
   // todo
//   m_pMachine->GetMem()->LoadLogicalROM (RomName, m_pMachine->GetSettings()->ROMPath ().c_str(), true, 0);


   // Read upper roms
   for (int i = 0; i < 8; i++)
   {
      // Read next ROM
      strcpy ( rom_name, (char*)&in_buffer[off] );
      off += strlen (rom_name);
      off ++;
      // todo
      //m_pMachine->GetMem()->LoadLogicalROM (RomName, m_pMachine->GetSettings()->ROMPath ().c_str(), false, i);
   }

}

void CSnapshot::HandleChunkCPCPLUS(unsigned char* chunk_header, unsigned char* in_buffer, int size)
{
   if (size != 0x8F8) return; // Wrong size !

   unsigned char * asic_registers = machine_->GetMem()->GetAsicRegisters();

   // 000 - 7FF	800	4000 - 4FFF	Sprite Bitmaps(note 1)
   for (int i = 0; i < 0x800; i++)
   {
      asic_registers[i * 2] = ((in_buffer[i]>>4)&0xF);
      asic_registers[i * 2 + 1] = (in_buffer[i] & 0xF);
   }

   // 800 - 87F	8 * 16	6000 - 607F	Sprite Attributes(see below) (note 2)
   for (int i = 0; i < 0x80; i++)
   {
      asic_registers[0x2000 + i] = in_buffer[0x800 + i];
   }

   // 880 - 8BF	32 * 2	6400 - 643F	Palettes(note 3)
   for (int i = 0; i < 32; i++)
   {
      asic_registers[0x2400 + i*2] = in_buffer[0x880 + i*2];
      asic_registers[0x2400 + i * 2+1] = in_buffer[0x880 + i * 2+1];

      unsigned char r = (asic_registers[(0x2400+i*2) & 0xFFFE] >> 4);
      unsigned char b = asic_registers[(0x2400 + i*2) & 0xFFFE] & 0x0F;
      unsigned char g = asic_registers[((0x2400 + i*2) & 0xFFFE) + 1] & 0xF;
      r <<= 4;
      g <<= 4;
      b <<= 4;
      if (i < 16) // Only first 16 are on the palette list
      {
         machine_->GetVGA()->ink_list_[i] = (r << 16) + (g << 8) + (b);
      }
      else if (i==16)
      {
         for (int j = 0; j < NB_BYTE_BORDER; j++)
            machine_->GetVGA()->video_border_[j] = (r << 16) + (g << 8) + (b);
      }
   }
   machine_->GetMonitor()->RecomputeAllColors();
   // 8C0	1	6800	Programmable Raster Interrupt(note 4)
   // 8C1	1	6801	Screen split scan - line(note 4)
   // 8C2	2	6802 - 6803	Screen split secondary screen - address(note 4)
   // 8C4	1	6804	Soft scroll control register (note 4)
   // 8C5	1	6805	Interrupt vector(note 4)
   for (int i = 0; i < 6; i++)
   {
      asic_registers[0x2800 + i] = in_buffer[0x8C0 + i];
   }

   // 8C6	1	Internal	gate array A0 register value(note 8a)
   // todo : ???

   // 8C6 - 8C7	1 - unused(0)
   // 8C8 - 8CF	8	6808 - 680f	Analogue input channels 0 - 7 (note 5)
   for (int i = 0; i < 8; i++)
   {
      asic_registers[0x2808 + i] = in_buffer[0x8C8 + i];
   }
   // 8D0 - 8DB	3 * 4	6C00 - 6C0B	Sound DMA channel attributes 0 - 2 (see below) (note 6)
   for (int i = 0; i < 12; i++)
   {
      asic_registers[0x2C00+ i] = in_buffer[0x8D0 + i];
   }
   // 8DC - 8DE	3 - unused(0)
   // 8DF	1	6C0F	DMA Control / Status(note 4)
   asic_registers[0x2C0F] = in_buffer[0x8DF];

   // 8E0 - 8F4	3 * 7	Internal	DMA channel 0 - 2 internal registers(see below) (note 7)
   for (int dma_channel = 0; dma_channel < 3; dma_channel++)
   {
      DMA* dma = machine_->GetDMA(dma_channel);
      dma->repeat_counter_ = in_buffer[0x8E0 + dma_channel * 7] + (in_buffer[0x8E0 + dma_channel * 7 + 1]<<8);
      dma->repeat_addr_ = in_buffer[0x8E2 + dma_channel * 7] + (in_buffer[0x8E2 + dma_channel * 7 + 1] << 8);
      dma->pause_counter_ = in_buffer[0x8E4 + dma_channel * 7] + (in_buffer[0x8E4 + dma_channel * 7 + 1] << 8);

      // pause prescalar : todo !
   }

   // 8F5	1	Internal	gate array A0 register value(note 8b)
   machine_->GetMem()->rmr2_ = in_buffer[0x8F5];

   // 8F6	1	Internal	gate array A0 lock : 0->locked, != 0->unlocked(note 9)
   machine_->GetVGA()->Unlock ( (in_buffer[0x8F6]!=0) );

   // 8F7	1	Internal	ASIC unlock sequence state(note 10)
   machine_->GetAsic()->SetIndexVerification(in_buffer[0x8F7]);
}

void CSnapshot::HandleChunkMem ( unsigned char* chunk, unsigned char* in_buffer, int size )
{
   // Set memory conf.
   // Depends on MEM0-MEM8 !
   unsigned char* dest_memory;
   switch ( (char)chunk[3] )
   {
   case '0':
      dest_memory = machine_->GetMem()->ram_buffer_[0];
      break;
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
      dest_memory = machine_->GetMem()->extended_ram_buffer_[ chunk[3]-'1' ][0];
      break;
   default:
      return;
   }

   // Load memory
   int offset = 0;
   int off_mem = 0;
   while ( offset < size  && off_mem < 0x10000)
   {
      // Read next byte
      unsigned char c = in_buffer[offset++];
      // Is it
      if ( c == 0xE5)
      {
         // Read length
         unsigned char s = in_buffer[offset++];
         if (s == 0)
         {
            dest_memory[off_mem++] = 0xE5;
         }
         else
         {
            unsigned char b = in_buffer[offset++];
            for (int i = 0; i < s && off_mem < 0x10000; i++)
               dest_memory[off_mem++] = b;
         }
      }
      else
      {
         dest_memory[off_mem++] = c;
      }
   }
}

bool CSnapshot::HandleSnr ( FILE* f )
{
#ifndef __circle__
   // First ten bytes : Should it be the current keyboard state ?
   if ( fread ( key_buffer_, 1, 10, f ) != 10)
      // Wrong !
      return false;

   machine_->GetKeyboardHandler()->ForceKeyboardState ( key_buffer_ );

   // 48 following bytes... Maybe something about RTC or Mouse ?
   unsigned char buffer[0x48];
   if ( fread ( buffer, 1, 0x48, f ) != 0x48)
      // Wrong !
      return false;


   // Then, the keystrokes.
   if ( replay_buffer_ != NULL) delete []replay_buffer_;


   fpos_t current_pos;
   fgetpos ( f, &current_pos  );
   int currentPos = ftell ( f );

   fseek (f, 0, SEEK_END);

   //fpos_t totalSize;
   //fgetpos ( f, &totalSize);

   int totalSize = ftell ( f );

   //unsigned int remainingSize = static_cast<unsigned int >(totalSize - currentPos);
   unsigned int remainingSize = static_cast<unsigned int >(totalSize - currentPos);

   fsetpos(f, &current_pos);

   replay_buffer_ = new unsigned char [remainingSize];

   if ( fread ( replay_buffer_, 1, remainingSize, f ) != remainingSize)
   {
      delete []replay_buffer_;
      replay_buffer_ = NULL;
      return false;
   }
   replay_size_ = remainingSize;
   replay_offset_ = 0;
   replay_ = true;
   last_keystroke_size_ = 0;

   current_frame_to_wait_ = replay_buffer_[replay_offset_++];
   if ( current_frame_to_wait_ == 0)
   {
      current_frame_to_wait_ = (replay_buffer_[replay_offset_]<<8) + replay_buffer_[replay_offset_+1];
      replay_offset_ += 2;
   }

   machine_->ResetCounter();
   // Set monitor to begining of VSync
   //m_pMachine->GetMonitor()->m_Y = 0;
   //m_pMachine->GetMonitor()->m_VerticalState = CMonitor::Sync;
   machine_->Resync ();

   /*int offset = 0;
   while ( offset < remainingSize )
   {
      // Read number of frame elapsed
      unsigned int nbFrameElapsed = m_pReplayBuffer[offset++];
      if ( nbFrameElapsed == 0)
      {
         nbFrameElapsed = m_pReplayBuffer[offset]<<8 + m_pReplayBuffer[offset+1];
         offset += 2;
      }

      // read BLS
      unsigned char BLS = m_pReplayBuffer[offset++];

      // Depending on BLS :
      // Sync bloc ?

      //
   }
   */


   /*
   // Nb frames :
   // 0 => dword, otherwise, byte.

   // Keys :
   // + Clock
   // + Mouse

   */
#endif
   return true;
}


#define BASIC_RECORD_SIZE 256;
void CSnapshot::StartRecord (const char* path_file)
{
	if (replay_ )
	{
		replay_  = false;
		StopPlayback ();
	}
	if ( record_)
	{
		StopRecord ();
		record_  = false;
	}

   // Init the structures
   if ( record_buffer_!= NULL)
   {
      delete []record_buffer_;
      record_buffer_ = NULL;
   }
   record_buffer_offset_ = 0;
   record_buffer_size_ = BASIC_RECORD_SIZE;
   record_buffer_ = new unsigned char [record_buffer_size_];




   if ( fopen_s ( &record_file_, path_file, "wb") != 0)
   {
      start_record_ = false;
      record_ = false;
      record_file_ = NULL;
   }
   else
   {
      start_record_ = true;


   }
}

void CSnapshot::InitRecord ()
{
#ifndef __circle__
   record_ = true;
   start_record_ = false;
   // Write SNA v3
   // Create header
   unsigned char header [0x100] = {0};
   memcpy ( header, "RW - SNR", 8);

   WriteSnapshotV3 ( record_file_, header, 8 );

   // Then some SNRV chunk
   unsigned char snrv_chunk[9] = {0};
   memcpy ( snrv_chunk, "SNRV", 4 );
   snrv_chunk[4] = 1; // Size
   snrv_chunk[8] = 1; // Version

   fwrite  ( snrv_chunk, 9, 1, record_file_);

   // Write SNRV
   fwrite  ( "SNR ", 4, 1, record_file_);

   // Keyboard state
   for (int i = 0; i < 10; i++)
   {
      key_buffer_[i] = machine_->GetKeyboardHandler()->GetKeyboardMap(i);
   }
   /*unsigned char* current_state = machine_->GetKeyboardHandler()->GetKeyboardState ();
   memcpy ( key_buffer_, current_state, 10);
   */
   fwrite ( key_buffer_, 10, 1, record_file_);

   // 0x48 next byte blank
   unsigned char buffer_blank [0x48] = {0};
   fwrite ( buffer_blank, 0x48, 1, record_file_);
   current_frame_to_wait_ = 0;


#endif
}

void CSnapshot::StopRecord ()
{
#ifndef __circle__

   // Write keystrokes
   fwrite ( record_buffer_, record_buffer_offset_, 1, record_file_);
   delete []record_buffer_;
   record_buffer_ = NULL;
   record_buffer_size_ = record_buffer_offset_ = 0;

   // Then, close the file
   fclose (record_file_);

   record_ = false;
#endif
}

bool CSnapshot::LoadSnr (const char* path_file)
{
#ifndef __circle__
   snr_filepath_ = path_file;
   start_replay_ = true;
   FILE * f;
   if ( fopen_s ( &f, snr_filepath_.c_str(), "rb") != 0)
   {
      return false;
   }
   fclose (f);
   return true;
#endif
}

void CSnapshot::InitReplay ()
{
#ifndef __circle__
   //
   start_replay_ = false;
   replay_ = false;

   FILE * f;
   if ( fopen_s ( &f, snr_filepath_.c_str(), "rb") != 0)
   {
      if (notifier_) notifier_->ItemLoaded ( snr_filepath_.c_str(), -1, -1);
      return ;
   }

   // Cheack header
   unsigned char header [0x100] = {0};
   fread (header, 0x100, 1, f);
   if (strncmp( (char*)header, "RW - SNR", 8) != 0)
   {
      fclose(f);
      if (notifier_) notifier_->ItemLoaded ( snr_filepath_.c_str(), -1, -1);
      return ;
   }

   LoadStdSna ( header, f );

   // What's next ?
   unsigned char chunk [8];

   while ( fread ( chunk, 1, 8, f ) == 8)
   {
      // Read chunk, and size
      if (memcmp ( chunk, "SNR ", 4) == 0)
      {
         // Actual SNR data : End with this.
         fseek ( f, -4, SEEK_CUR );
         HandleSnr ( f );

         // Then.... Start to play it !
      }
      else
      {
         int size =  chunk[4] +
                     (chunk[5]<<8)+
                     (chunk[6]<<16)+
                     (chunk[7]<<24);
         unsigned char* buffer = new unsigned char[size];
         if ( fread ( buffer, 1, size, f ) == size)
         {
            // Handle the chunk
            if (memcmp ( chunk, "MEM", 3) == 0)
            {
               // Handle memory
               HandleChunkMem ( chunk, buffer, size );
            }
            else if (memcmp ( chunk, "CPC+", 4) == 0)
            {
               HandleChunkCPCPLUS(chunk, buffer, size);
            }
            else if (memcmp ( chunk, "BRKS", 4) == 0)
            {
               // TODO
            }
            else if (memcmp ( chunk, "DSCA", 4) == 0)
            {
               // TODO
            }
            else if (memcmp ( chunk, "DSCB", 4) == 0)
            {
               // TODO
            }
            else if (memcmp ( chunk, "ROMS", 4) == 0)
            {
               HandleChunkROMS ( chunk, buffer, size );
            }
            else if (memcmp ( chunk, "SNRV", 4) == 0)
            {
               // TODO - ??
            }
            else if (memcmp(chunk, "SYMB", 4) == 0)
            {
               HandleChunkSYMB(chunk, buffer, size);
            }


         }
         delete []buffer;
      }

   }

   // Specific adaptation of timing and emulation :
   machine_->GetProc()->ReinitProc ();
   //machine_->GetProc()->ReinitProc ();

   fclose (f);

   if (notifier_) notifier_->ItemLoaded ( snr_filepath_.c_str(), 0, -1);
#endif
}

bool CSnapshot::LoadSnapshot (const char* path_file)
{
   //
   if (log_)log_->WriteLog("Entering snapshot...");
   FILE * f;
   if ( fopen_s ( &f, path_file, "rb") != 0)
   {
      if (notifier_) notifier_->ItemLoaded ( snr_filepath_.c_str(), -1, -1);
      return false;
   }

   if (log_)log_->WriteLog("Snapshot opened successfully.");
   // Cheack header
   unsigned char header [0x100] = {0};
   fread (header, 0x100, 1, f);
   if (strncmp( (char*)header, "MV - SNA", 8) != 0)
   {
      if (log_)log_->WriteLog("Error : Not a valid file...");
      fclose(f);
      if (notifier_) notifier_->ItemLoaded ( snr_filepath_.c_str(), -1, -1);
      return false;
   }

   // Header Ok, Read values
   LoadStdSna ( header, f );


   /*
   // Memory dump size
   bool bUpRAM = (header[0x6B] == 128);

   // Memory dump
   // Lower RAM
   if (!feof(f))
   {
      int count = 0;
      for (int i = 0; i < 4; i++)
      {
         count = fread_s ( m_pMachine->GetMem()->ram_buffer_[i], 0x4000, 1, 0x4000, f);
         int err = ferror(f);
         if ( err != 0)
         {
            int dbg = 1;
         }
      }
   }
   // Upper RAM
   if (bUpRAM)
   {
      if (!feof(f))
      {
         for (int i = 0; i < 4; i++)
         {
            fread_s ( m_pMachine->GetMem()->ExtendedRamBuffer[0][i], 0x4000, 1, 0x4000, f);
         }
      }
         //fread_s( &m_pMachine->GetMem()->ExtendedRamBuffer[0][0] , sizeof(CMemoire::tRamBank) * 4, sizeof(CMemoire::tRamBank) * 4, 1, f);
   }
   */
   // Chunk
   unsigned char chunk[8];

   //while (!feof(f))
   {
      while (fread (chunk, 8, 1, f ) == 1)
      {
         // Handle the chunk
         unsigned int length = chunk[ 4 ]
                              +(chunk[ 5 ] <<8)
                              +(chunk[ 6 ] <<8)
                              +(chunk[ 7 ] <<8);
         unsigned char* buffer = new unsigned char[length];
         fread ( buffer, length, 1, f );

         // What kind of chunk is it ??
         if (memcmp(chunk, "MEM", 3) == 0)
         {
            // Handle memory
            HandleChunkMem(chunk, buffer, length);
         }
         else if (memcmp (chunk, "CPC+", 4 ) == 0)
         {
            // CPC + Chunk
            HandleChunkCPCPLUS(chunk, buffer, length);
         }
         else if (memcmp(chunk, "BRKS", 4) == 0)
         {

            HandleChunkBRKS(chunk, buffer, length);
         }
         else if (memcmp(chunk, "BRKC", 4) == 0)
         {

            HandleChunkBRKC(chunk, buffer, length);
         }
         else if (memcmp(chunk, "DSCA", 4) == 0)
         {
            // TODO
         }
         else if (memcmp(chunk, "DSCB", 4) == 0)
         {
            // TODO
         }
         else if (memcmp(chunk, "ROMS", 4) == 0)
         {
            HandleChunkROMS(chunk, buffer, length);
         }
         else if (memcmp(chunk, "SNRV", 4) == 0)
         {
            // TODO - ??
         }
         else if (memcmp(chunk, "SYMB", 4) == 0)
         {
            HandleChunkSYMB(chunk, buffer, length);
         }
         delete []buffer;
      }
   }

   machine_->GetMem()->SetMemoryMap();

   // Specific adaptation of timing and emulation :
   machine_->GetProc()->ReinitProc ();
   //machine_->GetProc()->ReinitProc ();

   fclose (f);

   if (notifier_) notifier_->ItemLoaded ( path_file, 0, -1);

   machine_->Resync ();


   return true;
}

bool CSnapshot::SaveSnapshot (const char* path_file)
{
   FILE * f;
   if (log_)log_->WriteLog("Entering snapshot saving...");
   if ( fopen_s ( &f, path_file, "wb") != 0)
   {
      if (log_)log_->WriteLog("ERROR : File is not valid...");
      return false;
   }

   // Create header
   unsigned char header [0x100] = {0};
   memcpy ( header, "MV - SNA", 8);

   WriteSnapshotV3 ( f, header, 8 );

   fclose (f);
   return true;
}

void CSnapshot::WriteSnapshotV3 ( FILE * f, unsigned char * base_header, unsigned int headerSize )
{
   if (log_)log_->WriteLog("Writing SNA v3...");
   char header [0x100] = {0};
   memcpy ( header, base_header, headerSize );

   // Snapshot version (3)

   header[0x10] = 3;

   // Write header
   header[0x11] = machine_->GetProc()->af_.b.l ;
   header[0x12] = machine_->GetProc()->af_.b.h ;
   header[0x13] = machine_->GetProc()->bc_.b.l ;
   header[0x14] = machine_->GetProc()->bc_.b.h ;
   header[0x15] = machine_->GetProc()->de_.b.l ;
   header[0x16] = machine_->GetProc()->de_.b.h ;
   header[0x17] = machine_->GetProc()->hl_.b.l ;
   header[0x18] = machine_->GetProc()->hl_.b.h ;
   header[0x19] = machine_->GetProc()->ir_.b.l ;
   header[0x1A] = machine_->GetProc()->ir_.b.h ;

   header[0x1B] = machine_->GetProc()->iff1_?1:0;
   header[0x1C] = machine_->GetProc()->iff2_?1:0;

   header[0x1D] = machine_->GetProc()->ix_.w & 0xFF;
   header[0x1E] = machine_->GetProc()->ix_.w >>8;

   header[0x1F] = machine_->GetProc()->iy_.w & 0xFF;
   header[0x20] = machine_->GetProc()->iy_.w >>8;

   header[0x21] = machine_->GetProc()->sp_ & 0xFF;
   header[0x22] = machine_->GetProc()->sp_ >>8;

   header[0x23] = machine_->GetProc()->pc_ & 0xFF;
   header[0x24] = machine_->GetProc()->pc_ >>8;
   header[0x25] = machine_->GetProc()->interrupt_mode_;

   header[0x26] = machine_->GetProc()->af_p_.b.l;
   header[0x27] = machine_->GetProc()->af_p_.b.h;
   header[0x28] = machine_->GetProc()->bc_p_.b.l;
   header[0x29] = machine_->GetProc()->bc_p_.b.h;
   header[0x2A] = machine_->GetProc()->de_p_.b.l;
   header[0x2B] = machine_->GetProc()->de_p_.b.h;
   header[0x2C] = machine_->GetProc()->hl_p_.b.l;
   header[0x2D] = machine_->GetProc()->hl_p_.b.h;

   // GA
   header[0x2E] = machine_->GetVGA()->pen_r_;

   for (int i =0; i <= 0x10; i++)
   {
      unsigned int ink = 0;
      if ( i == 0x10)
      {
         ink = machine_->GetVGA()->video_border_[0];
      }
      else
      {
         if ( machine_->GetVGA()->buffered_ink_available_ && i == machine_->GetVGA()->pen_r_)
         {
            ink = machine_->GetVGA()->buffered_ink_;
         }
         else
         {
            ink = machine_->GetVGA()->ink_list_[i];
         }
      }
      // Get proper color
      bool found = false;
      for (auto j =0x40; j < 0x60 && (!found); j++)
      {
         if ( ink == ListeColorsIndex[j] )
         {
            // Recherche de l'index...
            header[0x2F+i] = j-0x40;
            found = true;
         }
      }
   }

   //header[0x3F] = m_pMachine->GetVGA()->m_Border;

   header[0x40] = 0x80 | ((machine_->GetVGA()->buffered_screen_mode_ & 3) | (machine_->GetMem()->inf_rom_connected_?0:0x4) | (machine_->GetMem()->sup_rom_connected_?0:0x8));

   // RAM Conf
   header[0x41] = machine_->GetMem()->connected_bank_;

   // CRTC
   header[0x42] = machine_->GetCRTC()->adddress_register_;

   for (int i = 0; i < 18; i++)
   {
      header[0x43+i] = machine_->GetCRTC()->registers_list_[i];
   }

   // ROM Selection
   header[0x55] = machine_->GetMem()->rom_number_;

   // PPI
   header[0x56] = machine_->GetPPI()->port_a_;
   header[0x57] = machine_->GetPPI()->port_b_ & 0x7E | (machine_->GetSig()->v_sync_?1:0) ;
   header[0x58] = machine_->GetPPI()->port_c_;

   // PPI Control register
   unsigned char data = 0x80;
   data |= machine_->GetPPI()->control_word_.byte;
   
   header[0x59] = data;

   // PSG
   header[0x5A] = machine_->GetPSG()->register_address_;

   for (int i = 0; i < 16; i++)
   {
      // Get PSG state
      // TODO
      header[0x5B+i] = machine_->GetPSG()->register_[i];
   }

   // CPC Type : To do (6128)?
#if 0
   switch ( machine_->GetMachineType())
   {
   case 0:header[0x6D] = 0; break;  // 464
   case 1:header[0x6D] = 1; break;  // 664
   case 2:header[0x6D] = 2; break;  // 6128
   case 3:header[0x6D] = 5; break;  // 464+
   case 4:header[0x6D] = 4; break;  // 6128+
   }
#else
   header[0x6D] = 2;
#endif


   // MemEnable - Bit 7 set if used. Bit 0 = Bank C4..C7, Bit 1 = Banks C4..DF
   // Bit 2 = Banks E4..FF, Bit 3 = Full 4M expansion
   // TODO
   header[0x9A] ;

   //   9C	 1	 FDD motor drive state (0=off, 1=on)
   header[0x9c] = machine_->GetFDC()->IsMotorOn()?1:0;

   //   9D-A0	 1	 FDD current physical track (note 15)
   header[0x9D] = machine_->GetFDC()->GetCurrentTrack (0);
   header[0x9E] = machine_->GetFDC()->GetCurrentTrack (1);
   header[0x9F] = machine_->GetFDC()->GetCurrentTrack (2);
   header[0xA0] = machine_->GetFDC()->GetCurrentTrack (3);

   //   A1	 1	 Printer Data/Strobe Register (note 1)
   // TODO

   // A2 - A3 : Y line
   header[0xA2] = (machine_->GetMonitor()->y_ & 0xFF);
   header[0xA3] = ((machine_->GetMonitor()->y_ >>8)& 0xFF);

   //   A4	 1	CRTC type:
   //   CRTC type:
   //   0 = HD6845S/UM6845
   //   1 = UM6845R
   //   2 = MC6845
   //   3 = 6845 in CPC+ ASIC
   //   4 = 6845 in Pre-ASIC
   header[0xA4] = machine_->GetCRTC()->type_crtc_;

   //   A9	 1	 CRTC horizontal character counter register (note 11)
   header[0xA9] = machine_->GetCRTC()->hcc_;
   //   AA	 1	 unused (0)
   //   AB	 1	 CRTC character-line counter register (note 2)
   header[0xAB] = machine_->GetCRTC()->vcc_ ;
   //   AC	 1	 CRTC raster-line counter register (note 3)
   header[0xAC] = machine_->GetCRTC()->vlc_ ;
   //   AD	 1	 CRTC vertical total adjust counter register (note 4)
   header[0xAD] = machine_->GetCRTC()->vertical_adjust_counter_;
   //   AE	 1	 CRTC horizontal sync width counter (note 5)
   header[0xAE] = machine_->GetCRTC()->horinzontal_pulse_ ;
   //   AF	 1	 CRTC vertical sync width counter (note 6)
   header[0xAF] = machine_->GetCRTC()->scanline_vbl_ ;

   //   B0-B1	 2
   data = 0;
   //   CRTC state flags. (note 7)
   //   Bit	 Function
   //   0	 if "1" VSYNC is active, if "0" VSYNC is inactive (note 8)
   data |= machine_->GetSig()->h_sync_ ?0x01:0;
   //   1	 if "1" HSYNC is active, if "0" HSYNC is inactive (note 9)
   data |= machine_->GetCRTC()->ff4_?0x02:0;
   //   2-7	 reserved
   //   7	 if "1" Vertical Total Adjust is active, if "0" Vertical Total Adjust is inactive (note 10)
   data |= machine_->GetCRTC()->r4_reached_?0x80:0;
   //   8-15	 Reserved (0)
   header[0xB0] = data;
   header[0xB1] = 0;

   //   B2	 1	 GA vsync delay counter (note 14)
   data = machine_->GetVGA()->wait_for_hsync_;
   if (data > 0) data = 3-machine_->GetVGA()->wait_for_hsync_;
   header[0xB2] = data;

   //   B3	 1	 GA interrupt scanline counter (note 12)*
   header[0xB3] = machine_->GetVGA()->interrupt_counter_ +1;
   //   B4	 1	 interrupt request flag (0=no interrupt requested, 1=interrupt requested) (note 13)
   header[0xB4] = machine_->GetVGA()->sig_handler_->int_?1:0;

   //   B5-FF	 75	 unused (0)

   // TODO : Use this for version 1&2. V3 use MEM0-8 Chunks !

   // Memory dump size
   header[0x6B] = machine_->GetMem()->extended_ram_available_[0]?128:64;

   fwrite ( header, 0x100, 1, f );

   // Memory dump
   // Lower RAM
   for (int i = 0; i < 4; i++)
   {
      fwrite ( machine_->GetMem()->ram_buffer_[i], 1, 0x4000, f);
   }
   // Upper RAM
   if (machine_->GetMem()->extended_ram_available_[0])
   {
      for (int i = 0; i < 4; i++)
      {
         fwrite ( machine_->GetMem()->extended_ram_buffer_[0][i],  1, 0x4000, f);
      }
   }

   // Chunk
   // TODO CPC+
   if (machine_->IsPLUS())
   {
      unsigned char chunk_header[8] = {0};
      unsigned char plus_chunk[0x8F8] = { 0 };
      memcpy(chunk_header, "CPC+", 4);
      chunk_header[4] = 0xF8;
      chunk_header[5] = 0x08;

      unsigned char * asic_registers = machine_->GetMem()->GetAsicRegisters();
      // 000 - 7FF	800	4000 - 4FFF	Sprite Bitmaps(note 1)
      for (int i = 0; i < 0x800; i++)
      {
         plus_chunk[i] = (((asic_registers[i * 2]&0xF) << 4) | (asic_registers[i * 2 + 1]&0xF));
      }

      // 800 - 87F	8 * 16	6000 - 607F	Sprite Attributes(see below) (note 2)
      for (int i = 0; i < 0x80; i++)
      {
         plus_chunk[0x800 + i] = asic_registers[0x2000 + i];
      }

      // 880 - 8BF	32 * 2	6400 - 643F	Palettes(note 3)
      for (int i = 0; i < 64; i++)
      {
         plus_chunk[0x880 + i] = asic_registers[0x2400 + i];
      }

      // 8C0	1	6800	Programmable Raster Interrupt(note 4)
      // 8C1	1	6801	Screen split scan - line(note 4)
      // 8C2	2	6802 - 6803	Screen split secondary screen - address(note 4)
      // 8C4	1	6804	Soft scroll control register (note 4)
      // 8C5	1	6805	Interrupt vector(note 4)
      for (int i = 0; i < 6; i++)
      {
         plus_chunk[0x8C0 + i] = asic_registers[0x2800 + i];
      }

      // 8C6	1	Internal	gate array A0 register value(note 8a)
      // todo : ???

      // 8C6 - 8C7	1 - unused(0)
      // 8C8 - 8CF	8	6808 - 680f	Analogue input channels 0 - 7 (note 5)
      for (int i = 0; i < 8; i++)
      {
         plus_chunk[0x8C8 + i] = asic_registers[0x2808 + i];
      }
      // 8D0 - 8DB	3 * 4	6C00 - 6C0B	Sound DMA channel attributes 0 - 2 (see below) (note 6)
      for (int i = 0; i < 12; i++)
      {
         plus_chunk[0x8D0 + i] = asic_registers[0x2C00 + i];
      }
      // 8DC - 8DE	3 - unused(0)
      // 8DF	1	6C0F	DMA Control / Status(note 4)
      plus_chunk[0x8DF] = asic_registers[0x2C0F];

      // 8E0 - 8F4	3 * 7	Internal	DMA channel 0 - 2 internal registers(see below) (note 7)
      for (int dma_channel = 0; dma_channel < 3; dma_channel++)
      {
         DMA* dma = machine_->GetDMA(dma_channel);
         plus_chunk[0x8E0 + dma_channel * 7] = dma->repeat_counter_ & 0xFF;
         plus_chunk[0x8E0 + dma_channel * 7 + 1] = (dma->repeat_counter_ >>8);

         plus_chunk[0x8E0 + dma_channel * 7 + 2] = dma->repeat_addr_ & 0xFF;
         plus_chunk[0x8E0 + dma_channel * 7 + 3] = (dma->repeat_addr_ >> 8);

         plus_chunk[0x8E0 + dma_channel * 7 + 4] = dma->pause_counter_ & 0xFF;
         plus_chunk[0x8E0 + dma_channel * 7 + 5] = (dma->pause_counter_ >> 8);

         // pause prescalar : todo !
      }

      // 8F5	1	Internal	gate array A0 register value(note 8b)
      plus_chunk[0x8F5] = machine_->GetMem()->rmr2_;

      // 8F6	1	Internal	gate array A0 lock : 0->locked, != 0->unlocked(note 9)
      plus_chunk[0x8F6] = (machine_->GetVGA()->IsAsicLocked()?0:1);

      // 8F7	1	Internal	ASIC unlock sequence state(note 10)
      plus_chunk[0x8F7] = machine_->GetAsic()->GetIndexVerification();

      fwrite(chunk_header, 8, 1, f);
      fwrite(plus_chunk, 0x8F8, 1, f);
   }
   // TODO ROMS
   // TODO MEM0-8

}

