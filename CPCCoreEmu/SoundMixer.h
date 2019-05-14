#pragma once

/*
 * (C) Copyright 2017 Thomas GUILLEMIN
 * 
 * 
 *
 */
#include "ISound.h"
#include "IComponent.h"

#include "simple_stdio.h"

#ifndef MINIMUM_DEPENDENCIES
#include <atomic>
#include <thread>
#endif


class SoundBuffer 
{
public:
   SoundBuffer();
   virtual ~SoundBuffer();

   virtual void InitBuffer();
   virtual void AddSound(double  left, double  right);
   virtual bool Advance();
   double * GetLeftBuffer() { return buffer_left_; }
   double * GetRightBuffer() { return buffer_right_; }
protected:
#define BUFFER_SIZE 2500
   double buffer_left_[BUFFER_SIZE];
   double buffer_right_[BUFFER_SIZE];
   int offset_;
};

class SoundMixer : public ISoundMixer
{
public:
   SoundMixer();
   virtual ~SoundMixer();

   virtual void StopMixer();
   virtual void StartMixer();

   void Init(ISound* sound, IExternalSource* tape = nullptr);

   // Add sound
   void AddSound(double  volume_left, double  volume_right);
   unsigned int Tick();
   void PrepareBufferThread();

   void BeginRecord();
   void EndRecord() {
      stop_recording_ = true;
   };
   bool IsRecording() { return record_; };

protected:

   // Filtering on output
   void FiltrerOnSamples(double* array_left, double* array_right, unsigned int nb_samples);

   double tape_adjust_volume_;
   double *xv_left_;
   double *xv_right_;
   double *yv_left_;
   double *yv_right_;

   double gain_;
   double * coefx_;
   double * coefy_;

   bool start_recording_;
   bool stop_recording_;

   void BeginRecordImp();
   void EndRecordImp();
   struct Wave {
      char riff[4];        // RIFF chunk 
      int taille;          // La taille du fichier - 8 Ou subTaille2 + 44 - 8
      char wave[4];        // "WAVE"
      char fmt[4];         // "fmt "
      int sub_taille1;      // Header size until "data"
      short format_audio;   // File format
      short nombre_canaux;  // Channel number
      int freq_ech;         // Sample frequency
      int byte_rate;        // Byte rate : number of octet per second.freqEch * nombreCanaux * bitsParEch / 8
      short align;         // Number of octet by sample. nombreCanaux * bitsParEch / 8 
      short bits_par_ech;    // Bits par échantillon
      char data[4];       // "data" word
      int sub_taille2;      // data size
   };

   // Buffers pool
   class BufferItem
   {
   public:
      BufferItem() {}
      virtual ~BufferItem(){}
      void Init() {
         status_ = FREE; buffer_.InitBuffer();
         sample_number_ = 0;
      }

      enum BufferState {
         FREE,
         IN_USE,
         LOCKED,
         TO_PLAY,
      };

      SoundBuffer buffer_;
      BufferState status_;
      int sample_number_;
   };

   BufferItem buffer_list_[16];
   // Current buffer lists
   int index_current_buffer_;

   ISound* sound_;
   int sample_number_;

#ifndef NO_MULTITHREAD
   std::thread * worker_thread_;
   std::atomic_bool finished_;
#endif

   /////////////////////// Conversion 
   // Convertion of full buffer
   void ConvertToWav(SoundBuffer* buffer_in);
   double buffer_left_[BUFFER_SIZE*2];
   double buffer_right_[BUFFER_SIZE*2];
   int offset_buffer_to_convert_;
   IWaveHDR* current_wav_buffer_;
   int current_wav_index_;

   ///////////////////////
   // Tape reference
   IExternalSource* tape_;

   ///////////////////////
   // Record to wav file
   bool GetNewSoundFile(char * buffer, unsigned int size);
   void AddRecord(short left, short right);
   void InitWave(Wave* wav, unsigned int length);

   bool record_;
   FILE * rec_file_;
   unsigned int data_rec_size_;

};

class SoundSource 
{
public:
   SoundSource(SoundMixer * sound_hub);

   virtual void AddSound(int volume_left, int volume_right, int volume_center);
   virtual void AddSoundLeft(int volume_1, int volume_2, int volume_3);
   virtual void AddSoundRight(int volume_1, int volume_2, int volume_3);

   inline void Reinit() {}
protected:
   // Counter for buffer creation
   double volume_[16];

   SoundMixer * sound_mixer_;

};
