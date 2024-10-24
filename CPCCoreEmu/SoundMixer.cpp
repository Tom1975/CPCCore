// todo : include header
// todo : handle various output (8/16/32 bits, sample frequency, etc.)
// todo : record selected sources only

#include "stdafx.h"
#include "SoundMixer.h"

#include "simple_stdio.h"

#ifndef NOFILTER

#include <regex>
#include <cmath>

#include "mkfilter.h"
#endif


#ifdef  __circle__
#include <circle/timer.h>
#include <circle/spinlock.h>
static CSpinLock mutex_sound;
#endif

#ifdef LOG_MIXER
#define LOG(str)  if (log_) log_->WriteLog (str);
#define LOGEOL if (log_) log_->EndOfLine ();
#define LOGB(str)  if (log_) log_->WriteLogByte (str);
#else
#define LOG(str)
#define LOGB(str)
#define LOGEOL
#endif

double SoundSource::volume_[16] = {  
   0.0055242730304598808, 
   0.0078125018626451492, 
   0.011048545129597187,
   0.015625003725290298,
   0.022097090259194374, 
   0.031250007450580597, 
   0.044194180518388748,
   0.062500007450580597,
   0.088388353586196899,
   0.12500001490116119,
   0.17677670717239380, 
   0.25000002980232239,
   0.35355338454246521,
   0.50000005960464478,
   0.70710676908493042,
   1.0 };

SoundBuffer::SoundBuffer() : offset_(0)
{
}

SoundBuffer::~SoundBuffer()
{
}

void SoundBuffer::AddSound(double  left, double  right)
{
   buffer_left_[offset_] += left;
   buffer_right_[offset_] += right;
}

bool SoundBuffer::Advance()
{
   offset_++;
   return (offset_ < BUFFER_SIZE);
}

void SoundBuffer::InitBuffer()
{
   memset(buffer_left_, 0, sizeof(buffer_left_));
   memset(buffer_right_, 0, sizeof(buffer_right_));
   offset_ = 0;
}

#define ORDER 1
#define TYPE_OF_FILTER 1

SoundMixer::SoundMixer() : 
   log_(nullptr),
   sync_on_sound_(true),
   tape_adjust_volume_(0.02),
   start_recording_(false), 
   stop_recording_(false), 
   sample_number_(0),
#ifndef NO_MULTITHREAD
   worker_thread_(nullptr),
   finished_(true),
#else
   finished_(true),
#endif
   current_wav_buffer_(nullptr), 
   current_wav_index_(0),
   tape_(nullptr),
   record_(false)
   
   
{
   // Everything, except firt, is on the "free buffer" list
   buffer_list_ = new BufferItem[NB_BUFFERS];
   buffer_list_[0].Init();
   buffer_list_[0].status_ = BufferItem::IN_USE;
   buffer_list_[0].sample_number_ = sample_number_++;
   index_current_buffer_ = 0;

   for (int i = 1; i < NB_BUFFERS; i++)
   {
      buffer_list_[i].Init();
   }
   memset(buffer_left_, 0, sizeof(buffer_left_));
   memset(buffer_right_, 0, sizeof(buffer_right_));
   offset_buffer_to_convert_ = 0;

   xv_left_ = new double[ORDER *2 + 1];
   xv_right_ = new double[ORDER * 2 + 1];
   yv_left_ = new double[ORDER * 2 + 1];
   yv_right_ = new double[ORDER * 2 + 1];

  coefx_ = new double[ORDER * 2 + 1];
  coefy_ = new double[ORDER * 2 + 1];

   memset(xv_left_, 0, sizeof(double)*(ORDER * 2 + 1));
   memset(xv_right_, 0, sizeof(double)*(ORDER * 2 + 1));
   memset(yv_left_, 0, sizeof(double)*(ORDER * 2 + 1));
   memset(yv_right_, 0, sizeof(double)*(ORDER * 2 + 1));

   memset(coefx_, 0, sizeof(double)*(ORDER * 2 + 1));
   memset(coefy_, 0, sizeof(double)*(ORDER * 2 + 1));


#ifdef _WIN32
#ifndef _WIN64
   // Force 53 bits floating points...
   volatile short int  cw = 0x27F;
   volatile short int oldcw = 0;
   __asm {
      fstcw oldcw
      fldcw cw
   }
#endif
#endif

#ifndef NOFILTER
   // Compute gain and coeff.
   switch (TYPE_OF_FILTER)
   {
   case 01:
      ComputeBandPass(TYPE_OF_FILTER, ORDER, 125000, 20833, 12, &gain_, coefx_, coefy_);
      break;
   }
#endif

#ifdef _WIN32
#ifndef _WIN64
   __asm {
      fldcw oldcw
   }
#endif
#endif
}

