#include "stdafx.h"
#include "CRTC.h"
#include "VGA.h"

///////////////////////////////////////////////////////////////
//
// CRTC 0 (HD6845S / UM6845) - Compendium 10.3.1, 11.2.2, 11.3.1, 12.2, 13.2, 13.7.2, 16.3, 16.4.1
//
// One call is one character clock edge : it ends the character C0 = prev and starts the next one.
// Register writes done since the previous call landed during the character prev.
//
// This CRTC does not evaluate its vertical counters continuously : comparators are sampled by
// latches at fixed positions of C0, and the latches drive the counters on the next line end.
// - line_end_         registered C0==R0, sampled at the start of a character. It drives C4 and the frame
//                     logic on the next edge, while C0 itself is reset by the live C0==R0 comparator :
//                     R0 written during the last character resets C0 with its new value, but the line end
//                     has already been registered (13.7.2).
// - c4_increment_     registered C9==R9, sampled at the start of each character while C9 is handled : the
//                     value seen at the start of the last character increments C4. C9 itself is compared
//                     with R9 on the C0 reset : R9 written on C0==R0 increments both C4 and C9 (10.3.1).
// - c9_managed_       C9 handling, enabled when C0 reaches 1, disabled when C0 restarts at 0. With R0=0
//                     C0 never reaches 1 : C9, C3h and the C4 increment latch freeze (13.2.4, 13.2.6).
// - last_line_        C4==R4 && C9==R9, sampled at the end of characters 0 and 1 only (12.2).
// - adjust_           additional management, armed as soon as last_line_ is sampled true. At the end of
//                     character 2 it is cancelled if it is the last line and no line is to be added
//                     (R5=0), confirmed otherwise (13.2.1). With R0<2 it is never cancelled (13.2.5).
// - adjust_confirmed_ C9 no longer returns to 0 on R9 (11.2.2). Once C4 differs from R4, C9 is compared
//                     with R5 instead of R9 and C4 is kept.
// - adjust_end_       next C9 == R5, sampled at the end of characters 0 to 2 : R5 written later on the
//                     line is not taken into account (11.3.1, 11.4.2).
// - vsync_allowed_    set at the end of character 2, cleared on the C0 reset : C4==R7 starts a VSYNC only
//                     if C0 reached 2 since (13.2.2, 16.4.1).
//
///////////////////////////////////////////////////////////////

