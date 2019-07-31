#pragma once

#include "ICfg.h"

#define CPCCOREEMU_API 

class IFullScreenInterface
{
public :
   virtual ~IFullScreenInterface() = default;
   virtual void SetFullScreen (bool on) = 0;
};

class CPCCOREEMU_API IDisplay
{
public:
   
   virtual ~IDisplay() = default;

   using SizeEnum = enum {
      S_STANDARD = 1,
      S_BIG,
      S_MAX
   };

   virtual void WindowsToTexture(int &x, int &y) {};
   virtual void SetScanlines ( int scan ) = 0;
   virtual bool AFrameIsReady  () = 0;
   virtual void Display() = 0;
   virtual void Refresh() {};
   virtual void Config () = 0;
   virtual const char* GetInformations () = 0;
   virtual int GetWidth () = 0;
   virtual int GetHeight () = 0;
   virtual void SetSize (SizeEnum size) = 0;
   virtual SizeEnum  GetSize () = 0; 
   virtual void VSync (bool dbg=false) = 0;

   // Start of sync
   virtual void StartSync() = 0;
   // Wait VBL
   virtual void WaitVbl () = 0;


   virtual int* GetVideoBuffer (int y) = 0;
   virtual void Reset () = 0;
   virtual void FullScreenToggle () = 0;
   virtual void ForceFullScreen (bool fullscreen ) = 0;
   virtual void Screenshot () = 0;
   virtual void ScreenshotEveryFrame(int on) = 0;
   virtual bool IsEveryFrameScreened() = 0;

   virtual bool SetSyncWithVbl ( int speed ) = 0;
   virtual bool IsWaitHandled() = 0;
   virtual bool IsDisplayed () = 0;
   virtual bool GetBlackScreenInterval () = 0;
   virtual void SetBlackScreenInterval (bool on) = 0;

   virtual void WindowChanged (int x_in, int y_in, int wnd_width, int wnd_height) = 0;

   virtual void ResetLoadingMedia() = 0;
   virtual void SetLoadingMedia() = 0;

   virtual void ResetDragnDropDisplay () = 0;
   virtual void SetDragnDropDisplay  ( int type ) = 0;
   virtual void SetCurrentPart ( int x, int y ) = 0;
   virtual int GetDnDPart () = 0;

   // Capability of device
   virtual bool CanVSync () { return false; }
   virtual bool CanInsertBlackFrame() { return false; }
   virtual void Activate ( bool on ) {};

};

