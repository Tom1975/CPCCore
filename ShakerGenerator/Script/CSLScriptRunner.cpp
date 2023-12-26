#include "CSLScriptRunner.h"

#include "Commands.h"
#include <iostream>
#include <fstream>

std::map<std::string, std::function<ICommand* (std::vector<std::string>&)>> CSLScriptRunner::function_map_ = {
    { "csl_version" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "reset" , [](std::vector<std::string>& args)->ICommand* { return new CommandReset(); }},
    { "crtc_select" , [](std::vector<std::string>& args)->ICommand* { return new CommandSelectCRTC(args[1]); }},
    { "disk_insert" , [](std::vector<std::string>& args)->ICommand* { return new CommandInsertDisk(args[1].c_str()); }},
    { "disk_dir" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "tape_insert" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "tape_dir" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "tape_play" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "tape_stop" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "tape_rewind" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "snapshot_load" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "snapshot_dir" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "snapshot_name" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "key_delay" , [](std::vector<std::string>& args)->ICommand* { return new CommandKeyDelay(args); }},
    { "key_output" , [](std::vector<std::string>& args)->ICommand* { return new CommandKeyOutput(args[1].c_str()); }},
    { "key_from_file" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "wait" , [](std::vector<std::string>& args)->ICommand* { return new CommandWait(strtol(args[1].c_str(), NULL, 10) * 4); }},
    { "wait_driveonoff" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "wait_vsyncoffon" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "screenshot_name" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "screenshot_dir" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "screenshot" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "snapshot" , [](std::vector<std::string>& args)->ICommand* { return nullptr; }},
    { "csl_load" , [](std::vector<std::string>& args)->ICommand* { return new CommandLoadScript(args[1].c_str()); }}
};

ICommand* CSLScriptRunner::GetCommand(std::vector<std::string>& args)
{
    ICommand* current_command = nullptr;
    if (function_map_.find(args[0]) != function_map_.end())
    {
        current_command = function_map_[args[0]](args);
    }
    return current_command;
}

void CSLScriptRunner::LoadScript(std::filesystem::path& script_path)
{
    std::ifstream f(script_path);
    std::string line;

    while (std::getline(f, line))
    {
        // Handle line
        std::string::size_type begin = line.find_first_not_of(" \f\t\v");

        // Skip blank lines
        if (begin == std::string::npos) continue;
        // Skip commentary
        std::string::size_type end = line.find_first_of(";");
        if (end != std::string::npos)
            line = line.substr(begin, end);

        if (line.empty()) continue;

        // Get command
        std::vector<std::string> command_parameters;

        std::string current_parameter;
        char current_delim = ' ';
        for (auto& c : line)
        {
            if (c == ';')
            {
                break;
            }
            if (c == current_delim)
            {
                // New 
                if (current_parameter.size() > 0)
                    command_parameters.push_back(current_parameter);
                current_parameter.clear();
                current_delim = ' ';
            }
            else if (c == '\'' && current_parameter.size() == 0)
            {
                current_delim = '\'';
            }
            else if (c == '\"' && current_parameter.size() == 0)
            {
                current_delim = '\"';
            }
            else
            {
                current_parameter += c;
            }
        }
        // Comment : no more parameter to handle
        if (current_parameter.size() > 0)
            command_parameters.push_back(current_parameter);

        // Look for command
        ICommand* command = GetCommand(command_parameters);

        if (command != nullptr)
        {
            AddCommand(command);
        }
        else
        {
            std::cout << "unknow command : " << command_parameters[0] << std::endl;
        }
    }
}
