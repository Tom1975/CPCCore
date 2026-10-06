#include "stdafx.h"
#include "CRTC.h"
#include "VGA.h"

///////////////////////////////////////////////////////////////
//
//
// CRTC 3/4 (ASIC 40489 / 40226) : the vertical counters, R6 and the VSYNC condition are all
// evaluated once per line, when C0 restarts at 0 ; register writes during the line are taken into
// account at the line end. C9 uses a magnitude comparator (C9 >= R9) and cannot overflow.
//

void CRTC::ClockTick34 ()
{
   ClockDispTmg();

   bool ff1_set = false;

   bool ff3_set = false;
   bool ff3_reset = false;

   bool ff4_set = false;
   bool ff4_reset = false;

   // Status 1:
   status1_ = 0xFF;

   // Clock tick
   if (hcc_ == registers_list_[0] )
   {
      hcc_ = 0;  // Reset to 0 at the next count


      // SPLT
      if (vcc_ == registers_list_[4] && vlc_ == registers_list_[9])
      {

         if (gate_array_->memory_->GetSPLT() && (vcc_ != 0 || vlc_ != 0))
         {
            unsigned char splt = gate_array_->memory_->GetSPLT();
            if (vcc_ == (splt >> 3)
               && vlc_ == (splt & 0x7))

            {
               bu_ = ssa_ = gate_array_->/*memory_->*/GetSSA();
               // Use when RCC = 1, VCC = 0
               shifted_ssa_ = true;
               ssa_ready_ = true;
            }
         }
      }
   }
   else
   {
      hcc_++;
      ma_++;
   }



   // Counter actions, all evaluated when C0 restarts at 0 (Compendium 10.3.4, 11.2.6, 12.5)
   if (hcc_ == 0 )
   {
      // Vertical sync width counter
      if (ff4_)    // CE
      {
         // C3h is a 4-bit counter : R3h=0 (or a fixed 16) ends the VSYNC after 16 lines
         scanline_vbl_ ++;
         scanline_vbl_ &= 0x0F;

         if (scanline_vbl_ == (vertical_sync_width_ & 0x0F))
         {
            scanline_vbl_ = 0;
            ff4_reset = true;
         }
      }

      // VMA is reloaded from VMA'
      ma_ = bu_;

      // ParitéFrame of the frame that ends : an even frame gets the additional interlace line, and the
      // VSYNC condition on the frame start (R7=0) is processed before the parity toggles (19.6.4, 19.7.3)
      const bool frame_even = even_field_;
      const bool interlace_line = InterlaceOn() && frame_even;

      bool new_frame = false;
      if (interlace_line_)
      {
         // End of the additional interlace line
         interlace_line_ = false;
         new_frame = true;
      }
      else if (adjust_)
      {
         // Additional lines : C4 stays at R4, C9 counts from 0 and is compared with R5 ; a R5 lower
         // than the next C9 ends the adjustment at once (11.2.6, 11.3.3)
         const unsigned char next_c9 = vlc_ + 1;
         if (next_c9 >= registers_list_[5])
         {
            if (interlace_line)
            {
               // The additional interlace line always has C9=0, C4 stays at R4 (19.6.4)
               adjust_ = false;
               interlace_line_ = true;
               vlc_ = 0;
            }
            else
            {
               new_frame = true;
            }
         }
         else
         {
            vlc_ = next_c9;
         }
      }
      else
      {
         // End of a character : magnitude comparator C9 >= R9, C9 cannot overflow (10.3.4).
         // In Interlace Video Mode C9 carries ParitéC9 and counts by 2 (19.8.4)
         const bool interlace_video = InterlaceVideo();
         if (vlc_ >= registers_list_[9])
         {
            vlc_ = 0;
            if (vcc_ == registers_list_[4])
            {
               // Last character : C4 is not incremented by the additional management (12.5)
               if (registers_list_[5] != 0)
                  adjust_ = true;
               else if (interlace_line)
                  interlace_line_ = true;
               else
                  new_frame = true;
            }
            else
            {
               // C4 == R4 is an equality : a R4 lower than C4 makes C4 overflow (12.5)
               vcc_ = (vcc_ + 1) & 0x7F;
               // ParitéC9 toggles on each C4 when R9 is odd (19.5.5)
               if (registers_list_[9] & 1)
                  parity_c9_ = !parity_c9_;
               if (interlace_video)
                  vlc_ = parity_c9_ ? 1 : 0;
            }
         }
         else if (interlace_video)
         {
            vlc_ = ((vlc_ + 2) | (parity_c9_ ? 1 : 0)) & 0x1F;
         }
         else
         {
            vlc_ = (vlc_ + 1) & 0x1F;
         }
      }

      if (new_frame)
      {
         vlc_ = 0;
         vcc_ = 0;
         adjust_ = false;

         ma_ = registers_list_[13] + ((registers_list_[12] & 0x3F) << 8);
         bu_ = ma_;

         ff3_set = true;
         frame_counter_ = (frame_counter_ + 1) & 0x1F;

         // Next frame : ParitéFrame toggles, ParitéC9 starts from it (19.5.5)
         even_field_ = !even_field_;
         parity_c9_ = !even_field_;
         if (InterlaceVideo())
            vlc_ = parity_c9_ ? 1 : 0;
      }

      // R6 is only tested when C0 restarts at 0 (18.2.4)
      if (vcc_ == registers_list_[6])
      {
         ff3_reset = true;
      }

      // VSYNC only when C4==R7 on C9=C0=0, with no re-entrance protection : it starts again if
      // the condition is still true when it ends (16.3, 16.4.4)
      if (vcc_ == registers_list_[7] && vlc_ == 0 && (!ff4_ || ff4_reset))
      {
         // MID-VSYNC on an even frame in interlace (19.7.3)
         if (InterlaceOn() && frame_even)
            vsync_mid_pending_ = true;
         else
            ff4_set = true;
      }
   }
   if (ClockMidVSync())
   {
      ff4_set = true;
   }


   if ( hcc_ == registers_list_[1] )
   {
      unsigned char splt = gate_array_->memory_->GetSPLT();
      if (gate_array_->memory_->GetSPLT() && (vcc_ != registers_list_[4] || vlc_ != registers_list_[9]) &&
         (vcc_ == (splt >> 3)
            && vlc_ == (splt & 0x7)))
      {
         bu_ = gate_array_->GetSSA();
      }
      else
      {
         // VMA' is not updated during the additional lines (11.2.6)
         int vertical_shift = (gate_array_->memory_->GetSSCR()&0x7F) >> 4;
         if ( !adjust_ && !interlace_line_ && ((vlc_ + vertical_shift) &0x7)== ((registers_list_[9] ) & 0x7))
         {
            bu_ = ma_;
         }
      }
   }

   // Flip flop computations
   if (hcc_ == 0)
   {
      ff1_set = true;
      if (gate_array_->memory_->GetSSCR() & 0x80)
      {
         sscr_bit_8_ = 0;
      }
      else
      {
         sscr_bit_8_ = 1;
      }
   }
   else if (hcc_ > 1)
   {
      sscr_bit_8_ = 1;
   }

   const bool hsync_was_active = signals_->h_sync_;
   bool hsync_started, hsync_ended;
   ClockHSync(hsync_started, hsync_ended);
   if (hsync_started)
   {
      // Bit 3	0 : CRTC Horizontal Count == Horizontal Sync Position(Reg 2)
      status1_ &= ~0x08;
   }

   signals_->h_sync_on_begining_of_line_ = ((hcc_ == 0) && (hsync_was_active || hsync_started));

   if (hsync_ended)
   {
      // Bit 4	0 : CRTC is on last char of HSYNC
      status1_ &= ~0x10;
   }

   if (hcc_ == registers_list_[1])
   {
      //ff1_reset = true;
      // Bit 2	0 : CRTC Horizontal Count == Horizontal Displayed(Reg 1)
      //status1_ &= ~0x04;
      if (!ff1_set)
      {
         ff1_ = false;
      }
      else 
      {
         // Nothing .
         ff1_ = false;// !ff1_reset;
      }
   }
   else if(ff1_set)
   {
      ff1_ = true;
   }

   // The R6 border wins over the new frame (18.2.4)
   if (ff3_reset)
   {
      ff3_ = false;
   }
   else if (ff3_set)
   {
      ff3_ = true;
   }
   // Flip flop computation
   if ( ff4_reset && !ff4_set)
   {
      ff4_ = false;
   }
   else
   {
      if (!ff4_reset && ff4_set)
      {
         ff4_ = true;
         if ((scanline_vbl_ + 1 == vertical_sync_width_)) status1_ &= ~0x20;
      }
      else if (ff4_reset && ff4_set)
      {
         // Nothing .
         int dbg = 1;
         if (ff4_ && (scanline_vbl_ + 1 == vertical_sync_width_)) status1_ &= ~0x20;
      }
      
   }

}
