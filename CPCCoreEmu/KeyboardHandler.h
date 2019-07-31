#pragma once

#include "IConfiguration.h"
#include "IDirectories.h"
#include "IKeyboard.h"

class KeyboardHandler : public IKeyboard, public IKeyboardHandler
{
public:

   // Types
   typedef struct {
      unsigned int scan_code;
      unsigned int scan_code_alt;
      char c;
      char c_alt;
      char c_upper;
      char c_upper_alt;
      char ctrl_c;
      char ctrl_upper_c;
   } Key;
   
   typedef Key Keymap[10][8];

   KeyboardHandler();
   virtual ~KeyboardHandler(void);

   virtual void Init(bool* register_replaced);
   virtual void SetDirectories(IDirectories * dir) {directories_ = dir;   };
   virtual void SetConfigurationManager(IConfiguration * configuration_manager) { configuration_manager_ = configuration_manager; }

   // Actions
   virtual const char* GetKeyboardConfig ();
   KeyboardHandler::Keymap& GetKeyboardMap() { return keyboard_map_; }

   unsigned char GetKeyboardMap(int index) { return keyboard_lines_[index]; }

   virtual void LoadKeyboardMap (const char * config);
   Key GetKeyValues ( const char* config, unsigned int line, unsigned int bit );

   Key GetKey (unsigned int line, unsigned int bit){return keyboard_map_[line][bit];};
   void SetKey (unsigned int line, unsigned int bit, Key key){ keyboard_map_[line][bit] = key;};

   unsigned int GetScanCode ( unsigned int line, unsigned int bit){return keyboard_map_[line][bit].scan_code ;}
   void SetScanCode ( unsigned int line, unsigned int bit, unsigned short scanCode){keyboard_map_[line][bit].scan_code = scanCode ;}

   unsigned int GetScanCodeAlternate ( unsigned int line, unsigned int bit){return keyboard_map_[line][bit].scan_code_alt ;}
   void SetScanCodeAlternate ( unsigned int line, unsigned int bit, unsigned short scanCode){keyboard_map_[line][bit].scan_code_alt = scanCode ;}

   virtual void CharPressed (char c) ;
   virtual void CharReleased (char c) ;

   virtual void SendScanCode ( unsigned short, bool pressed );



   virtual void JoystickAction (unsigned int joy, unsigned int action);

   void ForceKeyboardState ( unsigned char key_states[10]){ memcpy (keyboard_lines_, key_states, 10 );};
   unsigned char* GetKeyboardState () {return keyboard_lines_;};

protected :

   /////////////////////////////////
   // Keyboard   
   Keymap keyboard_map_;

   void InitKeyboard ();
   bool* register_replaced_;
   // Keyboard definition
   unsigned char keyboard_lines_ [10];

   void CharAction(char c, bool pressed) ;

   char keyboard_config_ [32];


   IConfiguration * configuration_manager_;
   IDirectories * directories_;
};

