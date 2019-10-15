#pragma once
#include "Sig.h"
#include "PPI.h"
#include "ILog.h"
#include "IComponent.h"
#include "IPlayback.h"
#include "ClockLine.h"

class GateArray;

class CRTC : public IComponent
{
friend class EmulatorEngine;
public:

   CRTC(void);
   virtual ~CRTC(void);


   typedef enum {
      HD6845S  = 0,
      UM6845   = 0,
      UM6845R  = 1,
      MC6845   = 2,
      AMS40489 = 3,
      AMS40226    = 4,
      MAX_CRTC
   } TypeCRTC;

   void SetPlayback (IPlayback* playback) { play_back_ = playback;}
   void DefinirTypeCRTC(TypeCRTC type_crtc);
   void Reset ();
   void SetLog ( ILog* log ) {log_ = log;};
   void SetSig ( CSig* sig ) {signals_ = sig;signals_->h_sync_ = false;signals_->v_sync_ = false;};
   void SetGateArray ( GateArray* vga ) {gate_array_ = vga;};
   void SetPPI ( PPI8255* ppi) {ppi_ = ppi;};
   void Out (unsigned short address, unsigned char data);
   unsigned int Tick ( );
   void SetCursorLine(IClockable * cursor_line) {cursor_line_ = cursor_line;};

   void GunSet(int x, int y, int button) { gun_x_ = x; gun_y_ = y; gun_button_ = button; };

   unsigned char In ( unsigned short address );



   TypeCRTC type_crtc_;

   int gun_x_;
   int gun_y_;
   int gun_button_;

   unsigned short ma_;
   unsigned char vlc_;          // Raster counter

   // Attributes
   unsigned char registers_list_[32];       // From R0 to R17
   unsigned char registers_mask_[32];   // Mask for bit that are usefull
   unsigned char adddress_register_;
   unsigned char status_register_;

   // Signaux;
   CSig* signals_;
   GateArray* gate_array_;
   PPI8255* ppi_;

   // Status (CRTC3)
   unsigned char status1_;
   unsigned char status2_;

   // Internal Counters
   unsigned char hcc_;          // Horizontal character counter
   unsigned char horinzontal_pulse_;       // Horizontal sync width counter
   unsigned char vcc_;          // Line counter
   unsigned char scanline_vbl_;  // Vertical sync width counter
   bool r4_reached_;

   bool ff1_; 
   //bool ff2_;
   bool ff3_;
   bool ff4_;
   bool mux_;
   bool mux_set_ ;
   bool mux_reset_ ;

   bool lightpen_input_;
   bool de_bug_;

   unsigned char vertical_sync_width_;
   unsigned char horizontal_sync_width_;   
   unsigned char vertical_adjust_counter_;

   unsigned short bu_;

   bool r9_triggered_;
   bool r4_triggered_;

   bool even_field_;
   int sscr_bit_8_;

   typedef void (CRTC::*Func)();
   Func TickFunction;

//protected:

   IPlayback* play_back_ ;
   ILog* log_ ;

   //void ClockTick ();
   void ClockTick0 ();
   void ClockTick1 ();
   void ClockTick2 ();
   void ClockTick34 ();

   void ComputeMux1 ();

   bool v_no_sync_;
   bool h_no_sync_;


//   bool h_;
//   bool h_end_;
//   bool HSyncLowEdge;
//   bool ff4_reset ;

//   bool m_bResetVLC;

//   bool m_bTrickR4;

   bool inc_vcc_;

   IClockable * cursor_line_;

   // PLUS FEATURES
   unsigned short ssa_;
   bool ssa_ready_;
   bool shifted_ssa_;
   bool splt_on_;
};

