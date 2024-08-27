#include "stdafx.h"
#include "KeyboardHandler.h"

#include "simple_filesystem.h"
#include "IDirectories.h"

extern const char * SugarboxPath;


KeyboardHandler::KeyboardHandler() : directories_(nullptr), register_replaced_(nullptr)
{
   InitKeyboard ();
   memset ( keyboard_config_, 0, sizeof keyboard_config_);
}


KeyboardHandler::~KeyboardHandler(void)
{
}

void KeyboardHandler::Init(bool* register_replaced)
{
   register_replaced_ = register_replaced;
}

#define KEY_BUFFER_SIZE 32

KeyboardHandler::Key KeyboardHandler::GetKeyValues ( const char* config, unsigned int line, unsigned int bit )
{
   Key key;
   memset (&key, 0, sizeof  (Key ));

   // Get value from config file
   fs::path exe_path((directories_ != nullptr) ? directories_->GetBaseDirectory() : ".");
   exe_path /= "CONF";
   exe_path /= "KeyboardMaps.ini";
   std::string filepath = exe_path.string();

   char key_buffer[KEY_BUFFER_SIZE];
   char char_buffer[2];
   sprintf ( key_buffer, "%i_%i_SC", line, bit);
   key.scan_code = configuration_manager_->GetConfigurationInt  ( config, key_buffer, 0, filepath.c_str() );

   sprintf ( key_buffer, "%i_%i_SCA", line, bit);
   key.scan_code_alt = configuration_manager_->GetConfigurationInt( config, key_buffer, 0, filepath.c_str() );

   sprintf ( key_buffer, "%i_%i_char", line, bit);
   configuration_manager_->GetConfiguration( config, key_buffer, "", char_buffer, 2, filepath.c_str() );
   if ( char_buffer[0] == 0)
   {
      // Exception : Read the alternative key
      sprintf ( key_buffer, "%i_%i_char_value", line, bit);
      char_buffer[0] = configuration_manager_->GetConfigurationInt ( config, key_buffer, -1, filepath.c_str() );

   }
   key.c = char_buffer[0];

   sprintf ( key_buffer, "%i_%i_char_Alt", line, bit);
   configuration_manager_->GetConfiguration  ( config, key_buffer, "", char_buffer, 2, filepath.c_str() );
   if ( char_buffer[0] == 0)
   {
      // Exception : Read the alternative key
      sprintf ( key_buffer, "%i_%i_char_value_Alt", line, bit);
      char_buffer[0] = configuration_manager_->GetConfigurationInt ( config, key_buffer, -1, filepath.c_str() );

   }
   key.c_alt = char_buffer[0];

   sprintf ( key_buffer, "%i_%i_UCHAR", line, bit);
   configuration_manager_->GetConfiguration  ( config, key_buffer, "", char_buffer, 2, filepath.c_str() );
   if ( char_buffer[0] == 0)
   {
      // Exception : Read the alternative key
      sprintf ( key_buffer, "%i_%i_UCHAR_value", line, bit);
      char_buffer[0] = (char)configuration_manager_->GetConfigurationInt ( config, key_buffer, 0, filepath.c_str() );

   }
   key.c_upper = char_buffer[0];

   sprintf ( key_buffer, "%i_%i_UCHAR_Alt", line, bit);
   configuration_manager_->GetConfiguration  ( config, key_buffer, "", char_buffer, 2, filepath.c_str() );
   if ( char_buffer[0] == 0)
   {
      // Exception : Read the alternative key
      sprintf ( key_buffer, "%i_%i_UCHAR_value_Alt", line, bit);
      char_buffer[0] = (char)configuration_manager_->GetConfigurationInt ( config, key_buffer, 0, filepath.c_str() );

   }
   key.c_upper_alt = char_buffer[0];

   sprintf ( key_buffer, "%i_%i_charCtrl", line, bit);
   configuration_manager_->GetConfiguration  ( config, key_buffer, "", char_buffer, 2, filepath.c_str() );
   if ( char_buffer[0] == 0)
   {
      // Exception : Read the alternative key
      sprintf ( key_buffer, "%i_%i_charCtrl_value", line, bit);
      char_buffer[0] = (char)configuration_manager_->GetConfigurationInt ( config, key_buffer, 0, filepath.c_str() );

   }
   key.ctrl_c = char_buffer[0];

   sprintf ( key_buffer, "%i_%i_UCHARCTRL", line, bit);
   configuration_manager_->GetConfiguration  ( config, key_buffer, "", char_buffer, 2, filepath.c_str() );
   if ( char_buffer[0] == 0)
   {
      // Exception : Read the alternative key
      sprintf ( key_buffer, "%i_%i_UCHARCTRL_value", line, bit);
      char_buffer[0] = (char)configuration_manager_->GetConfigurationInt ( config, key_buffer, 0, filepath.c_str() );

   }
   key.ctrl_upper_c = char_buffer[0];

   //

   return key;
}

