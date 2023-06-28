#pragma once

#include <string>
#include <vector>

#include "IComponent.h"

template <typename T>
class Bus
{
public:
   Bus():data_(0), highz_(-1){};
   virtual ~Bus() {};

   T data_;
   T highz_;
};

class BusLine
{
public:
   BusLine();
   virtual ~BusLine();

   void AddComponent(IComponent* component);

   void Tick();
   unsigned char GetLevel();
   void ForceLevel(unsigned char level);

protected:

   std::vector<IComponent*> component_list_;
   unsigned char up_;
};

