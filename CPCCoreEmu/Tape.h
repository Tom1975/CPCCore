#pragma once

#include "IConfiguration.h"
#include "IComponent.h"
#include "ILog.h"
#include "IDirectories.h"
#include "ISound.h"
#include "VGA.h"
#include "Inotify.h"
#include "ITapeOut.h"
#include "ILoadingProgress.h"

class PPI8255;

#ifndef NOFILTER
class Filter
{
public:
   Filter(int type_of_filter, bool lpf, double fe, double fc, int order);
   virtual ~Filter();

   void Init();
   void Filtrer(double* array, unsigned int nb_samples);

protected:
   volatile short int oldcw_;

   int order_;
   double *xv_;
   double *yv_;
   double gain_;
   double * coefx_;
   double * coefy_;

}; 


class IGenericFunction
{
public :
   virtual void Filtrer(double* array, unsigned int nb_samples) = 0;
};

class StandardFilter : public IGenericFunction
{
public:
   StandardFilter(int frequency, int type_of_hp_filter, float fc_hp, int order_hp, int type_of_lp_filter, float fc_lp, int order_lp, float gain);
   virtual ~StandardFilter();

   virtual void Filtrer(double* array, unsigned int nb_samples);

protected:

   Filter hp_filter_;
   Filter lp_filter_;

   float gain_;
};
#endif

class CTape : public IComponent, public IExternalSource, public ILoadingProgress
{
public:
   CTape(void);
   ~CTape(void);

   void SetLog ( ILog* log ) {log_ = log;};
   void SetNotifier ( IFdcNotify* notify_tape){tape_notifier_ = notify_tape;}
   void SetVGA ( GateArray* gate_array) { gate_array_ = gate_array; }
   void Reset ();
   void Init(IDirectories* directories, ITapeOut * ppi, IConfiguration * configuration_manager) {
      directories_ = directories;  ppi8255_ = ppi; configuration_manager_ = configuration_manager;
   }


   virtual bool SoundOn(){ return motor_on_&&play_;}
   virtual double GetSoundVolume ();

   /////////////////////
   // External actions :
   int GetLastEjectPosition  ();
   int GetNbBlocks ();
   int GetBlockPosition (int num_block);
   char* GetTextBlock ( int num_block);

   void Eject ();
   bool IsTapeInserted () { return tape_array_ != NULL;}

   // RECORD
   void Record ();
   bool IsRecordOn (){ return record_; };

   // PLAY
   void Play ();
   bool IsPlayOn (){ return play_; };

   // REWIND
   void Rewind ();
   bool IsRewindOn (){ return false; };

   // FAST FORWARD
   void SetTapePosition ( unsigned int nb_sec_from_begining);
   void FastForward();
   bool IsFastForwardOn (){ return false; };

   // STOP EJECT
   void StopEject ();
   bool IsEjectOn (){ return false; };

   // PAUSE
   void Pause ();
   bool IsPauseOn (){ return false; };

   // Insert new tape
   void InsertBlankTape ();

   int CompareToTape(CTape* other_tape);
   int InsertTape(unsigned char * buffer, unsigned int size);
   int InsertTape(IContainedElement* element);
   int InsertTape (const char* filepath );
   int InsertTapeDelayed();

   void SetMotorOn ( bool on ) ;
   bool GetMotor () { return motor_on_; };

   // Intrenal timing actions
   unsigned int Tick ();     // Tick is every z80 tick (4mhz)

   unsigned int LengthOfTape (){return tape_length_;};
   unsigned int GetCounter (){return counter_sec_;};
   const char* GetTapePath () { return current_tape_.c_str(); };

   bool IsSaveAvailable () {return nb_inversions_ > 0;};
   void SaveAsWav (const char* filepath);
   void SaveAsCdtDrb (const char* filepath);
#ifndef NOZLIB
   void SaveAsCSW (const char* filepath, unsigned char  type, unsigned char version);
#endif
   void SaveAsCdtCSW (const char* filepath);

   void SetPolarityInversion (bool set){polarity_inversion_ = set;};
   bool GetPolarityInversion (){return polarity_inversion_ ;};

   virtual void SetProgress(int progress) { load_progress_ = progress; };
   virtual int GetCurrentProgress() { return load_progress_; }

   void Filtre(double* array, unsigned int nb_samples, double frequency, unsigned int cut_frequency, int num_filter);
   static void Gain(double* array, unsigned int nb_samples, double multiplier);
   static void Shift(double* array, unsigned int nb_samples, double offset);

   int counter_sec_;
   float filter_gain_;
   float fc_hp_;
   float fc_lp_;
   int filter_order_lp_;
   int filter_order_hp_;
   int filter_type_lp_;
   int filter_type_hp_;
   unsigned int tape_length_;

protected:
   void ClearList ();
   void ComputeLenghtOfTape ();
   void PushPulse (int length , bool no_ratio = false );
   void PushPause (int length );
   void PushBit ( unsigned char bit );
   void PushByte ( unsigned char byte );

