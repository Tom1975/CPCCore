
#ifdef _WIN32

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#if defined (__unix) || (__MORPHOS__) || (__APPLE__) || (RASPPI)
#ifdef MINIMUM_DEPENDENCIES
#else
#define fopen_s(pFile,filename,mode) (((*(pFile))=fopen((filename), (mode))) == NULL)
#include <sys/stat.h>
#endif
#endif

#include <iostream>
#include <stdio.h>
#include "gtest/gtest.h"

#include "TestWorkspace.h"
#include "DiskContainer.h"
#include "FormatTypeRAW.h"
#include "Tape.h"

#include "Display.h"
#include "TestUtils.h"
#include "Cartridge.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LOADING TEST : These test check that loading will give same result from a file and from a buffer
///
/// - For a disk (with every supported format)
/// - For a tape (with every supported format)


/////////////////////////////////////////////////////////////
// Check that LoadDiskFromBuffer have the same generated disk than LoadDisk

TEST(DiskFormat, Kryoflux)
{
   DiskBuilder disk_builder;
   IDisk * d1;
   IDisk * d2;

   ASSERT_EQ(0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/After Burner/track00.0.raw").c_str(), d1));

   char name[256];
   d1->side_[0].nb_tracks = 42; // HACK : Set number of tracks to 42 (not default of 43)

   std::vector<FormatType::TrackItem> buffer_list;
   for (int i = 0; i < 42; i++)
   {
      FILE* f;
      sprintf(name, "res/After Burner/track%2.2d.0.raw", i);
      const std::string track_path = TestWorkspace::Fixture(name);

      if (fopen_s(&f, track_path.c_str(), "rb") == 0)
      {
         FormatType::TrackItem item;
         fseek(f, 0, SEEK_END);
         item.size = ftell(f);
         rewind(f);
         unsigned char* buffer = new unsigned char[item.size];

         fread(buffer, item.size, 1, f);
         item.buffer = buffer;
         item.path = name;
         buffer_list.push_back(item);
         fclose(f);
      }
   }

   ASSERT_EQ(0, disk_builder.LoadDisk(buffer_list, d2));

   ASSERT_EQ(true, (d1->CompareToDisk(d2, true) == 0));
   delete d1;
   delete d2;
}

/////////////////////////////////////////////////////////////
// Disk conversions 
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// False detection test
TEST(DiskConversion, FalseDetection)
{
   DiskBuilder disk_builder;
   IDisk * d1;
   IDisk * d2;

   ASSERT_EQ (0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/DEATHSWORD128K.DSK").c_str(), d1));
   ASSERT_EQ(0, disk_builder.SaveDisk("FalseDetection.IPF", d1, "IPF"));   
   ASSERT_EQ(0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/Discology60PlusA1 [MAXIT][SAMdisk36B19][Original].dsk").c_str(), d2));
   ASSERT_EQ(false, (d1->CompareToDisk(d2, false) == 0));
   delete d1;
   delete d2;

}
/////////////////////////////////////////////////////////////
// IPF
TEST(DiskConversion, DSK2IPF)
{
   DiskBuilder disk_builder;
   IDisk * d1, *d2;
   FormatType *type;

   ASSERT_EQ(true, disk_builder.CanLoad(TestWorkspace::Fixture("res/DEATHSWORD128K.DSK").c_str(), type));
   ASSERT_EQ(0, strcmp(type->GetFormatName(), "DSK"));
   ASSERT_EQ(0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/DEATHSWORD128K.DSK").c_str(), d1));
   ASSERT_EQ(0, disk_builder.SaveDisk("DSK2IPF.IPF", d1, "IPF"));
   ASSERT_EQ(0, disk_builder.LoadDisk("DSK2IPF.IPF", d2));
   ASSERT_EQ(true, (d1->CompareToDisk(d2, false) == 0));
   delete d1;
   delete d2;
}

TEST(DiskConversion, EDSK2EDSK)
{
   DiskBuilder disk_builder;
   IDisk * d1, *d2;

   ASSERT_EQ(0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/Airwolf (1985)(Elite)[cr XOR][t +3 XOR].dsk").c_str(), d1));
   ASSERT_EQ(0, disk_builder.SaveDisk("EDSK2EDSK.DSK", d1, "EDSK"));
   ASSERT_EQ(0, disk_builder.LoadDisk("EDSK2EDSK.DSK", d2));
   ASSERT_EQ(true, (d1->CompareToDisk(d2, false) == 0));
   delete d1;
   delete d2;
}

TEST(DiskConversion, EDSK2IPF)
{
   DiskBuilder disk_builder;
   IDisk * d1, *d2;

   ASSERT_EQ(0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/Discology60PlusA1 [MAXIT][SAMdisk36B19][Original].dsk").c_str(), d1));
   ASSERT_EQ(0, disk_builder.SaveDisk("EDSK2IPF.IPF", d1, "IPF"));
   ASSERT_EQ(0, disk_builder.LoadDisk("EDSK2IPF.IPF", d2));
   ASSERT_EQ(true, (d1->CompareToDisk(d2, false) == 0));

   delete d1;
   delete d2;
}

TEST(DiskConversion, HFE2IPF)
{
   DiskBuilder disk_builder;
   IDisk * d1, *d2;

   ASSERT_EQ(0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/30YMD double sides 1 and 2.hfe").c_str(), d1));
   ASSERT_EQ(0, disk_builder.SaveDisk("HFE2IPF.IPF", d1, "IPF"));
   ASSERT_EQ(0, disk_builder.LoadDisk("HFE2IPF.IPF", d2));
   ASSERT_EQ(true, (d1->CompareToDisk(d2, false) == 0));
   delete d1;
   delete d2;
}

TEST(DiskConversion, IPF2IPF)
{
   DiskBuilder disk_builder;
   IDisk * d1, *d2;

   ASSERT_EQ(0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/After Burner (UK) (1988) [Activision SEGA] (Pre-release).ipf").c_str(), d1));
   ASSERT_EQ(0, disk_builder.SaveDisk("IPF2IPF.IPF", d1, "IPF"));
   ASSERT_EQ(0, disk_builder.LoadDisk("IPF2IPF.IPF", d2));
   ASSERT_EQ(true, (d1->CompareToDisk(d2, false) == 0));
   delete d1;
   delete d2;
}

TEST(DiskConversion, Kryoflux2IPF)
{
   DiskBuilder disk_builder;
   IDisk *d1, *d2;

   ASSERT_EQ(0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/After Burner/track00.0.raw").c_str(), d1));
   ASSERT_EQ(0, disk_builder.SaveDisk("Kryoflux2IPF.IPF", d1, "IPF"));
   ASSERT_EQ(0, disk_builder.LoadDisk("Kryoflux2IPF.IPF", d2));
   ASSERT_EQ(true, (d1->CompareToDisk(d2, false) == 0));
   delete d2;
}

TEST(DiskConversion, CTRAW2IPF)
{
   DiskBuilder disk_builder;
   IDisk * d1, *d2;

   ASSERT_EQ(0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/1942.raw").c_str(), d1));
   ASSERT_EQ(0, disk_builder.SaveDisk("CTRAW2IPF.IPF", d1, "IPF"));
   ASSERT_EQ(0, disk_builder.LoadDisk("CTRAW2IPF.IPF", d2));
   ASSERT_EQ(true, (d1->CompareToDisk(d2, false) == 0));
   delete d1;
   delete d2;
}

TEST(DiskConversion, SCP2IPF)
{
   DiskBuilder disk_builder;
   IDisk * d1, *d2;
   ASSERT_EQ(0, disk_builder.LoadDisk(TestWorkspace::Fixture("res/The Demo [A].scp").c_str(), d1));
   ASSERT_EQ(0, disk_builder.SaveDisk("SCP2IPF.IPF", d1, "IPF"));
   ASSERT_EQ(0, disk_builder.LoadDisk("SCP2IPF.IPF", d2));
   ASSERT_EQ(true, (d1->CompareToDisk(d2, false) == 0));
   delete d1;
   delete d2;
}

/////////////////////////////////////////////////////////////
// EDSK : todo

/////////////////////////////////////////////////////////////
// SCP : todo

/////////////////////////////////////////////////////////////
// HFE : waiting for v3 ? todo

/////////////////////////////////////////////////////////////
// Kryoflux : todo (also , to code !)





/////////////////////////////////////////////////////////////
// Check that LoadDiskFromBuffer have the same generated disk than InsertTape

TEST(TapeFormat, TZX)
{
   ASSERT_EQ(true, CompareTape("res/Basil The Great Mouse Detective (UK) (1987) [Original] [TAPE].cdt"));
}

TEST(TapeFormat, CSW11)
{
   ASSERT_EQ(true, CompareTape("res/Mask_1_1.csw"));
}

TEST(TapeFormat, CSW20)
{
   ASSERT_EQ(true, CompareTape("res/Ultima Ratio (Firebird)(UK)(1987)[Original][TAPE].csw"));
}

TEST(TapeFormat, WAV)
{
   ASSERT_EQ(true, CompareTape("res/FootBallerOfTheYear2.wav"));
}

TEST(TapeFormat, VOC)
{
   ASSERT_EQ(true, CompareTape("res/K7_MICK.VOC"));
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// Motherboard test
TEST(Motherboard, Init)
{
   SoundMixer        sound_mixer ;
   Motherboard       *motherboard_emulation;
   CDisplay          *display = new CDisplay;
   KeyboardForTest   *keyboard = new KeyboardForTest;

   
#ifdef _DEBUG
   display->Init(true);
   display->Show(true);
#else
   display->Init(false);
   display->Show(false);
#endif

   motherboard_emulation = new Motherboard(&sound_mixer, keyboard);
   
   motherboard_emulation->SetPlus(true);
   motherboard_emulation->InitMotherbard(nullptr, nullptr, display, nullptr, nullptr, nullptr);
   motherboard_emulation->OnOff();
   motherboard_emulation->GetMem()->InitMemory();
   motherboard_emulation->GetMem()->SetRam(1);
   motherboard_emulation->GetCRTC()->DefinirTypeCRTC(CRTC::AMS40226);
   motherboard_emulation->GetVGA()->SetPAL(true);
   LoadCprFromBuffer(motherboard_emulation, Ghostngoblins_cpr, sizeof(Ghostngoblins_cpr));
   motherboard_emulation->GetPSG()->Reset();
   motherboard_emulation->GetSig()->Reset();


   motherboard_emulation->InitStartOptimizedPlus();

   motherboard_emulation->OnOff();
   //motherboard_emulation->Start(Motherboard::HW_PLUS|Motherboard::HW_FDC, 4000 * 50 * 20);
   motherboard_emulation->StartOptimizedPlus<true, true, false>(4000 * 50 * 20);

   delete motherboard_emulation;
   delete display;
   delete keyboard;

}
