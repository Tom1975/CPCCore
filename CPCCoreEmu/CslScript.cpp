#include "CslScript.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>

#include "Machine.h"

namespace fs = std::filesystem;

namespace
{
// 4 MHz ticks per emulated microsecond
constexpr uint64_t kTicksPerUs = 4;

// Keys of the CSL annex that are not characters : (line, bit) on the CPC keyboard matrix
struct MatrixKey { unsigned char line; unsigned char bit; };
const std::map<std::string, MatrixKey>& EscapeKeys()
{
   static const std::map<std::string, MatrixKey> keys = {
      { "ESC", { 8, 2 } }, { "TAB", { 8, 4 } }, { "CAP", { 8, 6 } }, { "SHI", { 2, 5 } },
      { "CTR", { 2, 7 } }, { "COP", { 1, 1 } }, { "CLR", { 2, 0 } }, { "DEL", { 9, 7 } },
      { "RET", { 2, 2 } }, { "ENT", { 0, 6 } }, { "ARL", { 1, 0 } }, { "ARR", { 0, 1 } },
      { "ARU", { 0, 0 } }, { "ARD", { 0, 2 } },
      { "FN0", { 1, 7 } }, { "FN1", { 1, 5 } }, { "FN2", { 1, 6 } }, { "FN3", { 0, 5 } },
      { "FN4", { 2, 4 } }, { "FN5", { 1, 4 } }, { "FN6", { 0, 4 } }, { "FN7", { 1, 2 } },
      { "FN8", { 1, 3 } }, { "FN9", { 0, 3 } },
   };
   return keys;
}

// A key of a key_output text : a matrix key (>= 0x100, line * 8 + bit + 0x100) or a character
constexpr int kMatrix = 0x100;
constexpr int kReturnKey = kMatrix + 2 * 8 + 2;

std::string Lower(std::string s)
{
   std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
   return s;
}

std::string Trim(const std::string& s)
{
   size_t b = s.find_first_not_of(" \t\r\n");
   if (b == std::string::npos) return std::string();
   size_t e = s.find_last_not_of(" \t\r\n");
   return s.substr(b, e - b + 1);
}

// Quotes : ' and ", and the typographic ones of the standard's examples (UTF-8 ‘ ’ “ ”).
// Returns the kind of quote at s[i] (0 = none) and its length in bytes.
enum QuoteKind { kNoQuote = 0, kSingle, kDouble, kTypographic };
QuoteKind QuoteAt(const std::string& s, size_t i, size_t& len)
{
   if (s[i] == '\'') { len = 1; return kSingle; }
   if (s[i] == '"') { len = 1; return kDouble; }
   if (i + 2 < s.size() && (unsigned char)s[i] == 0xE2 && (unsigned char)s[i + 1] == 0x80
       && ((unsigned char)s[i + 2] >= 0x98 && (unsigned char)s[i + 2] <= 0x9D))
   {
      len = 3;
      return kTypographic;
   }
   return kNoQuote;
}

bool ParseUnsigned(const std::string& s, uint64_t& out)
{
   if (s.empty()) return false;
   char* end = nullptr;
   const bool hex = s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X');
   out = std::strtoull(hex ? s.c_str() + 2 : s.c_str(), &end, hex ? 16 : 10);
   return end != nullptr && *end == '\0';
}

// The standard writes directories as prefixes, Windows style : 'C:\MyDsk\'
std::string PortablePath(std::string s)
{
   std::replace(s.begin(), s.end(), '\\', '/');
   return s;
}
}

std::string CslError::ToString() const
{
   std::ostringstream os;
   os << script.string() << ":" << line << ": '" << instruction << "' : " << reason
      << " (script version " << (version.empty() ? "?" : version) << ", supported " << CslScript::kSupportedVersion << ")";
   return os.str();
}

CslScript::CslScript(EmulatorEngine* engine, ICslHost* host) : engine_(engine), host_(host)
{
   EnableSsm();
}

CslScript::~CslScript()
{
   DisableSsm();
}

