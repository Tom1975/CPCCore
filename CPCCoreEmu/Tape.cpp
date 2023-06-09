#include "stdafx.h"

#include "Tape.h"

#include "simple_math.h"
#include "simple_regex.h"
#include "simple_filesystem.h"
#include "simple_stdio.h"

#ifndef NOZLIB
#include "zlib.h"
#endif

#include "PPI.h"
#include "VGA.h"

#ifndef NOFILTER
#include "mkfilter.h"
#endif

#define LOGFDC

#ifdef LOGFDC
#define LOG(str) \
   if (log_) log_->WriteLog (str);
#define LOGEOL if (log_) log_->EndOfLine ();
#define LOGB(str) \
   if (log_) log_->WriteLogByte (str);
#else
#define LOG(str)
#define LOGB(str)
#define LOGEOL
#endif

// Delay : 50 ms ?  - TODO : seems to be 180 ms
#define MOTOR_DELAY   240000
/*
Tout le fonctionnement de la chaine tête de lecture vers le PPI est documenté : c'est le schéma du CPC :P

Pour faire simple (à partir des schéma dans l'amendment service manual) :
Le 464 :
1er étage (transistor): pre-ampli du signal de la tête de lecture (~ x37) + filtre passe haut du 1er ordre (FC=7 hz)
2eme étage (AO) : 1er étage d'amplification (~ x70) + filtre passe haut du 1er ordre (FC= 500Hz)
(d'apres tech manual : 0.330 -> 0.520 mV en sortie...)
3eme étage (AO) : 2eme étage d'amplification (~ x7)
Soit, d'apres manual : 2.31 -> 3.64 ??

Le 6128 possède uniquement un filtre passe haut du 1er ordre (FC=250Hz) et le 2eme étage d'amplification (les deux autres sont dans le magnéto)

Dans les deux cas, la sortie des amplis est centrée sur 1.7V et attaque directement l'entrée du 8255, dont la tension de seuil haut est de 2.2V.

Les points les plus importants sont :
Le filtre passe haut a 250/500Hz et la sortie centrée sur 1.7V.
- Le filtre passe haut permet le retour à 0 de l'entrée du PPI lors des pauses.
- Le seuil d'entrée du PPI à 2.2v (à comparer au 1.7V) qui permet d'ignorer les signaux trop faible.

Pour simuler ça, il suffit d'appliquer le filtre passe haut, un offset de 1.7V et un comparateur a 2.2V. Le seul problème est le gain, qui dépend évidement de la source.
*/



size_t WriteShort ( unsigned short s, FILE * file )
{
   unsigned char buff[2];
   buff[0] = (s&0xFF);
   buff[1] = (s>>8);
   return fwrite ( buff, 1, 2, file);
}

size_t Write3Bytes ( unsigned int i, FILE * file )
{
   unsigned char buff[4];
   buff[0] = (i&0xFF);
   buff[1] = (i>>8);
   buff[2] = (i>>16);
   return fwrite ( buff, 1, 3, file);
}


size_t WriteInt ( unsigned int i, FILE * file )
{
   unsigned char buff[4];
   buff[0] = (i&0xFF);
   buff[1] = (i>>8);
   buff[2] = (i>>16);
   buff[3] = (i>>24);
   return fwrite ( buff, 1, 4, file);
}


#define GET_WORD(b) (b[0]+(b[1]<<8))
#define GET_DWORD(b) (b[0]+(b[1]<<8)+(b[2]<<16)+(b[3]<<24))

#define T_STATE_FREQ 4000000.0
//#define T_STATE_FREQ_TZX 4000000.0
#define T_STATE_FREQ_TZX 3500000.0
#define RATIO_FREQ (T_STATE_FREQ /T_STATE_FREQ_TZX)
//#define RATIO_FREQ 1

CTape::CTape(void) : ppi8255_(nullptr), configuration_manager_(nullptr), tape_buffer_size_(0), tape_buffer_to_load_(nullptr), pending_tape_(false),
   load_progress_(-1),
   size_of_blocklist_(0),
   block_list_(nullptr),
   nb_blocks_ (0),
   tape_changed_(false)
{
   log_ = nullptr;
   nb_inversions_ = 0;
   is_tape_inserted_ = false;
   play_ = false;
   tape_notifier_ = nullptr;

   tape_array_ = nullptr;
   nb_inversions_ = 0;
   Reset ();
   motor_on_ = false;
   record_ = false;
   start_record_ = false;
   current_tape_ = "";

   tape_length_ = 0;
   /*
   m_FilterGain = 0.025;
   m_FcHP = 600;
   m_FcLP = 4100;
   m_TypeOfFilter = 1;
   m_FilterOrder = 3;
*/

   // 8 tests / 8 ok !
   // 700/6200 - 0.015 - f 1 ; o 3
   // Pas ok sur la toute fin de "After the war", mais à investiguer car peu compréhensible.
   fc_hp_ = 700;
   fc_lp_ = 6200;
   filter_gain_ = 0.015F;
   filter_order_hp_ = 1;
   filter_type_hp_ = 2;
   filter_order_lp_ = 3;
   filter_type_lp_ = 2;

   /*m_FilterGain = 0.008;
   m_FcHP = 640;
   m_FcLP = 8500;*/

   time_to_change_motor_state_ = 0;

   polarity_inversion_ = false;

   
}


CTape::~CTape(void)
{
   delete[]block_list_;
   delete []tape_array_;
}

void CTape::Reset ()
{
   tape_changed_ = false;
   remaining_reversal_flux_ = 0;
   tape_position_ = 0;

   if ( tape_position_ < nb_inversions_)
      remaining_reversal_flux_ = tape_array_[tape_position_].length;

   counter_us_ = 0;
   counter_sec_ = 0;
   ComputeLenghtOfTape ();

   current_level_ = true;
   previous_motor_on_ = false;
}


int CTape::GetLastEjectPosition  ()
{
   if (directories_ != nullptr)
   {
      fs::path exe_path(directories_->GetBaseDirectory());
      exe_path /= "TapeRecord.ini";
      std::string ini_file = exe_path.string();

      fs::path path(current_tape_);
      std::string filename = path.filename().string();

      return configuration_manager_->GetConfigurationInt("Tape", filename.c_str(), 0, ini_file.c_str());
   }
   return -1;
}

int CTape::GetNbBlocks ()
{
   return nb_blocks_;
}

int CTape::GetBlockPosition (unsigned int numBlock)
{
   return (numBlock<nb_blocks_)? block_list_[numBlock].block:0;
}

char* CTape::GetTextBlock (unsigned int numBlock)
{
   return (numBlock < nb_blocks_) ? block_list_[numBlock].description: nullptr;
}

void CTape::Eject ()
{
   if (directories_ != nullptr && configuration_manager_ != nullptr)
   {
      fs::path exe_path(directories_->GetBaseDirectory());
      exe_path /= "TapeRecord.ini";
      std::string ini_file = exe_path.string();

      fs::path path(current_tape_);

      std::string filename = path.filename().string();
      char buffer[16];
      sprintf(buffer, "%u", counter_sec_);
      configuration_manager_->SetConfiguration("Tape", filename.c_str(), buffer, ini_file.c_str());
   }
     
   if (ppi8255_) ppi8255_->SetDataRecorderR ( false);

   ClearList ();
   delete []tape_array_;
   tape_array_ = NULL;
   nb_inversions_ = 0;

   remaining_reversal_flux_ = 0;
   tape_position_ = 0;
   counter_us_ = 0;
   counter_sec_ = 0;

   tape_changed_ = false;

}
void CTape::ClearList ()
{
   nb_blocks_ = 0;
}

void CTape::InsertBlankTape ()
{
   // Clean existing
   Eject ();

   // New blank, silence tape
   array_size_ = 1024;
   nb_inversions_ = 1;
   tape_array_ = new FluxInversion [array_size_] ;
   memset (tape_array_ , 0, sizeof (FluxInversion ) * array_size_);
   tape_array_ [0].high = false;
   // Duration of tape : 20" ?
   tape_array_ [0].length = 20LL * 60LL * 4000000LL;
}

#define 	M_PI   3.14159265358979323846	/* pi */


void CTape::Shift ( double* array, const unsigned int nb_samples, double offset)
{

   // Gain = 10 log (vs/ve)²
   // vs = sqrt( exp10 ( Gain/10 ) ) * ve

   for (unsigned int i = 0; i < nb_samples; i++)
   {
      array[i] = array[i] + offset;
   }

}

void CTape::Gain ( double* array, const unsigned int nb_samples, double multiplier)
{

   // Gain = 10 log (vs/ve)²
   // vs = sqrt( exp10 ( Gain/10 ) ) * ve

   for (unsigned int i = 0; i < nb_samples; i++)
   {
      array[i] = array[i] * multiplier;
   }

}

//////////////////////////////////////////////////
// Filtres :
//////////////////////////////////////////////////
#ifndef NOFILTER
Filter::Filter(int type_of_filter, bool lpf, double fe, double fc, int order) : order_(order)
{
   xv_ = new double[order + 1];
   yv_ = new double[order + 1];
   coefx_ = new double[order + 1];
   coefy_ = new double[order + 1];

   memset(coefx_, 0, sizeof(double)*(order + 1));
   memset(coefy_, 0, sizeof(double)*(order + 1));

   volatile short int  cw = 0x27F;
   volatile short oldcw = 0;
   oldcw_ = 0;

   // Force 53 bits floating points...
#ifdef _WIN32
#ifndef _WIN64
   __asm {
      fstcw oldcw
      fldcw cw
   }
   oldcw_ = oldcw;
#endif
#endif
   // Compute gain and coeff.
   switch (type_of_filter)
   {
   case 01:
      ComputeBessel(order, lpf, fc, fe, &gain_, coefx_, coefy_);
      break;
   case 02:
      ComputeButterworth(order, lpf, fc, fe, &gain_, coefx_, coefy_);
      break;
   }
}

Filter::~Filter()
{
   delete[]xv_;
   delete[]yv_;
   delete[]coefx_;
   delete[]coefy_;

#ifdef _WIN32
#ifndef _WIN64
   volatile short int oldcw = oldcw_;
   __asm {
      fldcw oldcw
   }
#endif
#endif

}

void Filter::Init()
{
   memset(xv_, 0, sizeof(double)*(order_ + 1));
   memset(yv_, 0, sizeof(double)*(order_ + 1));

}

//////////////////////////////////////////////////
// High pass filter, Butterworth, fc et ordre paramétrable.
//////////////////////////////////////////////////
// 01 : Bessel
// ...
void Filter::Filtrer ( double* array, unsigned int nb_samples )
{

   // Force 53 bits floating points...
#ifdef _WIN32
#ifndef _WIN64
   volatile short int oldcw = oldcw_;
   volatile short int  cw = 0x27F;
   __asm {
   fstcw oldcw
   fldcw cw
   }
#endif
#endif
   
   // Lets apply theses values.
   for (unsigned int i = 0; i < nb_samples; i++)
   {
      // Shift values
      for (int j = 0; j < order_; j++)
      {
         xv_[j] = xv_[j+1];
         yv_[j] = yv_[j+1];
      }

      xv_[order_] = array[i] / gain_;
      // Compute output/last yv value
      double yvout = 0;
      for (int j = 0; j < order_; j++)
      {
         yvout += xv_[j] * coefx_[j];
         yvout += yv_[j] * coefy_[j];
      }
      yvout += xv_[order_];
      yv_[order_] = yvout;
      array[i]  = yvout;
   }
#ifdef _WIN32
#ifndef _WIN64
   __asm {
   fldcw oldcw
   }
#endif
#endif
}


//////////////////////////////////////////////////
// band pass filter
//////////////////////////////////////////////////
// 01 : Bessel / 02 : Butterworth
// ...

void CTape::FiltrerPb ( double* array, unsigned int nb_samples, int type_of_filter, double fe, double fcb, double fch, int order )
{

   double *xv = new double[order*2+1];
   double *yv = new double[order*2+1];
   double gain;
   double * coefx = new double[order*2+1];
   double * coefy = new double[order*2+1];

   memset (xv, 0, sizeof(double)*(order*2+1));
   memset (yv, 0, sizeof(double)*(order*2+1));

   //unsigned int currCtrl;

   volatile short int  cw = 0x27F;
   volatile short int oldcw = 0;

#ifdef _WIN32
#ifndef _WIN64
   // Force 53 bits floating points...
   __asm {
   fstcw oldcw
   fldcw cw
   }
#endif
#endif
   // Can be replaced by....
   //_controlfp_s ( &currCtrl, _PC_53 , _MCW_PC  );

   // Compute gain and coeff.
   ComputeBandPass ( type_of_filter, order, fe, fcb, fch, &gain ,  coefx,  coefy);

#ifndef _LIVRAISON_
   char buff_trace[256];
   sprintf (buff_trace, "Type %i : Order : %i FE : %0.7f FCB : %0.7f FCH : %0.7f", type_of_filter, order, fe, fcb, fch );
   LOG (buff_trace)
   LOGEOL
   for (int j = 0; j < order*2; j++)
   {
      sprintf (buff_trace, "coef x %i : %0.7f - ", j, coefx[j] );
      LOG (buff_trace)
      sprintf (buff_trace, "coef y %i : %0.7f", j, coefy[j] );
      LOG (buff_trace)
      LOGEOL
   }
   sprintf (buff_trace, "gain %0.7f", gain );
   LOG (buff_trace)
   LOGEOL
#endif

   // Lets apply theses values.
   for (unsigned int i = 0; i < nb_samples; i++)
   {
      // Shift values
      for (int j = 0; j < order*2; j++)
      {
         xv[j] = xv[j+1];
         yv[j] = yv[j+1];
      }

      xv[order*2] = array[i] / gain;
      // Compute output/last yv value
      double yvout = 0;
      for (int j = 0; j < order*2; j++)
      {
         yvout += xv[j] * coefx[j];
         yvout += yv[j] * coefy[j];
      }
      yvout += xv[order*2];
      yv[order*2] = yvout;
      array[i]  = yvout;
   }
   delete []xv;
   delete []yv;
   delete []coefx;
   delete []coefy;
#ifdef _WIN32
#ifndef _WIN64
   __asm {
   fldcw oldcw
   }
#endif
#endif
}


