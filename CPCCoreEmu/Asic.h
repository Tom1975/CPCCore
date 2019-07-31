#pragma once

/*
 * (C) Copyright 2017 Thomas GUILLEMIN
 * 
 * 
 *
 */

#include "IComponent.h"
#include "CRTC.h"
#include "VGA.h"
#include "PPI.h"

class Asic : public IComponent
{
public:

   Asic();
   virtual ~Asic();

   void Init(GateArray* vga, CRTC* crtc, ILog* log = nullptr);

   void HardReset();
   unsigned int Tick() override {return crtc_->Tick(); }
   virtual void M1 ();

   // Adress
   virtual void Out( const unsigned short address, const unsigned char data);
   virtual void In(unsigned char* a, unsigned short address);

   const bool IsAsicLocked() { return vga_->IsAsicLocked(); }
   void SetIndexVerification(const int index) { index_verification_ = index; }
   int GetIndexVerification() { return index_verification_ ; }
public:

   // ASIC is emuling the following :
   // CRTC
   CRTC * crtc_;

   // Gate Array & PAL
   GateArray * vga_;

   // 8255
   PPI8255 ppi_;

   // Specific stuff
   static const unsigned char locking_sequence[];
   int index_verification_;
   ILog* log_;
};

