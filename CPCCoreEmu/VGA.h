#pragma once

#include "Bus.h"
#include "Sig.h"
#include "Memoire.h"
#include "Screen.h"
#include "Monitor.h"
#include "IComponent.h"
#include "ClockLine.h"
#include "DMA.h"

//#include "dma.h"

/*#ifdef CPCCOREEMU_EXPORTS
#define CPCCOREEMU_API __declspec(dllexport)
#else
#define CPCCOREEMU_API __declspec(dllimport)
#endif
*/
#define CPCCOREEMU_API 

class CRTC;
class DMA;

class CPCCOREEMU_API GateArray : public IComponent
{
   friend class EmulatorEngine;
   friend class Monitor;
   friend class CSnapshot;
public:

   typedef enum {
      GA_40010    = 0,
      GA_OTHER    = 1,
      MAX_GA
   } TypeGa;

   GateArray(void);
   virtual ~GateArray(void);


   void Reset ();

   DMA* GetDMAChannel(int channel);
   void SetDMA(DMA* dma_list) {
      dma_list_ = dma_list;
   }
   void SetPAL (bool bPAL) { pal_present_ = bPAL ; }
   void SetPlus(bool plus) { plus_ = plus; }
   // Initialisation et branchement des pins
   void SetMonitor(Monitor* monitor);
   void SetBus (Bus* address, Bus* data);
   void SetSig ( CSig* sig ) {sig_handler_ = sig;}
   void SetMemory  (Memory* mem) { memory_ = mem;memory_ram_buffer_ = memory_->ram_buffer_[0];}
   void SetCRTC (CRTC * crtc){crtc_ = crtc;}
   void SetScanlines ( int scan ) {scanline_type_ = scan;}
   int GetScanlines ( ) {return scanline_type_ ;}
   

   void ComputeSpritePerLine(int sprite_number );
   void ComputeSpritePerColumn(int sprite_number);
   void DrawSprites(int * buffer_display);
   void HandleDMA();
   unsigned short GetSSA() { return ssa_; }

   const bool IsAsicLocked() { return unlocked_ == false; }
   void Unlock(bool unlock)
   {
      unlocked_ = unlock;
   }

   // Demarrage
   unsigned int Tick ();
   void PreciseTick ();
   void TickIO ();
   void TickDisplays ();


   //int m_X, m_Y;
   unsigned char* memory_ram_buffer_;
   unsigned char interrupt_counter_;   // 6 bits counter
   bool interrupt_raised_;
   unsigned char wait_for_hsync_ ;

   // Sync is already devided into two usable signals
   bool hsync_;
   bool vsync_;
   bool speed_;

//protected:

   bool pal_present_;
   // Cached !
   bool dispen_buffered_;

   // 
   int scanline_type_;       // 1 : scanlines / 0 : copylines
   // CRTC 
   CRTC * crtc_;

   // Bus d'adresse
   Bus* address_bus_;

   // Bus de données
   Bus* data_bus_;

   // Signaux;
   CSig* sig_handler_;

   // Memory
   Memory * memory_;
   
   // Colors
   // Penr
   unsigned char pen_r_;
   unsigned int ink_list_ [16];
   unsigned int sprite_ink_list_[16];
   unsigned int buffered_ink_ ;
   bool buffered_ink_available_;

   unsigned char screen_mode_;
   unsigned char buffered_screen_mode_;
 
   // Internal counter
   bool h_old_sync_;
   bool v_old_sync_;

   unsigned char hsync_counter_;
   unsigned char vsync_counter_;
   unsigned char activation_mode_;

   Monitor* monitor_;

   union 
   {

      unsigned short word;
      struct  {
         unsigned char l;
         unsigned char h;
      } byte;

   } display_short_;

   TypeGa type_gate_array_;
  
   int vsync_count_;
   
   /////////////////////////////
   // Precise datas
   CClockLine clock_4_mhz_;
   CClockLine clock_1_mhz_;

   // Clocks 
   unsigned char clock_count_;

#define NB_BYTE_BORDER 4
   unsigned int video_border_[NB_BYTE_BORDER];
   unsigned int byte_to_pixel00_[0x100];
   unsigned int byte_to_pixel01_[0x100];
   unsigned int byte_to_pixel03_[0x100];
   unsigned int byte_to_pixel03_b_[0x100];
   unsigned int byte_to_pixel_mode01[0x100];

   bool unlocked_;
   bool plus_;

   unsigned short prev_col_;
   unsigned char pri_;

   DMA* dma_list_;

   unsigned short ssa_new_counter_;
   unsigned short ssa_new_;
   unsigned short ssa_;

   // Precomputed sprite (for faster sprite drawing)
   unsigned short sprite_lines_[0x200];
   short sprite_line_begin[0x10];

   unsigned short sprite_column_[0x100];

};

