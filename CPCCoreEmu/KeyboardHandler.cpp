#include "stdafx.h"
#include "KeyboardHandler.h"

#include <filesystem>

#include <vector>
#include "IDirectories.h"

extern const char * SugarboxPath;

#ifdef __circle__
   #define KEYBOARD_SCANCODES_FILE "101_keyboard"
#elif  _WIN32
   #define KEYBOARD_SCANCODES_FILE "101_keyboard_win"
#elif __linux__ 
   #define KEYBOARD_SCANCODES_FILE "101_keyboard_linux"
#else
   #define KEYBOARD_SCANCODES_FILE "101_keyboard_linux"
   #pragma error "TODO : Generate a keyboard map for your OS !" 
#endif

unsigned int ExtractLine(const char* buffer, int size, std::string& out)
{
   if (size == 0)
   {
      return 0;
   }

   // looking for /n
   int offset = 0;
   while (buffer[offset] != 0x0A && buffer[offset] != 0x0D && offset < size)
   {
      offset++;
   }

   char* line = new char[offset + 1];
   memcpy(line, buffer, offset);
   line[offset] = '\0';
   out = std::string(line);
   delete[]line;
   return (offset == size) ? offset : offset + 1;
}

/// <summary>
/// Keyboard handler.
/// Main goal is to associate :
/// - a hardware scan code for each line/bit of the matrix keyboard (handle key pressed)
/// - Association between a character and scan code, to allow pasting to be done properly
/// </summary>

