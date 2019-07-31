#pragma once

/*
 * (C) Copyright 2017 Thomas GUILLEMIN
 * 
 * 
 *
 */

#include "IClockable.h"
#include "ClockLine.h"

class Z84C30 : public IClockable
{
public:

   enum ChannelId
   {
      CHANNEL_0 = 0,
      CHANNEL_1,
      CHANNEL_2,
      CHANNEL_3,
   } ;

   Z84C30();
   virtual ~Z84C30(void);

   
   void HardReset();
   virtual unsigned int Tick();
   virtual void M1 ();

   // Adress : CS0, CS1; 
   virtual void Out( unsigned short address, unsigned char data);
   virtual void In(unsigned char* data, unsigned short address);

   void Init(ChannelId channel, IClockable* out_line, IClockable* int_line);
   IClockable* GetCounter(ChannelId channel);

public:
   class CTCCounter : public IClockable
   {
   public:
      enum ControlWord
      {
         CTRL_CONTROL_VECTOR  = 0x01,
         CTRL_RESET           = 0x02,
         CTRL_TIME_CST        = 0x04,
         CTRL_TIMER           = 0x08,
         CTRL_EDGE            = 0x10,
         CTRL_PRESCALER       = 0x20,
         CTRL_MODE            = 0x40,
         CTRL_INTERRUPT       = 0x80
      };

      CTCCounter();
      virtual ~CTCCounter();

      virtual void Reset(bool hard);
      virtual void SetOutput(IClockable* output, IClockable* interrupt);
      virtual void Out (unsigned char data);
      virtual int Trg( bool up);
      virtual unsigned int Tick();

   public:
      bool enabled_;
      unsigned char control_;
      unsigned char time_constant_;
      unsigned char down_counter_;
      unsigned char prescaler_;
      bool decrement_;
      bool wait_for_time_constraint_;
      bool count_enabled_;
      bool reload_;
      IClockable* output_;
      IClockable* interrupt_;
   };

   unsigned char interrupt_vector_;
   

   CTCCounter channel_[4];
};