//////////////////////////////////////////////////
// Remove noise Low filter pass (44100 hz / 4100 hz), 3e ordre Butterworth
void CTape::RemoveNoise ( double* array, unsigned int nb_samples, double frequency )
{

#define NZEROS_7_RN 3
#define NPOLES_7_RN 3
#define GAIN_7_RN   6.656267301e+00

static double xv[NZEROS_7_RN+1], yv[NPOLES_7_RN +1];


static double gain;
static double valx ;
static double valy0 ;
static double valy1 ;
static double valy2 ;

   // Depends on the frequency
   if (frequency == 44100)
   {
      gain = 6.656267301e+00;
      valy0 = 0.3052373668;
      valy1 = 1.2708422851;
      valy2 = 1.8454174372;
      valx = 3;
   }
   else if (frequency == 22050)
   {
      gain = 1.206174174e+00;
      valy0 = 0.0754469872 ;
      valy1 = 0.4792891682;
      valy2 = 0.7405880527;
      valx = 3;
   }
   else if (frequency == 48000)
   {
      gain = 8.282061910e+00;
      valy0 = 0.3371022865 ;
      valy1 = 1.3712197070;
      valy2 = 1.9375231172 ;
      valx = 3;
   }

   // RC Circuit
   for (unsigned int i = 0; i < nb_samples; i++)
   {
      { xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3];
        xv[3] = array[i] / gain;
        yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3];
        yv[3] =   (xv[0] + xv[3]) + valx * (xv[1] + xv[2])
                     + (  valy0 * yv[0]) + ( -valy1 * yv[1])
                     + (  valy2 * yv[2]);
        array[i]  = yv[1];
      }
   }

}

//////////////////////////////////////////////////
// High pass filter, fc = 7Hz
//////////////////////////////////////////////////

void CTape::HighPass ( double* array, unsigned int nb_samples, double frequency )
{

#define NZEROS_7_HP 1
#define NPOLES_7_HP 1
#define GAIN_7_HP   1.000498666e+00

static double xv[NZEROS_7_HP +1], yv[NPOLES_7_HP +1];

static double gain = 1.000498666e+00;
static double valy = 0.9990031660;

   // Depends on the frequency
   if (frequency == 44100)
   {
      gain = 1.000498666e+00;
      valy = 0.9990031660;
   }
   else if (frequency == 22050)
   {
      gain = 1.071358681e+00;
      valy = 0.8667884395 ;
   }
   else if (frequency == 48000)
   {
      gain = 1.000458149e+00;
      valy = 0.9990841217   ;
   }
   // RC Circuit
   for (unsigned int i = 0; i < nb_samples; i++)
   {
      { xv[0] = xv[1];
        xv[1] = array[i] / gain;
        yv[0] = yv[1];
        yv[1] =   (xv[1] - xv[0])
                     + (  valy * yv[0]);
        array[i]  = yv[1];
      }
   }
}


//////////////////////////////////////////////////
// High pass filter, fc = 500hz
//////////////////////////////////////////////////
void CTape::Filtre ( double* array, unsigned int nb_samples, double frequency, unsigned int cut_frequency, int num_filter)
{

#define NZEROS 1
#define NPOLES 1
//#define GAIN   1.035634035e+00

static double xv[NZEROS+1], yv[NPOLES+1];

static double gain = 1.035634035e+00;
static double valy = 0.9311841169;

   // Depends on the frequency
   if (frequency == 44100)
   {
      gain = 1.035634035e+00;
      valy = 0.9311841169;
   }
   else if (frequency == 22050)
   {
      gain = 1.071358681e+00;
      valy = 0.8667884395 ;
   }
   else if (frequency == 48000)
   {
      gain = 1.032736610e+00;
      valy = 0.9366022080  ;
   }

   // RC Circuit
   for (unsigned int i = 0; i < nb_samples; i++)
   {
      { xv[0] = xv[1];
        xv[1] = array[i] / gain;
        yv[0] = yv[1];
        yv[1] =   (xv[1] - xv[0])
                     + (  valy * yv[0]);
        array[i]  = yv[1];
      }
   }
}
#endif

// So, for your implementation, probably the key thing is to maintain a running average of the last however many input samples
// and then check for a new sample that's above or below a certain threshold from that and when you detect that drive the output low or high depending on the direction.

// Algorithm :
// - Keep average from the xxx last samples. Samples are from -1 to 1.
// - set a threshold, say 0.05.
// - If current level is up, then change occur on average - threshold
// - If current level is down, then change occur on average + threshold

double CTape::GetSampleValue ( unsigned char *chunk, unsigned int * offset, int nb_channels, int bit_per_sample )
{
   int value = 0;
   int local_value = 0;;

   for (int i =0; i < nb_channels && i < 16; i++)
   {
      // x bytes for first channel
      local_value = 0;

      for (int j =0; j < (bit_per_sample/8); j++)
      {
         local_value += (chunk[ (*offset)++]<<(j*8));

      }
      //if (i == 0)
      {
         if ( bit_per_sample == 8 )
         {
            value += local_value;
         }
         else if ( bit_per_sample == 16 )
         {
            value += (short)local_value;
         }
      }
   }
   value = value / nb_channels;

   double val = 0;
   if ( bit_per_sample == 8 )
   {
      val = ((value - 128.0)/128);
   }
   else if ( bit_per_sample == 16 )
   {
      short short_value = (short)value;
      val = (((double)short_value)/((double)(0xFFFF/2)));
   }
   return val;
}

void CTape::TraceSamples ( double *val,  int nb_samples, double Frequency)
{
   FILE * file;

   double sample_rate = Frequency;

   if (fopen_s (&file, "trace_wav.wav", "wb") == 0)
   {
      int size = 0;
      size += fwrite ( "RIFF", 1, 4, file  );
      size += WriteInt ( 0, file  );
      size += fwrite ( "WAVE", 1, 4, file  );

      int block_size = 0x10;
      size += fwrite ( "fmt ", 1, 4, file  );
      size += WriteInt ( block_size, file  );

      size += WriteShort ( 1, file  ); // PCM
      size += WriteShort ( 1, file  ); // 1 channel
      size += WriteInt ( (int)sample_rate, file  ); // 11025 Hz
      size += WriteInt ( (int)sample_rate, file  ); //
      size += WriteShort ( 1, file  ); //
      size += WriteShort ( 8, file  ); //

      // Data
      size += fwrite ( "data ", 1, 4, file  );
      unsigned int datasizeplace = nb_samples;
      size += WriteInt ( datasizeplace, file  ); //

      unsigned char car_buf[1];
      for (int i = 0; i < nb_samples; i++)
      {
         // Size of this inversion / sample rate = nb of samples

         // Value
         if ( val[i] < -1 ) val[i] = -1;
         if ( val[i] > 1 ) val[i] = 0.99;

         car_buf[0] = static_cast<unsigned char>(val[i] * 128 + 128);

         // Add a sample
         size += fwrite ( car_buf, 1, 1, file );
      }

      // Set data size

      // Set file size
      fseek ( file, 4, SEEK_SET );
      fwrite ( &size, 1, 4, file );
      fclose ( file );
   }
}

int CTape::SampleToPPI ( double val )
{
   // Gain : A affiner...
   //val *= 6588;

   // TO ADD : Specific gain...
   // Sortie centrée sur 1.7v ->
   //val += 1.7;

   // Entrée du PPI a 2.2v, on coupe donc les signaux trop faibles
   // If value < average - threshold, then return is "low"

   //if ( val > 2.2 )
   //if ( val > 0.02 /*1.7*/ )
   if ( val > 0 )
   {


#define NO_REVERSE
#ifdef NO_REVERSE
      return 1;
#else
      return 0;
#endif
   }
   else
   {
#ifdef NO_REVERSE
      return 0;
#else
      return 1;
#endif
   }
}

bool CTape::IsArraySquareWave(unsigned char* value_tab, int nb_samples, int nb_channels, int bit_per_sample)
{
   // Compute each value.
   double min = 0;
   int nb_min = 0;
   double max = 0;
   int nb_max = 0;

   double truemin = 0;
   double truemax = 0;

   for (int i = 0; i < nb_samples; i++)
   {
      // between -1 and -0.33 ? Minus value
      if (value_tab[i] < -0.33)
      {
         min += -value_tab[i];
         nb_min++;
      }
      // between +1 and +0.33 ? Max value
      else if (value_tab[i] > 0.33)
      {
         max += value_tab[i];
         nb_max++;
      }
      // Otherwise, pause. Don't compute it.
      if (value_tab[i] > truemax) truemax = value_tab[i];
      if (value_tab[i] < truemin) truemin = value_tab[i];
   }

   // Min and max average. If they are above 90% of the abs max, consider wav as a square wave.
   double min_avg = min / nb_min;
   double max_avg = max / nb_max;

   if (min_avg > 0.9 && max_avg > 0.9)
      return true;

   //
   min = max = 0;
   for (int i = 0; i < nb_samples; i++)
   {
      // between -1 and -0.33 ? Minus value
      if (value_tab[i] < -0.33)
      {
         min += abs(min_avg + value_tab[i]);
         nb_min++;
      }
      // between +1 and +0.33 ? Max value
      else if (value_tab[i] > 0.33)
      {
         max += abs(max_avg - value_tab[i]);
         nb_max++;
      }
      // Otherwise, pause. Don't compute it.
   }

   double min_avg2 = abs(min / nb_min / truemin);
   double max_avg2 = abs(max / nb_max / truemax);

   return (min_avg2 < 0.001 && max_avg2 < 0.001);
}

bool CTape::IsArraySquareWave(double* value_array, int nb_samples)
{
   // Compute each value.
   double min = 0;
   int nb_min = 0;
   double max = 0;
   int nb_max = 0;

   double truemin = 0;
   double truemax = 0;

   for (int i = 0; i < nb_samples; i++)
   {
      // between -1 and -0.33 ? Minus value
      if ( value_array[i] < -0.33)
      {
         min += -value_array[i];
         nb_min++;
      }
      // between +1 and +0.33 ? Max value
      else if ( value_array[i] > 0.33)
      {
         max += value_array[i];
         nb_max++;
      }
      // Otherwise, pause. Don't compute it.
      if ( value_array[i] > truemax ) truemax  = value_array[i];
      if ( value_array[i] < truemin ) truemin  = value_array[i];
   }

   // Min and max average. If they are above 90% of the abs max, consider wav as a square wave.
   double min_avg = min / nb_min;
   double max_avg = max / nb_max;

   if ( min_avg > 0.9 && max_avg > 0.9)
      return true;

   //
   min = max = 0;
   for (int i = 0; i < nb_samples; i++)
   {
      // between -1 and -0.33 ? Minus value
      if ( value_array[i] < -0.33)
      {
         min += abs(min_avg+value_array[i]);
         nb_min++;
      }
      // between +1 and +0.33 ? Max value
      else if ( value_array[i] > 0.33)
      {
         max += abs(max_avg - value_array[i]);
         nb_max++;
      }
      // Otherwise, pause. Don't compute it.
   }

   double min_avg2 = abs(min / nb_min / truemin);
   double max_avg2 = abs(max / nb_max / truemax);

   return ( min_avg2 < 0.001 && max_avg2 < 0.001);
}

void TriggerSchmitt ( double* value_tab, int nb_samples, double vtp, double vtm, double vsat )
{
   bool state_high = false;

   // Comparateur à deux seuils inverseur (de schmitt)
   for (int i = 0; i < nb_samples; i++)
   {
      // Depending on state
      if (state_high && value_tab [i] < vtm)
      {
         state_high = false;
      }
      else if ( (!state_high) && value_tab [i] > vtp)
      {
         state_high = true;
      }

      value_tab [i] = state_high?vsat:(-vsat);
   }
}


