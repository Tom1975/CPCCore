#ifndef CPCCORE_MEDIAMANAGER_H
#define CPCCORE_MEDIAMANAGER_H

#include "DiskContainer.h"
#include "IDisk.h"
#include "ILoadingProgress.h"
#include "DiskBuilder.h"

/////////////////////////////////////////////////
// 
class IMedia
{
public :
   IMedia(int type) : type_(type)
   {
   }

   virtual int GetType() { return type_; }

protected:
   int type_;
};

class Disk : public IMedia
{
public:
   Disk(int type, IContainedElement* contained_element): IMedia(type), contained_element_(contained_element)
   {
   }

   virtual IDisk* CreateDisk(ILoadingProgress* loading_progress, ILog* log = nullptr);
protected:
   IContainedElement* contained_element_;
   DiskBuilder disk_builder_;
};

class Snapshot : public IMedia
{
public:
   Snapshot(int type): IMedia(type)
   {
   }
};

class SnapshotReplay : public IMedia
{
public:
   SnapshotReplay(int type): IMedia(type)
   {
   }
};

class Tape : public IMedia
{
public:
   Tape(int type): IMedia(type)
   {
   }
};

class Binary : public IMedia
{
public:
   Binary(int type): IMedia(type)
   {
   }
};

class CPR : public IMedia
{
public:
   CPR(int type) : IMedia(type)
   {
   }
};

/////////////////////////////////////////////////
// 
class CPCCOREEMU_API MediaManager : public ITypeManager
{
public:

   enum MediaType
   {
      MEDIA_UNDEF = 0,
      // udnefined type
      MEDIA_SNA = 1,
      // Snapshot
      MEDIA_ROM,
      // ROM 
      MEDIA_DISK,
      // Disk image
      MEDIA_TAPE,
      // Tape image
      MEDIA_SNR,
      // Snapshot Replay
      MEDIA_BIN,
      // Binary file
      MEDIA_TRACK,
      // Single Track
      MEDIA_CPR,
      // Cartridge
   };

   MediaManager(DataContainer* container);
   virtual ~MediaManager();

   virtual int GetType(std::vector<MediaType> wanted_types);

   // Interface ITypeManager implementation
   virtual int GetTypeFromFile(const char* str);
   virtual int GetTypeFromBuffer(unsigned char* buffer, int size);
   virtual int GetTypeFromMultipleTypes(int* type_list, int nb_types);

   // Extract media types
   std::vector<IDisk*> GetDisk(ILoadingProgress* loading_progress, ILog* log = nullptr);

protected:
   ///////////////////////////////
   // Check the container for known media
   void Init();
   // Clear media list
   void ClearList();

   ///////////////////////////////
   // Pointer to data container
   DataContainer* container_;

   // List of media 
   std::vector<IMedia*> media_list_;
};

#endif  // CPCCORE_MEDIAMANAGER_H
