#pragma once

#include "Screen.h"
#include "IComponent.h"
#include "CRTC.h"
#include "KeyboardHandler.h"
#include "IPlayback.h"
#include "FramePacer.h"

/*#ifdef CPCCOREEMU_EXPORTS
#define CPCCOREEMU_API __declspec(dllexport)
#else
#define CPCCOREEMU_API __declspec(dllimport)
#endif
*/
#define CPCCOREEMU_API 

#define RESY 312

class GateArray;

class CPCCOREEMU_API Monitor //: public IComponent
{
public:
   Monitor(void);
   virtual ~Monitor(void);

   void SetPlayback (IPlayback* playback) { playback_ = playback;}
   void SetCRTC (CRTC * crtc){crtc_ = crtc;};
   // Horizontal position setting of the monitor, in Mode 2 pixels (positive : image to the right)
   void SetHorizontalPosition(int pixels) { horizontal_position_ = pixels; }
   // H-HOLD setting of the monitor (VR403 / VR404) : free running line period minus 64 us, in Mode 2
   // pixels per line. kStandardHorizontalHold is the setting of the monitor of the SHAKER photos : without
   // HSYNC, its picture slides to the left by about 10 pixels in 64 lines (D8, #0392).
   static constexpr double kStandardHorizontalHold = 1.10;
   void SetHorizontalHold(double pixels_per_line) { horizontal_hold_ = pixels_per_line; }
   void SetVGA(GateArray * vga);
   void SetKeyboard(IKeyboardHandler* keyboard){keyboard_ = keyboard;}

   void SetScreen ( IDisplay* screen) {screen_ = screen;video_buffer_ = screen_->GetVideoBuffer (0); };
   // Paces each frame before it is handed to the screen (nullptr : no pacing)
   void SetFramePacer(FramePacer* frame_pacer) { frame_pacer_ = frame_pacer; }
   // Emulated time since the last call, in us (the monitor is ticked every us)
   unsigned int TakeElapsedTime() { const unsigned int elapsed = elapsed_us_; elapsed_us_ = 0; return elapsed; }
   void Reset ();
   void Tick ( );

   int* GetVideoBufferForInc () const { if(video_buffer_ != nullptr && x_ < 1024-16) return video_buffer_; else return nullptr;}
   void IncVideoBuffer(){video_buffer_ += 16;};

   unsigned int GetX () { return x_;}
   unsigned int GetY () { return y_;}

   void RecomputeColors ();
   void RecomputeAllColors ();


   typedef enum{
      DISPLAY,
      SYNC,
      BACKPORCH
   } MonitorState;

   ///////////////////////////
   // Sync attributes
   int line_sync_;
   // Beam position x_ where the line flyback of the picture starts : kNoFlyback until the oscillator
   // ends its ramp
   static const int kNoFlyback = 0x40000000;
   int x_total_;
   bool flyback_asked_;
   // AFC loop (LA7800) : position since the flyback start, voltages of pin 1 (C413) and of C414, ramp of
   // the oscillator (0 to 1), true flyback start before the pixel 0 of the line (fraction of pixel)
   int line_position_;
   double afc_pin1_;
   double afc_c414_;
   double oscillator_ramp_;
   double flyback_phase_;
   double horizontal_hold_;
   double LinePeriod() const;
   MonitorState horizontal_state_;
   int horizontal_position_;
   int horizontal_hold_count_;
   bool syncv_;

   MonitorState vertical_state_;

   // Beam position
   int x_;
   int y_;
   
   // Screen implementation1  
   int horizontal_total_;
   int* video_buffer_;

   IDisplay*   screen_;
   FramePacer* frame_pacer_;
   unsigned int elapsed_us_;
   GateArray* gate_array_;
   Memory* memory_;
   IPlayback* playback_;
   CRTC* crtc_;
   IKeyboardHandler* keyboard_;

   ////////////////////
   // Far attributes
   int vertical_tolerate_;
   int vertical_sync_;

   // Static values
   int res_x_;
   int vertical_display_;
   
   int backporch_;

   int vertical_sync_start_;
   int expected_vertical_sync_start_;

   unsigned int byte_to_pixel02_[0x100];
   unsigned int byte_to_pixel00_[0x100];
   unsigned int byte_to_pixel01_[0x100];
   unsigned int byte_to_pixel03_[0x100];
   unsigned int byte_to_pixel03_b_[0x100];

   bool playback_sync_;

};

