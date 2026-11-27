/* mkfilter -- given n, compute recurrence relation
to implement Butterworth, Bessel or Chebyshev filter of order n
A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
September 1992 */

#include "stdafx.h"

#include <stdio.h>
#include <math.h>

#include "mkfilter.h"
#include "complex.h"

#define opt_be 0x00001	/* -Be		Bessel characteristic	       */
#define opt_bu 0x00002	/* -Bu		Butterworth characteristic     */
#define opt_ch 0x00004	/* -Ch		Chebyshev characteristic       */
#define opt_re 0x00008	/* -Re		Resonator		       */
#define opt_pi 0x00010	/* -Pi		proportional-integral	       */

#define opt_lp 0x00020	/* -Lp		lowpass			       */
#define opt_hp 0x00040	/* -Hp		highpass		       */
#define opt_bp 0x00080	/* -Bp		bandpass		       */
#define opt_bs 0x00100	/* -Bs		bandstop		       */
#define opt_ap 0x00200	/* -Ap		allpass			       */

#define opt_a  0x00400	/* -a		alpha value		       */
#define opt_l  0x00800	/* -l		just list filter parameters    */
#define opt_o  0x01000	/* -o		order of filter		       */
#define opt_p  0x02000	/* -p		specified poles only	       */
#define opt_w  0x04000	/* -w		don't pre-warp		       */
#define opt_z  0x08000	/* -z		use matched z-transform	       */
#define opt_Z  0x10000	/* -Z		additional zero		       */

struct pzrep
{ Complex poles[MAXPZ], zeros[MAXPZ];
int numpoles, numzeros;
};

static pzrep splane, zplane;
static int order;
static double raw_alpha1, raw_alpha2;
static Complex dc_gain, fc_gain, hf_gain;
static uint options;
static double warped_alpha1, warped_alpha2, chebrip;
static uint polemask;
static double xcoeffs[MAXPZ+1], ycoeffs[MAXPZ+1];

static CComplex bessel_poles[] =
{ /* table produced by /usr/fisher/bessel --	N.B. only one member of each C.Conj. pair is listed */
   { -1.00000000000e+00, 0.00000000000e+00}, { -1.10160133059e+00, 6.36009824757e-01},
   { -1.32267579991e+00, 0.00000000000e+00}, { -1.04740916101e+00, 9.99264436281e-01},
   { -1.37006783055e+00, 4.10249717494e-01}, { -9.95208764350e-01, 1.25710573945e+00},
   { -1.50231627145e+00, 0.00000000000e+00}, { -1.38087732586e+00, 7.17909587627e-01},
   { -9.57676548563e-01, 1.47112432073e+00}, { -1.57149040362e+00, 3.20896374221e-01},
   { -1.38185809760e+00, 9.71471890712e-01}, { -9.30656522947e-01, 1.66186326894e+00},
   { -1.68436817927e+00, 0.00000000000e+00}, { -1.61203876622e+00, 5.89244506931e-01},
   { -1.37890321680e+00, 1.19156677780e+00}, { -9.09867780623e-01, 1.83645135304e+00},
   { -1.75740840040e+00, 2.72867575103e-01}, { -1.63693941813e+00, 8.22795625139e-01},
   { -1.37384121764e+00, 1.38835657588e+00}, { -8.92869718847e-01, 1.99832584364e+00},
   { -1.85660050123e+00, 0.00000000000e+00}, { -1.80717053496e+00, 5.12383730575e-01},
   { -1.65239648458e+00, 1.03138956698e+00}, { -1.36758830979e+00, 1.56773371224e+00},
   { -8.78399276161e-01, 2.14980052431e+00}, { -1.92761969145e+00, 2.41623471082e-01},
   { -1.84219624443e+00, 7.27257597722e-01}, { -1.66181024140e+00, 1.22110021857e+00},
   { -1.36069227838e+00, 1.73350574267e+00}, { -8.65756901707e-01, 2.29260483098e+00},
};

