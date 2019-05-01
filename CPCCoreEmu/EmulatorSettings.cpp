#include "stdafx.h"
#include "EmulatorSettings.h"
#include "psg.h"
#include <sstream>

//////////////////////////////////////////////////////////
JoystickSettings::JoystickSettings()
{

}

JoystickSettings::~JoystickSettings()
{

}

JoystickSettings::Shortcut::Shortcut(unsigned int shortcut) : shortcut_(shortcut), virtual_key_(0)
{

}

JoystickSettings::JoystickShortcut::JoystickShortcut() :
   left_scancode_(Ay8912::joy_left),
   right_scancode_(Ay8912::joy_right),
   top_scancode_(Ay8912::joy_up),
   bottom_scancode_(Ay8912::joy_down),
   button_1_scancode_(Ay8912::joy_but1),
   button_2_scancode_(Ay8912::joy_but2),
   button_3_scancode_(Ay8912::joy_but3),
   action_(0)
{

}

unsigned int JoystickSettings::JoystickShortcut::GetFullAction()
{
   return action_;
}

void JoystickSettings::JoystickShortcut::KeyDown(unsigned int vkey)
{
   if (left_scancode_.virtual_key_== vkey ) action_ |= left_scancode_.shortcut_;
   if (right_scancode_.virtual_key_== vkey) action_ |= right_scancode_.shortcut_;
   if (top_scancode_.virtual_key_== vkey) action_ |= top_scancode_.shortcut_;
   if (bottom_scancode_.virtual_key_== vkey) action_ |= bottom_scancode_.shortcut_;
   if (button_1_scancode_.virtual_key_== vkey) action_ |= button_1_scancode_.shortcut_;
   if (button_2_scancode_.virtual_key_== vkey) action_ |= button_2_scancode_.shortcut_;
   if (button_3_scancode_.virtual_key_== vkey) action_ |= button_3_scancode_.shortcut_;
}

void JoystickSettings::JoystickShortcut::KeyUp(unsigned int vkey)
{
   if (left_scancode_.virtual_key_ == vkey) action_ &= ~left_scancode_.shortcut_;
   if (right_scancode_.virtual_key_ == vkey) action_ &= ~right_scancode_.shortcut_;
   if (top_scancode_.virtual_key_ == vkey) action_ &= ~top_scancode_.shortcut_;
   if (bottom_scancode_.virtual_key_ == vkey) action_ &= ~bottom_scancode_.shortcut_;
   if (button_1_scancode_.virtual_key_ == vkey) action_ &= ~button_1_scancode_.shortcut_;
   if (button_2_scancode_.virtual_key_ == vkey) action_ &= ~button_2_scancode_.shortcut_;
   if (button_3_scancode_.virtual_key_ == vkey) action_ &= ~button_3_scancode_.shortcut_;

}

std::string JoystickSettings::JoystickShortcut::Serialize()
{
   std::stringstream joy_settings_str;
   joy_settings_str << left_scancode_.virtual_key_ << " ";
   joy_settings_str << right_scancode_.virtual_key_ << " ";
   joy_settings_str << top_scancode_.virtual_key_ << " ";
   joy_settings_str << bottom_scancode_.virtual_key_ << " ";
   joy_settings_str << button_1_scancode_.virtual_key_ << " ";
   joy_settings_str << button_2_scancode_.virtual_key_ << " ";
   joy_settings_str << button_3_scancode_.virtual_key_ << " ";
   return joy_settings_str.str();
}
void JoystickSettings::JoystickShortcut::Unserialize(std::string param)
{
   std::stringstream joy_settings_str;
   joy_settings_str.str( param);

   joy_settings_str >> left_scancode_.virtual_key_;
   joy_settings_str >> right_scancode_.virtual_key_;
   joy_settings_str >> top_scancode_.virtual_key_;
   joy_settings_str >> bottom_scancode_.virtual_key_;
   joy_settings_str >> button_1_scancode_.virtual_key_;
   joy_settings_str >> button_2_scancode_.virtual_key_;
   joy_settings_str >> button_3_scancode_.virtual_key_;


}

//////////////////////////////////////////////////////////
EmulatorSettings::EmulatorSettings() : sound_(nullptr), configuration_manager_(nullptr), sound_factory_(nullptr)
{

}

EmulatorSettings::~EmulatorSettings()
{

}

EmulatorSettings& EmulatorSettings::operator= (const EmulatorSettings& other)
{
   configuration_manager_ = other.configuration_manager_;
   sound_ = other.sound_;
   sound_factory_ = other.sound_factory_;

   joystick_settings_ = other.joystick_settings_;

   return *this;
}

JoystickSettings* EmulatorSettings::GetJoystickSettings()
{
   return &joystick_settings_;
}

void EmulatorSettings::SetJoystickSettings(JoystickSettings* settings)
{
   joystick_settings_ = *settings;
}


void EmulatorSettings::Init(IConfiguration * configuration_manager, ISoundFactory* sound_factory)
{
   configuration_manager_ = configuration_manager;
   sound_factory_ = sound_factory;
   sound_ = sound_factory_->GetSound(NULL);
}


void EmulatorSettings::Load(const char* path)
{
   if (configuration_manager_ == nullptr) return;

   char buf[128];
   configuration_manager_->GetConfiguration("Sound", "SoundType", "", buf, sizeof(buf), path);
   if (sound_factory_ != NULL)
   {
      sound_ = sound_factory_->GetSound(buf);
      if (sound_ != NULL)
         sound_->LoadConfiguration("Sound", path);
   }

   // joystick
   char buffer[128];
   configuration_manager_->GetConfiguration("Joystick", "1", "", buffer, 128, path);
   joystick_settings_.joystick_1_.Unserialize(buffer);

   configuration_manager_->GetConfiguration("Joystick", "2", "", buffer, 128, path);
   joystick_settings_.joystick_2_.Unserialize(buffer);


}

void EmulatorSettings::Save(const char* path)
{
   if (configuration_manager_ == nullptr) return;

   // Sound
   if (sound_factory_ == nullptr || sound_ == nullptr)
   {
      const char * name = sound_factory_->GetSoundName(sound_);
      configuration_manager_->SetConfiguration("Sound", "SoundType", name, path);
      sound_->SaveConfiguration("Sound", path);
   }

   // Joystick settings
   configuration_manager_->SetConfiguration("Joystick", "1", joystick_settings_.joystick_1_.Serialize().c_str(), path);
   configuration_manager_->SetConfiguration("Joystick", "2", joystick_settings_.joystick_2_.Serialize().c_str(), path);
}
