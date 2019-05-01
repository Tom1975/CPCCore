
struct CComplex
  { double re, im;
  };

struct Complex
  { double re, im;
    Complex(double r, double i = 0.0) { re = r; im = i; }
    Complex() { }					/* uninitialized Complex */
    Complex(CComplex z) { re = z.re; im = z.im; }	/* init from denotation */
  };

extern Complex Csqrt(Complex), Cexp(Complex), Expj(double);	    /* from Complex.C */
extern Complex Evaluate(Complex[], int, Complex[], int, Complex);   /* from Complex.C */

inline double Hypot(Complex z) { return ::hypot(z.im, z.re); }
inline double Atan2(Complex z) { return ::atan2(z.im, z.re); }

inline Complex Cconj(Complex z)
  { z.im = -z.im;
    return z;
  }

inline Complex operator * (double a, Complex z)
  { z.re *= a; z.im *= a;
    return z;
  }

inline Complex operator / (Complex z, double a)
  { z.re /= a; z.im /= a;
    return z;
  }

inline void operator /= (Complex &z, double a)
  { z = z / a;
  }

extern Complex operator * (Complex, Complex);
extern Complex operator / (Complex, Complex);

inline Complex operator + (Complex z1, Complex z2)
  { z1.re += z2.re;
    z1.im += z2.im;
    return z1;
  }

inline Complex operator - (Complex z1, Complex z2)
  { z1.re -= z2.re;
    z1.im -= z2.im;
    return z1;
  }

inline Complex operator - (Complex z)
  { return 0.0 - z;
  }

inline bool operator == (Complex z1, Complex z2)
  { return (z1.re == z2.re) && (z1.im == z2.im);
  }

inline Complex Sqr(Complex z)
  { return z*z;
  }
