#include "stdafx.h"
#include "Monitor.h"

#include "VGA.h"


#include <cmath>

#define NBPIXELADDED 16



#define DISP(c) *(m_VideoBuffer)/*[m_X]*/ = c

unsigned char Mode0Lut[0x100][0x10];
unsigned char Mode1Lut[0x100][0x10];
unsigned char Mode2Lut[0x100][0x10];

unsigned int Mode0ExtendedLut[0x100][0x8];
unsigned int Mode1ExtendedLut[0x100][0x8];
unsigned int Mode2ExtendedLut[0x100][0x8];
unsigned int Mode3ExtendedLut[0x100][0x8];


// Horizontal deflection of the CTM644 (service manual of the CPC6128 / CTM644, page 26 : IC401 is a Sanyo
// LA7800, sync separator and line oscillator). The line oscillator is locked to the C-HSYNC by an AFC loop.
//
// PROVISIONAL : the structure comes from the schematic, the values the schematic does not give come from
// the photos of real CPCs, fitted together with one H-HOLD per monitor (afc_study/best_t5joint.json) :
// SHAKER 2.7 D3 "2 CSYNC / LINE STUDIES" (44 screens, 3.25 pixels RMS) and D8 "NO HSYNC FOR XX LINES"
// (1.74), and the 8 screens of RealCpc T5 on another CTM644 (7.15, its H-HOLD +4.8). Nothing was measured
// on a CTM644 : RealCpc/README.md, part 1 bis. Study : UnitTests/res/Shaker/afc_study.
//
//  - pin 16, comparison signal : the line flyback pulse integrated by R406 (6.2 k) and C404 (39 nF), a
//    sawtooth that rises during the flyback (12 us) and falls back during the trace (52 us), plus the part
//    of the pulse that C403 (180 pF) / C404 pass (a step).
//  - phase comparator : a differential pair switched on by the C-HSYNC. Its output current follows
//    tanh((sawtooth - reference) / kComparatorRange) : all or nothing as soon as the sawtooth is away from
//    the reference, proportional only around it. Every pulse counts, wherever it falls on the line : a
//    second C-HSYNC in the middle of the line (R2 or R3 updated during the HSYNC, ACCC 15.3) brings a
//    current set by its width, and the picture moves until the main pulse compensates for it (ACCC 15.7).
//    The side that sources the current is stronger than the side that sinks it (kSourceRatio).
//  - pin 1, loop filter : the current goes into C413 (10 nF), which shares it with C414 (1 uF) through
//    R418 (8.2 k) ; R417 (56 k) leaks it towards pin 2. The photos T5 need a leak of about 25 k : another
//    path of about 45 k, not on the schematic (input of pin 2 of the LA7800 ?), PROVISIONAL.
//  - pin 2, oscillator : a ramp (C415, 5.6 nF, charged through the H-HOLD network) whose slope follows the
//    voltage of pin 1. The line flyback starts when the ramp ends. A current pulse moves the end of the
//    ramp all the more that it comes late in the ramp (relaxation oscillator), kRampMemory.
// Units : pixel (62.5 ns) ; charge = sink current of the comparator during one pixel ; C413 = 1.
static const int kNominalLine = 1024;            // 64 us
static const int kFlyback = 192;                 // 12 us : line sync + back porch
static const double kSawtoothStep = 0.23;        // C403 / C404 part of the flyback pulse (sawtooth = +-1)
static const double kComparatorReference = 0.06679;
static const double kComparatorRange = 0.09545;  // linear zone of the differential pair
static const double kSourceRatio = 1.41842;      // source current / sink current
static const double kFilterShare = 1.0 / 1312.0; // 1 / (R418 x C413)
static const double kFilterRatio = 100.0;        // C414 / C413
static const double kFilterLeak = 1.0 / 3950.6;  // 1 / (R x C413), R about 25 k : R417 (56 k) is not enough (T5)
static const double kOscillatorGain = 0.09676;   // pixels of line period per unit of pin 1 voltage
static const double kRampMemory = 2027.8;        // pixels
static const double kRampStart = 132.35;         // start of the ramp, in pixels after the flyback start
static const double kLockRange = 64.0;           // widest period change of the oscillator, in pixels per line
// The beam flyback of the picture starts this number of pixels after the flyback start of the model : it
// puts the standard picture at its place (horizontal phase of the monitor)
static const int kPictureDelay = 43;

