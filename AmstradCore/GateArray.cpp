
#include <iostream>
#include <sstream>

#include "GateArray.h"

///////////////////////////////////////
// GateArray
//
GateArray::GateArray() : counter(0), line_4_mhz_(nullptr), line_CCLK_mhz_(nullptr)
{
   
}
GateArray::~GateArray()
{
   
}

void GateArray::Create()
{
   Reset();
}

void GateArray::Reset()
{
   // Reset inner counters
   
}

void GateArray::Sequencer()
{
   // reset to 11111111 if
   // - IRQ ack from z80
   // - s & 0xC0 = 0x40
   if ( ( (s_ & 0xC0 ) == 0x40)
      || ((!line_m1_->GetLevel())&(!line_iorq_->GetLevel())& (!line_rd_->GetLevel()) & (line_reset_->GetLevel())
         )
      )
   {
      s_ = 0xFF;
   }
   else
   {
      s_ = (s_ & 0x80) ? (s_ << 1) : ((s_ << 1) | 1);
   }
}

void GateArray::SequencerDecode()
{
   // PHI :
   switch (s_ & 0xAA)
   {
   case 0:
   case 0xA:
   case 0xAA:
   case 0xA0:
      line_4_mhz_->ForceLevel(0);
      break;
   default:
      line_4_mhz_->ForceLevel(1);
      break;
   }

   // RAS
   // CASAD

   // READY :
   // if ( ((S6 ||(S7)) || S0 ) == 0 ) & Ready) || (S3 & !S6)
   // compute  READY
   line_ready_->ForceLevel( (( s_>>3) & (!(s_>>6))) | (line_ready_->GetLevel()& u313_ ));

   u313_ = (~u312_)&0x1;
   u312_ = (((s_ >> 6) | (~(s_ >> 2))) & s_ ) & 0x1;


   // CPU : ~(S1 & (!S7))
   line_CPU_ADDR_mhz_->ForceLevel((s_& 0x82) != 0x02 );

   // CLK
   line_CCLK_mhz_->ForceLevel((s_&0x24) == 0);

   // MWE
   // 244E

}

void GateArray::TickUp()
{
   SequencerDecode();

   Sequencer();
}

void GateArray::TickDown()
{
}