// Convert sample array : From an array with sample in double, between -1 an 1, convert it to a tFluxInversion array.
int CTape::ConvertSampleArray (bool first_call, double* value_tab, int nb_samples, double frequency
#ifndef NOFILTER
   , IGenericFunction* filter
#endif
) //Filter& hp_filter, Filter& lp_filter)
{
   // Step 1 : Compute average max and min, to determine if it's a quare signal (from PPI sampling / WAV generation)

      // HISTORY :
      // Stable version :

      // MOST Stable version !
      // Runs Gauntlet, Kikekankoi, Skate crazy, Basil, FOTY2
      // *****  2 =>
      /*
      Gain (value_array, nb_samples, 0.025);
      Filtrer ( value_array, nb_samples, 02, false, frequency, 600, 3);
      Gain (value_array, nb_samples, 37*70*7);
      Filtrer ( value_array, nb_samples, 02, true, frequency, 4100, 3);
      Shift (value_array, nb_samples, -0.5 );
      */


      // Ok pour Werewolves & Ultima ratio. ko pour Gauntlet et AtW
      /*
      Gain (value_array, nb_samples, 0.025);
      Filtrer ( value_array, nb_samples, 01, false, frequency, 600, 2);
      Gain (value_array, nb_samples, 37*70*7);
      Filtrer ( value_array, nb_samples, 01, true, frequency, 4100, 2);
      Shift (value_array, nb_samples, -0.5 );
      */

      /*
      // Rationalisation
      double max = 0;
      double min = 0;
      for (int i = 0; i < nb_samples; i++)
      {
         if ( value_array[i] < min) min = value_array[i];
         if ( value_array[i] > max) max = value_array[i];
      }
      // Ratio
      max = (max > (abs(min)))?max:(-min);
      double rat = 1 / max;
      for (int i = 0; i < nb_samples; i++)
      {
         value_array[i] *= rat;
      }

      */
      // Ok pour Ultima ratio.& gauntlet  ko pour AtW et skate crazy
      /*
      Filtrer ( value_array, nb_samples, 02, false, frequency, 700, 3);
      Filtrer ( value_array, nb_samples, 02, true, frequency, 4100, 3);
      Gain (value_array, nb_samples, 37*70*7*0.025);
      */
      // 700 : ok UR, gauntlet, ko : AtW, skate
      // 650 : ok : skate, gauntlet, ko : UR
      // 660 : ok : gauntlet,    ko : skate, UR
      // 670 : ko : skate, UR

      //  UR : 690 +
      // skate ! 600 -> 650
      //
      /*
      m_FilterGain = 37*70*7*0.025;
      m_FcHP = 600;
      m_FcLP = 4100;
      */
      // UR : m_FilterGain = 37*70*7*0.025;
      /*
      m_FilterGain = 0.005;
      m_FcHP = 580;
      m_FcLP = 5500;
      m_TypeOfFilter = 2;
      m_FilterOrder = 3;
      */
      /*
      Filtrer ( value_array, nb_samples, 02, false, frequency, m_FcHP, 3);
      Filtrer ( value_array, nb_samples, 02, true, frequency, m_FcLP, 3);
      Gain (value_array, nb_samples, m_FilterGain*18130);
      Shift (value_array, nb_samples, -0.5 );
      */
//      FiltrerPB ( value_array, nb_samples, m_TypeOfFilter, frequency, m_FcLP, m_FcHP, m_FilterOrder);
      //Gain (value_array, nb_samples, m_FilterGain*18130);

   /*m_FcHP = 700;
   m_FcLP = 6200;
   m_FilterGain = 0.015;
   m_TypeOfFilterHP = 1;
   m_TypeOfFilterLP = 1;
   m_FilterOrderLP = 3;
   m_FilterOrderHP = 3;*/

#ifndef NOFILTER
   if (filter != nullptr)
   {
      filter->Filtrer(value_tab, nb_samples);

      //TriggerSchmitt ( value_array, nb_samples, 0.5, -0.5, 4.9 );
      // Ici, on est en sortie pin 7 du la6324. Donc, valeur max entre 350 et 500 mv.... (??)
      // Vt+ = Vcc/2 * (R1+R2) / R2
      // Vt- = Vcc - Vcc/2 * (R1+R2) / R2
      // TODO  : Ajouter le trigger de schmitt inverseur (Vt+ = 4.9 / 2 * (10+47)k/(47k))
      double vtp = 5 * (10.0/57.0);
      double vtm = -5 * (10.0/57.0);
   }
#endif
   // Step 2 : Convert values to this format
   int offset = 0;
   int value = 0;
   if (first_call)
   {
      value = SampleToPPI(value_tab[offset++]);

      tape_array_[nb_inversions_].high = (value == 1) ? true : false;
      tape_array_[nb_inversions_].length = static_cast<unsigned long long>(T_STATE_FREQ / frequency);
      tape_array_[nb_inversions_].place = tape_array_[nb_inversions_].length;
      nb_sample_to_read_ = 1;
      carry_ = 0;
      oldcarry_ = 0;
      nb_samples = 1;
      last_time_ = 0;
   } 
   int length = 1;
   
   
   while (offset < nb_samples)
   {
      ++nb_sample_to_read_;
      value = SampleToPPI ( value_tab[offset++]);
      AddSample (value , frequency , length);
   }

   return 1;
}

int CTape::CompareToTape(CTape* other_tape)
{
   // compare flux
   if (nb_inversions_ != other_tape->nb_inversions_) return -1;
   for (unsigned int i = 0; i < nb_inversions_; i++)
   {
      if (memcmp(&tape_array_[i], &other_tape->tape_array_[i], sizeof(FluxInversion)) != 0)
      {
         return i;
         //return -1;
      }
   }


   return 0;
}

int CTape::InsertTape(unsigned char * buffer, unsigned int size)
{
   int ret = -2;
   Eject();
   unsigned char header[23] = { 0 };
   // check the 12 first char
   memcpy(header, buffer, 23);

   if ((memcmp(header, "ZXTape!", 7) == 0)
      && (header[7] == 0x1A)
      )
   {
      ret = LoadTZX(buffer, size);
   }
   else if ((memcmp(header, "Compressed Square Wave", 22) == 0)
      && (header[0x16] == 0x1A)
      )
   {
      ret = LoadCSW(buffer, size);
   }
   else if (memcmp(header, "RIFF", 4) == 0)
   {
      ret = LoadWav(buffer, size);
   }
   else if (memcmp(header, "Creative Voice File", 19) == 0)
   {
      ret = LoadVOC(buffer, size);
   }
   /*else if (PathMatchSpec(filepath, _T("*.tap")) == TRUE)
   {
      ret = LoadTAP(buffer, size);
   }*/
   else
   {
      // Unknown format
      current_tape_.clear();
      ret = -2;
   }
   load_progress_ = -1;
   return ret;
}

int CTape::InsertTape(IContainedElement* element)
{
   pending_tape_ = true;
   load_progress_ = -1;
   delete[]tape_buffer_to_load_;
   tape_buffer_size_ = element->GetSize(0);
   tape_buffer_to_load_ = new unsigned char[tape_buffer_size_];
   memcpy(tape_buffer_to_load_, element->GetBuffer(0), tape_buffer_size_);
   return 0;
}

int CTape::InsertTape(const char* file)
{
   load_progress_ = -1;
   pending_tape_ = true;
   tape_buffer_size_ = 0;
   tape_buffer_to_load_ = nullptr;
   current_tape_ = file;
   return 0;
}

int CTape::InsertTapeDelayed()
{
   int ret = 0;
   Eject ();

   FILE * file;

   
   if (fopen_s (&file, current_tape_.c_str(), "rb") == 0)
   {
      unsigned char header [23] = {0};
      // check the 12 first char
      fread ( header , 1, 23, file );
      fclose (file);

      if (  ( memcmp ( header, "ZXTape!", 7) == 0)
         && ( header[7] == 0x1A)
         )
      {
         ret = LoadTZX (current_tape_.c_str());
      }
      else if (( memcmp ( header, "Compressed Square Wave", 22) == 0)
               && ( header[0x16] == 0x1A)
            )
      {
         ret = LoadCSW (current_tape_.c_str());
      }
      //else if (PathMatchSpec(filepath,"*.tap") == TRUE)
      else if ( memcmp ( header, "RIFF", 4) == 0)
      {
         ret = LoadWav (current_tape_.c_str());
      }
      else if (memcmp ( header, "Creative Voice File", 19) == 0)
      {
         ret = LoadVOC (current_tape_.c_str());
      }
      else if (IsExtensionMatch(current_tape_.c_str(), "tap"))
      {
         ret = LoadTap(current_tape_.c_str());
      }
      else
      {
         // Unknown format
         current_tape_.clear();
         ret = -2;
      }
   }
   else
   {
      // Erreur : File not found
      current_tape_.clear();
      ret = -1;
   }

   load_progress_ = -1;

   if ( tape_notifier_ ) tape_notifier_->ItemLoaded (current_tape_.c_str(), ret, -1);
   return ret;

}



void CTape::PushPulse (int length, bool noRatio )
{
   IncInversions ();

   tape_array_[nb_inversions_].high = current_level_;
   tape_array_[nb_inversions_].length = static_cast<unsigned long long>(Round ( (double)length * (noRatio?1:RATIO_FREQ)));

   if ( nb_inversions_ > 0)
   {
      tape_array_[nb_inversions_].place = tape_array_[nb_inversions_ - 1].place + tape_array_[nb_inversions_].length;
   }
   else
   {
      tape_array_[nb_inversions_].place = tape_array_[nb_inversions_].length;
   }
   current_level_ = !current_level_;
}

void CTape::PushPause (int length )
{
   if ( length != 0 )
   {
      if ( current_level_ )
      {
         PushPulse (1 * 4000, true);
         PushPulse (length * 4000, true);

      }
      else
      { 
         PushPulse(length * 4000, true);
      }

      // Set the next level to low...
      current_level_ = true;

   }
   else
   {
      int dbg = 1;
   }

}

void CTape::PushBit ( unsigned char bit )
{
   // Add a one or zero tho the structure
   unsigned int len = (bit==1)?one_:zero_;
   PushPulse ( len );
   PushPulse ( len );

}

void CTape::PushByte ( unsigned char byte )
{
   // Add these byte ath the end of the whole structure
   for (auto i = 7; i >= 0; i--)
   {
      PushBit ( (byte >> i) & 0x1);
   }
}

void CTape::DecodeTapBuffer ( unsigned char* tap_buffer, size_t length, unsigned char used_bit_in_last_byte )
{
   // Decode Next block
   unsigned int offset = 0;
   if ( length == 0 ) return;

   while (offset < (length-1))
   {
      offset = DecodeTapBlock (tap_buffer, offset, length-1 );
   }
   // last byte
   unsigned char byte = tap_buffer[offset++];
   for (int i = 7; i >= (8-used_bit_in_last_byte); i--)
   {
      PushBit ( (byte >> i) & 0x1);
   }




}

unsigned int CTape::DecodeTapBlock (unsigned char* tap_buffer, unsigned int offset, size_t length )
{
   // Write all these blocks to the structure
   while ( offset < length)
   {
      PushByte ( tap_buffer[offset] );
      offset++;
   }
   return offset;
}

int CTape::LoadTap(unsigned char* buffer, size_t size)
{
   // Init internal structure
   array_size_ = 1024;
   nb_inversions_ = 0;
   tape_array_ = new FluxInversion[array_size_];
   memset(tape_array_, 0, sizeof (FluxInversion)  * array_size_);

   DecodeTapBuffer(buffer, size, 8);

   is_tape_inserted_ = true;
   play_ = true;
   ComputeLenghtOfTape();
   Reset();
   return 0;
}

int CTape::LoadTap (const char* file_path)
{
   FILE * file;
   int ret = -1;
   if (fopen_s (&file, file_path, "rb") == 0)
   {
      // Length ?
      fseek (file, 0, SEEK_END);   // non-portable
      int nLength=ftell (file);
      rewind (file);   // non-portable

      // Read datas
      unsigned char* tap_buffer = new unsigned char[nLength];
      fread ( tap_buffer, 1, nLength,  file );
      fclose(file);

      ret = LoadTap(tap_buffer, nLength);

      delete []tap_buffer;
   }

   return 0;
}


int CTape::DecodeCSWBloc ( unsigned char* buffer, unsigned int length, unsigned int sample_rate, unsigned char compress_type, unsigned int num_pulse )
{
   double ratio = 4000000.0 / (double)sample_rate;

   // Set initial value

   switch (compress_type )
   {
   case 1:  // RLE
      {
         unsigned int index = 0;
         while (index  < length )
         {
            // Next value
            unsigned int pulse_length = buffer[index++];
            if ( pulse_length == 0 && index + 3 < length )
            {
               pulse_length = buffer[index]
                        |(buffer[index+1] <<8)
                        |(buffer[index+2] <<16)
                        |(buffer[index+3] <<24);
               index += 4;
            }

            // Add it
            unsigned int length_t_states = static_cast<unsigned int>(Round(pulse_length *ratio));

            tape_array_[nb_inversions_].length = length_t_states ;

            // Next pulse
            IncInversions ();
            current_level_ = !current_level_;
            tape_array_[nb_inversions_].high = current_level_;
            tape_array_[nb_inversions_].length = 0;
            tape_array_[nb_inversions_].block_number = tape_array_[nb_inversions_-1].block_number;
            tape_array_[nb_inversions_].place = tape_array_[nb_inversions_-1].place + tape_array_[nb_inversions_-1].length;
            tape_array_[nb_inversions_].block_type = 0x18;
         }
         break;
      }

   case 2: // Z-RLE
#ifndef NOZLIB
      // uncompress the flux to another buffer
      uLongf size_max = num_pulse * 5;
      unsigned char* dest_buffer = new unsigned char [size_max];

      if ( uncompress ( dest_buffer, &size_max, buffer, length ) == Z_OK)
      {
         // Then use type i compression
         DecodeCSWBloc ( dest_buffer, size_max, sample_rate, 1, num_pulse );

      }
      else
      {
         // TODO
      }
      delete[]dest_buffer;
#endif
      break;
   }
   return 0;
}

