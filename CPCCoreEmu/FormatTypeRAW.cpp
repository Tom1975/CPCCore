#include "stdafx.h"
#include "FormatTypeRAW.h"
#include <cmath>
#include <regex>
#include <sstream>

#include "FileAccess.h"

#include "simple_stdio.h"
using namespace std;

#define TXT_OUT_PROF _T("Duree load disk")
#define PROF

#define DEFAULT_FLUX_SIZE     6500

FormatTypeRAW::FormatTypeRAW()
{
   mck_ /*Master Clock Frequency	 */ = ((18432000 * 73) / 14) / 2;
   sck_ /*Sample Frequency	 */ = mck_ / 2;
   sck_ = 24027428.5714285;

   ick_ /*Index Frequency	 */ = mck_ / 16;
   ick_ = 3003428.57428571;
   sample_to_index_ratio_ = sck_ / ick_;
   index_to_sample_ratio_ = ick_ / sck_;
   //m_CurrentSideLoad = 0;
   //   m_CurrentTrackLoad = 0;
}

FormatTypeRAW::~FormatTypeRAW()
{
}

bool FormatTypeRAW::CanLoad(const char* file_path)
{
   return true;
}


int FormatTypeRAW::LoadDiskTrackFromBuffer(IDisk*& created_disk, int side, int track, const unsigned char* buffer,
                                           int size)
{
   unsigned char h; // byte header
   //   m_CurrentTrackLoad = track;
   //   m_CurrentSideLoad = side;

   unsigned int flux_value = 0;
   last_stream_index_position_ = 0;
   finished_ = false;

   bool overflow = false;
   stream_position_ = 0;
   size_of_flux_value_ = DEFAULT_FLUX_SIZE;

   int index_pos = 0;

   flux_list_ = new FluxElement[size_of_flux_value_];
   nb_flux_value_ = 0;
   index_list_.clear();

   // Next byte header
   h = GetFromBuffer(buffer, index_pos);

   while (!finished_ && index_pos < size)
   {
      // decode
      switch (h)
      {
      case 0x0A:
         GetFromBuffer(buffer, index_pos); // Skip this byte
      case 0x09:
         GetFromBuffer(buffer, index_pos); // Skip this byte
      case 0x08:
         ; // Nothing : Skip header byte
         break;
      case 0x0B:
         // Overflow
         flux_value += 0x10000;
         overflow = true;
         break;
      case 0x0C:
         {
            if (!overflow) flux_value = 0;

            unsigned char h1 = GetFromBuffer(buffer, index_pos);
            unsigned char h2 = GetFromBuffer(buffer, index_pos);
            flux_value += (h1 << 8) + h2;
            InsertFluxValue(flux_value);
            overflow = false;
            break;
         }
      case 0x0D:
         --stream_position_;
         HandleOOB(buffer, index_pos);
         break;
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7: // Flux 2
         {
            if (!overflow) flux_value = 0;

            unsigned char h1 = GetFromBuffer(buffer, index_pos);;
            flux_value += ((h << 8) + h1);
            InsertFluxValue(flux_value);
            overflow = false;
            break;
         }
      default: // Flux 1
         if (!overflow) flux_value = 0;

         flux_value += h;
         InsertFluxValue(flux_value);
         overflow = false;
         break;
      }

      // Next byte header
      h = GetFromBuffer(buffer, index_pos);;
   }

   // Compute datas
   ComputeTrack(created_disk, side, track);
   created_disk->CreateSingleTrackFromMultiRevolutions(side, track);

   delete[]flux_list_;
   return 0;
}

