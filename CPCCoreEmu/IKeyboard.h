#pragma once

class IKeyboardHandler
{
public:
   // joystick
   static const unsigned int joy_up = 0x0001;
   static const unsigned int joy_down = 0x0002;
   static const unsigned int joy_left = 0x0004;
   static const unsigned int joy_right = 0x0008;
   static const unsigned int joy_but1 = 0x0010;
   static const unsigned int joy_but2 = 0x0020;
   static const unsigned int joy_but3 = 0x0040;

   virtual unsigned char GetKeyboardMap(int index) = 0;
   virtual void Init(bool* register_replaced) = 0;
   virtual void ForceKeyboardState(unsigned char key_states[10]) = 0;
}; 

class IKeyboard 
{
public:
   virtual void SendScanCode ( unsigned int, bool pressed) = 0;
   virtual unsigned int GetScanCode(unsigned int line , unsigned int bit) = 0;
   virtual void JoystickAction (unsigned int joy, unsigned int action) = 0;
};
