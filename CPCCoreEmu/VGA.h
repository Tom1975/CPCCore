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
   // R52 asks the Z80 for an interrupt
   void RequestRasterInterrupt();

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
   // One microsecond is two steps around the clock of the CRTC : TickVideo() at the end of the
   // microsecond (before the CRTC moves to the next character), TickSignals() at its start (after)
   unsigned int TickVideo ();
   void TickSignals ();
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
   bool dispen_buffered_;     // DISPEN for the first byte of the character
   bool dispen_buffered_h_;   // DISPEN for the second byte (half microsecond) of the character
   bool half_border_;         // only one of the two bytes is displayed
   void ApplyHalfBorder(int* buffer);

   // HSYNC black (CBLACK_HSYNC, Compendium 16.2.3). A block is drawn on the tick of its character, but
   // the HSYNC pin of that character is final only at the next tick (an I/O can move an edge inside the
   // character : R2.JIT, R3.JIT). The black is applied then, and the block is given to the monitor.
   struct BlackEdge { unsigned char pixel; bool level; };
   void FinalizeBlock();
   // GATE ARRAY 40007, 40008, 40010 and ASIC 40226 : the pixels come out of an 8 bit shift register
   // (9.1, 9.2.2, 9.3.4). A byte is loaded on the 1st Mode 2 pixel of its half of the block in Mode 2,
   // on the 2nd one in the other modes ; a divider shifts it every 1, 2 or 4 pixels (Mode 2, 1, 0 / 3),
   // zeros come in. A new mode reaches the decoder on the pixel 5 of the block, the divider on the 6th.
   void ShiftRegisterBlock(int* buffer);
   unsigned char sr_;                // shift register
   unsigned char sr_div_;            // pixels since the last shift
   unsigned char sr_mode_;           // mode of the decoder
   unsigned char sr_div_mode_;       // mode of the divider
   bool sr_shown_;                   // DISPEN of the byte in the register (else : border)
   int* last_block_;               // block drawn on the previous tick (nullptr : out of the screen)
   bool monitor_pending_;          // the previous block has not been given to the monitor yet
   bool cblack_hsync_;             // CBLACK_HSYNC at the start of the next block to finalize
   bool hsync_pin_last_;           // HSYNC pin at the end of the last finalized character
   unsigned char nb_black_edges_;  // CBLACK_HSYNC edges not reached yet (pixel from the next block)
   BlackEdge black_edges_[8];
   // C-HSYNC to the monitor (SIG_GA_HSYNC, 16.2.2), Mode 2 pixel precision
   int chsync_countdown_;          // pixels before H06 reaches 2 (-1 : not counting)
   bool chsync_;                   // SIG_GA_HSYNC
   int chsync_length_;             // pixels since SIG_GA_HSYNC is high
   unsigned short chsync_mask_;    // SIG_GA_HSYNC on each pixel of the finalized block (bit 0 = 1st pixel)
   // VSYNC black (CBLACK_VSYNC, 16.2.3) : from the rising edge of the CRTC VSYNC to the end of the 26th HSYNC
   bool vsync_rise_in_block_;      // the CRTC VSYNC rose on the block not finalized yet
   bool vsync_rise_pending_;       // the CRTC VSYNC rose on this character : for the block drawn at its end
   bool cblack_vsync_;
   unsigned char v26_;

   // 
   int scanline_type_;       // 1 : scanlines / 0 : copylines
   // CRTC 
   CRTC * crtc_;

   // Bus d'adresse
   Bus* address_bus_;

   // Data bus
   Bus* data_bus_;

   // Signaux;
   CSig* sig_handler_;

   // Memory
   Memory * memory_;
   
   // Colors
   // Penr
   unsigned char pen_r_;
   unsigned int ink_list_ [16];
   unsigned char ink_regs_ [16];  // raw GA color byte (data & 0x5F) for each pen
   unsigned char border_reg_;     // raw GA color byte for border
   unsigned int sprite_ink_list_[16];
   unsigned int buffered_ink_ ;
   unsigned char buffered_ink_reg_;
   unsigned char buffered_pen_;     // pen of the pending ink (PENR may change before the ink is applied)
   bool buffered_ink_available_;

   unsigned char screen_mode_;
   unsigned char buffered_screen_mode_;
 
   // Internal counter
   bool h_old_sync_;
   bool v_old_sync_;

   unsigned char hsync_counter_;
   unsigned char vsync_counter_;
   unsigned char activation_mode_;
   bool mode_window_;                // this microsecond is the 2nd to 6th of the HSYNC of the GATE ARRAY

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

