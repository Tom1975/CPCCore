#include "stdafx.h"
#include "CRTC.h"
#include "VGA.h"


// Type of register /
#define N      0x0
#define R      0x1
#define W      0x2
#define RW     0x3

#ifdef _LogCRC
#define LOG(str) \
   if (log_) log_->WriteLog (str);
#define LOGEOL if (log_) log_->EndOfLine ();
#define LOGB(str) \
   if (log_) log_->WriteLogByte (str);
#else
#define LOG(str)
#define LOGB(str)
#define LOGEOL
#endif


typedef  unsigned char  CRTCRegistersAcces[CRTC::MAX_CRTC] ;

CRTCRegistersAcces CRTCAccess[32] = {

//   TYPE 0                TYPE 1            TYPE 2               TYPE 3            TYPE4
{ W,                       W,                W,                   W,                W     },     // 0
{ W,                       W,                W,                   W,                W     },     // 1
{ W,                       W,                W,                   W,                W     },     // 2
{ W,                       W,                W,                   W,                W     },     // 3
{ W,                       W,                W,                   W,                W     },     // 4
{ W,                       W,                W,                   W,                W     },     // 5
{ W,                       W,                W,                   W,                W     },     // 6
{ W,                       W,                W,                   W,                W     },     // 7
{ W,                       W,                W,                   W,                W     },     // 8
{ W,                       W,                W,                   W,                W     },     // 9
{ W,                       W,                W,                   W,                W     },     // 10
{ W,                       W,                W,                   W,                W     },     // 11
{ RW,                      W,                W,                   RW,               RW     },     // 12
{ RW,                      W,                W,                   RW,               RW     },     // 13
{ RW,                      RW,               RW,                  RW,               RW     },     // 14
{ RW,                      RW,               RW,                  RW,               RW     },     // 15
{ R,                       R,                R,                   R,                R     },     // 16
{ R,                       R,                R,                   R,                R     },     // 17
{ N,                       N,                N,                   N,                N     },     // 18
{ N,                       N,                N,                   N,                N     },     // 19
{ N,                       N,                N,                   N,                N     },     // 20
{ N,                       N,                N,                   N,                N     },     // 21
{ N,                       N,                N,                   N,                N     },     // 22
{ N,                       N,                N,                   N,                N     },     // 23
{ N,                       N,                N,                   N,                N     },     // 24
{ N,                       N,                N,                   N,                N     },     // 25
{ N,                       N,                N,                   N,                N     },     // 26
{ N,                       N,                N,                   N,                N     },     // 27
{ N,                       N,                N,                   N,                N     },     // 28
{ N,                       N,                N,                   N,                N     },     // 29
{ N,                       N,                N,                   N,                N     },     // 30
{ N,                       N,                N,                   N,                N     },     // 31
};


CRTC::CRTC(void) : cursor_line_(nullptr)
{
   DefinirTypeCRTC(UM6845R);

   signals_ = NULL;
   log_ = NULL;
   Reset();
}

CRTC::~CRTC(void)
{
}

/*void CCRTC::SetBus (Bus* address_bus, Bus* data_bus)
{
   address_bus_ = address_bus;
   data_bus_ = data_bus;
}*/

