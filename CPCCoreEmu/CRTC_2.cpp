#include "stdafx.h"
#include "CRTC.h"
#include "VGA.h"

void CRTC::ClockTick2 ()
{
   bool ff1_set = false;
   bool ff1_reset = false;

   bool ff2_set = false;
   bool ff2_reset = false;

   bool ff3_set = false;
   bool ff3_reset = false;

   bool ff4_set = false;
   bool ff4_reset = false;

   //bool bMuxSet = false;
   //bool bMuxReset = false;


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


   if (ff2_ )
   {
      horinzontal_pulse_++;
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
            //FF4 = false;
         }
      }

      if ( r4_reached_)
         vertical_adjust_counter_ = (++vertical_adjust_counter_)&0x1F;

      mux_reset_ = (vlc_ == registers_list_ [9]);

      // Adress depends on VLC : If VLC < R9, Inc the R0-R2 bits of adress
      // Adress is : CLK - MA0 -> MA9- R0->R2 - MA12 MA13
      if (  ( (vlc_ == registers_list_ [9])
              || (   ( ( registers_list_[8]&0x3) == 0x3 )
                  && ( (vlc_+1) == registers_list_ [9])
                 )
            )
         )

      {
         vlc_ = 0;

         if ( vcc_ == registers_list_[4])
         {
            //m_VerticalAdjustCounter = (++m_VerticalAdjustCounter)&0x1F;
            if ( !r4_reached_)
            {
               r4_reached_ = true;
               //m_VerticalAdjustCounter = 0;
            }
         }
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
         /*if ( m_bR4Reached)
            m_VerticalAdjustCounter = (++m_VerticalAdjustCounter)&0x1F;*/
      }

      // MUX set if
      mux_set_ = r4_reached_ && (vertical_adjust_counter_ == registers_list_[5]);

      if (vertical_adjust_counter_ == registers_list_[5])vertical_adjust_counter_ = 0;

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

      // CRTC 1 : m_BA is refreshed
      if (!mux_)
      {
         ma_ = bu_;
      }
      else
      {
         ma_ = registers_list_[13] + ((registers_list_[12]&0x3F)<<8);
      }
   }

   if ( hcc_ == registers_list_[1] && vlc_ == registers_list_ [9] )
   {
      bu_ = ma_;
   }

   if (hcc_ == 0)
   {
      if (mux_set_)//m_bR4Reached && (m_VerticalAdjustCounter == m_Register[5]))
      {
         vlc_ = 0;
         mux_set_ = false;
      }
      if (mux_)
      {
         r4_reached_ = false;
         //m_VerticalAdjustCounter = 0;
         //FF3 = true;
         ff3_set = true;

         // Next frame
         vcc_ = 0;

         // Adress is : CLK - MA0 -> MA9- R0->R2 - MA12 MA13
         //            0            1 .... 8           9 - 10                    11-12-13      14 - 15
         //           CLK -      MA0 -> MA7          MA8-MA9                    R0-R2         MA12 MA 13
         //m_MA = m_Register[13] + ((m_Register[12]&0x3F)<<8);
         even_field_ = !even_field_;
      }
   }

   if ((vcc_ == registers_list_[7]) )
   {
      if ( v_no_sync_ )
      {
         //m_ScanLineVBL = 0;
         //FF4 = true;
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
      //FF3 = false;
      ff3_reset = true;
   }

   // Flip flop computations
   if (hcc_ == 0)
   {
       ff1_set = true;
      //FF1 = true;
   }

   if( hcc_ == registers_list_[2])
   {
      if (h_no_sync_)
      {
         h_no_sync_ = false;
         signals_->hsync_raise_ = true;
         ff2_set = true;
      }
   }
   else
   {
      h_no_sync_ = true;
   }

   if ((horinzontal_pulse_ >= horizontal_sync_width_) && (ff2_||signals_->hsync_raise_))
   {
      //m_Sig->HsyncFallWr = true;
      signals_->hsync_fall_ = true;
      ff2_reset = true;
      horinzontal_pulse_ = 0;
      //FF2 = false;
      // Something to do ?

   }

   if (hcc_ == registers_list_[1])
   {
      //FF1 = false;
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
      int dbg=1;
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
      int dbg=1;
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
   else if ( !ff4_reset && ff4_set)
   {
      ff4_ = true;
   }
   else if ( ff4_reset && ff4_set)
   {
      // Nothing .
      int dbg=1;
   }
}
