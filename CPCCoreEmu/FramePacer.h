#pragma once

#include <chrono>
#include <thread>

// Paces the emulation on the frames of the emulated monitor : each frame is handed to the display
// at the moment it is due in real time. On a variable refresh rate screen (G-Sync, FreeSync), the
// screen refreshes when the frame arrives : the frames must leave evenly spaced, not in bursts.
// The emulated time comes from the monitor, so the pace follows the real frame rate of the
// machine (50.08 Hz, 60 Hz, or anything a CRTC program produces).
class FramePacer
{
public:
   void SetActive(bool active)
   {
      active_ = active;
      resync_ = true;
   }

   bool IsActive() const { return active_; }

   // Number of frames paced since the last call
   unsigned int TakeFrameCount()
   {
      const unsigned int count = frame_count_;
      frame_count_ = 0;
      return count;
   }

   // A frame ends : wait until the real time catches up with the emulated time (in us)
   void FrameEnd(unsigned int emulated_us)
   {
      if (!active_) return;
      ++frame_count_;
      Wait(emulated_us);
   }

   // Wait for some emulated time (in us) that ended without any frame
   void Wait(unsigned int emulated_us)
   {
      if (!active_) return;

      const clock::time_point now = clock::now();
      // Late by far (pause, debugger, loading, window moved...) : start again from now, rather than
      // running at full speed to catch up
      if (resync_ || now > deadline_ + max_late_)
      {
         deadline_ = now;
         resync_ = false;
      }
      deadline_ += std::chrono::microseconds(emulated_us);
      if (deadline_ > now + max_ahead_)
      {
         deadline_ = now + max_ahead_;
      }

      // Sleep while far from the deadline (a sleep can last more than asked), then spin to hit it
      for (;;)
      {
         const clock::duration remaining = deadline_ - clock::now();
         if (remaining <= clock::duration::zero())
            break;
         if (remaining > spin_)
            std::this_thread::sleep_for(remaining - spin_);
         else
            std::this_thread::yield();
      }
   }

protected:
   using clock = std::chrono::steady_clock;

   const clock::duration max_late_ = std::chrono::milliseconds(100);
   const clock::duration max_ahead_ = std::chrono::milliseconds(200);
   const clock::duration spin_ = std::chrono::microseconds(1500);

   bool active_ = false;
   bool resync_ = true;
   unsigned int frame_count_ = 0;
   clock::time_point deadline_;
};