////////////////////////////////////////////////////////
/// Parsing
///
/// One instruction per line, ';' starts a comment (outside a quoted text). An argument is a
/// word, or a quoted text that may hold spaces. Inside a text, an escape \(...) is kept as it
/// is, so \(') does not close the text.
bool CslScript::Parse(const std::string& text, std::vector<Instruction>& out, std::string& error, unsigned int& error_line)
{
   std::istringstream in(text);
   std::string raw;
   unsigned int line_number = 0;
   while (std::getline(in, raw))
   {
      ++line_number;
      Instruction ins;
      ins.line = line_number;

      std::vector<std::string> words;
      std::string current;
      bool in_word = false;
      size_t i = 0;
      size_t comment = raw.size();
      while (i < raw.size())
      {
         size_t qlen = 0;
         const char c = raw[i];
         if (c == ';')
         {
            comment = i;
            break;
         }
         const QuoteKind opening = QuoteAt(raw, i, qlen);
         if (opening != kNoQuote)
         {
            // Quoted text : up to the next quote of the same kind outside an escape. The other
            // quotes are text : key_output 'RUN"SHAKE27A"\(RET)'
            size_t j = i + qlen;
            std::string quoted;
            bool closed = false;
            while (j < raw.size())
            {
               size_t l2 = 0;
               if (raw.compare(j, 2, "\\(") == 0)
               {
                  // \()) and \(() are escapes of a single character
                  size_t close = raw.find(')', j + 3);
                  if (close == std::string::npos) close = raw.size() - 1;
                  quoted += raw.substr(j, close - j + 1);
                  j = close + 1;
                  continue;
               }
               if (QuoteAt(raw, j, l2) == opening)
               {
                  closed = true;
                  j += l2;
                  break;
               }
               quoted += raw[j++];
            }
            if (!closed)
            {
               error = "unterminated text";
               error_line = line_number;
               return false;
            }
            current += quoted;
            in_word = true;
            i = j;
            continue;
         }
         if (c == ' ' || c == '\t' || c == '\r' || c == ',')
         {
            // keyboard_write separates its values with commas
            if (in_word) { words.push_back(current); current.clear(); in_word = false; }
            ++i;
            continue;
         }
         current += c;
         in_word = true;
         ++i;
      }
      if (in_word) words.push_back(current);
      if (words.empty()) continue;

      ins.text = Trim(raw.substr(0, comment));
      ins.name = Lower(words[0]);
      ins.args.assign(words.begin() + 1, words.end());
      out.push_back(ins);
   }
   return true;
}

////////////////////////////////////////////////////////
/// Run
bool CslScript::RunFile(const fs::path& file)
{
   std::ifstream f(file, std::ios::binary);
   if (!f)
   {
      error_ = CslError();
      error_.script = file;
      error_.reason = "cannot open the script";
      if (host_) host_->Log(error_.ToString());
      return false;
   }
   std::stringstream ss;
   ss << f.rdbuf();
   return RunText(ss.str(), file);
}

bool CslScript::RunText(const std::string& text, const fs::path& origin)
{
   if (nesting_ == 0)
   {
      error_ = CslError();
      next_script_.clear();
   }

   std::vector<Instruction> program;
   std::string parse_error;
   unsigned int parse_line = 0;
   if (!Parse(text, program, parse_error, parse_line))
   {
      error_.script = origin;
      error_.line = parse_line;
      error_.reason = parse_error;
      if (host_) host_->Log(error_.ToString());
      return false;
   }

   const fs::path previous_script = script_;
   script_ = origin;
   bool ok = true;
   for (const Instruction& ins : program)
   {
      if (host_)
      {
         std::ostringstream os;
         os << origin.filename().string() << ":" << ins.line << ": +" << time_us_ << "us " << ins.text;
         host_->Log(os.str());
      }
      if (!Execute(ins))
      {
         ok = false;
         break;
      }
      // csl_load without following the chain : this script ends here
      if (!next_script_.empty() && !follow_csl_load_)
         break;
   }

   // A screenshot asked by the last instructions is still to be taken
   if (ok && nesting_ == 0 && host_)
   {
      Instruction flush{ 0, "(end of script)", "", {} };
      WaitUntil([this]() { return !host_->IsScreenshotPending(); }, flush, "the last screenshot", 64);
   }

   script_ = previous_script;
   return ok;
}

