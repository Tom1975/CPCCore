#include "stdafx.h"
#include "Monitor.h"

#include "VGA.h"


#include "simple_math.h"

//#define PROF
#ifdef PROF
#define START_CHRONO  if(countLoop==0){QueryPerformanceFrequency((LARGE_INTEGER*)&freq);;QueryPerformanceCounter ((LARGE_INTEGER*)&s1);}
#define STOP_CHRONO   if(countLoop==100000000){QueryPerformanceCounter ((LARGE_INTEGER*)&s2);totalcount++;total+=(DWORD)(((s2 - s1) * 1000000) / freq);t=total/totalcount;countLoop=0;_stprintf_s(s, 1024, _T("%s: %d us\n"),TXT_OUT_PROF, t);OutputDebugString (s);}else{countLoop++;}

static __int64 s1, s2, freq, total = 0;
static DWORD t = 0;
static char s[1024];
static int countLoop = 0;
static int totalcount = 0;
#else
#define START_CHRONO
#define STOP_CHRONO
#endif

#define NBPIXELADDED 16



#define DISP(c) *(m_VideoBuffer)/*[m_X]*/ = c

unsigned char Mode0Lut[0x100][0x10];
unsigned char Mode1Lut[0x100][0x10];
unsigned char Mode2Lut[0x100][0x10];

unsigned int Mode0ExtendedLut[0x100][0x8];
unsigned int Mode1ExtendedLut[0x100][0x8];
unsigned int Mode2ExtendedLut[0x100][0x8];
unsigned int Mode3ExtendedLut[0x100][0x8];


Monitor::Monitor(void) : memory_(0), playback_sync_(false)
{
   playback_ = nullptr;
   int i, b;


   video_buffer_ = nullptr;
   screen_ = nullptr;

   hsync_total_ = 0;

   // Look up table
   for (i = 0; i < 0x100; i++)
   {
      byte_to_pixel00_[i] = ((i & 0x80) ? 1 : 0) + ((i & 0x8) ? 2 : 0) + ((i & 0x20) ? 4 : 0) + ((i & 0x2) ? 8 : 0);
      byte_to_pixel01_[i] = ((i & 0x40) ? 1 : 0) + ((i & 0x4) ? 2 : 0) + ((i & 0x10) ? 4 : 0) + ((i & 0x1) ? 8 : 0);
      byte_to_pixel03_[i] = ((i & 0x80) ? 1 : 0) + ((i & 0x8) ? 2 : 0);
      byte_to_pixel03_b_[i] = ((i & 0x40) ? 1 : 0) + ((i & 0x4) ? 2 : 0);

      for (int j = 0; j < 4; j++)
      {
         Mode0Lut[i][j] = byte_to_pixel00_[i];
         Mode0Lut[i][4 + j] = byte_to_pixel01_[i];
      }
   }

   // Look up table mode 1
   for (i = 0; i < 16; i++)
   {
      for (b = 0; b < 0x100; b++)
      {

         if ((i % 8) < 2)
            Mode1Lut[b][i] = ((b & 0x80) ? 1 : 0) + ((b & 0x8) ? 2 : 0);
         else if ((i % 8) < 4)
            Mode1Lut[b][i] = ((b & 0x40) ? 1 : 0) + ((b & 0x4) ? 2 : 0);
         else if ((i % 8) < 6)
            Mode1Lut[b][i] = ((b & 0x20) ? 1 : 0) + ((b & 0x2) ? 2 : 0);
         else
            Mode1Lut[b][i] = ((b & 0x10) ? 1 : 0) + ((b & 0x1) ? 2 : 0);
      }
   }

   // Look up table mode 2
   for (b = 0; b < 0x100; ++b)
   {
      for (auto i = 0; i < 8; ++i)
      {
         Mode2Lut[b][i] = ((b &(0x80 >> i)) ? 1 : 0);
      }
   }

   Reset();
}

void Monitor::SetVGA(GateArray * vga)
{
   gate_array_ = vga;
   memory_ = gate_array_->memory_;
   RecomputeAllColors();
}