int FormatTypeRAW::LoadTrack(IDisk*& created_disk, const char* file_path, int side, int track)
{
   FILE* file;

   unsigned char h; // byte header

   unsigned int flux_value = 0;
   last_stream_index_position_ = 0;
   finished_ = false;

   if (fopen_s(&file, file_path, "rb") == 0)
   {
      bool overflow = false;
      stream_position_ = 0;
      size_of_flux_value_ = DEFAULT_FLUX_SIZE;

      flux_list_ = new FluxElement[size_of_flux_value_];
      nb_flux_value_ = 0;
      index_list_.clear();

      // Next byte header
      h = Get(file);

      while (!finished_ && !feof(file))
      {
         // decode
         switch (h)
         {
         case 0x0A:
            Get(file); // Skip this byte
         case 0x09:
            Get(file); // Skip this byte
         case 0x08:
            ; // Nothing : Skip header byte
            break;
         case 0x0B:
            // Overflow
            flux_value += 0x10000;
            overflow = true;
            break;
         case 0x0C:
            {
               if (!overflow) flux_value = 0;

               unsigned char h1 = Get(file);
               unsigned char h2 = Get(file);
               flux_value += (h1 << 8) + h2;
               InsertFluxValue(flux_value);
               overflow = false;
               break;
            }
         case 0x0D:
            --stream_position_;
            HandleOOB(file);
            break;
         case 0:
         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
         case 6:
         case 7: // Flux 2
            {
               if (!overflow) flux_value = 0;

               unsigned char h1 = Get(file);
               flux_value += ((h << 8) + h1);
               InsertFluxValue(flux_value);
               overflow = false;
               break;
            }
         default: // Flux 1
            if (!overflow) flux_value = 0;

            flux_value += h;
            InsertFluxValue(flux_value);
            overflow = false;
            break;
         }

         // Next byte header
         h = Get(file);
      }

      fclose(file);

      // Compute datas
      ComputeTrack(created_disk, side, track);
      created_disk->CreateSingleTrackFromMultiRevolutions(side, track);

      delete[]flux_list_;
   }
   else
   {
      return -1;
   }

   return 0;
}

void FormatTypeRAW::HandleOOB(FILE* file)
{
   char type;
   unsigned short size;

   type = Get(file);
   size = GetWord(file);
   stream_position_ -= 3;

   switch (type)
   {
   case 0x00: // Invalid O_OB
      {
         //LOG("ERROR : INVALID O_OB !");
         //LOGEOL;
         //m_bFinished = true;
         break;
      }
   case 0x01:
      {
         // size should be 8
         // Use it for check ?
         unsigned int streamPos = GetDualWord(file);
         unsigned int transfert_time = GetDualWord(file);
         stream_position_ -= 8;
         if (stream_position_ != streamPos)
         {
            int dbg = 1;
         }

         // TODO
         break;
      }

   case 0x02: // Index signal data
      {
         // size should be 0x0C - TOCHECK ?
         unsigned int streamPos = GetDualWord(file);
         unsigned int sampleCounter = GetDualWord(file);
         unsigned int indexCounter = GetDualWord(file);
         stream_position_ -= 12;
         char buffer[256];
         if (last_stream_index_position_ == 0)
         {
            sprintf(buffer, "Index %i found at pos : %i - Pos in file : %i - Sample counter : %i", indexCounter,
                    streamPos, stream_position_, sampleCounter);
         }
         else
         {
            sprintf(buffer,
                    "Index found at pos : %i - Diff with last : %i - Pos in file : %i - Diff with last : %i - Sample counter : %i",
                    indexCounter, indexCounter - last_stream_index_count_, streamPos,
                    streamPos - last_stream_index_position_, sampleCounter);
         }
         last_stream_index_position_ = streamPos;
         last_stream_index_count_ = indexCounter;
         //LOG  (buffer);
         //LOGEOL;
         IndexElement element;

         element.flux_value = sampleCounter;
         element.index_clock = indexCounter;
         element.stream_position = streamPos;


         index_list_.push_back(element);

         break;
      }

   case 0x03: // No more flux to transfer (one per track)
      {
         // size should be 8 - TOCHECK ?
         unsigned int stream_pos = GetDualWord(file);
         unsigned int result_code = GetDualWord(file);
         stream_position_ -= 8;
         if (stream_position_ != stream_pos)
         {
            /*LOGEOL
            LOG(_T("Stream position error.... "));
            LOGEOL*/
         }
         break;
      }
   case 0x04: // HW Information from KryoFlux device
      {
         char* info = new char[size];

         ReadASCII(file, info, size);
         // Handle it
         HandleHWInfo(info, size);
         delete[]info;

         break;
      }
   case 0xD: // End of file (no more data to process)
      finished_ = true;
      break;
   default:
      // ERROR !
      break;
   }
}

