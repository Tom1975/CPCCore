
#ifdef _WIN32

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include "TestUtils.h"


/////////////////////////////////////////////////////////////
/// Helper functions
ConfigurationManager::ConfigurationManager()
{
}

ConfigurationManager::~ConfigurationManager()
{
   // Clear everything
   Clear();
}

void ConfigurationManager::Clear()
{
   for (auto const& ent1 : config_file_) 
   {
      // ent1.first is the first key
      for (auto const& ent2 : *ent1.second) 
      {
         // ent2.first is the second key
         // ent2.second is the data
      }
   }
}

void ConfigurationManager::OpenFile(const char* config_file)
{
   Clear();
   std::string s, key, value;
   std::ifstream f(config_file);
   std::string current_section = "";

   while (std::getline(f, s))
   {
      std::string::size_type begin = s.find_first_not_of(" \f\t\v");

      // Skip blank lines
      if (begin == std::string::npos) continue;
      // Skip commentary
      if (std::string("#;").find(s[begin]) != std::string::npos) continue;

      // Search sections
      std::string::size_type begin_section = s.find_first_of("[");
      if (begin_section != std::string::npos)
      {
         std::string::size_type end_section = s.find_first_of("]", begin_section);
         if (end_section != std::string::npos)
         {
            current_section = s.substr(begin_section+1, end_section-1);
         }
      }

      // Search key (if a section is already defined)
      if (current_section.size() > 0)
      {
         // Extract the key value
         std::string::size_type end = s.find('=', begin);

         if (end == std::string::npos) continue;

         key = s.substr(begin, end - begin);

         // (No leading or trailing whitespace allowed)
         key.erase(key.find_last_not_of(" \f\t\v") + 1);

         // No blank keys allowed
         if (key.empty()) continue;

         // Extract the value (no leading or trailing whitespace allowed)
         begin = s.find_first_not_of(" \f\n\r\t\v", end + 1);
         if (begin == std::string::npos)
         {
            value = "";
         }
         else
         {
            end = s.find_last_not_of(" \f\n\r\t\v") + 1;
            value = s.substr(begin, end - begin);
         }

         

         // Add this key/value to current section
         data* d;
         if (config_file_.count(current_section) > 0)
         {
             d = config_file_.at(current_section);
         }
         else
         {
            d = new data;
            config_file_[current_section] = d;
         }
         (*d)[key] = value;
      }
   }
}

void ConfigurationManager::SetConfiguration(const char* section, const char* cle, const char* valeur, const char* file)
{
   OpenFile(file);
   // Add or update key
   // rewrite whole file
}

size_t ConfigurationManager::GetConfiguration(const char* section, const char* cle, const char* default_value, char* out_buffer, size_t buffer_size, const char* file)
{
   OpenFile(file);
   if (config_file_.count(section) > 0)
   {
      if (config_file_[section]->count(cle) > 0)
      {
         std::string value = config_file_[section]->at(cle);
         if (value.size() < buffer_size)
         {
            strncpy(out_buffer, value.c_str(), buffer_size);
            return strlen(out_buffer);
         }
         else
         {
            strncpy(out_buffer, value.c_str(), buffer_size);
            return buffer_size;
         }
      }
   }

   return 0;
}

unsigned int ConfigurationManager::GetConfigurationInt(const char* section, const char* cle, unsigned int default_value, const char* file)
{
   OpenFile(file);
   if (config_file_.count(section) > 0)
   {
      if (config_file_[section]->count(cle) > 0)
      {
         std::string value = config_file_[section]->at(cle);
         return atoi(value.c_str());
      }
   }
   return default_value;
}


/////////////////////////////////////////////////////////////
/// Helper functions - Disk

/////////////////////////////////////////////////////////////
/// Helper functions - Tape
bool CompareTape(std::string p1)
{
   CTape t1, t2;
   FILE* f;

   // Check loading from buffer
   if (fopen_s(&f, p1.c_str(), "rb") == 0)
   {
      fseek(f, 0, SEEK_END);
      int buffer_size = ftell(f);
      rewind(f);
      unsigned char* buffer = new unsigned char[buffer_size];

      fread(buffer, buffer_size, 1, f);
      fclose(f);
      int insert_tape = t1.InsertTape(buffer, buffer_size);
      delete[]buffer;

      if (insert_tape != 0)
      {
         return false;
      }
   }
   else
   {
      return false;
   }

   // Check loading from file
   if (t2.InsertTape(p1.c_str()) != 0) return false;
   if (t2.InsertTapeDelayed() != 0) return false;

   // Check that results are equal
   return (t1.CompareToTape(&t2) == 0);
}


