
#include <iostream>
#include <sstream>

#include "GateArray.h"

#define S(n)(s_>>n)

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

void GateArray::SequencerDecodeDown()
{
   u313_ = (~u312_) & 0x1;
}

void GateArray::SequencerDecode()
{
   // PHI :
   line_4_mhz_->ForceLevel((~ (((s_ >> 1) ^ (s_ >> 3)) | ( (s_>>5)^(s_>>7)))) & 0x1);
   u312_ = (((s_ >> 6) | (~(s_ >> 2))) & s_) & 0x1;


   // RAS
   // CASAD

}

void GateArray::TickUp()
{
   SequencerDecode();

   Sequencer();

   // WAIT
   line_ready_->ForceLevel((((s_ >> 3) & (~(s_ >> 6))) | (line_ready_->GetLevel() & u313_)) & 0x1);

   // Set the immediate value from s_
   // CPU : ~(S1 & (!S7))
   line_CPU_ADDR_mhz_->ForceLevel((~(S(1) & (~S(7)))) & 0x1);

   // CLK
   line_CCLK_mhz_->ForceLevel((~(S(2) | S(5)))&0x1);

   // MWE
   // 244E


}

void GateArray::TickDown()
{
   SequencerDecodeDown();
   line_ready_->ForceLevel((((s_ >> 3) & (~(s_ >> 6))) | (line_ready_->GetLevel() & u313_)) & 0x1);
}

