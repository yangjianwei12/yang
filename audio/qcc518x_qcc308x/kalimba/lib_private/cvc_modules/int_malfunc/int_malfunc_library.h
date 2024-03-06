// *****************************************************************************
// Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
// *****************************************************************************

#ifndef _INT_MALFUNC_LIB_H
#define _INT_MALFUNC_LIB_H

// -----------------------------------------------------------------------------
//                               int_malfunc data object structure
// -----------------------------------------------------------------------------
// pointer to primary extmic
// format : integer
.CONST $int_malfunc.D0_FIELD                             0;    
// pointer to intmic
// format : integer
.CONST $int_malfunc.D_INT_FIELD                          ADDR_PER_WORD + $int_malfunc.D0_FIELD;
// pointer to int_malfunc params
// format : integer
.CONST $int_malfunc.PARAM_PTR_FIELD                      ADDR_PER_WORD + $int_malfunc.D_INT_FIELD;
// Pointer to cvc variant variable
// format : integer
.CONST $int_malfunc.PTR_VARIANT_FIELD                    ADDR_PER_WORD + $int_malfunc.PARAM_PTR_FIELD;

.CONST $int_malfunc.SCRATCH_DM1_FIELD                    ADDR_PER_WORD + $int_malfunc.PTR_VARIANT_FIELD;

.CONST $int_malfunc.SCRATCH_DM2_FIELD                    ADDR_PER_WORD + $int_malfunc.SCRATCH_DM1_FIELD;

// Internal fields
.CONST $int_malfunc.D_INT_MAP_REAL                       ADDR_PER_WORD + $int_malfunc.SCRATCH_DM2_FIELD;
.CONST $int_malfunc.D_INT_MAP_IMAG                       ADDR_PER_WORD + $int_malfunc.D_INT_MAP_REAL;
// low freq index
// format : integer
.CONST $int_malfunc.LOW_FREQ_IDX                         ADDR_PER_WORD + $int_malfunc.D_INT_MAP_IMAG;
// number of frequency bands
// format : integer
.CONST $int_malfunc.NUMBANDS_FIELD                       ADDR_PER_WORD + $int_malfunc.LOW_FREQ_IDX;

// smoothing factor
// format : q1.31
.CONST $int_malfunc.SMOOTH_FACTOR                        ADDR_PER_WORD + $int_malfunc.NUMBANDS_FIELD;
.CONST $int_malfunc.P_PRIM_MIC                           ADDR_PER_WORD + $int_malfunc.SMOOTH_FACTOR;
.CONST $int_malfunc.P_PRIM_MIC_EXP                       ADDR_PER_WORD + $int_malfunc.P_PRIM_MIC;

.CONST $int_malfunc.P_PRIM_MIC1                          ADDR_PER_WORD + $int_malfunc.P_PRIM_MIC_EXP;
.CONST $int_malfunc.P_PRIM_MIC1_EXP                      ADDR_PER_WORD + $int_malfunc.P_PRIM_MIC1;

.CONST $int_malfunc.POW_DIFF                             ADDR_PER_WORD + $int_malfunc.P_PRIM_MIC1_EXP;

// int_malfunc output flag
// format : integer
.CONST $int_malfunc.INT_MIC_FAIL_INST                    ADDR_PER_WORD + $int_malfunc.POW_DIFF;
.CONST $int_malfunc.INT_MALFUNC_FLAG                     ADDR_PER_WORD + $int_malfunc.INT_MIC_FAIL_INST;

// int_malfunc structure size
// format : integer
.CONST $int_malfunc.STRUC_SIZE                           ($int_malfunc.INT_MALFUNC_FLAG >> LOG2_ADDR_PER_WORD)  + $hyst.STRUCT_SIZE;


// -----------------------------------------------------------------------------
//                               int_malfunc parameters object structure
// -----------------------------------------------------------------------------

// Decision Threshold power in Q8.24 in log2
.CONST $int_malfunc.param.POWER_DIFF_THRESH          0;

.CONST $int_malfunc.params.STRUC_SIZE                1 +  ($int_malfunc.param.POWER_DIFF_THRESH >> LOG2_ADDR_PER_WORD);

#endif  // _INT_MALFUNC_LIB_H