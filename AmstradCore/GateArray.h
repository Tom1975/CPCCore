#pragma once

#include <string>
#include <vector>

#include "IComponent.h"
#include "BusLine.h"

class GateArray : public IComponent
{
public:
   GateArray();
   virtual ~GateArray();

   void CreateGateArray(BusLine* line_4, BusLine* line_1, BusLine* line_wait);

   void Reset();
   void Tick() override;

protected:
   unsigned int counter;
   BusLine* line_4_mhz_;
   BusLine* line_CCLK_mhz_;
   BusLine* line_wait_;
};