bool CslScript::Fail(const Instruction& ins, const std::string& reason)
{
   error_.script = script_;
   error_.line = ins.line;
   error_.instruction = ins.text;
   error_.reason = reason;
   error_.version = version_;
   if (host_) host_->Log("CSL error : " + error_.ToString());
   return false;
}

fs::path CslScript::Resolve(const fs::path& dir, const std::string& name) const
{
   fs::path p(PortablePath(name));
   if (p.is_absolute()) return p;
   // The directory is a prefix (standard) : "dir" + "name"
   if (!dir.empty())
   {
      std::string prefix = PortablePath(dir.string());
      if (!prefix.empty() && prefix.back() != '/') prefix += '/';
      p = fs::path(prefix + PortablePath(name));
   }
   if (p.is_relative()) p = script_.parent_path() / p;
   return p;
}

fs::path CslScript::ScreenshotPath(const std::string& name) const
{
   return Resolve(screenshot_dir_, name);
}

int CslScript::CrtcNumber() const
{
   return (int)engine_->GetCRTC()->type_crtc_;
}

std::string CslScript::SsmScreenshotName(uint16_t code) const
{
   char buffer[16];
   snprintf(buffer, sizeof(buffer), "%04X", code);
   return emulator_name_ + "_" + std::to_string(CrtcNumber()) + "_" + buffer;
}

void CslScript::Wait(uint64_t us)
{
   // Slices of one frame keep the pending engine actions (snapshots) handled regularly
   const uint64_t slice = 19968;
   while (us > 0)
   {
      const uint64_t run = us < slice ? us : slice;
      engine_->RunCycles((unsigned int)(run * kTicksPerUs));
      time_us_ += run;
      us -= run;
   }
}

bool CslScript::WaitUntil(const std::function<bool()>& done, const Instruction& ins, const char* what, unsigned int step_us)
{
   // The condition is looked at every 'step_us' microseconds : 1 for a VSYNC edge, more for the
   // waits that last seconds (SSM code, drive motor)
   uint64_t waited = 0;
   while (!done())
   {
      if (wait_timeout_us_ != 0 && waited >= wait_timeout_us_)
         return Fail(ins, std::string("timeout while waiting for ") + what);
      engine_->RunCycles((unsigned int)(step_us * kTicksPerUs));
      time_us_ += step_us;
      waited += step_us;
   }
   return true;
}

////////////////////////////////////////////////////////
/// Keyboard
void CslScript::KeyGroup(const std::vector<int>& keys, bool pressed)
{
   KeyboardHandler* kb = engine_->GetKeyboardHandler();
   for (int k : keys)
   {
      if (k >= kMatrix)
      {
         kb->KeyAction((k - kMatrix) / 8, (k - kMatrix) % 8, pressed);
         continue;
      }
      // A character is the key of the main keyboard that types it (with SHIFT if needed) : a
      // program that reads the matrix waits for that key, not for the one of the keypad
      unsigned int line = 0, bit = 0;
      bool shift = false;
      if (k == '\n') { line = 2; bit = 2; }
      else if (!kb->FindCharKey((char)k, line, bit, shift)) continue;   // unknown : skipped
      kb->KeyAction(line, bit, pressed);
      if (shift) kb->KeyAction(2, 5, pressed);
   }
   // The keys change at the microsecond the script says. The handler otherwise hands its changes
   // to the matrix at the monitor VSYNC, next to the firmware scan of the keyboard : a scan cut
   // in two by that copy reads the line of SHIFT before it and the line of the key after it.
   kb->ValidateKeyboardMap();
}

