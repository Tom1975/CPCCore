#include "stdafx.h"
#include "YMZ294.h"

//#define CLOCK_DIV 4


YMZ294::YMZ294(/*SoundMixer * mixer, */Output output, SoundMixer *sound_hub) : output_(output), /*mixer_(mixer), */channel_a_freq_(1),channel_a_freq_counter_(0), channel_b_freq_(1), channel_b_freq_counter_(0), channel_c_freq_(1), channel_c_freq_counter_(0),
            mixer_control_register_ (0), noise_frequency_(1), channel_a_volume_(0), channel_b_volume_(0), channel_c_volume_(0), channel_a_high_(0), channel_b_high_(0), channel_c_high_(0),
            channel_noise_high_ (0), noise_shift_register_(0), volume_enveloppe_frequency_(0), sound_source_(sound_hub)
{
    //mixer_->AddSoundSource(&sound_source_);
    Reset();
}


YMZ294::~YMZ294(void)
{
   //mixer_->RemoveSoundSource(&sound_source_);
}

void YMZ294::Reset()
{
   memset(register_, 0, sizeof(register_));
   channel_a_freq_ = 1;
   channel_b_freq_ = 1;
   channel_c_freq_ = 1;

   noise_shift_register_ = 1;

   noise_frequency_ = 1<< CLOCK_DIV;
   mixer_control_register_ = 0xFF;

   channel_a_volume_ = 0;
   channel_b_volume_ = 0;
   channel_c_volume_ = 0;

   volume_enveloppe_frequency_ = 1 << 4;
   volume_enveloppe_shape_ = 0;

   counter_a_ = 0;
   counter_b_ = 0;
   counter_c_ = 0;

   channel_a_high_ = 1;
   channel_b_high_ = 1;
   channel_c_high_ = 1;
   channel_noise_high_ = 1;

   counter_noise_ = 0;
   counter_env_ = 0;
   counter_state_env_ = 0;
   envelope_volume_ = 0;
   stop_ = false;

   register_address_ = 0;
   sound_source_.Reinit();
}

