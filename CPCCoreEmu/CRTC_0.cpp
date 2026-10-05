#include "stdafx.h"
#include "CRTC.h"
#include "VGA.h"


void CRTC::ClockTick0 ()
{
   ClockDispTmg();

   bool ff1_set = false;
   bool ff1_reset = false;

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

   // C0=1 allows C9 to be handled again on the next C0=0 (Compendium 13.2.1)
   if (hcc_ == 1)
   {
      c9_managed_ = true;
   }

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
      else
      {
         // TODO : hhtr atteind ?
         // oui => on teste immédiatement vlc/r9
         // A VERIFIER !!!
      }


      if (!c9_managed_)
      {
         // C0 did not reach 1 since the last C0=0 (R0=0) : C9 is frozen, and R4/R5/R9 are ignored.
         // Only a C4 increment armed on the previous C0=0 (C9==R9) still happens, once ; if this was
         // the last line, the additional management it starts cannot be cancelled (Compendium 13.2.4, 13.2.6)
         if (r9_triggered_)
         {
            r9_triggered_ = false;
            if (r4_triggered_)
            {
               r4_triggered_ = false;
               r4_reached_ = true;
            }
            vcc_ = (vcc_ + 1) & 0x7F;
         }
         ma_ = bu_;
      }
      else
      {
         c9_managed_ = false;

         if ( r4_reached_ )
            vertical_adjust_counter_ = (++vertical_adjust_counter_)&0x1F;

         // Adress is : CLK - MA0 -> MA9- R0->R2 - MA12 MA13
         if (  r9_triggered_ )
         {
            if (/*( m_VLC == m_Register[9] ) &&*/ ( r4_triggered_))
            {
               r4_triggered_ = false;
               if ( !r4_reached_)
               {
                  r4_reached_ = true;
               }
            }
         }

         mux_set_ = (r4_reached_ && (vertical_adjust_counter_ == registers_list_[5]) );
         if (vertical_adjust_counter_ == registers_list_[5])vertical_adjust_counter_ = 0;

         // CRTC 0 : m_BA is refreshed
         if (!mux_set_)
         {
            ma_ = bu_;
         }

         if ( r9_triggered_ )
         {
            inc_vcc_ = true;
            vlc_ = 0;
            r9_triggered_ = vlc_ == registers_list_[9];
            vcc_ = (++vcc_)&0x7F;
            if (/*m_bMR_R9 &&*/ vcc_ == registers_list_ [4]) r4_triggered_ = true;
         }
         else
         {
            if (( registers_list_[8]&0x3) == 0x3)
            {
               vlc_ = (++(++vlc_))&0x1F;
               r9_triggered_ = ((vlc_ == registers_list_[9])||(vlc_+1 == registers_list_[9]));
            }
            else
            {
               vlc_ = (++vlc_)&0x1F;
               r9_triggered_ = vlc_ == registers_list_[9];
            }
            if (r9_triggered_ && vcc_ == registers_list_ [4]) r4_triggered_ = true;
         }

         if (mux_set_)
         {
            vlc_ = 0;
            r9_triggered_ = vlc_ == registers_list_[9];

            vcc_ = 0;

            // R0 in the process of changing ?
            /*if ( ( m_Sig->IORW == true) && (( m_AdressBus->GetShortBus () & 0x4300) == 0x0100) && m_AdressRegister == 0 )
            {
               m_MA = m_BU;
            }
            else*/
            {
               ma_ = registers_list_[13] + ((registers_list_[12]&0x3F)<<8);
               bu_ = ma_;
            }

            if ( r9_triggered_ && vcc_ == registers_list_ [4]) r4_triggered_ = true;

            ff3_set = true;
            r4_reached_ = false;

            // Next frame
            even_field_ = !even_field_;
         }

         // Recompute the mux
         mux_set_ = false;
      }
   }

   if ( hcc_ == registers_list_[1] && vlc_ == registers_list_ [9] )
   {
      bu_ = ma_;
   }


   if (vcc_ == registers_list_[7])
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

   bool hsync_started, hsync_ended;
   ClockHSync(hsync_started, hsync_ended);
   if (hsync_ended)
   {
      // Something to do ?
      if (inc_vcc_)
      {
         inc_vcc_ = false;
      //   m_VCC = (++m_VCC)&0x7F;
         //if (m_VCC == m_Register [4]) m_bMR_R4 = true;


      }

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
      // Detector of rising edge : If FF1 is already true, add the "DE" bug for 4 pixels
      de_bug_ = ff1_;

      ff1_ = true;
   }
   else if ( ff1_reset && ff1_set)
   {
      // Nothing .
      ff1_ = false;
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

