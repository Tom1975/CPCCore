#pragma once

/*
 * (C) Copyright 2017 Thomas GUILLEMIN
 * 
 * 
 *
 */

#include "IComponent.h"
#include "PSG.h"
#include "DMAStop.h"
class Memory;

class DMA : public IComponent
{
public:

   DMA();
   virtual ~DMA(void);

   void Init(int channel, IDmaSTOP* dma_stop, Memory * memory, unsigned char* reg, Ay8912* psg, CSig* sig)
   {
      channel_ = channel; dma_stop_ = dma_stop;  memory_ = memory; dma_register_ = reg; psg_ = psg; sig_ = sig;
   }
   void SetPrescalar(unsigned char prescalar);
   void EnableNext() {
      enable_next_ = true;
   }
   ;
   void HardReset();

   void AcqInt() { interrupt_on_ = false; }

   virtual unsigned int Tick();
   virtual void Hbl();
   virtual void M1 ();

public:

   enum
   {
      NONE, 
      PAUSE
   } dma_cycle_;

   bool enable_next_;

   unsigned char *dma_register_;
   unsigned char ppr_;
   Memory * memory_;

   IDmaSTOP* dma_stop_;
   int channel_;
   unsigned short curent_instr_;
   Ay8912* psg_;
   int pause_counter_;
   int repeat_counter_;
   unsigned short repeat_addr_;

   CSig* sig_;

   bool interrupt_on_;
   unsigned char prescalar_;
   unsigned char prescalar_counter_;
};
