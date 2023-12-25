#pragma once

#include "ICommand.h"
#include "IScriptRunner.h"
#include "Machine.h"
#include "../Display.h"

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

    virtual bool Action(IScriptRunner* script_runner)
    {
        script_runner->GetEmulatorEngine()->GetSettings()->SetCRTCType(type_crtc_);
        script_runner->GetEmulatorEngine()->GetCRTC()->DefinirTypeCRTC(type_crtc_);
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
    virtual bool Action(IScriptRunner* script_runner) { script_runner->GetEmulatorEngine()->AddBreakpoint(bp_); return true; };
protected:
    unsigned short bp_;
};

class CommandEjectDisk : public ICommand
{
public:
    virtual bool Action(IScriptRunner* script_runner) { script_runner->GetEmulatorEngine()->Eject(); return true; };

protected:

};
class CommandInsertDisk : public ICommand
{
public:
    CommandInsertDisk(const char* pathfile) :pathfile_(pathfile) {};
    virtual bool Action(IScriptRunner* script_runner)
    {
        std::filesystem::path path = "C:/Thierry/Amstrad/DSK/DSK/Developpement";

        return script_runner->GetEmulatorEngine()->LoadDisk((path / pathfile_).string().c_str(), 0, false) == 0;
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

    virtual bool Action(IScriptRunner* script_runner)
    {
        for (int i = 0; i < nb_cycles_; i++)
        {
            script_runner->GetEmulatorEngine()->RunTimeSlice(true);
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

    virtual bool Action(IScriptRunner* script_runner)
    {
        script_runner->GetEmulatorEngine()->AddBreakpoint(bp_);
        for (int i = 0; i < nb_cycles_; i++)
        {
            script_runner->GetEmulatorEngine()->RunTimeSlice(false);

            // If Breakpoint, end loop
            if (script_runner->GetEmulatorEngine()->GetProc()->GetPC() == bp_)
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
    CommandRunToScreenshot(CDisplay* display, std::string filename, int nb_cycles_timeout) : display_(display), filename_(filename), nb_cycles_timeout_(nb_cycles_timeout)
    {

    }
    virtual bool Action(IScriptRunner* script_runner)
    {
        if (!display_->InitScreenshotDetection(filename_.c_str())) return false;
        for (int i = 0; i < nb_cycles_timeout_; i++)
        {
            script_runner->GetEmulatorEngine()->RunTimeSlice(true);

            if (display_->IsScreenshotFound())
                return true;
        }


        return false;
    }
protected:
    CDisplay* display_;
    std::string filename_;
    int nb_cycles_timeout_;

};

class CommandSaveScreenshot : public ICommand
{
public:
    CommandSaveScreenshot(CDisplay* display, std::filesystem::path filename, bool verify) : display_(display), filename_(filename), verify_(verify)
    {

    }
    virtual bool Action(IScriptRunner* script_runner)
    {
        // Get current screen
        // Verify or save ?
        if (verify_)
        {
            //return display_->CompareScreenshot(filename_.c_str());
            if (!display_->InitScreenshotDetection(filename_.string().c_str())) return false;
            for (int i = 0; i < 100; i++)
            {
                script_runner->GetEmulatorEngine()->RunTimeSlice(true);

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
                script_runner->GetEmulatorEngine()->RunTimeSlice(false);
            }
            //display_->ScreenshotToFile(filename_.c_str());
        }
        return true;
    }
protected:
    CDisplay* display_;
    std::filesystem::path filename_;
    bool verify_;

};

class CommandTrace : public ICommand
{
public:

    CommandTrace(std::function<bool(EmulatorEngine* machine)> lambda) : lambda_(lambda)
    {
    }

    virtual bool Action(IScriptRunner* script_runner)
    {
        lambda_(script_runner->GetEmulatorEngine());
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

    virtual bool Action(IScriptRunner* script_runner)
    {
        for (int i = 0; i < nb_cycles_; i++)
        {
            script_runner->GetEmulatorEngine()->RunTimeSlice(false);

            // If Breakpoint, end loop
            if (lambda_(script_runner->GetEmulatorEngine()))
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

    virtual bool Action(IScriptRunner* script_runner)
    {
        script_runner->Wait(nb_us_);

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

    virtual bool Action(IScriptRunner* script_runner)
    {
        script_runner->GetEmulatorEngine()->Paste(command_.c_str());
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

    virtual bool Action(IScriptRunner* script_runner)
    {
        // For each character :
        int index = 0;
        std::vector<unsigned int> next_char;
        index = GetNextKey(command_, index, next_char, script_runner->GetEmulatorEngine());
        while (index != -1)
        {
            // Press the key
            for (auto& it : next_char)
            {
                if ((it & 0xFFFFFF00) == 0)
                {
                    script_runner->GetEmulatorEngine()->GetKeyboardHandler()->CharPressed(it);
                }
                else
                {
                    script_runner->GetEmulatorEngine()->GetKeyboardHandler()->SendScanCode(it, true);
                }

            }

            // wait
            script_runner->Wait(script_runner->GetKeyPressDelay());

            // unpress the key
            for (auto& it : next_char)
            {
                if ((it & 0xFFFFFF00) == 0)
                {
                    script_runner->GetEmulatorEngine()->GetKeyboardHandler()->CharReleased(it);
                }
                else
                {
                    script_runner->GetEmulatorEngine()->GetKeyboardHandler()->SendScanCode(it, false);
                }
            }

            // wait again
            // wait
            script_runner->Wait(script_runner->GetKeyDelay());

            next_char.clear();
            index = GetNextKey(command_, index, next_char, script_runner->GetEmulatorEngine());
        }

        return true;
    }

protected:
    std::string command_;

    static std::map<std::string, std::pair<unsigned int, unsigned int>> Escape_map_;

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

    virtual bool Action(IScriptRunner* script_runner)
    {
        script_runner->SetKeyDelay(delay_press_, delay_, delay_cr_);

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

    virtual bool Action(IScriptRunner* script_runner)
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

    virtual bool Action(IScriptRunner* script_runner)
    {
        script_runner->GetEmulatorEngine()->GetKeyboardHandler()->JoystickAction(joy_, action_);
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

    virtual bool Action(IScriptRunner* script_runner)
    {
        script_runner->GetEmulatorEngine()->OnOff();
        return true;
    }
};