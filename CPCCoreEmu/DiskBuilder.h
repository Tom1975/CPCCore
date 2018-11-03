#pragma once

#include "ILoadingProgress.h"
#include "IDisk.h"

//////////////////////////////////////////////////////////
// Format types :
//////////////////////////////////////////////////////////
// Handle to format
class FormatType
{
public:
   enum OperationReturn
   {
      NOT_IMPLEMENTED = -2,
      FILE_ERROR = -1,
      OK = 0
   };

   //////////////////////////////////////////////////////////
   // Format description : Name, descriptor, extension
   virtual const char* GetFormatName() = 0;
   virtual const char* GetFormatDescriptor() = 0;
   virtual const char* GetFormatExt() = 0;

   //////////////////////////////////////////////////////////
   // Format capability : Is it possible to read or write in this format ?
   virtual bool CanLoad() const = 0;
   virtual bool CanSave() const = 0;

   //////////////////////////////////////////////////////////
   // Multitrack loader
   typedef struct
   {
      const unsigned char* buffer;
      size_t size;
      std::string path;
   } TrackItem;

   //////////////////////////////////////////////////////////
   // File versions of loader
   virtual bool CanLoad(const char* file_path) = 0;
   virtual int LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress = nullptr) = 0;
   virtual int LoadDisk(std::vector<TrackItem> track_list, IDisk*& created_disk,
                        ILoadingProgress* loading_progress = nullptr) = 0;
   virtual int LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                        ILoadingProgress* loading_progress = nullptr) = 0;
   virtual int SaveDisk(const char* file_path, IDisk* disk, ILoadingProgress* loading_progress = nullptr) const = 0;
};

class DiskBuilder
{
public:
   typedef enum
   {
      READ = 1,
      WRITE = 2,
   } FormatAction;

   //////////////////////////////////////////////////////////
   // Constructor / destructor
   DiskBuilder(void);
   virtual ~DiskBuilder(void);

   //////////////////////////////////////////////////////////
   // Get a list of supported formats
   std::vector<FormatType*> GetFormatsList(DiskBuilder::FormatAction action);
   // 
   //////////////////////////////////////////////////////////
   // Disk creation methods 
   virtual bool CanLoad(const char* file_path, FormatType*& format);
   virtual int LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress = nullptr);
   virtual int LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                        ILoadingProgress* loading_progress = nullptr);
   virtual int LoadDisk(std::vector<FormatType::TrackItem> file_list, IDisk*& created_disk,
                        ILoadingProgress* loading_progress = nullptr);

   //////////////////////////////////////////////////////////
   // Save a disk
   virtual int SaveDisk(const char* file_path, IDisk* disk, const FormatType* format,
                        ILoadingProgress* loading_progress = nullptr);
   virtual int SaveDisk(const char* file_path, IDisk* disk, const char* format,
                        ILoadingProgress* loading_progress = nullptr);

private :
   std::vector<FormatType*> format_list_;
};
