#include "stdafx.h"
#include "CRTC.h"
#include "VGA.h"

///////////////////////////////////////////////////////////////
//
// CRTC 2 (MC6845) - Compendium 11.2.5, 12.4, 13.4, 15.4.4, 15.5, 15.6, 16.4.3, 17.4.3, 18.2.2
//
// One call is one character clock edge. The frame end is driven by a "Dernière Ligne" (DL) state :
// - last_line_ (DL)         evaluated at the end of character 0 : C4==R4 with the current R4, but C9==R9
//                           sampled at the start of the character (c9_eq_r9_at_start_ : a R9 written on
//                           C0=0 is seen too late). Refused if the previous line was already a last line
//                           (DLP) or if the HSYNC output is active on C0=0. During the line it is also set by a rising
//                           edge of C4==R4 && C9==R9 (register write) if GDL is active, outside the HSYNC ;
//                           the comparator sees the new R9 of a write on C0=0 from the next character, which
//                           gives that edge there.
//                           Once true, it cannot be cancelled in the line.
// - dlp_ (DLP)              C4==R4 && C9==R9 during the last character of the HSYNC.
// - gdl_reenabled_          set on the last HSYNC character when C4!=R4 || C9!=R9 : GDL ("Gestion Dernière
//                           Ligne") is active except on the first line of a frame (C4=C9=0), unless re-enabled.
// - VMA'                    loaded on C0==R1 with R12/R13 if DL, else with VMA if C9==R9. VMA is loaded with
//                           VMA' at each line start, the first line of a frame included (17.4.3). R12/R13
//                           reach VMA' by an AND strobe then an OR strobe : with R1=0 on a frame start, DL
//                           falls between them and only the AND is done (17.4.3, AND bug).
// - vsync_ghost_            a VSYNC condition met during a character on which the HSYNC output is active (a
//                           condition reached on C0=0 is seen on the edge that ends the previous character, so a
//                           HSYNC spanning C0=0 hides it) starts a GHOST VSYNC : C3h counts the lines,
//                           no new VSYNC is accepted, but the pin stays low (15.4.4).
// - the border is not lifted on C0=0 while a HSYNC is active (15.5).
//
///////////////////////////////////////////////////////////////