bool TestDump::Test(const char* conf, const char* initfile, const char* dump_to_load, const char* run_command, CommandList* cmd_list, bool bFixedSpeed, int seed)
{
   // Creation dela machine
   
#ifdef _DEBUG
   display.Init(true);
   display.Show(true);
#else
   display.Init(false);
   display.Show(false);
#endif

   machine_->SetDirectories(&dirImp);

   machine_->SetLog(&log);

   ConfigurationManager conf_manager;
   machine_->SetConfigurationManager(&conf_manager);

   machine_->Init(&display, &soundFactory);
   machine_->GetMem()->Initialisation();
   machine_->GetMem()->Initialisation();

   machine_->LoadConfiguration(conf, initfile);
   machine_->Reinit();

   //  Fix a seed for randomness
   srand(seed);
   machine_->SetFixedSpeed(bFixedSpeed);

   // Load disk
   machine_->LoadDisk(dump_to_load, 0, false);
   //DiskContainer* container = machine_->CanLoad(dump_to_load);
   //machine_->LoadMedia(container);

   // Set run command
   machine_->Paste(run_command);

   int nbcycle_for_paste = strlen(run_command) * 3;
   for (int i = 0; i < nbcycle_for_paste; i++)
   {
      machine_->RunTimeSlice(false);
   }

   // Run preliminary actions
   bool no_error = cmd_list->RunFirstCommand(machine_);
   while (cmd_list->IsFinished() == false && no_error)
   {
      no_error = cmd_list->RunNextCommand(machine_);
   }
   return no_error;
}


bool InitBinary(char* conf, char* initfile, char* binary_to_load, unsigned short addr_ok, unsigned short addr_ko, unsigned int tolerated_error )
{
   bool result = true;
   // Creation dela machine
   DirectoriesImp dirImp;
   ConfigurationManager conf_manager;
   CDisplay display;
   
#ifdef _DEBUG
   display.Init(true);
   display.Show(true);
#else
   display.Init(false);
   display.Show(false);
#endif


   SoundFactory soundFactory;
   EmulatorEngine* machine = new EmulatorEngine();
   machine->SetDirectories(&dirImp);
   machine->SetConfigurationManager(&conf_manager);

   machine->SetLog(NULL);
   machine->Init(&display, &soundFactory);
   machine->GetMem()->Initialisation();
   machine->SetFixedSpeed(true);
   machine->GetMem()->Initialisation();

   machine->LoadConfiguration(conf, initfile);
   machine->Reinit();

   // Run until ok 
   for (int i = 0; i < 100; i++)
   {
      machine->RunTimeSlice();
   }

   // Load binary
   machine->LoadBinInt(binary_to_load);


   // Set breakpoints : To OK, KO, finisehd
   machine->AddBreakpoint(addr_ok);  // KO
   machine->AddBreakpoint(addr_ko);  // Error at the end

   bool finished = false;
   unsigned int error = 0;
   while (!finished)
   {
      machine->RunTimeSlice(false); // Run debug
                                    // Check next break : OK = continue
      if (machine->GetProc()->GetPC() == addr_ko)
      {
         error++;
         if (error > tolerated_error)
         {
            result =  false;
            //FAIL() << "Fatal Error !\n" << "Number of errors : " << error;;
            finished = true;
         }
      }
      else if (machine->GetProc()->GetPC() == addr_ok)
      {
         result = true;
         finished = true;
         //SUCCEED() << "Number of errors : " << error;
      }

      // Press space (in case !)
      if (machine->PasteBufferIsEmpty ())
         machine->Paste(" ");
   }
   delete machine;
   return result;
}


int TestTape::RunTimeSlicesDbg(int nbSlices)
{
   /*MSG msg;
   for (int i = 0; i < nbSlices; i++)
   {
      while (PeekMessage(&msg, NULL, 0, 0, TRUE))
      {
         if (msg.message == WM_KEYDOWN)
         {
            return 2;
         };

         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
      if (machine_->RunTimeSlice(false) == 1)
         return 1;
   }*/
   if (machine_->RunTimeSlice(false) == 1)
      return 1;

   return 0;
}


unsigned short TestTape::GetRegister(char* pRegister)
{
   // Add a breakpoint
   if (strcmp(pRegister, "A") == 0) return machine_->GetProc()->af_.b.h;
   if (strcmp(pRegister, "B") == 0) return machine_->GetProc()->bc_.b.h;
   if (strcmp(pRegister, "C") == 0) return machine_->GetProc()->bc_.b.l;
   if (strcmp(pRegister, "D") == 0) return machine_->GetProc()->de_.b.h;
   if (strcmp(pRegister, "E") == 0) return machine_->GetProc()->de_.b.l;
   if (strcmp(pRegister, "H") == 0) return machine_->GetProc()->hl_.b.h;
   if (strcmp(pRegister, "L") == 0) return machine_->GetProc()->hl_.b.l;
   if (strcmp(pRegister, "PC") == 0) return machine_->GetProc()->pc_;


   return 0;
}