void CTape::AddSample ( unsigned int value,  double frequency , int& lenght)
{
   if ( ( value == -1)
      || (tape_array_[nb_inversions_].high == ((value==1)?true:false))
      )
   {

      double this_time = (nb_sample_to_read_ * T_STATE_FREQ + oldcarry_)/ frequency ;
      unsigned long long computation_int = static_cast<unsigned long long> ( nb_sample_to_read_ * T_STATE_FREQ+ oldcarry_);
      unsigned int value = (int(this_time + 0.5));
      carry_ = static_cast<int> (computation_int- value * frequency );

      tape_array_[nb_inversions_].length = value;

      if ( nb_inversions_ == 0)
      {
         tape_array_[nb_inversions_].place += tape_array_[nb_inversions_].length;
      }
      else
      {
         tape_array_[nb_inversions_].place = tape_array_[nb_inversions_-1].place + tape_array_[nb_inversions_].length;
      }
   }
   else
   {
      last_time_ = ((nb_sample_to_read_-1) * T_STATE_FREQ + carry_ )/ frequency;
      oldcarry_ = carry_;
      carry_ = 0;
      IncInversions ();

      nb_sample_to_read_ = 1;

      tape_array_[nb_inversions_].high = (value==1)?true:false;
      double this_time = (nb_sample_to_read_ * T_STATE_FREQ + oldcarry_) / frequency;
      unsigned long long computation_int = static_cast<unsigned long long>(nb_sample_to_read_ * T_STATE_FREQ + oldcarry_);
      unsigned int value = (int(this_time + 0.5));
      carry_ = static_cast<int>(computation_int - value * frequency);

      tape_array_[nb_inversions_].length = value;
                                                                                                    
      tape_array_[nb_inversions_].place += tape_array_[nb_inversions_].length;
      lenght = 1;
   }
}

void CTape::HandleVOCData ( bool first, unsigned char* voc_buffer, unsigned int data_length, unsigned char codec_id, 
                           double sample_rate, unsigned char nb_channels, unsigned char bit_per_sample
#ifndef NOFILTER
   , IGenericFunction* filter
#endif
)
{
   unsigned int offset = 0;

   int value;
   double convFactor_L = T_STATE_FREQ / ((double)sample_rate);
   int nb_samples = data_length / (nb_channels * (bit_per_sample/8));

   switch (codec_id)
   {
      // 0x00  8 bits unsigned PCM
      case 0x00:
      {
         // First value
         if (first)
         {
            // Init the array with 1024 inversions
            array_size_ = 1024;
            nb_samples_ = 0;
            nb_inversions_ = 0;
            tape_array_ = new FluxInversion [array_size_] ;
            memset (tape_array_ , 0, sizeof (FluxInversion ) * array_size_);

            value = 0;
            tape_array_[nb_inversions_].high = (value==1)?true:false;
            tape_array_[nb_inversions_].length = (unsigned int)convFactor_L;
            tape_array_[nb_inversions_].place = tape_array_[nb_inversions_].length;

         }
         // Nexts
         // First record from the first sample

         double* value_array = new double[nb_samples];

         for (int i = 0; i < nb_samples ; i++)
         {
            // Next sample
            value_array[i] = GetSampleValue ( voc_buffer, &offset, nb_channels, 8 );

         }

         // Handle this sample array
         bool square_wave = CTape::IsArraySquareWave(value_array, nb_samples);

         //ConvertSampleArray (square_wave, bFirst, value_array, nb_samples, sampleRate, *hp_filter, *lp_filter);

         ConvertSampleArray( first, value_array, nb_samples, sample_rate
#ifndef NOFILTER
            , (square_wave==false)? filter :nullptr
#endif
         );
         delete[]value_array;

         break;
      }
      // 0x01  4 bits to 8 bits Creative ADPCM
      case 0x01:
      {
         break;
      }
      // 0x02  3 bits to 8 bits Creative ADPCM (AKA 2.6 bits)
      case 0x02:
      {
         // TODO
         break;
      }
      // 0x03  2 bits to 8 bits Creative ADPCM
      case 0x03:
      {
         // TODO
         break;
      }
      // 0x04  16 bits signed PCM
      case 0x04:
      {
         // TODO
         break;
      }
      // 0x06  alaw
      case 0x06:
      {
         // TODO
         break;
      }
      // 0x07  ulaw
      case 0x07:
      {
         // TODO
         break;
      }
      // 0x0200  4 bits to 16 bits Creative ADPCM (only valid in block type 0x09)
      /*case 0x0200:
      {
         // TODO
         break;
      }*/
   }
}

int CTape::LoadVOC(unsigned char* voc_buffer, size_t size)
{
   // header
   unsigned int index_pos = 0;
   current_block_ = 0;
   current_block_type_ = 0;

   unsigned char header[26] = { 0 };
   //fread(header, 1, 26, file);
   memcpy(header, &voc_buffer[index_pos], 26);
   index_pos += 26;

   // Version
   unsigned char minVersion = header[22];
   unsigned char majVersion = header[23];

   // Validity check
   unsigned short validity = GET_WORD((&header[24]));
   if (validity != (~((majVersion << 8) | minVersion)) + 0x1234)
   {
      // ERROR
      return -1;
   }

   // Data blocs
   bool finished = false;

   finished = (index_pos + 1 > size);
   //fread(header, 1, 1, file) != 1);
   memcpy(header, &voc_buffer[index_pos], 1);
   index_pos += 1;


   sample_rate_ = 0;
   bool extra_info = false;
   unsigned int data_length = 0;
   unsigned char* buffer = NULL;
   unsigned char codec_id = 0;
   unsigned char channel_number = 0;
   unsigned char nb_channels = 1;
   unsigned char bit_per_sample = 8;

   // todo : Frequency 
   //Filter = 
#ifndef NOFILTER
   StandardFilter filter(frequency_, filter_type_hp_, fc_hp_, filter_order_hp_, filter_type_lp_, fc_lp_, filter_order_lp_, filter_gain_);
#endif
   //Filter hp_filter(m_TypeOfFilterHP, false, m_Frequency, m_FcHP, m_FilterOrderHP);
   //Filter lp_filter(m_TypeOfFilterLP, false, m_Frequency, m_FcLP, m_FilterOrderLP);

   bool first = true;
   while (!finished)
   {
      unsigned char id = header[0];
      if (id != 0)
      {
         // Read size
         //fread(header, 1, 3, file);
         memcpy(header, &voc_buffer[index_pos], 3);
         index_pos += 3;

         data_length = header[0x0] + (header[0x1] << 8) + (header[0x2] << 16);
         buffer = new unsigned char[data_length];
         memcpy(buffer, &voc_buffer[index_pos], data_length);
         index_pos += data_length;

      }


      switch (id)
      {
      case 0x00:  // Terminator
         finished = true;
         break;
      case 0x01:  // Sound data
      {
         if (!extra_info)
         {
            unsigned char freq_divisor = buffer[0];
            codec_id = buffer[1];
            sample_rate_ = 1000000 / (256 - freq_divisor);
         }
         else
         {
            extra_info = false;
         }
         // Data
         HandleVOCData(first, (&buffer[2]), data_length - 2, codec_id, sample_rate_, nb_channels, bit_per_sample
#ifndef NOFILTER            
            , &filter
#endif
         );
         first = false;
         break;
      }
      case 0x02:  // Sound data continuation
         HandleVOCData(first, (&buffer[2]), data_length - 2, codec_id, sample_rate_, nb_channels, bit_per_sample
#ifndef NOFILTER            
            , &filter
#endif
         );
         first = false;
         break;
      case 0x03:  // Silence
      {
         //
         unsigned short length_silence = GET_WORD((&buffer[0]));
         unsigned char freq_divisor = buffer[2];
         sample_rate_ = 1000000 / (256 - freq_divisor);
         break;
      }
      case 0x04:  // Marker
                  // Mark value, not used here
         break;
      case 0x05:  // Text
                  // No used here
         break;
      case 0x06:  // Repeat start
                  // Not used here
         break;
      case 0x07:  // Repeat end
                  // Not used here
         break;
      case 0x08:  // Extra info
      {
         unsigned short freq_divisor = GET_WORD((&buffer[0]));
         codec_id = buffer[1];
         sample_rate_ = 256000000 / (nb_channels * (65536 - freq_divisor));
         channel_number = buffer[3];
         extra_info = true;
         break;
      }
      case 0x09:  // Sound data (New format)
      {
         sample_rate_ = GET_DWORD((&buffer[0]));
         bit_per_sample = buffer[4];
         nb_channels = buffer[5];
         codec_id = GET_WORD((&buffer[6]));

         HandleVOCData(first, (&buffer[12]), data_length - 2, codec_id, sample_rate_, nb_channels, bit_per_sample
#ifndef NOFILTER            
            , &filter
#endif
         );
         first = false;
         break;
      }

      }
      if (header[0] != 0)
      {
         delete[]buffer;
      }
      // Next bloc id
      finished = (index_pos + 1 > size);
      memcpy(header, &voc_buffer[index_pos], 1);
      index_pos += 1;

   }

   PushPause(1000);
   is_tape_inserted_ = true;
   play_ = true;
   ComputeLenghtOfTape();
   Reset();

   return 0;
}

int CTape::LoadVOC (const char* file)
{
   FILE * f;
   int ret = -1;
   if (fopen_s (&f, file, "rb") == 0)
   {
      fseek(f, 0, SEEK_END);
      int buffer_size = ftell(f);
      rewind(f);
      unsigned char* buffer = new unsigned char[buffer_size];

      fread(buffer, buffer_size, 1, f);
      fclose(f);
      ret = LoadVOC(buffer, buffer_size);

      delete[]buffer;

   }
   return ret;
}
int CTape::LoadCSW(unsigned char* buffer, size_t size)
{
   // Read format chunk
   sample_rate_ = 0;
   current_block_ = 0;
   current_block_type_ = 0;

   unsigned int num_pulse = 0;
   unsigned char compress_type = 0;
   unsigned char flag = 0x1;
   int index_pos = 0;

   unsigned char header[0x35] = { 0 };
   //fread(header, 1, 0x19, file);
   memcpy(header, buffer, 0x19);
   index_pos = 0x19;

   // Major revision
   unsigned char maj_rev = header[0x17];
   // Minor revision
   unsigned char min_rev = header[0x18];

   // CSW 2
   if (maj_rev == 2)
   {
      //fread((&header[0x19]), 1, 27, file);
      memcpy(&header[0x19], &buffer[index_pos], 27);
      index_pos += 27;

      // Sample rate
      sample_rate_ = GET_DWORD((&header[0x19]));

      // Number of pulse
      num_pulse = GET_DWORD((&header[0x1D]));

      // Compression type
      compress_type = header[0x21];

      flag = header[0x22];

      unsigned char HDRlength = header[0x23];

      // Now, read the HDR (and dont do anything : Nothin specified at this time
      //fread(header, 1, HDRlength, file);
      memcpy(header, &buffer[index_pos], HDRlength);
      index_pos += HDRlength;

   }
   else if (maj_rev == 1)
   {
      // CSW 1
      //fread((&header[0x19]), 1, 7, file);
      memcpy(&header[0x19], &buffer[index_pos], 7);
      index_pos += 7;

      sample_rate_ = GET_WORD((&header[0x19]));
      compress_type = header[0x1B];
      // No compression on CSW 1.1 !
      compress_type = 1;
      flag = header[0x1C];
   }

   unsigned char* ptr_buffer = &buffer[index_pos];

   // Set to the begining of the tape
   nb_inversions_ = 0;
   array_size_ = 1024;
   nb_samples_ = 0;
   nb_inversions_ = 0;
   tape_array_ = new FluxInversion[array_size_];
   memset(tape_array_, 0, sizeof (FluxInversion)  * array_size_);

   current_level_ = (flag == 0x01);
   tape_array_[nb_inversions_].high = current_level_;
   tape_array_[nb_inversions_].block_number = -1;
   tape_array_[nb_inversions_].block_type = 0x18;
   tape_array_[nb_inversions_].length = 0;

   DecodeCSWBloc(ptr_buffer, size - index_pos, static_cast<unsigned int>(sample_rate_), compress_type, num_pulse);

   PushPause(1000);
   is_tape_inserted_ = true;
   play_ = true;
   Reset();
   return 0;
}

int CTape::LoadCSW (const char* file)
{
   FILE * f;
   int ret = -1;
   sample_rate_ = 0;

   if (fopen_s (&f, file, "rb") == 0)
   {
      fseek(f, 0, SEEK_END);
      int buffer_size = ftell(f);
      rewind(f);
      unsigned char* buffer = new unsigned char[buffer_size];

      fread(buffer, buffer_size, 1, f);
      fclose(f);
      ret = LoadCSW(buffer, buffer_size);

      delete[]buffer;
   }
   return ret;
}

