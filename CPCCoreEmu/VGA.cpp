#include "stdafx.h"
#include "VGA.h"
#include "CRTC.h"
#include <math.h>
#include "DMA.h"

// MACRO de profilage
// MACRO de profilage


extern unsigned int Mode0ExtendedLut [0x100][0x8];
extern unsigned int Mode1ExtendedLut [0x100][0x8];
extern unsigned int Mode2ExtendedLut [0x100][0x8];

extern unsigned char Mode0Lut[0x100][0x10];
extern unsigned char Mode1Lut[0x100][0x10];
extern unsigned char Mode2Lut[0x100][0x10];

#ifdef _DEBUG
//#define PROF
#endif

#ifdef PROF
#define START_CHRONO  QueryPerformanceFrequency((LARGE_INTEGER*)&freq);;QueryPerformanceCounter ((LARGE_INTEGER*)&s1);
#define STOP_CHRONO   QueryPerformanceCounter ((LARGE_INTEGER*)&s2);t=(DWORD)(((s2 - s1) * 1000000) / freq);
#define PROF_DISPLAY _stprintf(s, _T("Duree displays Frame: %d us\n"), t);OutputDebugString (s);

static __int64 s1, s2, freq;
static DWORD t;
static char s [1024];
#else
   #define START_CHRONO
   #define STOP_CHRONO
   #define PROF_DISPLAY
#endif


unsigned int ListeColorsIndexConvert[32] =
{
   0x54, 0x44, 0x55, 0x5c, 0x58, 0x5D, 0x4c, 0x45, 0x4d, 0x56, 0x46, 0x57, 0x5e, 0x40, 0x5f, 0x4e, 0x47, 0x4f, 0x52, 0x42, 0x53, 0x5a, 0x59,
   0x5b, 0x4a, 0x43, 0x4b, 0x41, 0x48, 0x49, 0x50, 0x51
};

unsigned int ListeColorsIndex[0x100];

unsigned int ListeColors[] =
{
   0x000201,
   0x00026B,
   0x0C02F4,
   0x6C0201,
   0x690268,
   0x6C02F2,
   0xF30506,
   0xF00268,
   0xF302F4,
   0x027801,
   0x007868,
   0x0C7BF4,
   0x6E7B01,
   0x6E7D6B,
   0x6E7BF6,
   0xF37D0D,
   0xF37D6B,
   0xFA80F9,
   0x02F001,
   0x00F36B,
   0x0FF3F2,
   0x71F504,
   0x71F36B,
   0x71F3F4,
   0xF3F30D,
   0xF3F36D,
   0xFFF3F9,
   0x6E7B6D,
   0xF30268,
   0xF3F36B,
   0x000268,
   0x02F36B
};


GateArray::GateArray(void) : unlocked_(false), plus_(false), dma_list_(nullptr)
{
   memory_ram_buffer_ = 0;
   scanline_type_ = 0;

   // Init index of colors
   int i;
   buffered_ink_available_ = false;
   for (i =0; i < 32; i++)
   {
      ListeColorsIndex[ListeColorsIndexConvert[i]] = ListeColors[i];
   }


   // Look up table
   for (i = 0; i < 0x100; i++)
   {
      byte_to_pixel00_[i] = ((i & 0x80) ? 1 : 0) + ((i & 0x8) ? 2 : 0) + ((i & 0x20) ? 4 : 0) + ((i & 0x2) ? 8 : 0);
      byte_to_pixel01_[i] = ((i & 0x40) ? 1 : 0) + ((i & 0x4) ? 2 : 0) + ((i & 0x10) ? 4 : 0) + ((i & 0x1) ? 8 : 0);
      byte_to_pixel03_[i] = ((i & 0x80) ? 1 : 0) + ((i & 0x8) ? 2 : 0);
      byte_to_pixel03_b_[i] = ((i & 0x40) ? 1 : 0) + ((i & 0x4) ? 2 : 0);
   }

   // Init mode 0
   /*
   for (i = 0; i < 0x100; i++)
   {
      ByteToPixel00[i] = ((i&0x80)?1:0) + ((i&0x8)?2:0) +((i&0x20)?4:0) +((i&0x2)?8:0);
      ByteToPixel01[i] = ((i&0x40)?1:0) + ((i&0x4)?2:0) +((i&0x10)?4:0) +((i&0x1)?8:0);
      ByteToPixel03[i] = ((i&0x80)?1:0) + ((i&0x8)?2:0) ;
      ByteToPixel03_B[i] = ((i&0x40)?1:0) + ((i&0x4)?2:0);
   }*/
   monitor_ = NULL;
   //m_VideoBuffer = NULL;
   //m_Screen = NULL;

   Reset ();
   type_gate_array_ = GA_40010 ;
START_CHRONO
}



