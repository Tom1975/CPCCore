#pragma once

/*
 * (C) Copyright 2017 Thomas GUILLEMIN
 * 
 * 
 *
 */

#include "IExpansion.h"
#include "Memoire.h"
class CSig;

class MaxiRom : public IExpansion
{
public:
   MaxiRom(CSig* memory);
   virtual ~MaxiRom(void);

   virtual void Plug(bool plug);
   virtual void Reset();
   virtual unsigned int Tick();
   virtual void Out( unsigned short address, unsigned char data);
   virtual void In(unsigned char* data, unsigned short address);


protected:
   CSig* sig_;
};

