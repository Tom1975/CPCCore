#pragma once
#include "ICfg.h"

class IWaveHDR
{
public:
   enum HDRStatus
   {
      UNUSED = 0,
      USED = 1,
      INQUEUE = 2,
      DONE = 2
   };

   char * data_;
   int buffer_length_;

   HDRStatus status_;
};

class ISoundMixer
{
public:
   virtual void StopMixer()=0;
   virtual void StartMixer() = 0;
};

class ISound : public ICfg
{
public:
   virtual bool Init(int sample_rate, int sample_bits, int nb_channels) = 0;
   virtual void Reinit() = 0;
   virtual unsigned int GetMaxValue() = 0;
   virtual unsigned int GetMinValue() = 0;
   virtual unsigned int GetSampleRate() = 0;
   virtual unsigned int GetBitDepth() = 0;
   virtual unsigned int GetNbChannels() = 0;
   virtual void CheckBuffersStatus() = 0;

   virtual IWaveHDR* GetFreeBuffer() = 0;
   virtual void AddBufferToPlay(IWaveHDR*) = 0;
   virtual void SyncOnSound(bool set) = 0;
   virtual void SyncWithSound() {};

   // Samples handed to the device and not played yet (-1 : unknown)
   virtual int GetPendingSamples() { return -1; }
   // The stream (re)starts only once this many samples wait to be played (0 : at once). Starting
   // with some audio ahead absorbs the irregular arrival of the buffers without any underrun.
   virtual void SetPrebuffer(unsigned int nb_samples) {}
};


class IExternalSource
{
public:
   virtual bool SoundOn() = 0;
   virtual double GetSoundVolume() = 0;
};

class ISoundFactory
{
public:

	virtual ISound* GetSound(const char* name) = 0;
	virtual const char* GetSoundName(ISound*) = 0;

	virtual const char* GetFirstSoundName() = 0;
	virtual const char* GetNextSoundName() = 0;

};