void CRTC::ClockTick2 ()
{
   bool ff1_set = false;
   bool ff1_reset = false;


   bool ff4_set = false;
   bool ff4_reset = false;

   const unsigned char prev = hcc_;
   const bool hsync_in_previous_char = hsync_;

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

   if (c0_reset && prev == 0)
   {
      // A line of one character (R0 = 0) : its C0==0 position is also its end. The "Derniere Ligne"
      // state is evaluated with the counters of this line, before those of the next one (13.4 : R0 = 0
      // does not disturb the counting of C9 and C4)
      last_line_ = (vcc_ == registers_list_[4]) && c9_eq_r9_at_start_ && !dlp_ && !hsync_on_line_start_;
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

      const bool c9_eq_r9 = C9EqualsR9();
      // Additional interlace line when ParitéR6 is odd, counted as one more R5 line (19.6.3)
      const unsigned char adjust_lines = registers_list_[5] + ((InterlaceOn() && parity_r6_) ? 1 : 0);

      bool new_frame = false;
      if (adjust_)
      {
         // The interlace line (the one after the R5 lines) is no longer one when the interlace is switched
         // off during it : the additional management ends, C9 and C4 count as usual and C4 == R4 can make a
         // last line again (11.9)
         if (vertical_adjust_counter_ == registers_list_[5] && !InterlaceOn())
         {
            adjust_ = false;
         }
         else
         {
            // C5 counts the additional lines (11.2.5)
            vertical_adjust_counter_ = (vertical_adjust_counter_ + 1) & 0x1F;
            new_frame = (vertical_adjust_counter_ == adjust_lines);
         }
      }
      else if (last_line_)
      {
         if (adjust_lines != 0)
         {
            // The additional lines are displayed before C4 and C9 return to 0
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
         // C9 returns to 0 on R9 and increments C4, whatever R4 : only DL returns C4 to 0
         // C9.IVM counts all the time : reset with C9, and when C9 reaches R9/2 (19.8.3)
         if (c9_eq_r9)
         {
            vlc_ = 0;
            vcc_ = (vcc_ + 1) & 0x7F;
            c9_ivm_ = 0;
         }
         else
         {
            c9_ivm_ = (vlc_ == (registers_list_[9] >> 1)) ? 0 : ((c9_ivm_ + 1) & 0x1F);
            vlc_ = (vlc_ + 1) & 0x1F;
         }
      }
      else
      {
         vlc_ = 0;
         c9_ivm_ = 0;
         vcc_ = 0;
         adjust_ = false;

         // Next frame : ParitéFrame = ParitéR6 (19.5.4)
         even_field_ = !parity_r6_;
      }

      // With R1=0, VMA' is loaded (with the DL state of the line that ends) before VMA=VMA' (17.4.3).
      // VMA' takes R12/R13 by two strobes of the C0=R1 character, both gated by DL : an AND (clears the bits
      // at 0) then an OR (sets the bits at 1). On C0=0 the DL state changes between them : on the first line
      // of a frame only the AND is done, on the VMA' loaded with VMA on C0=0 of the last line (AND bug)
      if (registers_list_[1] == 0)
      {
         if (last_line_)
            bu_ &= registers_list_[13] + ((registers_list_[12] & 0x3F) << 8);
         else if (vlc_ == registers_list_[9])
            bu_ = ma_;
      }

      // VMA is loaded with VMA', on the first line of a frame too (17.4.3)
      ma_ = bu_;

      // C9==R9 seen at the start of the line, before any write on C0=0 (12.4.1)
      c9_eq_r9_at_start_ = C9EqualsR9();
   }

   // C4==R4 && C9==R9 comparator. During the character C0==0 its R9 input is still the R9 of the line start :
   // a R9 written on C0==0 changes its output on the next character only (12.4.1)
   const bool c9_eq_r9_seen = (prev == 0 && !c0_reset) ? c9_eq_r9_at_start_ : C9EqualsR9();
   const bool line_eq = (vcc_ == registers_list_[4]) && c9_eq_r9_seen;

   ///////////////////////////////
   // VSYNC (15.4.4, 16.3, 16.4.3) : evaluated on every C0. A condition met during a character on which the HSYNC
   // output was active starts a GHOST VSYNC (chronogram p156 : R7 = C4 written on C0 = R2 - 1 gives a real
   // VSYNC, on C0 = R2 to R2 + R3l - 1 a ghost one, on C0 = R2 + R3l a real one again)
   const bool hsync_window = hsync_in_previous_char;
   if (vcc_ == registers_list_[7])
   {
      if ( v_no_sync_ && (!ff4_ || ff4_reset))
      {
         if (InterlaceOn() && even_field_)
            vsync_mid_pending_ = true;
         else
            ff4_set = true;
         vsync_ghost_ = hsync_window;

         v_no_sync_ = false;
      }
   }
   else
   {
      v_no_sync_ = true;
   }
   if (ClockMidVSync())
   {
      ff4_set = true;
   }

   bool hsync_started, hsync_ended;
   ClockHSync(hsync_started, hsync_ended);

   if (c0_reset)
   {
      // HSYNC output active during the character C0=0, started on it or before (15.6 : R2 = 50, R3l = 15 on
      // R0 = 63 cancels the last line as R2 = 0 does). It inhibits the "Derniere Ligne" evaluation of C0=0,
      // as it inhibits the border lift (15.5)
      hsync_on_line_start_ = hsync_;
   }

   ///////////////////////////////
   // Last line state
   const bool first_line = (vcc_ == 0 && vlc_ == 0);
   // Evaluated on the C0==0 position, at the end of the character (a line of one character, R0 = 0,
   // is evaluated before the next line starts, see above)
   if (prev == 0 && !c0_reset)
   {
      last_line_ = line_eq && !dlp_ && !hsync_on_line_start_;
   }
   else if (!c0_reset && !last_line_ && line_eq && !last_line_eq_ && !hsync_ && (!first_line || gdl_reenabled_))
   {
      // R4 / R9 written outside the HSYNC (15.6)
      last_line_ = true;
   }
   // Comparator output during the character that ends on this edge
   const bool line_eq_in_previous_char = last_line_eq_;
   last_line_eq_ = line_eq;

   if (hsync_ended)
   {
      // DLP is clocked by the edge that ends the last HSYNC character : it samples the comparator output of
      // that character, before the counters move (a HSYNC ending on C0=R0 samples the line that ends, 15.6)
      if (line_eq_in_previous_char)
      {
         dlp_ = true;
      }
      else
      {
         dlp_ = false;
         gdl_reenabled_ = true;
      }
   }
   if (c0_reset)
   {
      gdl_reenabled_ = false;
   }

   // VMA' (17.4.3) ; in IVM also in the middle of the character, on C9 = R9/2 (19.8.3)
   if (hcc_ == registers_list_[1] && !c0_reset)
   {
      if (last_line_)
         bu_ = registers_list_[13] + ((registers_list_[12] & 0x3F) << 8);
      else if (vlc_ == registers_list_[9] || (InterlaceVideo() && vlc_ == (registers_list_[9] >> 1)))
         bu_ = ma_;
   }

   ClockParityR6();

   // The border is not lifted on C0=0 during a HSYNC (15.5)
   if (hcc_ == 0 && !hsync_)
   {
       ff1_set = true;
   }

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
      vsync_ghost_ = false;
   }
   else if ( !ff4_reset && ff4_set)
   {
      ff4_ = true;
   }
}