int CTape::LoadTZX(unsigned char* buffer, size_t size)
{
   // Init default values
   pilot_pulse_ = 2168;
   pilot_length_ = 4096;
   zero_ = 855;
   one_ = 1710;
   unsigned int place_in_file = 0;

   // Read format chunk
   unsigned char header[12] = { 0 };
   //fread(header, 1, 10, file);
   memcpy(header, &buffer[place_in_file], 10);
   place_in_file += 10;

   // TZX Format : Check version
   int maj_rev = header[8];
   int min_rev = header[9];

   // Check revision
   if (maj_rev > 1)
   {
      return -2;
   }

   // Init internal structure
   array_size_ = 1024;
   nb_samples_ = 0;
   nb_inversions_ = 0;
   tape_array_ = new FluxInversion[array_size_];
   memset(tape_array_, 0, sizeof (FluxInversion) * array_size_);

   // Read next chunk
   bool not_finished = true;

   current_level_ = false;
   tape_array_[nb_inversions_].high = current_level_;
   tape_array_[nb_inversions_].block_number = -1;
   tape_array_[nb_inversions_].block_type = 0;
   tape_array_[nb_inversions_].place = 0;
   tape_array_[nb_inversions_].length = 1; //4000000 * 2;

   int num = 0;
   while (not_finished)
   {
      if (place_in_file+1 < size )
      {
         memcpy(header, &buffer[place_in_file], 1);
         place_in_file++;

         // ID
         char buff_trace[256];
         num++;
         int s = (tape_array_[nb_inversions_].place / 4000000LL) % 60;
         int m = static_cast<int>((tape_array_[nb_inversions_].place / (4000 * 1000 * 60)));
         int ms = ((tape_array_[nb_inversions_].place / 4000LL) % (1000));

         // Add to block list
         BlockList block;
         block.block = s + (m * 60);
         block.description = NULL;

         if (nb_blocks_ + 1 >= size_of_blocklist_)
         {
            BlockList* tmp = block_list_;
            size_of_blocklist_ += 50;
            block_list_ = new BlockList[size_of_blocklist_];
            memcpy(block_list_, tmp, sizeof(BlockList) * nb_blocks_);
         }
         block_list_[nb_blocks_++] = block;
         

         sprintf(buff_trace, "Block %4.4i : ID : %2.2X - Pos : %2.2im %2.2is : %3.3ims", num, header[0], m, s, ms);
         LOG(buff_trace);
         // TRACE !
         sprintf(buff_trace, " - Offset du bloc : %i", place_in_file);
         LOG(buff_trace);
         current_block_ = num;
         current_block_type_ = header[0];

         switch (header[0])
         {
            // usefull chunks
         case 0x10:
         {
            //m_bCurrentLevel = false;
            //fread(header, 1, 4, file);
            if (place_in_file + 4 > size)
            {

               return -2;
            }
            memcpy(header, &buffer[place_in_file], 4);
            place_in_file += 4;

            // Standard Speed Data Block
            pilot_pulse_ = 2168;
            unsigned short sync_first_pulse = 667;
            unsigned short sync_2nd_pulse = 735;
            zero_ = 855;
            one_ = 1710;
            //m_nPilotLength = 3220; // 4096;
            pilot_length_ = 4095;

            // 0x00	-	WORD	Pause after this block (ms.) {1000}
            unsigned short pause = GET_WORD(header);

            // 0x02	N	WORD	Length of data that follow
            unsigned short length = GET_WORD((&header[2]));

            // 0x04	-	BYTE[N]	Data as in .TAP files
            if (place_in_file + length > size)
            {

               return -2;
            }

            auto tap_buffer = new unsigned char[length];
            //fread(tap_buffer, 1, nLength, file);
            memcpy(tap_buffer, &buffer[place_in_file], length);
            place_in_file += length;
            // Compute all of this :
            // Write Pilot
            //
            unsigned short tmp = one_;
            one_ = pilot_pulse_;
            for (int cpt = 0; cpt < pilot_length_; cpt++)
            {
               PushPulse ( pilot_pulse_ );
               //PushBit(1);
            }
            one_ = tmp;

            // Write Sync ; 2 Pulse
            PushPulse(sync_first_pulse);
            PushPulse(sync_2nd_pulse);

            // Handle these datas
            DecodeTapBuffer(tap_buffer, length, 8);
            PushPause(pause);
            delete[]tap_buffer;
            break;
         }

         case 0x11:  
            // Turbo speed data bloc
         {
            unsigned char buffer11[0x12];

            if (place_in_file + 18 > size)
            {

               return -2;
            }
            memcpy(buffer11, &buffer[place_in_file], 18);
            //fread(buffer11, 1, 18, file);

            place_in_file += 18;
            //
            pilot_pulse_ = GET_WORD((&buffer11[0]));
            unsigned short sync_first_pulse = GET_WORD((&buffer11[2]));
            unsigned short sync_2nd_pulse = GET_WORD((&buffer11[4]));
            zero_ = GET_WORD((&buffer11[6]));
            one_ = GET_WORD((&buffer11[8]));
            pilot_length_ = GET_WORD((&buffer11[0x0A]));
            unsigned char used_bits_last_byte = buffer11[0xC];
            unsigned short n_pause_after_this_block = GET_WORD((&buffer11[0x0D]));
            unsigned int data_length = buffer11[0x0F] + (buffer11[0x10] << 8) + (buffer11[0x11] << 16);

            // Read data
            if (place_in_file + data_length > size)
            {

               return -2;
            }
            unsigned char* tap_buffer = new unsigned char[data_length];
            memcpy(tap_buffer, &buffer[place_in_file], data_length);


            place_in_file += data_length;
            // Compute all of this :
            // Write Pilot
            //
            unsigned short tmp = one_;
            one_ = pilot_pulse_;
            for (int cpt = 0; cpt < pilot_length_; cpt++)
            {
               // if PushBit, prevents Mickey Mouse from working
               PushPulse(pilot_pulse_&0xFFFFFFFE);
            }
            one_ = tmp;

            // Write Sync ; 2 Pulse
            PushPulse(sync_first_pulse);
            PushPulse(sync_2nd_pulse);

            // Decode TAP buffer
            DecodeTapBuffer(tap_buffer, data_length, used_bits_last_byte);

            delete[]tap_buffer;

            // Add a pause
            PushPause(n_pause_after_this_block);
         }
         break;

         case 0x12:
         {
            unsigned char buffer12[4];
            if (place_in_file + 4 > size)
            {

               return -2;
            }
            memcpy(buffer12, &buffer[place_in_file], 4);
            place_in_file += 4;

            unsigned short length_of_pulse = GET_WORD((&buffer12[0]));
            unsigned short nb_pulse = GET_WORD((&buffer12[02]));
            for (int cpt = 0; cpt < nb_pulse; cpt++)
            {
               PushPulse(length_of_pulse);
            }

         }
         break;

         case 0x13:
         {
            if (place_in_file + 1 > size)
            {

               return -2;
            }
            memcpy(header, &buffer[place_in_file], 1);
            place_in_file += 1;

            unsigned char count = header[0];
            if (place_in_file + count * 2 > size)
            {

               return -2;
            }
            unsigned char* buffer13 = new unsigned char[count * 2];
            memcpy(buffer13, &buffer[place_in_file], count * 2);
            place_in_file += count * 2;

            for (int i = 0; i < count; i++)
            {
               unsigned short len = GET_WORD((&buffer13[i * 2]));
               PushPulse(len);
            }
            delete[]buffer13;

         }
         break;
         case 0x14:
         {

            if (current_block_ >= 6915)
            {
               int dbg = 1;
            }
            unsigned char buffer14[0xA];
            if (place_in_file + 10 > size)
            {

               return -2;
            }
            memcpy(buffer14, &buffer[place_in_file], 10);
            //fread(buffer14, 1, 10, file);
            place_in_file += 10;

            zero_ = GET_WORD((&buffer14[0]));
            one_ = GET_WORD((&buffer14[2]));


            unsigned char UsedBitsLastByte = buffer14[4];
            // TODO : remove ?? This make "Mask" work...
            /*if ( UsedBitsLastByte < 8)
            UsedBitsLastByte ++;*/
            //UsedBitsLastByte = 8;


            //m_nPilotLength = (m_nPilotLength >>( 8-UsedBitsLastByte));

            unsigned short nPauseAfterThisBlock = GET_WORD((&buffer14[5]));
            unsigned int data_length = buffer14[7] + (buffer14[8] << 8) + (buffer14[9] << 16);

            // Read data
            if (place_in_file + data_length > size)
            {

               return -2;
            }
            unsigned char* tap_buffer = new unsigned char[data_length];
            memcpy(tap_buffer, &buffer[place_in_file], data_length);
            //fread(tap_buffer, 1, data_length, file);
            place_in_file += data_length;
            // Compute all of this :
            // Decode TAP buffer
            DecodeTapBuffer(tap_buffer, data_length, UsedBitsLastByte);

            delete[]tap_buffer;


            // Add a pause
            if (nPauseAfterThisBlock > 0)
            {
               PushPause(nPauseAfterThisBlock);
            }
            else
            {
               int dbg = 1;
               /*if (m_bCurrentLevel)
               PushPulse ( m_pTapeArray[m_NbInversions].Length );*/
            }

         }

         break;
         case 0x15:// Direct Recording
         {
            // Pause ??
            if (place_in_file + 8 > size)
            {

               return -2;
            }
            memcpy(header, &buffer[place_in_file], 8);
            //fread(header, 1, 8, file);
            place_in_file += 8;
            //PushPulse (0 );
            unsigned int nbTStates = (unsigned int)((GET_WORD(header))   * RATIO_FREQ);
            unsigned short nPause = GET_WORD((&header[2]));
            unsigned char UsedBitsLastByte = header[4];
            unsigned int data_length = header[5] + (header[6] << 8) + (header[7] << 16);
            // Read data
            if (place_in_file + data_length > size)
            {

               return -2;
            }
            unsigned char* tap_buffer = new unsigned char[data_length];
            memcpy(tap_buffer, &buffer[place_in_file], data_length);
            //            fread(tap_buffer, 1, data_length, file);
            place_in_file += data_length;
            // Compute each samples
            // First one . ?
            //
            int last_index = 0;
            for (unsigned int i = 0; i < ((data_length - 1) * 8 + UsedBitsLastByte); i++)
            {
               // Inversion ?
               bool bLevel = (((tap_buffer[i >> 3]) >> (7 - (i & 7)))&(0x1));
               if (bLevel != tape_array_[nb_inversions_].high)
               {
                  IncInversions();
                  tape_array_[nb_inversions_].length = 0;
                  tape_array_[nb_inversions_].place = tape_array_[nb_inversions_ - 1].place;
                  last_index = i;
               }
               tape_array_[nb_inversions_].high = bLevel;

               tape_array_[nb_inversions_].place += nbTStates;
               tape_array_[nb_inversions_].length += nbTStates;
               //m_pTapeArray[m_NbInversions].Length = ((i - lastIndex + 1 )*nbTStates)/* * RATIO_FREQ*/;


            }
            current_level_ = tape_array_[nb_inversions_].high;

            // Trailer (32 bits ?)
            /*for (int cpt = 0; cpt < 32; cpt++)
            {
            PushBit ( 1 );
            }*/

            PushPause(nPause);

            delete[]tap_buffer;

            break;
         }


         case 0x20:
         {
            // Pause (silence) or 'Stop the Tape' command
            if (place_in_file + 2 > size)
            {

            return -2;
            }
            memcpy(header, &buffer[place_in_file], 2);
            place_in_file += 2;
            unsigned short npause = GET_WORD(header);
            // Add pause to the tracks
            PushPause(npause);
            break;
         }
         case 0x2B:
         {
            // Set signal level
            if (place_in_file + 5 > size)
            {

               return -2;
            }
            memcpy(header, &buffer[place_in_file], 5);
            place_in_file += 5;
            // Set signal
            bool bLevel = (header[4] == 1) ? true : false;

            current_level_ = bLevel;
            break;
         }
         case 0x33:
         {
            if (place_in_file + 1 > size)
            {

               return -2;
            }
            memcpy(header, &buffer[place_in_file], 1);

            place_in_file += 1;
            int data_length = header[0] * 3;
            if (place_in_file + data_length > size)
            {

               return -2;
            }
            unsigned char* data_buffer = new unsigned char[data_length];
            memcpy(data_buffer, &buffer[place_in_file], data_length);
            
            place_in_file += data_length;
            delete[]data_buffer;
            break;
         }
         case 0x35:
         {
            if (place_in_file + 0x14 > size)
            {

               return -2;
            }
            unsigned char* data_buffer = new unsigned char[0x14];
            memcpy(data_buffer, &buffer[place_in_file], 0x14);
            place_in_file += 0x14;
            int data_length = GET_DWORD((&data_buffer[0x10]));

            delete[]data_buffer;
            if (place_in_file + data_length > size)
            {

               return -2;
            }
            data_buffer = new unsigned char[data_length];
            memcpy(data_buffer, &buffer[place_in_file], data_length);
            place_in_file += data_length;
            delete[]data_buffer;
            break;
         }
         case 0x5A:
            // Glue code
            if (place_in_file + 9 > size)
            {
               return -2;
            }
            memcpy(header, &buffer[place_in_file], 9);

            place_in_file += 9;
            break;

            //////////////////////////////////
            // Neutral chunk

         case 0x18:
         {
            // CSW Recording
            if (place_in_file + 4 > size)
            {

               return -2;
            }
            memcpy(header, &buffer[place_in_file], 4);
            place_in_file += 4;
            unsigned int length = GET_DWORD(header);

            // Read the header buffer
            if (place_in_file + length > size)
            {

               return -2;
            }
            unsigned char* data_buffer = new unsigned char[length];
            memcpy(data_buffer, &buffer[place_in_file], length);
            place_in_file += length;
            // Sampling rate
            int SampleRate = data_buffer[2]
               | (data_buffer[3] << 8)
               | (data_buffer[4] << 16);
            int type = data_buffer[5];
            int nbPulse = data_buffer[6]
               | (data_buffer[7] << 8)
               | (data_buffer[8] << 16)
               | (data_buffer[9] << 24);


            DecodeCSWBloc(&(data_buffer[0x0A]), length - 0x0A, SampleRate, type, nbPulse);

            // Add a pause
            PushPause(data_buffer[0] + (data_buffer[1] << 8));

            delete[]data_buffer;
            break;
         }
         case 0x19:
         {
            if (place_in_file + 4 > size)
            {

               return -2;
            }
            memcpy(header, &buffer[place_in_file], 4);
            place_in_file += 4;
            unsigned int length = GET_DWORD(header);

            // Read the header buffer
            if (place_in_file + length > size)
            {

               return -2;
            }
            unsigned char* data_buffer = new unsigned char[length];
            memcpy(data_buffer, &buffer[place_in_file], length);
            place_in_file += length;
            // TODO

            delete[]data_buffer;
            break;
         }
         case 0x21:
            // Group start
         {
            if (place_in_file + 1 > size)
            {

               return -2;
            }
            memcpy(header, &buffer[place_in_file], 1);
            place_in_file += 1;
            unsigned char l = header[0];
            unsigned char data_buffer[256];
            if (place_in_file + l > size)
            {

               return -2;
            }
            memcpy(data_buffer, &buffer[place_in_file], l);
            place_in_file += l;
            break;
         }
         case 0x22:
            // End group - nothing more to do
            break;


         case 0x23:
            break;
         case 0x24:
            break;
         case 0x25:
            break;
         case 0x26:
            break;
         case 0x27:
            break;
         case 0x28:
            break;
         case 0x30:
            // Text description
         {
            if (place_in_file + 1 > size)
            {

               return -2;
            }
            memcpy(header, &buffer[place_in_file], 1);
            place_in_file += 1;
            unsigned char l = header[0];
            unsigned char data_buffer[256] = { 0 };
            if (place_in_file + l > size)
            {

               return -2;
            }
            memcpy(data_buffer, &buffer[place_in_file], l);
            place_in_file += l;
            block_list_[nb_blocks_-1].description = new char[l + 1];
            size_t converted_chars = 0;
            strcpy(block_list_[nb_blocks_ - 1].description, (const char*)data_buffer);
            // Save this for display purpose
            break;
         }
         case 0x31:
         {
            // Message block
            unsigned char data_buffer[256];
            if (place_in_file + 2 > size)
            {

               return -2;
            }
            memcpy(header, &buffer[place_in_file], 2);
            place_in_file += 2;
            if (place_in_file + header[1] > size)
            {

               return -2;
            }
            memcpy(data_buffer, &buffer[place_in_file], header[1]);
            place_in_file += header[1];
            break;
         }
         case 0x32:
         {
            // Read size
            unsigned char data_buffer[256];
            if (place_in_file + 2 > size)
            {
               return -2;
            }
            memcpy(data_buffer, &buffer[place_in_file], 2);
            unsigned int data_length = data_buffer[0] + (data_buffer[1] << 8);
            place_in_file += 2;
            if (place_in_file + 2 + data_length > size)
            {
               return -2;
            }
            unsigned char* buffer_32 = new unsigned char[data_length];
            memcpy(buffer_32, &buffer[place_in_file], data_length);
            // Read number of strings
            unsigned int buffer_index = 0;
            unsigned char nb_strings = buffer_32[buffer_index++];
            place_in_file += data_length;
            // Read each strings
            
            char str_id[256+1];
            memset(str_id, 0, sizeof(str_id));
            for (unsigned int i = 0; i < nb_strings; ++i)
            {
               if (buffer_index + 2 > data_length) return -2;

               // ID
               unsigned char id = buffer_32[buffer_index++];

               // Size
               unsigned char str_size = buffer_32[buffer_index++];

               // String
               if (buffer_index + str_size > data_length) return -2;
               memcpy(str_id, &buffer_32[buffer_index], str_size);
               buffer_index += str_size;
               LOG(str_id);
               LOGEOL
               memset(str_id, 0, sizeof(str_id));
            }
            break;
         }
            
            //////////////////////////////////
            // No need to handle on CPC, just shunt them, or deprecated ones
         case 0x34:
         {
            unsigned char length[8];
            if (place_in_file + 8 > size)
            {

               return -2;
            }
            memcpy(length, &buffer[place_in_file], 8);
            place_in_file += 8;
         }
         break;
         case 0x40:
         {
            unsigned char length[4];
            if (place_in_file + 4 > size)
            {

               return -2;
            }
            memcpy(length, &buffer[place_in_file], 4);
            place_in_file += 4;
            unsigned int data_length = length[1] + (length[2] << 8) + (length[3] << 16);

            // Read data
            if (place_in_file + data_length > size)
            {

               return -2;
            }
            unsigned char* tap_buffer = new unsigned char[data_length];
            memcpy(tap_buffer, &buffer[place_in_file], data_length);
            place_in_file += data_length;
            delete[]tap_buffer;
         }
         break;
         case 0x16:
         case 0x17:
         case 0x2A:
            //////////////////////////////////
         default:   // Unknown chunk : Read size then skip datas (extension rules)
         {
            // Check for added blocks ("File...")
            unsigned char length[4];
            if (place_in_file + 4 > size)
            {

               return -2;
            }
            memcpy(length, &buffer[place_in_file], 4);
            if (memcmp(&buffer[place_in_file-1], "File", 4) == 0)
            {
               not_finished = false;
            }
            else
            {
               place_in_file += 4;
               unsigned int l = GET_DWORD(length);
               if (place_in_file + l > size)
               {

                  return -2;
               }
               unsigned char* data_buffer = new unsigned char[l];
               memcpy(data_buffer, &buffer[place_in_file], l);
               place_in_file += l;
               delete[]data_buffer;
            }
            break;
         }
         }
         LOGEOL;

      }
      else
      {
         // End properly last wave
         if (current_level_)
         {
            PushPulse(1000);
         }
         not_finished = false;
      }

   }

   // Add an ending level length (to correct powerboat for example)
   PushPause(1000);
   is_tape_inserted_ = true;
   play_ = true;
   Reset();
   return 0;
}

