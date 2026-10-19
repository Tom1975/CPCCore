#pragma once

#include <filesystem>
#include <string>

// Each test runs in its own working directory, so whatever a test writes stays
// there and can be looked at after the run. The fixtures are not copied: they
// are 347 MB and there are close to 300 tests, so they stay where they are and
// are reached through an absolute path captured before the first test starts.
//
// A test therefore reads through TestWorkspace::Fixture() and writes with a
// plain relative path.
namespace TestWorkspace
{
   // Directory holding ROM/, res/, CONF/, CART/, Keyboards/ and TestConf*.ini.
   // The directory the binary was started in, unless CPCCORE_TEST_FIXTURES says
   // otherwise.
   const std::filesystem::path& FixtureRoot();

   // Absolute path of a fixture. An absolute path is returned unchanged, so it
   // is safe to wrap a path that has already been resolved. Fixtures come from
   // more than one place -- the installed layout has them all next to the
   // binary, while a build tree keeps res/ and ROM/ in UnitTests and the
   // keyboard maps in CPCCoreEmu -- so the roots are searched in order and the
   // first one that has the file wins. A file that is nowhere resolves against
   // the first root, so the error names the path that was expected.
   std::string Fixture(const std::string& relative_path);

   // The directory the binary was started in, whatever a test later does with
   // the current directory. A relative argv[0] resolves against it.
   const std::filesystem::path& StartDirectory();

   // Directory the per-test working directories are created in. Defaults to
   // <fixtures>/test-work, and CPCCORE_TEST_WORK overrides it so a build can
   // keep its outputs out of the source tree.
   const std::filesystem::path& WorkRoot();

   // Working directory of the running test, empty outside a test.
   const std::filesystem::path& CurrentTestDir();
}
