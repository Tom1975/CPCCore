
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#ifdef _WIN32
#include <filesystem>
namespace fs = std::filesystem;
#elif __MORPHOS__
// fs does not exist there

#elif __circle__
#include <filesystem>
// fs does not exist there
namespace fs = std::filesystem;
#else
// fs does not exist there
#include <filesystem>
// Only for C++17 !
namespace fs = std::filesystem;
#endif

#include <iostream>

#include "gtest/gtest.h"
#include "MediaContainer.h"
#include "DiskGen.h"
#include "FormatTypeRAW.h"

/////////////////////////////////////////////////////////////
/// Helper functions
bool TestMedia(const char* path_to_test)
{
   DiskGen disk_gen;
   DskTypeManager m_DiskTypeManager;
   disk_gen.Init(&m_DiskTypeManager);
   DataContainer inserted_media(&m_DiskTypeManager);
   inserted_media.Clear();

   fs::path path(path_to_test);
   inserted_media.AddSourceFile(path.generic_string().c_str());

   std::vector<IDisk*> disk_list = disk_gen.CreateDisk(&inserted_media);

   DiskBuilder disk_builder;
   IDisk* d1;
   disk_builder.LoadDisk(path_to_test, d1);
   return (disk_list.size() == 1 && disk_list[0]->CompareToDisk(d1, true) == 0);
}

/////////////////////////////////////////////////////////////
// Test that a regular RAW kryoflux, and the same dump zipped are equal one loaded.
TEST(MediaContainer, test_zipped_kryoflux)
{
   DiskGen disk_gen;
   DskTypeManager m_DiskTypeManager;
   disk_gen.Init(&m_DiskTypeManager);
   DataContainer inserted_media(&m_DiskTypeManager);

   inserted_media.Clear();

   fs::path path("res/After Burner.zip");
   inserted_media.AddSourceFile(path.generic_string().c_str());

   std::vector<IDisk*> disk_list = disk_gen.CreateDisk(&inserted_media);

   DiskBuilder disk_builder;
   IDisk* d1;
   ASSERT_EQ(0, disk_builder.LoadDisk("res/After Burner/track00.0.raw", d1));

   ASSERT_EQ(1, disk_list.size());
   
   ASSERT_EQ(true, (disk_list[0]->CompareToDisk(d1, true)==0));

}

/////////////////////////////////////////////////////////////
// Test a CTRaw is correctly loaded
TEST(MediaContainer, test_media_ctraw)
{
   ASSERT_EQ(true, TestMedia("res/1942.raw"));
}

// Test a DSK is correctly loaded
TEST(MediaContainer, test_media_dsk)
{
   ASSERT_EQ(true, TestMedia("res/DEATHSWORD128K.DSK"));
}

// Test a EDSK is correctly loaded
TEST(MediaContainer, test_media_edsk)
{
   ASSERT_EQ(true, TestMedia("res/Airwolf (1985)(Elite)[cr XOR][t +3 XOR].dsk"));
}

// Test a HFEis correctly loaded
TEST(MediaContainer, test_media_hfe)
{
   ASSERT_EQ(true, TestMedia("res/30YMD double sides 1 and 2.hfe"));
}

// Test a IPF is correctly loaded
TEST(MediaContainer, test_media_ipf)
{
   ASSERT_EQ(true, TestMedia("res/After Burner (UK) (1988) [Activision SEGA] (Pre-release).ipf"));
}

// Test a SCP is correctly loaded
TEST(MediaContainer, test_media_scp)
{
   ASSERT_EQ(true, TestMedia("res/The Demo [A].scp"));
}

// Test a Kryoflux RAW is correctly loaded
TEST(MediaContainer, test_media_kryoflux)
{
   ASSERT_EQ(true, TestMedia("res/After Burner/track00.0.raw"));
}

/////////////////////////////////////////////////////////////
// 
/*
TEST(MediaContainer_Directory, test_directory)
{
   MediaContainer container;

   // Add a simple source file
   // Check that only one file is available
   //ASSERT_EQ(true, CompareDisks(&d1, &d2, "res/necro.sfw"));
   FAIL();
}

TEST(MediaContainer_MultipleDirectory, test_multiple_directories)
{
   MediaContainer container;

   // Add a simple source file
   // Check that only one file is available
   //ASSERT_EQ(true, CompareDisks(&d1, &d2, "res/necro.sfw"));
   FAIL();
}

TEST(MediaContainer_Single_Zipped, test_single_zip)
{
   MediaContainer container;

   // Add a simple source file
   // Check that only one file is available
   //ASSERT_EQ(true, CompareDisks(&d1, &d2, "res/necro.sfw"));
   FAIL();
}

TEST(MediaContainer_Multiple_Zipped, test_multiple_zip)
{
   MediaContainer container;

   // Add a simple source file
   // Check that only one file is available
   //ASSERT_EQ(true, CompareDisks(&d1, &d2, "res/necro.sfw"));
   FAIL();
}
*/