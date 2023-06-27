#pragma once

#include <string>
#include <vector>

#include "IComponent.h"
#include "BusLine.h"
#include "GateArray.h"
#include "SingleLineSample.h"


class Motherboard
{
public:

   ///////////////////////////////////////
   // CTor / DTor
   Motherboard();
   virtual ~Motherboard();

   ///////////////////////////////////////
   // Creation
   void Create();

   ///////////////////////////////////////
   // Reset
   void Reset();

   ///////////////////////////////////////
   // Run
   void Tick();

   ///////////////////////////////////////
   // Sample
   void StartSample();
   std::string StopSample();

protected:
   ///////////////////////////////////////
   // Sample
   bool sample_{};

   std::vector<SingleLineSample> samples_;

   ///////////////////////////////////////
   // Inner components
   BusLine line_16_mhz_;
   BusLine line_4_mhz_;
   BusLine line_CCLK_mhz_;
   BusLine line_wait_;


   GateArray gate_array_;
};
