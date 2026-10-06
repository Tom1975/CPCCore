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


// R8 bits kept on write, per CRTC type (Compendium 19.1/19.2) : bits 0-1 = interlace on all types,
// bits 4-5 = SKEW-DISPTMG (BORDER delay / force) on CRTC 0/3/4, bits 6-7 = cursor skew on CRTC 0 only.
static unsigned char R8Mask(CRTC::TypeCRTC type_crtc)
{
   switch (type_crtc)
   {
   case CRTC::HD6845S:
      return 0xF3;
   case CRTC::AMS40489:
   case CRTC::AMS40226:
      return 0x33;
   default:
      return 0x03;
   }
}

CRTC::CRTC(void) : signals_(nullptr), gate_array_(nullptr), ppi_(nullptr), play_back_(nullptr), log_(nullptr), cursor_line_(nullptr)
{
   DefinirTypeCRTC(UM6845R);

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
   registers_list_[8] = 0x00;   registers_mask_[8] = R8Mask(type_crtc_);
   registers_list_[9] = 0x07;   registers_mask_[9] = 0x1F;

   registers_list_[10] = 0x0;   registers_mask_[10] =0x7F;
   registers_list_[11] = 0x0;   registers_mask_[11] =0x1F;
   registers_list_[12] = 0x20;  registers_mask_[12] =0x3F;
   registers_list_[13] = 0x0;   registers_mask_[13] =0xFF;
   registers_list_[14] = 0x0;   registers_mask_[14] =0x3F;
   registers_list_[15] = 0x0;   registers_mask_[15] =0xFF;
   registers_list_[16] = 0x0;   registers_mask_[16] =0x3F;
   registers_list_[17] = 0x0;   registers_mask_[17] =0xFF;

   

   lightpen_input_ = true;

   adddress_register_ = 0;
   ComputeSyncWidths();

   hcc_ = 0;
   vcc_ = 0;
   vlc_ = 0;
   ma_ = 0;
   bu_ = 0;
   scanline_vbl_ = 0;
   horinzontal_pulse_ = 0;
   r4_reached_ = false;
   c9_managed_ = true;
   line_end_ = false;
   c4_increment_ = false;
   last_line_ = false;
   adjust_ = false;
   adjust_confirmed_ = false;
   adjust_end_ = false;
   vsync_allowed_ = false;
   c3h_load_ = false;
   frame_counter_ = 0;
   parity_r6_ = false;
   r6_eq_prev_ = false;
   vsync_mid_pending_ = false;
   interlace_line_ = false;
   parity_c9_ = false;
   rfd_parity_ = false;
   ivm_latched_ = false;
   vsync_line_delay_ = false;
   c9_ivm_ = 0;
   vma_reload_ = true;
   vma_reload_clear_ = false;
   rfd_ = false;
   status_border_r6_ = false;
   c9_eq_r9_at_start_ = false;
   hsync_on_line_start_ = false;
   last_line_eq_ = false;
   dlp_ = false;
   gdl_reenabled_ = false;
   vsync_ghost_ = false;
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
   mux_ = false;
   mux_set_ = false;
   mux_reset_ = false;
   // RESET forces DISPEN and VSYNC inactive
   ff1_ = false;
   ff3_ = false;
   ff4_ = false;
//   m_bResetVLC = false;

//   m_bTrickR4 = false;
   inc_vcc_ = false;
   dispen_history_ = 0;
   dispen_half0_ = false;
   dispen_half1_ = false;

   shifted_ssa_ = false;
   ssa_ready_ = false;

}