bool YMZ294::Access(unsigned char *data_bus, unsigned char bus_ctrl, unsigned char data)
{
   // Depends onthe BusCTRL
   if (bus_ctrl & 0x2)     // BDIR = 1
   {
      if (bus_ctrl & 0x1)
      {
         // Write address
         register_address_ = *data_bus;
         return true;
      }
      else
      {
         switch (register_address_)
         {
         case 0x00: register_[register_address_] = *data_bus; channel_a_freq_ = ((channel_a_freq_ & 0xff00) + *data_bus); channel_a_freq_counter_ = (channel_a_freq_ << CLOCK_DIV); break;
         case 0x01: register_[register_address_] = *data_bus & 0x0F; channel_a_freq_ = ((channel_a_freq_ & 0xff) + ((*data_bus & 0xF) << 8)); channel_a_freq_counter_ = (channel_a_freq_ << CLOCK_DIV); break;
         case 0x02: register_[register_address_] = *data_bus; channel_b_freq_ = ((channel_b_freq_ & 0xff00) + *data_bus); channel_b_freq_counter_ = (channel_b_freq_ << CLOCK_DIV); break;
         case 0x03: register_[register_address_] = *data_bus & 0x0F; channel_b_freq_ = ((channel_b_freq_ & 0xff) + ((*data_bus & 0xF) << 8)); channel_b_freq_counter_ = (channel_b_freq_ << CLOCK_DIV); break;
         case 0x04: register_[register_address_] = *data_bus; channel_c_freq_ = ((channel_c_freq_ & 0xff00) + *data_bus); channel_c_freq_counter_ = (channel_c_freq_ << CLOCK_DIV); break;
         case 0x05: register_[register_address_] = *data_bus & 0xF; channel_c_freq_ = ((channel_c_freq_ & 0xff) + ((*data_bus & 0xF) << 8)); channel_c_freq_counter_ = (channel_c_freq_ << CLOCK_DIV); break;
         case 0x06: register_[register_address_] = *data_bus & 0x1F; noise_frequency_ = ((*data_bus == 0) ? 1 : (*data_bus & 0x1F))<< CLOCK_DIV; break;
         case 0x07: register_[register_address_] = *data_bus; mixer_control_register_ = *data_bus; break;
         case 0x08: register_[register_address_] = *data_bus & 0x1F; channel_a_volume_ = *data_bus & 0x1F; break;
         case 0x09: register_[register_address_] = *data_bus & 0x1F; channel_b_volume_ = *data_bus & 0x1F; break;
         case 0x0A: register_[register_address_] = *data_bus & 0x1F; channel_c_volume_ = *data_bus & 0x1F; break;
         case 0x0B: register_[register_address_] = *data_bus; volume_enveloppe_frequency_ = ((register_[0x0B]) + (register_[0x0C] << 8)) << CLOCK_DIV; break;
         case 0x0C: register_[register_address_] = *data_bus; volume_enveloppe_frequency_ = ((register_[0x0B]) + (register_[0x0C] << 8)) << CLOCK_DIV; break;
         case 0x0D:
         {
            register_[register_address_] = *data_bus & 0x0F;
            volume_enveloppe_shape_ = (*data_bus & 0xF);
            counter_env_ = 0;
            stop_ = false;
            up_ = ((volume_enveloppe_shape_ & 0x04) == 0x04);
            envelope_volume_ = (up_) ? 0 : 15;
         }
         break;

         default: return false;
         }
      }
      // Correction :
      if (volume_enveloppe_frequency_ == 0) volume_enveloppe_frequency_ = 1 << 2 << CLOCK_DIV;
      if (channel_a_freq_ == 0) { channel_a_freq_ = 1; channel_a_freq_counter_ = (channel_a_freq_<< CLOCK_DIV); }
      if (channel_b_freq_ == 0) { channel_b_freq_ = 1; channel_b_freq_counter_ = (channel_b_freq_<< CLOCK_DIV); }
      if (channel_c_freq_ == 0) { channel_c_freq_ = 1; channel_c_freq_counter_ = (channel_c_freq_<< CLOCK_DIV); }
   }
   else                   // BDIR = 0
   {
      if (bus_ctrl & 0x01)
      {
         // Lire le registre du psg.
         //addr = addr &0xF;
         // Exemple : Registre 14 (clavier) , on retourne un truc pour voir
         switch (register_address_)
         {
         case 0x00: //*dataBus = (channel_a_freq_ & 0xff);  break;
         case 0x01: //*dataBus = (channel_a_freq_ >> 8); break;
         case 0x02: //*dataBus = (channel_b_freq_ & 0xff); break;
         case 0x03: //*dataBus = (channel_b_freq_ >> 8); break;
         case 0x04: //*dataBus = (channel_c_freq_ & 0xff); break;
         case 0x05: //*dataBus = (channel_c_freq_ >> 8); break;
         case 0x06: //*dataBus = (noise_frequency_>>3); break;
         case 0x07: //*dataBus = mixer_control_register_; break;
         case 0x08: //*dataBus = channel_a_volume_; break;
         case 0x09: //*dataBus = channel_b_volume_; break;
         case 0x0A: //*dataBus = channel_c_volume_; break;
         case 0x0B: //*dataBus = (volume_enveloppe_frequency_ & 0xff); break;
         case 0x0C: //*dataBus = (volume_enveloppe_frequency_ >> 8); break;
            *data_bus = register_[register_address_]; break;
         case 0x0D:
         {
            *data_bus = volume_enveloppe_shape_;
            break;
         }

         default: return false;
         }
      }
      else
      {
         *data_bus = 0xFF;
      }
   }
   return true;
}

