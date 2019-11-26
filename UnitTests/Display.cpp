
#include "Display.h"

#include <SFML/Window.hpp>

#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   800
#define DISP_WINDOW_Y   600


CDisplay::CDisplay () : screenshot_detection_(false), window_(nullptr)
{
}

CDisplay::~CDisplay()
{
   delete renderTexture_;
   delete framebuffer_;
   delete window_;

}

unsigned int CDisplay::ConvertRGB(unsigned int rgb)
{
   return  (0xFF000000 | ((rgb & 0xFF)<<16) | ((rgb & 0xFF00) ) | ((rgb & 0xFF0000)>>16));
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
   return &framebufferArray_[ REAL_DISP_X * y*2];
}

void CDisplay::Reset () 
{
   memset( framebufferArray_ , 0, REAL_DISP_X * REAL_DISP_Y*4);
}

void CDisplay::Show ( bool bShow )
{
   m_bShow = bShow;
   if (window_)
      window_->setVisible(bShow);
}

void CDisplay::Init (bool show)
{
   if (show)
      window_ = new sf::RenderWindow(sf::VideoMode(680, 500), "My window");
   framebuffer_ = new sf::Texture();
   renderTexture_ = new sf::RenderTexture();
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

   screenshot_texture_.loadFromFile(pathFile);
   screenshot_buffer_ = screenshot_texture_.getPixelsPtr();
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
      // Extract (143, 47, 680, 500) from current image
      framebuffer_->update((const sf::Uint8*)framebufferArray_);
      sf::Image img = framebuffer_->copyToImage();
      sf::Image img_src;
      const unsigned char* src_buffer = img.getPixelsPtr();

      // Compare 
      bool ok = true;
      int* int_src_buff = (int*)src_buffer;
      for (int i = 0; i < 500 && ok; i++)
      {
         for (int j = 0; j < 680 && ok; j++)
         {
            if ( framebufferArray_[143 + j + (i+47) * REAL_DISP_X ] != int_src_buff[143 + j + (i + 47) * REAL_DISP_X/*j* REAL_DISP_X + i*/] )
               ok = false;
         }
      }
      //if (memcmp(screenshot_buffer_, src_buffer, 680 * 500 * 4) == 0)
      if (ok)
      {
         screenshot_found_ = true;
         screenshot_detection_ = false;
      }
      else
      {
         screenshot_found_ = false;
      }

   }
   else
   {
      screenshot_found_ = false;
      framebuffer_->update((const sf::Uint8*)framebufferArray_);
   }

   if (window_)
   {
      if (m_bShow)
      {

         sf::Sprite sprite;
         sprite.setTexture(*framebuffer_);
         sprite.setTextureRect(sf::IntRect(143, 47, 680, 500));

         window_->draw(sprite);
         window_->display();
      }
      sf::Event event;
      while (window_->pollEvent(event))
      {
         // process event...
      }
   }
   Reset();
   if (stop_)
      return;

}

void CDisplay::WaitVbl ()
{
   //if (m_pDD7 != NULL) m_pDD7->WaitForVerticalBlank  (DDWAITVB_BLOCKBEGIN, NULL);
}

