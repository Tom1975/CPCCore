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

	virtual void LoadScript(std::filesystem::path& path) = 0;
	void AddCommand(ICommand* cmd)
	{
		command_list_.AddCommand(cmd);
	};

	EmulatorEngine* GetEmulatorEngine() { return emulator_engine_; }

	bool Run()
	{
		bool no_error = command_list_.RunFirstCommand(this);
		while (command_list_.IsFinished() == false && no_error)
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

protected:
	CommandList command_list_;
	EmulatorEngine* emulator_engine_;

	unsigned int delay_press_;
	unsigned int delay_;
	unsigned int delay_cr_;
};