// Sawtooth of pin 16, as a function of the position in the line (pixels since the flyback start), minus
// the reference of the comparator
static double AfcSawtooth(double u)
{
   const double value = (u < kFlyback) ? -1.0 + 2.0 * u / kFlyback + kSawtoothStep
                                       : 1.0 - 2.0 * (u - kFlyback) / (kNominalLine - kFlyback);
   return value - (kSawtoothStep * kFlyback / kNominalLine + kComparatorReference);
}

// Charge the comparator brings to pin 1 during one pixel of C-HSYNC, 'u' pixels after the flyback start
static double AfcCharge(double u)
{
   u = std::fmod(u, (double)kNominalLine);
   if (u < 0) u += kNominalLine;
   double current = std::tanh(AfcSawtooth(u) / kComparatorRange);
   if (current > 0) current *= kSourceRatio;
   // Relaxation oscillator : weight of the pulse by the time left before the end of the ramp
   double ramp = u - kRampStart;
   if (ramp < 0) ramp += kNominalLine;
   static const double norm = (kNominalLine / kRampMemory) / (1.0 - std::exp(-kNominalLine / kRampMemory));
   return current * std::exp((ramp - kNominalLine) / kRampMemory) * norm;
}

// Loop filter over 'pixels' pixels : exponential of the matrix of the network (C413, R418 + C414, R417)
struct AfcFilterStep
{
   double m[2][2];
   explicit AfcFilterStep(double pixels)
   {
      const double a[2][2] = { { -(kFilterShare + kFilterLeak) * pixels, kFilterShare * pixels },
                               { kFilterShare / kFilterRatio * pixels, -kFilterShare / kFilterRatio * pixels } };
      double term[2][2] = { { 1, 0 }, { 0, 1 } };
      m[0][0] = m[1][1] = 1; m[0][1] = m[1][0] = 0;
      for (int n = 1; n < 12; n++)
      {
         double next[2][2];
         for (int i = 0; i < 2; i++)
            for (int j = 0; j < 2; j++)
               next[i][j] = (term[i][0] * a[0][j] + term[i][1] * a[1][j]) / n;
         for (int i = 0; i < 2; i++)
            for (int j = 0; j < 2; j++)
            {
               term[i][j] = next[i][j];
               m[i][j] += next[i][j];
            }
      }
   }
};
static const AfcFilterStep kFilterPixel(1.0);
static const AfcFilterStep kFilterBlock(NBPIXELADDED);

Monitor::Monitor(void) : horizontal_hold_(kStandardHorizontalHold), horizontal_position_(0),memory_(0), keyboard_(nullptr), playback_sync_(false)
{
   playback_ = nullptr;
   int i, b;


   video_buffer_ = nullptr;
   screen_ = nullptr;
   frame_pacer_ = nullptr;
   elapsed_us_ = 0;


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

   x_total_ = kNoFlyback;
   flyback_asked_ = false;
   line_position_ = 0;
   // Locked state : the voltage of pin 1 that brings the oscillator back to 64 us
   afc_pin1_ = afc_c414_ = -horizontal_hold_ / kOscillatorGain;
   oscillator_ramp_ = 0;
   flyback_phase_ = 0;

   horizontal_state_ = vertical_state_ = DISPLAY;

   horizontal_hold_count_ = 0;
   vertical_sync_start_ = 0;
   expected_vertical_sync_start_ = 286;

}


Monitor::~Monitor(void)
{
}

// Line period of the oscillator for a voltage of pin 1, in pixels
double Monitor::LinePeriod() const
{
   double period = kNominalLine + horizontal_hold_ + kOscillatorGain * afc_pin1_;
   if (period < kNominalLine - kLockRange) period = kNominalLine - kLockRange;
   if (period > kNominalLine + kLockRange) period = kNominalLine + kLockRange;
   return period;
}

