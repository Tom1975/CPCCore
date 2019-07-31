#include "stdafx.h"
#include "Z84C30.h"


Z84C30::CTCCounter::CTCCounter() : output_(nullptr), interrupt_(nullptr), prescaler_(0), wait_for_time_constraint_(false),
                                    control_(0), count_enabled_(false), decrement_(0), time_constant_(0), down_counter_(0)
{
}

Z84C30::CTCCounter::~CTCCounter()
{

}

void Z84C30::CTCCounter::SetOutput(IClockable* output, IClockable* interrupt)
{
   output_ = output;
   interrupt_ = interrupt;
}

int Z84C30::CTCCounter::Trg(bool up)
{
   if (((control_ & CTRL_EDGE) && up)
      ||((control_ & CTRL_EDGE) == 0 && (!up)))
   {
      decrement_ = true;
   }
   if (!count_enabled_)
   {
      prescaler_ = (control_ & CTRL_PRESCALER) ? 256 : 16;
      count_enabled_ = true;
      //down_counter_ = time_constant_;
      reload_ = true;
   }
   return 1;
}


void Z84C30::CTCCounter::Out(unsigned char data)
{
   if (wait_for_time_constraint_)
   {
      time_constant_ = (data==0)?256:data;
      wait_for_time_constraint_ = false;

      count_enabled_ = ((control_ & CTRL_TIMER)==0) ;


      // todo : if count_enabled_, add shift for counter (compute from write cycle to T2 of next M1 operation

      // If timer mode & CTRL_TIMER, trigger it
      if (count_enabled_)
      {
         prescaler_ = (control_ & CTRL_PRESCALER) ? 256 : 16;
         //down_counter_ = time_constant_;
         reload_ = true;
      }
      enabled_ = true;

   }
   else
   {
      control_ = data;

      wait_for_time_constraint_ = control_ & CTRL_TIME_CST;
      enabled_ = true;

      // Something to set ?
      if (control_ & CTRL_RESET)
      {
         // Software reset
         Reset(false);
      }
   }
}

void Z84C30::CTCCounter::Reset(bool hard)
{

   if (hard)
   {
      wait_for_time_constraint_ = false;
      // Terminate all down-counts
      down_counter_ = 0;
      // Disable interrupts
      control_ &= 0x7F;
      enabled_ = false;

   }
   else
   {
      // Stop counting
      count_enabled_ = false;

   }
}

unsigned int Z84C30::CTCCounter::Tick()
{
   // Timer mode
   if (count_enabled_ && (wait_for_time_constraint_ == false ) && enabled_)
   {
      if (reload_)
      {
         down_counter_ = time_constant_;
         reload_ = false;
      }
      else
      {
         if (control_ & CTRL_MODE)
         {
            // Decrement ?
            if (decrement_)
            {
               decrement_ = false;
               --down_counter_;
            }
         }
         else
         {
            --prescaler_;
            if (prescaler_ == 0)
            {
               prescaler_ = (control_ & CTRL_PRESCALER) ? 256 : 16;
               --down_counter_;
            }
         }
      }
      if (down_counter_ == 0)
      {
         reload_ = true;

         // Triger or interrupt !
         if (control_ & CTRL_INTERRUPT)
         {
            if (interrupt_)
            {
               interrupt_->Tick();
            }
         }
         //else
         {
            if (output_)
            {
                  output_->Tick();
            }
         }
      }
   }
   return 1;
}



Z84C30::Z84C30()
{

}


Z84C30::~Z84C30(void)
{
}

void Z84C30::Init(ChannelId channel, IClockable* out_line, IClockable* int_line)
{
   channel_[channel].SetOutput(out_line, int_line);
}

void Z84C30::HardReset()
{
   // IE0 = IEI
   channel_[0].Reset(true);
   channel_[1].Reset(true);
   channel_[2].Reset(true);
   channel_[3].Reset(true);

}

IClockable* Z84C30::GetCounter(ChannelId channel)
{
   switch (channel)
   {
   case CHANNEL_0:
      return &channel_[0];
   case CHANNEL_1:
      return &channel_[1];
   case CHANNEL_2:
      return &channel_[2];
   case CHANNEL_3:
      return &channel_[3];
   }
   return nullptr;
}

unsigned int Z84C30::Tick()
{

   channel_[0].Tick();
   channel_[1].Tick();
   channel_[2].Tick();
   channel_[3].Tick();
   return 1;
}

void Z84C30::M1()
{
   // Arbitrate the CTC internal priorities
   // Set interrupt vector
}

// address : 2 bits : CS0, CS1
void Z84C30::Out(unsigned short address, unsigned char data)
{
   // Send control word to proper channel
   if (address == 0 && ((data & 0x1) == 0) && (channel_[0].wait_for_time_constraint_ == false))
   {
      interrupt_vector_ = data;
   }
   else
   {
      channel_[address & 0x3].Out(data);
   }

}

void Z84C30::In(unsigned char* data, unsigned short address)
{
}