int CTape::LoadTZX (const char* file)
{
   FILE * f;

   // Init default values
   pilot_pulse_ = 2168;
   pilot_length_ = 4096;
   zero_ = 855;
   one_ = 1710;
   int ret = -1;

   if (fopen_s (&f, file, "rb") == 0)
   {
      fseek(f, 0, SEEK_END);
      int buffer_size = ftell(f);
      rewind(f);
      unsigned char* buffer = new unsigned char[buffer_size];

      fread(buffer, buffer_size, 1, f);
      fclose(f);
      ret = LoadTZX(buffer, buffer_size);

      delete[]buffer;
   }

   return ret;
}

#define MULTIPLIER 3/2
#define ADDER 100000
void CTape::IncInversions ()
{
   ++nb_inversions_;
   if ( nb_inversions_ >= array_size_ )
   {
      unsigned int new_size = array_size_ * 2;
      FluxInversion* tmp = new FluxInversion[new_size];
      memset ( tmp, 0, (new_size)* sizeof(FluxInversion));
      memcpy ( tmp, tape_array_, sizeof (FluxInversion) * array_size_ );
      array_size_  = new_size;
      delete[]tape_array_;
      tape_array_ = tmp;
   }

   tape_array_[nb_inversions_].block_number = current_block_ ;
   tape_array_[nb_inversions_].block_type = current_block_type_;
}

int CTape::LoadWav(unsigned char* buffer, size_t size)
{
   Eject ();
   // Read format chunk
   unsigned int index_pos = 0;
   current_block_ = 0;
   current_block_type_ = 0;
   unsigned char header[12] = { 0 };
   memcpy(header, &buffer[index_pos], 12);
   index_pos += 12;

   // Keep track of file size
   int file_size = header[4]
      + (header[5] << 8)
      + (header[6] << 16)
      + (header[7] << 24);

   // Check file format
   if (memcmp(&header[8], "WAVE", 4) == 0)
   {
      unsigned char fmt[24] = { 0 };
      //fread(fmt, 1, 8, file);
      memcpy(fmt, &buffer[index_pos], 8);
      index_pos += 8;


      if (memcmp(fmt, "fmt ", 4) == 0)
      {
         int format_size = fmt[4]
            + (fmt[5] << 8)
            + (fmt[6] << 16)
            + (fmt[7] << 24);

         // Should be 0x10 -
         if (format_size <= 24)
         {
            //fread(fmt, 1, formatSize, file);
            memcpy(fmt, &buffer[index_pos], format_size);
            index_pos += format_size;

            // Type of format
            int format = fmt[0] + (fmt[1] << 8);
            unsigned int nb_channels = fmt[2] + (fmt[3] << 8);
            frequency_ = fmt[4]
               + (fmt[5] << 8)
               + (fmt[6] << 16)
               + (fmt[7] << 24);
            sample_rate_ = 1.0 / frequency_;
            // Sample conversion to tick
            // One tick = 4 Mhz
            // One sample = freq hz
            // one sample = 4 Mhz / freq
            unsigned int bit_per_sample = fmt[14] + (fmt[15] << 8);
#ifndef NOFILTER
            StandardFilter filter(frequency_, filter_type_hp_, fc_hp_, filter_order_hp_, filter_type_lp_, fc_lp_, filter_order_lp_, filter_gain_);
            Filter hp_filter (filter_type_hp_, false, frequency_, fc_hp_, filter_order_hp_);
            Filter lp_filter (filter_type_lp_, true, frequency_, fc_lp_, filter_order_lp_);
#endif
            // Other chunks :
            bool not_finished = true;

            while (not_finished)
            {
               //if (fread(header, 1, 8, file) == 8)
               if (index_pos + 8 > size)
               {
                  return -2;
               }
               memcpy(header, &buffer[index_pos], 8);
               index_pos += 8;

               {
                  // Data chunk
                  unsigned int datasize = header[4]
                     + (header[5] << 8)
                     + (header[6] << 16)
                     + (header[7] << 24);

                  if (index_pos + datasize > size)
                  {
                     //return -2;
                     datasize = size - index_pos;
                  }

                  unsigned char *chunk = &buffer[index_pos]; // new unsigned char[Datasize];
                  //fread(chunk, 1, Datasize, file);
                  //memcpy(chunk, &buffer[index_pos], Datasize);
                  index_pos += datasize;


                  if (memcmp(header, "data ", 4) == 0)
                  {
                     // Handle datas !
                     // Initialize the while thing :
                     bool first = true;
                     // Init the array with 1024 inversions
                     array_size_ = 1024;
                     nb_samples_ = 0;
                     nb_inversions_ = 0;
                     tape_array_ = new FluxInversion[array_size_];
                     memset(tape_array_, 0, sizeof (FluxInversion)  * array_size_);

                     unsigned int nb_samples = (datasize) / ((bit_per_sample / 8) * nb_channels);

                     // First record from the first sample
                     unsigned int offset = 0;

                     bool square_wave = CTape::IsArraySquareWave(chunk, nb_samples, nb_channels, bit_per_sample);

                     // Read evey samples and make them consistents, which is : One value, between -1 and 1, for evey sample (despite of number of encoding bits, channels, or other...)
                     unsigned int sample_count;
                     const unsigned int sample_buffer = frequency_ * 60; // One minute buffer
                     double* value_array = new double[sample_buffer];
                     while (nb_samples > 0)
                     {
                        if (nb_samples < sample_buffer)
                        {
                           sample_count = nb_samples;
                        }
                        else
                        {
                           sample_count = sample_buffer;
                        }
                        nb_samples -= sample_count;

                        for (unsigned int i = 0; i < sample_count && offset < datasize; i++)
                        {
                           // Next sample
                           value_array[i] = GetSampleValue(chunk, &offset, nb_channels, bit_per_sample);

                        }

                        // Handle this sample array                        
                        ConvertSampleArray(first, value_array, sample_count, frequency_
#ifndef NOFILTER
                           , square_wave?nullptr:&filter
#endif
                        );
                        first = false;

                        // Progress
                        float f = (float)(index_pos - datasize + offset) / (float)size;

                        SetProgress(static_cast<int>(f*100.0f));

                     }
                     delete[]value_array;

                     is_tape_inserted_ = true;
                     play_ = true;
                     Reset();


                     return 0;
                  }
                  //delete[]chunk;
               }
            }
            return 0;
         }
      }
      return 0;
   }
   return -1;
}

int CTape::LoadWav (const char* filepath)
{
   FILE * f;
   int ret = -1;
   if (fopen_s (&f, filepath, "rb") == 0)
   {
      fseek(f, 0, SEEK_END);
      int buffer_size = ftell(f);
      rewind(f);
      unsigned char* buffer = new unsigned char[buffer_size];

      fread(buffer, buffer_size, 1, f);
      fclose(f);
      ret = LoadWav(buffer, buffer_size);

      delete[]buffer;

   }
   return ret;
}


double CTape::GetSoundVolume ()
{
   return tape_array_[tape_position_].high ? 1:-1;
}

