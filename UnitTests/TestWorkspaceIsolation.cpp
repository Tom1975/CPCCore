#include "gtest/gtest.h"

#include "TestWorkspace.h"

#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace
{
   void Write(const std::string& name, const std::string& content)
   {
      std::ofstream file(name);
      ASSERT_TRUE(file.good()) << "could not write " << name << " in " << fs::current_path().string();
      file << content;
   }

   std::string Read(const std::string& name)
   {
      std::ifstream file(name);
      return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
   }
}

TEST(TestWorkspaceIsolation, ATestRunsInItsOwnDirectory)
{
   EXPECT_EQ(TestWorkspace::CurrentTestDir(), fs::current_path());
   EXPECT_NE(TestWorkspace::CurrentTestDir(), TestWorkspace::FixtureRoot())
      << "a test must not run in the fixture directory";
   EXPECT_NE(std::string::npos,
      TestWorkspace::CurrentTestDir().string().find("TestWorkspaceIsolation.ATestRunsInItsOwnDirectory"))
      << "the directory is named after the test, so a failure can be inspected";
}

// The two tests below write the same file name on purpose: that is what makes
// running the suite in parallel unsafe when every test shares one directory.
TEST(TestWorkspaceIsolation, WritesGoToTheTestDirectory)
{
   Write("shared_name.txt", "written by the first test");

   EXPECT_EQ("written by the first test", Read("shared_name.txt"));
   EXPECT_TRUE(fs::exists(TestWorkspace::CurrentTestDir() / "shared_name.txt"));
   EXPECT_FALSE(fs::exists(TestWorkspace::FixtureRoot() / "shared_name.txt"))
      << "a test must not write into the fixture directory";
}

TEST(TestWorkspaceIsolation, AnotherTestWritingTheSameNameIsNotDisturbed)
{
   EXPECT_FALSE(fs::exists("shared_name.txt")) << "the other test's file must not be visible here";

   Write("shared_name.txt", "written by the second test");
   EXPECT_EQ("written by the second test", Read("shared_name.txt"));
}

TEST(TestWorkspaceIsolation, FixturesAreReachedThroughAnAbsolutePath)
{
   const std::string conf = TestWorkspace::Fixture("./TestConf.ini");

   EXPECT_TRUE(fs::path(conf).is_absolute()) << conf;
   EXPECT_TRUE(fs::exists(conf)) << "fixtures stay reachable from inside a test directory: " << conf;
   EXPECT_TRUE(fs::exists(TestWorkspace::Fixture("ROM"))) << "the engine's ROM directory";
}

TEST(TestWorkspaceIsolation, AnAbsolutePathIsLeftAlone)
{
   const std::string absolute = TestWorkspace::FixtureRoot().string();

   EXPECT_EQ(absolute, TestWorkspace::Fixture(absolute));
}