void FormatTypeRAW::HandleOOB(const unsigned char* buffer, int& pos)
{
   char type;
   unsigned short size;

   type = GetFromBuffer(buffer, pos);
   size = GetWordFromBuffer(buffer, pos);
   stream_position_ -= 3;

   switch (type)
   {
   case 0x00: // Invalid O_OB
      {
         //      LOG(_T("ERROR : INVALID O_OB !"));
         //      LOGEOL;
         //m_bFinished = true;
         break;
      }
   case 0x01:
      {
         // size should be 8
         // Use it for check ?
         unsigned int stream_pos = GetDualWordFromBuffer(buffer, pos);
         unsigned int transfert_time = GetDualWordFromBuffer(buffer, pos);
         stream_position_ -= 8;
         if (stream_position_ != stream_pos)
         {
            int dbg = 1;
         }
         break;
      }

   case 0x02: // Index signal data
      {
         // size should be 0x0C - TOCHECK ?
         unsigned int streamPos = GetDualWordFromBuffer(buffer, pos);
         unsigned int sampleCounter = GetDualWordFromBuffer(buffer, pos);
         unsigned int indexCounter = GetDualWordFromBuffer(buffer, pos);
         stream_position_ -= 12;
         char buffer[256];
         if (last_stream_index_position_ == 0)
         {
            sprintf(buffer, "Index %i found at pos : %i - Pos in file : %i - Sample counter : %i", indexCounter,
                    streamPos, stream_position_, sampleCounter);
         }
         else
         {
            sprintf(buffer,
                    "Index found at pos : %i - Diff with last : %i - Pos in file : %i - Diff with last : %i - Sample counter : %i",
                    indexCounter, indexCounter - last_stream_index_count_, streamPos,
                    streamPos - last_stream_index_position_, sampleCounter);
         }
         last_stream_index_position_ = streamPos;
         last_stream_index_count_ = indexCounter;
         /*LOG(buffer);
         LOGEOL;*/
         IndexElement element;

         element.flux_value = sampleCounter;
         element.index_clock = indexCounter;
         element.stream_position = streamPos;


         index_list_.push_back(element);

         break;
      }

   case 0x03: // No more flux to transfer (one per track)
      {
         // size should be 8 - TOCHECK ?
         unsigned int stream_pos = GetDualWordFromBuffer(buffer, pos);
         unsigned int result_code = GetDualWordFromBuffer(buffer, pos);
         stream_position_ -= 8;
         if (stream_position_ != stream_pos)
         {
            /*LOGEOL
            LOG(_T("Stream position error.... "));
            LOGEOL*/
         }
         break;
      }
   case 0x04: // HW Information from KryoFlux device
      {
         char* info = new char[size];

         ReadASCIIFromBuffer(buffer, pos, info, size);
         // Handle it
         HandleHWInfo(info, size);
         delete[]info;

         break;
      }
   case 0xD: // End of file (no more data to process)
      finished_ = true;
      break;
   default:
      // ERROR !
      break;
   }
}

void FormatTypeRAW::InsertFluxValue(unsigned int FluxValue)
{
   if (nb_flux_value_ >= size_of_flux_value_)
   {
      // Make the array bigger...
      FluxElement* new_bigger_flux = new FluxElement[size_of_flux_value_ * 2];
      memcpy(new_bigger_flux, flux_list_, sizeof(FluxElement) * size_of_flux_value_);
      delete[]flux_list_;
      size_of_flux_value_ = size_of_flux_value_ * 2;

      flux_list_ = new_bigger_flux;
   }

   flux_list_[nb_flux_value_].flux_value = FluxValue;
   flux_list_[nb_flux_value_].stream_position = stream_position_;

   ++nb_flux_value_;
}