static void compute_s(), choosepole(Complex), prewarp(), normalize(), compute_z_blt();
static Complex blt(Complex);
static void expandpoly(), expand(Complex[], int, Complex[]), multin(Complex, int, Complex[]);

//////////////////////////////////////////////////////////
//
// Helper functions
//
//////////////////////////////////////////////////////////
void ComputeGeneric ( int typeOfFilter, int filter_order, bool lpf, double fc, double fe, double* gain , double* xv, double* yv)
{
   // Reinit static variable.
   splane = pzrep();
   zplane = pzrep();

   dc_gain.im = 0;
   dc_gain.re = 0;

   fc_gain.im = 0;
   fc_gain.re = 0;

   hf_gain.im = 0;
   hf_gain.re = 0;

   warped_alpha1 = 0;
   warped_alpha2 = 0;
   chebrip = 0;
   memset(xcoeffs, 0, sizeof(xcoeffs));
   memset(ycoeffs, 0, sizeof(ycoeffs));

   options = typeOfFilter | ((lpf)?opt_lp:opt_hp) | opt_o | opt_a;
   order = filter_order;

   polemask = ~0; /* use all poles */

   raw_alpha1 = fc/fe;
   raw_alpha2 = raw_alpha1;
   compute_s ();
   prewarp();
   normalize();
   compute_z_blt();
   expandpoly();

   // Return computed values
   // Gain
   *gain = Hypot ( (lpf)?dc_gain:hf_gain );

   // Polynomes
   for (int i = 0; i < zplane.numzeros+1; i++)
   {
      xv[i] = xcoeffs[i];
   }
   for (int i = 0; i < zplane.numpoles; i++)
   {
      yv[i] = ycoeffs[i];
   }
}

void ComputeBandPass ( int typeofilter, int filter_order, double fe, double fcb, double fch, double* gain , double* xv, double* yv)
{
   options = typeofilter | opt_bp | opt_o | opt_a;
   order = filter_order;

   polemask = ~0; /* use all poles */

   raw_alpha1 = fch/fe;
   raw_alpha2 = fcb/fe;
   compute_s ();
   prewarp();
   normalize();
   compute_z_blt();
   expandpoly();

   // Return computed values
   // Gain
   *gain = Hypot (fc_gain);

   // Polynomes
   for (int i = 0; i < zplane.numzeros+1; i++)
   {
      xv[i] = xcoeffs[i];
   }
   for (int i = 0; i < zplane.numpoles; i++)
   {
      yv[i] = ycoeffs[i];
   }
}


void ComputeBessel ( int filter_order, bool lpf, double fc, double fe, double* gain , double* xv, double* yv)
{
   ComputeGeneric (opt_be, filter_order, lpf, fc, fe, gain , xv, yv);
}

void ComputeButterworth ( int filter_order, bool lpf, double fc, double fe, double* gain , double* xv, double* yv)
{
   ComputeGeneric (opt_bu, filter_order, lpf, fc, fe, gain , xv, yv);
}

static void compute_s() /* compute S-plane poles for prototype LP filter */
{
   splane.numpoles = 0;
   if (options & opt_be)
   { /* Bessel filter */
      int p = (order*order)/4; /* ptr into table */
      if (order & 1) choosepole(bessel_poles[p++]);
      for (int i = 0; i < order/2; i++)
      {
         choosepole(bessel_poles[p]);
         choosepole(Cconj(bessel_poles[p]));
         p++;
      }
   }
   if (options & (opt_bu | opt_ch))
   {
      /* Butterworth filter */
      for (int i = 0; i < 2*order; i++)
      {
         double theta = (order & 1) ? (i*PI) / order : ((i+0.5)*PI) / order;
         choosepole(Expj(theta));
      }
   }
   if (options & opt_ch)
   {
      /* modify for Chebyshev (p. 136 DeFatta et al.) */
      if (chebrip >= 0.0)
      {
         fprintf(stderr, "mkfilter: Chebyshev ripple is %g dB; must be .lt. 0.0\n", chebrip);
         exit(1);
      }
      double rip = pow(10.0, -chebrip / 10.0);
      double eps = sqrt(rip - 1.0);
      double y = asinh(1.0 / eps) / (double) order;
      if (y <= 0.0)
      {
         fprintf(stderr, "mkfilter: bug: Chebyshev y=%g; must be .gt. 0.0\n", y);
         exit(1);
      }
      for (int i = 0; i < splane.numpoles; i++)
      {
         splane.poles[i].re *= sinh(y);
         splane.poles[i].im *= cosh(y);
      }
   }
}

