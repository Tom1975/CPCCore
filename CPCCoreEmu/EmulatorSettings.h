#pragma once

//////////////////////////////////////////////////////////
// EmulatorSettings.h
//
// This class handle one settings of the emulator.
//
// (c) T.Guillemin 2018
//
//
//////////////////////////////////////////////////////////

#include "ISound.h"
#include "IConfiguration.h"


class JoystickSettings
{
public:
   JoystickSettings();
   virtual ~JoystickSettings();

   // Embedded shortcut class
   class Shortcut
   {
   public:
      Shortcut(unsigned int shortcut);
      // Virtual key code
      unsigned int virtual_key_;
      // shortcut
      unsigned int shortcut_;
   };

   class JoystickShortcut
   {
   public:
      JoystickShortcut();
      unsigned int GetFullAction();
      void KeyDown(unsigned int vkey);
      void KeyUp(unsigned int vkey);

      std::string Serialize();
      void Unserialize(std::string param);

      Shortcut left_scancode_;
      Shortcut right_scancode_;
      Shortcut top_scancode_;
      Shortcut bottom_scancode_;
      Shortcut button_1_scancode_;
      Shortcut button_2_scancode_;
      Shortcut button_3_scancode_;

   protected:
      unsigned int action_;
   };

   JoystickShortcut joystick_1_;
   JoystickShortcut joystick_2_;
};

class EmulatorSettings
{
public:
   EmulatorSettings();
   virtual ~EmulatorSettings();

   EmulatorSettings& operator= (const EmulatorSettings& other);

   // Initialisation
   void Init(IConfiguration * configuration_manager, ISoundFactory* sound_factory);

   ////////////////////////////////////////////////////////////////
   // Load / Save
   virtual void Load(const char* path);
   virtual void Save(const char* path);


   ////////////////////////////////////////////////////////////////
   // Sound description
   ISound* GetSound() { return sound_; };
   void SetSound(ISound* sound) { sound_ = sound; };

   ////////////////////////////////////////////////////////////////
   // Joystick description
   JoystickSettings* GetJoystickSettings();
   void SetJoystickSettings(JoystickSettings* settings);

   ////////////////////////////////////////////////////////////////
   // CPC Power and AMstrad EU acces

   ////////////////////////////////////////////////////////////////
   // ...


protected:
   // Configuration manager : generic access
   IConfiguration * configuration_manager_;

   // Sound
   ISound * sound_;
   ISoundFactory* sound_factory_;

   // Joystick settings
   JoystickSettings joystick_settings_;
};


