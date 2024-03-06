// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

// Header file for C stubs of "math" library
// Comments show the syntax to call the routine

#if !defined(FFT_TWIDDLE_ALLOC_C_STUBS_H)
#define FFT_TWIDDLE_ALLOC_C_STUBS_H


/* PUBLIC FUNCTION PROTOTYPES ***********************************************/




//    C interface for $math.fft_twiddle.alloc
//
// DESCRIPTION:
//    Allocates and populates memory for fft twiddle factors. If the factors are
//    already allocated just register interest.
//
// INPUTS:
//    - r0 = num fft points required
//
// OUTPUTS:
//    - r0 = result (1 - SUCCESS, 0 - FAIL)
//
// TRASHED REGISTERS:
//    Follows C Guildelines

bool math_fft_twiddle_alloc(int fft_num_pts);


//    C interface for $math.fft_twiddle.release
//
// DESCRIPTION:
//    Unregisters interest in fft twiddle factors then frees them if no-one else
//    is interested
//
// INPUTS:
//    - r0 = num fft points in use
//
// OUTPUTS:
//    - r0 = result (1 - SUCCESS, 0 - FAIL)
//
// TRASHED REGISTERS:
//    Follows C Guidelines
bool math_fft_twiddle_release(int fft_num_pts);

//    C interface for $math.fft_twiddle.free
//
// DESCRIPTION:
//    Free the current twiddle factors memory (real and imag)
//    Note this routine is intended as internal utility.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    Follows C Guidelines
bool math_fft_twiddle_free(int fft_num_pts);


#endif