unsigned int YMZ294::   Tick()
{
   // Tone generator

   // One more clock tick
   // Count each Chan : Do we have to invert square output ?
   ++counter_a_;
   ++counter_b_;
   ++counter_c_;

   if (counter_a_ >= channel_a_freq_counter_ /*(channel_a_freq_<< CLOCK_DIV)*/)
   {
      // Inversion !
      channel_a_high_ ^= 1;
      counter_a_ = 0;
   }
   if (counter_b_ >= channel_b_freq_counter_/*(channel_b_freq_<< CLOCK_DIV)*/)
   {
      // Inversion !
      channel_b_high_ ^= 1;
      counter_b_ = 0;
   }
   if (counter_c_ >= channel_c_freq_counter_/*(channel_c_freq_<< CLOCK_DIV)*/)
   {
      // Inversion !
      channel_c_high_ ^= 1;
      counter_c_ = 0;
   }


   // Noise
   counter_noise_++;
   if (counter_noise_ >= noise_frequency_
      && (counter_noise_ & 1))
      //if ((counter_noise_ & 1) && counter_noise_ >= noise_frequency_)
   {
      // Inversion !
      counter_noise_ = 0;

      if ((noise_shift_register_ + 1) & 2)
      {
         channel_noise_high_ ^= 1;
      }

      if (noise_shift_register_ & 1)
         noise_shift_register_ ^= 0x24000;
      noise_shift_register_ >>= 1;
   }

   // Enveloppe - *2 to map 16 step instead of 32 (todo : fix it)
   if (counter_env_ >= (volume_enveloppe_frequency_*2))
   {
      counter_env_ = 0;
      if (stop_)
      {
         // Hold or continue;
      }
      else
      {
         // One tick of the enveloppe
         counter_state_env_++;

         // End ?
         if (counter_state_env_ >= 16 )
         {
            counter_state_env_ = 0;
            // Continue
            if ((volume_enveloppe_shape_ & 0x08) == 0x00)
            {
               envelope_volume_ = 0;
               stop_ = true;
            }
            else
            {
               // Alternate ?
               if ((volume_enveloppe_shape_ & 0x02) == 0x02)
               {
                  up_ = !up_;
               }
               else
               {
                  envelope_volume_ = up_ ? 0 : 15;
               }

               // Hold
               if ((volume_enveloppe_shape_ & 0x01) == 0x01)
               {
                  stop_ = true;

                  if (up_)
                  {

                     envelope_volume_ = 15;
                  }
                  else
                  {
                     envelope_volume_ = 0;
                  }
               }
            }
         }
         else
         {
            // Normal step
            // Attack
            if (up_)
            {
               envelope_volume_++;
            }
            else
            {
               envelope_volume_--;
            }
         }
      }
   }
   envelope_volume_ &= 0xF;
   ++counter_env_;

   // Default implementation : A is Left, B middle, C right
#define DEF_CHAN_VALUE 0x0F
/*
m_ATotal += ((channel_a_volume_ & 0xF0)?envelope_volume_:channel_a_volume_)& (!((((mixer_control_register_ & 0x1) | channel_a_high_)&(((mixer_control_register_ >> 3) & 0x1) | channel_noise_high_)))-1);
m_BTotal += ((channel_b_volume_ & 0xF0)?envelope_volume_:channel_b_volume_)& (!((((mixer_control_register_ >> 1) & 0x1) | channel_b_high_)& (((mixer_control_register_ >> 4) & 0x1) | channel_noise_high_)) - 1);
m_CTotal += ((channel_c_volume_ & 0xF0)? envelope_volume_: channel_c_volume_)& (!((((mixer_control_register_ >> 2) & 0x1) | channel_c_high_)&(((mixer_control_register_ >> 5) & 0x1) | channel_noise_high_))-1);
*/

   char mix_a = 1;
   if ((mixer_control_register_ & 0x1) == 0)  mix_a &= channel_a_high_;
   if ((mixer_control_register_ & 0x8) == 0)  mix_a &= channel_noise_high_;

   int add_l = -1;
   if (mix_a)
   {
      //m_ATotal += (channel_a_volume_<0x10) ? channel_a_volume_ : envelope_volume_;
      add_l = (channel_a_volume_ < 0x10) ? channel_a_volume_ : envelope_volume_;
   }
   else
   {
      //add_l = 0; // ((channel_a_volume_ < 0x10) ? /*(channel_a_high_ ?0:(-1 * channel_a_volume_ ))*/0 : envelope_volume_);
      add_l = ((channel_a_volume_ < 0x10) ? /*(channel_a_high_ ?0:(-1 * channel_a_volume_ ))*/0 : envelope_volume_);
   }

   char mix_b = 1;
   int add_c = -1;
   if ((mixer_control_register_ & 0x2) == 0)   mix_b &= channel_b_high_;
   if ((mixer_control_register_ & 0x10) == 0)  mix_b &= channel_noise_high_;

   if (mix_b)
   {
      //m_BTotal += (channel_b_volume_<0x10) ? channel_b_volume_ : envelope_volume_;
      //add_c = (channel_b_volume_ < 0x10) ? channel_b_volume_ : envelope_volume_;
      add_c = ((channel_b_volume_ < 0x10) ? channel_b_volume_/*(channel_b_high_ ? 0 : (-1 * channel_b_volume_)) */ : envelope_volume_);
   }
   else
   {
      add_c = ((channel_b_volume_<0x10) ? 0 : envelope_volume_);
   }

   char mix_c = 1;
   int add_r = -1;
   if ((mixer_control_register_ & 0x4) == 0)  mix_c &= channel_c_high_;
   if ((mixer_control_register_ & 0x20) == 0) mix_c &= channel_noise_high_;
   if (mix_c)
   {
      //m_CTotal += >(channel_c_volume_<0x10) ? channel_c_volume_ : envelope_volume_;
      //add_r = (channel_c_volume_ < 0x10) ? channel_c_volume_ : envelope_volume_;
      add_r = ((channel_c_volume_ < 0x10) ? channel_c_volume_/*(channel_c_high_ ? 0 : (-1 * channel_c_volume_ ))*/ : envelope_volume_);
   }
   else
   {
      add_r =((channel_c_volume_<0x10) ? 0 : envelope_volume_);
   }

   switch (output_)
   {
   case LEFT:
      sound_source_.AddSoundLeft(add_l, add_c, add_r);
      break;
   case RIGHT:
      sound_source_.AddSoundRight(add_l, add_c, add_r);
      break;
   case BOTH:
      sound_source_.AddSound(add_l, add_c, add_r);
      break;
   }
   return 1;
}
