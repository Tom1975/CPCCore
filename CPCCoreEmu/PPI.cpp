#include "stdafx.h"
#include "PPI.h"
#include "PSG.h"
#include "Sig.h"
#include "Tape.h"


#define LOG_PPI

#ifdef LOG_PPI
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

PPI8255::PPI8255() : plus_(false)
{
   Reset();
}

PPI8255::~PPI8255()
{

}

void PPI8255::Reset()
{
   port_b_ = 0x1E;
   tape_level_ = 0;
   inner_control_.byte = 0x7F;
   // Default inner values : All ports in input mode
   control_word_.control_word.io_a = 1;
   control_word_.control_word.io_b = 1;

   // plus : Port C is always output
   if (plus_)
   {
      control_word_.control_word.io_high_c = 0;
      control_word_.control_word.io_low_c = 0;
   }
   else
   {
      control_word_.control_word.io_high_c = 1;
      control_word_.control_word.io_low_c = 1;
   }

   // All port latches are cleared to 0
   port_a_ = 0;
   port_c_ = 0;

   // All ports group are set to mode 0
   control_word_.control_word.mode_a = 0;
   control_word_.control_word.mode_b = 0;
}

void PPI8255::DataRead(unsigned char* data, unsigned char Adress)
{
   // Read something : Depend on the address ! : RD is low / WR is high / CS is 0.
   // Operation Output.
   switch (Adress & 0x3)
   {
      //
      case 0: // Port A -> Data Bus
         {
            // Depends on the mode :
            switch (control_word_.control_word.mode_a)
            {
            case 0: // Propagate
               if ( control_word_.control_word.io_a)
               {
                  psg_->Access(&port_a_, (port_c_ >> 6) | 0x4, port_c_ & 0xF);
                  *data = port_a_;
               }
               else
               {
                  *data = port_a_;
               }
               break;
            case 1:
               // RD :
               *data = port_a_ ;
               // Reset the IBF
               inner_control_.inner_control.control_a.ibf = 1;
               break;
            case 2:
               // R/W : No need of control word
               break;
            }

         }
         break;
      case 1: // Port B -> Data Bus
            // Depends on the mode :
            switch (control_word_.control_word.mode_b)
            {
            case 0: // Propagate
               if ( control_word_.control_word.io_b ||plus_)
               {
                  // Check true values : verything is linked (or almost)
                  port_b_ = tape_level_| 0x1E|(sig_->v_sync_?1:0)|((sig_->printer_port_!=NULL)?(sig_->printer_port_->Busy()?0x40:0):0x40);
                  *data = port_b_ ;
               }
               else
               {
                  *data = port_b_;
               }
               break;
            case 1:
               // RD :
               if ( control_word_.control_word.io_b || plus_)
               {
                  port_b_ = tape_level_ | 0x1E|(sig_->v_sync_?1:0)|((sig_->printer_port_!=NULL)?(sig_->printer_port_->Busy()?0x40:0):0);
                  *data = port_b_ ;
                  // Reset the IBF
                  inner_control_.inner_control.control_b.input.ibf = 1;
               }
               else
               {
                  *data = port_b_;
               }
               break;
            }
         break;
      case 2: // Port C -> Data Bus
         {
            unsigned char port_c = 0;
            // Depends on various things :
            // mode, i/o, etc
            switch ( control_word_.control_word.mode_a)
            {
            case 0:
               // Ok.
               if ((!control_word_.control_word.io_high_c) || plus_)
               {
                  port_c |= port_c_ & 0xF0;
               }
               else
               {
                  port_c |=  0x2F & 0xF0;
               }
               if ((!control_word_.control_word.io_low_c) || plus_)
               {
                  port_c |= port_c_ & 0x08;
               }
               else
               {
                  port_c |= 0x2F & 0x08;
               }
               break;
            case 1:
               if ((!control_word_.control_word.io_high_c) || plus_) // Input
               {
                  if ( !control_word_.control_word.io_a)
                  {
                     port_c |= inner_control_.byte & 0xC0;
                     port_c |= port_c_ & 0x30;
                  }
                  else
                  {
                     port_c |= inner_control_.byte & 0x30;
                     port_c |= port_c_ & 0xC0;
                  }
               }
               else
               {
                  // 0x2F for CPC, 0x1F for KC compact
                  port_c |=  0x2F & 0xF0;
               }
               if ((!control_word_.control_word.io_low_c) || plus_)
               {
                  if ( control_word_.control_word.io_a)
                  {
                     port_c |= inner_control_.byte & 0x08;
                  }
                  else
                  {
                     port_c |= inner_control_.byte & 0x08;
                  }
               }
               else
               {
                  // 0x2F for CPC, 0x1F for KC compact
                  port_c |= 0x2F & 0x08;
               }
               break;
            case 2:
               if ((!control_word_.control_word.io_high_c) || plus_)
               {
                  port_c |= inner_control_.byte & 0xF0;
               }
               else
               {
                  // 0x2F for CPC, 0x1F for KC compact
                  port_c |= 0x2F & 0xF0;

               }
               if ((!control_word_.control_word.io_low_c) || plus_)
               {
                  port_c |= inner_control_.byte & 0x08;
               }
               else
               {
                  // 0x2F for CPC, 0x1F for KC compact
                  port_c |= 0x2F & 0x08;
               }
               break;
            }

            switch ( control_word_.control_word.mode_b)
            {
            case 0:
               if ((!control_word_.control_word.io_low_c) || plus_)
               {
                  port_c |= port_c_ & 0x07;
               }
               else
               {
                  port_c |= 0x07;
               }
               break;

            case 1:
               if ((!control_word_.control_word.io_low_c) || plus_)
               {
                  if ( control_word_.control_word.io_b || plus_)
                  {
                     port_c |= inner_control_.byte & 0x07;
                  }
                  else
                  {
                     port_c |= inner_control_.byte & 0x07;
                  }
               }
               else
               {
                  port_c |= *data & 0x07;
               }
               break;
            }

            *data = port_c;
            break;
         }
      case 3: // Illegal condition :
         // 0x07f in type 2 and type 1 cpc6128 and cpc664
         // 0x0ff in type 0 cpc6128..depends on last value written though ? ? ?
         // 0x01a on kcc
         // 0xff in plus
         // todo : Add specific
         *data = 0x7F;
         break;
   }
}

