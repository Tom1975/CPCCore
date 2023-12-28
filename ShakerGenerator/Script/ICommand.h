#pragma once

#include <vector>
#include <iostream>

class IScriptRunner;

class ICommand
{
public:
    virtual bool Action(IScriptRunner* scriptRunner) = 0;
};

class CommandList
{
public:
    CommandList(): index_(0) 
    {
    }
    ;
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

    bool RunNextCommand(IScriptRunner* scriptRunner)
    {
        std::cout << "Running command script " << index_ << std::endl;

        if (index_ < command_list_.size())
        {
            return command_list_[index_++]->Action(scriptRunner);
        }
        else return false;

    }

    bool IsFinished()
    {
        return (index_ == command_list_.size());
    }

protected:
    std::vector<ICommand*> command_list_;
    unsigned int index_;
};