void CRTC::Reset()
{
   status1_ = 0;
   status2_ = 0;

   gun_button_ = 0;

   memset(registers_mask_, 0xFF, 32);

   registers_list_[0] = 0x3F;   registers_mask_[0] = 0xFF;
   registers_list_[1] = 0x28;   registers_mask_[1] = 0xFF;
   registers_list_[2] = 0x2E;   registers_mask_[2] = 0xFF;
   registers_list_[3] = 0x8E;   registers_mask_[3] = 0xFF;
   registers_list_[4] = 0x26;   registers_mask_[4] = 0x7F;
   registers_list_[5] = 0x00;   registers_mask_[5] = 0x1F;
   registers_list_[6] = 0x19;   registers_mask_[6] = 0x7F;
   registers_list_[7] = 0x1E;   registers_mask_[7] = 0x7F;
   registers_list_[8] = 0x00;   registers_mask_[8] = 0x03;
   registers_list_[9] = 0x07;   registers_mask_[9] = 0xFF;

   registers_list_[10] = 0x0;   registers_mask_[10] =0x7F;
   registers_list_[11] = 0x0;   registers_mask_[11] =0x1F;
   registers_list_[12] = 0x20;  registers_mask_[12] =0x3F;
   registers_list_[13] = 0x0;   registers_mask_[13] =0xFF;
   registers_list_[14] = 0x0;   registers_mask_[14] =0x3F;
   registers_list_[15] = 0x0;   registers_mask_[15] =0xFF;
   registers_list_[16] = 0x0;   registers_mask_[16] =0x3F;
   registers_list_[17] = 0x0;   registers_mask_[17] =0xFF;

   

   lightpen_input_ = true;

   hcc_ = 0;
   vcc_ = 0;
   vlc_ = 0;
   ma_ = 0;
   scanline_vbl_ = 0;
   r4_reached_ = false;
   vertical_adjust_counter_ = 0;
   sscr_bit_8_ = 1;
//   m_LineCounter = 0;

   if ( signals_ != NULL)
   {
      signals_->h_sync_ = false;
      signals_->v_sync_ = false;
      signals_->hsync_fall_ = false;
      signals_->hsync_raise_ = false;

   }
   status_register_ = 0;

   r9_triggered_ = false;
   r4_triggered_ = false;

   even_field_ = true;
   v_no_sync_ = true;
   h_no_sync_ = true;
   mux_set_ = false;
   mux_reset_ = false;
//   m_bResetVLC = false;

//   m_bTrickR4 = false;
   inc_vcc_ = false;
   de_bug_ = false;

   shifted_ssa_ = false;
   ssa_ready_ = false;

}

unsigned char CRTC::In ( unsigned short address )
{
   if (( address & 0x4300) == 0x0000)
   {
      //m_Sig->IORW = false;
      return adddress_register_;
   }

   else if (( address & 0x4300) == 0x0200)
   {
      // Adress = 0xBExx
      // Return the Status register (CRTC type 1, 3, 4)
      switch (type_crtc_ )
      {
      case 0:
         return 0xFF;
      case 1:
         return status_register_|((ff3_)?0x00:0x20)| (lightpen_input_?0x40:0);
      case 2:
         return 0xFF;
      case 3:
      case 4:
         if ( (CRTCAccess[adddress_register_][type_crtc_] & R) == R )
            return registers_list_[adddress_register_];
         break;
      default:
         break;
      }
   }
   else if (( address & 0x4300) == 0x0300)
   {
      // Adress = 0xBFxx
      // Return the selected register, if possible (TODO : Implement differences)
      //m_Sig->IORW = false;

      // Status update
      if ( (adddress_register_ == 12  || adddress_register_ == 13)
         && type_crtc_ == 2)
      {
         return 0;
      }


      if (adddress_register_ == 31)
      {
         status_register_ &=  0x7F;
         switch (type_crtc_ )
         {
         case 0:
            return 0;
         case 1:
            return 0xFF;
         case 2:
            return 0;
         case 3:
         case 4:
            return 0;
         default:
            break;
         }
      }
      //if (m_AdressRegister == 16 || m_AdressRegister == 17 ) m_StatusRegister &=  0xBF;
      if (adddress_register_ == 16 || adddress_register_ == 17)
      {
         lightpen_input_ = false;
      }

      if (type_crtc_ == 3 || type_crtc_ == 4)
      {
         switch (adddress_register_)
         {
         case 6: // Unclear : Return 0 ?
         case 7:
         case 14:
         case 15:
         case 22:
         case 23:
         case 30:
         case 31:
            return 0;

         case 2:  //Status 1
         case 10:
         case 18:
         case 26:
         {
            // Compute status
            if (type_crtc_ == 3 || type_crtc_ == 4)
            {
               if (hcc_ == registers_list_[1]) status1_ &= ~0x04;
               if (hcc_ == registers_list_[0] / 2) status1_ &= ~0x02;
               if (hcc_ != registers_list_[0]) status1_ &= ~0x01;
            }
            return status1_;
         }

         case 3: // Status 2
         case 11:
         case 19:
         case 27:
            if (type_crtc_ == 3 || type_crtc_ == 4)
            {
               status2_ = (vlc_ == 0) ? (~0x80) : 0xFF;
               if (vlc_ == registers_list_[9]) status2_ &= ~0x20;

            }
            return status2_;

         case 0:// REG 16
         case 8:
         case 16:
         case 24:
            return (registers_list_[16]);
         case 1:// REG 17
         case 9:
         case 17:
         case 25:
            return (registers_list_[17]);

         case 4:// REG 12
         case 12:
         case 20:
         case 28:
            return (registers_list_[12]);
         case 5:// REG 13
         case 13:
         case 21:
         case 29:
            return (registers_list_[13]);
         }
      }

      if ( (CRTCAccess[adddress_register_][type_crtc_] & R) == R )
         return registers_list_[adddress_register_] ;
      else
      {
         if (type_crtc_ == 0 || type_crtc_ == 1)
         {
            return 0;
         }
      }

   }
   return signals_->data_bus_->GetByteBus();
}

