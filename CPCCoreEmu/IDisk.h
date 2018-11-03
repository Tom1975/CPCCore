#pragma once
//////////////////////////////////////////////////////////
// IDisk.h
//
// Generic disk interface.
//
// (c) T.Guillemin 2018
//
//
//////////////////////////////////////////////////////////

#include "includes.h"
#include "ILog.h"
#include "CRC.h"
#include <string>

///////////////////////////////////////////////////////////////
// Generic define for MFM
#define DAAM_OFF       0xFB
#define DAAM_ERASED    0xF8

// Only data bits here !
#define SYNC_BIT_NUMBER 40
#define SYNC_BYTE 0x01
#define WEAK_BYTE 0x02

#define DEFAULT_TRACK_SIZE 16384


// BIT position return :

#define BIT_0           0     // 0
#define BIT_1           1     // 1
#define BIT_INDEX       2     // Index hole
#define BIT_WEAK        4     // Weak bit
#define BIT_OPTIONAL    8     // Optional bit : Sometime present, sometime not

// IDisk
class IDisk
{
public:
   enum DiskType
   {
      VENDOR,
      DATA
   };

   IDisk();
   IDisk(IDisk::DiskType disk_type);
   virtual ~IDisk();

   ///////////////////////////////////////////////////////////////
   //
   enum FaceSelection
   {
      NO_FACE = 0,
      FACE_1 = 1,
      FACE_2 = 2,
      FACE_BOTH = 3,
   };

   typedef enum
   {
      AUTO_UNKNOWN = 0,
      AUTO_CPM = 1,
      AUTO_FILE = 2
   } AutorunType;

   // Format support
   const char* GetCurrentLoadedDisk() { return current_disk_path_.c_str(); };
   virtual IDisk::FaceSelection FilterSide(IDisk::FaceSelection face_selection);

   virtual void SetWrite(int side, unsigned int track);
   virtual void DumpTrack(int side, unsigned int track, int rev = 0);
   virtual void DumpTrackRev(int side, unsigned int track, int rev);

   virtual int GetCurrentLoadProgress() { return load_progress_; }

   virtual void CombineWithDisk(IDisk* other_disk);

   virtual void CleanSide(int side);
   virtual void CleanDisk();
   void SetFixedSpeed(bool fixed_speed) { fixed_speed_ = fixed_speed; };

   // Change track
   virtual void ChangeTrack(int side, int new_track);

   // MFM track accessor
   virtual unsigned char GetNextBit(int side, int track);
   virtual void WriteBit(int side, int track, unsigned char bit);

   // Structure accessor
   void SetLog(ILog* log) { log_ = log; };

   virtual int GetNumberOfSide() { return nb_sides_; };
   virtual unsigned int GetNumberOfTracks(int side) { return side_[side].nb_tracks; };
   virtual unsigned int GetNumberOfTracks() { return side_[0].nb_tracks > side_[1].nb_tracks ? side_[0].nb_tracks : side_[1].nb_tracks; };

   virtual unsigned int GetNumberOfRevolutions(int side, int track)
   {
      return side_[side].tracks[track].nb_revolutions;
   };

   virtual bool IsFixedStatus() { return false; };

   bool IsDiskModified() const { return disk_modified_; };

   std::vector<std::string>  GetCat(IDisk::AutorunType& autorun_type, int user=0);

   ///////////////////////////////////////////////////////////////
   // Usefull generic fonctions
   static void FillTrack(unsigned char* track, const unsigned char cylinder, IDisk::DiskType type,
                         unsigned int size_of_track);

   static int AddByteWithoutCrc(unsigned char* track, int index, unsigned char b, bool sync = false);
   static int AddByteWithCrc(unsigned char* track, int index, unsigned char b, CRC& crc, bool sync = false);

   static bool MFMCorrect(const unsigned char pattern[16], unsigned char previous_bit);
   static unsigned char ConvertMFMToByte(const unsigned char pattern[16]);
   static unsigned char ConvertToByte(const unsigned char pattern[8]);
   static bool NoWeakBits(unsigned char* buffer, int offset, int size);

   virtual int GetPos() { return head_position_; };
   int GetSizeOfTrack(int side, int track) { return side_[side].tracks[track].size; }

   int AddByteToSpecificTrack(int side, unsigned int track, unsigned int index, unsigned char byte, int size = 16);
   static int AddByteToTrack(unsigned char* track, unsigned int index, unsigned char byte, int size = 8,
                             unsigned char previous_bit = 0);
   static int AddSyncByteToTrack(unsigned char* track, unsigned int index, unsigned char byte);
   int AddMFMByteToAllTrack(int side, int track, unsigned int index, unsigned char byte, int data_size);
   int AddMFMByteToSpecificTrack(int side, int track, int rev, unsigned int index, unsigned char byte, int data_size);

   static int CompareForSync(unsigned char* src, unsigned char* src2, int size);

   virtual void CreateSingleTrackFromMultiRevolutions(int side, int track);
   virtual void CreateTrackFromMultiRevolutions();
   virtual bool CreateTrackFromRevolution(int side, int track);
   virtual int CreateTrackFromRevolutionWithSize(int side, int track, int size_of_track);
   virtual int CreateUnformattedTrack(int side, int track);

