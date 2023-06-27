#pragma once

#include <string>
#include <vector>

class IComponent
{
public:
   virtual void Tick() = 0;
};
