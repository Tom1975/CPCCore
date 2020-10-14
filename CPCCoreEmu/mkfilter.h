/* mkfilter -- given n, compute recurrence relation
   to implement Butterworth, Bessel or Chebyshev filter of order n
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   September 1992 */

/* Header file */
#ifndef _MKFILTER_
#define _MKFILTER_

#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define VERSION_MKFILTER	    "4.6"
#undef	PI
#define PI	    3.14159265358979323846  /* Microsoft C++ does not define M_PI ! */
#define TWOPI	    (2.0 * PI)
#define EPS	    1e-10
#define MAXORDER    10
#define MAXPZ	    512	    /* .ge. 2*MAXORDER, to allow for doubling of poles in BP filter;
			       high values needed for FIR filters */
#define MAXSTRING   256

typedef void (*proc)();
typedef unsigned int uint;

extern "C"
  { double atof(const char*);
    //int atoi(char*);
    //void exit(int);
  };

void ComputeBessel ( int filter_order, bool lpf, double fc, double fe, double* gain , double* xv, double* yv);
void ComputeButterworth ( int filter_order, bool lpf, double fc, double fe, double* gain , double* xv, double* yv);
void ComputeBandPass ( int typeofilter, int filter_order, double fe, double fcb, double fch, double* gain , double* xv, double* yv);
void Filtrer(double* pArray, unsigned int nbSamples, int typeOfFilter, bool lpf, double fe, double fc, int order);

extern char *progname;
extern void readdata(char*, double&, int&, double*, int&, double*);

inline double sqr(double x)	    { return x*x;			       }
inline bool seq(char *s1, char *s2) { return strcmp(s1,s2) == 0;	       }
inline bool onebit(uint m)	    { return (m != 0) && ((m & m-1) == 0);     }

inline double asinh(double x)
  { /* Microsoft C++ does not define */
    return log(x + sqrt(1.0 + sqr(x)));
  }

inline double fix(double x)
  { /* nearest integer */
    return (x >= 0.0) ? floor(0.5+x) : -floor(0.5-x);
  }

#endif //_MKFILTER_