static void choosepole(Complex z)
{
   if (z.re < 0.0)
   {
      if (polemask & 1) splane.poles[splane.numpoles++] = z;
      polemask >>= 1;
   }
}

static void prewarp()
{ /* for bilinear transform, perform pre-warp on alpha values */
   if (options & (opt_w | opt_z))
   {
      warped_alpha1 = raw_alpha1;
      warped_alpha2 = raw_alpha2;
   }
   else
   {
      warped_alpha1 = tan(PI * raw_alpha1) / PI;
      warped_alpha2 = tan(PI * raw_alpha2) / PI;
   }
}

static void normalize()		/* called for trad, not for -Re or -Pi */
{
   double w1 = TWOPI * warped_alpha1;
   double w2 = TWOPI * warped_alpha2;
   /* transform prototype into appropriate filter type (lp/hp/bp/bs) */
   switch (options & (opt_lp | opt_hp | opt_bp| opt_bs))
   {
   case opt_lp:
      {
         for (int i = 0; i < splane.numpoles; i++) splane.poles[i] = splane.poles[i] * w1;
         splane.numzeros = 0;
         break;
      }

   case opt_hp:
      {
         int i;
         for (i=0; i < splane.numpoles; i++) splane.poles[i] = w1 / splane.poles[i];
         for (i=0; i < splane.numpoles; i++) splane.zeros[i] = 0.0;	 /* also N zeros at (0,0) */
         splane.numzeros = splane.numpoles;
         break;
      }

   case opt_bp:
      {
         double w0 = sqrt(w1*w2), bw = w2-w1; int i;
         for (i=0; i < splane.numpoles; i++)
         {
            Complex hba = 0.5 * (splane.poles[i] * bw);
            Complex temp = Csqrt(1.0 - Sqr(w0 / hba));
            splane.poles[i] = hba * (1.0 + temp);
            splane.poles[splane.numpoles+i] = hba * (1.0 - temp);
         }
         for (i=0; i < splane.numpoles; i++) splane.zeros[i] = 0.0;	 /* also N zeros at (0,0) */
         splane.numzeros = splane.numpoles;
         splane.numpoles *= 2;
         break;
      }

   case opt_bs:
      {
         double w0 = sqrt(w1*w2), bw = w2-w1; int i;
         for (i=0; i < splane.numpoles; i++)
         { Complex hba = 0.5 * (bw / splane.poles[i]);
         Complex temp = Csqrt(1.0 - Sqr(w0 / hba));
         splane.poles[i] = hba * (1.0 + temp);
         splane.poles[splane.numpoles+i] = hba * (1.0 - temp);
         }
         for (i=0; i < splane.numpoles; i++)	   /* also 2N zeros at (0, +-w0) */
         {
            splane.zeros[i] = Complex(0.0, +w0);
            splane.zeros[splane.numpoles+i] = Complex(0.0, -w0);
         }
         splane.numpoles *= 2;
         splane.numzeros = splane.numpoles;
         break;
      }
   }
}

static void compute_z_blt() /* given S-plane poles & zeros, compute Z-plane poles & zeros, by bilinear transform */
{
   int i;
   zplane.numpoles = splane.numpoles;
   zplane.numzeros = splane.numzeros;
   for (i=0; i < zplane.numpoles; i++) zplane.poles[i] = blt(splane.poles[i]);
   for (i=0; i < zplane.numzeros; i++) zplane.zeros[i] = blt(splane.zeros[i]);
   while (zplane.numzeros < zplane.numpoles) zplane.zeros[zplane.numzeros++] = -1.0;
}

