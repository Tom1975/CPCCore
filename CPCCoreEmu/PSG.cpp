#include "stdafx.h"
#include "PSG.h"

#include "simple_string.h"
#include "simple_filesystem.h"
#include "IDirectories.h"

extern const char * SugarboxPath;

// The measured levels of the 4-bits DAC output as UINT16 are:
// MEASURE = [0,231,695,1158,2084,2779,4168,6716,8105,13200,18294,24315,32189,40757,52799,65535]

// The following lookup tables are based on the measured values and calculated like this:
// MONO = (MEASURE/2) * 0.333;
// SIDE = (MEASURE/2) * 0.687;
// CENTER = (MEASURE/2) * 0.313;

Ay8912::Ay8912( SoundMixer *sound_hub, IKeyboardHandler* keyboard_handler) :
   sound_source_(sound_hub), 
   sound_hub_(sound_hub), 
   directories_(nullptr), 
   keyboard_handler_(keyboard_handler)
{
   keyboard_handler_->Init(&register_replaced_);
   sound_ = nullptr;

   Reset();
}


Ay8912::~Ay8912(void)
{
}

void Ay8912::Reset ()
{
   memset (register_, 0, sizeof(register_));
   channel_a_freq_ = 1;
   channel_b_freq_ = 1;
   channel_c_freq_ = 1;

   noise_frequency_ = 1/*<<3*/;
   mixer_control_register_ = 0xFF;

   noise_shift_register_ = 1;

   channel_a_volume_ = 0;
   channel_b_volume_ = 0;
   channel_c_volume_ = 0;

   volume_enveloppe_frequency_ = 1;
   volume_enveloppe_shape_ = 0;

   external_data_register_b_ = 0xFF;

   counter_a_ = 0;
   counter_b_ = 0;
   counter_c_ = 0;

   chan_a_high_ = 1;
   chan_b_high_ = 1;
   chan_c_high_ = 1;
   chan_noise_high_ = 1;

   counter_noise_ = 0;
   counter_env_ = 0;
   counter_state_env_ = 0;
   enveloppe_volume_ = 0;
   enveloppe_stop_ = false;

   register_14_ = 0;
   register_replaced_ = true;
   register_address_ = 0;
   sound_source_.Reinit();
}


#define KEY_BUFFER_SIZE 32