void Monitor::RecomputeAllColors()
{

   // Look up table mode 0, 1, 2
   for (int b = 0; b < 0x100; ++b)
   {
      Mode0ExtendedLut[b][0] = gate_array_->ink_list_[byte_to_pixel00_[b]];
      Mode0ExtendedLut[b][1] = gate_array_->ink_list_[byte_to_pixel00_[b]];
      Mode0ExtendedLut[b][2] = gate_array_->ink_list_[byte_to_pixel00_[b]];
      Mode0ExtendedLut[b][3] = gate_array_->ink_list_[byte_to_pixel00_[b]];
      Mode0ExtendedLut[b][4] = gate_array_->ink_list_[byte_to_pixel01_[b]];
      Mode0ExtendedLut[b][5] = gate_array_->ink_list_[byte_to_pixel01_[b]];
      Mode0ExtendedLut[b][6] = gate_array_->ink_list_[byte_to_pixel01_[b]];
      Mode0ExtendedLut[b][7] = gate_array_->ink_list_[byte_to_pixel01_[b]];

      for (int i = 0; i < 8; ++i)
      {
         Mode1ExtendedLut[b][i] = gate_array_->ink_list_[Mode1Lut[b][i]];
         Mode2ExtendedLut[b][i] = gate_array_->ink_list_[Mode2Lut[b][i]];
      }
   }
}

void Monitor::RecomputeColors()
{
   gate_array_->buffered_ink_available_ = false;
   unsigned int p = gate_array_->pen_r_;
   if (gate_array_->ink_list_[p] != gate_array_->buffered_ink_)
   {
      // PLUS : Update the ASIC Palette
      // todo : if plus (otherwise : not needed !)
      //memory_->UpdateAsicPalette( p, m_pVGA->m_CachedInk);

      gate_array_->ink_list_[p] = gate_array_->buffered_ink_;

      // Look up table mode 0, 1, 2
      for (int b = 0; b < 0x100; ++b)
      {
         if (p == byte_to_pixel00_[b])
         {
            Mode0ExtendedLut[b][0] = gate_array_->ink_list_[byte_to_pixel00_[b]];
            Mode0ExtendedLut[b][1] = gate_array_->ink_list_[byte_to_pixel00_[b]];
            Mode0ExtendedLut[b][2] = gate_array_->ink_list_[byte_to_pixel00_[b]];
            Mode0ExtendedLut[b][3] = gate_array_->ink_list_[byte_to_pixel00_[b]];
         }
         if (p == byte_to_pixel01_[b])
         {
            Mode0ExtendedLut[b][4] = gate_array_->ink_list_[byte_to_pixel01_[b]];
            Mode0ExtendedLut[b][5] = gate_array_->ink_list_[byte_to_pixel01_[b]];
            Mode0ExtendedLut[b][6] = gate_array_->ink_list_[byte_to_pixel01_[b]];
            Mode0ExtendedLut[b][7] = gate_array_->ink_list_[byte_to_pixel01_[b]];
         }

         Mode1ExtendedLut[b][0] = gate_array_->ink_list_[Mode1Lut[b][0]];
         Mode1ExtendedLut[b][1] = gate_array_->ink_list_[Mode1Lut[b][1]];
         Mode1ExtendedLut[b][2] = gate_array_->ink_list_[Mode1Lut[b][2]];
         Mode1ExtendedLut[b][3] = gate_array_->ink_list_[Mode1Lut[b][3]];
         Mode1ExtendedLut[b][4] = gate_array_->ink_list_[Mode1Lut[b][4]];
         Mode1ExtendedLut[b][5] = gate_array_->ink_list_[Mode1Lut[b][5]];
         Mode1ExtendedLut[b][6] = gate_array_->ink_list_[Mode1Lut[b][6]];
         Mode1ExtendedLut[b][7] = gate_array_->ink_list_[Mode1Lut[b][7]];

         Mode2ExtendedLut[b][0] = gate_array_->ink_list_[Mode2Lut[b][0]];
         Mode2ExtendedLut[b][1] = gate_array_->ink_list_[Mode2Lut[b][1]];
         Mode2ExtendedLut[b][2] = gate_array_->ink_list_[Mode2Lut[b][2]];
         Mode2ExtendedLut[b][3] = gate_array_->ink_list_[Mode2Lut[b][3]];
         Mode2ExtendedLut[b][4] = gate_array_->ink_list_[Mode2Lut[b][4]];
         Mode2ExtendedLut[b][5] = gate_array_->ink_list_[Mode2Lut[b][5]];
         Mode2ExtendedLut[b][6] = gate_array_->ink_list_[Mode2Lut[b][6]];
         Mode2ExtendedLut[b][7] = gate_array_->ink_list_[Mode2Lut[b][7]];

      }

   }
}

