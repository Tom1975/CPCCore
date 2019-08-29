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

   ///////////////////////////////////////
   // CTor / DTor
   Motherboard(SoundMixer* sound_mixer, IKeyboardHandler* keyboard_handler);
   virtual ~Motherboard();

   ///////////////////////////////////////
   // Init the motherboard & emulation
   void InitMotherbard(ILog *log, IPlayback * sna_handler, IDisplay* display, IFdcNotify* notifier, IDirectories * directories, IConfiguration * configuration_manager);
   virtual void SetSupervisor(ISupervisor* supervisor) { supervisor_ = supervisor; }
   virtual void SetConfigurationManager(IConfiguration * configuration_manager) {
      psg_.SetConfigurationManager(configuration_manager);//keyboardhandler_.SetConfigurationManager(configuration_manager);
   }
   virtual void SetDirectories(IDirectories * dir) {default_printer_.SetDirectories(dir);}
   void SetLog(ILog* log) { fdc_.SetLog(log); crtc_.SetLog(log); signals_.SetLog(log); ppi_.SetLog(log); }
   void SetNotifier(IFdcNotify* notifier) { fdc_.SetNotifier(notifier); tape_.SetNotifier(notifier); }
   
   ///////////////////////////////////////
   // Breakpoints

   void AddBreakpoint(unsigned short addr);
   void ChangeBreakpoint(unsigned short  old_bp, unsigned short new_bp);
   void RemoveBreakpoint(unsigned short addr);
   void CleanBreakpoints();
   void SetGenericBreakpoint(IBreakpoint* generic_breakpoint) { generic_breakpoint_ = generic_breakpoint;}
   // Configuration
   void SetPlus(bool plus);
   bool IsPLUS() { return plus_; }

   ///////////////////////////////////////
   // Get machine info
   Memory* GetMem() { return &memory_; };
   Asic* GetAsic() { return &asic_; }
   Ay8912* GetPSG() { return &psg_; };
   CRTC* GetCRTC() { return &crtc_; }
   GateArray* GetVGA() { return &vga_; }
   FDC* GetFDC() { return &fdc_; };
   CTape* GetTape() { return &tape_; };
   Monitor* GetMonitor() { return &monitor_; }
   PPI8255* GetPPI() { return &ppi_; }
   CSig* GetSig() { return &signals_; }
   Z80* GetProc() { return &z80_; };
   IPrinterPort* GetPrinter() { return printer_; };
   PlayCity* GetPlayCity() { return &play_city_; }
   DMA* GetDMA(int i) { return &dma_[i]; }
   IKeyboardHandler * GetKeyboardHandler() { return keyboardhandler_; }

   ///////////////////////////////////////
   // Use the machine
   void OnOff();
   void Resync();
   void ForceTick(IComponent* component, int ticks);
   //void StartOptimized(unsigned int nb_cycles);

   enum
   {
      HW_PLUS = 0x1,
      HW_FDC = 0x2,
      HW_TAPE = 0x4,
      HW_COMP = 0x8,
      HW_FULL = 0xF
   }Hardware_Specific;

   typedef void (Motherboard::*Func)(unsigned int nb_cycles);
   typedef Func StartFunction[0x10];
   StartFunction list_start_;

   void Start(unsigned int config, unsigned int nb_cycles);

   //template <bool plus, bool fdc_present, bool components_present>
   template <unsigned int config>
   void StartOptimizedPlus(unsigned int nb_cycles);

   int DebugNew(unsigned int nb_cycles);
   unsigned int GetSpeed() { return speed_percent_; }

   unsigned char* GetCartridge(int index) { return memory_.GetCartridge(index); }
   void EjectCartridge() {memory_.EjectCartridge(); }

   unsigned char* GetRamBuffer() { return memory_.GetRamBuffer(); }

   void ResetCounter() { counter_ = 0; }
   void UpdateExternalDevices();
   void InitStartOptimized();
   void InitStartOptimizedPlus();

   bool run_;
   bool step_in_;
   bool step_;