void CRTC::Out (unsigned short address, unsigned char data)
{
   // Something to decode from Adress ?
   // Conditions are :
   // EN = 1
//   if ( m_Sig->IORW == true)
   {
      //unsigned char data = data_bus_->GetByteBus ();
      // Adress = 0xBCxx
      if ((address & 0x4300) == 0x0000) // x0xxxx00
      {
         // Select Register
         adddress_register_ = data & 0x1F ;
         //m_Sig->IORW = false;
      }
      else if ((address & 0x4300) == 0x0100) // x0xxxx01
      {
         // Adress = 0xBDxx
         // Ecriture
         if ( (CRTCAccess[adddress_register_][type_crtc_] & W) == W )
         {
            // LOG : Only if change occurs
#ifdef _LogCRC
            if (m_Register[m_AdressRegister] != (data & (m_RegisterMask[m_AdressRegister])))
            {
               LOG(_T("CRTC CHANGE : Reg "));
               LOG(m_AdressRegister);
               LOG(_T(" = "));
               LOG(data & (m_RegisterMask[m_AdressRegister]));
               LOGEOL
            }
#endif
            registers_list_[adddress_register_] = (data & (registers_mask_[adddress_register_]));

            // Case of some type of CRTC - TODO
            switch ( adddress_register_)
            {
            case 0:

               if ( type_crtc_ == 0)
               {
                  if ( registers_list_[adddress_register_] == 0)
                     registers_list_[adddress_register_] = 1;
               }
               break;
            case 2:
               break;
            case 3:  // VSync width depends on the CRTC type*
               horizontal_sync_width_ = (registers_list_ [3] & 0x0F);
               switch (type_crtc_)
               {
               case 0: //
                  vertical_sync_width_ = ((registers_list_ [3]&0x80 )== 0x80)?16:8;
                  if (vertical_sync_width_ == 0)vertical_sync_width_ = 16;
                  break;
               case 1:
               case 2:
                  vertical_sync_width_ = 16;
                  break;
               case 3:
               case 4:
                  vertical_sync_width_ = registers_list_ [3] >> 4;
                  if (vertical_sync_width_ == 0)vertical_sync_width_ = 16;
                  if (horizontal_sync_width_ == 0) horizontal_sync_width_ = 16;
                  break;
               case MAX_CRTC:
               default:
                  break;
               }
               if (scanline_vbl_ == vertical_sync_width_)
               {
                  ff4_ = false;
               }
               //ComputeMux_1 ();

               break;
            case 4:
               {
                  // This test is a bit wtf....
                  // TODO : Sort out why it works HERE for camembert 4 without messing all other demos
                  /*
                     Notice a first timing trick at the frontier between the 2 blocks: it seems that on some CRTCs it is not a good idea
                     to program R4 when VC is zero. Although I cannot be affirmative on this, I think it is because there is a small
                     period of time when the register file of the CRTC gets written, where the register written will seem to be zero.
                     This would cause another match with VC, and the sequence of VC values would be 18,0,0,1 thus
                     repeating the top character line of block 2 !
                  */
                  switch (type_crtc_)
                  {
                  case 1:
                     if ( registers_list_[4] == 0 && vcc_ == 0 )
                     {
                        // -> NOT CORRECT : This would prevent Chany dream 2 from working
                        mux_reset_ = (vlc_ == registers_list_ [9]);
                        mux_set_ = false;
                        ComputeMux1 ();
                     }
                     break;
                  default:
                     break;
                  }
                  break;
               }
            case 5:
               {
                  break;
               }
            case 9:
               {
                  r9_triggered_ = vlc_ == registers_list_[9];
                  if (type_crtc_ == 1)
                  {
                     // AJOUT TO TEST
                     if ( registers_list_[4] == 0 && registers_list_[9] == 0)
                     {
                        r9_triggered_ = true;
                     }
                  }
                  break;
               }

            case 12:
            case 13:
               {
               break;
               }
            case 7:
               {
                  if (vcc_ == registers_list_[7])
                  {
                     v_no_sync_ = true;
                  }
                  break;
               }
            case 8:
               {
               }
               break;
            }

#ifdef _LogCRC
            // LOG :
            char buf [256];
            _stprintf_s ( buf, 256, _T("OUT : R%i <- %i; HCC = %i, VCC = %i, VLC = %i, X = %i, Y = %i"),
               m_AdressRegister, m_Register[m_AdressRegister], m_HCC, m_VCC, m_VLC, vga_->monitor_->m_X, vga_->monitor_->m_Y);

            LOG (buf);
            LOGEOL;
#endif
         }
      }
   }

}