// BusCTRL : BC2, BDIR, BC1
void Ay8912::Access(unsigned char *data_bus, const unsigned char bus_ctrl, const unsigned char data)// data = line
{
   // Depends onthe BusCTRL
   if (bus_ctrl & 0x2)     // BDIR = 1
   {
      if (bus_ctrl & 0x1)
      {
         // Write address
         register_address_ = *data_bus;
      }
      else
      {
         switch (register_address_)
         {
         case 0x00: register_[register_address_] = *data_bus;channel_a_freq_ = ((channel_a_freq_ & 0xff00) + *data_bus); channel_a_freq_counter_ = (channel_a_freq_ /*<< 3*/); break;
         case 0x01: register_[register_address_] = *data_bus&0x0F;channel_a_freq_ = ((channel_a_freq_ & 0xff) + ((*data_bus & 0xF) << 8)); channel_a_freq_counter_ = (channel_a_freq_ /*<< 3*/); break;
         case 0x02: register_[register_address_] = *data_bus;channel_b_freq_ = ((channel_b_freq_ & 0xff00) + *data_bus); channel_b_freq_counter_ = (channel_b_freq_ /*<< 3*/); break;
         case 0x03: register_[register_address_] = *data_bus&0x0F;channel_b_freq_ = ((channel_b_freq_ & 0xff) + ((*data_bus & 0xF) << 8)); channel_b_freq_counter_ = (channel_b_freq_ /*<< 3*/); break;
         case 0x04: register_[register_address_] = *data_bus; channel_c_freq_ = ((channel_c_freq_ & 0xff00) + *data_bus); channel_c_freq_counter_ = (channel_c_freq_ /*<< 3*/); break;
         case 0x05: register_[register_address_] = *data_bus&0xF; channel_c_freq_ = ((channel_c_freq_ & 0xff) + ((*data_bus & 0xF) << 8)); channel_c_freq_counter_ = (channel_c_freq_ /*<< 3*/); break;
         case 0x06: register_[register_address_] = *data_bus&0x1F; noise_frequency_ = ((*data_bus==0)?1:(*data_bus & 0x1F))/*<<3*/; break;
         case 0x07: register_[register_address_] = *data_bus; mixer_control_register_ = *data_bus; break;
         case 0x08: register_[register_address_] = *data_bus&0x1F; channel_a_volume_ = *data_bus & 0x1F; break;
         case 0x09: register_[register_address_] = *data_bus&0x1F; channel_b_volume_ = *data_bus & 0x1F; break;
         case 0x0A: register_[register_address_] = *data_bus&0x1F; channel_c_volume_ = *data_bus & 0x1F; break;
         case 0x0B: register_[register_address_] = *data_bus;volume_enveloppe_frequency_ = ((register_[0x0B]) + (register_[0x0C] << 8)) /*<< 4*/; break;
         case 0x0C: register_[register_address_] = *data_bus;volume_enveloppe_frequency_ = ((register_[0x0B] ) + (register_[0x0C] << 8))/*<<4*/; break;
         case 0x0D:
         {
            register_[register_address_] = *data_bus&0x0F;
            volume_enveloppe_shape_ = (*data_bus & 0xF);
            counter_env_ = 0;
            enveloppe_stop_ = false;
            enveloppe_up_ = ((volume_enveloppe_shape_ & 0x04) == 0x04);
            enveloppe_volume_ = (enveloppe_up_) ? 0 : 15;
         }

         case 0x0E: register_[register_address_] = *data_bus;register_14_ = *data_bus; register_replaced_ = false; break;
         case 0x0F: register_[register_address_] = *data_bus;external_data_register_b_ = *data_bus; break;

         default: break;
         }
      }
      // Correction :
      if (volume_enveloppe_frequency_ == 0) volume_enveloppe_frequency_ = 1/*<<4*/;
      if (channel_a_freq_ == 0) {channel_a_freq_ = 1;channel_a_freq_counter_ = (channel_a_freq_/*<<3*/);}
      if (channel_b_freq_ == 0) {channel_b_freq_ = 1;channel_b_freq_counter_ = (channel_b_freq_/*<<3*/);}
      if (channel_c_freq_ == 0) {channel_c_freq_ = 1;channel_c_freq_counter_ = (channel_c_freq_/*<<3*/);}
   }
   else                   // BDIR = 0
   {
      if (bus_ctrl & 0x01)
      {
         // Lire le registre du psg.
         switch (register_address_)
         {
         case 0x00:
         case 0x01:
         case 0x02:
         case 0x03:
         case 0x04:
         case 0x05:
         case 0x06:
         case 0x07:
         case 0x08:
         case 0x09:
         case 0x0A:
         case 0x0B:
         case 0x0C:
            *data_bus = register_[register_address_]; break;
         case 0x0D:
         {
            *data_bus = volume_enveloppe_shape_;
            break;
         }
         case 0x0E:
         {
            if ((mixer_control_register_ & 0x40) == 0)
            {
               // Input
               *data_bus = ((data < 10) ? keyboard_handler_->GetKeyboardMap(data) : 0);
               register_replaced_ = true;
               break;
            }
            else
            {
               // Output
               *data_bus = (register_replaced_) ? ((data < 10) ? keyboard_handler_->GetKeyboardMap(data) : 0) : register_14_;
               break;
            }

         }
         case 0x0F:
         {
            if ((mixer_control_register_ & 0x80) == 0x80)
            {
               // Input
               *data_bus = external_data_register_b_; break;
            }
            else
            {
               // Output
               *data_bus = 0xFF;
            }
         }

         default: *data_bus = 0xFF; break;
         }
      }
      else
      {
         *data_bus = 0xFF;
      }
   }
}

void Ay8912::InitSound (ISound* sound)
{
   sound_ = sound;
}


