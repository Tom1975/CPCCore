#pragma once

#include "IComponent.h"
#include "BusLine.h"

class AY8912 : public IComponent
{
public:
   AY8912();
   virtual ~AY8912();

   void Create();

   void Reset();
   void TickUp() override;
   void TickDown() override;


protected:



};

