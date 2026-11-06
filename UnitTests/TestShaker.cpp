// SHAKER 2.7 (Longshot / Logon System, https://shaker.logonsystem.eu) run by its own CSL scripts.
//
// Each module (A to E) has one script per CRTC (0 to 4) : the script selects the CRTC, resets,
// runs the module and walks through every test. The module asks a screenshot of each test screen
// with an SSM code (ED <LL> ED <HH>), and the screenshot is compared with the reference of the
// same name in res/Shaker/reference/<module>/<crtc>/.
//
// SHAKER is a survey of the emulation, not a list of tests that must pass. A reference is a
// screenshot validated by hand against what the SHAKER site expects on a real CPC ; a screenshot
// without reference is "new" (to validate) and does not fail the test. Only the validated ones
// that differ, or are no longer produced, fail it. tools/run_shaker.py --accept adds references.
//
// The run is long (about 2 h 35 of emulated time for the 25 scripts), so the tests are skipped
// unless CPCCORE_SHAKER is set (CPCCORE_SHAKER=1). CPCCORE_SHAKER_REFERENCE gives the reference
// directory (default : res/Shaker/reference next to the fixture disk).
// tools/run_shaker.py runs the 25 scripts in parallel and builds the HTML report from the
// shaker_result.json each test writes in its working directory.

#include "gtest/gtest.h"

#include "CslScript.h"
#include "Display.h"
#include "Machine.h"
#include "TestUtils.h"
#include "TestWorkspace.h"

#include <SFML/Graphics/Image.hpp>

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace fs = std::filesystem;

namespace
{
////////////////////////////////////////////////////////
/// Headless display : no window, no OpenGL. It keeps the frame the monitor draws, and is the
/// CSL host : a screenshot asked is the last complete frame when the request comes. SHAKER emits
/// its codes from a table (frames to wait, code) : #01DA to #01DC, or the series #0314 onward, name
/// one precise frame, and this is the one the photos of the real CPC show.
class ShakerDisplay : public IDisplay, public ICslHost
{
public:
   // Same geometry as the test CDisplay : the monitor draws every other line of a 1024 x 624
   // buffer. A screenshot keeps the drawn lines only (1024 x 312).
   static constexpr int kWidth = 1024;
   static constexpr int kLines = 312;

   ShakerDisplay() : buffer_(kWidth * kLines * 2, 0), complete_frame_(buffer_) {}

   // IDisplay
   unsigned int ConvertRGB(unsigned int rgb) override
   {
      // Bytes in memory : R, G, B, A
      return 0xFF000000 | ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | ((rgb & 0xFF0000) >> 16);
   }
   void SetScanlines(int) override {}
   bool AFrameIsReady() override { return true; }
   void Display() override {}
   void Config() override {}
   const char* GetInformations() override { return "Shaker headless display"; }
   int GetWidth() override { return kWidth; }
   int GetHeight() override { return kLines; }
   void SetSize(SizeEnum) override {}
   SizeEnum GetSize() override { return S_STANDARD; }
   void SyncOnFrame(bool) override {}
   void StartSync() override {}
   void WaitVbl() override {}
   int* GetVideoBuffer(int y) override { return &buffer_[(size_t)kWidth * y * 2]; }
   void Reset() override { std::fill(buffer_.begin(), buffer_.end(), 0); }
   void FullScreenToggle() override {}
   void ForceFullScreen(bool) override {}
   void Screenshot(const char*) override {}
   void ScreenshotEveryFrame(int) override {}
   bool IsEveryFrameScreened() override { return false; }
   bool SetSyncWithVbl(int) override { return false; }
   bool IsWaitHandled() override { return false; }
   bool IsDisplayed() override { return false; }
   bool GetBlackScreenInterval() override { return false; }
   void SetBlackScreenInterval(bool) override {}
   void WindowChanged(int, int, int, int) override {}
   void ResetLoadingMedia() override {}
   void SetLoadingMedia() override {}
   void ResetDragnDropDisplay() override {}
   void SetDragnDropDisplay(int) override {}
   void SetCurrentPart(int, int) override {}
   int GetDnDPart() override { return 0; }

   // The frame that ends here is complete
   void VSync(bool) override { complete_frame_ = buffer_; }