   int LoadWav (unsigned char* buffer, size_t size);
   int LoadWav(const char* filepath);
   int LoadTap (unsigned char* buffer, size_t size);
   int LoadTap(const char* filepath);
   int LoadTZX(unsigned char* buffer, size_t size);
   int LoadTZX (const char* filepath);
   int LoadCSW (unsigned char* buffer, size_t size);
   int LoadCSW(const char* filepath);
   int LoadVOC (unsigned char* buffer, size_t size);
   int LoadVOC(const char* filepath);


   void AddSample ( unsigned int value,  double frequency , int& lenght);

   void HandleVOCData ( bool first, unsigned char* buffer, unsigned int data_length, unsigned char codec_id, double sample_rate, unsigned char nb_channels, unsigned char bit_per_sample
#ifndef NOFILTER
      , IGenericFunction* filter
#endif
   );
   int DecodeCSWBloc  ( unsigned char* buffer, unsigned int length, unsigned int sample_rate, unsigned char compress_type, unsigned int num_pulse );
   void DecodeTapBuffer ( unsigned char* tap_buffer, size_t length, unsigned char used_bit_in_last_byte );
   unsigned int DecodeTapBlock (unsigned char* tap_buffer, unsigned int offset, size_t length );

   double GetSampleValue ( unsigned char *chunk, unsigned int *offset, int nb_channels, int bit_per_sample );
   int SampleToPPI ( double val );
   int ConvertSampleArray(bool first_call, double* value_array, int nb_samples, double frequency
#ifndef NOFILTER
      , IGenericFunction* filter
#endif
   );
   static bool IsArraySquareWave(double* value_array, int nb_samples);
   static bool IsArraySquareWave(unsigned char* value_tab, int nb_samples, int nb_channels, int bit_per_sample);

#ifndef NOFILTER
   void RemoveNoise ( double* array, unsigned int nb_samples, double frequency  );
   void HighPass ( double* array, unsigned int nb_samples, double frequency );


   void InitFilter(int type_of_filter, bool lpf, double fe, double fc, int order);
   void Filtrer  ( double* array, unsigned int nb_samples );
   void FiltrerPb ( double* array, unsigned int nb_samples, int type_of_filter, double fe, double fcb, double fch, int order );
#endif

   void TraceSamples ( double *chunk,  int nb_samples, double frequency);

   // Loading interthread
   bool pending_tape_;
   unsigned char* tape_buffer_to_load_;
   unsigned int tape_buffer_size_;

   ITapeOut * ppi8255_;
   GateArray* gate_array_;
   ILog* log_ ;
   IFdcNotify* tape_notifier_;
   // Inversion handling

   // Internal data
   // Tape data. Here is recorded the length between two signal inversion, in Tick value.
#pragma pack(push)
   typedef struct
   {
#if defined (__unix) || (RASPPI)
    __uint64_t length;
    __uint64_t place;
#else
      unsigned _int64 length;
      unsigned _int64 place;
#endif
      unsigned short block_number;
      unsigned char block_type;
      bool high;
   } FluxInversion;
#pragma pack(pop)

   FluxInversion* tape_array_;
   unsigned int nb_inversions_;

   // Internal helper functions
   unsigned int array_size_;
   void IncInversions ();

   // Motor is on ?
   bool motor_on_;
   bool previous_motor_on_;

   // Shift the start/stop of the motor.
   bool next_motor_state_;
   int time_to_change_motor_state_;

   // Play button pressed ?
   bool play_;

   // Record
   bool record_;
   bool start_record_ ;

   // Tape is inserted ?
   bool is_tape_inserted_;

   // Current stat of reading
   unsigned int tape_position_;
   unsigned long long remaining_reversal_flux_;

   int frequency_;

   // Data format
   unsigned short pilot_pulse_ ;
   unsigned short pilot_length_ ;
   unsigned short zero_ ;
   unsigned short one_;

   // Counter
   unsigned long long counter_us_;

   std::string current_tape_;

   bool current_level_;
   int current_block_type_;
   int current_block_;

   bool polarity_inversion_;

   unsigned int nb_samples_;
   double sample_rate_;

   int nb_sample_to_read_ ;
   double last_time_;
   int carry_;
   int oldcarry_;

   // Block data
   typedef struct
   {
      int block;
      char* description;
   }BlockList;

   IConfiguration * configuration_manager_;

   unsigned int nb_blocks_;
   unsigned int size_of_blocklist_;
   BlockList* block_list_;

   IDirectories* directories_;
   int load_progress_;
};