KeyboardHandler::KeyboardHandler() : directories_(nullptr), register_replaced_(nullptr)
{
   fs::path exe_path((directories_ != nullptr) ? directories_->GetBaseDirectory() : ".");
   exe_path /= "Keyboards";
   exe_path /= KEYBOARD_SCANCODES_FILE;   
   InitKeyboard (exe_path.string().c_str());
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

void KeyboardHandler::ValidateKeyboardMap()
{
   memcpy(keyboard_lines_, keyboard_lines_cached_, sizeof(keyboard_lines_cached_));
}

unsigned char KeyboardHandler::GetKeyboardMap(int index)
{
   
    return keyboard_lines_[index]; 
}


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

bool KeyboardHandler::LoadScanCodeToMatrix(const char* path, RawToCPC* char_map, unsigned char* dead_key, unsigned char *keyboard_lines, Keymap * keyboard_map, unsigned char* raw_to_functions)
{

   // Open file
   FILE* f;
   f = fopen(path, "r+b");
   if (f == NULL)
   {
      printf("*** ERROR LOADING Keyboard %s\n", path);
      return false;
   }

   // Load every known gamepad to internal structure
   fseek(f, 0, SEEK_END);
   unsigned int buffer_size_ = ftell(f);
   rewind(f);
   unsigned char* buff = new unsigned char[buffer_size_];
   unsigned nBytesRead;

   nBytesRead = fread(buff, 1, buffer_size_, f);
   if (buffer_size_ != nBytesRead)
   {
      // ERROR
      printf("*** ERROR READING Keyboard %s\n", path);
      delete[]buff;
      fclose(f);
      return false;
   }

   memset(char_map, 0, sizeof raw_to_cpc_map_);
   memset(dead_key, 0, sizeof dead_key_);
   memset(dead_key, 0, sizeof raw_to_functions_);
   
   // get next line
   const char* ptr_buffer = (char*)buff;
   unsigned int offset = 0;
   unsigned int end_line;
   std::string s;
   int line_index = 0;
   while ((end_line = ExtractLine(&ptr_buffer[offset], nBytesRead, s)) > 0 && line_index < 11)
   {
      nBytesRead -= end_line;

      // Do not use emty lines, and comment lines
      if (s.size() == 0 || s[0] == '#')
      {
         offset += end_line;
         continue;
      }

      // Decode line to buffer
      // Remove spaces
      while (ptr_buffer[offset] == ' ')
      {
         offset++;
         end_line--;
      }

      // Extract every word in a list
      // words are separated with spaces
      std::vector< std::vector<std::string>> key_list;

      while (ptr_buffer[offset] != '#' && end_line > 0)
      {
         std::vector<std::string> current_word_list;
         std::string new_word;
         while (ptr_buffer[offset] != ' ' && ptr_buffer[offset] != '#' )
         {
            if (ptr_buffer[offset] != ';')
            {
               new_word += ptr_buffer[offset];
            }
            else
            {
               // New word !
               current_word_list.push_back(new_word);
               new_word.clear();
            }
            offset++;
            end_line--;
         }
         if (new_word.size() > 0)
         {
            current_word_list.push_back(new_word);
            key_list.push_back(current_word_list);
         }

         if (ptr_buffer[offset] == '#')
            break;
         offset++;
         end_line--;
      }

      unsigned int raw_key = 0;
      for (auto& it : key_list)
      {
         for (auto& it2 : it)
         {
            unsigned short value = strtoul(it2.c_str(), NULL, 16);
            if ((value & 0x800) == 0x800)
            {
               value &= ~0x800;
               dead_key[value] = 1;
            }

            if (line_index < 10)
            {
               char_map[value].line_index = &keyboard_lines[line_index];
               char_map[value].line_number = line_index;
               char_map[value].bit = 1 << raw_key;

               (*keyboard_map)[line_index][raw_key].scan_code = value;
            }
            else
            {
               // Function key
               raw_to_functions[value] = raw_key+1;   // Function from 1 to...
            }
         }
         raw_key++;
      }
      offset += end_line;
      line_index++;
   }

   delete[]buff;
   fclose(f);
   return true;
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
   
   // Load scan code association
   // This one depends on the keyboard type : It should be the same for AZERTY,QWERTY and so one.
   // Maybe it can differ depending on the keyboad ? (Pi can be different from PC for example)
   // TODO : Handle these keyboard in a smart way
   fs::path exe_path((directories_ != nullptr) ? directories_->GetBaseDirectory() : ".");
   exe_path /= "Keyboards";
   exe_path /= KEYBOARD_SCANCODES_FILE;
   LoadScanCodeToMatrix(exe_path.string().c_str(), raw_to_cpc_map_, dead_key_, keyboard_lines_cached_, &keyboard_map_, raw_to_functions_);
}

void KeyboardHandler::InitKeyboard (const char* path)
{
   memset ( keyboard_lines_, 0xff, sizeof (keyboard_lines_));
   memset ( keyboard_lines_cached_, 0xff, sizeof (keyboard_lines_));

   LoadScanCodeToMatrix(path, raw_to_cpc_map_, dead_key_, keyboard_lines_cached_, &keyboard_map_, raw_to_functions_);

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

bool KeyboardHandler::IsDeadKey(unsigned int car)
{
   return (dead_key_[car] == 1);
}


void KeyboardHandler::SendScanCode ( unsigned int scancode, bool bPressed )
{
   if (raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE-1)].bit != 0)
   {
      if (bPressed)
      {
         if ((keyboard_lines_[raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE - 1)].line_number] & (raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE - 1)].bit)) != 0 && (register_replaced_ != nullptr)) *register_replaced_ = true;
         keyboard_lines_[raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE - 1)].line_number] &= ~(raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE - 1)].bit);
      }
      else
      {
         if ((keyboard_lines_[raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE-1)].line_number] & (raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE-1)].bit)) != 1 && (register_replaced_ != nullptr)) *register_replaced_ = true;
         keyboard_lines_[raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE-1)].line_number] |= (raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE-1)].bit);
      }
      //logger_->Write("KeyboardPi", LogNotice, "UnpressKey %X - line : %i, bit : %X", scancode, raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE-1)].line_number, raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE-1)].bit);
      //*raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE-1)].line_index &= ~(raw_to_cpc_map_[scancode & (SCANCODE_MAP_SIZE-1)].bit);
   }
   return;

   // Look for scan code in the base
   for (auto line = 0; line < 10; line++)
   {
      for (auto b=7; b >=0; b--)
      {
         // Get values from conf
         if ( (keyboard_map_[line][b].scan_code == scancode)
            ||(keyboard_map_[line][b].scan_code_alt == scancode)
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
