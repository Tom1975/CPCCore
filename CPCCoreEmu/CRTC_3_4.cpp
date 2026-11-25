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
      // Vertical sync width counter C3h (16.3), 4 bits : loaded with 1 by the VSYNC start (flip flop below), it
      // counts the lines. The C3h == R3h comparator, true during the last VSYNC line, resets the VSYNC on the line
      // start that follows it ; R3h = 0 gives 16 lines (C3h wraps to 0). The counter is enabled by a value other
      // than 0, or by the VSYNC as long as the comparator does not end it : it runs past the VSYNC up to 0, where it
      // stops. The same comparator output is the status 1 bit 5 (21.3.4.1, CRTC::ReadRegister).
      // PROVISIONAL (RealCpc/README.md, P6) : the run past the VSYNC comes from SHAKER D3 #03C8 (R3h = 8 : bit 5 at
      // 1 on 15 lines), #03C9 (R3h = 7 : 0 on one line)
      const bool c3h_eq_r3h = (c3h_status_ == (vertical_sync_width_ & 0x0F));
      if (ff4_ && c3h_eq_r3h)
      {
         ff4_reset = true;
      }
      if (c3h_status_ != 0 || (ff4_ && !c3h_eq_r3h))
      {
         c3h_status_ = (c3h_status_ + 1) & 0x0F;
      }

      // VMA is reloaded from VMA'
      ma_ = bu_;

      // ParitéFrame of the frame that ends : an even frame gets the additional interlace line, and the
      // VSYNC condition on the frame start (R7=0) is processed before the parity toggles (19.6.4, 19.7.3)
      const bool frame_even = even_field_;
      const bool interlace_line = InterlaceOn() && frame_even;

      bool new_frame = false;
      bool row_start = false;   // C9 reloaded with its start value on this line
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
               row_start = true;
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
            row_start = true;
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

         row_start = true;

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
      // ("C9=0" is the first line of a character : in IVM it starts with C9 = ParitéC9)
      if (vsync_line_delay_)
      {
         vsync_line_delay_ = false;
         ff4_set = true;
      }
      else if (vcc_ == registers_list_[7] && row_start && (!ff4_ || ff4_reset))
      {
         // IVM with R9 odd : on an odd C4 of an odd frame the VSYNC is delayed by one line (19.5.5)
         if (InterlaceVideo() && (registers_list_[9] & 1) && (vcc_ & 1) && !frame_even)
            vsync_line_delay_ = true;
         // MID-VSYNC on an even frame in interlace (19.7.3)
         else if (InterlaceOn() && frame_even)
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
         // VMA' is loaded with VMA on the last line of the character : the output of the magnitude comparator
         // C9 >= R9 that ends the character (10.3.4). In IVM with R9 odd the rows of even C9 end on C9 = R9 + 1,
         // and the pointer moves on every row (19.8.4, SHAKER B_3/B_4 INTERLACE C4/C9 COUNTERS #00D2) ; a R9
         // written lower than C9 ends the character the same way. With the ASIC vertical soft scroll the
         // compared C9 is shifted (SSCR) and keeps its 3-bit equality.
         // VMA' is not updated during the additional lines (11.2.6)
         int vertical_shift = (gate_array_->memory_->GetSSCR()&0x7F) >> 4;
         const bool last_line = (vertical_shift == 0) ? (vlc_ >= registers_list_[9])
                                                       : (((vlc_ + vertical_shift) & 0x7) == (registers_list_[9] & 0x7));
         if ( !adjust_ && !interlace_line_ && last_line)
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

   bool hsync_started, hsync_ended;
   ClockHSync(hsync_started, hsync_ended);
   if (hsync_started)
   {
      // Bit 3	0 : CRTC Horizontal Count == Horizontal Sync Position(Reg 2)
      status1_ &= ~0x08;
   }


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
   else if (ff4_set)
   {
      // A VSYNC start (also one that restarts on its end, 16.4.4) loads C3h with 1
      ff4_ = true;
      c3h_status_ = 1;
   }


   ClockDispEnHalves();
}
