#include "stdafx.h"
#include "CRTC.h"
#include "VGA.h"

void CRTC::ComputeMux1 ()
{
   if (mux_set_ && ! mux_reset_)
   {
      mux_ = true;
   }
   else if (!mux_set_ &&  mux_reset_)
   {
      mux_ = false;
   }
   else if (mux_set_ &&  mux_reset_)
   {
      // ???
      mux_ = true;
   }
}

void CRTC::ClockTick1 ()
{
   bool ff1_set = false;
   bool ff1_reset = false;

   bool ff2_set = false;
   bool ff2_reset = false;

   bool ff3_set = false;
   bool ff3_reset = false;

   bool ff4_set = false;
   bool ff4_reset = false;

   // Clock tick
   if (hcc_ == registers_list_[0] )
   {
      hcc_ = 0;  // Reset to 0 at the next count
   }
   else
   {
      hcc_++;
      ma_++;
   }

   if (ff2_ && (horinzontal_pulse_ != horizontal_sync_width_))
   {
      horinzontal_pulse_ = (horinzontal_pulse_+1)&0xF;
   }

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

      if ( r4_reached_ )
         vertical_adjust_counter_ = (++vertical_adjust_counter_)&0x1F;

      // Adress depends on VLC : If VLC < R9, Inc the R0-R2 bits of adress
      //bMuxReset = (m_VLC == m_Register [9]);

      // Adress is : CLK - MA0 -> MA9- R0->R2 - MA12 MA13
      if (  ( (vlc_ == registers_list_ [9])
              || (   ( ( registers_list_[8]&0x3) == 0x3 )
                  && ( (vlc_+1) == registers_list_ [9])
                 )
            )
         )
      {
         if ( vcc_ == registers_list_[4])
         {
            if ( !r4_reached_)
            {
               r4_reached_ = true;
            }
         }
      }

      // MUX set if
      // ADD THIS to fix madness (but Camembert4 now have double scroller...)
      // Note 1 : Cam4 ko si bMuxReset present
      // Note 2 : From scratch ko si bMuxreset absent
      //bMuxReset = ((m_VLC == m_Register [9])&&(m_HPulse == m_HSyncWidth) && (FF2));
      mux_reset_ = (vlc_ == registers_list_ [9]);
      mux_set_ = r4_reached_ && (vertical_adjust_counter_ == registers_list_[5]);
      ComputeMux1 ();

      if (vertical_adjust_counter_ == registers_list_[5])vertical_adjust_counter_ = 0;

      // CRTC 1 : m_BA is refreshed
      if (!mux_)
      {
         ma_ = bu_;
      }
      else
      {
         ma_ = registers_list_[13] + ((registers_list_[12]&0x3F)<<8);
         if ( hcc_ == registers_list_[1] && vlc_ == registers_list_ [9] )
            bu_ = ma_;
      }
   }

   if (hcc_ == 0)
   {
      if (  ( (vlc_ == registers_list_ [9])
              || (   ( ( registers_list_[8]&0x3) == 0x3 )
                  && ( (vlc_+1) == registers_list_ [9])
                 )
            )
         )
      {
         vlc_ = 0;
         vcc_ = (++vcc_)&0x7F;
      }
      else
      {

         if (( registers_list_[8]&0x3) == 0x3)
         {
            vlc_ = (++(++vlc_))&0x1F;
         }
         else
         {
            vlc_ = (++vlc_)&0x1F;
         }
      }

      if (mux_set_)
      {
         vlc_ = 0;

         vcc_ = 0;
      }
      if (mux_)
      {
         ff3_set = true;
         //m_VLC = 0;
         r4_reached_ = false;
         //ff3_set = true;

         // Next frame
         //m_VCC = 0;
         even_field_ = !even_field_;
      }

      // Recompute the mux
      mux_set_ = false;
   }

   if ( hcc_ == registers_list_[1] && vlc_ == registers_list_ [9] )
   {
      bu_ = ma_;
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
   }

   if( hcc_ == registers_list_[2])
   {
      h_no_sync_ = false;
      signals_->hsync_raise_ = true;
      ff2_set = true;
   }
   else
   {
      h_no_sync_ = true;
   }

   // Todo : This is NOT correct. This can be fixed with a OUT_N_A_ with a m_CurrentOpcodeTick of 9... Which breaks lots of other things
   if ((horinzontal_pulse_ == horizontal_sync_width_) && (ff2_||signals_->hsync_raise_))
   {
      if (!signals_->hsync_raise_)
      {
         // NOTE 2 : FROM SCRATCH OK SI "ComputeMux_1" absent
         // Note 1 : Cam4 ok si "ComputeMux_1 ();" Present
         //bMuxReset = (m_VLC == m_Register [9]);
         //ComputeMux_1 ();
      }

      //m_Sig->HsyncFallWr = true;
      signals_->hsync_fall_ = true;

      //m_Sig->HsyncFall = true;
      ff2_reset = true;
      horinzontal_pulse_ = 0;
      // Something to do ?
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
      // Nothing .
      ff1_ = !ff1_reset;
   }
   // Flip flop computation
   if ( ff2_reset && !ff2_set)
   {
      ff2_ = false;
   }
   else if ( !ff2_reset && ff2_set)
   {
      ff2_ = true;
   }
   else if ( ff2_reset && ff2_set)
   {
      // Nothing .
      ff2_ = false; // s&ko ?
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
   }
   // Flip flop computation
   if ( ff4_reset && !ff4_set)
   {
      ff4_ = false;
   }
   else if ( !ff4_reset && ff4_set)
   {
      ff4_ = true;
   }
   else if ( ff4_reset && ff4_set)
   {
      // Nothing .
   }

}
