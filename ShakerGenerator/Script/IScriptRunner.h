#pragma once

#include <filesystem>
#include "ICommand.h"
#include "Machine.h"

class EmulatorEngine;

class IScriptRunner
{
public:
	IScriptRunner(EmulatorEngine* emulator_engine) : emulator_engine_(emulator_engine)
	{}

	virtual void LoadScript(const char* path) = 0;
	void AddCommand(ICommand* cmd)
	{
		command_list_.AddCommand(cmd);
	};

	EmulatorEngine* GetEmulatorEngine() { return emulator_engine_; }

	virtual void SetScreenshotHandler() = 0;

	bool Run()
	{
		bool no_error = true;
		while (!command_list_.IsFinished() && no_error)
		{
			no_error = command_list_.RunNextCommand(this);
		}
		return no_error;
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

	void Wait(unsigned int nb_us)
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
			emulator_engine_->GetMotherboard()->DebugNew(tick_to_run);

			nb_us -= tick_to_run;
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	std::filesystem::path GetScriptDirectory() { return script_path_; }
	void SetScriptDirectory(const char* path) { script_path_ = path; }

	std::filesystem::path GetDiskDirectory() { return disk_path_; }
	void SetDiskDirectory(const char* path) { disk_path_ = path; }

	std::filesystem::path GetScreenshotDirectory() { return screenshot_path_; }
	void SetScreenshotDirectory(const char* path) { screenshot_path_ = path; }

protected:
	CommandList command_list_;
	EmulatorEngine* emulator_engine_;

	unsigned int delay_press_;
	unsigned int delay_;
	unsigned int delay_cr_;

	std::filesystem::path script_path_;
	std::filesystem::path disk_path_;
	std::filesystem::path screenshot_path_;
};