bool CslScript::TypeText(const std::string& text, const Instruction& ins)
{
   // Cuts the text in groups : a character, an escape \(...), or the keys between braces
   struct Group { std::vector<int> keys; bool no_delay = false; };
   std::vector<Group> groups;

   auto read_key = [&](size_t& i, std::vector<int>& keys, bool& kof) -> bool
   {
      if (text.compare(i, 2, "\\(") == 0)
      {
         size_t close = text.find(')', i + 3);
         if (close == std::string::npos) return Fail(ins, "unterminated escape in '" + text + "'");
         const std::string code = text.substr(i + 2, close - i - 2);
         i = close + 1;
         auto it = EscapeKeys().find(code);
         if (it != EscapeKeys().end()) keys.push_back(kMatrix + it->second.line * 8 + it->second.bit);
         else if (code == "KOF") kof = true;
         else if (code.size() == 1) keys.push_back((unsigned char)code[0]);   // \({) \(}) \(\) \(')
         // Unknown key : skipped (standard)
         return true;
      }
      keys.push_back((unsigned char)text[i++]);
      return true;
   };

   size_t i = 0;
   while (i < text.size())
   {
      Group g;
      bool kof = false;
      if (text[i] == '{')
      {
         size_t close = text.find('}', i + 1);
         if (close == std::string::npos) return Fail(ins, "unterminated key group in '" + text + "'");
         size_t j = i + 1;
         while (j < close)
         {
            if (!read_key(j, g.keys, kof)) return false;
         }
         i = close + 1;
      }
      else
      {
         if (!read_key(i, g.keys, kof)) return false;
      }
      if (kof)
      {
         // \(KOF) follows a key : no delay after that key
         if (!groups.empty() && g.keys.empty()) groups.back().no_delay = true;
         else g.no_delay = true;
      }
      if (!g.keys.empty()) groups.push_back(g);
   }

   for (const Group& g : groups)
   {
      KeyGroup(g.keys, true);
      Wait(key_press_us_);
      KeyGroup(g.keys, false);
      if (!g.no_delay)
      {
         const bool cr = std::find(g.keys.begin(), g.keys.end(), kReturnKey) != g.keys.end()
            || std::find(g.keys.begin(), g.keys.end(), (int)'\n') != g.keys.end();
         Wait(cr ? key_delay_cr_us_ : key_delay_us_);
      }
   }
   return true;
}

