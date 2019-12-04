#pragma once 

#include <iostream>
#include <functional>

#include "Machine.h"
#include "Display.h"

// SCR_COMPARE  = false : Generate screenshot
// SCR_COMPARE  = true: Compare screenshot
#define NO_INIT_SCREENSHOT

#define  SCR_COMPARE true

#define USE_COUNT LARGE_INTEGER li;static double PCFreq=0;QueryPerformanceFrequency(&li);PCFreq = double(li.QuadPart)/1000.0;TCHAR buf[256];
#define START_COUNT(s) {LARGE_INTEGER start; QueryPerformanceCounter(&start);OutputDebugString (s);
#define END_COUNT LARGE_INTEGER end;QueryPerformanceCounter(&end); double val = (end.QuadPart-start.QuadPart)/PCFreq;sprintf_s(buf, 255,"Val= %f\n",val);OutputDebugString (buf);}


/////////////////////////////////////////////////////////////
/// Helper functions

bool CompareDisks(IDisk *d1, IDisk* d2, std::string p1);
bool CompareTape(std::string p1);

bool InitBinary(char* conf, char* initfile, char* binary_to_load, unsigned short addr_ok, unsigned short addr_ko, unsigned int tolerated_error = 0);
int LoadCprFromBuffer(Motherboard* motherboard, unsigned char* buffer, int size);

/////////////////////////////////////////////////////////////
/// Helper functions

class ConfigurationManager : public IConfiguration
{
public:

   ConfigurationManager();
   virtual ~ConfigurationManager();

   virtual void OpenFile(const char* config_file);
   virtual void SetConfiguration(const char* section, const char* cle, const char* valeur, const char* file);
   virtual size_t GetConfiguration(const char* section, const char* cle, const char* default_value, char* out_buffer, size_t buffer_size, const char* file);
   virtual unsigned int GetConfigurationInt(const char* section, const char* cle, unsigned int default_value, const char* file);

protected:
   void Clear();

   struct data : std::map <std::string, std::string>
   {
      // Here is a little convenience method...
      bool iskey(const std::string& s) const
      {
         return count(s) != 0;
      }
   };

   typedef std::map <std::string, data* > ConfigFile;

   ConfigFile config_file_;
   std::string current_config_file_;
};

class FileLog : public ILog
{
public:
   FileLog(const char* file)
   {
      fopen_s(&f_, file, "w");
   }
   virtual ~FileLog()
   {
      fclose(f_);
   }
   virtual void WriteLog(const char* pLog) { fwrite (pLog, strlen(pLog), 1, f_); };

   virtual void WriteLogByte(unsigned char pNumber) 
   {
      char buf[256]; sprintf_s(buf, 256, " %2.2X ", pNumber); fwrite(buf, strlen(buf) , 1, f_);
   }
   virtual void WriteLogShort(unsigned short pNumber) { char buf[256]; sprintf_s(buf, 256, " %4.4X ", pNumber); fwrite(buf, strlen(buf) , 1, f_);
   }
   virtual void WriteLog(unsigned int pNumber) { char buf[256]; sprintf_s(buf, 256, " %8.8X ", pNumber); fwrite(buf, strlen(buf) , 1, f_);
   }
   virtual void EndOfLine() { fwrite("\n", 1 , 1, f_);}
protected:
   FILE * f_;
};

class Log : public ILog
{
public:

   virtual void WriteLog(const char* pLog) {  };
   virtual void WriteLogByte(unsigned char pNumber) { char buf[256]; sprintf_s(buf, 256, " %2.2X ", pNumber); }
   virtual void WriteLogShort(unsigned short pNumber) { char buf[256]; sprintf_s(buf, 256, " %4.4X ", pNumber); }
   virtual void WriteLog(unsigned int pNumber) { char buf[256]; sprintf_s(buf, 256, " %8.8X ", pNumber); };
   virtual void EndOfLine() { };

};
class SoundFactory : public ISoundFactory
{
   virtual ISound* GetSound(const char* pName) { return nullptr; };
   virtual const char* GetSoundName(ISound*) { return "Mock sound"; };

   virtual const char* GetFirstSoundName() { return nullptr; };
   virtual const char* GetNextSoundName() { return nullptr; };
};

