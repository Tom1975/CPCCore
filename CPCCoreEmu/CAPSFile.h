/*MIT License

Copyright (c) 2017 Thomas GUILLEMIN

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/


#pragma once

#include <vector>
#include "ILoadingProgress.h"
#include "IDisk.h"


#pragma pack(push, 4)

class CAPSFile
{
public:
   CAPSFile();
   virtual ~CAPSFile();

   virtual void Clear();

   // Create disk from buffer
   virtual int ReadBuffer(const unsigned char* buffer, unsigned int size_of_buffer, IDisk* disk,
                          ILoadingProgress* loading_progress = nullptr);

   // Create inner data from IDisk
   virtual int CreateIpf(IDisk* disk);
   virtual unsigned int ComputeSize();
   virtual int WriteToBuffer(unsigned char* buffer, unsigned int size_of_buffer);

protected:

   typedef struct
   {
      unsigned char type[4];
      unsigned int length;
      unsigned int crc;
   } RecordHeader;

   typedef struct
   {
      RecordHeader header;
      unsigned int media_type;
      unsigned int encoder_type;
      unsigned int encoder_rev;
      unsigned int file_key;
      unsigned int file_rev;
      unsigned int origin_crc32;
      unsigned int track_min;
      unsigned int track_max;
      unsigned int side_min;
      unsigned int side_max;
      unsigned int creation_date;
      unsigned int creation_time;
      unsigned int platform_1;
      unsigned int platform_2;
      unsigned int platform_3;
      unsigned int platform_4;
      unsigned int disk_number;
      unsigned int creator_id;
      unsigned int reserved;
      unsigned int reserved_2;
      unsigned int reserved_3;
   } RecordInfo;

   typedef struct
   {
      unsigned int size;
      unsigned char sample_size;
      unsigned char sample;
   } StreamItem;


   virtual bool ReadRecordHeader(const unsigned char* buffer, RecordHeader* header_out, size_t size);
   virtual bool GetStreamGapItem(CAPSFile::StreamItem* item, unsigned char* buffer, unsigned int& offset,
                                 unsigned int size_of_buffer);
   virtual int CreateData(IDisk* disk, int side, unsigned int track, int datakey);
   virtual bool NoWeakBitInPattern(unsigned char* pattern, unsigned int size_of_pattern);

   typedef struct
   {
      RecordHeader header;
      int track_number;
      int side;
      int density;
      int signal_type;
      int track_byte;
      int start_byte_pos;
      unsigned int start_bit_pos;
      int data_bits;
      int gap_bits;
      int track_bits;
      DWORD block_count;
      int encoder_process;
      int track_flags;
      DWORD data_key;
      unsigned char reserved[12];
   }ImgeItem;


   // List of DATA chunks
   typedef struct
   {
      RecordHeader header;
      DWORD size;
      DWORD bit_size;
      DWORD data_crc;
      DWORD data_id;
      unsigned char* buffer;
   } DataChunks;

   typedef struct
   {
      unsigned int block_size;
      unsigned int gap_size;
   } EncoderCapsBlockDescriptor;

   typedef struct
   {
      unsigned int gap_offset;
      unsigned int cell_type;
   } EncoderSpsBlockDescriptor;

   typedef struct
   {
      unsigned int block_bits;
      unsigned int gap_bits;
      EncoderSpsBlockDescriptor sps_specific;
      unsigned int encoder_type;
      unsigned int block_flag;
      unsigned int gap_value;
      unsigned int data_offset;
   } BlockDescriptor;

   // TRACKS datas 
   typedef struct
   {
      unsigned int ctype;
      unsigned int cyl;
      unsigned int head;
      unsigned int did;
   } Trackchunk;

   typedef struct
   {
      unsigned char* buffer;
      unsigned int size;
      unsigned int offset;
   } StreamRecording;

   int encoder_type_;

   // Record header
   RecordHeader caps_header_{};
   RecordInfo caps_info_{};

   std::vector<ImgeItem> list_imge_;
   std::vector<DataChunks> list_data_;
   std::vector<Trackchunk> list_tracks_;

   bool DecodeData(DataChunks item, IDisk* disk);
   int DecompressData(IDisk* disk, DWORD ctype, DWORD cyl, DWORD head, DataChunks image_item);
   unsigned int* ExtractDensity(unsigned char* buffer, int density_buf_size, DWORD ctype, DWORD cyl, DWORD head);
   int ExtractTrack(IDisk* disk, unsigned char* buffer, int density_buf_size, DWORD ctype, DWORD cyl, DWORD head);
   int AddGapRecord(StreamRecording* record, int nb_byte_to_add, unsigned char byte);
   int AddDataRecord(StreamRecording* record, unsigned char type, int nb_byte_to_add, unsigned char* buffer,
                     bool byte_size);
   int AddDataRecordInBits(StreamRecording* record, unsigned char type, unsigned int nb_byte_to_add,
                           unsigned char* buffer);
   bool AddStreamRecord(StreamRecording* record, unsigned char* buffer, unsigned int offset_begin,
                        unsigned int offser_end, unsigned int size_of_track);
   int AddShortStreamRecord(StreamRecording* record, unsigned char* buffer, unsigned int offset_begin,
                            unsigned int offset_end);
};

#pragma pack ( pop )