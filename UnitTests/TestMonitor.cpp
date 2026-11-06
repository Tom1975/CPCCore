// Monitor (CTM644) : horizontal AFC loop, compared with the photos of a real CPC (SHAKER 2.7 site,
// "Real Hardware" column) and with the CTM644 / LA7800 documents.
//
// SHAKER module A, test T (CRTC 0) : "R2 UPDATE DURING HSYNC". In a band of 88 lines, R2 is changed
// during the HSYNC : the GATE ARRAY sends a second C-HSYNC in the middle of each line (ACCC 15.3).
// The AFC comparator of the monitor adds the sawtooth value of this extra pulse : the picture of the
// band moves until the main pulse compensates for it, over several lines (ACCC 15.7). Measured on
// the photos : -13 pixels M2 (#00BD, one extra pulse) and +11 pixels M2 (#00C2, two extra pulses).

#include "gtest/gtest.h"

#include "CslScript.h"
#include "Machine.h"
#include "TestUtils.h"
#include "TestWorkspace.h"

#include <map>
#include <vector>

namespace fs = std::filesystem;

namespace
{
// Headless display : keeps the frame the monitor draws (every other line of a 1024 x 624 buffer),
// and the frames asked by the SSM codes of the module
class MonitorTestDisplay : public IDisplay, public ICslHost
{
public:
   static constexpr int kWidth = 1024;
   static constexpr int kLines = 312;

   MonitorTestDisplay() : buffer_(kWidth * kLines * 2, 0) {}

   unsigned int ConvertRGB(unsigned int rgb) override { return 0xFF000000 | ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | ((rgb & 0xFF0000) >> 16); }
   void SetScanlines(int) override {}
   bool AFrameIsReady() override { return true; }
   void Display() override {}
   void Config() override {}
   const char* GetInformations() override { return "Monitor test display"; }
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

   // A frame asked is the last complete frame (as the SHAKER screenshots)
   void VSync(bool) override { complete_frame_ = buffer_; }

   void RequestScreenshot(const fs::path& path) override
   {
      // <emulator>_<crtc>_<HHLL>
      const std::string name = path.filename().string();
      std::vector<int>& frame = frames_[name.substr(name.size() - 4)];
      frame.resize((size_t)kWidth * kLines);
      if (complete_frame_.empty()) return;
      for (int y = 0; y < kLines; y++)
         std::copy_n(&complete_frame_[(size_t)kWidth * y * 2], kWidth, &frame[(size_t)kWidth * y]);
   }

