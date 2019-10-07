#include "stdafx.h"
#include "CRTC.h"
#include "VGA.h"

///////////////////////////////////////////////////////////////
//
//
// Informations on CRTC3/4
//
// Specific behaviours :
//
//

void CRTC::ClockTick34 ()
{
   bool ff1_set = false;
   bool ff2_set = false;

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


   if (signals_->h_sync_)
   {
      horinzontal_pulse_ = (++horinzontal_pulse_) & 0xF;
   }

   // Counter actions
   if (hcc_ == 0 )
   {
      // Vertical sync width counter
      if (ff4_)    // CE
      {
         scanline_vbl_ ++;
         scanline_vbl_ &= 0x1F;

         if (scanline_vbl_ == vertical_sync_width_)
         {
            scanline_vbl_ = 0;
            ff4_reset = true;
         }
      }

      if ( r4_reached_)
         vertical_adjust_counter_ = (++vertical_adjust_counter_)&0x1F;

      // Adress is : CLK - MA0 -> MA9- R0->R2 - MA12 MA13
      if (r9_triggered_)
      {
         if ( r4_triggered_)
         {
            r4_triggered_ = false;
            if (!r4_reached_)
            {
               r4_reached_ = true;
            }
         }
      }

      mux_set_ = (r4_reached_ && (vertical_adjust_counter_ == registers_list_[5]));
      if (vertical_adjust_counter_ == registers_list_[5])vertical_adjust_counter_ = 0;

      // CRTC 0 : m_BA is refreshed
      if (!mux_set_)
      {
         ma_ = bu_;
      }

      if (r9_triggered_)
      {
         inc_vcc_ = true;
         vlc_ = 0;
         r9_triggered_ = vlc_ == registers_list_[9];
         vcc_ = (++vcc_) & 0x7F;
         if ( vcc_ == registers_list_[4]) r4_triggered_ = true;
      }
      else
      {
         if ((registers_list_[8] & 0x3) == 0x3)
         {
            vlc_ = (++(++vlc_)) & 0x1F;
            r9_triggered_ = ((vlc_ == registers_list_[9]) || (vlc_ + 1 == registers_list_[9]));
         }
         else
         {
            vlc_ = (++vlc_) & 0x1F;
            r9_triggered_ = vlc_ == registers_list_[9];
         }
         if (r9_triggered_ && vcc_ == registers_list_[4]) r4_triggered_ = true;
      }

      if (mux_set_)
      {
         vlc_ = 0;
         r9_triggered_ = vlc_ == registers_list_[9];

         vcc_ = 0;

         ma_ = registers_list_[13] + ((registers_list_[12] & 0x3F) << 8);
         bu_ = ma_;

         if (r9_triggered_ && vcc_ == registers_list_[4]) r4_triggered_ = true;

         ff3_set = true;
         r4_reached_ = false;

         // Next frame
         even_field_ = !even_field_;
      }

      // Recompute the mux
      mux_set_ = false;
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
         int vertical_shift = (gate_array_->memory_->GetSSCR()&0x7F) >> 4;
         if ( ((vlc_ + vertical_shift) &0x7)== ((registers_list_[9] ) & 0x7))
         {
            bu_ = ma_;
         }
      }
   }

   if ((vcc_ == registers_list_[7]) )
   {
      if ( v_no_sync_ )
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

   if( hcc_ == registers_list_[2])
   {
      if (h_no_sync_)
      {
         h_no_sync_ = false;
         signals_->hsync_raise_ = true;
         ff2_set = true;
         // Bit 3	0 : CRTC Horizontal Count == Horizontal Sync Position(Reg 2)
         status1_ &= ~0x08;
      }
   }
   else
   {
      h_no_sync_ = true;
   }

   signals_->h_sync_on_begining_of_line_ = ((hcc_ == 0) && (signals_->h_sync_ || signals_->hsync_raise_));

   if ((horinzontal_pulse_ == horizontal_sync_width_) && (signals_->h_sync_ ||signals_->hsync_raise_))
   {
      signals_->hsync_fall_ = true;
      //ff2_reset = true;
      // Bit 4	0 : CRTC is on last char of HSYNC
      status1_ &= ~0x10;
      horinzontal_pulse_ = 0;
      // Something to do ?
      if (inc_vcc_)
      {
         inc_vcc_ = false;
      }

      if (!ff2_set)
      {
         signals_->h_sync_ = false;
      }
      else 
      {
         // Nothing .
         int dbg = 1;
      }

   }
   else if (ff2_set)
   {
      signals_->h_sync_ = true;
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

   if ( ff3_reset && !ff3_set)
   {
      ff3_ = false;
   }
   else if ( !ff3_reset && ff3_set)
   {
      ff3_ = true;
   }
   else if ( ff3_reset && ff3_set)
   {
      // Nothing .
      int dbg=1;
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
