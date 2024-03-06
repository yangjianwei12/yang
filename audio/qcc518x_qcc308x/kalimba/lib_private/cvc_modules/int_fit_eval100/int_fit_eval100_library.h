// *****************************************************************************
// Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
// *****************************************************************************

#ifndef _INT_FIT_EVAL_LIB_H
#define _INT_FIT_EVAL_LIB_H

// -----------------------------------------------------------------------------
//                               internal mic fit module data object structure
// -----------------------------------------------------------------------------
// pointer to processed internal mic
// format : integer
.CONST $int_fit_eval100.D_INT_MIC                            0;
// pointer to internal mic fit params
// format : integer
.CONST $int_fit_eval100.PARAM_PTR_FIELD                      ADDR_PER_WORD + $int_fit_eval100.D_INT_MIC;
// Pointer to cvc variant variable
// format : integer
.CONST $int_fit_eval100.PTR_VARIANT_FIELD                    ADDR_PER_WORD + $int_fit_eval100.PARAM_PTR_FIELD;
// Previous power difference
// format : q8.24
.CONST $int_fit_eval100.POW_DIFF                             ADDR_PER_WORD + $int_fit_eval100.PTR_VARIANT_FIELD;
// Previous low band power, shifted by previous block exponent
// format : q8.24
.CONST $int_fit_eval100.P_INT_MIC_L_BAND_EXP                 ADDR_PER_WORD + $int_fit_eval100.POW_DIFF;
.CONST $int_fit_eval100.P_INT_MIC_L_BAND                     ADDR_PER_WORD + $int_fit_eval100.P_INT_MIC_L_BAND_EXP;
// Previous high band power, shifted by previous block exponent
// format : q8.24
.CONST $int_fit_eval100.P_INT_MIC_H_BAND_EXP                 ADDR_PER_WORD + $int_fit_eval100.P_INT_MIC_L_BAND;
.CONST $int_fit_eval100.P_INT_MIC_H_BAND                     ADDR_PER_WORD + $int_fit_eval100.P_INT_MIC_H_BAND_EXP;
// smoothing factor
// format : q.31
.CONST $int_fit_eval100.ALPHA                                ADDR_PER_WORD + $int_fit_eval100.P_INT_MIC_H_BAND;
// Lower frequency band 1
// format : integer
.CONST $int_fit_eval100.L_BAND_INDX_1                        ADDR_PER_WORD + $int_fit_eval100.ALPHA;
// Number of bins of frequency band 1
// format : integer
.CONST $int_fit_eval100.NUM_BINS_1                           ADDR_PER_WORD + $int_fit_eval100.L_BAND_INDX_1;
// Lower frequency band 2
// format : integer
.CONST $int_fit_eval100.L_BAND_INDX_2                        ADDR_PER_WORD + $int_fit_eval100.NUM_BINS_1;
// Number of bins of frequency band 2
// format : integer
.CONST $int_fit_eval100.NUM_BINS_2                           ADDR_PER_WORD + $int_fit_eval100.L_BAND_INDX_2;
// The loose fit flag
// format : integer
.CONST $int_fit_eval100.LOOSE_FIT_FLAG_0                     ADDR_PER_WORD + $int_fit_eval100.NUM_BINS_2;
.CONST $int_fit_eval100.LOOSE_FIT_FLAG                       ADDR_PER_WORD + $int_fit_eval100.LOOSE_FIT_FLAG_0;

// Internal mic fit structure size
// format : integer
.CONST $int_fit_eval100.STRUC_SIZE                           ($int_fit_eval100.LOOSE_FIT_FLAG >> LOG2_ADDR_PER_WORD) + $hyst.STRUCT_SIZE;


// -----------------------------------------------------------------------------
//                               int_fit_eval parameters object structure
// -----------------------------------------------------------------------------
// decision making threshold in log2() domain
// format : q8.24
.CONST $int_fit_eval100.param.POW_DIFF_TH                      0;

// internal mic fit module params structure size
// format : integer
.CONST $int_fit_eval100.params.STRUC_SIZE                      1 +  ($int_fit_eval100.param.POW_DIFF_TH >> LOG2_ADDR_PER_WORD);


#endif  // _INT_FIT_EVAL_LIB_H