/* mkfilter -- given n, compute recurrence relation
   to implement Butterworth, Bessel or Chebyshev filter of order n
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   September 1992 */

/* Routines for Complex arithmetic */

#include "stdafx.h"
#include <math.h>

#include "mkfilter.h"
#include "Complex.h"

static Complex Eval(Complex[], int, Complex);
static double Xsqrt(double);


global Complex Evaluate(Complex topco[], int nz, Complex botco[], int np, Complex z)
  { /* evaluate response, substituting for z */
    return Eval(topco, nz, z) / Eval(botco, np, z);
  }

static Complex Eval(Complex coeffs[], int npz, Complex z)
  { /* evaluate polynomial in z, substituting for z */
    Complex sum = Complex(0.0);
    for (int i = npz; i >= 0; i--) sum = (sum * z) + coeffs[i];
    return sum;
  }

global Complex Csqrt(Complex x)
  { double r = Hypot(x);
    Complex z = Complex(Xsqrt(0.5 * (r + x.re)),
			Xsqrt(0.5 * (r - x.re)));
    if (x.im < 0.0) z.im = -z.im;
    return z;
  }

static double Xsqrt(double x)
  { /* because of deficiencies in hypot on Sparc, it's possible for arg of Xsqrt to be small and -ve,
       which logically it can't be (since r >= |x.re|).	 Take it as 0. */
    return (x >= 0.0) ? sqrt(x) : 0.0;
  }

global Complex Cexp(Complex z)
  { return exp(z.re) * Expj(z.im);
  }

global Complex Expj(double theta)
  { return Complex(cos(theta), sin(theta));
  }

global Complex operator * (Complex z1, Complex z2)
  { return Complex(z1.re*z2.re - z1.im*z2.im,
		   z1.re*z2.im + z1.im*z2.re);
  }

global Complex operator / (Complex z1, Complex z2)
  { double mag = (z2.re * z2.re) + (z2.im * z2.im);
    return Complex (((z1.re * z2.re) + (z1.im * z2.im)) / mag,
		    ((z1.im * z2.re) - (z1.re * z2.im)) / mag);
  }
