#pragma once

#include "Z80_Full.h"
#include "Sig.h"
#include "Asic.h"
#include "DMA.h"
#include "MultifaceII.h"
#include "SoundMixer.h"
#include "PlayCity.h"
#include "DMA.h"
#include "Bus.h"
#include "IPlayback.h"
#include "PrinterDefault.h"
#include "DskTypeManager.h"

#include "Breakpoint.h"

#define NB_BP_MAX    10


class NetList : public IClockable
{
public:
   NetList(bool* signal) :signal_(signal) {};
   virtual unsigned int Tick() { if (signal_)*signal_ = true; return 1; }

private:
   bool* signal_;
};

class NetListINT : public IClockable
{
public:
   NetListINT(CSig* z80_int) :z80_int_(z80_int) {};
   virtual unsigned int Tick() { if (z80_int_)z80_int_->req_int_ = true; return 1; }

private:
   CSig* z80_int_;
};


class CursorLine : public IClockable
{
public:
   CursorLine(PlayCity* playcity) : playcity_(playcity)
   {}
   virtual unsigned int Tick() { playcity_->GetCtc()->channel_[1].Trg(true); playcity_->GetCtc()->channel_[1].Trg(false); return 1; }
protected:
   PlayCity* playcity_;
};

class ISupervisor
{
public:
   virtual void EmulationStopped() = 0;
   virtual void SetEfficience(double p) = 0;
   virtual void RefreshRunningData() = 0;
};

class Motherboard : public IMachine
{
public:

   // CTor / DTor
   Motherboard(SoundMixer* sound_mixer);
   virtual ~Motherboard();

   // Init the motherboard & emulation
   void InitMotherbard(ILog *log, IPlayback * sna_handler, IDisplay* display, IFdcNotify* notifier, IDirectories * directories, IConfiguration * configuration_manager);
   virtual void SetSupervisor(ISupervisor* supervisor) { supervisor_ = supervisor; }

   // Configuration
   void SetPlus(bool plus);
   bool IsPLUS() { return plus_; }

   // Use the machine
   void ForceTick(IComponent* component, int ticks);
   void StartOptimized(unsigned int nb_cycles);
   void StartOptimizedPlus(unsigned int nb_cycles);
   int DebugNew(unsigned int nb_cycles);
   unsigned int GetSpeed() { return speed_percent_; }

   void UpdateExternalDevices();
   void InitStartOptimized();
   void InitStartOptimizedPlus();

protected:


   // Debug & running attributes
   bool run_;
   bool step_in_;
   bool step_;
   unsigned int stop_pc_;
   bool remember_step_;
   unsigned int counter_;

   // Speed handling
   unsigned int speed_percent_;

   // Breakpoint generic
   IBreakpoint* generic_breakpoint_;
   // Max 10 breakpoints
   unsigned short breakpoint_list_[NB_BP_MAX];
   unsigned int breakpoint_index_;
   ISupervisor* supervisor_;


   // Specific configuration
   bool plus_;

   // Inner hardware
   Z80 z80_;
   CSig signals_;
   Asic asic_;
   DMA dma_[3];
   GateArray vga_;
   CRTC crtc_;
   Ay8912 psg_;
   PPI8255 ppi_;
   Monitor monitor_;
   CTape tape_;
   FDC fdc_;
   Memory memory_;
   MultifaceII multiface2_;
   PlayCity play_city_;
   NetListINT netlist_int_;
   NetList netlist_nmi_;
   CursorLine cursor_line_; 
   
   Bus address_bus_;
   Bus data_bus_;

   IPrinterPort* printer_;
   PrinterDefault default_printer_;

   DskTypeManager disk_type_manager_;

   // Running the emulation
   IComponent * component_list_[10]; // Used to avoid allocation
   unsigned int component_elapsed_time_[32];

   int nb_components_;
   int z80_index_;

};