void PPI8255::DataWrite(unsigned char* data, unsigned char Adress)
{
   unsigned char inVal = *data;

   // Write something : Depend on the address ! : RD is high / WR is low / CS is 0.
   // Operation Output.
   switch (Adress & 0x3)
   {
      //
   case 0: // Data Bus -> Port A
      if ( !control_word_.control_word.io_a || plus_) // always set to write if write is asked, on the plus
      {
         port_a_ = inVal;
         // Update signals if mode 1/2 ?
         //2 / Update control if mode 1 / 2
         switch (control_word_.control_word.mode_a)
         {
         case 0: // Propagate
         {
            unsigned char port_a = port_a_;
            psg_->Access(&port_a, (port_c_ >> 6) | 0x4, port_c_ & 0xF);
            break;
         }
         case 1:
            // OBF to low
            inner_control_.inner_control.control_a.obf = 0;
            break;
         case 2:
            // Nothing : all is control from C port
            break;
         }
      }
      else
      {
         // Port A is input :
         port_a_ = 0xFF;
      }
      break;
   case 1: // Data Bus -> Port B
      {
         if ( !control_word_.control_word.io_b && (!plus_))
         {
            port_b_ = inVal;
            // Update signals if mode 1/2 ?
            UpdateSignalForPortB ();
         }
      }
      break;
   case 2: // Data Bus -> Port C
      if (control_word_.control_word.mode_a == 0 && control_word_.control_word.mode_b == 0)
      {
         if (( !control_word_.control_word.io_low_c ) || plus_)
         {
            port_c_ &= 0xF0;
            port_c_ |= (inVal&0x0F);
         }
         if (( !control_word_.control_word.io_high_c ) || plus_)
         {
            port_c_ &= 0x0F;
            port_c_ |= (inVal&0xF0);
         }
      }
      else
      {
         if (( !control_word_.control_word.io_low_c ) || plus_)
         {
            port_c_ &= 0xF8;
            port_c_ |= (inVal&0x07);
         }
         else
         {
            port_c_ |= 0x07;
         }
      }

      if (control_word_.control_word.mode_a == 0 && control_word_.control_word.mode_b == 0)
      {
         TraitePortC ();
      }
      else
      {
         TraitePortCAsCtrl ();
      }
      break;
   case 3: // Data Bus -> Control Register
      if (inVal & 0x80)
      {
         // Control word
         control_word_.byte = inVal & 0x7F;

         if ( control_word_.control_word.io_b || plus_)
         {
            port_b_ = tape_level_ |0x1E|(sig_->v_sync_?1:0)|((sig_->printer_port_!=NULL)?(sig_->printer_port_->Busy()?0x40:0):0);
         }
         else
         {
            port_b_ = 0;
         }
         if (!plus_)
         {
            // Only on 8255. CPC+ is different (see a different implementation) - if mode changed
            port_a_ = 0;
            port_c_ = 0;
         }
      }
      else
      {
         // Port C set/reset :
         unsigned num = ((inVal & 0x0E) >> 1);

         // Check if IO is ok on this quartet
         // Num bit a modifier
         unsigned char b = inVal & 0x1;
         b = b << num;
         unsigned char c = 1 << num;
         c = ~c;

         // Operation
         port_c_ &= c;
         port_c_ |= b;

         // Propagate if proper mode for port A or B
         if (control_word_.control_word.mode_a == 0 && control_word_.control_word.mode_b == 0)
         {
            // Propagate to keyboard line
            TraitePortC();
         }
         else
         {
            // Control word for mode 1/2 of port A or mode 1 of port B
            TraitePortCAsCtrl();
         }
      }
      break;
   }
}