void FormatTypeRAW::HandleHWInfo(char* info, int size)
{
   // scan for sck / ick
   char* buffer = strstr(info, "sck=");
   if (buffer != NULL)
   {
      // replace first ',' by a \0
      char* pch = strchr(buffer, ',');
      if (pch != NULL) pch[0] = '\0';


      sscanf(buffer, "sck=%Lf", &sck_);

      if (pch != NULL) pch[0] = ',';
   }
   buffer = strstr(info, "ick=");
   if (buffer != NULL)
   {
      // replace first ',' by a \0
      char* pch = strchr(buffer, ',');
      if (pch != NULL) pch[0] = '\0';
      sscanf(buffer, "ick=%Lf", &ick_);
   }
}

// 0 : Ok
// -1 : Not enough index
int FormatTypeRAW::ComputeTrack(IDisk*& created_disk, int side, int track)
{
   // Number of Index copy :
   int indexNumber = index_list_.size();

   if (indexNumber < 2)
      return -1;

   created_disk->side_[side].tracks[track].revolution = new IDisk::Revolution[(indexNumber - 1)];
   created_disk->side_[side].tracks[track].nb_revolutions = indexNumber - 1;

   for (int index_revolution = 0; index_revolution < (indexNumber - 1); index_revolution++)
   {
      DecodeFluxFromIndex(index_revolution, created_disk->side_[side].tracks[track].revolution);
   }
   return 0;
}

////////////////////////////////////////////////
// Core function : Get cell width from various datas...
/*unsigned int FormatTypeRAW::GetCellWidth(double fluxValue)
{
   double realvCellTime = fluxValue / sck;
   double celllegnth = realvCellTime * 512000;

   celllegnth = celllegnth / m_RpmCorrection;
   return IDisk::GetCellWidth(celllegnth);
}*/

unsigned int FormatTypeRAW::DecodeFluxFromIndex(int index, IDisk::Revolution* revolution)
{
   /*if (m_CurrentTrackLoad == 26)
   {
      int dbg = 1;
   }*/
   revolution[index].size = ComputeTrack(index, NULL);
   revolution[index].bitfield = new unsigned char[revolution[index].size];

   return revolution[index].size = ComputeTrack(index, revolution);
}

