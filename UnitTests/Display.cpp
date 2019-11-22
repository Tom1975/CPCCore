
#include "Display.h"

#include <SFML/Window.hpp>

#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   800
#define DISP_WINDOW_Y   600


CDisplay::CDisplay ()
{
   window_ = new sf::RenderWindow (sf::VideoMode(800, 600), "My window");
   framebuffer_ = new sf::Texture ();
   renderTexture_ = new sf::RenderTexture ();
}

CDisplay::~CDisplay()
{
   delete renderTexture_;
   delete framebuffer_;
   delete window_;

}

int CDisplay::GetWidth ()
{
   return m_Width;
}

int CDisplay::GetHeight ()
{
   return m_Height; //REAL_DISP_Y;
}

int* CDisplay::GetVideoBuffer (int y )
{
   return &framebufferArray_[y * REAL_DISP_X];
}

void CDisplay::Reset () 
{
}

void CDisplay::Show ( bool bShow )
{
   m_bShow = bShow;
}

void CDisplay::Init ()
{
   framebufferArray_ = new int[REAL_DISP_X * REAL_DISP_Y ];
   if (!framebuffer_->create(REAL_DISP_X, REAL_DISP_Y))
   {
      // error...
   }

   Reset();

}

void CDisplay::InitScreenshotDetection(const char* pathFile)
{
   screenshot_detection_ = true;
   screenshot_found_ = false;
}

bool CDisplay::IsScreenshotFound()
{
   return screenshot_found_;
}

bool CDisplay::CompareScreenshot(const char* pathFile)
{
   return false;

}

void CDisplay::TakeScreenshot(const char* pathFile)
{
   screenshot_name_ = pathFile;
   screenshot_take_ = true;
}
void CDisplay::ScreenshotToFile(const char* pathFile)
{
   
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
   }
   else
   {
      screenshot_found_ = false;
   }


   //if (m_bShow)
   {
      framebuffer_->update((const sf::Uint8*)framebufferArray_);
      sf::Sprite sprite;
      sprite.setTexture(*framebuffer_);

      window_->draw(sprite);
      window_->display();
   }
      sf::Event event;
   while (window_->pollEvent(event))
   {
      // process event...
   }
   //Reset();
   if (stop_)
      return;

}

void CDisplay::WaitVbl ()
{
   //if (m_pDD7 != NULL) m_pDD7->WaitForVerticalBlank  (DDWAITVB_BLOCKBEGIN, NULL);
}