void CRTC::ComputeSyncWidths()
{
   horizontal_sync_width_ = (registers_list_ [3] & 0x0F);
   switch (type_crtc_)
   {
   case 0:  // R3 = vvvvhhhh, VSYNC 0 = 16 lines, HSYNC 0 = no HSYNC
      vertical_sync_width_ = registers_list_ [3] >> 4;
      if (vertical_sync_width_ == 0)vertical_sync_width_ = 16;
      break;
   case 1:  // R3 = xxxxhhhh, VSYNC always 16 lines, HSYNC 0 = no HSYNC
      vertical_sync_width_ = 16;
      break;
   case 2:  // R3 = xxxxhhhh, VSYNC always 16 lines, HSYNC 0 = 16 us
      vertical_sync_width_ = 16;
      if (horizontal_sync_width_ == 0) horizontal_sync_width_ = 16;
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
}

// HSYNC generation, once per CRTC character, after C0 has been updated (Compendium 14 & 15).
// C3l (horinzontal_pulse_) is a 4-bit counter, reset when the HSYNC starts on C0=R2 : the
// HSYNC ends when C3l reaches R3l (R3l=0 : 16 us on CRTC 2, 3, 4 - no HSYNC at all on CRTC 0, 1).
// C0=R2 is ignored while the HSYNC is active.
void CRTC::ClockHSync(bool& started, bool& ended)
{
   started = ended = false;
   const bool c0_is_r2 = (hcc_ == registers_list_[2]);

   if (signals_->h_sync_)
   {
      horinzontal_pulse_ = (horinzontal_pulse_ + 1) & 0x0F;

      if (type_crtc_ == UM6845R && horizontal_sync_width_ == 0)
      {
         // CRTC 1 keeps handling R3l=0 (no HSYNC) during the HSYNC : it is cancelled (14.5.2)
         ended = true;
      }
      else if (horinzontal_pulse_ == (horizontal_sync_width_ & 0x0F))
      {
         if (c0_is_r2 && type_crtc_ != HD6845S)
         {
            // CRTC 1, 2, 3, 4 : C0=R2 on the last HSYNC position prevents the HSYNC from ending,
            // and C3l, which is not reset, overflows (15.3). CRTC 1 drops and raises the signal
            // again fast enough to be invisible, but the GATE ARRAY sees a new HSYNC (15.3.4).
            if (type_crtc_ == UM6845R)
            {
               signals_->hsync_fall_ = true;
               signals_->hsync_raise_ = true;
            }
         }
         else
         {
            // CRTC 0 is protected : the HSYNC ends, and cannot restart on this position (15.3)
            ended = true;
         }
      }

      if (ended)
      {
         signals_->h_sync_ = false;
         signals_->hsync_fall_ = true;
         horinzontal_pulse_ = 0;
      }
   }
   else if (c0_is_r2 && horizontal_sync_width_ != 0)
   {
      signals_->h_sync_ = true;
      signals_->hsync_raise_ = true;
      horinzontal_pulse_ = 0;
      started = true;
   }
}

// DISPTMG output, after the SKEW-DISPTMG function of R8 (bits 5-4, CRTC 0/3/4 only - Compendium 19.2) :
// 00 : no delay, 01/10 : the border is handled 1/2 characters later, 11 : BORDER ON (no display).
// A change of R8 is taken into account immediately within the line.
bool CRTC::DispEn(int half) const
{
   const int skew = (registers_list_[8] >> 4) & 0x03;
   switch (skew)
   {
   case 0:
      // CRTC 1 : R6=0 forces the border as long as it stays 0 (18.3.3)
      return (half ? dispen_half1_ : dispen_half0_) && !(type_crtc_ == UM6845R && registers_list_[6] == 0);
   case 3:
      return false;
   default:
      // The SKEW delay line keeps both halves of each character
      return ((dispen_history_ >> (2 * (skew - 1) + (half ? 1 : 0))) & 1) != 0;
   }
}

// CRTC 0/2 : DISPEN over the two halves of a character. The outputs are latched on both edges of the
// character clock (17.6, 18.2.2, 18.3.2) :
// - phase A (character start) : the R6 border is cleared on the first line of a frame (C4=C9=0) while the
//   R1 border is not active, else set by C4==R6 ;
// - phase B (half character) : C4==R6 sets the R6 border ; C0==R0 sets the border for the second half only
//   (cleared by the next character) : when R1>R0 it replaces C0==R1.
// On the first line with R6=0 both happen on every character : one displayed byte, one border byte.
void CRTC::ClockDispEnHalvesCrtc02()
{
   if (vcc_ == 0 && vlc_ == 0 && ff1_)
      ff3_ = true;
   else if (vcc_ == registers_list_[6])
      ff3_ = false;
   dispen_half0_ = ff1_ && ff3_;

   if (vcc_ == registers_list_[6])
      ff3_ = false;
   dispen_half1_ = ff1_ && ff3_ && hcc_ != registers_list_[0];
}

// CRTC 0 ParitéC9 in Interlace Video Mode : ParitéFrame, alternated on each C4 when R9 is odd (19.5.2)
unsigned int CRTC::ParityC9Crtc0() const
{
   return (even_field_ ? 0 : 1) ^ (registers_list_[9] & vcc_ & 1);
}

// End of character comparator (C9==R9), Interlace Video Mode included (19.8)
bool CRTC::C9EqualsR9() const
{
   const unsigned char r9 = registers_list_[9];
   if (type_crtc_ == HD6845S)
   {
      // CRTC 0 : the IVM state of the address is taken at the line start, the parity in the R9 test at once :
      // IVM entered during the line, C9 is compared with R9 | ParitéFrame ; IVM left during the line,
      // C9.VMA is compared with R9 without the parity (19.8.1)
      const bool ivm_now = InterlaceVideo();
      if (!ivm_latched_)
         return vlc_ == (ivm_now ? (r9 | (even_field_ ? 0 : 1)) : r9);
      if (!ivm_now)
         return AddressC9() == r9;
   }
   else if (!InterlaceVideo())
   {
      return vlc_ == r9;
   }

   switch (type_crtc_)
   {
   case HD6845S:
      // C9 counts the lines of one field and the address uses C9.VMA = 2 x C9 | ParitéC9 : the character
      // ends when C9.VMA reaches R9 rounded to the parity, i.e. C9 == (R9 + 1 - ParitéC9) / 2 (19.8.1)
      return vlc_ == ((r9 + 1 - ParityC9Crtc0()) >> 1);
   case UM6845R:
      // C9 carries the parity and counts by 2 : compared without its bit 0, after adding !R9.0 (19.8.2)
      return ((vlc_ + ((r9 & 1) ? 0 : 1)) & 0x1E) == (r9 & 0x1E);
   default:
      // CRTC 2 : C9 is compared with R9 normally, the address uses the C9.IVM counter (19.8.3)
      return vlc_ == r9;
   }
}

// C9 used by the GATE ARRAY to build the address : C9.VMA = 2 x C9 | ParitéC9 on CRTC 0 in Interlace Video Mode
unsigned char CRTC::AddressC9() const
{
   if (type_crtc_ == HD6845S && ivm_latched_)
      return ((vlc_ << 1) | ParityC9Crtc0()) & 0x1F;
   // CRTC 2 : C9.VMA = 2 x C9.IVM | ParitéFrame, taken at once when R8 changes (19.8.3)
   if (type_crtc_ == MC6845 && InterlaceVideo())
      return ((c9_ivm_ << 1) | (even_field_ ? 0 : 1)) & 0x1F;
   return vlc_;
}

// CRTC 0/2 : ParitéR6 is loaded with the opposite of ParitéFrame on the rising edge of the C4==R6
// comparator (the one that sets the R6 border). With R6 > R4 it is never loaded : the parity freezes (19.5.2)
void CRTC::ClockParityR6()
{
   const bool eq = (vcc_ == registers_list_[6]);
   if (eq && !r6_eq_prev_)
   {
      parity_r6_ = even_field_;
   }
   r6_eq_prev_ = eq;
}

// MID-VSYNC : a VSYNC condition met on an even frame in interlace starts the VSYNC when C0 reaches R0/2 (19.7)
bool CRTC::ClockMidVSync()
{
   if (vsync_mid_pending_ && hcc_ == registers_list_[0] / 2)
   {
      vsync_mid_pending_ = false;
      return true;
   }
   return false;
}

// Register read on &BF00 (and &BE00 on CRTC 3/4) - Compendium 21.2
unsigned char CRTC::ReadRegister()
{
   if (type_crtc_ == AMS40489 || type_crtc_ == AMS40226)
   {
      // Only the 3 low bits of the selected register are used (21.2.3)
      switch (adddress_register_ & 0x07)
      {
      case 0:
         lightpen_input_ = false;
         return registers_list_[16];
      case 1:
         lightpen_input_ = false;
         return registers_list_[17];
      case 2: // Status 1 (R10) - 21.3.4.1
      {
         // Bits 3, 4, 5 (HSYNC start / end, VSYNC line) are latched by the tick ; the others decode
         // the counters and registers
         const unsigned char r0 = registers_list_[0];
         unsigned char status = (status1_ & 0x38) | 0x40;
         if (hcc_ == r0) status |= 0x01;
         if (hcc_ != r0 / 2) status |= 0x02;
         if (!(r0 >= registers_list_[1] && hcc_ == ((registers_list_[1] - 1) & 0xFF))) status |= 0x04;
         const bool vma_lsb_wraps = (hcc_ != r0) ? ((ma_ & 0xFF) == 0xFF) : ((bu_ & 0xFF) == 0x00);
         if (!vma_lsb_wraps) status |= 0x80;
         return status;
      }
      case 3: // Status 2 (R11) - 21.3.4.2
      {
         const bool c9_eq_r9 = (vlc_ == registers_list_[9]);
         const bool last_char_of_line = c9_eq_r9 && hcc_ == registers_list_[0];
         unsigned char status = 0x10;
         if (!(last_char_of_line && vcc_ == registers_list_[4])) status |= 0x01;
         if (!(last_char_of_line && vcc_ == ((registers_list_[6] - 1) & 0x7F))) status |= 0x02;
         if (!(last_char_of_line && vcc_ == ((registers_list_[7] - 1) & 0x7F))) status |= 0x04;
         if (frame_counter_ & 0x10) status |= 0x08;
         if (!c9_eq_r9) status |= 0x20;
         if (last_char_of_line || (vlc_ == 0 && hcc_ != registers_list_[0])) status |= 0x80;
         status2_ = status;
         return status;
      }
      case 4:
         return registers_list_[12];
      case 5:
         return registers_list_[13];
      case 6:
         return registers_list_[14];
      default:
         return registers_list_[15];
      }
   }

   if (adddress_register_ == 16 || adddress_register_ == 17)
   {
      lightpen_input_ = false;
   }

   if (adddress_register_ == 31 && type_crtc_ == UM6845R)
   {
      // Unused register on UM6845R, reads non-zero (21.2.2)
      status_register_ &= 0x7F;
      return 0xFF;
   }

   // Readable registers depend on the CRTC (21.2.1, 21.2.2) : any other one reads 0
   if ((CRTCAccess[adddress_register_][type_crtc_] & R) == R)
      return registers_list_[adddress_register_];
   return 0;
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
      // Adress = 0xBExx (Compendium 21.3)
      switch (type_crtc_ )
      {
      case UM6845R:
         // Status register : bit 6 = light pen, bit 5 = BORDER R6
         // Status register : bit 6 = light pen, bit 5 = BORDER R6 state, updated at the line end (21.3.3)
         return status_register_|(status_border_r6_?0x20:0x00)| (lightpen_input_?0x40:0);
      case AMS40489:
      case AMS40226:
         // Mirror of the read port
         return ReadRegister();
      default:
         // No status register on CRTC 0 and 2
         return 0xFF;
      }
   }
   else if (( address & 0x4300) == 0x0300)
   {
      // Adress = 0xBFxx
      return ReadRegister();
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
            const unsigned char previous_value = registers_list_[adddress_register_];
            registers_list_[adddress_register_] = (data & (registers_mask_[adddress_register_]));

            // Case of some type of CRTC - TODO
            switch ( adddress_register_)
            {
            case 0:

               // R0=0 is a valid value on every CRTC (Compendium 13.2.6 for CRTC 0)
               break;
            case 2:
               break;
            case 3:  // VSync width depends on the CRTC type*
               // A new VSYNC width is only compared when C3h is incremented (Compendium 14.2)
               ComputeSyncWidths();
               //ComputeMux_1 ();

               break;
            case 5:
               {
                  // CRTC 1 : R5 going from 0 to another value while C0==R0 changes the R5 comparator while the
                  // line end is being processed : R.F.D., VMA is reloaded from R12/R13 whatever C4 (11.6)
                  if (type_crtc_ == UM6845R && previous_value == 0 && registers_list_[5] != 0 && hcc_ == registers_list_[0])
                  {
                     rfd_ = true;
                  }
                  break;
               }
            case 8:
               {
                  // The interlace parity flip-flops are clocked by the R8 write (19.5.3, 19.5.5)
                  const bool ivm_before = ((previous_value & 0x03) == 0x03);
                  const bool ivm_after = InterlaceVideo();
                  if (type_crtc_ == UM6845R && ivm_before != ivm_after)
                  {
                     const unsigned int c4_odd_r9_even = (vcc_ & 1) & ((registers_list_[9] & 1) ^ 1);
                     unsigned int parity_frame = even_field_ ? 0 : 1;
                     unsigned int parity_c9 = (vlc_ & 1) ^ c4_odd_r9_even;
                     if (ivm_after)
                     {
                        if (parity_frame == 0)
                           parity_c9 = c4_odd_r9_even;
                        parity_frame = parity_frame & (parity_c9 ^ c4_odd_r9_even);
                     }
                     else
                     {
                        parity_frame = parity_c9;
                     }
                     parity_c9_ = (parity_c9 != 0);
                     even_field_ = (parity_frame == 0);
                  }
                  else if ((type_crtc_ == AMS40489 || type_crtc_ == AMS40226) && (previous_value & 0x01) == 0 && InterlaceOn())
                  {
                     parity_c9_ = (vlc_ & 1) != 0;
                  }
                  break;
               }
            case 9:
               {
                  r9_triggered_ = vlc_ == registers_list_[9];
                  break;
               }

            case 12:
            case 13:
               {
               break;
               }
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
   registers_mask_[8] = R8Mask(type_crtc_);
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
   signals_->v_sync_ = VSyncPin();

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