void CRTC::DefinirTypeCRTC(TypeCRTC type_crtc)
{
   type_crtc_ = type_crtc;
   switch (type_crtc_)
   {
   case 0:
      TickFunction = &CRTC::ClockTick0;
      break;
   case 1:
      TickFunction = &CRTC::ClockTick1;
      break;
   case 2:
      TickFunction = &CRTC::ClockTick2;
      break;
   default:
      TickFunction = &CRTC::ClockTick34;
   }
   
}

unsigned int CRTC::Tick (/*unsigned int nbTicks*/)
{
   (this->*(TickFunction))();

   /////////////////////////
   // DISPMSG
   //m_Sig->DISPEN = ( FF3 & FF1 );

   /////////////////////////
   // HSYNC
   //signals_->h_sync_ = ff2_;
   /////////////////////////
   // VSYNC
   signals_->v_sync_ = ff4_;

   // Lightgun :
   // If X/Y is in the current zone => do something
   gate_array_->Tick();
   
   // Cursor
   if ( vlc_ >= registers_list_[10] && vlc_ <= registers_list_[11]
      && ma_ == registers_list_[15] + ((registers_list_[14] & 0x3F) << 8))
   {
      // Set CURSOR
      if (cursor_line_) cursor_line_->Tick();
   }

   if (gun_button_ == 1)
   {
      if (   ((((gate_array_)->monitor_)->x_ +16 - gun_x_)&0x7FFFFFFF) < 16
         && ((gate_array_)->monitor_)->y_*2 == gun_y_
         )
      {
         // Found
         // GUNSTICK : Joystick down
         registers_list_[16] = (ma_ >> 8) & 0x3F;
         registers_list_[17] = (ma_  & 0xFF);
      }
   }
   else
   {
      // todo : this rand is too expensive !
      //if (rand() & 1)
      {
         registers_list_[16] = (ma_ >> 8) & 0x3F;
         registers_list_[17] = (ma_ & 0xFF);
      }
   }


   /////////////////////////
   // CUDISP
   // TODO
   return 4;
}