const char* KeyboardHandler::GetKeyboardConfig ()
{
   return keyboard_config_;
}

void KeyboardHandler::LoadKeyboardMap (const char * config)
{
   strcpy ( keyboard_config_, config );
   for (int line = 0; line < 10; line++)
   {
      for (int b=7; b >=0; b--)
      {
         memset (&keyboard_map_[line][b], 0, sizeof (Key));

         // Get values from conf
         keyboard_map_[line][b] = GetKeyValues ( config, line, b );
      }
   }
}

void KeyboardHandler::InitKeyboard ()
{
   memset ( keyboard_lines_, 0xff, sizeof (keyboard_lines_));
}

void KeyboardHandler::CharPressed (char c)
{
   CharAction(c, true);
}

void KeyboardHandler::CharReleased(char c)
{
   CharAction(c, false);
}

void KeyboardHandler::CharAction (char c, bool bPressed)
{
   // Lookup table
   for (int l = 0; l < 10; l++)
   {
      for (int b = 0; b < 8; b++)
      {
         if ( keyboard_map_[l][b].c == c
            ||keyboard_map_[l][b].c_alt == c)
         {
            if (bPressed) keyboard_lines_[l] &= ~(1<<b);
               else keyboard_lines_[l] |= (1<<b);
            keyboard_lines_[2] |= (1<<5);
            if (register_replaced_ != nullptr) *register_replaced_ = true;
            return;
         }
         else if (  keyboard_map_[l][b].c_upper == c
                  ||keyboard_map_[l][b].c_upper_alt == c)
         {
            if (bPressed) keyboard_lines_[l] &= ~(1<<b);
               else keyboard_lines_[l] |= (1<<b);
            // Shift
            if (bPressed) keyboard_lines_[2] &= ~(1<<5);
               else keyboard_lines_[2] |= (1<<5);

            if (register_replaced_ != nullptr)
               *register_replaced_ = true;
            return;
         }
      }
   }
}

#define SET_KEY_LINE_BIT(val, line, bit)\
   if ( (action & val) == val )\
   {\
      if ((keyboard_lines_[line]&(1<<bit))!=0 && (register_replaced_ != nullptr)) *register_replaced_ = true;\
      keyboard_lines_[line] &= ~(1<<bit);\
   }\
   else \
   { if ((keyboard_lines_[line]&(1<<bit))==0 && (register_replaced_ != nullptr))\
      *register_replaced_ = true;\
   keyboard_lines_[line] |= (1<<bit);\
   }\


void KeyboardHandler::JoystickAction (unsigned int joy, unsigned int action)
{
   if ( joy == 0)
   {
      SET_KEY_LINE_BIT(joy_up, 9, 0);
      SET_KEY_LINE_BIT(joy_down, 9, 1);
      SET_KEY_LINE_BIT(joy_left, 9, 2);
      SET_KEY_LINE_BIT(joy_right, 9, 3);
      SET_KEY_LINE_BIT(joy_but1, 9, 6);
      SET_KEY_LINE_BIT(joy_but2, 9, 5);
      SET_KEY_LINE_BIT(joy_but3, 9, 4);
   }
   // TODO : JOY2 just mess up the keyboard...
   /*else
   {
      SET_KEY_LINE_BIT(joy_up, 6, 0);
      SET_KEY_LINE_BIT(joy_down, 6, 1);
      SET_KEY_LINE_BIT(joy_left, 6, 2);
      SET_KEY_LINE_BIT(joy_right, 6, 3);
      SET_KEY_LINE_BIT(joy_but1, 6, 6);
      SET_KEY_LINE_BIT(joy_but2, 6, 5);
      SET_KEY_LINE_BIT(joy_but3, 6, 4);
   }*/
}

void KeyboardHandler::SendScanCode ( unsigned int scanCode, bool bPressed )
{
   // Look for scan code in the base
   for (auto line = 0; line < 10; line++)
   {
      for (auto b=7; b >=0; b--)
      {
         // Get values from conf
         if ( (keyboard_map_[line][b].scan_code == scanCode)
            ||(keyboard_map_[line][b].scan_code_alt == scanCode)
            )
         {
            //
            if (bPressed)
            {
               if ((keyboard_lines_[line] & (1 << b)) != 0 && (register_replaced_ != nullptr)) *register_replaced_ = true;
               keyboard_lines_[line] &= ~(1<<b);
            }
            else
            {
               if ((keyboard_lines_[line] & (1 << b)) != 1 && (register_replaced_ != nullptr)) *register_replaced_ = true;
               keyboard_lines_[line] |= (1<<b);
            }
         }
      }
   }
}
