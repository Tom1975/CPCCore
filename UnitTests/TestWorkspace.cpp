#include "TestWorkspace.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
   fs::path start_directory;
   fs::path fixture_root;
   std::vector<fs::path> extra_roots;
   fs::path work_root;
   fs::path current_test_dir;

   // Test names reach the file system, and a parameterised test carries '/'.
   std::string Sanitise(const std::string& name)
   {
      std::string out(name);
      std::replace_if(out.begin(), out.end(),
         [](unsigned char c) { return std::isalnum(c) == 0 && c != '.' && c != '_' && c != '-'; },
         '_');
      return out;
   }

   class WorkingDirectoryPerTest : public ::testing::EmptyTestEventListener
   {
      void OnTestStart(const ::testing::TestInfo& test_info) override
      {
         const fs::path dir = TestWorkspace::WorkRoot() /
            (Sanitise(test_info.test_suite_name()) + "." + Sanitise(test_info.name()));

         if (std::getenv("CPCCORE_TEST_NO_CHDIR") != nullptr) return;   // debugging escape hatch

         std::error_code error;
         // A previous run's output would be misleading when reading the results.
         fs::remove_all(dir, error);
         fs::create_directories(dir, error);
         current_test_dir = dir;
         fs::current_path(dir, error);
      }

      void OnTestEnd(const ::testing::TestInfo&) override
      {
         if (std::getenv("CPCCORE_TEST_NO_CHDIR") != nullptr) return;

         std::error_code error;
         fs::current_path(TestWorkspace::FixtureRoot(), error);

         // Keep only the directories a test actually wrote something into.
         if (!current_test_dir.empty() && fs::is_empty(current_test_dir, error) && !error)
         {
            fs::remove(current_test_dir, error);
         }
         current_test_dir.clear();
      }
   };

   // Static initialisation: this runs before main(), so the root is the
   // directory the binary was started in, before any test moves away from it.
   struct Installer
   {
      Installer()
      {
         std::error_code error;
         start_directory = fs::current_path(error);

         // CTest's ENVIRONMENT property takes key/value pairs, so several
         // variables cannot be set through gtest_discover_tests without the
         // second being read as a property name. One variable carries them all:
         // "FIXTURES=...|EXTRA=...|WORK=...". The individual variables below
         // still work, for a run by hand and for the child process test.
         if (const char* const packed = std::getenv("CPCCORE_TEST_PATHS"))
         {
            std::string list(packed);
            size_t start = 0;
            while (start <= list.size())
            {
               const size_t end = list.find('|', start);
               const std::string one = list.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
               const size_t equals = one.find('=');
               if (equals != std::string::npos)
               {
                  const std::string key = one.substr(0, equals);
                  const std::string value = one.substr(equals + 1);
                  if (!value.empty())
                  {
#ifdef _WIN32
                     _putenv_s(("CPCCORE_TEST_" + key).c_str(), value.c_str());
#else
                     setenv(("CPCCORE_TEST_" + key).c_str(), value.c_str(), 0);
#endif
                  }
               }
               if (end == std::string::npos) break;
               start = end + 1;
            }
         }

         // Canonical throughout: a test compares these against current_path(),
         // which is canonical, so "a/../b" spellings must not survive.
         const char* const from_environment = std::getenv("CPCCORE_TEST_FIXTURES");
         fixture_root = (from_environment != nullptr)
            ? fs::weakly_canonical(fs::absolute(from_environment, error), error)
            : start_directory;

         // Additional read-only roots, separated the way the platform separates
         // path lists.
         if (const char* const extra = std::getenv("CPCCORE_TEST_FIXTURES_EXTRA"))
         {
#ifdef _WIN32
            const char separator = ';';
#else
            const char separator = ':';
#endif
            std::string list(extra);
            size_t start = 0;
            while (start <= list.size())
            {
               const size_t end = list.find(separator, start);
               const std::string one = list.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
               if (!one.empty())
               {
                  extra_roots.push_back(fs::weakly_canonical(fs::absolute(one, error), error));
               }
               if (end == std::string::npos) break;
               start = end + 1;
            }
         }

         const char* const work = std::getenv("CPCCORE_TEST_WORK");
         work_root = (work != nullptr)
            ? fs::weakly_canonical(fs::absolute(work, error), error)
            : fixture_root / "test-work";

         ::testing::UnitTest::GetInstance()->listeners().Append(new WorkingDirectoryPerTest);
      }
   };

   Installer installer;
}

const fs::path& TestWorkspace::FixtureRoot()
{
   return fixture_root;
}

std::string TestWorkspace::Fixture(const std::string& relative_path)
{
   const fs::path given(relative_path);
   if (given.is_absolute())
   {
      return relative_path;
   }

   const fs::path first = (fixture_root / given).lexically_normal();
   std::error_code error;
   if (fs::exists(first, error))
   {
      return first.string();
   }

   for (size_t i = 0; i < extra_roots.size(); ++i)
   {
      const fs::path candidate = (extra_roots[i] / given).lexically_normal();
      if (fs::exists(candidate, error))
      {
         return candidate.string();
      }
   }

   return first.string();
}

const fs::path& TestWorkspace::StartDirectory()
{
   return start_directory;
}

const fs::path& TestWorkspace::WorkRoot()
{
   return work_root;
}

const fs::path& TestWorkspace::CurrentTestDir()
{
   return current_test_dir;
}
