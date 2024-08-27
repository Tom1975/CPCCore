#pragma once

#include <string>
#include <vector>


#define DECLARE_REFERENCE_LINE(l) BusLine *l;
#define DECLARE_REFERENCE_BUS_ADDRESS(b) Bus<unsigned short>*b;
#define DECLARE_REFERENCE_BUS_DATA(b) Bus<unsigned char>*b;

#define LINK_LINE(c,l) \
   c.l = &l;

#define LINK_BUS(c,b) \
   c.b = &b;

class IComponent
{
public:
   virtual void TickUp() = 0;
   virtual void TickDown() = 0;
};
