
#include "Display.h"

#ifdef WIN32

#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   800
#define DISP_WINDOW_Y   600


char * g_szWindowClass = "SugarBoxTCLWindowClass";
ATOM pAtom = NULL;
CDisplay* sMainWindow;


ATOM MyRegisterClass(HINSTANCE hInstance)
{
   GdiplusStartupInput gdiplusStartupInput;
   ULONG_PTR           gdiplusToken;
   GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

   WNDCLASSEX wcex;

   wcex.cbSize = sizeof(WNDCLASSEX);

   wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wcex.lpfnWndProc = CDisplay::WndProc;
   wcex.cbClsExtra = 0;
   wcex.cbWndExtra = 0;
   wcex.hInstance = hInstance;
   wcex.hIcon = NULL;
   wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
   wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
   wcex.lpszMenuName = NULL;
   wcex.lpszClassName = g_szWindowClass;
   wcex.hIconSm = NULL;

   return RegisterClassExA(&wcex);
}
#endif

CDisplay::CDisplay () : bBytes(nullptr)
#ifdef WIN32
, m_BmpMem(nullptr)
#endif
{
#ifdef WIN32
   m_bShow = true;
   m_hWnd = NULL;
   stop_ = false;
   m_X = m_Y = m_Width = m_Height = 0;
   if (pAtom == NULL)
      pAtom = MyRegisterClass(NULL);

   sMainWindow = this;
#endif
}

CDisplay::~CDisplay()
{
#ifdef WIN32
#else
   delete[]bBytes;
#endif
}

int CDisplay::GetWidth ()
{
   return 1024;
}

int CDisplay::GetHeight ()
{
   return 1024; //REAL_DISP_Y;
}

int* CDisplay::GetVideoBuffer (int y ){
#ifdef WIN32
   return &bBytes[(REAL_DISP_Y - y * 2 - 2) * REAL_DISP_X];
#else
   return bBytes;
#endif
}
void CDisplay::Reset () {;}

void CDisplay::Show ( bool bShow )
{
#ifdef WIN32
   m_bShow = bShow;
   ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
#endif
}

void CDisplay::Init ()
{
#ifdef WIN32
   m_hWnd = CreateWindowEx(0, g_szWindowClass, "SugarTCL", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER,
      0, 0, DISP_WIDTH, DISP_HEIGHT, NULL, NULL, NULL, NULL);

   m_hwndDC = GetDC(NULL);
   m_MemDC = CreateCompatibleDC(m_hwndDC);

   // DIB Section
   bi24BitInfo.bmiHeader.biBitCount = 32; // rgb 8 bytes for each component(3)
   bi24BitInfo.bmiHeader.biCompression = BI_RGB;// rgb = 3 components
   bi24BitInfo.bmiHeader.biPlanes = 1;
   bi24BitInfo.bmiHeader.biSize = sizeof(bi24BitInfo.bmiHeader); // size of this struct
   bi24BitInfo.bmiHeader.biWidth = REAL_DISP_X/*DISP_WIDTH*/; // width of window
   bi24BitInfo.bmiHeader.biHeight = REAL_DISP_Y/*DISP_HEIGHT*/; // height of window
   bi24BitInfo.bmiHeader.biSizeImage = 0;


   m_Width = REAL_DISP_X;
   m_Height = REAL_DISP_Y;
   //bBytes = new BYTE[bi24BitInfo.bmiHeader.biWidth * bi24BitInfo.bmiHeader.biHeight * 4]; // create enough room. all pixels * each color component
   HDC hDC;
   hDC = CreateCompatibleDC(m_hwndDC);
   m_iBitmap = CreateDIBSection(m_hwndDC, &bi24BitInfo, DIB_RGB_COLORS, (void**)&bBytes, 0, 0); // create a dib section for the dc

   m_BmpMem = new Bitmap(m_iBitmap, NULL);

   m_iOldBmp = (HBITMAP)SelectObject(m_MemDC, m_iBitmap); // assign the dib section to the dc
   DeleteDC(hDC);

   ReleaseDC(NULL, m_hwndDC);

   Reset();
#else
   bBytes = new int[1024 * 4]; // create enough room. all pixels * each color component
#endif
}



void CDisplay::HSync ()
{
}

void CDisplay::StartSync()
{

}

void CDisplay::VSync (bool bDbg)
{
#ifdef WIN32
   if (m_bShow)
   {
      m_hwndDC = GetDC(m_hWnd);
      BitBlt(m_hwndDC, 0, 0, m_Width, m_Height, m_MemDC, m_X, m_Y, SRCCOPY);
      ReleaseDC(m_hWnd, m_hwndDC);

   }
#endif
}

void CDisplay::WaitVbl ()
{
}


LRESULT CALLBACK CDisplay::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   //	int wmId, wmEvent;
   PAINTSTRUCT ps;
   HDC hdc;

   // Get instance ID
   CDisplay* pMainWindow = sMainWindow;

   switch (message)
   {
   case WM_LBUTTONDOWN:
      SetFocus(hWnd);
      break;
   case WM_SYSKEYDOWN:
   case WM_KEYDOWN:
      // Accelerator
      if (pMainWindow != NULL)
      {
         MSG msg;
         msg.hwnd = hWnd;
         msg.wParam = wParam;
         msg.lParam = lParam;
         msg.message = message;

         //pMainWindow->m_pKeyHandler->SendScanCode( (lParam >>16)&0x1FF, true);
         //}
      }
      break;
   case WM_SYSKEYUP:
   case WM_KEYUP:
      //if (pMainWindow!= NULL)pMainWindow->m_pKeyHandler->SendScanCode( (lParam >>16)&0x1FF, false);
      break;
   case WM_ERASEBKGND:
      return TRUE;
      break;
   case WM_CTLCOLORDLG:
      return TRUE;
      break;

      /*
   case WM_DROPFILES:
   {
      if (pMainWindow!= NULL)
      {
         HDROP hDrop = (HDROP)wParam;
         pMainWindow->DropFiles ( hDrop );

      }
      DragFinish((HDROP)wParam);

      return 0;
   }*/
   case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      // TODO: Add any drawing code here...
      EndPaint(hWnd, &ps);
      break;
   default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}