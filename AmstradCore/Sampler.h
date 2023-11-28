#pragma once

#include <string>
#include <vector>

#include "SingleLineSample.h"


class Sampler
{
public:

   ///////////////////////////////////////
   // CTor / DTor
   Sampler();
   virtual ~Sampler();

   void Clear();
   void AddLineToSample(std::string name, BusLine* line);

   ///////////////////////////////////////
   // Sample
   void AddSample();
   std::string GetWaves();

protected:
   ///////////////////////////////////////
   // Sample
   bool sample_;
   std::vector<SingleLineSample> samples_;


};