bool TestTape::Test( char* conf, char* initfile, char* dump_to_load, char* fic_to_scan,
                     unsigned short addr, unsigned short end_addr, char* reg, int timeout, bool build)
{
   // Creation dela machine
   
#ifdef _DEBUG
   display.Init(true);
   display.Show(true);
#else
   display.Init(false);
   display.Show(false);
#endif

   machine_->SetDirectories(&dirImp);

   machine_->SetLog(&log);

   machine_->SetConfigurationManager(&conf_manager);

   machine_->Init(&display, &soundFactory);
   machine_->GetMem()->Initialisation();
   machine_->GetMem()->Initialisation();

   machine_->LoadConfiguration(conf, initfile);
   machine_->Reinit();

   //  Fix a seed for randomness
   srand(0xE7123456);
   machine_->SetFixedSpeed(true);

   // Load disk
   machine_->LoadTape(dump_to_load);

   for (int i = 0; i < 100; i++)
   {
      machine_->RunTimeSlice();
   }

   // Set run command
   machine_->Paste("RUN\"\r");

   // Wait then press any key
   for (int i = 0; i < 20; i++)
   {
      machine_->RunTimeSlice(false);
   }
   machine_->Paste(" ");

   machine_->CleanBreakpoints();
   machine_->AddBreakpoint(addr);
   machine_->AddBreakpoint(end_addr);

   unsigned int nb_car = 0;

   unsigned int finished = 0;

   FILE* f;
   char readcar[5] = {0};
   if (!build)
   {
      f = fopen(fic_to_scan, "rb");
      fread(readcar, 3, 1, f);
   }
   else 
   {
      f = fopen(fic_to_scan, "wb");
   }

   unsigned int bytecount = 0;
   unsigned int byteinbuffer = 0;
   int count = 0;
   unsigned int nbCar = 0;
   char buffer[128];

   while (finished == 0)
   {
      unsigned int valRet = RunTimeSlicesDbg(1);
      //unsigned int valRet = machine_->RunTimeSlice();

      count++;
      if ((machine_->GetProc()->GetPC() == addr)
         && valRet == 1)
      {

         unsigned char b = (GetRegister(reg) & 0xFF);

         if (build)
         {
            nbCar++;

            if (nbCar == 16)
            {
               sprintf(buffer, "%2.2X\r\n", b);
               fwrite(buffer, 4, 1, f);
               nbCar = 0;
            }
            else
            {
               sprintf(buffer, "%2.2X ", b);
               fwrite(buffer, 3, 1, f);

            }
            bytecount++;
         }
         else
         {
            nbCar++;
            if (nbCar == 16)
            {
               sprintf(buffer, "%2.2X\r\n", b);
               nbCar = 0;
            }
            else
            {
               sprintf(buffer, "%2.2X ", b);
            }

            if (memcmp(buffer, readcar,2) == 0) // Just check the writable stuff
            {
               // Ok
               memset(readcar, 0, 5);
            }
            else
            {
               fclose(f);
               return false;
            }
            byteinbuffer = 0;

            if (nbCar == 15)
            {
               size_t lgRead = fread(readcar, 3, 1, f); // Read the two last char of the line, with CR/LF
               // Read one more char is 3rd char is not '\n' (to avoir problemes with '\r'\'n')
               if (readcar[2] != '\n')
                  lgRead = fread(&readcar[3], 1, 1, f);

               if (lgRead > 0)
               {
                  byteinbuffer = 1;
               }
               /*else
               {
                  finished = 1;
               }*/
            }
            else
            {
               size_t lgRead = fread(readcar, 3, 1, f);
               if (lgRead > 0)
               {
                  byteinbuffer = 1;
               }
               /*else
               {
                  finished = 1;
               }*/
            }

         }
      }


      if (build)
      {
         if ((machine_->GetProc()->GetPC() == end_addr)
            && valRet == 1)

         {
            finished = 1;
         }
      }
      else
      {
         if ((machine_->GetProc()->GetPC() == end_addr)
            && valRet == 1)
         {
            finished = 1;
         }
         else 
            if (timeout > 0)
         {
            if (count > timeout)
            {
               fclose(f);
               return false;
            }
         }
      }
   }
   
   fclose(f);

   return true;
}

