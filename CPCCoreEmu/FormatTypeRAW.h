#pragma once
#include "IDisk.h"
#include "DiskBuilder.h"

#include <vector>

// Clock average computation
#define NB_CLOCK_DATAS 10

class FormatTypeRAW : public FormatType
{
public:
   FormatTypeRAW();
   virtual ~FormatTypeRAW();

   virtual const char* GetFormatName()
   {
      return "KRYOFLUX";
   }

   virtual const char* GetFormatDescriptor()
   {
      return "Kryoflux RAW file";
   }

   virtual const char* GetFormatExt()
   {
      return "raw";
   }


   virtual bool CanLoad() const
   {
      return true;
   }

   virtual bool CanSave() const
   {
      return false;
   }

   //////////////////////////////////////////////////////////
   // Multifile structure


   //////////////////////////////////////////////////////////
   // File versions of loader
   virtual bool CanLoad(const char* file_path);
   virtual int LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress = nullptr);
   virtual int LoadDisk(std::vector<FormatType::TrackItem> buffer_list, IDisk*& created_disk,
                        ILoadingProgress* loading_progress = nullptr);
   virtual int LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                        ILoadingProgress* loading_progress = nullptr);
   virtual int SaveDisk(const char* file_path, IDisk* disk, ILoadingProgress* loading_progress = nullptr) const;

private:
   FormatTypeRAW::OperationReturn GetFileListFromName(const char* file_path, std::vector<TrackItem>& file_list);
   unsigned int ComputeTrack(int index, IDisk::Revolution* revolution);
   bool CreateDiskStructureFromFileList(std::vector<TrackItem> file_list, IDisk*& created_disk);
   bool GetDiskFromFilename(const char* filename, unsigned int& side, unsigned int& track);

   // 
   int LoadTrack(IDisk*& created_disk, const char* file_path, int side, int track);
   int LoadDiskTrackFromBuffer(IDisk*& created_disk, int side, int track, const unsigned char* buffer, int size);
   void InsertFluxValue(unsigned int flux_value);
   void HandleOOB(FILE* file);
   void HandleOOB(const unsigned char* buffer, int& pos);
   void HandleHWInfo(char* info, int size);

   int ComputeTrack(IDisk*& created_disk, int side, int track);

   // File access

   unsigned char Get(FILE* file);
   unsigned char GetFromBuffer(const unsigned char* buffer, int& pos);
   unsigned short GetWord(FILE* file);
   unsigned short GetWordFromBuffer(const unsigned char* buffer, int& pos);
   unsigned long GetDualWord(FILE* file);
   unsigned long GetDualWordFromBuffer(const unsigned char* buffer, int& pos);
   void ReadASCII(FILE* file, char* info, int size);
   void ReadASCIIFromBuffer(const unsigned char* buffer, int& pos, char* info, int size);

   unsigned int GetCellWidth(double flux_value);

   bool finished_;

   typedef struct
   {
      unsigned int flux_value;
      unsigned int stream_position;
   } FluxElement;

   typedef struct
   {
      unsigned int flux_value;
      unsigned int stream_position;
      unsigned int index_clock;
   } IndexElement;

   unsigned int stream_position_;

   unsigned int last_stream_index_position_;
   unsigned int last_stream_index_count_;

   FluxElement* flux_list_;
   unsigned int size_of_flux_value_;
   unsigned int nb_flux_value_;

   std::vector<IndexElement> index_list_;


   double clock_average_datas_[NB_CLOCK_DATAS];
   int clock_average_cell_[NB_CLOCK_DATAS];

   int clock_average_index_;

   unsigned int DecodeFluxFromIndex(int index, IDisk::Revolution* revolution);

   long double mck_;
   long double sck_;
   long double ick_;
   double sample_to_index_ratio_;
   double index_to_sample_ratio_;

   double rpm_correction_;
};
