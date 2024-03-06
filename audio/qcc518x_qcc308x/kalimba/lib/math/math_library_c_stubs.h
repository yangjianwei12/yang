// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

// Header file for C stubs of "math" library
// Comments show the syntax to call the routine

#if !defined(MATH_LIBRARY_C_STUBS_H)
#define MATH_LIBRARY_C_STUBS_H

#include <stdfix.h>

/* PUBLIC TYPES DEFINITIONS *************************************************/
typedef struct tFFTStuctTag
{
    unsigned int numPoints;
    int *real;
    int *imag;
} tFFTStruct;



/* PUBLIC FUNCTION PROTOTYPES ***********************************************/


// Library subroutine to evaluate the square root of a number, specifying i
// output q formats
//    input value between 0 and 2^23-1 (ie. 0 and 1.0 fractional)
//    q_in value between 0 and 32 
//    q_out value between 0 and 32
//    result (accurate to 16bits, RMS error is 5bits))
int qsqrt( int x, int q_in, int q_out);

// Library subroutine to evaluate the square root of a double precision number
//    input value between 0 and 2^63-1 (ie. 0 and 1.0 fractional)
int sqrt_dp( accum x);

// Optimised FFT subroutine with a simple interface
//    input pointer to fft structure
void fft( tFFTStruct *x);

// bit-reverse an array
//     notes: out must be a circular buffer, size must be power of 2
void bitreverse_array( int *in, int *out, int size);

// Calculate y = log2(x) in taylor series method
//    input double precision, Q1.63 positive number
//    result Q8.24
int log2( long long x);

// Calculate y = log2(x) in taylor series method
//    input accum, Q9.63 positive number
//    result Q8.24
int log2_ext( accum x);

// Calculate power-of-2, y = pow2( x) in taylor series method, fixed point
//    input Q8.24 negative number
//    result Q1.31
int qpow2( int x);

// Calculate power-of-2, y = pow2(x) in table method, fixed point
//    input x, Q8.24 
//    input q, output q format (0 <= q <= 31) 
//    result Q1.31
int math_pow2_q(int x, int q);




#endif