void Monitor::Reset()
{
   // Nb us per
   res_x_ = 63 * 16;
   //   m_ResY = 312;

   x_ = 0;
   y_ = 0;

   expected_hbl_ = 0;

   tmp_sync_count_ = 0;

   hsync_found_ = false;
   hsync_count_ = 0;

   // 1024 pixels is :
   // 832 displaybale
   // 64 sync
   // 128 backproch


   // Line blanking = 12 us = 12*16 pixels (192)
   line_sync_ = 64;     // 4 us Line sync
   backporch_ = 128;      // 8 us backporch

   vertical_tolerate_ = 32;
   horizontal_total_ = res_x_ - (line_sync_ + backporch_)/*- m_OffsetX*/; // 768 !->832

   vertical_display_ = 288;
   vertical_sync_ = 8;
   
   if (screen_ != NULL)
   {
      // Also set the init
      screen_->StartSync();
      video_buffer_ = screen_->GetVideoBuffer(y_);

   }

   hsync_total_ = horizontal_total_;
   horizontal_synchronisation_ = 0;

   x_total_ = 832;
   offset_ = 0;

   horizontal_state_ = vertical_state_ = DISPLAY;

   horizontal_hold_count_ = 0;
   vertical_sync_start_ = 0;
   expected_vertical_sync_start_ = 286;

}


Monitor::~Monitor(void)
{
}


