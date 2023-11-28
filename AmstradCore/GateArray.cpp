
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
   // CASAD - todo
}

void GateArray::SequencerDecode()
{
   // PHI :
   line_4_mhz_->ForceLevel((~ (((s_ >> 1) ^ (s_ >> 3)) | ( (s_>>5)^(s_>>7)))) & 0x1);
   u312_ = (((s_ >> 6) | (~(s_ >> 2))) & s_) & 0x1;

   // RAS - todo


}

// TODO : find the best place to call this (RAM access ?)
void GateArray::RomMapping()
{
   // Input : LROMEN, A15, A14, HROMEN, MREQ, RD
   // Output : ROMEN, RAMRD
   unsigned char u601 = ((lromen_?0:1) & (~(bus_address_->data_ >> 15)) & (~(bus_address_->data_ >> 14)) ) & 0x01;
   unsigned char u602 = (bus_address_->data_ >> 15) & (bus_address_->data_ >> 14) & (hromen_ ? 0 : 1);
   unsigned char u603 = (u601 | u602) & 0x1;
   romen_ = ((~u603) | line_rd_->GetLevel() & line_mreq_->GetLevel())& 0x1;
   ramrd_ = ((u603) | line_rd_->GetLevel() & line_mreq_->GetLevel()) & 0x1;
}

void GateArray::CasGeneration()
{
   // todo
}

void GateArray::InkRegister()
{
   // Input : INKSEL; DATA(0-4), INKRE
   // output : INKR

   // 
   // TODO
   // 
}

void GateArray::HSync(bool set)
{
   if (set)
   {
      // Counters
      u815 = ~u815 & 0x1;
      if (!u815)
      {
         u821 = ~u821 & 0x1;
         if (!u821)
         {
            u826 = ~u826 & 0x1;
            if (!u826)
            {
               u830 = ~u830 & 0x1;
               if (!u830)
               {
                  u832 = ~u832 & 0x1;
                  if (!u832)
                  {
                     u835 = ~u835 & 0x1;
                     if (!u835)
                     {
                        // Clock u836
                        u836 = 1;
                        line_int_->ForceLevel(0);
                        // todo : check int + m1 + iorq or reset
                     }
                  }
               }
            }

            u816 = (u826 & u832 & u835 & 0x1) | u816;
         }
      }

      // Reset U808-13-18-24
      u808 = u813 = u818 = u824 = 0;
      mode_sync = false;

      // Clock U809
      u809 = ~u809 & 0x1;
      if (!u809)
      {
         u814 = ~u814 & 0x1;
         if (!u814)
         {
            u820 = ~u820 & 0x1;
            if (!u820)
            {
               u825 = ~u825 & 0x1;
               if (!u825)
               {
                  u829 = ~u829 & 0x1;
                  if (!u829)
                  {

                  }
               }
            }
         }
      }
      nsync_ = !(false ^ (u820 & (~u825) & (~u829)));
      
      // Reset HCNT ?
      if (u820 & u825 & u829 & 0x1) 
      {
         // reset
         u809 = u814 = u820 = u825 = u829 = 0;
         hcntlt28_ = false;
      }
      else
      {
         hcntlt28_ = true;
      }

      if (u820 & (~u825) & (~u829) & 0x1)
      {
         u817 = (~u812) & 0x1;
      }
      else
      {
         u817 = false;
      }
      if (u816 | u817)
      {
         u815 = u821 = u826 = u830 = u832 = u835 = 0;
      }
   }
   else
   {
      u815 = u821 = u826 = u830 = u832 = u835 = 0;
      u816 = 0;
   }

   
}

void GateArray::VSync(bool set)
{
   // Input : HSYNC, CCLK, RESET, VSYNC, IORQ, M1, IRQ_RESET
   // Output : INT, NSYNC, MODE_SYNC, HCNTL28
}

void GateArray::RegisterDecode()
{
   // Only active if M1, A15/14 = 01, IORQ, S0S7 =11
   if (((s_ & 0x81) == 0x81)
      && (line_iorq_->GetLevel() == 0)
      && ((bus_address_->data_ & 0xC000) == 0x4000)
      && (line_m1_->GetLevel() == 1)
      )
   {
      switch (bus_data_->data_ & 0xC0)
      {
      case 0x00:
         inksel_ = bus_data_->data_ & 0x1F;
         break;
      case 0x80:
         hromen_ = bus_data_->data_ & 0x8;
         lromen_ = bus_data_->data_ & 0x4;
         mode_ = bus_data_->data_ & 0x3;
         break;
      case 0x40:
         if (inksel_ & 0x10)
         {
            // todo handle reset 
            border_ = bus_data_->data_ & 0x1F;
         }
         inkre_ = true;
         // Ink register
         InkRegister();

         break;
      case 0xC0:
         // Nothing !
         break;
      }
   }
   irq_reset_ = (((s_ & 0x81) == 0x81)
      && (line_iorq_->GetLevel() == 0)
      && ((bus_address_->data_ & 0xC000) == 0x4000)
      && (line_m1_->GetLevel() == 1)
      && ((bus_data_->data_ & 0xD0) == 0x50)
      );

}

void GateArray::TickUp()
{
   ////////////////////////
   // These sequences needs clock and s_ register
   SequencerDecode();
   CasGeneration();


   ////////////////////////
   // Finalise : All that does immédiately need s_.
   Sequencer();

   // WAIT
   line_ready_->ForceLevel((((s_ >> 3) & (~(s_ >> 6))) | (line_ready_->GetLevel() & u313_)) & 0x1);

   // Set the immediate value from s_
   // CPU : ~(S1 & (!S7))
   line_CPU_ADDR_mhz_->ForceLevel((~(S(1) & (~S(7)))) & 0x1);

   // CLK
   line_CCLK_mhz_->ForceLevel((~(S(2) | S(5)))&0x1);

   // MWE - todo
   // 244E - todo
}

void GateArray::TickDown()
{
   SequencerDecodeDown();
   line_ready_->ForceLevel((((s_ >> 3) & (~(s_ >> 6))) | (line_ready_->GetLevel() & u313_)) & 0x1);
}