class DirectoriesImp : public IDirectories
{
public:
   virtual const char* GetBaseDirectory()
   {
      return ".";
   }
};


class ICommand
{
public:
   virtual bool Action(EmulatorEngine* machine) = 0;
};

class CommandAddBreakpoint : public ICommand
{
public:

   CommandAddBreakpoint(unsigned short bp) : bp_(bp)
   {
   }
   virtual bool Action(EmulatorEngine* machine) { machine->AddBreakpoint(bp_); return true; };
protected:
   unsigned short bp_;
};

class CommandEjectDisk : public ICommand
{
public:
   virtual bool Action(EmulatorEngine* machine) { machine->Eject(); return true; };

protected:

};
class CommandInsertDisk : public ICommand
{
public:
   CommandInsertDisk(const char* pathfile) :pathfile_(pathfile) {};
   virtual bool Action(EmulatorEngine* machine) { return machine->LoadDisk(pathfile_, 0, false) == 0; };

protected:
   const char* pathfile_;
};

class CommandRunCycles : public ICommand
{
public:

   CommandRunCycles(int nb_cycles) :nb_cycles_(nb_cycles)
   {
   }

   virtual bool Action(EmulatorEngine* machine)
   {
      for (int i = 0; i < nb_cycles_; i++)
      {
         machine->RunTimeSlice(true);
      }
      // If no condition, always true
      return true;
   }
protected:
   int nb_cycles_;
};


class CommandRunForBreakpoint : public ICommand
{
public:

   CommandRunForBreakpoint(unsigned short bp, int nb_cycles) :nb_cycles_(nb_cycles), bp_(bp)
   {
   }

   virtual bool Action(EmulatorEngine* machine)
   {
      machine->AddBreakpoint(bp_);
      for (int i = 0; i < nb_cycles_; i++)
      {
         machine->RunTimeSlice(false);

         // If Breakpoint, end loop
         if (machine->GetProc()->GetPC() == bp_)
            return true;
      }
      // No success condition ? error !
      return false;
   }
protected:
   int nb_cycles_;
   unsigned short bp_;
   std::function<bool(EmulatorEngine* machine)> lambda_;

};


class CommandRunToScreenshot : public ICommand
{
public:
   CommandRunToScreenshot(CDisplay * display, std::string filename, int nb_cycles_timeout) : display_(display), filename_(filename), nb_cycles_timeout_(nb_cycles_timeout)
   {

   }
   virtual bool Action(EmulatorEngine* machine)
   {
      display_->InitScreenshotDetection(filename_.c_str());
      for (int i = 0; i < nb_cycles_timeout_; i++)
      {
         machine->RunTimeSlice(true);

         if (display_->IsScreenshotFound())
            return true;
      }


      return false;
   }
protected:
   CDisplay * display_;
   std::string filename_;
   int nb_cycles_timeout_;

};

class CommandSaveScreenshot : public ICommand
{
public:
   CommandSaveScreenshot(CDisplay * display, std::string filename, bool verify) : display_(display), filename_(filename), verify_(verify)
   {

   }
   virtual bool Action(EmulatorEngine* machine)
   {
      // Get current screen
      // Verify or save ?
      if (verify_)
      {
         //return display_->CompareScreenshot(filename_.c_str());
         display_->InitScreenshotDetection(filename_.c_str());
         for (int i = 0; i < 100; i++)
         {
            machine->RunTimeSlice(true);

            if (display_->IsScreenshotFound())
               return true;
         }
         return false;
      }
      else
      {
         display_->TakeScreenshot(filename_.c_str());
         while (display_->IsScreenshotTaken() == true)
         {
            machine->RunTimeSlice(false);
         }
         //display_->ScreenshotToFile(filename_.c_str());
      }
      return true;
   }
protected:
   CDisplay * display_;
   std::string filename_;
   bool verify_;

};

class CommandTrace : public ICommand
{
public:

   CommandTrace(std::function<bool(EmulatorEngine* machine)> lambda) : lambda_(lambda)
   {
   }

   virtual bool Action(EmulatorEngine* machine)
   {
      lambda_(machine);
      return true;
   }
protected:
   std::function<bool(EmulatorEngine* machine)> lambda_;
};

class CommandRunCyclesCondition : public ICommand
{
public:

   CommandRunCyclesCondition(int nb_cycles, std::function<bool(EmulatorEngine* machine)> lambda) :nb_cycles_(nb_cycles), lambda_(lambda)
   {
   }

   virtual bool Action(EmulatorEngine* machine)
   {
      for (int i = 0; i < nb_cycles_; i++)
      {
         machine->RunTimeSlice(false);

         // If Breakpoint, end loop
         if (lambda_(machine))
            return true;
      }
      // No success condition ? error !
      return false;
   }
protected:
   int nb_cycles_;
   std::function<bool(EmulatorEngine* machine)> lambda_;

};

class CommandKeyboard : public ICommand
{
public:
   CommandKeyboard(char* command) :command_(command)
   {
   }

   virtual bool Action(EmulatorEngine* machine)
   {
      machine->Paste(command_.c_str());
      return true;
   }
protected:
   std::string command_;
};

class CommandScanCode : public ICommand
{
public:
   CommandScanCode(IKeyboard* pKeyHandler, unsigned short scancode, unsigned int pressed) : pKeyHandler_(pKeyHandler), scancode_(scancode), pressed_(pressed)
   {
   }

   virtual bool Action(EmulatorEngine* machine)
   {
      pKeyHandler_->SendScanCode(scancode_, (pressed_ == 1));
      return true;
   }
protected:
   IKeyboard* pKeyHandler_;
   unsigned short scancode_;
   unsigned int pressed_;

};

class CommandJoystick : public ICommand
{
public:
   CommandJoystick(int joy, int action) : action_(action), joy_(joy)
   {
   }

   virtual bool Action(EmulatorEngine* machine)
   {
      machine->GetKeyboardHandler()->JoystickAction(joy_, action_);
      return true;
   }
protected:
   int action_;
   int joy_;
};

class CommandList
{
public:
   CommandList() {};
   virtual ~CommandList()
   {
      for (std::vector<ICommand*>::iterator it = command_list_.begin(); it != command_list_.end(); it++)
      {
         delete *it;
      }
   };

   void AddCommand(ICommand* cmd)
   {
      command_list_.push_back(cmd);
   }

   bool RunFirstCommand(EmulatorEngine* machine)
   {
      it_ = command_list_.begin();
      if (it_ != command_list_.end())
      {
         return (*it_++)->Action(machine);

      }
      else return false;
   }

   bool RunNextCommand(EmulatorEngine* machine)
   {
      if (it_ != command_list_.end())
      {
         return (*it_++)->Action(machine);
      }
      else return false;

   }

   bool IsFinished()
   {
      return (it_ == command_list_.end());
   }
protected:
   std::vector<ICommand*> command_list_;
   std::vector<ICommand*>::iterator it_;

};

class TestDump
{
public:
   TestDump()
   {
      machine_ = new EmulatorEngine();
   }
   virtual ~TestDump()
   {
      delete machine_;
   }

   bool Test(const char* conf, const char* initfile, const char* dump_to_load, const char* run_command, CommandList* cmd_list, bool bFixedSpeed = true, int seed = 0xFEED1EE7);


   DirectoriesImp dirImp;
   CDisplay display;
   Log log;
   SoundFactory soundFactory;
   EmulatorEngine* machine_;
};

class TestTape
{
public:
   TestTape()
   {
      machine_ = new EmulatorEngine();
   }
   virtual ~TestTape()
   {
      delete machine_;
   }

   bool Test(char* conf, char* initfile, char* dump_to_load, char* fic_to_scan,
      unsigned short addr, unsigned short opcode, char* reg, int timeout, bool build);
   bool MoreTest(char* fic_to_scan, unsigned short addr, unsigned short end_addr, char* reg, int timeout, bool build);

   int RunTimeSlicesDbg(int nbSlices);
   unsigned short GetRegister(char* pRegister);

   DirectoriesImp dirImp;
   CDisplay display;
   Log log;
   SoundFactory soundFactory;
   EmulatorEngine* machine_;
   ConfigurationManager conf_manager;

};

class KeyboardForTest : public IKeyboardHandler
{
public:
   KeyboardForTest() {};
   virtual ~KeyboardForTest() {};
   virtual unsigned char GetKeyboardMap(int index) { return 0xFF; }
   virtual void Init(bool* register_replaced) {}
   virtual void ForceKeyboardState(unsigned char key_states[10]) {};
};