SoundMixer::~SoundMixer()
{
#ifndef NO_MULTITHREAD
   if (worker_thread_ != nullptr)
   {
      // Stop the thread
      finished_ = true;
      worker_thread_->join();
      // Delete it
      delete worker_thread_;
   }
#else
   finished_ = true;
#endif

   delete[]buffer_list_;
   delete[]xv_left_;
   delete[]xv_right_;
   delete[]yv_left_;
   delete[]yv_right_;
   delete[]coefx_;
   delete[]coefy_;
}


void SoundMixer::FiltrerOnSamples(double* array_left, double* array_right, unsigned int nb_samples)
{
#ifndef NOFILTER
#ifdef _WIN32
#ifndef _WIN64
   volatile short int  cw = 0x27F;
   volatile short int oldcw = 0;
   // Force 53 bits floating points...
   __asm {
      fstcw oldcw
      fldcw cw
   }
#endif
#endif
#endif

   // Can be replaced by....
   //_controlfp_s ( &currCtrl, _PC_53 , _MCW_PC  );

      // Lets apply theses values.
   for (unsigned int i = 0; i < nb_samples; i++)
   {
      // Shift values
      for (int j = 0; j < ORDER * 2; j++)
      {
         xv_left_[j] = xv_left_[j + 1];
         xv_right_[j] = xv_right_[j + 1];
         yv_left_[j] = yv_left_[j + 1];
         yv_right_[j] = yv_right_[j + 1];
      }

      xv_left_[ORDER * 2] = array_left[i] / gain_;
      xv_right_[ORDER * 2] = array_right[i] / gain_;
      // Compute output/last yv value
      double yvout_left = 0;
      double yvout_right = 0;
      for (int j = 0; j < ORDER * 2; j++)
      {
         yvout_left += xv_left_[j] * coefx_[j];
         yvout_left += yv_left_[j] * coefy_[j];

         yvout_right += xv_right_[j] * coefx_[j];
         yvout_right += yv_right_[j] * coefy_[j];
      }
      yvout_left += xv_left_[ORDER * 2];
      yv_left_[ORDER * 2] = yvout_left;
      array_left[i] = yvout_left;

      yvout_right += xv_right_[ORDER * 2];
      yv_right_[ORDER * 2] = yvout_right;
      array_right[i] = yvout_right;
   }
#ifdef _WIN32
#ifndef _WIN64
   __asm {
      fldcw oldcw
   }
#endif
#endif
}

void PrepareBuffer(SoundMixer* sound_hub)
{
   sound_hub->PrepareBufferThread();
}

void SoundMixer::StopMixer()
{
#ifndef NO_MULTITHREAD
   if (worker_thread_ != nullptr)
   {
      finished_ = true;
      worker_thread_->join();
      delete worker_thread_;
      worker_thread_ = nullptr;
   }
#else
   finished_ = true;
#endif
}

void SoundMixer::StartMixer()
{
#ifndef NO_MULTITHREAD
   finished_ = false;
   worker_thread_ = new std::thread(PrepareBuffer, this);
#else
   finished_ = false;
#endif
}


void SoundMixer::Init(ISound* sound, IExternalSource* tape)
{
   tape_ = tape;
#ifndef NO_MULTITHREAD
   if (worker_thread_ != nullptr)
   {
      finished_ = true;
      worker_thread_->join();
      delete worker_thread_;
      worker_thread_ = nullptr;
   }
   finished_ = false;
#else
   finished_ = false;
#endif
   sound_ = sound;
#ifndef NO_MULTITHREAD
   if (sound_ != nullptr)
   {
      worker_thread_ = new std::thread(PrepareBuffer, this);
   }
#endif
}