void CTape::SetTapePosition ( unsigned int nb_sec_from_begining)
{
   if (nb_inversions_ > 0)
   {
      unsigned long long nbu_sec = 0;
      int nb_sec = 0;
      unsigned int i;
      for (i = 0; i < nb_inversions_ && (nb_sec < nb_sec_from_begining); i++)
      {
         // Size of this inversion / sample rate = nb of samples
         nbu_sec += tape_array_[i].length;
         while (nbu_sec > 4000000)
         {
            nb_sec++;
            nbu_sec -= 4000000;
         }
      }
      tape_position_ = i;
      remaining_reversal_flux_ = tape_array_[tape_position_].length - nbu_sec;
      counter_sec_ = nb_sec;
      counter_us_ = nbu_sec;
   }

}

void CTape::ComputeLenghtOfTape ()
{
   tape_length_ = 0;
   unsigned long long nbu_sec = 0;
   for (unsigned int i = 0; i < nb_inversions_; i++)
   {
      // Size of this inversion / sample rate = nb of samples
      nbu_sec += tape_array_[i].length ;
      while ( nbu_sec > 4000000)
      {
         tape_length_ ++;
         nbu_sec -= 4000000;
      }
   }
}

unsigned int CTape::Tick (/*unsigned int nbTicks*/)
{
   // Something to load ?

   if (pending_tape_)
   {
      if (tape_buffer_size_ != 0)
      {
         if (tape_notifier_) tape_notifier_->ItemLoaded(current_tape_.c_str(), InsertTape(tape_buffer_to_load_, tape_buffer_size_), -1);
      }
      else
      {
         InsertTapeDelayed();
      }
      pending_tape_ = false;
      delete[]tape_buffer_to_load_;
      tape_buffer_to_load_ = nullptr;
      tape_buffer_size_ = 0;
   }
   // Counter
   bool motor_to_use = motor_on_;
   if (time_to_change_motor_state_ > 0)
   {
      time_to_change_motor_state_ -= this_tick_time_;

      if (time_to_change_motor_state_ <= 0)
      {
         motor_on_ = next_motor_state_;
         time_to_change_motor_state_ = 0;

         if ( !motor_on_ )ppi8255_->SetDataRecorderR(false);
      }

   }


   if (motor_to_use)
   {
      // If motor is just on, then dont add this ticks !
      if (!previous_motor_on_ )
         previous_motor_on_ = true;
      else
      {
         if ( tape_position_ <= nb_inversions_)
         {
            counter_us_ += this_tick_time_;
            if ( counter_us_ > 4000000)
            {
               counter_sec_ ++;
               counter_us_ -= 4000000;
            }
         }
      }

      // Recording ?
      if (record_)
      {
         // Get current level
         bool level = (ppi8255_!=nullptr)?ppi8255_->GetCassetteWriteData ():true;

         // If same as previous one,
         if ( tape_array_[tape_position_].high == level)
         {
            // just add the proper length
            tape_array_[tape_position_].length += this_tick_time_;
         }
         else
         {
            // Otherwise,
            // Shift remaining array
            nb_inversions_ += 1;
            if ( nb_inversions_ >= array_size_ )
            {
               FluxInversion* tmp = new FluxInversion[array_size_ *2];
               memset ( tmp, 0, array_size_ *2*sizeof (FluxInversion));
               memcpy ( tmp, tape_array_, sizeof (FluxInversion) * array_size_ );
               array_size_  = array_size_  * 2;
               delete[]tape_array_;
               tape_array_ = tmp;
            }
            // Move m_PosOfTape+1 tape by 2
            memmove ( &tape_array_[tape_position_+2], &tape_array_[tape_position_+1], sizeof (FluxInversion) *  (nb_inversions_ - (tape_position_+2) ));
            // Create new one
            ++tape_position_;
            tape_array_[tape_position_].high = level;
            tape_array_[tape_position_].length = this_tick_time_;
            tape_array_[tape_position_].block_number = 0;
            tape_array_[tape_position_].block_type = 0;
            tape_array_[tape_position_].place = tape_array_[tape_position_-1].place + tape_array_[tape_position_-1].length;
         }

         // Shorten now the following one.
         if ( tape_position_ + 1 < nb_inversions_)
         {
            unsigned long long nb_tick_to_remove = this_tick_time_;
            int i = 1;
            while ( tape_array_[tape_position_+i].length < nb_tick_to_remove && ( tape_position_ + i < nb_inversions_) )
            {
               // Remove this one !
               tape_array_[tape_position_+i].length = 0;
               // Reduce nb ticks
               nb_tick_to_remove -= tape_array_[tape_position_+i].length;
               ++i;

            }
            // Last one exists ?
            if ( tape_position_ + 1 < nb_inversions_)
            {
               tape_array_[tape_position_+1].length -= this_tick_time_;
            }
         }

         // Set the play cursor at the right place
         remaining_reversal_flux_ = 1;
      }
      else
      {
         // If motor is off, or no tape is in, nothing happen
         // Do we have something ?
         if (remaining_reversal_flux_ > this_tick_time_)
         {
            // Yes : Write the proper value to PPI port B
            remaining_reversal_flux_ -= this_tick_time_;
         }
         else
         {
            // Next reversal flux please !
            ++tape_position_;

            if ( tape_position_ > nb_inversions_)
            {
               // End of tape
               play_ = false;
               //m_bMotor = false;
               ppi8255_->SetDataRecorderR ( false);
               if (time_to_change_motor_state_ == 0)
                  return this_tick_time_ = 4000000;
               else
                  return time_to_change_motor_state_;
            }
            else
            {
               if (tape_array_[tape_position_].length > ( this_tick_time_ - remaining_reversal_flux_))
               {
                  remaining_reversal_flux_ = tape_array_[tape_position_].length - ( this_tick_time_ - remaining_reversal_flux_);

                  // Set the new value !
                  if (is_tape_inserted_ && play_ )
                  {
                     // Only if motor is not in a stop process
                     if (!polarity_inversion_)
                     {
                        ppi8255_->SetDataRecorderR(tape_array_[tape_position_].high ? true : false);
                     }
                     else
                     {
                        ppi8255_->SetDataRecorderR(tape_array_[tape_position_].high ? false : true);
                     }
                  }
               }
               else
               {
                  remaining_reversal_flux_ = 1;
                  int dbg = 1;
               }

            }
         }
         // If start of recording :
         if ( start_record_ )
         {
            record_ = true;
            start_record_ = false;

            // Set on the exact position of the tape is already done.

            // End this : start a new one, if opposite side.
            bool level = (ppi8255_ != nullptr) ? ppi8255_->GetCassetteWriteData ():true;
            tape_array_[tape_position_].length -= remaining_reversal_flux_;
            tape_changed_ = true;

            if ( tape_array_[tape_position_].high != level)
            {
               nb_inversions_ += 2;
               if ( nb_inversions_ >= array_size_ )
               {
                  FluxInversion* tmp = new FluxInversion[array_size_ *2];
                  memset ( tmp, 0, array_size_ *2 *sizeof (FluxInversion));
                  memcpy ( tmp, tape_array_, sizeof (FluxInversion) * array_size_ );
                  array_size_  = array_size_  * 2;
                  delete[]tape_array_;
                  tape_array_ = tmp;
               }
               // Move m_PosOfTape+1 tape by 2
               memmove ( &tape_array_[tape_position_+2], &tape_array_[tape_position_], sizeof (FluxInversion) *  (nb_inversions_ - (tape_position_+2) ));
               tape_position_++;
               // New one
               tape_array_[tape_position_].high = level;
               tape_array_[tape_position_].block_number = 0;
               tape_array_[tape_position_].block_type = 0;
               tape_array_[tape_position_].length = 0;
               tape_array_[tape_position_].place = tape_array_[tape_position_-1].place +tape_array_[tape_position_-1].length;

               // Remaining of the last one
               tape_array_[tape_position_+1].high = tape_array_[tape_position_-1].high;
               tape_array_[tape_position_+1].block_number = 0;
               tape_array_[tape_position_+1].block_type = 0;
               tape_array_[tape_position_+1].length = remaining_reversal_flux_;
               tape_array_[tape_position_+1].place = tape_array_[tape_position_].place +tape_array_[tape_position_].length;

            }
            else
            {
               // Shift news identic one
               nb_inversions_ += 1;
               if ( nb_inversions_ >= array_size_ )
               {
                  FluxInversion* tmp = new FluxInversion[array_size_ *2];
                  memset ( tmp, 0, array_size_ *2 *sizeof (FluxInversion));
                  memcpy ( tmp, tape_array_, sizeof (FluxInversion) * array_size_ );
                  array_size_  = array_size_  * 2;
                  delete[]tape_array_;
                  tape_array_ = tmp;
               }
               // Move m_PosOfTape+1 tape by 2
               memmove ( &tape_array_[tape_position_+1], &tape_array_[tape_position_], sizeof (FluxInversion) *  (nb_inversions_ - (tape_position_+1) ));
               // New one
               tape_array_[tape_position_+1].high = level;
               tape_array_[tape_position_+1].block_number = 0;
               tape_array_[tape_position_+1].block_type = 0;
               tape_array_[tape_position_+1].length = remaining_reversal_flux_;
               tape_array_[tape_position_+1].place = tape_array_[tape_position_-1].place +tape_array_[tape_position_-1].length;

            }
         }

      }
   }
   else
   {
      // No motor : We have lot's of time now !
      if (time_to_change_motor_state_ > 0)
         this_tick_time_ = time_to_change_motor_state_;
      else
         this_tick_time_ = 32;// 4000000;
      return this_tick_time_;
   }

   if (record_)
      return this_tick_time_=4;

   this_tick_time_= static_cast<int>(( remaining_reversal_flux_ > 4000000)?4000000:remaining_reversal_flux_);
   if (time_to_change_motor_state_ == 0)
      return this_tick_time_;
   if (this_tick_time_ > time_to_change_motor_state_)
      return time_to_change_motor_state_;
   else
      return this_tick_time_;
}

void CTape::SetMotorOn ( bool bOn )
{
   if (motor_on_ != bOn || (time_to_change_motor_state_ > 0 && bOn != next_motor_state_))
   {
      if (time_to_change_motor_state_ > 0 && bOn == next_motor_state_) return;

      LOG (bOn?"K7 Motor On":"K7 Motor Off");
      LOGEOL

        /* m_bMotor = */next_motor_state_ = bOn;
      time_to_change_motor_state_ = MOTOR_DELAY;
      //m_bMotor = bOn;
      machine_->ForceTick ( this, 1) ;
   }
}

void CTape::Record ()
{
   machine_->ForceTick ( this) ;
   start_record_ = true;
}

void CTape::Play ()
{
   play_ = true;
   machine_->ForceTick ( this, 1) ;
   // Force execute : TODO
}

void CTape::Rewind ()
{
   tape_position_ = 0;
   counter_sec_ = 0;
   counter_us_ = 0;
   if (tape_array_ != NULL)
      remaining_reversal_flux_ = tape_array_[tape_position_].length;

   machine_->ForceTick ( this) ;
}

void CTape::FastForward()
{
   machine_->ForceTick ( this) ;
}

void CTape::StopEject ()
{
   play_ = false;
   machine_->ForceTick ( this) ;
}

void CTape::Pause ()
{
   play_ = false;
   machine_->ForceTick ( this) ;
}

void CTape::SaveAsWav (const char* filepath)
{
   FILE * file;

   double sample_rate = 44100.0;

   if (fopen_s (&file, filepath, "wb") == 0)
   {
      int size = 0;
      size += fwrite ( "RIFF", 1, 4, file  );
      size += WriteInt ( 0, file  );
      size += fwrite ( "WAVE", 1, 4, file  );

      int blockSize = 0x10;
      size += fwrite ( "fmt ", 1, 4, file  );
      size += WriteInt ( blockSize, file  );

      size += WriteShort ( 1, file  ); // PCM
      size += WriteShort ( 1, file  ); // 1 channel
      size += WriteInt ( (int)sample_rate, file  ); // 11025 Hz
      size += WriteInt ( (int)sample_rate, file  ); //
      size += WriteShort ( 1, file  ); //
      size += WriteShort ( 8, file  ); //

      // Data
      size += fwrite ( "data ", 1, 4, file  );
      unsigned int data_size_place = size;
      size += WriteInt ( 0, file  ); //

      // Save as WAV : Sample the whole inversion array
      bool ended = nb_inversions_ == 0;

#define WRITE_BUFFER_SIZE 4096
      unsigned int buffer_count = 0;
      char write_buffer[WRITE_BUFFER_SIZE] = { 0 };
      unsigned int current_inversion = 0;
      // Rewind the tape
      // Play it 
#if defined (__unix) || (RASPPI) || (__APPLE__)
      __uint64_t total = 0;
      __uint64_t current_length = 0;
#else
      unsigned _int64 total = 0;
      unsigned _int64 current_length = 0;
#endif
      unsigned short block_number = 0;
      current_length = tape_array_[current_inversion].length;
      current_length = static_cast<unsigned long long > (current_length * sample_rate);

      do
      {
         char value = tape_array_[current_inversion].high?0xFF:0;

         // Add value to the write buffer
         write_buffer[buffer_count++] = value;

         // Is it full ? 
         if (buffer_count == WRITE_BUFFER_SIZE)
         {
            // Write it
            size += fwrite(write_buffer, 1, WRITE_BUFFER_SIZE, file);
            // reset it
            buffer_count = 0;
         }

         // Add another sample to the current time
         total += 4000000;
         while (current_length < total && current_inversion < nb_inversions_)
         {
            total -= current_length;
            current_inversion++;
            current_length = tape_array_[current_inversion].length;
            current_length = static_cast<unsigned long long> ( sample_rate * current_length);
         }
         // is it in the current inversion ? 
         // No, use next (until it's the end).
         if (current_inversion >= nb_inversions_)
         {
            ended = true;
         }

      } while (!ended);
      if (buffer_count != 0)
      {
         // Write it
         size += fwrite(write_buffer, 1, buffer_count, file);
      }


      // Set data size
      fseek ( file, data_size_place, SEEK_SET );
      WriteInt ( size - 44, file  ); //

      // Set file size
      fseek ( file, 4, SEEK_SET );
      fwrite ( &size, 1, 4, file );
      fclose ( file );
      tape_changed_ = false;
   }
}

