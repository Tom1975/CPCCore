#pragma once

#include <string>
#include <vector>

#include "IComponent.h"

class BusLine : public IComponent
{
public:
   BusLine();
   virtual ~BusLine();

   void AddComponent(IComponent* component);

   void Tick() override;
   bool GetLevel();
   void ForceLevel(bool level);

protected:

   std::vector<IComponent*> component_list_;
   bool up_;
};