////////////////////////////////////////////////////////
/// Instructions
bool CslScript::Execute(const Instruction& ins)
{
   const std::string& n = ins.name;
   const std::vector<std::string>& a = ins.args;
   auto need = [&](size_t count) { return a.size() >= count; };

   if (n == "csl_version")
   {
      if (need(1)) version_ = a[0];
      return true;
   }
   if (n == "reset")
   {
      const std::string kind = a.empty() ? "hard" : Lower(a[0]);
      if (kind == "soft") engine_->Reset();
      else if (kind == "hard") engine_->OnOff();
      else return Fail(ins, "unknown reset type");
      return true;
   }
   if (n == "crtc_select")
   {
      if (!need(1)) return Fail(ins, "missing CRTC number");
      static const std::map<std::string, CRTC::TypeCRTC> types = {
         { "0", CRTC::HD6845S }, { "1", CRTC::UM6845R }, { "1a", CRTC::UM6845R }, { "1b", CRTC::UM6845R },
         { "2", CRTC::MC6845 }, { "3", CRTC::AMS40489 }, { "4", CRTC::AMS40226 },
      };
      auto it = types.find(Lower(a[0]));
      if (it == types.end()) return Fail(ins, "CRTC not handled by the emulator");
      // In the settings too : a power on rebuilds the machine from them
      engine_->GetSettings()->SetCRTCType(it->second);
      engine_->GetCRTC()->DefinirTypeCRTC(it->second);
      return true;
   }
   if (n == "gate_array" || n == "cpc_model" || n == "memory_exp" || n == "rom_dir" || n == "rom_config"
       || n == "snapshot_version")
   {
      return Fail(ins, "instruction not supported by this emulator");
   }
   if (n == "disk_insert")
   {
      if (!need(1)) return Fail(ins, "missing disk name");
      unsigned int drive = 0;
      std::string name = a.back();
      if (a.size() >= 2)
      {
         const std::string d = Lower(a[0]);
         if (d == "a" || d == "0") drive = 0;
         else if (d == "b" || d == "1") drive = 1;
         else return Fail(ins, "unknown drive");
      }
      const fs::path p = Resolve(disk_dir_, name);
      if (!fs::exists(p)) return Fail(ins, "disk not found : " + p.string());
      if (engine_->LoadDisk(p.string().c_str(), drive, false) != 0) return Fail(ins, "cannot load the disk " + p.string());
      return true;
   }
   if (n == "disk_dir") { if (need(1)) disk_dir_ = PortablePath(a[0]); return true; }
   if (n == "tape_dir") { if (need(1)) tape_dir_ = PortablePath(a[0]); return true; }
   if (n == "snapshot_dir") { if (need(1)) snapshot_dir_ = PortablePath(a[0]); return true; }
   if (n == "screenshot_dir") { if (need(1)) screenshot_dir_ = PortablePath(a[0]); return true; }
   if (n == "tape_insert")
   {
      if (!need(1)) return Fail(ins, "missing tape name");
      const fs::path p = Resolve(tape_dir_, a[0]);
      if (!fs::exists(p)) return Fail(ins, "tape not found : " + p.string());
      if (engine_->LoadTape(p.string().c_str()) != 0) return Fail(ins, "cannot load the tape " + p.string());
      return true;
   }
   if (n == "tape_play") { engine_->GetTape()->Play(); return true; }
   if (n == "tape_stop") { engine_->GetTape()->StopEject(); return true; }
   if (n == "tape_rewind") { engine_->GetTape()->Rewind(); return true; }
   if (n == "snapshot_load")
   {
      if (!need(1)) return Fail(ins, "missing snapshot name");
      const fs::path p = Resolve(snapshot_dir_, a[0]);
      if (!engine_->LoadSnapshotNow(p.string().c_str())) return Fail(ins, "cannot load the snapshot " + p.string());
      return true;
   }
   if (n == "snapshot_name") { if (need(1)) snapshot_name_ = a[0]; return true; }
   if (n == "screenshot_name") { if (need(1)) screenshot_name_ = a[0]; return true; }
   if (n == "key_delay")
   {
      uint64_t press = 0, delay = 0, cr = 0;
      if (!need(2) || !ParseUnsigned(a[0], press) || !ParseUnsigned(a[1], delay)) return Fail(ins, "bad key delays");
      cr = delay;
      if (a.size() >= 3 && !ParseUnsigned(a[2], cr)) return Fail(ins, "bad delay after CR");
      key_press_us_ = press;
      key_delay_us_ = delay;
      key_delay_cr_us_ = cr;
      return true;
   }
   if (n == "key_output")
   {
      if (!need(1)) return Fail(ins, "missing text");
      return TypeText(a[0], ins);
   }
   if (n == "key_from_file")
   {
      if (!need(1)) return Fail(ins, "missing file name");
      const fs::path p = Resolve(fs::path(), a[0]);
      std::ifstream f(p, std::ios::binary);
      if (!f) return Fail(ins, "cannot open " + p.string());
      std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
      // An end of line is a RETURN; \r\n counts once
      content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());
      return TypeText(content, ins);
   }
   if (n == "keyboard_write")
   {
      if (a.size() != 10) return Fail(ins, "10 values expected");
      for (unsigned int line = 0; line < 10; line++)
      {
         uint64_t v = 0;
         if (!ParseUnsigned(a[line], v) || v > 255) return Fail(ins, "bad matrix value");
         for (unsigned int bit = 0; bit < 8; bit++)
            engine_->GetKeyboardHandler()->KeyAction(line, bit, (v & (1u << bit)) == 0);
      }
      return true;
   }
   if (n == "wait")
   {
      uint64_t us = 0;
      if (!need(1) || !ParseUnsigned(a[0], us)) return Fail(ins, "bad delay");
      Wait(us);
      return true;
   }
   if (n == "wait_driveonoff")
   {
      uint64_t count = 1;
      if (!a.empty() && !ParseUnsigned(a[0], count)) return Fail(ins, "bad count");
      FDC* fdc = engine_->GetFDC();
      for (uint64_t c = 0; c < count; c++)
      {
         if (!WaitUntil([fdc]() { return fdc->IsMotorOn(); }, ins, "the drive motor on", 64)) return false;
         if (!WaitUntil([fdc]() { return !fdc->IsMotorOn(); }, ins, "the drive motor off", 64)) return false;
      }
      return true;
   }
   if (n == "wait_vsyncoffon")
   {
      CRTC* crtc = engine_->GetCRTC();
      if (!WaitUntil([crtc]() { return !crtc->VSyncPin(); }, ins, "the VSYNC off")) return false;
      return WaitUntil([crtc]() { return crtc->VSyncPin(); }, ins, "the VSYNC on");
   }
   if (n == "wait_ssm0000" || n == "wait_ssm")
   {
      uint64_t code = 0;
      if (n == "wait_ssm" && (!need(1) || !ParseUnsigned(a[0], code) || code > 0xFFFF)) return Fail(ins, "bad SSM code");
      // The code may come before the wait starts : SHAKER sends #0000 as soon as it reads the
      // key the previous key_output pressed. A code received since the previous wait_ssm ends
      // the wait at once (the standard does not say; the SHAKER scripts rely on it).
      // A code ends the wait, the codes that came after it stay for the next wait_ssm : SHAKER sends
      // #01DA, #01DB, #01DC one frame apart and the script waits for them in turn.
      auto consume = [this, code]()
      {
         auto it = std::find(ssm_since_wait_.begin(), ssm_since_wait_.end(), (uint16_t)code);
         if (it == ssm_since_wait_.end()) return false;
         ssm_since_wait_.erase(ssm_since_wait_.begin(), it + 1);
         return true;
      };
      const bool already = consume();
      if (!already) ssm_since_wait_.clear();
      bool ok = true;
      if (!already)
      {
         ssm_wait_ = true;
         ssm_wait_code_ = (uint16_t)code;
         ssm_wait_hit_ = false;
         ok = WaitUntil([this]() { return ssm_wait_hit_; }, ins, "the SSM code", 64);
         ssm_wait_ = false;
         consume();
      }
      // SSM : the screenshot is taken "immediately after reading the #HH byte". Here it is the first
      // complete frame after the code : the script goes on once it is taken, or the next key would
      // already change the screen it shows (SHAKER leaves the page ~90 ms after the key)
      if (ok && host_)
         ok = WaitUntil([this]() { return !host_->IsScreenshotPending(); }, ins, "the screenshot of the SSM code", 64);
      return ok;
   }
   if (n == "screenshot" || n == "snapshot")
   {
      if (!a.empty())
      {
         if (Lower(a[0]) != "vsync") return Fail(ins, "unknown option");
         CRTC* crtc = engine_->GetCRTC();
         if (!WaitUntil([crtc]() { return !crtc->VSyncPin(); }, ins, "the VSYNC off")) return false;
         if (!WaitUntil([crtc]() { return crtc->VSyncPin(); }, ins, "the VSYNC on")) return false;
      }
      if (n == "screenshot")
      {
         if (!host_) return Fail(ins, "no display to take a screenshot");
         const std::string name = screenshot_name_.empty() ? SsmScreenshotName(0xFFFE) : screenshot_name_;
         screenshot_name_.clear();
         const fs::path p = ScreenshotPath(name);
         host_->RequestScreenshot(p);
         screenshots_.push_back(p);
      }
      else
      {
         const std::string name = snapshot_name_.empty() ? SsmScreenshotName(0xFFFF) : snapshot_name_;
         snapshot_name_.clear();
         fs::path p = Resolve(snapshot_dir_, name);
         if (!p.has_extension()) p += ".sna";
         std::vector<unsigned char> sna;
         if (!engine_->SaveSnapshotNow(sna)) return Fail(ins, "cannot build the snapshot");
         std::ofstream f(p, std::ios::binary);
         if (!f) return Fail(ins, "cannot write " + p.string());
         f.write((const char*)sna.data(), (std::streamsize)sna.size());
      }
      return true;
   }
   if (n == "csl_load")
   {
      if (!need(1)) return Fail(ins, "missing script name");
      fs::path p = Resolve(fs::path(), a[0]);
      if (!fs::exists(p) && !p.has_extension())
      {
         for (const char* ext : { ".CSL", ".csl" })
         {
            fs::path q = p;
            q += ext;
            if (fs::exists(q)) { p = q; break; }
         }
      }
      if (!fs::exists(p)) return Fail(ins, "script not found : " + p.string());
      if (!follow_csl_load_)
      {
         next_script_ = p;
         return true;
      }
      if (nesting_ >= 16) return Fail(ins, "too many nested csl_load");
      ++nesting_;
      std::ifstream f(p, std::ios::binary);
      std::stringstream ss;
      ss << f.rdbuf();
      const bool ok = RunText(ss.str(), p);
      --nesting_;
      return ok;
   }

   return Fail(ins, "unknown instruction");
}