static Complex blt(Complex pz)
{
   return (2.0 + pz) / (2.0 - pz);
}

static void expandpoly() /* given Z-plane poles & zeros, compute top & bot polynomials in Z, and then recurrence relation */
{
   Complex topcoeffs[MAXPZ+1], botcoeffs[MAXPZ+1]; int i;
   expand(zplane.zeros, zplane.numzeros, topcoeffs);
   expand(zplane.poles, zplane.numpoles, botcoeffs);
   dc_gain = Evaluate(topcoeffs, zplane.numzeros, botcoeffs, zplane.numpoles, 1.0);
   double theta = TWOPI * 0.5 * (raw_alpha1 + raw_alpha2); /* "jwT" for centre freq. */
   fc_gain = Evaluate(topcoeffs, zplane.numzeros, botcoeffs, zplane.numpoles, Expj(theta));
   hf_gain = Evaluate(topcoeffs, zplane.numzeros, botcoeffs, zplane.numpoles, -1.0);
   for (i = 0; i <= zplane.numzeros; i++) xcoeffs[i] = +(topcoeffs[i].re / botcoeffs[zplane.numpoles].re);
   for (i = 0; i <= zplane.numpoles; i++) ycoeffs[i] = -(botcoeffs[i].re / botcoeffs[zplane.numpoles].re);
}

static void expand(Complex pz[], int npz, Complex coeffs[])
{ /* compute product of poles or zeros as a polynomial of z */
   int i;
   coeffs[0] = 1.0;
   for (i=0; i < npz; i++) coeffs[i+1] = 0.0;
   for (i=0; i < npz; i++) multin(pz[i], npz, coeffs);
   /* check computed coeffs of z^k are all real */
   for (i=0; i < npz+1; i++)
   {
      if (fabs(coeffs[i].im) > EPS)
      {
         //fprintf(stderr, "mkfilter: coeff of z^%d is not real; poles/zeros are not complex conjugates\n", i);
         //exit(1);
      }
   }
}

static void multin(Complex w, int npz, Complex coeffs[])
{ /* multiply factor (z-w) into coeffs */
   Complex nw = -w;
   for (int i = npz; i >= 1; i--) coeffs[i] = (nw * coeffs[i]) + coeffs[i-1];
   coeffs[0] = nw * coeffs[0];
}

void Filtrer(double* pArray, unsigned int nbSamples, int typeOfFilter, bool lpf, double fe, double fc, int order)
{
   double *xv = new double[order + 1];
   double *yv = new double[order + 1];
   double gain;
   double * coefx = new double[order + 1];
   double * coefy = new double[order + 1];

   memset(xv, 0, sizeof(double)*(order + 1));
   memset(yv, 0, sizeof(double)*(order + 1));

   memset(coefx, 0, sizeof(double)*(order + 1));
   memset(coefy, 0, sizeof(double)*(order + 1));


   // Compute gain and coeff.
   switch (typeOfFilter)
   {
   case 01:
      ComputeBessel(order, lpf, fc, fe, &gain, coefx, coefy);
      break;
   case 02:
      ComputeButterworth(order, lpf, fc, fe, &gain, coefx, coefy);
      break;
   }

   // Lets apply theses values.
   for (unsigned int i = 0; i < nbSamples; i++)
   {
      // Shift values
      for (int j = 0; j < order; j++)
      {
         xv[j] = xv[j + 1];
         yv[j] = yv[j + 1];
      }

      xv[order] = pArray[i] / gain;
      // Compute output/last yv value
      double yvout = 0;
      for (int j = 0; j < order; j++)
      {
         yvout += xv[j] * coefx[j];
         yvout += yv[j] * coefy[j];
      }
      yvout += xv[order];
      yv[order] = yvout;
      pArray[i] = yvout;
   }
   delete[]xv;
   delete[]yv;
   delete[]coefx;
   delete[]coefy;
}