bool TestTape::MoreTest(char* fic_to_scan, unsigned short addr, unsigned short end_addr, char* reg, int timeout, bool build)
{
   machine_->CleanBreakpoints();
   machine_->AddBreakpoint(addr);
   machine_->AddBreakpoint(end_addr);

   unsigned int nb_car = 0;

   unsigned int finished = 0;

   FILE* f;
   char readcar[5] = { 0 };
   if (!build)
   {
      f = fopen(fic_to_scan, "rb");
      fread(readcar, 3, 1, f);
   }
   else
   {
      f = fopen(fic_to_scan, "wb");
   }

   unsigned int bytecount = 0;
   unsigned int byteinbuffer = 0;
   int count = 0;
   unsigned int nbCar = 0;
   char buffer[128];

   while (finished == 0)
   {
      unsigned int valRet = RunTimeSlicesDbg(1);
      //unsigned int valRet = machine_->RunTimeSlice();

      count++;
      if (/*(machine_->GetProc()->GetCurrentOpcode() == opcode)
         && */(machine_->GetProc()->GetPC() == addr)
         && valRet == 1)
      {

         unsigned char b = (GetRegister(reg) & 0xFF);

         if (build)
         {
            nbCar++;

            if (nbCar == 16)
            {
               sprintf(buffer, "%2.2X\r\n", b);
               fwrite(buffer, 4, 1, f);
               nbCar = 0;
            }
            else
            {
               sprintf(buffer, "%2.2X ", b);
               fwrite(buffer, 3, 1, f);

            }
            bytecount++;
         }
         else
         {
            nbCar++;
            if (nbCar == 16)
            {
               sprintf(buffer, "%2.2X\r\n", b);
               nbCar = 0;
            }
            else
            {
               sprintf(buffer, "%2.2X ", b);
            }

            if (memcmp(buffer, readcar, 2) == 0) // Just check the writable stuff
            {
               // Ok
               memset(readcar, 0, 5);
            }
            else
            {
               char msg_buffer[256];
               sprintf(msg_buffer, "FAILED : Found %s instead of %s\n", buffer, readcar);
               fclose(f);
               return false;
            }
            byteinbuffer = 0;

            if (nbCar == 15)
            {
               size_t lgRead = fread(readcar, 3, 1, f); // Read the two last char of the line, with CR/LF
               // Read one more char is 3rd char is not '\n' (to avoir problemes with '\r'\'n')
               if (readcar[2] != '\n')
                  lgRead = fread(&readcar[3], 1, 1, f);

               if (lgRead > 0)
               {
                  byteinbuffer = 1;
               }
               else
               {
                  finished = 1;
               }
            }
            else
            {
               size_t lgRead = fread(readcar, 3, 1, f);
               if (lgRead > 0)
               {
                  byteinbuffer = 1;
               }
               else
               {
                  finished = 1;
               }
            }

         }
      }


      if (build)
      {
         if (/*(machine_->GetProc()->GetCurrentOpcode() == opcode)
            && */(machine_->GetProc()->GetPC() == end_addr)
            && valRet == 1)

         {
            finished = 1;
         }
      }
      else
      {
         if (timeout > 0)
         {
            if (count > timeout)
            {
               fclose(f);
               return false;
            }
         }
      }
   }

   fclose(f);

   return true;
}


int LoadCprFromBuffer(Motherboard* motherboard, unsigned char* buffer, int size)
{

   // Check RIFF chunk
   int index = 0;
   if (size >= 12
      && (memcmp(&buffer[0], "RIFF", 4) == 0)
      && (memcmp(&buffer[8], "AMS!", 4) == 0)
      )
   {
      // Reinit Cartridge
      motherboard->EjectCartridge();

      // Ok, it's correct.
      index += 4;
      // Check the whole size

      int chunk_size = buffer[index]
         + (buffer[index + 1] << 8)
         + (buffer[index + 2] << 16)
         + (buffer[index + 3] << 24);

      index += 8;

      // 'fmt ' chunk ? skip it
      if (index + 8 < size && (memcmp(&buffer[index], "fmt ", 4) == 0))
      {
         index += 8;
      }

      // Good.
      // Now we are at the first cbxx
      while (index + 8 < size)
      {
         if (buffer[index] == 'c' && buffer[index + 1] == 'b')
         {
            index += 2;
            char buffer_block_number[3] = { 0 };
            memcpy(buffer_block_number, &buffer[index], 2);
            int block_number = (buffer_block_number[0] - '0') * 10 + (buffer_block_number[1] - '0');
            index += 2;

            // Read size
            int block_size = buffer[index]
               + (buffer[index + 1] << 8)
               + (buffer[index + 2] << 16)
               + (buffer[index + 3] << 24);
            index += 4;

            if (block_size <= size && block_number < 256)
            {
               // Copy datas to proper ROM
               unsigned char* rom = motherboard->GetCartridge(block_number);
               memset(rom, 0, 0x1000);
               memcpy(rom, &buffer[index], block_size);
               index += block_size;
            }
            else
            {
               return -1;
            }
         }
         else
         {
            return -1;
         }
      }
   }
   else
   {
      // Incorrect headers
      return -1;
   }

   return 0;
}
