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

    void AddCommand(ICommand* cmd, const char* str)
    {
        command_list_.push_back(cmd);
        command_list_str_.push_back(str);
    }

    bool RunNextCommand(IScriptRunner* scriptRunner)
    {
        if (index_ < command_list_.size())
        {
            std::cout << command_list_str_[index_] << std::endl;
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
    std::vector<std::string> command_list_str_;
    unsigned int index_;
};