GateArray::~GateArray(void)
{
}

void GateArray::Reset ()
{
   ssa_new_ = ssa_new_counter_ = ssa_ = 0;

   // todo :  check this ?
   unlocked_ = false;

   clock_count_ = 0;
   interrupt_raised_ = false;
   interrupt_counter_ = 0;
   wait_for_hsync_ = 0;
//   m_X = 0;
//   m_Y = 0;

   /*if (m_Screen!= NULL)
      m_BeginingOfLine = m_VideoBuffer = m_Screen->GetVideoBuffer (m_Y);*/

   screen_mode_ = 0 ; // TODO : Check this
   buffered_screen_mode_ = screen_mode_ ;
   activation_mode_ = 2;

   hsync_counter_ = 0;
   h_old_sync_ = false;
   hsync_ = false;
   vsync_ = false;
}

void GateArray::SetBus (Bus* address, Bus* data)
{
   address_bus_ = address;
   data_bus_ = data;

}

//////////////////////////////////////////
//
void GateArray::PreciseTick ()
{
   // Up
   // Clock divisions
   clock_count_++;
   if ((clock_count_ & 0x01) == 0x0)
   {
      // 4 Mhz Clock inversion
      clock_4_mhz_.Tick ();

      if ((clock_count_ & 0x07) == 0)
      {
         // 1 Mhz Clock inversion
         clock_1_mhz_.Tick ();
      }
   }

   // Standard Gate array behaviour


   // Down

}

