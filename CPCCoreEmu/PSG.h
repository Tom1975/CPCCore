#pragma once

//#include "MMsystem.h"
#include "IComponent.h"
#include "IConfiguration.h"
#include "SoundMixer.h"
#include "Tape.h"
#include "ISound.h"

class IKeyboard
{
public:
   virtual void SendScanCode ( unsigned short, bool pressed) = 0;
   virtual void JoystickAction (unsigned int joy, unsigned int action) = 0;
};


/*#ifdef CPCCOREEMU_EXPORTS
#define CPCCOREEMU_API __declspec(dllexport)
#else
#define CPCCOREEMU_API __declspec(dllimport)
#endif
*/
#define CPCCOREEMU_API 

class CPCCOREEMU_API Ay8912 : public IKeyboard, public IComponent
{
friend class EmulatorEngine;
friend class CSnapshot;
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
   


   Ay8912(SoundMixer *sound_hub);
   virtual ~Ay8912(void);

   virtual void SetDirectories(IDirectories * dir) {directories_ = dir;   };
   virtual void SetConfigurationManager(IConfiguration * configuration_manager) { configuration_manager_ = configuration_manager; }
   void Reset ();
   // BusCTRL : BC2, BDIR, BC1
   void Access( unsigned char *data_bus , unsigned char bus_ctrl, unsigned char data);

   // Actions
   virtual const char* GetKeyboardConfig ();

   virtual void LoadKeyboardMap (const char * config);
   virtual void SaveKeyboardMap (const char * config);
   Key GetKeyValues ( const char* config, unsigned int line, unsigned int bit );

   Key GetKey (unsigned int line, unsigned int bit){return keyboard_map_[line][bit];};
   void SetKey (unsigned int line, unsigned int bit, Key key){ keyboard_map_[line][bit] = key;};

   void SetKeyValues ( const char* config, Ay8912::Key key , unsigned int line, unsigned int bit);
   unsigned int GetScanCode ( unsigned int line, unsigned int bit){return keyboard_map_[line][bit].scan_code ;}
   void SetScanCode ( unsigned int line, unsigned int bit, unsigned short scanCode){keyboard_map_[line][bit].scan_code = scanCode ;}

   unsigned int GetScanCodeAlternate ( unsigned int line, unsigned int bit){return keyboard_map_[line][bit].scan_code_alt ;}
   void SetScanCodeAlternate ( unsigned int line, unsigned int bit, unsigned short scanCode){keyboard_map_[line][bit].scan_code_alt = scanCode ;}

   virtual void CharPressed (char c) ;
   virtual void CharReleased (char c) ;

   virtual void SendScanCode ( unsigned short, bool pressed );

   // joystick
   static const unsigned int joy_up     = 0x0001;
   static const unsigned int joy_down   = 0x0002;
   static const unsigned int joy_left   = 0x0004;
   static const unsigned int joy_right  = 0x0008;
   static const unsigned int joy_but1   = 0x0010;
   static const unsigned int joy_but2   = 0x0020;
   static const unsigned int joy_but3   = 0x0040;

   virtual void JoystickAction (unsigned int joy, unsigned int action);

   void InitSound (ISound* sound);
   unsigned int Tick ( );

   void ForceKeyboardState ( unsigned char key_states[10]){ memcpy (keyboard_lines_, key_states, 10 );};
   unsigned char* GetKeyboardState () {return keyboard_lines_;};
   unsigned char GetRegisterAdress() {
      return register_address_
         ;
   }
protected :

   void TickSound ();

   // Registers - 12 bits for theses
   unsigned char register_address_;

   unsigned int channel_a_freq_;
   unsigned int channel_a_freq_counter_;

   unsigned int channel_b_freq_;
   unsigned int channel_b_freq_counter_;

   unsigned int channel_c_freq_;
   unsigned int channel_c_freq_counter_;

   unsigned int noise_frequency_;
   unsigned char mixer_control_register_;

   unsigned char channel_a_volume_;
   unsigned char channel_b_volume_;
   unsigned char channel_c_volume_;

   unsigned char enveloppe_volume_;

   unsigned int volume_enveloppe_frequency_;
   unsigned char volume_enveloppe_shape_;

   unsigned char external_data_register_b_;

   // Envelope
   bool enveloppe_up_;
   bool enveloppe_stop_;

   /////////////////////////////////
   // Keyboard
   Key keyboard_map_[10][8];
   bool register_replaced_;
   unsigned char register_14_;

   void InitKeyboard ();

   // Keyboard definition
   unsigned char keyboard_lines_ [10];

   void CharAction(char c, bool pressed) ;

   char keyboard_config_ [32];


   /////////////////////////////////
   // Precise sound output
   unsigned int counter_a_;
   unsigned int counter_b_;
   unsigned int counter_c_;
   unsigned int counter_noise_;
   unsigned int counter_env_;
   unsigned int counter_state_env_;

   unsigned char chan_a_high_;
   unsigned char chan_b_high_;
   unsigned char chan_c_high_;
   unsigned char chan_noise_high_;

   unsigned int noise_shift_register_;

   ISound* sound_;

   unsigned char register_[16];

   SoundSource sound_source_;
   SoundMixer *sound_hub_;

   IConfiguration * configuration_manager_;
   IDirectories * directories_;
};

