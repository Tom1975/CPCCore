#include "stdafx.h"
#include "Bus.h"


Bus::Bus(unsigned int size)
{
   data_ = 0;
   defined_data_ = 0xffffffff;
}


Bus::~Bus(void)
{
}

void Bus::SetBus ( unsigned char data )
{
   data_ = data;
   defined_data_ |= 0xff;
}

void Bus::SetBus ( unsigned short data )
{
   data_ = data;
   defined_data_ = 0xffff;
}

