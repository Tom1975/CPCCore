#pragma once

#include <vector>

class IScriptRunner;

class ICommand
{
public:
    virtual bool Action(IScriptRunner* scriptRunner) = 0;
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

    bool RunFirstCommand(IScriptRunner* scriptRunner)
    {
        it_ = command_list_.begin();
        if (it_ != command_list_.end())
        {
            return (*it_++)->Action(scriptRunner);

        }
        else return false;
    }

    bool RunNextCommand(IScriptRunner* scriptRunner)
    {
        if (it_ != command_list_.end())
        {
            return (*it_++)->Action(scriptRunner);
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