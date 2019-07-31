#pragma once

/*
 * (C) Copyright 2017 Thomas GUILLEMIN
 * 
 * 
 *
 */

#include "YMZ294.h"
#include "Z84C30.h"
#include "IExpansion.h"

class PlayCity : public IExpansion
{
public:
   PlayCity(IClockable* int_line, IClockable* nmi_line, /*SoundMixer *mixer, */SoundMixer *sound_hub);
   virtual ~PlayCity(void);

   virtual unsigned int Tick();
   virtual void Out( unsigned short address, unsigned char data);
   virtual void In(unsigned char* data, unsigned short address);

   Z84C30* GetCtc() {return &z84c30_;}
   virtual void Reset();

   class TRG0 : public IClockable
   {
   public:
      TRG0(PlayCity* playcity) :playcity_(playcity){};
      virtual unsigned int Tick() { playcity_->drop_next_tick_ = true; return 1; };
      PlayCity* playcity_;
   };

protected:

   // A Playcity is :
   // - 1 Z84C30
   Z84C30 z84c30_;
   // - 2 YMZ294
   YMZ294 ymz294_1_;
   YMZ294 ymz294_2_;

   TRG0 trg0_;
   bool trg0_update_;
   // ClockLine : 
   // z84 -> both clk of ymz294
   CClockLine inner_line_;
   CClockLine inner_line_channel_23_;

   // Shall the next YMZ tick be dropped (due to TRG0) ?
   bool drop_next_tick_;

   // YMZ count to avoid multiple & useless calls
   int next_call_ymz_;
};

