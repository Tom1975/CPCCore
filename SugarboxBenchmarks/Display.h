#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <string>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <Commctrl.h>
#include <Windowsx.h>
#include <Shellapi.h>
#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")
#include "screen.h"

// Display
class CDisplay : public IDisplay
{
public :
   CDisplay ();
   virtual ~CDisplay ();

   virtual void SetScanlines ( int scan ) {};
   virtual void Display() {};
   virtual bool AFrameIsReady  () {return true;};
   virtual void Config () {};
   virtual const char* GetInformations () { return "TCL GDI";};
   virtual int GetWidth () ;
   virtual int GetHeight () ;
   virtual void HSync () ;
   virtual void VSync (bool bDbg) ;
   virtual void StartSync();
   virtual void WaitVbl () ;
   virtual int* GetVideoBuffer (int y) ;
   virtual void Reset () ;
   virtual void Screenshot (){};
   virtual void ScreenshotEveryFrame(int bSetOn) {};
   virtual bool IsEveryFrameScreened() {
      return false;
   }
   virtual bool IsDisplayed() { return true;/* m_bShow;*/ };
   virtual void FullScreenToggle (){};
   virtual void ForceFullScreen (bool bSetFullScreen ){}
   virtual void WindowChanged (int xIn, int yIn, int wndWidth, int wndHeight){};
   virtual bool SetSyncWithVbl ( int speed ){return false; };
   virtual bool IsWaitHandled() { return false; };
   virtual bool GetBlackScreenInterval () { return false ;};
   virtual void SetBlackScreenInterval (bool bBS) { };

   void Init ();
   void Show ( bool bShow );
   virtual void StopMessaging  ( bool bStop ){stop_=bStop;};

   virtual void SetSize (SizeEnum size){};
   virtual SizeEnum  GetSize () { return S_STANDARD; };

   static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

   virtual void ResetLoadingMedia() {};
   virtual void SetLoadingMedia() {};

   virtual void ResetDragnDropDisplay () {};
   virtual void SetDragnDropDisplay  ( int type ){};
   virtual void SetCurrentPart (int x, int y ){};
   virtual int GetDnDPart () { return 0;};

protected:
   // Screenshot detection
   bool screenshot_detection_;
   bool screenshot_found_;
   Bitmap * bmp_too_detect_;
   BitmapData lockedBitmapData_;

   bool screenshot_take_;
   std::string screenshot_name_;

   bool m_bShow;
   bool stop_; 
   // Displayed window : 
   int m_X, m_Y;
   int m_Width;
   int m_Height;

   HWND        m_hWnd;
   HDC         m_hwndDC;
   HDC         m_MemDC;
   HBITMAP     m_Bitmap;
   HBITMAP     m_iOldBmp;
   HBITMAP     m_iBitmap;
   Bitmap*     m_BmpMem;
   BITMAPINFO  bi24BitInfo; // We set this up to grab what we want
   int *       bBytes ;

};
