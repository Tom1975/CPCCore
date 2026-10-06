#include "stdafx.h"
#include "CRTC.h"
#include "VGA.h"

///////////////////////////////////////////////////////////////
//
// CRTC 1 (UM6845R) - Compendium 10.3.2, 11.2.4, 11.3.2, 11.6, 12.3, 13.3, 16.4.2, 17.4.2, 18.3.3, 21.3.3
//
// One call is one character clock edge. Unlike CRTC 0, this CRTC compares its counters with the
// registers continuously : register writes are taken into account at the line end.
// - last_line_          C4==R4 && C9==R9, sampled when C0 reaches R0 : from there the frame end (or
//                       the additional management) cannot be cancelled any more (11.2.4).
// - adjust_             additional management : a dedicated C5 counter (vertical_adjust_counter_)
//                       counts the R5 lines, while C9 keeps counting with R9 and increments C4 whatever
//                       R4. R5=0 never ends it (11.3.2).
// - vma_reload_         VMA is loaded from R12/R13 at each line start instead of VMA' : set by a new
//                       frame or a R.F.D., cleared at the line end after C0==R1 && C9==R9 (or C9==R9 alone
//                       when R1>R0), unless C4==R4 at that line end (17.4.2, 11.2.4, 11.6).
// - status_border_r6_   status bit 5, updated at the line end (21.3.3).
// Not yet handled : the frame parity used by the R.F.D. in the C9==R9 test on C0==R1 (11.6.1, 19.5.3).
//
///////////////////////////////////////////////////////////////

void CRTC::ClockTick1 ()
{
   bool ff1_set = false;
   bool ff1_reset = false;

   bool ff3_set = false;
   bool ff3_reset = false;

   bool ff4_set = false;
   bool ff4_reset = false;

   const bool interlace_video = ((registers_list_[8] & 0x3) == 0x3);

   // Clock tick
   const bool c0_reset = (hcc_ == registers_list_[0]);
   if (c0_reset)
   {
      hcc_ = 0;
   }
   else
   {
      hcc_++;
      ma_++;
   }

   if (c0_reset)
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

      // Comparators on the line that ends, with the registers written during the line
      const bool c9_eq_r9 = C9EqualsR9();
      const bool c4_eq_r4 = (vcc_ == registers_list_[4]);

      bool new_frame = false;
      if (adjust_)
      {
         // C5 counts the additional lines ; R5=0 does not end them (11.3.2)
         vertical_adjust_counter_ = (vertical_adjust_counter_ + 1) & 0x1F;
         new_frame = (registers_list_[5] != 0 && vertical_adjust_counter_ == registers_list_[5]);
      }
      else if (last_line_)
      {
         if (registers_list_[5] != 0)
         {
            adjust_ = true;
            vertical_adjust_counter_ = 0;
         }
         else
         {
            new_frame = true;
         }
      }

      if (!new_frame)
      {
         // C9 returns to 0 on R9 and increments C4, whatever R4 (C4 may overflow, 12.3)
         if (c9_eq_r9)
         {
            vlc_ = 0;
            vcc_ = (vcc_ + 1) & 0x7F;
         }
         else
         {
            vlc_ = (vlc_ + (interlace_video ? 2 : 1)) & 0x1F;
         }
      }

      // VMA reload state (17.4.2, 11.6)
      if (rfd_)
      {
         vma_reload_ = true;
         rfd_ = false;
      }
      if ((vma_reload_clear_ || (registers_list_[1] > registers_list_[0] && c9_eq_r9)) && !c4_eq_r4)
      {
         vma_reload_ = false;
      }
      vma_reload_clear_ = false;

      if (new_frame)
      {
         vlc_ = 0;
         vcc_ = 0;
         adjust_ = false;
         vma_reload_ = true;
         ff3_set = true;

         // Next frame
         even_field_ = !even_field_;
      }

      ma_ = vma_reload_ ? (registers_list_[13] + ((registers_list_[12] & 0x3F) << 8)) : bu_;

      // Status bit 5 : BORDER R6 state (21.3.3)
      if (vlc_ == 0 && vcc_ == 0)
      {
         status_border_r6_ = false;
      }
      if (vlc_ == 0 && vcc_ == registers_list_[6])
      {
         status_border_r6_ = true;
      }
   }

   // Last line, sampled when C0 reaches R0
   if (hcc_ == registers_list_[0])
   {
      last_line_ = (vcc_ == registers_list_[4]) && C9EqualsR9();
   }

   // VMA' is updated when C0 reaches R1 on the last line of a character (17)
   if ( hcc_ == registers_list_[1] && vlc_ == registers_list_ [9] )
   {
      bu_ = ma_;
      vma_reload_clear_ = true;
   }

   // VSYNC (16.3, 16.4.2) : R7 is ignored during a VSYNC ; a new one needs the C4==R7 equality to change
   if (vcc_ == registers_list_[7])
   {
      if ( v_no_sync_ && (!ff4_ || ff4_reset))
      {
         ff4_set = true;

         v_no_sync_ = false;
      }
   }
   else
   {
      v_no_sync_ = true;
   }


   if (vcc_ == registers_list_[6])
   {
      ff3_reset = true;
   }

   // Flip flop computations
   if (hcc_ == 0)
   {
       ff1_set = true;
   }

   bool hsync_started, hsync_ended;
   ClockHSync(hsync_started, hsync_ended);

   if (hcc_ == registers_list_[1])
   {
      ff1_reset = true;
   }

   // Flip flop computation
   if ( ff1_reset && !ff1_set)
   {
      ff1_ = false;
   }
   else if ( !ff1_reset && ff1_set)
   {
      ff1_ = true;
   }
   else if ( ff1_reset && ff1_set)
   {
      ff1_ = false;
   }

   if ( ff3_reset && !ff3_set)
   {
      ff3_ = false;
   }
   else if ( !ff3_reset && ff3_set)
   {
      ff3_ = true;
   }

   if ( ff4_reset && !ff4_set)
   {
      ff4_ = false;
   }
   else if ( !ff4_reset && ff4_set)
   {
      ff4_ = true;
   }
}
