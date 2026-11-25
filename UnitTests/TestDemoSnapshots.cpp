#include "gtest/gtest.h"

#include "CompendiumMachine.h"
#include "TestWorkspace.h"

#include <vector>

// Regression tests on demo snapshots : the snapshot is run cycle by cycle and the frame timing it
// produces is checked against what the real machine gives.

namespace
{
// Runs 'frames' VSYNCs and returns, for each, the 4 MHz cycle of its start and the monitor line at
// that moment.
struct VSync
{
   unsigned long long cycle;
   unsigned int monitor_y;
};

std::vector<VSync> RunVSyncs(compendium::Machine& m, int frames)
{
   std::vector<VSync> out;
   bool previous = m.Crtc().ff4_;
   // A frame of a CPC is 19968 us : leave room for a broken frame twice as long
   const unsigned long long cap = (unsigned long long)frames * 4 * 19968 * 3;
   for (unsigned long long i = 0; i < cap && (int)out.size() < frames; ++i)
   {
      m.Cycle();
      const bool now = m.Crtc().ff4_;
      if (now && !previous)
         out.push_back({ m.Cycles(), m.Engine().GetMonitor()->GetY() });
      previous = now;
   }
   return out;
}
}

// "ovf prev2" (CRTC 0) : the effect writes R0=1 then R0=&31 on every line (RVI) with R9 > 0, and
// starts the VSYNC by writing R7=0 (16.4.1). The 2 us additional line that follows a last line with
// R0=1 must bring C4 and C9 back to 0 whatever R9 (13.2.5, 13.2.7) : otherwise C4 drifts, R7=0 is
// missed every other frame and the picture rolls.
TEST(DemoSnapshot, OvfPrev2IsStableOnCrtc0)
{
   compendium::Machine m(CRTC::HD6845S);
   ASSERT_TRUE(m.Engine().LoadSnapshotNow(TestWorkspace::Fixture("res/SNA/ovf prev2.sna").c_str()));
   ASSERT_EQ(CRTC::HD6845S, m.Crtc().type_crtc_);

   const std::vector<VSync> vsyncs = RunVSyncs(m, 12);
   ASSERT_EQ(12u, vsyncs.size()) << "the VSYNC stopped";

   // The first VSYNC comes after the snapshot counters : the frames are measured from the second one.
   // Every frame has the same length, and the monitor sees each VSYNC at the same line (locked).
   const unsigned long long frame = vsyncs[2].cycle - vsyncs[1].cycle;
   for (size_t i = 2; i < vsyncs.size(); ++i)
   {
      EXPECT_EQ(frame, vsyncs[i].cycle - vsyncs[i - 1].cycle) << "frame " << i;
      EXPECT_EQ(vsyncs[1].monitor_y, vsyncs[i].monitor_y) << "frame " << i << " : the picture rolls";
   }
}