void SoundMixer::AddSound(double  volume_left, double  volume_right)
{
   buffer_list_[index_current_buffer_].buffer_.AddSound(volume_left, volume_right);
}

//
// The downsampling (if needed) has to be rework with more math !
void SoundMixer::ConvertToWav(SoundBuffer* buffer_in)
{
   if (start_recording_ && !record_)
   {
      start_recording_ = false;
      BeginRecordImp();
   }

   if (stop_recording_ && record_)
   {
      stop_recording_ = false;
      EndRecordImp();
   }

   double *float_buffer_l = buffer_in->GetLeftBuffer();
   double *float_buffer_r = buffer_in->GetRightBuffer();

   // filter
#ifndef __circle__
   FiltrerOnSamples(float_buffer_l, float_buffer_r, BUFFER_SIZE);
#endif
   if (current_wav_buffer_ == nullptr)
   {
      // No buffer ? just wait until there is one (discard current sound)
      current_wav_buffer_ = sound_->GetFreeBuffer();
   }
   else
   {

      short* data = (short*)current_wav_buffer_->data_;

      // Copy input buffer to current stream
      if (offset_buffer_to_convert_ > BUFFER_SIZE)
         offset_buffer_to_convert_ = 0;

      //int maxValue = (1 << (sound_->GetBitDepth() ))-1;
      int maxValue = sound_->GetMaxValue();;
      int minValue = sound_->GetMinValue();;

      for (int i = 0; i < BUFFER_SIZE; i++)
      {
         buffer_left_[offset_buffer_to_convert_ + i] = minValue + float_buffer_l[i] * (maxValue- minValue);
         buffer_right_[offset_buffer_to_convert_ + i] = minValue + float_buffer_r[i] * (maxValue - minValue);
      }
      
 
      offset_buffer_to_convert_ += BUFFER_SIZE;
      // From convert_offset_  to 1024
      double div = 125000.0 / sound_->GetSampleRate(); // 44100.0;
      double add = 0.0;
      int left = 0, right = 0;
      int total_added = 0;
      int i = 0;
      for (i = 0; i < offset_buffer_to_convert_; )
      {
         while (add < div && total_added + i < offset_buffer_to_convert_)
         {
            left += static_cast<int>(buffer_left_[i + total_added]);
            right += static_cast<int>(buffer_right_[i + total_added]);

            total_added++;
            add++;
         }
         i += total_added;
         if (add >= div  && total_added != 0)
         {
            // If ok, add it
            left /= total_added;
            right /= total_added;

            if (right > 0x7FFF) right = 0x7FFF;
            if (right < -0x7FFF) right = -0x7FFF;
            if (left > 0x7FFF) left = 0x7FFF;
            if (left < -0x7FFF) left = -0x7FFF;
            AddRecord(left, right);
            data[current_wav_index_++] = (short)left;
            data[current_wav_index_++] = (short)right;
            //if (((current_wav_index_ + 2) * sizeof(short) ) > (unsigned int)current_wav_buffer_->buffer_length_)
            if (((current_wav_index_ + 2) * sizeof(short) ) > current_wav_buffer_->buffer_length_)
            {
               // Play it and set a new one
               sound_->AddBufferToPlay(current_wav_buffer_);

               current_wav_buffer_ = sound_->GetFreeBuffer();
               current_wav_index_ = 0;
               if (current_wav_buffer_ == nullptr)
               {
                  // No more buffer : Clear and quit
                  break;
               }
               data = (short*)current_wav_buffer_->data_;
            }
            total_added = 0;
            left = right = 0;
            add -= div;
         }
         else
         {
            // Otherwise, not enough samples : Keep them for next turn
            break;
         }
      }
      offset_buffer_to_convert_ -= i;
      // Copy remaining inbuffer, and keep it
      if (offset_buffer_to_convert_ > 0)
      {
         for (int j = 0; j < offset_buffer_to_convert_; j++)
         {
            buffer_left_[j] = buffer_left_[i+j];
            buffer_right_[j] = buffer_right_[i + j];
         }
      }
   }
}

