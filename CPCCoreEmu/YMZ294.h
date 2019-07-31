#pragma once

/*
 * (C) Copyright 2017 Thomas GUILLEMIN
 * 
 * 
 *
 */

#include "IClockable.h"
#include "SoundMixer.h"


const int CLOCK_DIV = 0;


class YMZ294 : public IClockable
{
public:
   typedef enum {
      LEFT, 
      RIGHT, 
      BOTH
   } Output;

   YMZ294(/*SoundMixer * mixer, */Output output, SoundMixer *sound_hub);
   virtual ~YMZ294(void);

   void Reset();
   bool Access(unsigned char *data_bus, unsigned char bus_ctrl, unsigned char data);
   virtual unsigned int Tick();


protected:

   
   // Sound Mixer
   Output output_;
   //SoundMixer * mixer_;
   SoundSource sound_source_;

   // Registers - 12 bits for theses
   unsigned char register_[16];
   unsigned char register_address_;

   unsigned int channel_a_freq_;
   unsigned int channel_a_freq_counter_;

   unsigned int channel_b_freq_;
   unsigned int channel_b_freq_counter_;

   unsigned int channel_c_freq_;
   unsigned int channel_c_freq_counter_;

   unsigned int noise_frequency_;
   unsigned char mixer_control_register_;

   unsigned char channel_a_volume_;
   unsigned char channel_b_volume_;
   unsigned char channel_c_volume_;

   unsigned char envelope_volume_;

   unsigned int volume_enveloppe_frequency_;
   unsigned char volume_enveloppe_shape_;

   // Envelope
   bool up_;
   bool stop_;

   /////////////////////////////////
   // Precise sound output
   unsigned int counter_a_;
   unsigned int counter_b_;
   unsigned int counter_c_;
   unsigned int counter_noise_;
   unsigned int counter_env_;
   unsigned int counter_state_env_;

   unsigned char channel_a_high_;
   unsigned char channel_b_high_;
   unsigned char channel_c_high_;
   unsigned char channel_noise_high_;

   unsigned int noise_shift_register_;
};

