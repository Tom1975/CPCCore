#include "stdafx.h"
#include "PlayCity.h"

//#define YMZ_CALL 32
#define YMZ_CALL (16>>CLOCK_DIV)


PlayCity::PlayCity(IClockable* int_line, IClockable* nmi_line, /*SoundMixer *mixer, */SoundMixer *sound_hub) : ymz294_1_(/*mixer, */YMZ294::LEFT, sound_hub), ymz294_2_(/*mixer,*/ YMZ294::RIGHT, sound_hub), trg0_(this)
{
   // Set the clk from ymz294's to output of CTC channel 0
   inner_line_.AddComponent(&ymz294_1_);
   inner_line_.AddComponent(&ymz294_2_);

   // Channel 0 => YMZ294
   z84c30_.Init(Z84C30::CHANNEL_0, &trg0_/*inner_line*/, int_line);
   // Channel 1 => NMI
   z84c30_.Init(Z84C30::CHANNEL_1, nmi_line, int_line);

   // Channel 2 => Channel 3
   inner_line_channel_23_.AddComponent(z84c30_.GetCounter(Z84C30::CHANNEL_3));
   z84c30_.Init(Z84C30::CHANNEL_2, &inner_line_channel_23_, int_line);
   // Channel 3 => ctrl_int
   z84c30_.Init(Z84C30::CHANNEL_3, int_line, nullptr);
   drop_next_tick_ = false;
}


PlayCity::~PlayCity(void)
{
}

void PlayCity::Reset()
{
   z84c30_.HardReset();
   ymz294_1_.Reset();
   ymz294_2_.Reset();

   next_call_ymz_ = 1;
}

unsigned int PlayCity::Tick()
{
   // Channel 0 Trigger is set to 4Mhz clock
   if (!drop_next_tick_)
   {
      --next_call_ymz_;
      if (next_call_ymz_ == 0)
      {
         ymz294_1_.Tick();
         ymz294_2_.Tick();

         next_call_ymz_ = YMZ_CALL;
      }
   }
   else
   {
      drop_next_tick_ = false;
   }
   z84c30_.channel_[0].Trg(trg0_update_);
   trg0_update_ = !trg0_update_;
   return z84c30_.Tick();
}

void PlayCity::Out(unsigned short address, unsigned char data)
{
   // 1111 100x 1000 xxxx
   // F880 -> F883 = Z84C30
   if ((address & 0xFEFF) == 0xF880)
   {
      z84c30_.Out(0, data);
   }
   else if ((address & 0xFEFF) == 0xF881)
   {
      z84c30_.Out(1, data);
   }
   else if ((address & 0xFEFF) == 0xF882)
   {
      z84c30_.Out(2, data);
   }
   else if ((address & 0xFEFF) == 0xF883)
   {
      z84c30_.Out(3, data);
   }
   // YMZ294 : Write (F884 = right, F888 = left)
   else if ((address & 0xFFFF) == 0xF884)
   {
       ymz294_1_.Access( &data, 0x2, 0);
   }
   else if ((address & 0xFFFF) == 0xF888)
   {
      ymz294_2_.Access( &data, 0x2, 0);
   }
   // YMZ : Select (F984  = right, F988 = left)
   else if ((address & 0xFFFF) == 0xF984)
   {
      ymz294_1_.Access( &data, 0x3, 0);
   }
   else if ((address & 0xFFFF) == 0xF988)
   {
      ymz294_2_.Access( &data, 0x3, 0);
   }
}

void PlayCity::In(unsigned char* data, unsigned short address)
{
   // No Output from Playcity
   if ((address & 0xFEFF) == 0xF880)
   {
      z84c30_.In(data, 0);
   }
   else if ((address & 0xFEFF) == 0xF881)
   {
      z84c30_.In(data, 1);
   }
   else if ((address & 0xFEFF) == 0xF882)
   {
      z84c30_.In(data, 2);
   }
   else if ((address & 0xFEFF) == 0xF883)
   {
      z84c30_.In(data, 3);
   }
   // YMZ294 : Write (F884 = right, F888 = left)
   else if ((address & 0xFFFF) == 0xF884)
   {
      ymz294_1_.Access(data, 0x2, 0);
   }
   else if ((address & 0xFFFF) == 0xF888)
   {
      ymz294_2_.Access(data, 0x2, 0);
   }
   // YMZ : Select (F984  = right, F988 = left)
   else if ((address & 0xFFFF) == 0xF984)
   {
      ymz294_1_.Access(data, 0x3, 0);
   }
   else if ((address & 0xFFFF) == 0xF988)
   {
      ymz294_2_.Access(data, 0x3, 0);
   }
}