   unsigned char nb_sides_;
   ILog* log_;

   // Used for EDSK saving
   typedef struct
   {
      unsigned int idam_offset;
      unsigned int idam_sync_offset;
      unsigned char c;
      unsigned char h;
      unsigned char r;
      unsigned char n;
      bool chrn_crc_ok;
      unsigned char status1;
      unsigned char status2;
      unsigned short real_size;
      unsigned int dam_offset;
      unsigned int dam_sync_offset;
      bool data_crc_ok;
      unsigned int index_end;
   } Sector;

   class Track
   {
   public:

      void Clear()
      {
         list_sector_.clear();
         full_size_ = 0;
         gap3_size_ = 0;
      };
      std::vector<Sector> list_sector_;
      int full_size_;
      int gap3_size_;

      bool GetEndIndexOfSector(unsigned char C, unsigned char H, unsigned char R, unsigned char N, bool bHeader,
                               int* end_index)
      {
         for (auto& it : list_sector_)
         {
            if (it.c == C
               && it.h == H
               && it.r == R
               && it.n == N)
            {
               *end_index = (bHeader) ? it.idam_offset + 160 : it.index_end;
               return true;
            }
         }
         return false;
      }

      bool GetRFromIndex(unsigned int index, unsigned char* c, unsigned char* h, unsigned char* r, unsigned char* n,
                         bool* header)
      {
         for (auto& it : list_sector_)
         {
            // 16*12 = sync bytes !
            if (index >= it.idam_sync_offset && index < it.idam_offset + 160)
            {
               *header = true;
               *c = it.c;
               *h = it.h;
               *r = it.r;
               *n = it.n;
               return true;
            }
            if (index >= it.dam_sync_offset && index < it.index_end)
            {
               *header = false;
               *c = it.c;
               *h = it.h;
               *r = it.r;
               *n = it.n;
               return true;
            }
         }
         return false;
      }

      bool IsBitInACorrectSector(unsigned int index, unsigned int* index_end)
      {
         for (auto& it : list_sector_)
         {
            // 16*12 = sync bytes !
            if (index >= it.idam_sync_offset && index < it.idam_offset + 160)
            {
               // Here
               *index_end = it.idam_offset + 160;
               if (it.chrn_crc_ok) return true;
            }
            if ((index >= it.dam_sync_offset && index < it.index_end)
               /*|| (it->indexEnd < it->DAMOffset && index < it->indexEnd)*/)
            {
               // Here
               *index_end = it.index_end;
               if (it.data_crc_ok) return true;
            }
         }
         return false;
      };
   };

   typedef struct
   {
      unsigned char* bitfield;
      unsigned int size;
      Track track_info;
   } Revolution;

   typedef struct
   {
      Revolution* revolution;
      unsigned int nb_revolutions;
      // These data are final data. Revolution will disapear !
      unsigned char* bitfield;
      unsigned int size;
   } MFMTrack;

   // MFM Tracks
   typedef struct
   {
      MFMTrack* tracks;
      unsigned int nb_tracks;
   } Side;

   Side side_[2]{};

   // Current head position
   unsigned int head_position_;

   bool disk_modified_;

   unsigned int GetTrackInfo(MFMTrack* mfm_track, Track* track_buffer);
   unsigned short GetTrackInfo(int side, int track, Track* track_buffer);
   unsigned short GetTrackInfoForRev(int side, int track, Track* track_buffer, int rev = 0);
   unsigned char GetNextByte(int side, unsigned int track, unsigned int offset) const;
   unsigned char GetNextByte(MFMTrack* track, unsigned int offset);
   unsigned char GetNextByteForRev(int side, unsigned int track, unsigned int offset, int rev);

   unsigned int AdjustSideOfTrack(unsigned char* bit_field, unsigned int nb_bit_in_flux);

   int FindEndOfWeakArea(int side, int track, unsigned int* current_search_rev, unsigned int* next_correct_index,
                         unsigned int nb_used_revolutions, int* rev_to_use, bool revolution_wrecked[16]);

   bool fixed_speed_;

   int CompareToDisk(IDisk* other_disk, bool exact = false);
   int CompareTracks(IDisk::MFMTrack* track1, IDisk::MFMTrack* track2);
   bool TrackFormatted(IDisk::MFMTrack* track);

   // Progress
   unsigned int load_progress_;

public:
   void SetName(const char* new_file_path);
   //protected:
   int SmartOpen(FILE** file, const char* file_path, const char* file_ext);
private:
   std::string current_disk_path_;
};


unsigned int FindNextPattern(unsigned char* buffer, unsigned int size, unsigned int offset,
                             const unsigned char* pattern, unsigned int size_of_pattern);
unsigned int FindNextSector(unsigned char* buffer, unsigned int size, unsigned int offset, int size_to_search);
unsigned int GetNextFuzzyBits(unsigned char* buffer, unsigned int offset_begin, unsigned int offset_end,
                              bool search_for_fuzzy);

#ifdef __MORPHOS__
const char * GetFilePart(const char *path);
bool IsDirectory(const char *path);
#endif