unsigned int FormatTypeRAW::ComputeTrack(int index, IDisk::Revolution* revolution)
{
#define CLOCK_MAX_ADJ 10
#define CLOCK_MIN(_c) (((_c) * (100 - CLOCK_MAX_ADJ)) / 100)
#define CLOCK_MAX(_c) (((_c) * (100 + CLOCK_MAX_ADJ)) / 100)

#define MCK_FREQ (((18432000 * 73) / 14) / 2)
#define SCK_FREQ (MCK_FREQ / 2)
#define ICK_FREQ (MCK_FREQ / 16)
#define SCK_PS_PER_TICK (1000000000/(SCK_FREQ/1000))
#define PERIOD_ADJ_PCT 5

   int flux = 0;
   int clock = 2000; // 2 us
   int clock_centre = clock;
   unsigned int i = 0;
   unsigned int nb_bit_in_flux = 0;
   int clocked_zeros = 0;

   IndexElement debut = index_list_[index];
   IndexElement fin = index_list_[index + 1];

   unsigned int j;
   double first_flux_value = 0, last_flux_value = 0;
   // Get next cell
   for (i = 0; i < nb_flux_value_; i++)
   {
      if (flux_list_[i].stream_position >= debut.stream_position)
      {
         break;
      }
   }
   for (j = i; j < nb_flux_value_; j++)
   {
      if (flux_list_[j].stream_position == fin.stream_position)
      {
         //last_flux_value = fin.flux_value; // m_FluxList[j].flux_value;
         //j --;
         break;
      }
   }
   nb_bit_in_flux = 0;

   // Get the proper first cell
   first_flux_value = debut.flux_value;
   last_flux_value = fin.flux_value;

   int remaining = 0;
   int remaining_negative = 0;
   // Value of the first cell :
   if (i > 0)
   {
      if (flux_list_[i - 1].flux_value > first_flux_value)
         remaining = flux_list_[i - 1].flux_value - static_cast<int>(first_flux_value);
      else
         remaining_negative = static_cast<int>(first_flux_value) - flux_list_[i - 1].flux_value;
   }

   // Adjust first / last, to avoid having a double bit


   // Cell first ??
   flux = (remaining * SCK_PS_PER_TICK) / 1000u;
   int end_flux = 0;
   //flux = remaining;

   unsigned int k = i;
   //i = 0;
   //while (i < sizeOfBuffer*2)
   for (; k < j;)
   {
      // What's the current flux status
      // Under a half clock : add the next one
      if (flux < clock / 2 && k < j)
      {
         int localflux = flux_list_[k++].flux_value;
         if (k - 1 == i)
         {
            localflux -= remaining_negative;
         }
         else if (k == j)
         {
            end_flux = static_cast<int>((last_flux_value * SCK_PS_PER_TICK) / 1000u - clock);
            // Compute last flux : It's xomputed to know WHEN to stop, not WHAT is it containing.

            /*
            if (last_flux_value < localflux)
            localflux = last_flux_value;
            else
            {
            // To check
            int dbg = 1;
            localflux = last_flux_value;
            }*/
         }

         flux += (localflux * SCK_PS_PER_TICK) / 1000u;
         clocked_zeros = 0;
      }

      // Remove a clock value to the flux
      flux -= clock;

      // If it remains at least a half clock : A zero's there
      if (flux >= clock / 2)
      {
         if (revolution != NULL) revolution[index].bitfield[nb_bit_in_flux] = 0;
         clocked_zeros++;
         nb_bit_in_flux++;
      }
      else
      {
         // Otherwise, we have a one
         if (revolution != NULL) revolution[index].bitfield[nb_bit_in_flux] = 1;
         nb_bit_in_flux++;

         // PLL computations HERE !
         if ((clocked_zeros >= 1) && (clocked_zeros <= 3))
         {
            // In sync: adjust base clock by 10% of phase mismatch.
            int diff = flux / (int)(clocked_zeros + 1);
            clock += diff / PERIOD_ADJ_PCT;
         }
         else
         {
            clock += (clock_centre - clock) * PERIOD_ADJ_PCT / 100;
         }


         clock = max(CLOCK_MIN(clock_centre),
                     min(CLOCK_MAX(clock_centre), clock));

         flux = flux / 2;
      }
   }

   // End adjustement
   while (flux >= clock / 2 && end_flux > 0)
   {
      flux -= clock;
      end_flux -= clock;

      // If it remains at least a half clock : A zero's there
      if (flux >= clock / 2)
      {
         if (revolution != NULL) revolution[index].bitfield[nb_bit_in_flux] = 0;
         clocked_zeros++;
         nb_bit_in_flux++;
      }
      else
      {
         // NOT ALWAYS : We have a ending one when the last flux reversal is smaller than the flux index
         // Otherwise, we have a one
         //if (last_flux_value > m_FluxList[j-1].flux_value )
         // This should be rethought and adjusted.... TODO
         //if (flux >= -(clock / 2))
         {
            // To check
            if (revolution != NULL) revolution[index].bitfield[nb_bit_in_flux] = 1;
            nb_bit_in_flux++;
         }
      }
   }

   return nb_bit_in_flux;
}

unsigned char FormatTypeRAW::GetFromBuffer(const unsigned char* buffer, int& pos)
{
   stream_position_++;
   return buffer[pos++];
}

unsigned char FormatTypeRAW::Get(FILE* file)
{
   stream_position_++;
   unsigned char c;
   fread(&c, 1, 1, file);

   return c;
}

unsigned short FormatTypeRAW::GetWordFromBuffer(const unsigned char* buffer, int& pos)
{
   unsigned short v = buffer[pos++];
   v |= (buffer[pos++] << 8);
   stream_position_ += 2;

   return v;
}

