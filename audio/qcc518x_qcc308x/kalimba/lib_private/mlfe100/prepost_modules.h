// *****************************************************************************
// Copyright (c) 2023 Qualcomm Technologies International, Ltd.
//
// *****************************************************************************
#ifndef __PREPOST_MODULES__
#define __PREPOST_MODULES__

#include "mlfe100_internal.h"
#include "portability_macros.h"
#include "stack.h"

// -----------------------------------------------------------------------------
// MU LAW compand constants
// -----------------------------------------------------------------------------
.CONST $mulaw_compand100.inv_ln_oneplus_mu           0.18033688011112;                 // q.31
.CONST $mulaw_compand100.ln_2                        0.693147180559945;                // q.31
.CONST $mulaw_compand100.MIN_INT                     1;      
.CONST $mulaw_compand100.MU                          0x7F800000;                       // q.23
.CONST $mulaw_compand100.ONE_in_Q23                  0x800000;

// -----------------------------------------------------------------------------
// MU LAW expand constants
// -----------------------------------------------------------------------------
.CONST $mulaw_expand100.EIGHT_in_Q24                 0x8000000;                        // q.24
.CONST $mulaw_expand100.MIN_INT                      1;      
.CONST $mulaw_expand100.256_over_255                 0x40404040;                       // q2.30
.CONST $mulaw_expand100.one_over_255                 0x404040;                         // q2.30

// -----------------------------------------------------------------------------
// outputmix100 constants
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// outputmix100 constants
// -----------------------------------------------------------------------------

// enter scale factors below according to model_info file:
.CONST  $outputmix100.SCALE_INPUT1                   8;//7;
.CONST  $outputmix100.SCALE_INPUT2                   8;//7;
.CONST  $outputmix100.SCALE_OUTPUT1                  6;//5;
.CONST  $outputmix100.SCALE_OUTPUT2                  0;//1;

// Convert to Q.Y
.CONST  $outputmix100.Q_X2                          31 - $outputmix100.SCALE_INPUT2;
.CONST  $outputmix100.Q_SGN                         31 - $outputmix100.SCALE_OUTPUT1;
.CONST  $outputmix100.Q_G                           31 - $outputmix100.SCALE_OUTPUT2;
// Compute shift and Q factors
.CONST  $outputmix100.Q_MASK                        $outputmix100.Q_G + $outputmix100.Q_X2 - 31;     // Q_MASK = Q_X + Q_G - 31
.CONST  $outputmix100.Q_ONE                         $outputmix100.Q_MASK + 31 - $outputmix100.Q_SGN; // Q_MASK = Q_ONE + Q_SGN - 31 : Q_ONE = Q_MASK + 31 - Q_SGN
.CONST  $outputmix100.X_SHIFT                       $outputmix100.Q_MASK - $outputmix100.Q_X2;   // Q_X << SHIFT = Q_MASK


// **** TBD *** Eliminate this and replace with function arguments to mulaw_compand function (see outputmix100)
// -----------------------------------------------------------------------------
//                               mu law compand structure
// -----------------------------------------------------------------------------
// pointer to input data
// format : integer
.CONST $mulaw_compand100.INPUT_PTR_FIELD               0;
// pointer to output data
// format : integer
.CONST $mulaw_compand100.OUTPUT_PTR_FIELD              ADDR_PER_WORD + $mulaw_compand100.INPUT_PTR_FIELD;
// pointer to mu_law scratch
// format : integer
.CONST $mulaw_compand100.MULAW_SCRPTR_FIELD            ADDR_PER_WORD + $mulaw_compand100.OUTPUT_PTR_FIELD;
// pointer to mu_law scratch internal1
// format : integer
.CONST $mulaw_compand100.MULAW_SCRINT1_FIELD           ADDR_PER_WORD + $mulaw_compand100.MULAW_SCRPTR_FIELD;
// pointer to mu_law scratch internal2
// format : integer
.CONST $mulaw_compand100.MULAW_SCRINT2_FIELD           ADDR_PER_WORD + $mulaw_compand100.MULAW_SCRINT1_FIELD;
// number of bins
// format : integer
.CONST $mulaw_compand100.NUM_BINS_FIELD                ADDR_PER_WORD + $mulaw_compand100.MULAW_SCRINT2_FIELD;     
// structure size
// format : integer
.CONST $mulaw_compand100.STRUC_SIZE                    1 +  ($mulaw_compand100.NUM_BINS_FIELD >> LOG2_ADDR_PER_WORD);


// **** TBD *** Eliminate this and replace with function arguments to mulaw_expand function (see outputmix100)
// -----------------------------------------------------------------------------
//                               mu law expand structure
// -----------------------------------------------------------------------------
// pointer to input data
// format : integer
.CONST $mulaw_expand100.INPUT_PTR_FIELD               0;
// pointer to output data
// format : integer
.CONST $mulaw_expand100.OUTPUT_PTR_FIELD              ADDR_PER_WORD + $mulaw_expand100.INPUT_PTR_FIELD;
// pointer to mu_law scratch
// format : integer
.CONST $mulaw_expand100.MULAW_SCRPTR_FIELD            ADDR_PER_WORD + $mulaw_expand100.OUTPUT_PTR_FIELD;
// pointer to mu_law scratch internal1
// format : integer
.CONST $mulaw_expand100.MULAW_SCRINT1_FIELD           ADDR_PER_WORD + $mulaw_expand100.MULAW_SCRPTR_FIELD;
// pointer to mu_law scratch internal2
// format : integer
.CONST $mulaw_expand100.MULAW_SCRINT2_FIELD           ADDR_PER_WORD + $mulaw_expand100.MULAW_SCRINT1_FIELD;
// number of bins
// format : integer
.CONST $mulaw_expand100.NUM_BINS_FIELD                ADDR_PER_WORD + $mulaw_expand100.MULAW_SCRINT2_FIELD;     
// structure size
// format : integer
.CONST $mulaw_expand100.STRUC_SIZE                    1 +  ($mulaw_expand100.NUM_BINS_FIELD >> LOG2_ADDR_PER_WORD);





#endif // __PREPOST_MODULES__
