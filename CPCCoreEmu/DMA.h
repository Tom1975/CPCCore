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
   friend class MachineState;

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
   virtual void M1 ();

   // DMA sequencer of the ASIC (PROVISIONAL, see GateArray::HandleDMA) :
   // - HsyncTick() : start of the HSYNC. A channel in PAUSE clocks its prescaler, the others ask for the RAM
   //   to fetch their instruction (returns true) ;
   // - Schedule() : the sequencer gives the channel its fetch and execute microseconds ;
   // - Clock() : every microsecond, fetches then executes the instruction on its microseconds.
   bool HsyncTick();
   void ClearPrescalarWrite();
   void Schedule(int fetch_in, int exec_in) { fetch_in_ = fetch_in; exec_in_ = exec_in; }
   void Clock();
   void Fetch();
   void Execute();

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
   // Microseconds before the fetch and the execution of the instruction (0 : nothing scheduled)
   unsigned char fetch_in_;
   unsigned char exec_in_;
   // Write of the prescaler register, latched until the next HSYNC
   bool prescalar_write_;
};