unsigned short FormatTypeRAW::GetWord(FILE* file)
{
   unsigned short v = 0;

   unsigned char c[2];
   fread(&c, 2, 1, file);

   v = c[0];
   v |= (c[1] << 8);
   stream_position_ += 2;
   return v;
}

unsigned long FormatTypeRAW::GetDualWordFromBuffer(const unsigned char* buffer, int& pos)
{
   unsigned long v = 0;

   v = buffer[pos++];
   v |= (buffer[pos++] << 8);
   v |= (buffer[pos++] << 16);
   v |= (buffer[pos++] << 24);
   stream_position_ += 4;
   return v;
}

unsigned long FormatTypeRAW::GetDualWord(FILE* file)
{
   unsigned long v = 0;
   unsigned char c[4];
   fread(&c, 4, 1, file);

   v = c[0];
   v |= (c[1] << 8);
   v |= (c[2] << 16);
   v |= (c[3] << 24);
   stream_position_ += 4;
   return v;
}

void FormatTypeRAW::ReadASCIIFromBuffer(const unsigned char* buffer, int& pos, char* info, int size)
{
   strncpy(info, (char*)&buffer[pos], size);
   pos += size;
}

void FormatTypeRAW::ReadASCII(FILE* file, char* info, int size)
{
   fread(info, size, 1, file);
}


unsigned int FormatTypeRAW::GetCellWidth(double fluxValue)
{
   double realvCellTime = fluxValue / sck_;
   double celllegnth = realvCellTime * 512000;

   celllegnth = celllegnth / rpm_correction_;

   double clock_average = 1;
   int clock_count = 0;

   for (int i = 0; i < NB_CLOCK_DATAS; i++)
   {
      clock_average += clock_average_datas_[i];
      clock_count += clock_average_cell_[i];
   }
   if (clock_count > 0)
      clock_average /= clock_count;

   if (clock_average < 0.75)
   {
      clock_average = 0.75;
   }
   else if (clock_average > 1.25)
   {
      clock_average = 1.25;
   }


   unsigned int cell = 1;

   //clock_average = 1.0f;
   if (clock_count > 0)
      cell = lround(fluxValue / clock_average);
   else
      cell = lround(fluxValue);

   unsigned int cell2 = lround(fluxValue);

   if (cell2 != cell)
   {
      // Compare who's the nearest
      if (abs(((float)cell) - (fluxValue / clock_average)) < (abs(((float)cell2) - fluxValue)))
      {
         // Nothing
         int dbg = 1;
      }
      else
      {
         cell = cell2;
      }
      int dbg = 1;
      //cell = cell2;
   }

   // Adjust with current clock
   clock_average_datas_[clock_average_index_] = fluxValue;
   clock_average_cell_[clock_average_index_++] = cell;
   if (clock_average_index_ >= NB_CLOCK_DATAS) clock_average_index_ = 0;

   // Correction ?
   /*if (cell < 2 && cell > 1)
   {
   cell = 2;
   }
   if ( cell > 8 )
   {
   cell = 8;
   }*/

   return cell;
}

int FormatTypeRAW::LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress)
{
   // One single file to load : We have to compute the while files from it, in order to have a list to load.
   std::vector<TrackItem> file_list;
   if (GetFileListFromName(file_path, file_list) == OK)
   {
      return LoadDisk(file_list, created_disk, loading_progress);
   }
   else
   {
      return FILE_ERROR;
   }
}

