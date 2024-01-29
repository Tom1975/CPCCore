#pragma once 

#include <iostream>
#include <functional>
#include "simple_stdio.h"
#include "Machine.h"
#include "Display.h"
#include <filesystem>

// SCR_CREATE  = false : Generate screenshot
// SCR_COMPARE  = true: Compare screenshot
#define NO_INIT_SCREENSHOT

#define  SCR_COMPARE true
#define  SCR_CREATE false

/////////////////////////////////////////////////////////////
/// Helper functions

bool CompareDisks(IDisk *d1, IDisk* d2, std::string p1);
bool CompareTape(std::string p1);

bool InitBinary(const char* conf, const char* initfile, const char* binary_to_load, unsigned short addr_ok, unsigned short addr_ko, unsigned int tolerated_error = 0);
int LoadCprFromBuffer(Motherboard* motherboard, unsigned char* buffer, int size);

/////////////////////////////////////////////////////////////
/// Helper functions

class ConfigurationManager : public IConfiguration
{
public:

   ConfigurationManager();
   virtual ~ConfigurationManager();

   virtual void OpenFile(const char* config_file);
   virtual void SetConfiguration(const char* section, const char* cle, const char* valeur, const char* file);
   virtual unsigned int GetConfiguration(const char* section, const char* cle, const char* default_value, char* out_buffer, unsigned int buffer_size, const char* file);
   virtual unsigned int GetConfigurationInt(const char* section, const char* cle, unsigned int default_value, const char* file);

protected:
   void Clear();

   struct data : std::map <std::string, std::string>
   {
      // Here is a little convenience method...
      bool iskey(const std::string& s) const
      {
         return count(s) != 0;
      }
   };

   typedef std::map <std::string, data* > ConfigFile;

   ConfigFile config_file_;
   std::string current_config_file_;
};

class FileLog : public ILog
{
public:
   FileLog(const char* file);
   virtual ~FileLog();

   virtual void WriteLog(const char* pLog);

   virtual void WriteLogByte(unsigned char pNumber);
   virtual void WriteLogShort(unsigned short pNumber);
   virtual void WriteLog(unsigned int pNumber);
   virtual void EndOfLine();
protected:
   FILE * f_;
};

class Log : public ILog
{
public:

   virtual void WriteLog(const char* pLog) {  };
   virtual void WriteLogByte(unsigned char pNumber) { char buf[256]; sprintf(buf, " %2.2X ", pNumber); }
   virtual void WriteLogShort(unsigned short pNumber) { char buf[256]; sprintf(buf, " %4.4X ", pNumber); }
   virtual void WriteLog(unsigned int pNumber) { char buf[256]; sprintf(buf, " %8.8X ", pNumber); };
   virtual void EndOfLine() { };

};
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

extern std::map<std::string, std::pair<unsigned int, unsigned int>> Escape_map_;

class CommandList;
class CSLScriptRunner;

class TestDump
{
public:
   TestDump(bool display = false);
   virtual ~TestDump()
   {
      delete machine_;
      delete runner_;
   }

   bool Test(std::filesystem::path conf, std::filesystem::path initfile, char moduleName, bool bFixedSpeed = true, int seed = 0xFEED1EE7);

   DirectoriesImp dirImp_;
   CDisplay display_;
   Log log_;
   ConfigurationManager conf_manager_;
   SoundFactory soundFactory_;
   EmulatorEngine* machine_;
   CSLScriptRunner* runner_;
};

class TestTape
{
public:
   TestTape()
   {
      machine_ = new EmulatorEngine();
   }
   virtual ~TestTape()
   {
      delete machine_;
   }

   bool Test(const char* conf, const char* initfile, const char* dump_to_load, const char* fic_to_scan,
      unsigned short addr, unsigned short opcode, const char* reg, int timeout, bool build);
   bool MoreTest(const char* fic_to_scan, unsigned short addr, unsigned short end_addr, const char* reg, int timeout, bool build);

   int RunTimeSlicesDbg(int nbSlices);
   unsigned short GetRegister(const char* pRegister);

   DirectoriesImp dirImp;
   CDisplay display;
   Log log;
   SoundFactory soundFactory;
   EmulatorEngine* machine_;
   ConfigurationManager conf_manager;

};

class KeyboardForTest : public IKeyboardHandler
{
public:
   KeyboardForTest() {};
   virtual ~KeyboardForTest() {};
   virtual unsigned char GetKeyboardMap(int index) { return 0xFF; }
   virtual void Init(bool* register_replaced) {}
   virtual void ForceKeyboardState(unsigned char key_states[10]) {};
};
