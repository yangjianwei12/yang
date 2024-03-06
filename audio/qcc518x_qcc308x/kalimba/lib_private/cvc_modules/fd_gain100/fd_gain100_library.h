// *****************************************************************************
// Copyright (c) 2007 - 2020 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#include "portability_macros.h"

#ifndef _FD_GAIN_LIB_H
#define _FD_GAIN_LIB_H


   // --------------------------------------------------------------------------
   //                               fd_gain data object structure
   // --------------------------------------------------------------------------   
   // Pointer to input channel (real/imag/BExp)
   // format : freqbuf
   .CONST $fd_gain100.X_FIELD                     MK1 * 0;

   // Pointer to output channel(real/imag/BExp)
   // format : freqbuf
   .CONST $fd_gain100.Z_FIELD                     MK1 * 1;

   // pointer to fd_gain param
   // format : decibel scale, q8.24 fractional
   .CONST $fd_gain100.PARAM_PTR_FIELD             MK1 * 2;

   // num bands
   // format : integer
   .CONST $fd_gain100.NUMBANDS_FIELD              MK1 * 3;
   
   // structure size
   // format : integer
   .CONST $fd_gain100.STRUC_SIZE                  4;

   // --------------------------------------------------------------------------
   //                               fd_gain private members (if needed)
   // --------------------------------------------------------------------------   




#endif  