//
///////////////////////////////////////////////////
FormatTypeRAW::OperationReturn FormatTypeRAW::GetFileListFromName(const char* file_path,
                                                                  std::vector<TrackItem>& file_list)
{
#ifdef __MORPHOS__
   // Todo
   return -1;
#else

#endif

   // If file_path is a directory
   if (IsDirectory(file_path))
   {
      // walk the directory, 
      std::vector<std::string> file_list;
      GetDirectoryContent(file_path, file_list);

      // try to find pattern to get side / track
      // todo
      // fill the vector
   }
   else
   {
      // Otherwise, its a file : build a généric mask from this file
      std::string full_path = GetFullPath(file_path);
      std::string filename = GetFileFromPath(file_path);

      // Search for side / tracks. Filename should be of the form xxxx[track].[side].raw. Track si 00-41, side 0-1
      // Check last 8 characters
      if (filename.size() > 8)
      {
         std::string last_part = filename.substr(filename.size() - 8, 8);
         int track, side;
         if (sscanf(last_part.c_str(), "%i.%i.", &track, &side) != 2)
         {
            // Pattern not found
            return FILE_ERROR;
         }

         std::string dir_path = GetDirectoryFromPath(file_path);

         std::vector<std::string> directory_list;
         GetDirectoryContent(dir_path.c_str(), directory_list);

         std::string file_formatted = full_path.substr(0, full_path.size() - 8) + std::string("*.*.raw");

         for (auto it = directory_list.begin(); it != directory_list.end(); it++)
         {
            if (MatchTextWithWildcards(*it, file_formatted))
            {
               TrackItem item;
               item.buffer = nullptr;
               item.size = 0;
               item.path = *it;

               file_list.push_back(item);
            }
         }
      }
      else
      {
         // Not a regular RAW.... Will be hard to load !
         return FILE_ERROR;
      }


      // fill the vector
   }
   return OK;
}


bool FormatTypeRAW::GetDiskFromFilename(const char* filename, unsigned int& side, unsigned int& track)
{
   if (strlen(filename) <= 8)
   {
      return false;
   }

   std::string str = filename;
   std::string last_part = str.substr(str.size() - 8, 8);
   int track_tmp, side_tmp;
   if (sscanf(last_part.c_str(), "%d.%d.", &track_tmp, &side_tmp) == 2)
   {
      side = side_tmp;
      track = track_tmp;
      return true;
   }
   return false;
}

bool FormatTypeRAW::CreateDiskStructureFromFileList(std::vector<TrackItem> file_list, IDisk*& created_disk)
{
   unsigned int track_max_1 = 0;
   unsigned int track_max_2 = 0;

   unsigned int side;
   unsigned int track;

   for (auto it = file_list.begin(); it != file_list.end(); it++)
   {
      if (GetDiskFromFilename(it->path.c_str(), side, track))
      {
         if (side == 0 && track > track_max_1)
         {
            track_max_1 = track;
         }
         if (side == 1 && track > track_max_2)
         {
            track_max_2 = track;
         }
      }
      else
      {
         return false;
      }
   }
   // Create disk
   IDisk* new_disk = new IDisk();

   new_disk->side_[0].nb_tracks = track_max_1 + 1;
   new_disk->side_[0].tracks = new IDisk::MFMTrack[new_disk->side_[0].nb_tracks];
   memset(new_disk->side_[0].tracks, 0, sizeof(IDisk::MFMTrack) * new_disk->side_[0].nb_tracks);

   if (track_max_2 > 0)
   {
      new_disk->nb_sides_ = 2;
      new_disk->side_[1].nb_tracks = track_max_2 + 1;
      new_disk->side_[1].tracks = new IDisk::MFMTrack[new_disk->side_[1].nb_tracks];
      memset(new_disk->side_[1].tracks, 0, sizeof(IDisk::MFMTrack) * new_disk->side_[1].nb_tracks);
   }
   else
   {
      new_disk->nb_sides_ = 1;
   }
   created_disk = new_disk;
   return true;
}


