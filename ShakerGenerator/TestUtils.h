#pragma once 

#include <iostream>
#include <functional>
#include "simple_stdio.h"
#include "Machine.h"
#include "Display.h"
#include <filesystem>

// SCR_CREATE  = false : Generate screenshot
// SCR_COMPARE  = true: Compare screenshot
#define NO_INIT_SCREENSHOT

#define  SCR_COMPARE true
#define  SCR_CREATE false

/////////////////////////////////////////////////////////////
/// Helper functions

bool CompareDisks(IDisk *d1, IDisk* d2, std::string p1);
bool CompareTape(std::string p1);

bool InitBinary(const char* conf, const char* initfile, const char* binary_to_load, unsigned short addr_ok, unsigned short addr_ko, unsigned int tolerated_error = 0);
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
   virtual unsigned int GetConfiguration(const char* section, const char* cle, const char* default_value, char* out_buffer, unsigned int buffer_size, const char* file);
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
   FileLog(const char* file);
   virtual ~FileLog();

   virtual void WriteLog(const char* pLog);

   virtual void WriteLogByte(unsigned char pNumber);
   virtual void WriteLogShort(unsigned short pNumber);
   virtual void WriteLog(unsigned int pNumber);
   virtual void EndOfLine();
protected:
   FILE * f_;
};

class Log : public ILog
{
public:

   virtual void WriteLog(const char* pLog) {  };
   virtual void WriteLogByte(unsigned char pNumber) { char buf[256]; sprintf(buf, " %2.2X ", pNumber); }
   virtual void WriteLogShort(unsigned short pNumber) { char buf[256]; sprintf(buf, " %4.4X ", pNumber); }
   virtual void WriteLog(unsigned int pNumber) { char buf[256]; sprintf(buf, " %8.8X ", pNumber); };
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

extern std::map<std::string, std::pair<unsigned int, unsigned int>> Escape_map_;

class CommandList;

class ICommand
{
public:
   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list) = 0;

protected:
    static void Wait(EmulatorEngine* machine, unsigned int nb_us)
    {
        while (nb_us > 0)
        {
            unsigned long tick_to_run;
            if (nb_us < 4000 * 10) // 10ms
            {
                tick_to_run = nb_us;
            }
            else
            {
                tick_to_run = 4000 * 10;
            }
            machine->GetMotherboard()->DebugNew(tick_to_run);

            nb_us -= tick_to_run;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};

class CommandList
{
public:
    CommandList() {};
    virtual ~CommandList()
    {
        for (std::vector<ICommand*>::iterator it = command_list_.begin(); it != command_list_.end(); it++)
        {
            delete* it;
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
            return (*it_++)->Action(machine, this);

        }
        else return false;
    }

    bool RunNextCommand(EmulatorEngine* machine)
    {
        if (it_ != command_list_.end())
        {
            return (*it_++)->Action(machine, this);
        }
        else return false;

    }

    bool IsFinished()
    {
        return (it_ == command_list_.end());
    }

    void SetKeyDelay(unsigned int delay_press, unsigned int delay, unsigned int delay_cr)
    {
        delay_ = delay;
        delay_press_ = delay_press;
        delay_cr_ = delay_cr_;
    }

    unsigned int GetKeyPressDelay() { return delay_press_; }
    unsigned int GetKeyDelay() { return delay_; }
    unsigned int GetKeyDelayCR() { return delay_cr_; }
protected:
    std::vector<ICommand*> command_list_;
    std::vector<ICommand*>::iterator it_;

    unsigned int delay_press_;
    unsigned int delay_;
    unsigned int delay_cr_;
};

class CommandSelectCRTC : public ICommand
{
public:
    CommandSelectCRTC(std::string& crtc)
    {
        CRTC::TypeCRTC type_crtc = CRTC::HD6845S;

        if (crtc == "0") type_crtc_ = CRTC::HD6845S;
        else if (crtc == "1") type_crtc_ = CRTC::UM6845R;
        else if (crtc == "1A") type_crtc_ = CRTC::UM6845R;
        else if (crtc == "1B") type_crtc_ = CRTC::UM6845R;
        else if (crtc == "2") type_crtc_ = CRTC::MC6845;
        else if (crtc == "3") type_crtc_ = CRTC::AMS40489;
        else if (crtc == "4") type_crtc_ = CRTC::AMS40226;
        else if (crtc == "PUSSY") type_crtc_ = CRTC::UM6845R; // ?
    }

    virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list) 
    { 
        machine->GetCRTC()->DefinirTypeCRTC(type_crtc_);
        return true; 
    };

protected:
    CRTC::TypeCRTC type_crtc_;
};

class CommandAddBreakpoint : public ICommand
{
public:

   CommandAddBreakpoint(unsigned short bp) : bp_(bp)
   {
   }
   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list) { machine->AddBreakpoint(bp_); return true; };
protected:
   unsigned short bp_;
};

class CommandEjectDisk : public ICommand
{
public:
   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list) { machine->Eject(); return true; };

protected:

};
class CommandInsertDisk : public ICommand
{
public:
   CommandInsertDisk(const char* pathfile) :pathfile_(pathfile) {};
   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list) 
   { 
       std::filesystem::path path = "C:/Thierry/Amstrad/DSK/DSK/Developpement";

       return machine->LoadDisk((path / pathfile_).string().c_str(), 0, false) == 0;
   };

protected:
   std::filesystem::path pathfile_;
};

class CommandRunCycles : public ICommand
{
public:

   CommandRunCycles(int nb_cycles) :nb_cycles_(nb_cycles)
   {
   }

   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
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

   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
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
   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
   {
      if (!display_->InitScreenshotDetection(filename_.c_str())) return false;
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
   CommandSaveScreenshot(CDisplay * display, std::filesystem::path filename, bool verify) : display_(display), filename_(filename), verify_(verify)
   {

   }
   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
   {
      // Get current screen
      // Verify or save ?
      if (verify_)
      {
         //return display_->CompareScreenshot(filename_.c_str());
         if (!display_->InitScreenshotDetection(filename_.string().c_str())) return false;
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
         display_->TakeScreenshot(filename_.string().c_str());
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
   std::filesystem::path filename_;
   bool verify_;

};

class CommandTrace : public ICommand
{
public:

   CommandTrace(std::function<bool(EmulatorEngine* machine)> lambda) : lambda_(lambda)
   {
   }

   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
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

   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
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

class CommandWait : public ICommand
{
public:
    CommandWait(unsigned int nb_us) :nb_us_(nb_us)
    {
    }

    virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
    {
        Wait(machine, nb_us_);

        return true;
    }

protected:
    unsigned int nb_us_;
};

class CommandKeyboard : public ICommand
{
public:
   CommandKeyboard(const char* command) :command_(command)
   {
   }

   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
   {
      machine->Paste(command_.c_str());
      return true;
   }
protected:
   std::string command_;
};

class CommandKeyOutput : public ICommand
{
public:
    CommandKeyOutput(const char* command) :command_(command)
    {
    }

    virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
    {
        // For each character :
        int index = 0;
        std::vector<unsigned int> next_char;
        index = GetNextKey(command_, index, next_char, machine);
        while (index != -1)
        {
            // Press the key
            for (auto& it : next_char)
            {
                if ((it & 0xFFFFFF00) == 0)
                {
                    machine->GetKeyboardHandler()->CharPressed(it);
                }
                else
                {
                    machine->GetKeyboardHandler()->SendScanCode(it, true);
                }

            }

            // wait
            Wait(machine, cmd_list->GetKeyPressDelay());

            // unpress the key
            for (auto& it : next_char)
            {
                if ((it & 0xFFFFFF00) == 0)
                {
                    machine->GetKeyboardHandler()->CharReleased(it);
                }
                else
                {
                    machine->GetKeyboardHandler()->SendScanCode(it, false);
                }
            }

            // wait again
            // wait
            Wait(machine, cmd_list->GetKeyDelay());

            next_char.clear();
            index = GetNextKey(command_, index, next_char, machine);
        }

        return true;
    }

protected:
    std::string command_;

    static int GetNextKey(std::string& line, int index, std::vector<unsigned int>& next, EmulatorEngine* machine)
    {
        int return_index = -1;
        if (index < line.size())
        {
            if (strncmp(&line[index], "\\(", 2) == 0)
            {
                auto endseq = std::find(line.begin() + index, line.end(), ')');
                std::string spec = line.substr(index, endseq - line.begin() + index);

                if (Escape_map_.find(spec) != Escape_map_.end())
                {
                    unsigned int line = Escape_map_[spec].first;
                    unsigned int bit = Escape_map_[spec].second;

                    unsigned int scanCode = machine->GetKeyboardHandler()->GetScanCode(line, bit);
                    next.push_back(scanCode);
                }
                else
                {
                    return return_index;
                }
                return_index = index + spec.size();
            }
            else if (strncmp(&line[index], "\\{", 2) == 0)
            {
                auto endseq = std::find(line.begin() + index, line.end(), '}');
                std::string spec = line.substr(index, endseq - line.begin() + index);
                if (spec.size() > 0)
                {
                    return_index = index + spec.size();
                    for (auto& c : spec)
                    {
                        next.push_back((int)c & 0xFF);
                    }

                }
            }
            else
            {
                next.push_back((int)line[index++] & 0xFF);
                return_index = index;
            }
        }

        return return_index;
    }
};

class CommandKeyDelay : public ICommand
{
public:
    CommandKeyDelay(std::vector<std::string>& param)
    {
        if (param.size() < 3)
        {
            return;
        }

        unsigned int delay1, delay2, delay3;
        char* end;
        delay_press_ = strtol(param[1].c_str(), &end, 10);
        delay_ = strtol(param[2].c_str(), &end, 10);
        if (param.size() == 4)
        {
            delay_cr_ = strtol(param[3].c_str(), &end, 10);
        }
        else
        {
            delay_cr_ = delay_;
        }
    }

    virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
    {
        cmd_list->SetKeyDelay(delay_press_, delay_, delay_cr_);

        return true;
    }

protected:
    unsigned int delay_press_;
    unsigned int delay_;
    unsigned int delay_cr_;
};

class CommandScanCode : public ICommand
{
public:
   CommandScanCode(IKeyboard* pKeyHandler, unsigned short scancode, unsigned int pressed) : pKeyHandler_(pKeyHandler), scancode_(scancode), pressed_(pressed)
   {
   }

   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
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

   virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
   {
      machine->GetKeyboardHandler()->JoystickAction(joy_, action_);
      return true;
   }
protected:
   int action_;
   int joy_;
};

class CommandReset : public ICommand
{
public:
    CommandReset()
    {
    }

    virtual bool Action(EmulatorEngine* machine, CommandList* cmd_list)
    {
        machine->OnOff();
        return true;
    }
};

class TestDump
{
public:
   TestDump(bool display = false) : display_(display)
   {
      machine_ = new EmulatorEngine();
   }
   virtual ~TestDump()
   {
      delete machine_;
   }

   void CustomFunction(unsigned int i);
   void SetScreenshotHandler();
   bool Test(std::filesystem::path conf, std::filesystem::path initfile, CommandList* cmd_list, bool bFixedSpeed = true, int seed = 0xFEED1EE7);

   bool display_;
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

   bool Test(const char* conf, const char* initfile, const char* dump_to_load, const char* fic_to_scan,
      unsigned short addr, unsigned short opcode, const char* reg, int timeout, bool build);
   bool MoreTest(const char* fic_to_scan, unsigned short addr, unsigned short end_addr, const char* reg, int timeout, bool build);

   int RunTimeSlicesDbg(int nbSlices);
   unsigned short GetRegister(const char* pRegister);

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