void Monitor::Tick( )
{


      ///////////////////////////////////
      // Display : Monitor Part
   {
      x_ += NBPIXELADDED;
      ++elapsed_us_;

   // AFC loop, on the C-HSYNC of the GATE ARRAY (16.2.2)
   const unsigned short chsync_mask = gate_array_->chsync_mask_;
   const double block_ramp = NBPIXELADDED / LinePeriod();
   if (chsync_mask == 0 && oscillator_ramp_ + block_ramp < 0.999)
   {
      // No C-HSYNC and no end of ramp in these pixels : the filter and the ramp in one step
      const double pin1 = kFilterBlock.m[0][0] * afc_pin1_ + kFilterBlock.m[0][1] * afc_c414_;
      afc_c414_ = kFilterBlock.m[1][0] * afc_pin1_ + kFilterBlock.m[1][1] * afc_c414_;
      afc_pin1_ = pin1;
      oscillator_ramp_ += block_ramp;
      line_position_ += NBPIXELADDED;
   }
   else
   {
      for (int pixel = 0; pixel < NBPIXELADDED; ++pixel)
      {
         if ((chsync_mask >> pixel) & 1)
         {
            // Comparator : the true flyback start is 'flyback_phase_' pixels before the pixel 0 of the line
            afc_pin1_ += AfcCharge(line_position_ + 0.5 + flyback_phase_ - horizontal_position_);
         }
         const double pin1 = kFilterPixel.m[0][0] * afc_pin1_ + kFilterPixel.m[0][1] * afc_c414_;
         afc_c414_ = kFilterPixel.m[1][0] * afc_pin1_ + kFilterPixel.m[1][1] * afc_c414_;
         afc_pin1_ = pin1;
         const double period = LinePeriod();
         oscillator_ramp_ += 1.0 / period;
         ++line_position_;
         if (oscillator_ramp_ >= 1.0)
         {
            // End of the ramp : flyback start. The ramp restarts ; the part already done is the fraction
            // of pixel since the true end.
            oscillator_ramp_ -= 1.0;
            flyback_phase_ = oscillator_ramp_ * period;
            line_position_ = 0;
            // Beam : the flyback of the picture, on the pixel after
            if (horizontal_state_ == DISPLAY)
               x_total_ = x_ - NBPIXELADDED + pixel + 1 + kPictureDelay;
            else
               flyback_asked_ = true;   // the beam is still in its previous flyback
         }
      }
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

                  if (!syncv_ && gate_array_->c_vsync_)
                  {
                     vertical_sync_start_ = y_;
                  }

                  if (((y_ >= expected_vertical_sync_start_) && (gate_array_->c_vsync_))     // Either capacitor is loaded enough AND a vsync is met
                     || (y_ >= expected_vertical_sync_start_ + 24 + vertical_tolerate_ + 24)                    // Or the capacitor overload
                     )
                  {
                     gate_array_->vsync_count_ = 0;
                     // Set to sync - The sawtooth generator fall, either by sync with the sync signal, or because of the capacitor overload
                     syncv_ = gate_array_->c_vsync_;
                     vertical_state_ = SYNC;
                  }
                  break;
               }

               case SYNC:
               {
                  ++y_;
                  ++gate_array_->vsync_count_;

                  if (!syncv_ && gate_array_->c_vsync_)
                  {  
                     vertical_sync_start_ = y_;
                     syncv_ = true;
                  }

                  // Lines from the C-VSYNC to the first line of the emulated window. The C-VSYNC comes about 2 lines
                  // after the CRTC VSYNC (end of the 2nd HSYNC, 16.2.3) : 6 lines keep the window where it was when
                  // the flyback started on the CRTC VSYNC (8 lines). Calibration of the window, not a measure.
                  if (gate_array_->vsync_count_ >= 6)
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

                     // The frame leaves when it is due in real time, not when the emulation reaches it
                     const unsigned int frame_us = TakeElapsedTime();
                     if (frame_pacer_ != nullptr && frame_pacer_->IsActive())
                     {
                        frame_pacer_->FrameEnd(frame_us);
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
               default:
                  break;
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
            // The beam goes on until the oscillator ends its ramp (unless it already did)
            x_total_ = flyback_asked_ ? 0 : kNoFlyback;
            flyback_asked_ = false;
         }
      }
   }


      if (!syncv_ && gate_array_->c_vsync_)
      {
         vertical_sync_start_ = y_;
      }


   if (!playback_sync_)
   {

      // SNR value !
      if (playback_)playback_->Playback();
      playback_sync_ = true;

   }

   

//      return this_tick_time_ = NBPIXELADDED / 4;

}


