#pragma once

//#include "MMsystem.h"
#include "IComponent.h"
#include "IConfiguration.h"
#include "SoundMixer.h"
#include "Tape.h"
#include "ISound.h"
#include "KeyboardHandler.h"

class Ay8912 : public IComponent
{
friend class EmulatorEngine;
friend class CSnapshot;
public:
   
   Ay8912(SoundMixer *sound_hub, IKeyboardHandler* keyboard_handler);
   virtual ~Ay8912(void);

   virtual void SetDirectories(IDirectories * dir) {directories_ = dir;   };
   virtual void SetConfigurationManager(IConfiguration * configuration_manager) { configuration_manager_ = configuration_manager; }
   void Reset ();
   // BusCTRL : BC2, BDIR, BC1
   void Access( unsigned char *data_bus , unsigned char bus_ctrl, unsigned char data);

   void InitSound (ISound* sound);
   unsigned int Tick ( );

   unsigned char GetRegisterAdress() {
      return register_address_
         ;
   }
protected :

   void TickSound ();

   // Registers - 12 bits for theses
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

   unsigned char enveloppe_volume_;

   unsigned int volume_enveloppe_frequency_;
   unsigned char volume_enveloppe_shape_;

   unsigned char external_data_register_b_;

   // Envelope
   bool enveloppe_up_;
   bool enveloppe_stop_;

   /////////////////////////////////
   // Keyboard
   IKeyboardHandler* keyboard_handler_;
   bool register_replaced_;
   unsigned char register_14_;

   /////////////////////////////////
   // Precise sound output
   unsigned int counter_a_;
   unsigned int counter_b_;
   unsigned int counter_c_;
   unsigned int counter_noise_;
   unsigned int counter_env_;
   unsigned int counter_state_env_;

   unsigned char chan_a_high_;
   unsigned char chan_b_high_;
   unsigned char chan_c_high_;
   unsigned char chan_noise_high_;

   unsigned int noise_shift_register_;

   ISound* sound_;

   unsigned char register_[16];

   SoundSource sound_source_;
   SoundMixer *sound_hub_;

   IConfiguration * configuration_manager_;
   IDirectories * directories_;
};

