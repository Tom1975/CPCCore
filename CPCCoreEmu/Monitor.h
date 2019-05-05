#pragma once

#include "Screen.h"
#include "IComponent.h"
#include "CRTC.h"
#include "IPlayback.h"

/*#ifdef CPCCOREEMU_EXPORTS
#define CPCCOREEMU_API __declspec(dllexport)
#else
#define CPCCOREEMU_API __declspec(dllimport)
#endif
*/
#define CPCCOREEMU_API 

#define RESY 312

class GateArray;

class CPCCOREEMU_API Monitor : public IComponent
{
public:
   Monitor(void);
   virtual ~Monitor(void);

   void SetPlayback (IPlayback* playback) { playback_ = playback;}
   void SetCRTC (CRTC * crtc){crtc_ = crtc;};
   void SetVGA(GateArray * vga);

   void SetScreen ( IDisplay* screen) {screen_ = screen;video_buffer_ = screen_->GetVideoBuffer (0); };
   void Reset ();
   unsigned int Tick ( /*unsigned int nbTicks*/);

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
   int tmp_sync_count_;
   int line_sync_;
   int hsync_total_;
   bool hsync_found_;
   int x_total_;
   int expected_hbl_;
   int offset_;
   int hsync_count_;
   MonitorState horizontal_state_;
   int horizontal_synchronisation_;
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
   GateArray* gate_array_;
   Memory* memory_;
   IPlayback* playback_;
   CRTC* crtc_;

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

