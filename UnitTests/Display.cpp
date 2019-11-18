
#include "Display.h"


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

	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= CDisplay::WndProc;
	wcex.cbClsExtra	= 0;
	wcex.cbWndExtra	= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName= g_szWindowClass;
	wcex.hIconSm		= NULL;

	return RegisterClassExA(&wcex);
}

CDisplay::CDisplay () : screenshot_detection_(false), screenshot_found_(false), bmp_too_detect_(NULL), screenshot_take_(false),
   m_BmpMem(nullptr)
{

   m_bShow = true;
   m_hWnd = NULL;
   stop_ = false;
   m_X = m_Y = m_Width = m_Height = 0;
   if (pAtom == NULL)
      pAtom = MyRegisterClass (NULL);

   sMainWindow = this;

}

CDisplay::~CDisplay()
{
   if (m_hWnd != NULL )
   {
      DestroyWindow ( m_hWnd );
   }

   // release 
   SelectObject(m_MemDC, m_iOldBmp ); // assign the dib section to the dc
   DeleteObject ( m_MemDC);

   DeleteObject (m_iBitmap);
   delete m_BmpMem;
   delete bmp_too_detect_;
}

int CDisplay::GetWidth ()
{
   return m_Width;
}

int CDisplay::GetHeight ()
{
   return m_Height; //REAL_DISP_Y;
}

int* CDisplay::GetVideoBuffer (int y ){return &bBytes[ (REAL_DISP_Y - y*2 - 2) * REAL_DISP_X];}
void CDisplay::Reset () {memset (bBytes, 0, REAL_DISP_X*REAL_DISP_Y*4);}

void CDisplay::Show ( bool bShow )
{
   m_bShow = bShow;
   ShowWindow ( m_hWnd, bShow?SW_SHOW:SW_HIDE);
}

void CDisplay::Init ()
{

   m_hWnd  = CreateWindowEx (0, g_szWindowClass, "SugarTCL", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |WS_BORDER ,
      0, 0, DISP_WIDTH, DISP_HEIGHT, NULL, NULL, NULL, NULL);

   m_hwndDC = GetDC(NULL);
   m_MemDC = CreateCompatibleDC ( m_hwndDC );

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
   hDC=CreateCompatibleDC(m_hwndDC);
   m_iBitmap = CreateDIBSection (m_hwndDC, &bi24BitInfo, DIB_RGB_COLORS, (void**)&bBytes, 0, 0); // create a dib section for the dc
   
   m_BmpMem = new Bitmap( m_iBitmap,  NULL );

   m_iOldBmp = (HBITMAP) SelectObject(m_MemDC, m_iBitmap); // assign the dib section to the dc
   DeleteDC(hDC);

   ReleaseDC (NULL, m_hwndDC);

   Reset();

}



int CDisplay::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if (size == 0)
      return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if (pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for (UINT j = 0; j < num; ++j)
   {
      if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

void CDisplay::InitScreenshotDetection(const char* pathFile)
{
   wchar_t path_wchar[MAX_PATH] = { 0 };
   MultiByteToWideChar(CP_ACP, 0, pathFile, strlen(pathFile), path_wchar, MAX_PATH);

   bmp_too_detect_ = Bitmap::FromFile(path_wchar);

   // Compare bitfield 
   Rect rect{ 0, 0, 680, 500 };
   bmp_too_detect_->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &lockedBitmapData_);

   screenshot_detection_ = true;
   screenshot_found_ = false;
}

bool CDisplay::IsScreenshotFound()
{
   return screenshot_found_;
}

bool CDisplay::CompareScreenshot(const char* pathFile)
{
   // TODO
   HDC hWndDc = GetDC(m_hWnd);
   HBITMAP hTmpBmp = CreateCompatibleBitmap(hWndDc, 680, 500);
   
   // Open file
   wchar_t path_wchar[MAX_PATH];
   MultiByteToWideChar(CP_UTF8, 0, pathFile, strlen(pathFile), path_wchar, MAX_PATH);

   Bitmap * bmp = Bitmap::FromFile(path_wchar);

   // Compare bitfield 
   BitmapData lockedBitmapData, lockedBitmapData2;
   Rect rect{0, 0, 680, 500};
   bmp->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &lockedBitmapData);

   // Compare to 
   HBITMAP hTmpBmp2 = CreateCompatibleBitmap(hWndDc, 680, 500);
   HDC hDC = CreateCompatibleDC(hWndDc);
   HBITMAP oldBmp = (HBITMAP)SelectObject(hDC, hTmpBmp2);


   // Copy MemDC into it
   BitBlt(hDC, 0, 0, 680, 500, m_MemDC, 143, 47, SRCCOPY);

   // Save this bitmap to the file
   Bitmap bmp2(hTmpBmp2, NULL);

   bmp2.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &lockedBitmapData2);

   // COMPARE !
   int ret = memcmp(lockedBitmapData.Scan0, lockedBitmapData2.Scan0, 680 * 500 * 4);
   {
      // Yeah 

   }

   ReleaseDC(m_hWnd, hWndDc);
   return (ret == 0);

}

