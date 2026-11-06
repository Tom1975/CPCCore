#include "stdafx.h"
#include "Monitor.h"

#include "VGA.h"


#include <cmath>

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


// Horizontal deflection of the CTM644 (service manual of the CPC664 : IC401 is a Sanyo LA7800, sync
// separator and line oscillator). The line oscillator is locked to the C-HSYNC by an AFC loop :
//  - comparison signal : a sawtooth built by integrating the line flyback pulse. It rises during the
//    flyback (12 us) and falls back during the trace (52 us). Its rising edge crosses zero at the
//    position where the centre of the C-HSYNC settles (ACCC 14.4 : a C-HSYNC 1 us shorter moves the
//    picture by 0.5 us, the loop follows the centre of the pulse).
//  - phase comparator : while the C-HSYNC is present, it passes the sawtooth value to the loop filter
//    (gated current). Every pulse counts, wherever it falls on the line : a second C-HSYNC in the middle
//    of the line (R2 or R3 updated during the HSYNC, ACCC 15.3) brings the value of the trace part of
//    the sawtooth, and the loop moves the picture until the main pulse compensates for it (ACCC 15.7 :
//    the monitor takes several lines to settle). A short pulse brings little charge : with R3l=2 the
//    monitor hardly sees it (ACCC 14.4).
//  - loop filter (lag-lead network) : a proportional part that moves the phase of the next flyback,
//    and an integrating part (the capacitor) that holds the oscillator frequency.
// The charge brought during a line is sampled at the start of the next flyback.
static const int kNominalLine = 1024;      // 64 us : free running line (H-Hold set to 15625 Hz)
static const int kFlyback = 192;           // 12 us : line sync + back porch
static const double kLockCentre = 36.0;    // C-HSYNC centre in the locked state, in pixels after the flyback start (H-phase)
static const double kLoopProportional = 0.3;
static const double kLoopIntegral = 0.01;
static const double kLockRange = 64.0;     // Pull-in range of the oscillator, in pixels per line

// Sawtooth of the AFC comparison, as a function of the position in the line (pixels since the flyback start)
static double AfcSawtooth(double position)
{
   // Value at the centre of the pixel
   double u = std::fmod(position + 0.5 - (kLockCentre - kFlyback / 2.0), (double)kNominalLine);
   if (u < 0) u += kNominalLine;
   if (u < kFlyback)
      return -1.0 + 2.0 * u / kFlyback;
   return 1.0 - 2.0 * (u - kFlyback) / (kNominalLine - kFlyback);
}

Monitor::Monitor(void) : memory_(0), playback_sync_(false),keyboard_(nullptr), horizontal_position_(0)
{
   playback_ = nullptr;
   int i, b;


   video_buffer_ = nullptr;
   screen_ = nullptr;


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
   unsigned int p = gate_array_->buffered_pen_;
   if ((p & 0x10) != 0)
   {
      // Pen 16 : the border register (9.2)
      for (int i = 0; i < NB_BYTE_BORDER; i++) gate_array_->video_border_[i] = gate_array_->buffered_ink_ | 0xFF000000;
      gate_array_->border_reg_ = gate_array_->buffered_ink_reg_;
      return;
   }
   if (gate_array_->ink_list_[p] != gate_array_->buffered_ink_)
   {
      // PLUS : Update the ASIC Palette
      // todo : if plus (otherwise : not needed !)
      //memory_->UpdateAsicPalette( p, m_pVGA->m_CachedInk);

      gate_array_->ink_list_[p] = gate_array_->buffered_ink_ | 0xFF000000;
      gate_array_->ink_regs_[p] = gate_array_->buffered_ink_reg_;

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

   x_total_ = kNominalLine - line_sync_;
   next_flyback_x_ = x_total_;
   flyback_pending_ = true;
   line_position_ = 0;
   afc_charge_ = 0;
   afc_integral_ = 0;
   line_fraction_ = 0;
   flyback_phase_ = 0;

   horizontal_state_ = vertical_state_ = DISPLAY;

   horizontal_hold_count_ = 0;
   vertical_sync_start_ = 0;
   expected_vertical_sync_start_ = 286;

}


Monitor::~Monitor(void)
{
}

void Monitor::FlybackStart()
{
   // Charge of the line, as a phase error in pixels : a 64 pixels pulse whose centre is 'd' pixels late
   // brings 64 * 2d / kFlyback
   const double error = afc_charge_ * kFlyback / (2.0 * 64);
   afc_charge_ = 0;
   afc_integral_ += kLoopIntegral * error;
   if (afc_integral_ > kLockRange) afc_integral_ = kLockRange;
   if (afc_integral_ < -kLockRange) afc_integral_ = -kLockRange;
   double period = kNominalLine + afc_integral_ + kLoopProportional * error;
   if (period < kNominalLine - kLockRange) period = kNominalLine - kLockRange;
   if (period > kNominalLine + kLockRange) period = kNominalLine + kLockRange;
   // The flyback can only start on a pixel, the nearest one to its true start : the true start of this
   // flyback is 'flyback_phase_' pixels (-0.5 to 0.5) after the pixel where it starts, the comparator
   // works on the true position
   flyback_phase_ = line_fraction_;
   line_fraction_ += period;
   const int whole = (int)std::floor(line_fraction_ + 0.5);
   line_fraction_ -= whole;
   // The beam position x_ restarts at the end of the line sync : the next flyback starts at period - line sync
   next_flyback_x_ = whole - line_sync_;
   flyback_pending_ = false;
   line_position_ = 0;
}

void Monitor::Tick( )
{

   START_CHRONO

      ///////////////////////////////////
      // Display : Monitor Part
   {
      x_ += NBPIXELADDED;

   // AFC phase comparator, pixel by pixel on the C-HSYNC of the GATE ARRAY (16.2.2)
   const unsigned short chsync_mask = gate_array_->chsync_mask_;
   for (int pixel = 0; pixel < NBPIXELADDED; ++pixel)
   {
      // Flyback start : the beam reaches x_total_ (it can be the first pixel of the next block, already in SYNC)
      if (flyback_pending_ && (horizontal_state_ != DISPLAY || x_ - NBPIXELADDED + pixel >= x_total_))
      {
         FlybackStart();
      }
      if ((chsync_mask >> pixel) & 1)
      {
         afc_charge_ += AfcSawtooth(line_position_ - flyback_phase_ - horizontal_position_);
      }
      ++line_position_;
   }

   if (horizontal_state_ == DISPLAY)
   {
      if (x_ >= x_total_)  // 832 (52 car)
      {
         // Set to sync
         horizontal_state_ = SYNC;
         horizontal_hold_count_ = line_sync_ - (x_ - x_total_);
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

                     // Keyboard validation
                     if (keyboard_) keyboard_->ValidateKeyboardMap();
                     
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
            // The AFC loop has computed the next flyback during this flyback (FlybackStart)
            x_total_ = next_flyback_x_;
            flyback_pending_ = true;
         }
      }
   }


      if (!syncv_ && gate_array_->vsync_)
      {
         vertical_sync_start_ = y_;
      }


   if (!playback_sync_)
   {
      START_CHRONO

      // SNR value !
      if (playback_)playback_->Playback();
      playback_sync_ = true;

      STOP_CHRONO
   }

   

//      return this_tick_time_ = NBPIXELADDED / 4;

}