void SoundMixer::PrepareBufferThread()
{
   sound_->Reinit();

   if (current_wav_buffer_ == nullptr)
   {
      current_wav_buffer_ = sound_->GetFreeBuffer();
      current_wav_index_ = 0;
   }

#ifndef NO_MULTITHREAD
   while (!finished_)
#endif
   {
      // New buffer is          ready ?
      int index_to_convert = -1;
      int sample_number = -1;

      // Synchronise (todo)

      for (int i = 0; i < NB_BUFFERS; i++)
      {
         if (buffer_list_[i].status_ == BufferItem::TO_PLAY 
         && (sample_number == -1 || buffer_list_[i].sample_number_ <= sample_number))
         {
            sample_number = buffer_list_[i].sample_number_;
            index_to_convert = i;
         }
      }

      if (index_to_convert != -1)
      {
         buffer_list_[index_to_convert].status_ = BufferItem::LOCKED;

         // Release mutex (todo)

         ConvertToWav(&buffer_list_[index_to_convert].buffer_);

         // Put it back to 'free_buffers_'
         buffer_list_[index_to_convert].buffer_.InitBuffer();
#ifdef  __circle__
         mutex_sound.Acquire();
#endif
         buffer_list_[index_to_convert].status_ = BufferItem::FREE;
#ifdef  __circle__
         mutex_sound.Release();
#endif
      }
      else
      {
         // Release mutex (todo)
         sound_->CheckBuffersStatus();
#ifndef NO_MULTITHREAD
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
#elif __circle
         CTimer::Get()->MsDelay(1);
#endif
      }
   }
}

bool SoundMixer::GetNewSoundFile(char * buffer, unsigned int size)
{
#ifndef RASPPI
   bool name_is_ok = false;

   const fs::path exe_path =  "./REC/";

   // Create record directory if necessary
   if (!fs::exists(exe_path))
   {
      fs::create_directories(exe_path);
   }
   // Create new sound file
   unsigned int inc = 0;

   while (!name_is_ok && inc <= 9999)
   {
      char sound_name[16];
      sprintf(sound_name, "SND%4.4i.WAV", inc++);

      fs::path sound_file = exe_path / fs::path(sound_name);

      if ( !fs::exists(sound_file))
      {
         name_is_ok = true;
         strncpy(buffer, sound_file.string().c_str(), size);
      }

   }
   return name_is_ok;
#else
	strcpy(buffer, "/REC/SoundOutput.wav");
	return true;
#endif
}


void SoundMixer::AddRecord(short left, short right)
{
   if (record_)
   {
      unsigned char buff[4];

      buff[0] = left & 0xFF;
      buff[1] = left >> 8;
      buff[2] = right & 0xFF;
      buff[3] = right >> 8;

      fwrite(buff, 1, 4, rec_file_);
      //fflush(rec_file_);
      data_rec_size_++;
   }
}

void SoundMixer::BeginRecord()
{
   start_recording_ = true;
}

void SoundMixer::BeginRecordImp()
{
   if (!record_)
   {
      char path_file[MAX_PATH];
      // Prepare new file
      if (GetNewSoundFile(path_file, MAX_PATH))
      {
         // Open a new file
         fopen_s(&rec_file_, path_file, "wb");

         if (rec_file_ != NULL)
         {
            Wave wav;
            InitWave(&wav, 0);

            fwrite(&wav, 1, sizeof(wav), rec_file_);

            data_rec_size_ = 0;

            // Record ok
            record_ = true;
         }
      }
   }
}
void SoundMixer::InitWave(Wave* wav, unsigned int length)
{
   /* Les constantes symboliques */
   memset(wav, 0, sizeof(Wave));
   memcpy(wav->riff, "RIFF", 4);
   memcpy(wav->wave, "WAVE", 4);
   memcpy(wav->fmt, "fmt ", 4);
   memcpy(wav->data, "data", 4);

   wav->sub_taille1 = 16;
   wav->format_audio = 1;
   wav->nombre_canaux = 2;
   wav->freq_ech = sound_->GetSampleRate();
   wav->bits_par_ech = sound_->GetBitDepth();
   wav->byte_rate = wav->freq_ech * wav->nombre_canaux * (wav->bits_par_ech / 8);
   wav->align = wav->nombre_canaux * (wav->bits_par_ech / 8);

   wav->sub_taille2 = length * wav->nombre_canaux * (wav->bits_par_ech / 8);
   wav->taille = wav->sub_taille2 + 44 - 8;
}