void PPI8255::UpdateSignalForPortB ()
{
   // 2/ Update control if mode 1/2
   switch (control_word_.control_word.mode_b)
   {
   case 0: // Propagate
      if (false)
      {
         /*m_bVbl = */sig_->v_sync_ = ((port_b_&0x1)==1);
      }

      break;
   case 1:
      // OBF to low
      inner_control_.inner_control.control_b.output.obf = 0;
      break;
   }
}

void PPI8255::TraitePortCAsCtrl()
{
   // Port A
   if (control_word_.control_word.mode_a == 1)
   {
      // INPUT
         // STB falling ?
         // Yes : Input A => Port A
         // and Set IBF to High,
         // STB rising : IBF is High, set INTR

      // OUTPUT
      // Nothing to do here ?
         //
   }
   else if (control_word_.control_word.mode_a == 2)
   {
   }

   // Port B
   if (control_word_.control_word.mode_b == 1)
   {
   }

}

// C Port handling : No mode 1,2 (port A) or 1 (port B)
void PPI8255::TraitePortC()
{
   if (tape_ != NULL )
   {
      tape_->SetMotorOn((port_c_ & 0x10) == 0x10);

      tape_write_data_level_ = ((port_c_ & 0x20) == 0x20);

   }

   // Si bit 7,8 � 1, on charge l'adresse du PSG a partir a A
   unsigned char port_a = port_a_;
   psg_->Access(&port_a, (port_c_ >> 6) | 0x4, port_c_ & 0xF);

   if ( control_word_.control_word.io_a == 1)
      port_a_ = port_a;
}

void PPI8255::SetDataRecorderR ( bool set )
{
   if (set)
   {
      tape_level_ = 0x80;
   }
   else
   {
      tape_level_ = 0;
   }
}

void PPI8255::SetPrinterBusy ( bool set )
{
   if (set)
   {
      port_b_ |= 0x40;
   }
   else
   {
      port_b_ &= 0xBF;
   }
}

void PPI8255::SetExpSignal ( bool set )
{
   if (set)
   {
      port_b_ |= 0x20;
   }
   else
   {
      port_b_ &= 0xDF;
   }
}

void PPI8255::SetScreenState ( bool set )
{
   if (set)
   {
      port_b_ |= 0x10;
   }
   else
   {
      port_b_ &= 0xEF;
   }
}

void PPI8255::SetVbl ( bool set )
{
   if (set)
   {
      port_b_ |= 0x1;
   }
   else
   {
      port_b_ &= 0xFE;
   }
}

void PPI8255::SetAmstradType( unsigned char Type )
{
   port_b_ &= 0xF1;
   port_b_ |= (Type&0x0E);
}
