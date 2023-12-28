#pragma once

#include "IScriptRunner.h"

class CDisplay;

class CSLScriptRunner : public IScriptRunner
{
public:
	CSLScriptRunner(EmulatorEngine* emulator_engine, CDisplay* display) : IScriptRunner(emulator_engine), display_(display)
	{}

	virtual void LoadScript(const char* path);

	virtual void SetScreenshotHandler();

private:
	static std::map<std::string, std::function<ICommand* (std::vector<std::string>&)>> function_map_;

	CDisplay* display_;
	unsigned int screenshot_HHLL_ = 0;
	unsigned char screenshot_count_ = 0;

	ICommand* GetCommand(std::vector<std::string>& args);

	void CustomFunction(unsigned int i);
};