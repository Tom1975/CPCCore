#pragma once

#include "IScriptRunner.h"

class CSLScriptRunner : public IScriptRunner
{
public:
	CSLScriptRunner(EmulatorEngine* emulator_engine) : IScriptRunner(emulator_engine)
	{}

	virtual void LoadScript(std::filesystem::path& path);

private:
	static std::map<std::string, std::function<ICommand* (std::vector<std::string>&)>> function_map_;

	ICommand* GetCommand(std::vector<std::string>& args);
};