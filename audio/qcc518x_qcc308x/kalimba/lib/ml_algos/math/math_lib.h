/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

/**
 * Header file for C stubs of "ml_math" library
 * Comments show the syntax to call the routine
 */
#if !defined(EAI_MATH_LIBRARY_H)
#define EAI_MATH_LIBRARY_H

#include <stdfix.h>
#include <math_library_c_stubs.h>

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * bit-reverse an array
 * notes: out must be a circular buffer, size must be power of 2
 */
void ml_bitreverse_array( int *in, int *out, int size);

sat fract ml_sqrt(accum);

void ml_linspace(int * out, int min, int max,int count);
/**
 * Takes accum value to calculate log for
 * Returns signed sat fract value in format Q8.24
 */
sat fract ml_log2(accum x);
accum ml_log10(accum value);
accum ml_ln(accum value);
void ml_exp(sat fract * out, const sat fract * in, int len);
void ml_exp10(sat fract * out, const sat fract * in, int len);
sat fract ml_sin(sat fract in);

#define EAI_FFT_1024_POINT
#ifdef EAI_FFT_1024_POINT
#define EAI_FFT_SIZE (1024)
#define EAI_TWIDDLE_SIZE (512)
#else
#error FFT size not defined
#endif
/**
 * @brief ml_twiddle_load Assigns $fft.twiddle_real_address and $fft.twiddle_imag_address
 * @param num_points FFT points
 * @param real_ptr Pointer to real twiddle factors
 * @param imag_ptr Pointer to imaginary twiddle factors
 */
void ml_twiddle_load(int num_points, int *real_ptr, int *imag_ptr);
/**
 * @brief ml_twiddle_init Loads the twiddle values from ROM (slow) to RAM (faster)
 * @param num_points
 */
void ml_twiddle_init(int num_points);
/**
 * @brief ml_twiddle_release Frees the memory in RAM for twiddle factors
 */
void ml_twiddle_release(void);
/**
 * @brief ml_fft Calls the asm fft library
 * @param x
 */
void ml_fft(tFFTStruct *x);
/**
 * @brief ml_fft_scaled Calls the asm scaled fft library
 * @param x The fft data structure.
 */
void ml_fft_scaled(tFFTStruct* x);

#endif
