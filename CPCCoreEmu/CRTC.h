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
   friend class MachineState;

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
   // I/O write from the Z80 : t_state is the quarter of microsecond (0..3) of the Gate Array
   // microsecond where the I/O cycle asserts IORQ (T2 of the I/O cycle, 4.4.4)
   void Out (unsigned short address, unsigned char data, unsigned int t_state = 0);
   unsigned int Tick ( );
   // One CRTC clock : character logic, bus interface, VSYNC pin (everything but the Gate Array)
   void ClockCharacter();
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
   // CRTC 0 vertical logic : latches sampling comparators at fixed C0 positions (see CRTC_0.cpp)
   bool c9_managed_;            // C9 handling : enabled when C0 reaches 1, disabled when C0 restarts at 0
   bool line_end_;              // registered C0==R0, sampled at the start of a character
   bool c4_increment_;          // registered C9==R9 : C4 is incremented at the line end
   bool last_line_;             // C4==R4 && C9==R9, sampled at the end of characters 0 and 1
   bool adjust_;                // additional management armed
   bool adjust_confirmed_;      // additional management confirmed at the end of character 2 : C9 compared to R5
   bool adjust_end_;            // next C9 == R5, sampled at the end of characters 0 to 2
   bool vsync_allowed_;         // C4==R7 may start a VSYNC : set at the end of character 2, cleared at C0 reset
   bool c3h_load_;              // VSYNC started during a line : C3h is cleared at the next C0 reset
   // CRTC 1 (see CRTC_1.cpp)
   bool vma_reload_;            // VMA is loaded from R12/R13 at the line start instead of VMA'
   bool vma_reload_clear_;      // C0==R1 && C9==R9 seen : the reload state is cleared at the line end
   bool rfd_;                   // R5 0 -> !0 written on C0==R0 (R.F.D.)
   bool status_border_r6_;      // status bit 5 : BORDER R6 state, updated at the line end
   // CRTC 2 (see CRTC_2.cpp)
   bool c9_eq_r9_at_start_;     // C9==R9 sampled at the line start, before the writes on C0=0
   bool hsync_on_line_start_;   // a HSYNC started on C0=0
   bool last_line_eq_;          // previous C4==R4 && C9==R9, for the rising edge on a register write
   bool dlp_;                   // "Dernière Ligne Précédente"
   bool gdl_reenabled_;         // "Gestion Dernière Ligne" re-enabled on the last HSYNC character
   bool vsync_ghost_;           // GHOST VSYNC : counted, but the pin is not raised
   // CRTC 3/4
   unsigned char frame_counter_; // frames counter : STATUS 2 bit 3 toggles every 16 frames (21.3.4.2)
   // Interlace (19.5 to 19.7) : even_field_ is ParitéFrame (true = even frame)
   bool parity_r6_;             // CRTC 0/2 ParitéR6 (true = odd) : loaded with !ParitéFrame when C4 reaches R6
   bool r6_eq_prev_;            // previous C4==R6, for the rising edge loading ParitéR6
   bool vsync_mid_pending_;     // MID-VSYNC : the VSYNC starts when C0 reaches R0/2
   bool interlace_line_;        // CRTC 3/4 : additional interlace line in progress (C9 forced to 0)
   bool parity_c9_;             // CRTC 1/3/4 ParitéC9 (true = odd) : bit 0 of C9 in Interlace Video Mode
   bool rfd_parity_;            // CRTC 1 : R.F.D. armed the parity in the C9==R9 test on C0==R1 until the frame end
   bool ivm_latched_;           // CRTC 0 : Interlace Video Mode state taken when C0 restarts at 0 (19.8.1)
   bool vsync_line_delay_;      // CRTC 0/3/4 : IVM VSYNC delayed to the next line start (19.5.2, 19.5.5)
   unsigned char c9_ivm_;       // CRTC 2 : C9.IVM counter, used for the address in IVM (19.8.3)
   // Bus interface (4.4.4) : I/O seen too late in the current window, taken by the next one
   bool io_pending_;
   unsigned short io_pending_address_;
   unsigned char io_pending_data_;
   // HSYNC pin level on each quarter of microsecond (bit q = T-state q) of the current / previous
   // character : an I/O can move an edge inside the character (R2.JIT, R3.JIT - 14.5.4, 14.7.1)
   unsigned char hsync_quarters_;
   unsigned char hsync_quarters_previous_;
   // Internal HSYNC (C0==R2 / C3l==R3l logic) and its events on the current character ; the pin
   // follows it directly on CRTC 0, 1, 2, through a one character delay on CRTC 3, 4 (14.7.2)
   bool hsync_;
   bool hsync_rise_;
   bool hsync_fall_;
   bool hsync_pin_stage_;

   bool ff1_; 
   //bool ff2_;
   bool ff3_;
   bool ff4_;
   bool mux_;
   bool mux_set_ ;
   bool mux_reset_ ;

   bool lightpen_input_;
   unsigned char dispen_history_;   // DISPEN of the previous characters, 2 bits (halves) per character, for the R8 SKEW
   bool dispen_half0_;          // DISPEN for the first half (byte) of the current character
   bool dispen_half1_;          // DISPEN for the second half (byte) of the current character

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

   void WriteBus(unsigned short address, unsigned char data, unsigned int t_state);
   bool HSyncPinFollowsComparators() const;
   unsigned int HSyncPinEdge(bool rise, unsigned int t_state) const;
   void HSyncStartComparatorChanged(unsigned int t_state);
   void HSyncEndComparatorChanged(unsigned int t_state);
   unsigned int LastTStateInWindow() const;
   void ClockBusInterface();
   void ClockHSyncPin();
   void ComputeSyncWidths();
   void ClockHSync(bool& started, bool& ended);
   void ClockDispTmg() { dispen_history_ = (dispen_history_ << 2) | (dispen_half1_ ? 2 : 0) | (dispen_half0_ ? 1 : 0); }
   void ClockDispEnHalvesCrtc02();
   void ClockDispEnHalves() { dispen_half0_ = dispen_half1_ = (ff1_ && ff3_); }
   // DISPEN for the first (half = 0) or the second (half = 1) half of the current character
   bool DispEn(int half = 0) const;
   bool VSyncPin() const { return ff4_ && !vsync_ghost_; }
   unsigned char ReadRegister();
   bool C9EqualsR9() const;
   bool InterlaceOn() const { return (registers_list_[8] & 0x01) != 0; }
   bool InterlaceVideo() const { return (registers_list_[8] & 0x03) == 0x03; }
   unsigned int ParityC9Crtc0() const;
   unsigned char AddressC9() const;
   void ClockParityR6();
   bool ClockMidVSync();

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

