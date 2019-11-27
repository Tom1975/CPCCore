#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>

#include "gtest/gtest.h"
#include "Machine.h"
#include "Display.h"

/////////////////////////////////////////////////////////////
/// Helper functions


class SoundFactory : public ISoundFactory
{
   virtual ISound* GetSound(const char* pName) { return nullptr; };
   virtual const char* GetSoundName(ISound*) { return "Mock sound"; };

   virtual const char* GetFirstSoundName() { return nullptr; };
   virtual const char* GetNextSoundName() { return nullptr; };
};

class DirectoriesImp : public IDirectories
{
public:
   virtual const char* GetBaseDirectory()
   {
      return ".";
   }
};


/*void InitBinary(char* binary_to_load, unsigned short addr_ok, unsigned short addr_ko)
{
   // Creation dela machine
   DirectoriesImp dirImp;
   CDisplay display;
   display.Init();
   display.Show(true);

   SoundFactory soundFactory;
   EmulatorEngine* machine = new EmulatorEngine("."));
   machine->SetDirectories(&dirImp);

   machine->SetLog(NULL);
   machine->Init(&display, &soundFactory);
   machine->GetMem()->Initialisation();
   machine->SetFixedSpeed(true);
   machine->GetMem()->Initialisation();

   machine->LoadConfiguration("6128", ".\\TestConf.ini");
   machine->Reinit();

   // Load binary
   machine->LoadBinInt(binary_to_load);

   // Set breakpoints : To OK, KO, finisehd
   machine->AddBreakpoint(addr_ok);  // KO
   machine->AddBreakpoint(addr_ko);  // Error at the end

   bool finished = false;
   while (!finished)
   {
      machine->RunTimeSlice(false); // Run debug
                                    // Check next break : OK = continue
      if (machine->GetProc()->GetPC() == addr_ko)
      {
         finished = true; FAIL() << "Error at the end !"; break;
      }
      else if (machine->GetProc()->GetPC() == addr_ok)
      {
         finished = true; SUCCEED();
      }
   }
}*/

/////////////////////////////////////////////////////////////
// Test functions
TEST(FDC, fdctest)
{
   // todo : set correct address
   
}