////////////////////////////////////////////////////////
/// SSM
///
/// The "neutral" ED opcodes of the standard (no effect on a Z80A but time). ED ED is one of them :
/// code #00ED is ED ED ED 00. ED FE and ED FF belong to the reserved codes #FFxx. ED FF
/// is the emulator break otherwise (debugger, BRK of RASM) : a script owns it for its lifetime.
void CslScript::EnableSsm()
{
   auto neutral = [](unsigned int v)
   {
      return v <= 0x3F || (v >= 0x7F && v <= 0x9F) || (v >= 0xA4 && v <= 0xA7) || (v >= 0xAC && v <= 0xAF)
         || (v >= 0xB4 && v <= 0xB7) || (v >= 0xBC && v <= 0xBF) || v >= 0xC0;
   };
   Z80* z80 = engine_->GetProc();
   z80->SetEmuBreakEnabled(false);
   for (unsigned int v = 0; v < 0x100; v++)
   {
      if (neutral(v))
         z80->SetCustomOpcode<Z80::ED>((unsigned char)v, [this](unsigned int opcode) { OnEdOpcode(opcode); });
   }
   ssm_low_valid_ = false;
}

void CslScript::DisableSsm()
{
   engine_->GetProc()->ClearCustom<Z80::ED>();
   engine_->GetProc()->SetEmuBreakEnabled(true);
}