protected:

   IKeyboardHandler* keyboardhandler_;

   // Debug & running attributes
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
   Bus address_bus_;
   Bus data_bus_;
   Memory memory_;
   Ay8912 psg_;
   NetListINT netlist_int_;
   NetList netlist_nmi_;
   PlayCity play_city_;
   CursorLine cursor_line_;

   Z80 z80_;
   CSig signals_;
   Asic asic_;
   DMA dma_[3];
   GateArray vga_;
   CRTC crtc_;
   PPI8255 ppi_;
   Monitor monitor_;
   CTape tape_;
   FDC fdc_;
   MultifaceII multiface2_;
   
   IPrinterPort* printer_;
   PrinterDefault default_printer_;

   DskTypeManager disk_type_manager_;

   // Running the emulation
   IComponent * component_list_[10]; // Used to avoid allocation
   unsigned int component_elapsed_time_[32];

   int nb_components_;
   int z80_index_;


   template <int MAX_VAL>void Init()
   {
      Init<MAX_VAL - 1>();
      (*(&list_start_))[MAX_VAL] = &Motherboard::StartOptimizedPlus<MAX_VAL>;
   }
   template<>
   void Init<0>()
   {
      (*(&list_start_))[0x0] = &Motherboard::StartOptimizedPlus<0>;
   }
};


#define  RUN_COMPOSANT_N(c,v) if (v <= next_cycle ) v += c.Tick ();
/*
template<>
void Motherboard::Init<0>()
{
   (*(&list_start_))[0x0] = &Motherboard::StartOptimizedPlus<0>;
}*/

//template <bool plus, bool fdc_present, bool components_present>
template <unsigned int config>
void Motherboard::StartOptimizedPlus(unsigned int nb_cycles)
{
   unsigned int next_cycle = nb_cycles;
   unsigned int index = 0;
   unsigned int elapsed_time_psg = component_elapsed_time_[index++];
   unsigned int elapsed_time_z80 = component_elapsed_time_[index++];
   unsigned int elapsed_time_tape = component_elapsed_time_[index++];
   unsigned int elapsed_time_asic = component_elapsed_time_[index++];
   unsigned int elapsed_time_fdc = 0;
   if constexpr (config & HW_FDC)
      elapsed_time_fdc = component_elapsed_time_[index++];

   unsigned int elapsed_components[16];
   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      elapsed_components[i] = component_elapsed_time_[index++];
   }

   unsigned int* elapsed = component_elapsed_time_;
   for (unsigned int i = 0; i < index; ++i)
   {
      if (*elapsed < next_cycle)
      {
         next_cycle = *elapsed;
      }
      ++elapsed;
   }

   while (next_cycle < nb_cycles)
   {
      if (elapsed_time_psg == next_cycle)
      {
         elapsed_time_psg += psg_.Tick();
      }

      if constexpr (config & HW_TAPE)
         RUN_COMPOSANT_N(tape_, elapsed_time_tape);
      if constexpr (config & HW_PLUS)
      {
         if (elapsed_time_asic <= next_cycle) elapsed_time_asic += asic_.crtc_->Tick();
      }
      else
      {
         RUN_COMPOSANT_N(crtc_, elapsed_time_asic);
      }

      if constexpr (config & HW_FDC)
         RUN_COMPOSANT_N((fdc_), elapsed_time_fdc);

      RUN_COMPOSANT_N((z80_), elapsed_time_z80);

      /*if (elapsed_time_dma <= next_cycle)
      {
         dma_[0].Tick();
         dma_[1].Tick();
         elapsed_time_dma += dma_[2].Tick();
      }*/

      if constexpr (config & HW_COMP)
      for (int i = 0; i < signals_.nb_expansion_; i++)
      {
         if (elapsed_components[i] <= next_cycle) 
            elapsed_components[i] += signals_.exp_list_[i]->Tick();
      }

      // Propagate SIG
      signals_.Propagate();
      ++next_cycle;
   }
   index = 0;
   component_elapsed_time_[index++] = elapsed_time_psg - nb_cycles;
   component_elapsed_time_[index++] = elapsed_time_z80 - nb_cycles;
   component_elapsed_time_[index++] = elapsed_time_tape - nb_cycles;
   component_elapsed_time_[index++] = elapsed_time_asic - nb_cycles;
   if constexpr (config & HW_FDC)
      component_elapsed_time_[index++] = elapsed_time_fdc - nb_cycles;
   
   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      component_elapsed_time_[index++] = elapsed_components[i] - nb_cycles;
   }
}