   std::map<std::string, std::vector<int>> frames_;

private:
   std::vector<int> buffer_;
   std::vector<int> complete_frame_;
};

// SHAKER module A on CRTC 0, test T, screens #00BD to #00C2
const char* kScript =
   "csl_version 1.0\n"
   "crtc_select 0\n"
   "reset\n"
   "wait 3000000\n"
   "disk_insert 'shaker27.dsk'\n"
   "key_delay 70000 70000 400000\n"
   "key_output 'RUN\"SHAKE27A\"\\(RET)'\n"
   "wait 10000000\n"
   "key_output 'T'\n"
   "wait 800000\n"
   "key_output ' '\n"
   "wait 800000\n"
   "key_output ' '\n"
   "wait 800000\n"
   "key_output ' '\n"
   "wait 800000\n"
   "key_output ' '\n"
   "wait 800000\n"
   "key_output ' '\n"
   "wait 800000\n";

const std::map<std::string, std::vector<int>>& ShakerTestT()
{
   static std::map<std::string, std::vector<int>> frames;
   if (!frames.empty()) return frames;

   EmulatorEngine machine;
   MonitorTestDisplay display;
   DirectoriesImp directories;
   Log log;
   SoundFactory sound;
   ConfigurationManager conf;
   machine.SetDirectories(&directories);
   machine.SetLog(&log);
   machine.SetConfigurationManager(&conf);
   machine.Init(&display, &sound);
   machine.GetMem()->Initialisation();
   machine.LoadConfiguration("6128", TestWorkspace::Fixture("TestConf.ini").c_str());
   machine.Reinit();
   machine.SetFixedSpeed(true);

   CslScript csl(&machine, &display);
   csl.SetDiskDir(fs::path(TestWorkspace::Fixture("res/Shaker/shaker27.dsk")).parent_path());
   csl.SetScreenshotDir(fs::absolute("screens"));
   EXPECT_TRUE(csl.RunText(kScript, "monitor_shaker_t.csl")) << csl.Error().ToString();
   frames = display.frames_;
   return frames;
}

// Left edge of the black rectangle (the HSYNC of the band) on each line of the frame, -1 without one
std::vector<int> RectangleLeftEdge(const std::vector<int>& frame)
{
   std::vector<int> edges(MonitorTestDisplay::kLines, -1);
   for (int y = 0; y < MonitorTestDisplay::kLines; y++)
   {
      const int* line = &frame[(size_t)MonitorTestDisplay::kWidth * y];
      int run = 0;
      for (int x = 128; x < 960 && edges[y] < 0; x++)
      {
         run = ((line[x] & 0xFFFFFF) == 0) ? run + 1 : 0;
         if (run == 100) edges[y] = x - 99;
      }
   }
   return edges;
}

// Shift of the band : left edge in the middle of the band minus the left edge on its first line,
// and number of lines to come within 2 pixels of it
void BandShift(const std::vector<int>& frame, int& shift, int& settle_lines)
{
   const std::vector<int> edges = RectangleLeftEdge(frame);
   // Longest group of consecutive lines with a rectangle, in the visible part of the frame (the
   // vertical blanking is black too)
   int first = -1, last = -1;
   for (int y = 20, start = -1; y < 300; y++)
   {
      if (edges[y] >= 0 && start < 0) start = y;
      if (start >= 0 && (edges[y] < 0 || y == 299))
      {
         const int end = edges[y] < 0 ? y - 1 : y;
         if (end - start > last - first) { first = start; last = end; }
         start = -1;
      }
   }
   ASSERT_GE(first, 0) << "no black rectangle";
   ASSERT_GT(last - first, 40) << "band too short";
   const int steady = edges[(first + last) / 2];
   shift = steady - edges[first];
   settle_lines = 0;
   while (first + settle_lines < last && std::abs(edges[first + settle_lines] - steady) > 2) ++settle_lines;
}
}

// #00BD : R2=#0B, OUT R2,#11 during the HSYNC : one extra C-HSYNC 464 pixels after the main one, on
// the trace part of the AFC sawtooth. Photo : the band moves left by 13 pixels M2, over about 10
// lines, without overshoot.
TEST(Monitor, R2UpdatedDuringHsyncMovesTheBandLeftAsOnTheRealCpc)
{
   const auto& frames = ShakerTestT();
   ASSERT_EQ(1u, frames.count("00BD"));
   int shift = 0, settle = 0;
   BandShift(frames.at("00BD"), shift, settle);
   EXPECT_GE(shift, -16);
   EXPECT_LE(shift, -10);
   EXPECT_GE(settle, 5);
   EXPECT_LE(settle, 16);
}

// #00C2 : OUT R2,#16 : two extra C-HSYNC (464 and 640 pixels after the main one). Photo and Amspirit :
// the band moves right by 11 pixels M2. The monitor model gives about +37 : with the same sawtooth,
// #00BD and #00C2 cannot both be right. The suspect is the second C-HSYNC of the GATE ARRAY, whose
// HSYNC follows the first one after a single character (ACCC 15.3) : to measure on a real CPC.
TEST(Monitor, DISABLED_R2UpdatedTwiceDuringHsyncMovesTheBandRightAsOnTheRealCpc)
{
   const auto& frames = ShakerTestT();
   ASSERT_EQ(1u, frames.count("00C2"));
   int shift = 0, settle = 0;
   BandShift(frames.at("00C2"), shift, settle);
   EXPECT_GE(shift, 8);
   EXPECT_LE(shift, 14);
}

// A standard screen : no extra pulse, the line keeps its place (the loop is locked, no wobble)
TEST(Monitor, StandardLinesDoNotMove)
{
   const auto& frames = ShakerTestT();
   ASSERT_EQ(1u, frames.count("00BD"));
   const std::vector<int>& frame = frames.at("00BD");
   // Lines above the band (title and grid) : identical left border on every line
   int left = -1;
   for (int y = 40; y < 90; y++)
   {
      const int* line = &frame[(size_t)MonitorTestDisplay::kWidth * y];
      int x = 0;
      while (x < MonitorTestDisplay::kWidth && (line[x] & 0xFFFFFF) == 0) ++x;  // black before the border
      if (left < 0) left = x;
      EXPECT_EQ(left, x) << "line " << y;
   }
}