void CTape::SaveAsCdtDrb (const char* filepath)
{
   FILE * file;

   if (fopen_s (&file, filepath, "wb") == 0)
   {
      unsigned char buffer[10]={0};

      memcpy ( buffer, "ZXTape!", 7 );
      buffer[7] = 0x1A;
      buffer[8] = 1;
      buffer[9] = 20;

      size_t size = 0;
      size += fwrite ( buffer, 1, 10, file  );

      // DRB
      buffer[0] = 0x15;
      size += fwrite ( buffer, 1, 1, file  );

      double sample_rate = 44100.0;
#define T_STATES 4000000.0
      // Nb T-States / Sample :
      // 1 T state = 1/3500000 s.
      // 1 Sample = 1 / SampleRate s.
      // 1 sec = 3500000 T State = SampleRate samples.
      // 1 Sample = 3500000 / SampleRate T state.

      size += WriteShort (  static_cast<unsigned short>(T_STATES / sample_rate) , file  );

      // Pause after this bloc
      size += WriteShort ( 5000 , file  );

      // Used bit in last byte
      int pos_used_bit_last_samples = size;
      buffer[0] = 8;
      size += fwrite ( buffer, 1, 1, file  );

      // Data size
      buffer[0] = buffer[1] = buffer[2] = 0;;
      size += fwrite ( buffer, 1, 3, file  );


      // Data
      //size += fwrite ( "data ", 1, 4, file  );
      //unsigned int datasizeplace = size;
      //size += WriteInt ( 0, file  ); //
      size = 0;

      unsigned char used_bit_in_lastbye = 8;
      unsigned char car_buf = 0;
      int bit_counter = 0;
      unsigned long long remaining = 0;
      double mul = sample_rate / T_STATES;
      for (unsigned int i = 0; i <= nb_inversions_; i++)
      {
         unsigned long long l = static_cast<unsigned long long>(tape_array_[i].length / RATIO_FREQ);
         unsigned long long nb_samples = static_cast<unsigned long long>(Round (( l +remaining)* ( mul) ));

         remaining = static_cast<unsigned long long>((( l + remaining)* ( mul) - nb_samples) / ( mul));

         unsigned char bitBuf = (tape_array_[i].high?1:0);
         if ( i > nb_inversions_-4)
         {
            int dbg = 1;
         }
         for (int j = 0; j < nb_samples; j++)
         {

            // Size of this inversion / sample rate = nb of samples
            if ( bit_counter < 8 )
            {
               car_buf <<= 1;
            }
            else
            {
               // Write byte
               // Add a sample
               size += fwrite ( &car_buf, 1, 1, file );

               bit_counter = 0;
               car_buf = 0;
            }
            car_buf |= bitBuf;
            bit_counter++;
         }
      }
      // Last byte
      used_bit_in_lastbye = 8 - bit_counter;

      car_buf <<= used_bit_in_lastbye ;
      if ( bit_counter != 0 )
         size += fwrite ( &car_buf, 1, 1, file );

      fseek ( file, pos_used_bit_last_samples, SEEK_SET );
      fwrite ( &used_bit_in_lastbye, 1, 1, file );

      // Set data size
      // Convert size to char[3];
      buffer[0] = ((size)&0xFF);
      buffer[1] = (((size)>>8)&0xFF);
      buffer[2] = (((size)>>16)&0xFF);

      //  = Buffer[1] = Buffer[2] = 0;;
      size += fwrite ( buffer, 1, 3, file  );


      fclose ( file );
      tape_changed_ = false;
   }
}

void CTape::SaveAsCdtCSW (const char* filepath)
{
   FILE * file;

   if (fopen_s (&file, filepath, "wb") == 0)
   {
      int type  = 1;
      unsigned char buffer[10];

      memcpy ( buffer, "ZXTape!", 7 );
      buffer[7] = 0x1A;
      buffer[8] = 1;
      buffer[9] = 20;

      int size = 0;
      size += fwrite ( buffer, 1, 10, file  );

      // CSW
      buffer[0] = 0x18;
      size += fwrite ( buffer, 1, 1, file  );

      // Total length
      int post_of_length = size;
      size += WriteInt ( 0, file  );
      size = 0;

      // Pause after this bloc
      size += WriteShort ( 5000 , file  );

      double sample_rate = 44100.0;
      // Nb T-States / Sample :
      // 1 T state = 1/3500000 s.
      // 1 Sample = 1 / SampleRate s.
      // 1 sec = 3500000 T State = SampleRate samples.
      // 1 Sample = 3500000 / SampleRate T state.
      int sample_rate_i = static_cast<int>( sample_rate) ;
      buffer[0] = ((sample_rate_i )&0xFF);
      buffer[1] = (((sample_rate_i )>>8)&0xFF);
      buffer[2] = (((sample_rate_i )>>16)&0xFF);

      size += fwrite ( buffer, 1, 3, file  );

      // Compression
      // type
      size += fwrite ( type==1?"\1":"\2", 1, 1, file  );

      // Data size
      int pos_of_size = size;
      buffer[0] = buffer[1] = buffer[2] = buffer[3] = 0;
      size += fwrite ( buffer, 1, 4, file  );


      // Data
      //size += fwrite ( "data ", 1, 4, file  );
      //unsigned int datasizeplace = size;
      //size += WriteInt ( 0, file  ); //
      //size = 0;

      unsigned char used_bit_in_lastbye = 8;
      unsigned char car_buf = 0;
      int bit_counter = 0;
      unsigned int nb_sample_to_record = 0;
      double ratio = sample_rate / 4000000.0;

            for (unsigned int i = 0; i <= nb_inversions_; )
            {
               int nb_samples = static_cast<int>(Round ( tape_array_[i].length * ratio ));
               while ( i+1 <= nb_inversions_ && tape_array_[i].high == tape_array_[i+1].high)
               {
                  nb_samples += static_cast<int>(Round ( tape_array_[i+1].length * ratio ));
                  i++;
               }
               i++;
               // Add value
               if ( nb_samples != 0)
               {
                  ++nb_sample_to_record;
                  if ( nb_samples > 0xFF)
                  {
                     buffer[0] = 0;
                     buffer[1] = nb_samples&0xFF;
                     buffer[2] = (nb_samples>>8)&0xFF;
                     buffer[3] = (nb_samples>>16)&0xFF;
                     buffer[4] = (nb_samples>>24)&0xFF;
                     if  (type == 1)
                     {
                        size += fwrite ( buffer, 1, 5, file  );
                     }
                  }
                  else
                  {
                     buffer[0] = (nb_samples&0xFF);
                     if  (type == 1)
                        size += fwrite ( buffer, 1, 1, file  );
                  }
               }
               else
               {
                  int dbg = 1;
               }
            }
      // Last byte

      fseek ( file, post_of_length, SEEK_SET );

      // Set data size
      WriteInt ( size, file  );

      // Nb of pulse
      fseek ( file, post_of_length + 0x0A, SEEK_SET );
      WriteInt ( nb_sample_to_record, file  );


      fclose ( file );
      tape_changed_ = false;
   }
}
#ifndef NOZLIB
void CTape::SaveAsCSW (const char* filepath, unsigned char type, unsigned char version)
{
   FILE * file;

   if ( type != 1 && type != 2)
      return;

   if (fopen_s (&file, filepath, "wb") == 0)
   {
      unsigned char buffer[0x34] = {0};

      // Sample rate
      double sample_rate = 44100.0;

      memcpy ( buffer, "Compressed Square Wave", 22 );
      buffer[0x16] = 0x1A;

      if ( version == 1 )
      {
         buffer[0x17] = 1;
         buffer[0x18] = 1;

         fwrite ( buffer, 1, 0x19, file  );

         // Nb T-States / Sample :
         // 1 T state = 1/3500000 s.
         // 1 Sample = 1 / SampleRate s.
         // 1 sec = 3500000 T State = SampleRate samples.
         // 1 Sample = 3500000 / SampleRate T state.
         WriteShort ( (unsigned int)sample_rate, file  );

         // Compression type
         fwrite ( &type, 1, 1, file  );

         // Flag
         fwrite ( tape_array_[0].high?"\1":"\0", 1, 1, file  );

         // Reserved
         unsigned char buff[3] = {0};
         fwrite ( buff, 1, 3, file  );
      }

      else if ( version == 2 )
      {
         buffer[0x17] = 2;
         buffer[0x18] = 0;

         fwrite ( buffer, 1, 0x19, file  );

         // Nb T-States / Sample :
         // 1 T state = 1/3500000 s.
         // 1 Sample = 1 / SampleRate s.
         // 1 sec = 3500000 T State = SampleRate samples.
         // 1 Sample = 3500000 / SampleRate T state.
         WriteInt ( (unsigned int)sample_rate, file  );

         // Number of pulses
         WriteInt ( nb_inversions_+1, file  );

         // Compression type
         fwrite ( &type, 1, 1, file  );

         // Flag
         fwrite ( tape_array_[0].high?"\1":"\0", 1, 1, file  );

         // HDR size
         fwrite ( "\0", 1, 1, file  );

         // Encoding application description
         char encoding_app [16] = {0};
         strcpy ( encoding_app, "Sugarbox v0.23 ");
         fwrite ( encoding_app, 1, 16, file  );
      }

      // HDR

      // Simple RLE
      // Data
      double ratio = sample_rate / 4000000.0;
      unsigned int nb_sample_to_record = 0;

      unsigned char* data_buffer = NULL;


      if ( type == 2)
         data_buffer = new unsigned char [nb_inversions_ * 5];

      uLongf index = 0;
      //switch (type)
      {
         //case 1:
         {
            double remaining = 0.0;
            for (unsigned int i = 0; i <= nb_inversions_; )
            {
               //int nb_samples = round ( m_pTapeArray[i].Length * ratio );
               double length_of_sample = tape_array_[i].length + remaining;
               while ( i+1 <= nb_inversions_ && tape_array_[i].high == tape_array_[i+1].high)
               {
                  //nb_samples += round ( m_pTapeArray[i+1].Length * ratio );
                  length_of_sample += tape_array_[i+1].length;
                  i++;
               }
               int nb_samples = static_cast<int>(length_of_sample* sample_rate / 4000000.0);
               remaining = ((double)nb_samples) - (length_of_sample* sample_rate / 4000000.0);

               i++;
               // Add value
               if ( nb_samples != 0)
               {
                  ++nb_sample_to_record;
                  if ( nb_samples > 0xFF)
                  {
                     buffer[0] = 0;
                     buffer[1] = nb_samples&0xFF;
                     buffer[2] = (nb_samples>>8)&0xFF;
                     buffer[3] = (nb_samples>>16)&0xFF;
                     buffer[4] = (nb_samples>>24)&0xFF;
                     if  (type == 1)
                     {
                        fwrite ( buffer, 1, 5, file  );
                     }
                     else
                     {
                        memcpy ( &data_buffer[index], buffer, 5 );
                        index += 5;
                     }
                  }
                  else
                  {
                     buffer[0] = (nb_samples&0xFF);
                     if  (type == 1)
                        fwrite ( buffer, 1, 1, file  );
                     else
                        data_buffer[index++] = nb_samples&0xFF;
                  }
               }
               else
               {
                  int dbg = 1;
               }
            }
            //break;
         }
      }

      if (type == 2)
      {
         // Compress the buffer !
         uLongf size_of_zipped = compressBound ( index );
         unsigned char* zipped_buffer = new unsigned char [size_of_zipped ];
         if ( compress ( zipped_buffer, &size_of_zipped, data_buffer, index ) == Z_OK)
         {
            fwrite ( zipped_buffer, 1, size_of_zipped, file );
         }
         else
         {
            // Error : TODO
         }

         // Write corrected nbSampleToRecord
         fseek ( file,  0x1D , SEEK_SET );
         WriteInt ( nb_sample_to_record, file  );


         delete[]zipped_buffer;
         delete []data_buffer ;
      }

      fclose ( file );
      tape_changed_ = false;
   }
}
#endif

#ifndef NOFILTER
StandardFilter::StandardFilter(int frequency, int type_of_hp_filter, float fc_hp, int order_hp, int type_of_lp_filter, float fc_lp, int order_lp, float gain)
   : hp_filter_ (type_of_hp_filter, false, frequency, fc_hp, order_hp), 
     lp_filter_ (type_of_lp_filter, true, frequency, fc_lp, order_lp), gain_(gain)
{
   hp_filter_.Init();
   lp_filter_.Init();
}

StandardFilter::~StandardFilter()
{

}

void StandardFilter::Filtrer(double* array, unsigned int nb_samples)
{
   hp_filter_.Filtrer(array, nb_samples);
   lp_filter_.Filtrer(array, nb_samples);
   CTape::Gain (array, nb_samples, gain_ *18130);
   CTape::Shift (array, nb_samples, -0.5 );

}
#endif