/* mkfilter -- given n, compute recurrence relation
   to implement Butterworth, Bessel or Chebyshev filter of order n
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   September 1992 */

/* Header file */
#ifndef _MKFILTER_
#define _MKFILTER_

#define global
#undef	PI
#define PI	    3.14159265358979323846  /* Microsoft C++ does not define M_PI ! */
#define TWOPI	    (2.0 * PI)
#define EPS	    1e-10
#define MAXORDER    10
#define MAXPZ	    512	    /* .ge. 2*MAXORDER, to allow for doubling of poles in BP filter;
			       high values needed for FIR filters */

typedef unsigned int uint;

void ComputeBessel ( int filter_order, bool lpf, double fc, double fe, double* gain , double* xv, double* yv);
void ComputeButterworth ( int filter_order, bool lpf, double fc, double fe, double* gain , double* xv, double* yv);
void ComputeBandPass ( int typeofilter, int filter_order, double fe, double fcb, double fch, double* gain , double* xv, double* yv);
void Filtrer(double* pArray, unsigned int nbSamples, int typeOfFilter, bool lpf, double fe, double fc, int order);


inline double sqr(double x)	    { return x*x;			       }

inline double asinh(double x)
  { /* Microsoft C++ does not define */
    return log(x + sqrt(1.0 + sqr(x)));
  }

#endif //_MKFILTER_