void CDisplay::TakeScreenshot(const char* pathFile)
{
   screenshot_name_ = pathFile;
   screenshot_take_ = true;
}
void CDisplay::ScreenshotToFile(const char* pathFile)
{
   // Create new file
   // Create BMP of the right size
   HDC hWndDc = GetDC(m_hWnd);
   HDC hWndDc_null = GetDC(NULL);
   HBITMAP hTmpBmp = CreateCompatibleBitmap(hWndDc_null, 680, 500);
   HDC hDC = CreateCompatibleDC(hWndDc_null);
   HBITMAP oldBmp = (HBITMAP)SelectObject(hDC, hTmpBmp);

   // Copy MemDC into it
   BitBlt(hDC, 0, 0, 680, 500, m_MemDC, 143, 47, SRCCOPY);

   // Save this bitmap to the file
   Bitmap bmp(hTmpBmp, NULL);
   
   // Save the altered image.
   CLSID pngClsid;
   GetEncoderClsid(L"image/bmp", &pngClsid);

   wchar_t path_wchar[MAX_PATH]={0};
   MultiByteToWideChar(CP_UTF8, 0, pathFile, strlen(pathFile), path_wchar, MAX_PATH);

   bmp.Save(path_wchar, &pngClsid, NULL);

   SelectObject(hDC, oldBmp);
   DeleteObject(oldBmp);
   DeleteDC(hDC);

   ReleaseDC(m_hWnd, hWndDc);
   ReleaseDC(NULL, hWndDc_null);
}



void CDisplay::HSync ()
{
}

void CDisplay::StartSync()
{

}

void CDisplay::VSync (bool bDbg)
{
   // Add check screenshot (if necessary)
   if (screenshot_take_)
   {
      ScreenshotToFile(screenshot_name_.c_str());
      screenshot_take_ = false;
   }
   if (screenshot_detection_)
   { 
      HDC hWndDc = GetDC(m_hWnd);
      HBITMAP hTmpBmp = CreateCompatibleBitmap(hWndDc, 680, 500);
      BitmapData lockedBitmapData2;
      Rect rect{ 0, 0, 680, 500 };

      // Compare to 
      HBITMAP hTmpBmp2 = CreateCompatibleBitmap(hWndDc, 680, 500);
      HDC hDC = CreateCompatibleDC(hWndDc);
      HBITMAP oldBmp = (HBITMAP)SelectObject(hDC, hTmpBmp2);


      // Copy MemDC into it
      BitBlt(hDC, 0, 0, 680, 500, m_MemDC, 143, 47, SRCCOPY);

      // Save this bitmap to the file
      Bitmap bmp2(hTmpBmp2, NULL);

      bmp2.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &lockedBitmapData2);

      // COMPARE !
      screenshot_found_ = (memcmp(lockedBitmapData_.Scan0, lockedBitmapData2.Scan0, 680 * 500 * 4) == 0);

      if (screenshot_found_) screenshot_detection_ = false;

      bmp2.UnlockBits(&lockedBitmapData2);

      // todo : clear everything !
      SelectObject(hDC, oldBmp);
      DeleteDC(hDC);
      DeleteObject(hTmpBmp2);
      ReleaseDC(m_hWnd, hWndDc);

      /*
      Rect rect{ 0, 0, 680, 500 };
      HDC hWndDc = GetDC(m_hWnd);
      HBITMAP hTmpBmp2 = CreateCompatibleBitmap(hWndDc, 680, 500);
      HDC hDC = CreateCompatibleDC(hWndDc);
      HBITMAP oldBmp = (HBITMAP)SelectObject(hDC, hTmpBmp2);

      // Copy MemDC into it
      BitBlt(hDC, 0, 0, 680, 500, hWndDc, 143, 47, SRCCOPY);
      // Save this bitmap to the file
      Bitmap bmp2(hTmpBmp2, NULL);

      BitmapData lockedBitmapData2;
      bmp2.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &lockedBitmapData2);

      // COMPARE !
      screenshot_found_ = (memcmp(lockedBitmapData_.Scan0, lockedBitmapData2.Scan0, 680 * 500 * 4) == 0);
      bmp2.UnlockBits(&lockedBitmapData2);

      // todo : clear everything !
      SelectObject(hDC, oldBmp);
      DeleteDC(hDC);
      DeleteObject(hTmpBmp2);
      ReleaseDC(m_hWnd, hWndDc);*/
   }
   else
   {
      screenshot_found_ = false;
   }

   if (m_bShow)
   {
      m_hwndDC = GetDC(m_hWnd);
      BitBlt ( m_hwndDC , 0, 0, m_Width, m_Height, m_MemDC, m_X, m_Y, SRCCOPY );
      ReleaseDC (m_hWnd, m_hwndDC);
      
   }
   //Reset();
   if (stop_)
      return;

   MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, TRUE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void CDisplay::WaitVbl ()
{
   //if (m_pDD7 != NULL) m_pDD7->WaitForVerticalBlank  (DDWAITVB_BLOCKBEGIN, NULL);
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
      SetFocus (hWnd);
      break;
   case WM_SYSKEYDOWN:
   case WM_KEYDOWN:
      // Accelerator
      if (pMainWindow!= NULL)
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