void CslScript::OnEdOpcode(unsigned int opcode)
{
   // Called when the opcode runs : the two halves of a code are two ED opcodes in a row, so the
   // second one starts 2 bytes after the first. Anything else in between breaks the pair.
   const uint16_t pc = engine_->GetProc()->GetPC();
   const uint8_t value = (uint8_t)(opcode & 0xFF);
   if (ssm_low_valid_ && pc == (uint16_t)(ssm_low_pc_ + 2))
   {
      ssm_low_valid_ = false;
      OnSsmCode((uint16_t)((value << 8) | ssm_low_));
   }
   else
   {
      ssm_low_valid_ = true;
      ssm_low_ = value;
      ssm_low_pc_ = pc;
   }
}

void CslScript::OnSsmCode(uint16_t code)
{
   SsmEvent ev{ code, time_us_, fs::path() };
   if (host_)
   {
      char buffer[96];
      snprintf(buffer, sizeof(buffer), "SSM %04X at PC %04X, +%lluus", code, engine_->GetProc()->GetPC(), (unsigned long long)time_us_);
      host_->Log(buffer);
   }

   if (ssm_wait_ && code == ssm_wait_code_)
      ssm_wait_hit_ = true;
   ssm_since_wait_.push_back(code);

   // #0000 synchronises a script, #FFxx are reserved : no screenshot
   if (code != 0x0000 && (code & 0xFF00) != 0xFF00 && host_)
   {
      ev.screenshot = ScreenshotPath(SsmScreenshotName(code));
      host_->RequestScreenshot(ev.screenshot);
      screenshots_.push_back(ev.screenshot);
   }
   ssm_events_.push_back(ev);
}