int FormatTypeRAW::LoadDisk(std::vector<TrackItem> file_list, IDisk*& created_disk, ILoadingProgress* loading_progress)
{
   int result = OK;
   // Basic loading : only one file

   if (file_list.size() == 1)
   {
      return LoadDisk(file_list[0].path.c_str(), created_disk, loading_progress);
   }

   IDisk* new_disk;
   if (CreateDiskStructureFromFileList(file_list, new_disk) == false)
   {
      return FILE_ERROR;
   }

   int totalprogress = new_disk->side_[0].nb_tracks + ((new_disk->nb_sides_ == 2) ? new_disk->side_[1].nb_tracks : 0);
   int currentprogress = 0;

   for (auto it = file_list.begin(); it != file_list.end() && result == OK; it++)
   {
      unsigned int side, track;
      if (GetDiskFromFilename(it->path.c_str(), side, track))
      {
         // Either buffer or file !
         if (it->buffer != nullptr)
         {
            result = LoadDiskTrackFromBuffer(new_disk, side, track, it->buffer, it->size);
         }
         else
         {
            result = LoadTrack(new_disk, it->path.c_str(), side, track);
         }
      }
      else
      {
         result = FILE_ERROR;
      }

      if (loading_progress != nullptr)
      {
         currentprogress++;
         loading_progress->SetProgress((currentprogress * 100 / totalprogress));
      }
   }
   if (result == OK)
   {
      for (int side = 0; side < new_disk->nb_sides_; side++)
      {
         for (unsigned int i = 0; i < new_disk->side_[side].nb_tracks; i++)
         {
            if (new_disk->side_[side].tracks[i].bitfield == nullptr)
            {
               new_disk->side_[side].tracks[i].bitfield = new unsigned char[DEFAULT_TRACK_SIZE * 16];
               new_disk->side_[side].tracks[i].size = DEFAULT_TRACK_SIZE * 16;
               memset(new_disk->side_[side].tracks[i].bitfield, BIT_WEAK, DEFAULT_TRACK_SIZE * 16);
            }
         }
      }
      created_disk = new_disk;
   }
   else
   {
      delete new_disk;
   }
   return result;
}

/*
int FormatTypeRAW::LoadDisk(std::vector<buffer_item> buffer_list, std::vector<std::string> file_list, IDisk*& created_disk, ILoadingProgress* loading_progress)
{
   int result = OK;
   // Load each files 
   IDisk* new_disk;

   if (CreateDiskStructureFromFileList(file_list, new_disk) == false)
   {
      return FILE_ERROR;
   }

   int totalprogress = new_disk->disk_[0].NbTracks + ((new_disk->m_NbSide == 2) ? new_disk->disk_[1].NbTracks : 0);
   int currentprogress = 0;

   int maxtrack = min(buffer_list.size(), file_list.size());

   for (int i = 0; i < maxtrack && result == OK; i++)
   {
      unsigned int side, track;
      if (GetDiskFromFilename(file_list[i].c_str(), side, track))
      {
         result = LoadDiskTrackFromBuffer(new_disk, side, track, buffer_list[i].buffer, buffer_list[i].size);
         if (loading_progress != nullptr)
         {
            currentprogress++;
            loading_progress->SetProgress((currentprogress * 100 / totalprogress));
         }
      }
      result = FILE_ERROR;
   }
   if (result == OK)
   {
      for (int side = 0; side < new_disk->m_NbSide; side++)
      {
         for (unsigned int i = 0; i < new_disk->disk_[side].NbTracks; i++)
         {
            if (new_disk->disk_[side].Tracks[i].BitField == nullptr)
            {
               new_disk->disk_[side].Tracks[i].BitField = new unsigned char[DEFAULT_TRACK_SIZE * 16];
               new_disk->disk_[side].Tracks[i].Size = DEFAULT_TRACK_SIZE * 16;
               memset(new_disk->disk_[side].Tracks[i].BitField, BIT_WEAK, DEFAULT_TRACK_SIZE * 16);
            }
         }
      }
      created_disk = new_disk;
   }
   else
   {
      delete new_disk;
   }
   return result;
}*/

int FormatTypeRAW::LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                            ILoadingProgress* loading_progress)
{
   return NOT_IMPLEMENTED;
}

int FormatTypeRAW::SaveDisk(const char* file_path, IDisk* disk, ILoadingProgress* loading_progress) const
{
   return NOT_IMPLEMENTED;
}
