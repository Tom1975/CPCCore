#pragma once

// CPC Script Language (CSL 1.5) and ScreenShot Management (SSM 1.2), as defined by Longshot /
// Logon System (https://shaker.logonsystem.eu, CSL-STANDARD-EN.pdf and SSM-STANDARD-EN.pdf).
//
// A CSL file replaces the user : it selects the CRTC, resets the machine, inserts disks, types
// on the keyboard and waits a number of emulated microseconds. SSM lets the emulated code ask a
// screenshot : the sequence ED <LL> ED <HH> of two consecutive "neutral" ED opcodes carries the
// 16-bit code HHLL.
//
// The interpreter runs the emulation itself (EmulatorEngine::RunCycles), so a script runs
// headless and at full speed : every delay is emulated time, never real time. What the engine
// cannot do alone - turning a frame into an image file - is asked to the host (ICslHost).

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

class EmulatorEngine;

class ICslHost
{
public:
   virtual ~ICslHost() = default;

   // A screenshot is asked : the host saves the last complete frame, under 'path' plus the
   // extension of its image format.
   virtual void RequestScreenshot(const std::filesystem::path& path) = 0;

   // One line of the script log (instruction executed, error)
   virtual void Log(const std::string& /*line*/) {}
};

// What the CSL standard asks the emulator to report when a script stops on an error
struct CslError
{
   std::filesystem::path script;
   unsigned int line = 0;
   std::string instruction;
   std::string reason;
   std::string version;       // csl_version of the script, if given

   bool IsSet() const { return !reason.empty(); }
   std::string ToString() const;
};

// One SSM code received from the emulated code
struct SsmEvent
{
   uint16_t code;
   uint64_t time_us;                  // emulated time since the start of the run
   std::filesystem::path screenshot;  // empty when the code takes no screenshot
};

class CslScript
{
public:
   static constexpr const char* kSupportedVersion = "1.5";

   CslScript(EmulatorEngine* engine, ICslHost* host);
   ~CslScript();

   CslScript(const CslScript&) = delete;
   CslScript& operator=(const CslScript&) = delete;

   // Default directories, used until the script gives its own (disk_dir, ...). A script file
   // name without directory is looked for in the directory of the running script otherwise.
   void SetDiskDir(const std::filesystem::path& dir) { disk_dir_ = dir; }
   void SetTapeDir(const std::filesystem::path& dir) { tape_dir_ = dir; }
   void SetSnapshotDir(const std::filesystem::path& dir) { snapshot_dir_ = dir; }
   void SetScreenshotDir(const std::filesystem::path& dir) { screenshot_dir_ = dir; }

   // Prefix of the SSM screenshot names : <name>_<crtc>_<HHLL> (SSM, "image name format")
   void SetEmulatorName(const std::string& name) { emulator_name_ = name; }

   // csl_load runs the next file (default). When off, csl_load ends the run and the file it
   // names is given by NextScript() : the scripts of a chain can then run on their own.
   void SetFollowCslLoad(bool follow) { follow_csl_load_ = follow; }

   // Longest wait_ssm / wait_ssm0000 / wait_driveonoff / wait_vsyncoffon before the script
   // stops on an error, in emulated microseconds. The standard sets no limit; 0 = no limit.
   void SetWaitTimeout(uint64_t us) { wait_timeout_us_ = us; }

   // Runs a script file, or the text of a script ('origin' names it in the errors). Returns
   // false when the script stopped on an error (see Error()).
   bool RunFile(const std::filesystem::path& file);
   bool RunText(const std::string& text, const std::filesystem::path& origin);

   const CslError& Error() const { return error_; }
   const std::filesystem::path& NextScript() const { return next_script_; }
   const std::vector<SsmEvent>& SsmEvents() const { return ssm_events_; }
   const std::vector<std::filesystem::path>& Screenshots() const { return screenshots_; }
   uint64_t EmulatedMicroseconds() const { return time_us_; }

   // CSL screenshot name of an SSM code for the current CRTC
   std::string SsmScreenshotName(uint16_t code) const;

protected:
   struct Instruction
   {
      unsigned int line;
      std::string text;                // line without comment, for the reports
      std::string name;                // lower case
      std::vector<std::string> args;   // quotes removed
   };

   static bool Parse(const std::string& text, std::vector<Instruction>& out, std::string& error, unsigned int& error_line);

   bool Execute(const Instruction& ins);
   bool Fail(const Instruction& ins, const std::string& reason);

   // Runs the emulation for 'us' microseconds
   void Wait(uint64_t us);
   // Runs the emulation until 'done' is true, checking it every 'step_us' microseconds
   bool WaitUntil(const std::function<bool()>& done, const Instruction& ins, const char* what, unsigned int step_us = 1);

   bool TypeText(const std::string& text, const Instruction& ins);
   void KeyGroup(const std::vector<int>& keys, bool pressed);

   std::filesystem::path Resolve(const std::filesystem::path& dir, const std::string& name) const;
   std::filesystem::path ScreenshotPath(const std::string& name) const;
   int CrtcNumber() const;

   // SSM decoder
   void EnableSsm();
   void DisableSsm();
   void OnEdOpcode(unsigned int opcode);
   void OnSsmCode(uint16_t code);

   EmulatorEngine* engine_;
   ICslHost* host_;

   // Context of the current script
   std::filesystem::path script_;
   std::string version_;
   std::filesystem::path disk_dir_;
   std::filesystem::path tape_dir_;
   std::filesystem::path snapshot_dir_;
   std::filesystem::path screenshot_dir_;
   std::string screenshot_name_;
   std::string snapshot_name_;
   std::string emulator_name_ = "SUGARBOX";

   // key_delay : press time, time between two keys, time after a CR
   uint64_t key_press_us_ = 19968;
   uint64_t key_delay_us_ = 19968;
   uint64_t key_delay_cr_us_ = 19968;

   bool follow_csl_load_ = true;
   uint64_t wait_timeout_us_ = 0;
   int nesting_ = 0;

   CslError error_;
   std::filesystem::path next_script_;
   std::vector<SsmEvent> ssm_events_;
   std::vector<std::filesystem::path> screenshots_;
   uint64_t time_us_ = 0;

   // SSM : ED <LL> ED <HH> at consecutive addresses
   bool ssm_low_valid_ = false;
   uint8_t ssm_low_ = 0;
   uint16_t ssm_low_pc_ = 0;
   bool ssm_wait_ = false;           // a wait_ssm is running
   uint16_t ssm_wait_code_ = 0;
   bool ssm_wait_hit_ = false;
   std::vector<uint16_t> ssm_since_wait_;   // codes received since the last wait_ssm, in order
};