void CRTC::ClockTick0 ()
{
   ClockDispTmg();

   bool ff1_set = false;
   bool ff1_reset = false;

   bool ff4_set = false;
   bool ff4_reset = false;

   const unsigned char prev = hcc_;

   // Comparators on the character that ends
   const bool c9_eq_r9 = C9EqualsR9();
   // Additional management, once C4 differs from R4 : C9 is compared with R5 (11.2.2)
   const bool r5_mode = adjust_ && (vcc_ != registers_list_[4]);

   ///////////////////////////////
   // Line end : C4 and frame
   // Returning C4 and C9 to 0 is part of the C9 handling : with R0=0 it is frozen (13.2.3)
   bool new_frame = false;
   if (line_end_)
   {
      if (last_line_ && !adjust_)
      {
         new_frame = c9_managed_;
      }
      else if (r5_mode)
      {
         // C4 is kept : the adjustment ends when the next C9 reaches R5
         new_frame = c9_managed_ && adjust_end_;
      }
      else if (c4_increment_)
      {
         vcc_ = (vcc_ + 1) & 0x7F;
      }
      c4_increment_ = false;
   }

   ///////////////////////////////
   // C0
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
      if (c9_managed_)
      {
         // C3h counts the VSYNC lines on the C0 reset (14.2)
         if (ff4_)
         {
            if (c3h_load_)
            {
               // VSYNC started during the previous line : C3h starts here (16.4.1.1)
               scanline_vbl_ = 0;
               c3h_load_ = false;
            }
            else
            {
               // C3h is a 4-bit counter : R3h=0 ends the VSYNC after 16 lines
               scanline_vbl_ = (scanline_vbl_ + 1) & 0x0F;
               if (scanline_vbl_ == (vertical_sync_width_ & 0x0F))
               {
                  scanline_vbl_ = 0;
                  ff4_reset = true;
               }
            }
         }

         // C9 : returns to 0 on R9, except in a confirmed additional management compared with R5
         if (c9_eq_r9 && !(r5_mode && adjust_confirmed_))
            vlc_ = 0;
         else
            vlc_ = (vlc_ + 1) & 0x1F;
      }

      // VMA is reloaded from VMA'
      ma_ = bu_;

      // The IVM state of the address is taken when C0 restarts at 0 (19.8.1)
      ivm_latched_ = InterlaceVideo();

      // IVM VSYNC delayed by one line (19.5.2)
      if (vsync_line_delay_)
      {
         vsync_line_delay_ = false;
         ff4_set = true;
      }
   }

   if (new_frame)
   {
      vcc_ = 0;
      vlc_ = 0;
      adjust_ = false;
      adjust_confirmed_ = false;

      ma_ = registers_list_[13] + ((registers_list_[12] & 0x3F) << 8);
      bu_ = ma_;

      // Next frame : ParitéFrame = ParitéR6 (19.5.2)
      even_field_ = !parity_r6_;
   }

   ///////////////////////////////
   // Latches, sampled on the new counter values
   const bool c9_eq_r9_now = C9EqualsR9();

   if (prev == 0 || prev == 1)
   {
      last_line_ = (vcc_ == registers_list_[4]) && c9_eq_r9_now;
      if (last_line_)
      {
         adjust_ = true;
      }
   }

   // Additional interlace line at the end of the frame : interlace on and ParitéR6 odd (19.6.1)
   const bool interlace_line = InterlaceOn() && parity_r6_;

   if (prev == 2)
   {
      if (adjust_)
      {
         if (last_line_ && registers_list_[5] == 0 && !interlace_line)
            adjust_ = false;
         else
            adjust_confirmed_ = true;
      }
      vsync_allowed_ = true;
   }

   if (prev <= 2)
   {
      const unsigned char next_c9 = (c9_eq_r9_now && !adjust_confirmed_) ? 0 : ((vlc_ + 1) & 0x1F);
      adjust_end_ = (next_c9 == ((registers_list_[5] + (interlace_line ? 1 : 0)) & 0x1F));
   }

   if (c9_managed_)
   {
      c4_increment_ = c9_eq_r9_now;
   }

   if (c0_reset)
   {
      c9_managed_ = false;
   }
   if (hcc_ == 1)
   {
      c9_managed_ = true;
   }

   line_end_ = (hcc_ == registers_list_[0]);

   ClockParityR6();

   // VMA' is updated when C0 reaches R1 on the last line of a character (17, 19.8.1)
   if ( hcc_ == registers_list_[1] && C9EqualsR9() )
   {
      bu_ = ma_;
   }

   ///////////////////////////////
   // VSYNC (16.3, 16.4.1)
   if (vcc_ == registers_list_[7])
   {
      // R7 is ignored during a VSYNC ; a new one needs the C4==R7 equality to change
      if (v_no_sync_ && (!ff4_ || ff4_reset))
      {
         if (vsync_allowed_)
         {
            // IVM, odd number of lines per character (R9 odd) : on an odd C4 of an odd frame the VSYNC is
            // delayed by one line to balance the two frames (19.5.2)
            if (InterlaceVideo() && (registers_list_[9] & 1) && (vcc_ & 1) && !even_field_ && c0_reset)
            {
               vsync_line_delay_ = true;
            }
            else if (InterlaceOn() && even_field_)
            {
               vsync_mid_pending_ = true;
            }
            else
            {
               ff4_set = true;
               if (!c0_reset)
               {
                  c3h_load_ = true;
               }
            }
         }
         // Without the C0=2 authorisation, the VSYNC is blocked as if it had happened (13.2.2)
         v_no_sync_ = false;
      }
   }
   else
   {
      v_no_sync_ = true;
   }
   if (c0_reset)
   {
      vsync_allowed_ = false;
   }
   if (ClockMidVSync())
   {
      ff4_set = true;
      c3h_load_ = !c0_reset;
   }


   // DISPEN is enabled by the C0 reset, not by C0 overflowing to 0 (17.1)
   if (c0_reset)
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

   ClockDispEnHalvesCrtc02();

   if ( ff4_reset && !ff4_set)
   {
      ff4_ = false;
   }
   else if ( !ff4_reset && ff4_set)
   {
      ff4_ = true;
   }
}