unsigned int GateArray::Tick (/*unsigned int nbTicks*/)
{
   // MAJ SPLT ?
   if (ssa_new_counter_ > 0)
   {
      --ssa_new_counter_;
      if (ssa_new_counter_ == 0)
         ssa_ = ssa_new_;
   }


   //TickDisplays ();
// Something to do ?
   // Falling edge of HSync
   if ( sig_handler_->hsync_fall_)
   //if ( HOldSync == true && m_Sig->HSync == false )
   {
      interrupt_counter_ = (interrupt_counter_+1)&0x3F;

      // Plus ?
      if (plus_ && memory_->GetPRI() !=  0)
      {
         if (interrupt_counter_ == 52)interrupt_counter_ = 0;

         if (wait_for_hsync_ > 0)
         {
            wait_for_hsync_--;
            if (wait_for_hsync_ == 0)
               interrupt_counter_ = 0;
         }
      }
      else
      {

         //m_ModeEcranCached = m_ModeEcran ;
         if (wait_for_hsync_ > 0)
         {
            wait_for_hsync_--;
            if (wait_for_hsync_ == 0)
            {
               // If the counter>=32 (bit5=1), then no interrupt request is issued and counter is reset to 0.
               if (interrupt_counter_ >= 32)
               {
                  // If the counter<32 (bit5=0), then an interrupt request is issued and counter is reset to 0.

                  // plus todo : shift int by 1us
                  if (unlocked_)
                  {
                     sig_handler_->interrupt_io_data_ = (memory_->GetIVR() & 0xF8) | 0x6;
                  }
                  else
                  {
                     sig_handler_->interrupt_io_data_ = (plus_ ? 0x00 : 0xFF);
                  }
                  sig_handler_->InterruptRaster();
                  sig_handler_->int_is_gate_array_ = true;
                  interrupt_raised_ = true;
               }
               // TODO everytime ????? sure ??????????????
               // check with int from acid tests
               interrupt_counter_ = 0;

            }
         }
         else
         {
            if (interrupt_counter_ == 52)
            {
               // Raise ctrl_int signal
               if (unlocked_)
               {
                  sig_handler_->interrupt_io_data_ = (memory_->GetIVR() & 0xF8) | 0x6;
               }
               else
               {
                  sig_handler_->interrupt_io_data_ = (plus_ ? 0x00 : 0xFF);
               }
               sig_handler_->InterruptRaster();
               sig_handler_->int_is_gate_array_ = true;
               interrupt_raised_ = true;
               interrupt_counter_ = 0;
            }
         }
      }

      if(vsync_)
         vsync_counter_++;

      if ( vsync_  && (vsync_counter_ == 26))
      {
         vsync_ = false;
      }

      //m_Sig->HsyncFallWr = false;
      sig_handler_->hsync_fall_ = false;

   }

   //m_VSync = m_Sig->VSync;
   // Seems that it should occur either on HSync fall or when Monitor hsync is falling (ie 6us after hsync start at most)
   if ((sig_handler_->hsync_raise_/*HOldSync == false && m_Sig->HSync == true*/)
      || (sig_handler_->h_sync_on_begining_of_line_)
      || (sig_handler_->pri_changed_ && sig_handler_->h_sync_ == true))
   {
      if (plus_ /*&& (m_HSyncCounter == 10)*/) // 10 us after HSYNC start
      {
         if (/*unlocked_ && */memory_->GetPRI() != 0)
         {
            // Raster interrupt ?
            unsigned char pri = memory_->GetPRI();

            if ((crtc_->vcc_ &0x3F) == (pri >> 3)
               && (crtc_->vlc_ &07)== (pri & 0x7))

            {
               // Set interrupt vector
               // IVR ( top 5 bits) + source : raster interrupt
               if (unlocked_)
               {
                  sig_handler_->interrupt_io_data_ = (memory_->GetIVR() & 0xF8) | 0x6;
               }
               else
               {
                  sig_handler_->interrupt_io_data_ = 0x00;
               }

               // Update DCSR
               unsigned char dcsr = memory_->GetDCSR();
               dcsr |= 0x80;
               memory_->SetDCSR(dcsr);


               sig_handler_->InterruptRaster();
               interrupt_raised_ = true;

               // todo : sort this out
               //m_Interruptcounter = 0; // with this, it seems to be ok
                                       //m_Interruptcounter &= 0x1F;

            }
         }

      }
   }
   sig_handler_->pri_changed_ = false;
   // Begin of HSYNC
   if ( (h_old_sync_ == false && sig_handler_->h_sync_ == true ))
   {
      hsync_ = true;
      hsync_counter_ = 0;
   }
   else
   {
      hsync_counter_++;
   }




   // TODO : This is incorrect : the mode changing should occur after the 2nd us APRES la reception du HSYNC.
   // Mais ca empeche "Imperial Mahjong" de fonctionner correctement
   if (hsync_)
   {
      if ((sig_handler_->h_sync_ == false || hsync_counter_ == 6))
      {
         hsync_ = false;
      }

      if (activation_mode_ != 0)
      {
         activation_mode_--;

         if (activation_mode_ == 0)
         {
            buffered_screen_mode_ = screen_mode_;
         }
      }
   }
   // End of HSYNC period

   // Ack of ctrl_int : bit 5 of interrupt counter reset
   /*if (m_InterruptRaised && m_Sig->ctrl_int == false ) // Cleared interrupt
   {
      m_InterruptRaised = false;
      m_Interruptcounter &= 0x1F; // Bit 5 reset
   }*/

   if ( v_old_sync_ == false && sig_handler_->v_sync_ == true )
   {
      // Wait for two HSync
      wait_for_hsync_ = 2;
      vsync_ = true;
      vsync_counter_ = 0;

   }

   v_old_sync_ = sig_handler_->v_sync_;
   h_old_sync_ = sig_handler_->h_sync_;

   if (plus_ && sig_handler_->hsync_raise_)
   {
      // DMA ?
      HandleDMA();
      sig_handler_->hsync_raise_ = false;
   }

   
#ifdef __NoOffset
   display_short_ = *(short*)(memory_->ram_buffer_[0]+ADDRESS);
  dispen_buffered_ = sig_handler_->DISPEN;
#endif

#define MA unsigned short ma = (crtc_->ma_);
#define RA unsigned char ra = crtc_->vlc_+vertical_shift;/*if (ra > 7) ma += monitor_->m_pCRTC->m_Register[1];*/
#define ADDRESS  ((((ma )& 0x3FF)<<1) | (ra & 0x7) <<11| ((ma& 0x3000)<<2))

#define DISPEN_TEST dispen_buffered_ = (crtc_->ff3_ & crtc_->ff1_ /*& m_pCRTC->mb_SSCR_Bit_8*/)
#define END_OF_DISPLAY   {monitor_->IncVideoBuffer();MA ; RA; display_short_.word = *(short*)(memory_->ram_buffer_[0] + ADDRESS); DISPEN_TEST;monitor_->Tick();return 4;}

   // PLUS : Handle the SSCR register
   unsigned char vertical_shift = 0;
   unsigned char horizontal_shift = 0;
   unsigned char extended_border = 0;
   if (plus_)
   {
      vertical_shift = (memory_->GetSSCR()&0x7F) >> 4;
      horizontal_shift = memory_->GetSSCR() & 0xF;
      extended_border = (memory_->GetSSCR() & 0x80) ? 16 : 0;

      if (horizontal_shift > 0 || vertical_shift > 0)
      {
         int dbg = 1;
      }
   }

   // Fill the byte for the memory buffer
   // 16 pixels should be defined
   int* buffer_to_display = monitor_->GetVideoBufferForInc ();
   if (buffer_to_display == 0)
   {
      END_OF_DISPLAY
   }
   else
   {
      if ( hsync_
         || vsync_)
      {
         memset ( buffer_to_display, 0, 16);
         if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
         END_OF_DISPLAY
      }
      else
      {
         if ( dispen_buffered_/*m_Sig->DISPEN*/ )
         {
            switch (buffered_screen_mode_)
            {
            case 0:
            {
               if ( plus_)
               {
                  // get color palette number from pixels
                  //Mode1Lut[m_DisplayShort._word];
                  // COLOUR : P1, P2, P3, P4
                  if (display_short_.word != 0)
                  {
                     int dbg = 1;
                  }
                  unsigned short c1 = 0;
                  c1 += byte_to_pixel00_[display_short_.byte.l];
                  c1 <<= 4;
                  c1 += byte_to_pixel01_[display_short_.byte.l];
                  c1 <<= 4;
                  c1 += byte_to_pixel00_[display_short_.byte.h];
                  c1 <<= 4;
                  c1 += byte_to_pixel01_[display_short_.byte.h];
                  // Shift it
                  unsigned short oldcol = c1;
                  c1 >>= horizontal_shift;

                  // Add left remaining
                  c1 |= (prev_col_ << (16 - horizontal_shift));
                  prev_col_ = oldcol;

                  if (crtc_->sscr_bit_8_)
                  {

                     for (int i = 0; i < 4; i++)
                     {
                        buffer_to_display[i * 4] = ink_list_[(c1 >> (16 - (i * 4 + 4))) & 0xF];
                        buffer_to_display[i * 4 + 1] = buffer_to_display[i * 4];
                        buffer_to_display[i * 4 + 2] = buffer_to_display[i * 4];
                        buffer_to_display[i * 4 + 3] = buffer_to_display[i * 4];

                        if (i == 0 && buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     }
                     // Sprite
                     DrawSprites(buffer_to_display);

                  }
                  else
                  {
                     memcpy(buffer_to_display, video_border_, 4 * NB_BYTE_BORDER);
                     memcpy(&buffer_to_display[NB_BYTE_BORDER], video_border_, 4 * NB_BYTE_BORDER);
                     if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     memcpy(&buffer_to_display[NB_BYTE_BORDER * 2], video_border_, 4 * NB_BYTE_BORDER);
                     memcpy(&buffer_to_display[NB_BYTE_BORDER * 3], video_border_, 4 * NB_BYTE_BORDER);

                  }
                  // next byte

                  END_OF_DISPLAY
               }
               else
               {
                  if (!crtc_->de_bug_)
                  {
                     memcpy(buffer_to_display, Mode0ExtendedLut[display_short_.byte.l], 8 * sizeof(int));
                     if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     memcpy(&buffer_to_display[8], Mode0ExtendedLut[display_short_.byte.h], 8 * sizeof(int));

                     //END_OF_DISPLAY
                     monitor_->IncVideoBuffer();
                     unsigned int addr = ((((crtc_->ma_) & 0x3FF) << 1) | (((crtc_->vlc_ ) & 0x7) << 11) | ((crtc_->ma_ & 0x3000) << 2));
                     display_short_.word = *(short*)(memory_->ram_buffer_[0] + addr);
                     if (horizontal_shift > 0)
                     {
                        display_short_.word <<= horizontal_shift;
                        unsigned short prev = *(short*)(memory_->ram_buffer_[0] + addr - 2);
                        display_short_.word |= ((prev >> (16 - horizontal_shift)) & 0xFFFF);
                     }
                     DISPEN_TEST; monitor_->Tick();
                     return 4;

                  }
                  else
                  {
                     *buffer_to_display = video_border_[0];
                     crtc_->de_bug_ = false;
                     memcpy(&buffer_to_display[1], &Mode0ExtendedLut[display_short_.byte.l][1], 7 * sizeof(int));
                     if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     memcpy(&buffer_to_display[8], Mode0ExtendedLut[display_short_.byte.h], 8 * sizeof(int));

                     END_OF_DISPLAY
                  }
               }
            }
            case 1:

               if (plus_)
               {
                  // get color palette number from pixels
                  //Mode1Lut[m_DisplayShort._word];
                  // COLOUR : P1, P2, P3, P4
                  if (display_short_.word != 0)
                  {
                     int dbg = 1;
                  }
                  unsigned short c1 = ((display_short_.byte.l & 0x80) ? 0x80 : 0) + ((display_short_.byte.l & 0x8) ? 0x40 : 0)
                  + ((display_short_.byte.l & 0x40) ? 0x20 : 0) + ((display_short_.byte.l & 0x4) ? 0x10 : 0)
                  + ((display_short_.byte.l & 0x20) ? 0x8 : 0) + ((display_short_.byte.l & 0x2) ? 0x4 : 0)
                  + ((display_short_.byte.l & 0x10) ? 0x2 : 0) + ((display_short_.byte.l & 0x1) ? 0x1 : 0);
                  c1 <<= 8;
                  c1 += ((display_short_.byte.h & 0x80) ? 0x80 : 0) + ((display_short_.byte.h & 0x8) ? 0x40 : 0)
                     + ((display_short_.byte.h & 0x40) ? 0x20 : 0) + ((display_short_.byte.h & 0x4) ? 0x10 : 0)
                     + ((display_short_.byte.h & 0x20) ? 0x8 : 0) + ((display_short_.byte.h & 0x2) ? 0x4 : 0)
                     + ((display_short_.byte.h & 0x10) ? 0x2 : 0) + ((display_short_.byte.h & 0x1) ? 0x1 : 0);

                  // Shift it
                  unsigned short oldcol = c1;
                  c1 >>= horizontal_shift;

                  // Add left remaining
                  c1 |= (prev_col_ << (16 - horizontal_shift));
                  prev_col_ = oldcol;

                  if (crtc_->sscr_bit_8_)
                  {
                     for (int i = 0; i < 8; i++)
                     {
                        // todo : optimize this awfull fix !!!
                        unsigned char c = (c1 >> (16 - ((i + 1) * 2))) & 0x3;
                        unsigned char c01 = c & 1;
                        unsigned char c02 = c & 2;
                        c = (c02 >> 1) + (c01 << 1);

                        buffer_to_display[i * 2] = ink_list_[c];
                        buffer_to_display[i * 2 + 1] = ink_list_[c];

                        //pBufferToDisplay[i * 2] = m_InkList[(c1 >> (16-((i+1)*2))) & 0x3];
                        //pBufferToDisplay[i * 2+1] = m_InkList[(c1 >> (16 - ((i+1) * 2 ))) & 0x3];

                        if (i == 1 && buffered_ink_available_)
                        {
                           monitor_->RecomputeColors(); buffered_ink_available_ = false;
                        }
                     }
                     // Sprite
                     DrawSprites(buffer_to_display);

                  }
                  else
                  {
                     memcpy(buffer_to_display, video_border_, 4 * NB_BYTE_BORDER);
                     memcpy(&buffer_to_display[NB_BYTE_BORDER], video_border_, 4 * NB_BYTE_BORDER);
                     if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     memcpy(&buffer_to_display[NB_BYTE_BORDER * 2], video_border_, 4 * NB_BYTE_BORDER);
                     memcpy(&buffer_to_display[NB_BYTE_BORDER * 3], video_border_, 4 * NB_BYTE_BORDER);

                  }

                  // next byte
                  END_OF_DISPLAY
               }
               else
               {
                  if (!crtc_->de_bug_)
                  {
                     memcpy(&buffer_to_display[0], &Mode1ExtendedLut[(display_short_.byte.l)], 8 * sizeof(int));

                     if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     memcpy(&buffer_to_display[8], Mode1ExtendedLut[(display_short_.byte.h)], 8 * sizeof(int));
                     END_OF_DISPLAY
                  }
                  else
                  {
                     buffer_to_display[0] = video_border_[0];
                     crtc_->de_bug_ = false;
                     memcpy(&buffer_to_display[1], &Mode1ExtendedLut[(display_short_.byte.l)][1], 7 * sizeof(int));
                     if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     memcpy(&buffer_to_display[8], Mode1ExtendedLut[(display_short_.byte.h)], 8 * sizeof(int));
                     END_OF_DISPLAY
                  }
               }


            case 2:
            {
               if (plus_)
               {
                  // get color palette number from pixels
                  //Mode1Lut[m_DisplayShort._word];
                  // COLOUR : P1, P2, P3, P4
                  if (display_short_.word != 0)
                  {
                     int dbg = 1;
                  }
                  unsigned short c1 = display_short_.word;
                  c1 = 0;
                  for (int i = 0; i < 8; ++i)
                  {
                     c1 += (((display_short_.byte.l &(0x80 >> i)) ? 0x80 : 0) >> i);
                  }
                  c1 <<= 8;
                  for (int i = 0; i < 8; ++i)
                  {
                     c1 += (((display_short_.byte.h &(0x80 >> i)) ? 0x80 : 0) >> i);
                     //c1 += Mode2Lut[m_DisplayShort._byte.h][i];
                  }

                  // Shift it
                  unsigned short oldcol = c1;
                  c1 >>= horizontal_shift;

                  // Add left remaining
                  c1 |= (prev_col_ << (16 - horizontal_shift));
                  prev_col_ = oldcol;

                  for (int i = 0; i < 8; ++i)
                  {
                     Mode2ExtendedLut[c1&0xFF][i] = ink_list_[Mode2Lut[c1 & 0xFF][i]];
                     Mode2ExtendedLut[c1>>8][i] = ink_list_[Mode2Lut[c1 >> 8][i]];
                  }

                  if (crtc_->sscr_bit_8_)
                  {

                     for (int i = 0; i < 8; i++)
                     {
                        unsigned char c = (c1 >> (16 - (i + 1))) & 0x1;
                        buffer_to_display[i] = ink_list_[c];
                        if (i == 3 && buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     }
                     for (int i = 0; i < 8; i++)
                     {
                        unsigned char c = (c1 >> (8 - (i+1))) & 0x1;
                        buffer_to_display[i+8] = ink_list_[c];


                     }
                     // Sprite
                     DrawSprites(buffer_to_display);


                     /*memcpy(pBufferToDisplay, &Mode2ExtendedLut[c1 & 0xFF], 4 * sizeof(int));

                     if (m_bCachedInk) { monitor_->RecomputeColors(); m_bCachedInk = false; }
                     memcpy(&pBufferToDisplay[4], &Mode2ExtendedLut[c1 & 0xFF][4], 4 * sizeof(int));
                     memcpy(&pBufferToDisplay[8], &Mode2ExtendedLut[c1 >> 8], 8 * sizeof(int));*/
                  }
                  else
                  {
                     memcpy(buffer_to_display, video_border_, 4 * NB_BYTE_BORDER);
                     memcpy(&buffer_to_display[NB_BYTE_BORDER], video_border_, 4 * NB_BYTE_BORDER);
                     if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     memcpy(&buffer_to_display[NB_BYTE_BORDER * 2], video_border_, 4 * NB_BYTE_BORDER);
                     memcpy(&buffer_to_display[NB_BYTE_BORDER * 3], video_border_, 4 * NB_BYTE_BORDER);

                  }

                  // next byte
                  END_OF_DISPLAY
               }
               else
               {
                  if (!crtc_->de_bug_)
                  {
                     memcpy(buffer_to_display, &Mode2ExtendedLut[display_short_.byte.l], 8 * sizeof(int));

                     if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     memcpy(&buffer_to_display[8], &Mode2ExtendedLut[display_short_.byte.h], 8 * sizeof(int));

                     END_OF_DISPLAY
                  }
                  else
                  {
                     *buffer_to_display++ = video_border_[0];
                     crtc_->de_bug_ = false;
                     memcpy(&buffer_to_display[1], &Mode2ExtendedLut[display_short_.byte.l][1], 7 * sizeof(int));

                     if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
                     memcpy(&buffer_to_display[8], &Mode2ExtendedLut[display_short_.byte.h], 8 * sizeof(int));

                     END_OF_DISPLAY
                  }
               }

            }
            case 3:
            {
               if (plus_)
               {
                  // Sprite
                  DrawSprites(buffer_to_display);

                  END_OF_DISPLAY
               }
               else
               {
                  //unsigned int * pBufferToDisplay = monitor_->m_BufferToDisplay;
                  if (crtc_->de_bug_)
                  {
                     *buffer_to_display++ = video_border_[0];
                     crtc_->de_bug_ = false;
                  }
                  else
                     *buffer_to_display++ = ink_list_[byte_to_pixel03_[display_short_.word & 0xFF]];

                  *buffer_to_display++ = ink_list_[byte_to_pixel03_[display_short_.word & 0xFF]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_[display_short_.word & 0xFF]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_[display_short_.word & 0xFF]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_b_[display_short_.word & 0xFF]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_b_[display_short_.word & 0xFF]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_b_[display_short_.word & 0xFF]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_b_[display_short_.word & 0xFF]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_[display_short_.word >> 8]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_[display_short_.word >> 8]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_[display_short_.word >> 8]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_[display_short_.word >> 8]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_b_[display_short_.word >> 8]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_b_[display_short_.word >> 8]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_b_[display_short_.word >> 8]];
                  *buffer_to_display++ = ink_list_[byte_to_pixel03_b_[display_short_.word >> 8]];
                  END_OF_DISPLAY
               }
            }
            }
         }
         else
         {
            memcpy (buffer_to_display, video_border_, 4* NB_BYTE_BORDER);
            memcpy(&buffer_to_display[NB_BYTE_BORDER], video_border_, 4 * NB_BYTE_BORDER);
            if (buffered_ink_available_) { monitor_->RecomputeColors(); buffered_ink_available_ = false; }
            memcpy(&buffer_to_display [NB_BYTE_BORDER*2], video_border_, 4 * NB_BYTE_BORDER);
            memcpy(&buffer_to_display[NB_BYTE_BORDER * 3], video_border_, 4 * NB_BYTE_BORDER);
            END_OF_DISPLAY
         }
      }
      //monitor_->IncVideoBuffer();
   }
   END_OF_DISPLAY
}


void GateArray::TickIO ()
{
   // Something on IO Port ?
   // IORQ = 1 and Adress = 7F00 (01 for adress bit 15-14)
   unsigned char data = data_bus_->GetByteBus ();
   if (( address_bus_->GetShortBus () & 0xC000) == 0x4000)
   {
      // Decodage
      switch (data & 0xE0)
      {

      case 0x00 :  // PENR
      case 0x20 :  // PENR
         pen_r_ = data & 0x1F;
         /*if ( m_bSpeed)
         {
            monitor_->RecomputeColors ();
         }*/

         break;
      case 0x40 :  // INKR
      case 0x60 :  // INKR
         // TODO : Decaller le changement de couleur par 8 pixels (9 sur 40010 en mode 2)
         if ((pen_r_ & 0x10) == 0x10)
         {
            // Border
            //monitor_->SetBorder ( ListeColorsIndex [data & 0x5F] );
            for (int i = 0; i < NB_BYTE_BORDER; i++)video_border_[i] = ListeColorsIndex[data & 0x5F];
            memory_->UpdateAsicPalette(17, data - 0x40);
         }
         else
         {
            if (monitor_->screen_->IsDisplayed () )
            {
               buffered_ink_ = ListeColorsIndex [data & 0x5F];

               memory_->UpdateAsicPalette(pen_r_, data - 0x40);
               buffered_ink_available_ = true;
               /*if ( m_bSpeed)
               {
                  monitor_->RecomputeColors ();
               }*/
            }
            else
            {
               ink_list_ [pen_r_] = ListeColorsIndex [data & 0x5F];
            }

         }

         break;

      case 0x80 :  // RMR
      case 0xA0 :  // RMR
         if ((data & 0x20) == 0x20 && unlocked_)
         {
            memory_->SetRmr2(data & 0x1F);
         }
         else
         {
            screen_mode_ = data & 0x3;
            activation_mode_ = 1;

            // l'indiquer à la mémoire
            memory_->SetInfROMConnected((data & 0x4) ? false : true);
            memory_->SetSupROMConnected((data & 0x8) ? false : true);

            if (data & 0x10)
            {
               // Remise a zero diviseur d'interruption
               interrupt_counter_ = 0;
               sig_handler_->req_int_ = false;
               sig_handler_->int_ = false;
               //m_Sig->AcqInt ();

               interrupt_raised_ = false;
            }
         }

         break;
      case 0xC0:
      case 0xE0:
         // MMR
         {
            int dbg = 1;
         }
         break;

      }

   }
   // PAL

   if (pal_present_  && (( address_bus_->GetShortBus () & 0x8000) == 0x0000))
   {
      // todo
      switch (data & 0xE0)
      {
      case 0xC0:  // Memory management
      case 0xE0:  // Memory management
         {
            int i = 0;
            // Decode 64k page
            unsigned char p = data&0x38;
            p = p>>3;
            unsigned char b = data & 0x3;
            unsigned char s = data & 0x4;
            s = s >>2;
            memory_->ConnectBank ( p, s, b);

            break;
         }
      default:
         {
         int dbg = 1;
         break;
         }
      }
   }
      //m_Sig->IORW = false;

      if (( address_bus_->GetShortBus () & 0x2000) == 0x0000)
      {
         // Numero de ROM logique
         unsigned char data = data_bus_->GetByteBus ();
         memory_->SetLogicalROM ( data );
         // Already read
         //m_Sig->IORW = false;
      }
}

void GateArray::TickDisplays ()
{
}

void GateArray::DrawSprites(int * buffer_display)
{
   short x = (crtc_->hcc_ - 1) << 4;
   short y = (crtc_->vcc_);
   short sc = crtc_->vlc_;
   y = (y << 3) + sc;

   // Check every sprite
   for (int i = 15; i >= 0; i--)
   {
      // Check if sprite "i" should render here something different than 0
      Memory::TSpriteInfo* sprite = memory_->GetSpriteInfo(i);
      //if (magnificationX > 0 && magnificationY > 0)
      if (sprite->displayed)
      {

         if (y == 0)
         {
            int dbg = 1;
         }
         short disp_y = (y - sprite->y);
         short disp_x = (x - sprite->x);

         int magx = sprite->sizex;
         int magy = sprite->sizey;

         if (disp_x+16 >= 0 && disp_x < (magx) && disp_y >= 0 && disp_y < (magy))
         {
            unsigned char magnification_x = sprite->zoomx - 1;
            unsigned char* sprite_data = memory_->GetSprite(i) + (disp_y >> (sprite->zoomy - 1)) * 16;
            for (int buff_x = 0; buff_x < 16; buff_x++)
            {
               if (disp_x >= 0 && disp_x < (magx))
               {
                  // Display colour
                  int col = sprite_data[(disp_x >> magnification_x) /*+ index_y*/] & 0xF;

                  if (col != 0)
                  {
                     buffer_display[buff_x] = sprite_ink_list_[col];
                  }
               }
               disp_x++;
            }
         }
      }
   }
}

void GateArray::HandleDMA()
{
   unsigned char dcsr = memory_->GetDCSR();
   if ((dcsr & 0x1) == 0x1)
   {
      dma_list_[0].Hbl();
   }
   if ((dcsr & 0x2) == 0x2)
   {
      dma_list_[1].Hbl();
   }
   if ((dcsr & 0x4) == 0x4)
   {
      dma_list_[2].Hbl();
   }
}
DMA* GateArray::GetDMAChannel(int channel)
{
   return &dma_list_[channel];
}

GateArray * GateArray::CopyMe()
{
   GateArray * new_vga = new GateArray();
   *new_vga = *this;
   return new_vga;
}

void GateArray::DeleteCopy(GateArray*vga)
{
   delete vga;
}
bool GateArray::CompareToCopy(GateArray* other)
{
   if (other->interrupt_counter_ != interrupt_counter_) return false;
   if (other->wait_for_hsync_ != wait_for_hsync_) return false;
   if (other->hsync_ != hsync_) return false;
   if (other->vsync_ != vsync_) return false;
   if (other->dispen_buffered_ != dispen_buffered_) return false;

   if (other->pen_r_ != pen_r_) return false;
   if (memcmp( other->ink_list_, ink_list_, sizeof(ink_list_)) != 0 )return false;
   if (memcmp(other->sprite_ink_list_, sprite_ink_list_, sizeof(sprite_ink_list_)) != 0)return false;

   if (other->dispen_buffered_ != dispen_buffered_) return false;
   if (other->screen_mode_ != screen_mode_) return false;

   if (other->hsync_counter_ != hsync_counter_) return false;
   if (other->vsync_counter_ != vsync_counter_) return false;
   if (other->screen_mode_ != screen_mode_) return false;


   return true;
}