void Ay8912::TickSound ()
{
   // Tone generator

   // One more clock tick
   // Count each Chan : Do we have to invert square output ?
   ++counter_a_;
   ++counter_b_;
   ++counter_c_;

   if (counter_a_ >= channel_a_freq_counter_ )
   {
      if (channel_a_freq_counter_ == 1 && channel_a_volume_ > 0)
      {
         int dbg = 1;
      }
      // Inversion !
      chan_a_high_ ^= 1;
      counter_a_ = 0;
   }
   if (counter_b_ >= channel_b_freq_counter_)
   {
      // Inversion !
      chan_b_high_ ^= 1;
      counter_b_ = 0;
   }
   if (counter_c_ >= channel_c_freq_counter_)
   {
      // Inversion !
      chan_c_high_ ^= 1;
      counter_c_ = 0;
   }


   // Noise
   counter_noise_++;
   if (counter_noise_ >= noise_frequency_
      && (counter_noise_ & 1))
   {
      // Inversion !
      counter_noise_ = 0;

      if ((noise_shift_register_+1)&2)
      {
         chan_noise_high_  ^= 1;
      }

      if (noise_shift_register_ & 1)
         noise_shift_register_ ^= 0x24000;
      noise_shift_register_ >>= 1;
   }

   // Enveloppe
   if ( counter_env_ >= (volume_enveloppe_frequency_*2))
   {
      counter_env_ = 0;
      if (enveloppe_stop_)
      {
         // Hold or continue;
      }
      else
      {
         // One tick of the enveloppe
         counter_state_env_++;

         // End ?
         if (counter_state_env_ >= 16)
         {
            counter_state_env_ = 0;
            // Continue
            if ((volume_enveloppe_shape_ &0x08) == 0x00)
            {
               enveloppe_volume_ = 0;
               enveloppe_stop_ = true;
            }
            else
            {
               // Alternate ?
               if ((volume_enveloppe_shape_ &0x02) == 0x02)
               {
                  enveloppe_up_ = !enveloppe_up_;
               }
               else
               {
                  enveloppe_volume_ = enveloppe_up_ ? 0 : 15;
               }

               // Hold
               if ((volume_enveloppe_shape_ &0x01) == 0x01)
               {
                  enveloppe_stop_ = true;

                  if (enveloppe_up_)
                  {

                     enveloppe_volume_ = 15;
                  }
                  else
                  {
                     enveloppe_volume_ = 0;
                  }
               }
            }
         }
         else
         {
            // Normal step
            // Attack
            if (enveloppe_up_)
            {
               enveloppe_volume_++;
            }
            else
            {
               enveloppe_volume_--;
            }
         }
      }
   }
   enveloppe_volume_ &= 0xF;
   ++counter_env_;

   // Default implementation : A is Left, B middle, C right
#define DEF_CHAN_VALUE 0x0F

   char mix_a = 1;
   int add_l (0), add_r(0), add_c(0);

   // Channel A
   // Output is :
   // if nothing is up in the mixer : It seems to be +Channel Volume (to check in the real life)
   // Otherwise : Volume, +/- depending on High or Low
   // - from -vol / +vol if mixer is enabled ( noise or tune )
   // TODO :
   // - Check volume when nothing is enabled in the mixer
   // - Check evolution of volume when noise + tune are enabled
   //
   if ((mixer_control_register_ & 0x1) == 0)  mix_a &= chan_a_high_;
   if ((mixer_control_register_ & 0x8) == 0)  mix_a &= chan_noise_high_;
   if (mix_a)
   {
      // Mixer is on : Volume goest from -1 to 1
      add_l = (channel_a_volume_<0x10) ? channel_a_volume_ : enveloppe_volume_;
   }
   else
   {
      add_l = 0;
   }


   char mix_b = 1;
   if ((mixer_control_register_ & 0x2) == 0)   mix_b &= chan_b_high_;
   if ((mixer_control_register_ & 0x10) == 0)  mix_b &= chan_noise_high_;

   if (mix_b )
   {
      add_c = (channel_b_volume_<0x10) ? channel_b_volume_ : enveloppe_volume_;
   }
   else
   {
      add_c = 0;
   }

   char mix_c = 1;
   if ((mixer_control_register_ & 0x4) == 0)  mix_c &= chan_c_high_;
   if ((mixer_control_register_ & 0x20) == 0) mix_c &= chan_noise_high_;
   if (mix_c )
   {
      add_r = (channel_c_volume_<0x10) ? channel_c_volume_ : enveloppe_volume_;
   }
   else
   {
      add_r = 0;
   }
   sound_source_.AddSound(add_l, add_r, add_c);

   sound_hub_->Tick();
}

unsigned int Ay8912::Tick ( /*unsigned int nbTicks*/)
{
   if (sound_ == NULL)
   {
      if(log_) log_->WriteLog("PSG : sound = null");
      return 4000000;   // No need to update this more than one every seconds
   }
   TickSound ();

   return 4*8;
}