unsigned int Monitor::Tick( /*unsigned int nbTicks*/)
{

   START_CHRONO

      ///////////////////////////////////
      // Display : Monitor Part
   {
      x_ += NBPIXELADDED;
   /*
   The output HSYNC duration is actually MIN(0, MAX(4, (REG3 and #0f) - 2).
   The PLL in the monitor adjusts until the monitor HSYNC occurs in the centre of the HSYNC signal provided by the Gate Array.
   Therefore, if the REG3 value is #86 the HSYNC duration will be 4, the centre is 2 MODE 1 characters into the HSYNC,
   but if the value is #85 the HSYNC duration will be 3 and the centre is 1.5 MODE 1 characters into the HSYNC
   (ie. 0.5 MODE 1 characters earlier, resulting in the screen being shifted right by 1 MODE 2 character.
   */

   // Total count
   hsync_count_ += NBPIXELADDED;

   if (!gate_array_->hsync_)
   {
      // End ?
      if (hsync_found_)
      {
         if (horizontal_synchronisation_ >= 48)
         {
            if (hsync_total_ > 896 && hsync_total_ < 1024)
            {
               // Total length computation
               if (x_total_ != hsync_total_)
               {
                  //m_XTotal = m_HsyncTotal;

                  if (x_total_ < hsync_total_)
                  {
                     ++x_total_;
                  }
                  else if (x_total_ > hsync_total_)
                  {
                     --x_total_;
                  }
               }

               {
                  // Adjust : Where is exactly X ?
                  //

                  // Offset to synchronize Gate Array sync and real monitor sync
                  expected_hbl_ = x_ - (horizontal_synchronisation_ + line_sync_) / 2;

                  if (expected_hbl_ < 0)
                  {
                     expected_hbl_ += hsync_total_ + (line_sync_);
                  }

                  // HERE §!!!!!
                  //int offset = abs((line_sync_ - horizontal_synchronisation_) / 2);
                  int tot = (expected_hbl_ - hsync_total_);

                  if ((expected_hbl_ - hsync_total_ < 2)
                     && (expected_hbl_ - hsync_total_ > -2)
                     )
                  {
                     offset_ = 0;
                  }
                  else
                  {
                     if (tot < 0)
                     {
                        if (abs(tot) < ((hsync_total_ + (line_sync_) / 2) / 2))
                        {
                           offset_ = (int)sqrt((float)(hsync_total_ - expected_hbl_)) * -1;
                        }
                        else
                        {
                           offset_ = (int)sqrt((float)(hsync_total_ - expected_hbl_));
                        }
                     }
                     else
                     {
                        if (abs(tot) < ((hsync_total_ + (line_sync_) / 2) / 2))
                        {
                           offset_ = (int)sqrt((float)(expected_hbl_ - hsync_total_));
                        }
                        else
                        {
                           offset_ = (int)sqrt((float)(expected_hbl_ - hsync_total_)) * -1;
                        }
                     }
                  }
               }
            }
            else
            {
               offset_ = 0;
            }
            hsync_found_ = false;
            horizontal_synchronisation_ = 0;
         }
         else
         {
            hsync_found_ = false;
            // Forget about it....
            hsync_count_ = tmp_sync_count_ + horizontal_synchronisation_ - 1;
            horizontal_synchronisation_ = 0;
         }
      }
   }
   else
   {
      if (hsync_found_ == false)
      {
         tmp_sync_count_ = hsync_count_;
         // New total adjust

         int tmp = hsync_count_ - (line_sync_);
         if (tmp > 1088)
            tmp = tmp & 0x3ff;
         hsync_total_ = tmp;


         if (hsync_total_ < 0)
         {
            hsync_total_ += hsync_total_ + (line_sync_);
         }

         hsync_count_ = 0;
         hsync_found_ = true;
      }
      horizontal_synchronisation_ += NBPIXELADDED;
   }

   if (horizontal_state_ == DISPLAY)
   {
      if (x_ >= x_total_ + offset_)  // 832 (52 car)
      {
         // Set to sync
         horizontal_state_ = SYNC;
         horizontal_hold_count_ = line_sync_ - (x_ - (x_total_ + offset_));
      }
   }
   else
      if (horizontal_state_ == SYNC)
      {
         horizontal_hold_count_ -= NBPIXELADDED;
         if (horizontal_hold_count_ <= 0)
         {
            // Backporch
            x_ = 0 - horizontal_hold_count_;
            horizontal_hold_count_ += 16 * 8;
            horizontal_state_ = BACKPORCH;

            switch (vertical_state_)
            {
               case DISPLAY:
               {
                  ++y_;

                  if (!syncv_ && gate_array_->vsync_)
                  {
                     vertical_sync_start_ = y_;
                  }

                  if (((y_ >= expected_vertical_sync_start_) && (gate_array_->vsync_))     // Either capacitor is loaded enough AND a vsync is met
                     || (y_ >= expected_vertical_sync_start_ + 24 + vertical_tolerate_ + 24)                    // Or the capacitor overload
                     )
                  {
                     gate_array_->vsync_count_ = 0;
                     // Set to sync - The sawtooth generator fall, either by sync with the sync signal, or because of the capacitor overload
                     syncv_ = gate_array_->vsync_;
                     vertical_state_ = SYNC;
                  }
                  break;
               }

               case SYNC:
               {
                  ++y_;
                  ++gate_array_->vsync_count_;

                  if (!syncv_ && gate_array_->vsync_)
                  {  
                     vertical_sync_start_ = y_;
                     syncv_ = true;
                  }

                  if (gate_array_->vsync_count_ >= 8)
                  {
                     if (vertical_sync_start_ != 0 && syncv_)
                     {
                        // PLL Here : Compute difference between the two signals
                        // Then adjust the total by a fraction of it
                        if (vertical_sync_start_ < expected_vertical_sync_start_)
                        {
                           expected_vertical_sync_start_ -= (expected_vertical_sync_start_ - vertical_sync_start_) / 5;
                        }
                        else
                        {
                           expected_vertical_sync_start_ += (vertical_sync_start_ - expected_vertical_sync_start_) / 5;
                        }
                        if (expected_vertical_sync_start_ < 270)
                           expected_vertical_sync_start_ = 270;
                        if (expected_vertical_sync_start_ > 286)
                           expected_vertical_sync_start_ = 286;
                     }

                     screen_->VSync();

                     playback_sync_ = false;

                     vertical_sync_start_ = 0;

                     y_ = 0;
                     vertical_state_ = DISPLAY;
                     syncv_ = false;

                  }
                  break;
               }
            }

            int y = (((y_) % (RESY)));

            // Keep a track of what should be on the new line
            int* pixels_to_keep = video_buffer_ - x_;

            video_buffer_ = screen_->GetVideoBuffer(y);

            // Copie what's needed from
            if (y_ != 0)
            {
               memcpy(video_buffer_ ,pixels_to_keep, x_ * sizeof(int));
               video_buffer_ += x_;
            }
         }
      }
      else
      {
         horizontal_hold_count_ -= NBPIXELADDED;
         if (horizontal_hold_count_ < 0)
         {
            horizontal_state_ = DISPLAY;
         }
      }
   }


      if (!syncv_ && gate_array_->vsync_)
      {
         vertical_sync_start_ = y_;
      }


   if (!playback_sync_)
   {
      // SNR value !
      if (playback_)playback_->Playback();
      playback_sync_ = true;
   }

   STOP_CHRONO

      return this_tick_time_ = NBPIXELADDED / 4;

}


