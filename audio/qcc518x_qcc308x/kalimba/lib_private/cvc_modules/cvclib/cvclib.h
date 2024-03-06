// *****************************************************************************
// Copyright Qualcomm Technologies International, Ltd. 2015.
//
// %%version
//
// *****************************************************************************

#ifndef _CVCLIB_LIB_H
#define _CVCLIB_LIB_H

#include "portability_macros.h"
.CONST $cvclib.table.PBP_MEL2LIN_PTR      (0);
.CONST $cvclib.table.PBP_BINMAP           (1) * MK1;
.CONST $cvclib.table.PBP_LIN2MEL          (1 + 9) * MK1;
// -----------------------------------------------------------------------------
// HISTORY:
//    12-12-06 Thu wms 1.0.0: Initial Version
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// FP: BC7 uses Frame Pointer
// -----------------------------------------------------------------------------

// WARNING: do not access FP for 2 cycles
#define $cvclib.macro.set_FP(reg) \
   push reg;   \
   pop FP


// -----------------------------------------------------------------------------
// KP: Kalimba Module Data Object Pointer
// -----------------------------------------------------------------------------
#if defined(KP)
   #undef KP
#endif
#define KP     r9
#define $cvclib.macro.set_KP(reg) \
   KP = reg

// -----------------------------------------------------------------------------
// CHANNEL STRUCTURE
// WARNING!! DO NOT EDIT WITHOUT EDITING cvclib_c.h
// -----------------------------------------------------------------------------
// @DATA_OBJECT         CVCLIB_CHANNEL_OBJ

// @DOC_FIELD_TEXT Pointer to channel FFT real part
// @DOC_FIELD_FORMAT Pointer
.CONST $cvclib.channel.REAL                  MK1 * 0;

// @DOC_FIELD_TEXT Pointer to channel FFT imaginary part
// @DOC_FIELD_FORMAT Pointer
.CONST $cvclib.channel.IMAG                  MK1 * 1;

// @DOC_FIELD_TEXT Pointer to channel FFT Block Exponent Variable
// @DOC_FIELD_FORMAT Pointer
.CONST $cvclib.channel.BEXP                  MK1 * 2;

.CONST $cvclib.channel.STRUCT_SIZE                 3;

// @END DATA_OBJECT     CVCLIB_CHANNEL_OBJ


// -----------------------------------------------------------------------------
// CONSTANTS
// -----------------------------------------------------------------------------
#define $cvclib.Q1.FBIT                      (DAWTH-1)
#define $cvclib.Q8.FBIT                      (DAWTH-8)
#define $cvclib.Q16.FBIT                     (DAWTH-16)

#define $cvclib.Q8                           (1.0/(1<<7))
#define $cvclib.Q16                          (1.0/(1<<15))

#define $cvclib.Q8_N                         $cvclib.Q8

.CONST $cvclib.LDELTAA                      ( - DAWTH     );
.CONST $cvclib.LDELTAP                      ( - DAWTH * 2 );

.CONST $cvclib.LDELTAA_Q8_16                ( $cvclib.LDELTAA << (DAWTH - 8) );
.CONST $cvclib.LDELTAP_Q8_16                ( $cvclib.LDELTAP << (DAWTH - 8) );


.CONST $cvclib.VARIANT_NB           0;
.CONST $cvclib.VARIANT_WB           1;
.CONST $cvclib.VARIANT_UWB          2;
.CONST $cvclib.VARIANT_SWB          3;
.CONST $cvclib.VARIANT_FB           4;

#define $cvclib.Q_TC                (1.0/(1<<6))
#define $cvclib.L2E                 1.44269504088896340

#define $cvclib.sys_config.VARIANT        0 * MK1
#define $cvclib.sys_config.USE_CASE       1 * MK1
#define $cvclib.sys_config.NUM_MICS       2 * MK1
#define $cvclib.sys_config.MIC_CONFIG     3 * MK1
#define $cvclib.sys_config.LICENSE_KEY    4 * MK1

#define $cvclib.use_case.HEADSET          0
#define $cvclib.use_case.SPEAKER          1
#define $cvclib.use_case.AUTO             2

#define $cvclib.mic_array.DEFAULT         0
#define $cvclib.mic_array.ENDFIRE         1
#define $cvclib.mic_array.BROADSIDE       2
#define $cvclib.mic_array.SOUNDBAR        3
#define $cvclib.mic_array.INEAR           8
#define $cvclib.mic_array.AI              16

#endif // _CVCLIB_LIB_H
