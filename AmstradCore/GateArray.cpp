
#include <iostream>
#include <sstream>

#include "GateArray.h"

///////////////////////////////////////
// GateArray
//
GateArray::GateArray() : counter(0), line_4_mhz_(nullptr), line_CCLK_mhz_(nullptr)
{
   
}
GateArray::~GateArray()
{
   
}

void GateArray::CreateGateArray(BusLine* line_4, BusLine* line_1, BusLine* line_wait)
{
   line_4_mhz_ = line_4;
   line_CCLK_mhz_ = line_1;
   line_wait_ = line_wait;

   Reset();
}

void GateArray::Reset()
{
   // Reset inner counters
   
}

void GateArray::Tick()
{
   ++counter;

   if (counter == 2 || counter == 26) line_wait_->Tick();

   if ((counter&0x3) == 3) line_4_mhz_->Tick();

   if (counter == 12 || counter == 22) line_CCLK_mhz_->Tick();
}

