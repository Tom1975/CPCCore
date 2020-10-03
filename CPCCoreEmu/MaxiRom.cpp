#include "stdafx.h"
#include "MaxiRom.h"
#include "Sig.h"

MaxiRom::MaxiRom(CSig* sig) : sig_(sig)
{
}


MaxiRom::~MaxiRom(void)
{
}

void MaxiRom::Plug(bool plug)
{
   if (plug)
   {
      sig_->PlugExpansionModule(this);
   }
   else
   {
      sig_->UnplugExapnsionModule(this);
   }
   
}

void MaxiRom::Reset()
{
}

unsigned int MaxiRom::Tick()
{
   return 0;
}

void MaxiRom::Out(unsigned short address, unsigned char data)
{
   // 1111 100x 1000 xxxx
   // FFFF
   if ((address & 0xFFFF) == 0xFFFF)
   {
      sig_->memory_->ConnectRomList(data);
   }
}

void MaxiRom::In(unsigned char* data, unsigned short address)
{
   
}
