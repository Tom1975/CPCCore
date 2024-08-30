#pragma once

#include <stdio.h>

#include "IPlayback.h"
#include "Inotify.h"
#include "ILog.h"

class Motherboard;

class CSnapshot : public IPlayback
{
public:
   CSnapshot(ILog* log);
   virtual ~CSnapshot(void);

   virtual void StopPlayback (){replay_ = false;}
   virtual void StartRecord (const char* path_file);
   virtual void StopRecord ();
   bool IsRecording() { return record_; }
   bool IsReplaying() { return replay_; }

   bool LoadSnr (const char* path_file);
   bool LoadSnapshot (const char* path_file);
   bool SaveSnapshot (const char* path_file);
   void WriteSnapshotV3 ( FILE * f, unsigned char * base_header, unsigned int header_size );

   bool HandleSnr ( FILE* f );
   void HandleChunkBRKC(unsigned char* chunk, unsigned char* buffer, int size);
   void HandleChunkBRKS(unsigned char* chunk, unsigned char* buffer, int size);
   void HandleChunkCPCPLUS(unsigned char* chunk, unsigned char* buffer, int size);
   void HandleChunkMem ( unsigned char* chunk, unsigned char* buffer, int size );
   void HandleChunkROMS(unsigned char* chunk, unsigned char* buffer, int size);
   void HandleChunkSYMB(unsigned char* chunk, unsigned char* buffer, int size);
   void LoadStdSna ( unsigned char *header, FILE* f);

   virtual void SetMachine (Motherboard* machine) {machine_ = machine;}
   virtual void Playback ();
   void SetNotifier(IFdcNotify * notifier) { notifier_ = notifier; }

protected:
   Motherboard* machine_;
   IFdcNotify * notifier_;
   ILog* log_;
   void InitRecord ();
   void InitReplay ();

// Replay
   // Auto replay feature : The keystrokes.
   unsigned char* replay_buffer_;
   unsigned int replay_offset_;
   unsigned int replay_size_;
   bool start_record_;
   bool start_replay_;
   bool replay_;
   bool record_ ;
   int current_frame_to_wait_;

   unsigned char last_keystroke_[0xFF];
   unsigned char last_keystroke_size_;

   unsigned char key_buffer_[10];

   void CheckBufferSize ( int size_to_add = 1);
   unsigned char *record_buffer_;
   unsigned int record_buffer_size_;
   unsigned int record_buffer_offset_;
   FILE * record_file_;

   unsigned char crtc_sync_v_count_;
   unsigned char crtc_sync_h_count_;
   unsigned char lsb_total_cycles_;
   unsigned char lsb_pc_;

   std::string snr_filepath_ ;
};

