#pragma once

#include "IComponent.h"
#include "BusLine.h"

class CRTC_0 : public IComponent
{
public:
   CRTC_0();
   virtual ~CRTC_0();

   void Create();

   void Reset();
   void TickUp() override;
   void TickDown() override;


protected:



};