void SoundMixer::EndRecordImp()
{
   if (record_)
   {
      // Stop recording
      record_ = false;

      Wave wav;
      InitWave(&wav, data_rec_size_);

      // Taille du fichier a ecrire (octets 4-8)
      fseek(rec_file_, 0, SEEK_SET);
      fwrite(&wav, sizeof(wav), 1, rec_file_);

      // Flush and close the file properly
      fclose(rec_file_);
   }
}


// Sound Mixer : The soundmixer tick is 8us (125 khz)
// This is a pragmatic value, set because it is the tick rate of AY8912 used by both CPC and PlayCITY
unsigned int SoundMixer::Tick()
{
   if (tape_ && tape_->SoundOn())
   {
      double tapeSnd = tape_->GetSoundVolume() * tape_adjust_volume_;
      buffer_list_[index_current_buffer_].buffer_.AddSound(tapeSnd, tapeSnd);
   }

   // Advance
   if (buffer_list_[index_current_buffer_].buffer_.Advance() == false )
   {
      // Synchronize on Sound ?
      // Synchronize
      int free_buffer = 0;
      while ( sync_on_sound_ && free_buffer < NB_BUFFERS-4)
      {
#ifdef  __circle__
         mutex_sound.Acquire();
#endif
         free_buffer = NB_BUFFERS;
         for (int i = 0; i < NB_BUFFERS; i++)
         {
            if (buffer_list_[i].status_ != BufferItem::FREE)
            {
               free_buffer--;
            }
         }
#ifdef  __circle__
         mutex_sound.Release();
#endif
         #ifndef NO_MULTITHREAD
                  std::this_thread::sleep_for(std::chrono::milliseconds(1));
         #elif __circle
                  CTimer::Get()->MsDelay(1);
         #endif
      } 
      int next_to_play = -1;
      for (int i = 0; i < NB_BUFFERS; i++)
      {
         if (buffer_list_[i].status_ == BufferItem::FREE)
         {
            next_to_play = i;
            break;
         }

      }

      if (next_to_play != -1)
      {
         // Buffer is full ? Prepare next, and mark this one to be played
         buffer_list_[index_current_buffer_].status_ = BufferItem::TO_PLAY;

         index_current_buffer_ = next_to_play;
//#endif
         if (buffer_list_[index_current_buffer_].status_ != BufferItem::FREE)
         {
            buffer_list_[index_current_buffer_].buffer_.InitBuffer();
         }
      }
      else
      {
         buffer_list_[index_current_buffer_].buffer_.InitBuffer();
      }
      buffer_list_[index_current_buffer_].status_ = BufferItem::IN_USE;
      buffer_list_[index_current_buffer_].sample_number_ = sample_number_++;
   }

   return 32; // 8 us
}

SoundSource::SoundSource(SoundMixer * sound_hub): sound_mixer_(sound_hub)
{
#ifndef RASPPI
   for (int i = 0; i < 16; i++)
   {
      volume_[i] = ((1.0f / (pow(sqrt(2.0f), (15 - i))))) ;
   }
#endif
}

void SoundSource::AddSound(int volume_left, int volume_right, int volume_center)
{
   double vl = volume_[volume_left] * (2.0/3.0);
   double vr = volume_[volume_right] * (2.0 / 3.0);
   double vc = volume_[volume_center] / 3.0;

   sound_mixer_->AddSound( vl + vc, vr + vc);
}

void SoundSource::AddSoundLeft(int volume_1, int volume_2, int volume_3)
{
   sound_mixer_->AddSound((volume_[volume_1] + volume_[volume_2] + volume_[volume_3]) / 3, 0);
}

void SoundSource::AddSoundRight(int volume_1, int volume_2, int volume_3)
{
   sound_mixer_->AddSound(0, (volume_[volume_1] + volume_[volume_2] + volume_[volume_3]) / 3);
}