   // ICslHost
   void RequestScreenshot(const fs::path& path) override { Save(path); }
   void Log(const std::string& line) override { log_ << line << "\n"; }

   std::string LogText() const { return log_.str(); }
   const std::vector<std::string>& Errors() const { return errors_; }

   static fs::path PngPath(const fs::path& path)
   {
      fs::path p = path;
      p += ".png";
      return p;
   }

protected:
   void Save(const fs::path& path)
   {
      sf::Image image;
      image.create(kWidth, kLines);
      std::vector<sf::Uint8> rgba((size_t)kWidth * kLines * 4);
      for (int y = 0; y < kLines; y++)
         memcpy(&rgba[(size_t)y * kWidth * 4], &complete_frame_[(size_t)kWidth * y * 2], kWidth * 4);
      image.create(kWidth, kLines, rgba.data());
      std::error_code ec;
      fs::create_directories(path.parent_path(), ec);
      if (!image.saveToFile(PngPath(path).string()))
         errors_.push_back("cannot write " + PngPath(path).string());
   }

   std::vector<int> buffer_;
   std::vector<int> complete_frame_;
   std::vector<std::string> errors_;
   std::ostringstream log_;
};

////////////////////////////////////////////////////////
/// Comparison
enum class Verdict { Same, Different, New, Missing, Unreadable };

const char* VerdictName(Verdict v)
{
   switch (v)
   {
   case Verdict::Same: return "same";
   case Verdict::Different: return "different";
   case Verdict::New: return "new";
   case Verdict::Missing: return "missing";
   default: return "unreadable";
   }
}

// Part of a 1024 x 312 screenshot a real monitor shows (CPCCoreEmu/Monitor.cpp, as VISIBLE in
// tools/run_shaker.py) : per line 128 px of back porch, 832 displayable px, 64 px of line sync ;
// 288 displayable lines from line 16. Outside it (retrace), the picture depends on the instant the
// script presses the keys, not on what the CPC shows : it is not compared.
constexpr unsigned int kVisibleX = 128, kVisibleY = 16, kVisibleW = 832, kVisibleH = 288;

// Number of pixels that differ in the visible window, or -1 if an image cannot be read / sizes
// differ. A non-null 'diff' receives an image of the differences (red where the pixels differ).
long ComparePng(const fs::path& a, const fs::path& b, const fs::path& diff)
{
   sf::Image ia, ib;
   if (!ia.loadFromFile(a.string()) || !ib.loadFromFile(b.string())) return -1;
   if (ia.getSize() != ib.getSize()) return -1;
   const unsigned int w = ia.getSize().x, h = ia.getSize().y;
   const sf::Uint8* pa = ia.getPixelsPtr();
   const sf::Uint8* pb = ib.getPixelsPtr();
   long count = 0;
   std::vector<sf::Uint8> out((size_t)w * h * 4);
   for (size_t i = 0; i < (size_t)w * h; i++)
   {
      const unsigned int x = (unsigned int)(i % w), y = (unsigned int)(i / w);
      const bool visible = x >= kVisibleX && x < kVisibleX + kVisibleW && y >= kVisibleY && y < kVisibleY + kVisibleH;
      const bool same = !visible || memcmp(pa + i * 4, pb + i * 4, 3) == 0;
      if (!same) ++count;
      // Faded picture, differences in red
      out[i * 4 + 0] = same ? pa[i * 4 + 0] / 4 : 255;
      out[i * 4 + 1] = same ? pa[i * 4 + 1] / 4 : 0;
      out[i * 4 + 2] = same ? pa[i * 4 + 2] / 4 : 0;
      out[i * 4 + 3] = 255;
   }
   if (count != 0 && !diff.empty())
   {
      sf::Image id;
      id.create(w, h, out.data());
      id.saveToFile(diff.string());
   }
   return count;
}

std::string JsonEscape(const std::string& s)
{
   std::string out;
   for (char c : s)
   {
      switch (c)
      {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': break;
      case '\t': out += "\\t"; break;
      default:
         if ((unsigned char)c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
         else out += c;
      }
   }
   return out;
}

// Generic slash form for the report, whatever the platform
std::string Generic(const fs::path& p) { return p.generic_string(); }

////////////////////////////////////////////////////////
/// The scripts as distributed, and what the fixture changes in them
struct ShakerCase
{
   char module;
   int crtc;
};

std::string CaseName(const ShakerCase& c) { return std::string(1, c.module) + "_" + std::to_string(c.crtc); }

std::string ReadFile(const fs::path& p)
{
   std::ifstream f(p, std::ios::binary);
   std::stringstream ss;
   ss << f.rdbuf();
   return ss.str();
}

// The scripts are run as distributed (CSL 1.5, wait_ssm), with two changes listed in the report :
// - CRTC 3 is the CRTC of the ASIC, the scripts run on a 6128 Plus (SHAKER tells CRTC 3 from 4 by the
//   ASIC). Its cartridge starts on a menu : f1 (keypad) selects BASIC before the disk is inserted.
// - SHAKER reads the matrix : its "Z" key (R9 base) is line 8 bit 7, the key marked W on the AZERTY of
//   the 6128 FR. The key marked Z (line 7 bit 3) is read by nothing, the script would be stuck.
std::string FixScript(std::string text, int crtc, std::vector<std::string>& changes)
{
   if (crtc == 3)
   {
      const size_t pos = text.find("disk_insert");
      if (pos != std::string::npos)
      {
         text.insert(pos, "key_output '\\(FN1)'\nwait 2000000\n");
         changes.push_back("6128 Plus : key_output '\\(FN1)' (cartridge menu, BASIC) before disk_insert");
      }
   }
   const std::string key_z = "key_output 'Z'", key_w = "key_output 'W'";
   size_t count = 0;
   for (size_t pos = text.find(key_z); pos != std::string::npos; pos = text.find(key_z, pos + key_w.size()))
   {
      text.replace(pos, key_z.size(), key_w);
      ++count;
   }
   if (count != 0)
      changes.push_back("AZERTY : key_output 'Z' -> 'W', line 8 bit 7 read by SHAKER (x" + std::to_string(count) + ")");
   return text;
}

class ShakerSuite : public ::testing::TestWithParam<ShakerCase>
{
};

TEST_P(ShakerSuite, MatchesReference)
{
   const char* mode = std::getenv("CPCCORE_SHAKER");
   if (mode == nullptr || *mode == '\0')
      GTEST_SKIP() << "SHAKER run skipped : set CPCCORE_SHAKER=1";

   const ShakerCase c = GetParam();
   const fs::path shaker_dir = fs::path(TestWorkspace::Fixture("res/Shaker/shaker27.dsk")).parent_path();
   const fs::path script = shaker_dir / "CSL" / (std::string("MODULE_") + c.module)
      / (std::string("SHAKER-") + c.module + "-" + std::to_string(c.crtc) + ".CSL");
   ASSERT_TRUE(fs::exists(script)) << script;
   const char* reference_root = std::getenv("CPCCORE_SHAKER_REFERENCE");
   const fs::path reference_dir = (reference_root != nullptr && *reference_root != '\0' ? fs::path(reference_root) : shaker_dir / "reference")
      / std::string(1, c.module) / std::to_string(c.crtc);
   const fs::path out_dir = fs::absolute("screens");

   // Machine : CPC 6128 FR, as the scripts were written for (AMSpiriT, 6128 FR) ; 6128 Plus FR for
   // CRTC 3, the CRTC of its ASIC
   EmulatorEngine machine;
   ShakerDisplay display;
   DirectoriesImp directories;
   Log log;
   SoundFactory sound;
   ConfigurationManager conf;
   machine.SetDirectories(&directories);
   machine.SetLog(&log);
   machine.SetConfigurationManager(&conf);
   machine.Init(&display, &sound);
   machine.GetMem()->Initialisation();
   machine.LoadConfiguration(c.crtc == 3 ? "6128PLUS" : "6128", TestWorkspace::Fixture("TestConf.ini").c_str());
   machine.Reinit();
   machine.SetFixedSpeed(true);
   // As the CPC of the SHAKER photos : no printer, BUSY (PPI port B bit 6) reads 1 (tests AI)
   machine.GetMotherboard()->ConnectPrinter(false);

   CslScript csl(&machine, &display);
   csl.SetDiskDir(shaker_dir);
   csl.SetScreenshotDir(out_dir);
   csl.SetFollowCslLoad(false);              // one script per test : the chain runs in parallel
   // A wait_ssm that never comes : error after 5 min emulated (the CSL norm sets no limit ; the longest
   // SHAKER page, module C test 7, takes about 64 s)
   csl.SetWaitTimeout(300ull * 1000 * 1000);

   std::vector<std::string> changes;
   const std::string text = FixScript(ReadFile(script), c.crtc, changes);

   const auto start = std::chrono::steady_clock::now();
   const bool script_ok = csl.RunText(text, script);
   const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

   {
      std::ofstream f("csl.log", std::ios::binary);
      f << display.LogText();
   }
   // A script that stops is part of the survey (report) : only the validated screenshots it no
   // longer produces fail the test
   if (!script_ok) std::cout << "[ SHAKER ] " << CaseName(c) << " script stopped : " << csl.Error().ToString() << std::endl;
   for (const std::string& e : display.Errors()) ADD_FAILURE() << e;

   // Screenshots : the same SSM code may come twice, the last one is kept
   std::vector<std::string> names;
   std::set<std::string> seen;
   for (const fs::path& p : csl.Screenshots())
   {
      const std::string name = p.filename().string();
      if (seen.insert(name).second) names.push_back(name);
   }
   std::map<std::string, uint16_t> codes;
   for (const SsmEvent& e : csl.SsmEvents())
      if (!e.screenshot.empty()) codes[e.screenshot.filename().string()] = e.code;

   struct Shot { std::string name; Verdict verdict; long pixels; };
   std::vector<Shot> shots;
   for (const std::string& name : names)
   {
      const fs::path mine = ShakerDisplay::PngPath(out_dir / name);
      const fs::path ref = ShakerDisplay::PngPath(reference_dir / name);
      if (!fs::exists(mine))
      {
         shots.push_back({ name, Verdict::Unreadable, -1 });
         continue;
      }
      if (!fs::exists(ref))
      {
         shots.push_back({ name, Verdict::New, -1 });
         continue;
      }
      const long diff = ComparePng(mine, ref, ShakerDisplay::PngPath(out_dir / (name + ".diff")));
      shots.push_back({ name, diff == 0 ? Verdict::Same : (diff < 0 ? Verdict::Unreadable : Verdict::Different), diff });
   }
   // References that this run did not produce
   if (fs::is_directory(reference_dir))
   {
      for (const auto& entry : fs::directory_iterator(reference_dir))
      {
         if (entry.path().extension() != ".png") continue;
         const std::string name = entry.path().stem().string();
         if (seen.count(name) == 0)
         {
            shots.push_back({ name, Verdict::Missing, -1 });
         }
      }
   }

   // Result for the report
   {
      std::ofstream f("shaker_result.json", std::ios::binary);
      f << "{\n";
      f << "  \"case\": \"" << CaseName(c) << "\",\n";
      f << "  \"module\": \"" << c.module << "\",\n";
      f << "  \"crtc\": " << c.crtc << ",\n";
      f << "  \"script\": \"" << JsonEscape(Generic(script)) << "\",\n";
      f << "  \"mode\": \"" << "compare" << "\",\n";
      f << "  \"script_ok\": " << (script_ok ? "true" : "false") << ",\n";
      f << "  \"error\": \"" << JsonEscape(csl.Error().IsSet() ? csl.Error().ToString() : "") << "\",\n";
      f << "  \"next_script\": \"" << JsonEscape(Generic(csl.NextScript())) << "\",\n";
      f << "  \"emulated_seconds\": " << csl.EmulatedMicroseconds() / 1e6 << ",\n";
      f << "  \"real_seconds\": " << seconds << ",\n";
      f << "  \"ssm_codes\": " << csl.SsmEvents().size() << ",\n";
      f << "  \"screens_dir\": \"" << JsonEscape(Generic(out_dir)) << "\",\n";
      f << "  \"reference_dir\": \"" << JsonEscape(Generic(reference_dir)) << "\",\n";
      f << "  \"script_changes\": [";
      for (size_t i = 0; i < changes.size(); i++) f << (i ? ", " : "") << "\"" << JsonEscape(changes[i]) << "\"";
      f << "],\n";
      f << "  \"screenshots\": [\n";
      for (size_t i = 0; i < shots.size(); i++)
      {
         const Shot& s = shots[i];
         char code[8] = "";
         auto it = codes.find(s.name);
         if (it != codes.end()) snprintf(code, sizeof(code), "%04X", it->second);
         f << "    {\"name\": \"" << JsonEscape(s.name) << "\", \"code\": \"" << code << "\", \"verdict\": \""
           << VerdictName(s.verdict) << "\", \"pixels\": " << s.pixels << "}" << (i + 1 < shots.size() ? "," : "") << "\n";
      }
      f << "  ]\n}\n";
   }

   // Verdict
   int bad = 0;
   for (const Shot& s : shots)
   {
      if (s.verdict == Verdict::Same || s.verdict == Verdict::New) continue;   // new : not validated yet
      ++bad;
      ADD_FAILURE() << CaseName(c) << " " << s.name << " : " << VerdictName(s.verdict)
                    << (s.pixels > 0 ? " (" + std::to_string(s.pixels) + " pixels)" : std::string());
   }
   RecordProperty("screenshots", (int)names.size());
   RecordProperty("not_same", bad);
}

std::vector<ShakerCase> AllCases()
{
   std::vector<ShakerCase> cases;
   for (char m : { 'A', 'B', 'C', 'D', 'E' })
      for (int crtc = 0; crtc <= 4; crtc++)
         cases.push_back({ m, crtc });
   return cases;
}

INSTANTIATE_TEST_SUITE_P(Shaker, ShakerSuite, ::testing::ValuesIn(AllCases()),
   [](const ::testing::TestParamInfo<ShakerCase>& info) { return CaseName(info.param); });
}

////////////////////////////////////////////////////////
/// The CSL interpreter itself, on short scripts (always run)
namespace
{
class CslHostStub : public ICslHost
{
public:
   void RequestScreenshot(const fs::path& p) override { requests.push_back(p); }
   std::vector<fs::path> requests;
};

struct CslMachine
{
   CslMachine()
   {
      machine.SetDirectories(&directories);
      machine.SetLog(&log);
      machine.SetConfigurationManager(&conf);
      machine.Init(&display, &sound);
      machine.GetMem()->Initialisation();
      machine.LoadConfiguration("6128", TestWorkspace::Fixture("TestConf.ini").c_str());
      machine.Reinit();
      machine.SetFixedSpeed(true);
   }
   EmulatorEngine machine;
   ShakerDisplay display;
   DirectoriesImp directories;
   Log log;
   SoundFactory sound;
   ConfigurationManager conf;
};

// Writes ED <LL> ED <HH> at 'addr', then a JR $ to stay there
void PokeSsm(EmulatorEngine& m, unsigned short addr, uint16_t code)
{
   const unsigned char bytes[] = { 0xED, (unsigned char)(code & 0xFF), 0xED, (unsigned char)(code >> 8), 0x18, 0xFE };
   for (unsigned int i = 0; i < sizeof(bytes); i++) m.GetMem()->Set(addr + i, bytes[i]);
}
}

TEST(Csl, ParsesQuotedTextsAndComments)
{
   CslMachine m;
   CslHostStub host;
   CslScript csl(&m.machine, &host);
   // An unknown instruction stops the script with its line
   EXPECT_FALSE(csl.RunText("csl_version 1.5 ; comment\n\n  ; only a comment\nkey_delay 100 100\nfoo_bar 'a;b'\n", "t.csl"));
   EXPECT_EQ(5u, csl.Error().line);
   EXPECT_EQ("foo_bar 'a;b'", csl.Error().instruction);
   EXPECT_EQ("1.5", csl.Error().version);
   EXPECT_FALSE(csl.RunText("key_output 'abc\n", "t.csl"));
   EXPECT_EQ("unterminated text", csl.Error().reason);
}

// A text keeps the quotes of the other kind and the escapes; ';' in a text is not a comment
namespace
{
struct CslParser : CslScript
{
   using CslScript::Instruction;
   static std::vector<Instruction> Run(const std::string& text)
   {
      std::vector<Instruction> out;
      std::string error;
      unsigned int line = 0;
      EXPECT_TRUE(Parse(text, out, error, line)) << error;
      return out;
   }
};
}

TEST(Csl, ParsesTextsWithOtherQuotes)
{
   const auto p = CslParser::Run("Key_Output 'RUN\"SHAKE27A\"\\(RET)' ; run\n"
                                 "key_output \"it's;\" \n"
                                 "key_output 'a\\(')b'\n"
                                 "keyboard_write 255,255, 254 ,255,255,255,255,255,255,255\n");
   ASSERT_EQ(4u, p.size());
   EXPECT_EQ("key_output", p[0].name);
   ASSERT_EQ(1u, p[0].args.size());
   EXPECT_EQ("RUN\"SHAKE27A\"\\(RET)", p[0].args[0]);
   EXPECT_EQ("it's;", p[1].args[0]);
   EXPECT_EQ("a\\(')b", p[2].args[0]);
   ASSERT_EQ(10u, p[3].args.size());
   EXPECT_EQ("254", p[3].args[2]);
}

// key_output types a digit with the key of the main keyboard : on the AZERTY of the 6128 FR, SHIFT +
// the digit key. The keypad types the digits without SHIFT, and f4 is on line 2 (the other keypad
// keys on lines 0 and 1) : '4' gave f4, and the SHAKER menus, which read the main key, ignored it.
TEST(Csl, DigitsAreTypedOnTheMainKeyboard)
{
   CslMachine m;
   const KeyboardHandler* kb = m.machine.GetKeyboardHandler();
   // CPC matrix : '1' 8.0, '2' 8.1, '3' 7.1, '4' 7.0, '5' 6.1, '6' 6.0, '7' 5.1, '8' 5.0, '9' 4.1, '0' 4.0
   const unsigned int expected[10][2] = { {4, 0}, {8, 0}, {8, 1}, {7, 1}, {7, 0}, {6, 1}, {6, 0}, {5, 1}, {5, 0}, {4, 1} };
   for (int d = 0; d < 10; d++)
   {
      unsigned int line = 0, bit = 0;
      bool shift = false;
      ASSERT_TRUE(kb->FindCharKey((char)('0' + d), line, bit, shift)) << d;
      EXPECT_EQ(expected[d][0], line) << d;
      EXPECT_EQ(expected[d][1], bit) << d;
      EXPECT_TRUE(shift) << d;
   }
}

// SHAKER reads the "Z" of its menu at line 8 bit 7 : the key marked W on the 6128 FR
TEST(Csl, ShakerZKeyIsTheFrenchW)
{
   CslMachine m;
   const KeyboardHandler* kb = m.machine.GetKeyboardHandler();
   unsigned int line = 0, bit = 0;
   bool shift = false;
   ASSERT_TRUE(kb->FindCharKey('W', line, bit, shift));
   EXPECT_EQ(8u, line);
   EXPECT_EQ(7u, bit);
   std::vector<std::string> changes;
   EXPECT_EQ("key_output 'W'\t; base\nkey_output 'X'\nkey_output 'W'\n",
      FixScript("key_output 'Z'\t; base\nkey_output 'X'\nkey_output 'Z'\n", 1, changes));
   EXPECT_EQ(1u, changes.size());
}

TEST(Csl, WaitRunsEmulatedTime)
{
   CslMachine m;
   CslHostStub host;
   CslScript csl(&m.machine, &host);
   ASSERT_TRUE(csl.RunText("wait 100000\nwait 23\n", "t.csl")) << csl.Error().ToString();
   EXPECT_EQ(100023u, csl.EmulatedMicroseconds());
}

TEST(Csl, CrtcSelectSurvivesPowerOn)
{
   CslMachine m;
   CslHostStub host;
   CslScript csl(&m.machine, &host);
   for (int crtc = 0; crtc <= 4; crtc++)
   {
      ASSERT_TRUE(csl.RunText("crtc_select " + std::to_string(crtc) + "\nreset\nwait 1000\n", "t.csl"));
      EXPECT_EQ(crtc, (int)m.machine.GetCRTC()->type_crtc_);
   }
   ASSERT_TRUE(csl.RunText("crtc_select 1B\nreset\n", "t.csl"));
   EXPECT_EQ(CRTC::UM6845R, m.machine.GetCRTC()->type_crtc_);
   EXPECT_FALSE(csl.RunText("crtc_select 5\n", "t.csl"));
   EXPECT_EQ("CRTC not handled by the emulator", csl.Error().reason);
}

// SSM 1.2 : ED <LL> ED <HH>, code HHLL, screenshot <emulator>_<crtc>_<HHLL>
TEST(Csl, SsmCodeAsksANamedScreenshot)
{
   CslMachine m;
   CslHostStub host;
   CslScript csl(&m.machine, &host);
   csl.SetScreenshotDir("/out");
   ASSERT_TRUE(csl.RunText("crtc_select 2\nreset\nwait 3000000\n", "t.csl"));
   PokeSsm(m.machine, 0x4000, 0x02E3);
   m.machine.GetProc()->PrepareForFetch(0x4000);   // the firmware waits for a key : jump to the code
   ASSERT_TRUE(csl.RunText("wait 100\n", "t.csl"));
   ASSERT_EQ(1u, csl.SsmEvents().size());
   EXPECT_EQ(0x02E3, csl.SsmEvents()[0].code);
   ASSERT_EQ(1u, host.requests.size());
   EXPECT_EQ("SUGARBOX_2_02E3", host.requests[0].filename().string());
}

// #0000 takes no screenshot and ends a wait_ssm0000
TEST(Csl, WaitSsm0000EndsOnTheCode)
{
   CslMachine m;
   CslHostStub host;
   CslScript csl(&m.machine, &host);
   csl.SetWaitTimeout(1000000);
   ASSERT_TRUE(csl.RunText("reset\nwait 3000000\n", "t.csl"));
   PokeSsm(m.machine, 0x4000, 0x0000);
   m.machine.GetProc()->PrepareForFetch(0x4000);
   ASSERT_TRUE(csl.RunText("wait_ssm0000\n", "t.csl")) << csl.Error().ToString();
   EXPECT_TRUE(host.requests.empty());
   // No code : the wait ends on the timeout
   EXPECT_FALSE(csl.RunText("wait_ssm 0x1234\n", "t.csl"));
}

// Another opcode between the two halves breaks the code
TEST(Csl, SsmHalvesMustFollowEachOther)
{
   CslMachine m;
   CslHostStub host;
   CslScript csl(&m.machine, &host);
   ASSERT_TRUE(csl.RunText("reset\nwait 3000000\n", "t.csl"));
   // ED 3F 00 ED 3E ED 3D : the code is 3D3E (SSM standard example)
   const unsigned char bytes[] = { 0xED, 0x3F, 0x00, 0xED, 0x3E, 0xED, 0x3D, 0x18, 0xFE };
   for (unsigned int i = 0; i < sizeof(bytes); i++) m.machine.GetMem()->Set(0x4000 + i, bytes[i]);
   m.machine.GetProc()->PrepareForFetch(0x4000);
   ASSERT_TRUE(csl.RunText("wait 100\n", "t.csl"));
   ASSERT_EQ(1u, csl.SsmEvents().size());
   EXPECT_EQ(0x3D3E, csl.SsmEvents()[0].code);
}

// Three codes in a row, the script waits for them in turn : ending the wait of the first one must
// not lose the next ones (module B, #01DA #01DB #01DC one frame apart)
TEST(Csl, WaitSsmKeepsTheCodesThatFollow)
{
   CslMachine m;
   CslHostStub host;
   CslScript csl(&m.machine, &host);
   csl.SetWaitTimeout(100000);
   ASSERT_TRUE(csl.RunText("reset\nwait 3000000\n", "t.csl"));
   const unsigned char bytes[] = { 0xED, 0xDA, 0xED, 0x01, 0xED, 0xDB, 0xED, 0x01, 0xED, 0xDC, 0xED, 0x01, 0x18, 0xFE };
   for (unsigned int i = 0; i < sizeof(bytes); i++) m.machine.GetMem()->Set(0x4000 + i, bytes[i]);
   m.machine.GetProc()->PrepareForFetch(0x4000);
   EXPECT_TRUE(csl.RunText("wait 100\nwait_ssm 0x01DA\nwait_ssm 0x01DB\nwait_ssm 0x01DC\n", "t.csl")) << csl.Error().ToString();
   EXPECT_EQ(3u, csl.SsmEvents().size());
}

// ED is a byte of the standard like any other : #00ED is ED ED ED 00, #ED01 is ED 01 ED ED
TEST(Csl, SsmCodesCanHoldTheByteEd)
{
   for (uint16_t code : { (uint16_t)0x00ED, (uint16_t)0xED01 })
   {
      CslMachine m;
      CslHostStub host;
      CslScript csl(&m.machine, &host);
      ASSERT_TRUE(csl.RunText("reset\nwait 3000000\n", "t.csl"));
      PokeSsm(m.machine, 0x4000, code);
      m.machine.GetProc()->PrepareForFetch(0x4000);
      ASSERT_TRUE(csl.RunText("wait 100\n", "t.csl"));
      ASSERT_EQ(1u, csl.SsmEvents().size());
      EXPECT_EQ(code, csl.SsmEvents()[0].code);
   }
}

// ED FF : SSM reads it as the high byte of #FFxx while a script runs, it stays the emulator break
// outside (the debugger, BRK of RASM)
TEST(Csl, EdFfBelongsToSsmOnlyWhileAScriptRuns)
{
   CslMachine m;
   const unsigned char bytes[] = { 0xED, 0xFE, 0xED, 0xFF, 0x18, 0xFE };
   {
      CslHostStub host;
      CslScript csl(&m.machine, &host);
      ASSERT_TRUE(csl.RunText("reset\nwait 3000000\n", "t.csl"));
      for (unsigned int i = 0; i < sizeof(bytes); i++) m.machine.GetMem()->Set(0x4000 + i, bytes[i]);
      m.machine.GetProc()->PrepareForFetch(0x4000);
      ASSERT_TRUE(csl.RunText("wait 100\n", "t.csl"));
      ASSERT_EQ(1u, csl.SsmEvents().size());
      EXPECT_EQ(0xFFFE, csl.SsmEvents()[0].code);
      EXPECT_TRUE(m.machine.GetMotherboard()->run_);   // the run loop goes on
   }
   m.machine.GetProc()->PrepareForFetch(0x4000);
   m.machine.RunCycles(100 * 16);
   EXPECT_FALSE(m.machine.GetMotherboard()->run_);  // the break stopped the run loop
}

// A screenshot asked is the frame completed before the request, not the one being drawn
TEST(Csl, ScreenshotIsTheLastCompleteFrame)
{
   ShakerDisplay display;
   const int kComplete = 0xFF112233;
   *display.GetVideoBuffer(0) = kComplete;
   display.VSync(true);
   *display.GetVideoBuffer(0) = (int)0xFF445566;

   const fs::path path = fs::absolute("last_complete_frame");
   display.RequestScreenshot(path);
   sf::Image image;
   ASSERT_TRUE(image.loadFromFile(ShakerDisplay::PngPath(path).string()));
   const sf::Color pixel = image.getPixel(0, 0);
   // Bytes in memory : R, G, B, A
   EXPECT_EQ(0x33, pixel.r);
   EXPECT_EQ(0x22, pixel.g);
   EXPECT_EQ(0x11, pixel.b);
}

// csl_load without following the chain ends the script and names the next one
TEST(Csl, CslLoadCanStopTheChain)
{
   CslMachine m;
   CslHostStub host;
   CslScript csl(&m.machine, &host);
   csl.SetFollowCslLoad(false);
   const fs::path script = TestWorkspace::Fixture("res/Shaker/CSL/MODULE_E/SHAKER-E-0.CSL");
   ASSERT_TRUE(csl.RunText("csl_load 'SHAKER-E-1'\nwait 1000000\n", script)) << csl.Error().ToString();
   EXPECT_EQ("SHAKER-E-1.CSL", csl.NextScript().filename().string());
   EXPECT_EQ(0u, csl.EmulatedMicroseconds());
}

// Runs any CSL script headless : CPCCORE_CSL_SCRIPT=<file.csl>. The screenshots and the log are
// written in the working directory of the test. A tool more than a test : skipped without it.
TEST(Csl, RunScriptFromEnvironment)
{
   const char* file = std::getenv("CPCCORE_CSL_SCRIPT");
   if (file == nullptr || *file == '\0') GTEST_SKIP() << "set CPCCORE_CSL_SCRIPT=<file.csl>";
   CslMachine m;
   CslScript csl(&m.machine, &m.display);
   csl.SetDiskDir(fs::path(TestWorkspace::Fixture("res/Shaker/shaker27.dsk")).parent_path());
   csl.SetScreenshotDir(fs::absolute("screens"));
   csl.SetWaitTimeout(300ull * 1000 * 1000);
   std::error_code ec;
   const bool ok = csl.RunFile(fs::absolute(fs::path(file), ec));
   std::ofstream("csl.log", std::ios::binary) << m.display.LogText();
   EXPECT_TRUE(ok) << csl.Error().ToString();
}
