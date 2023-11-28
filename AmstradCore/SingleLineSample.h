#pragma once

#include <string>
#include <vector>

#include "BusLine.h"

class SingleLineSample
{
public:
   SingleLineSample(std::string label, BusLine* line);

   void Clear();
   std::string GetSample();

   std::string label_;
   BusLine* line_;
   std::vector<bool> samples_;
};

