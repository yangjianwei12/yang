// *****************************************************************************
// Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
// *****************************************************************************
#include "portability_macros.h"

#ifndef _SELFCLEAN_LIB_H
#define _SELFCLEAN_LIB_H
    
   // *****************************************************************************
   //                               selfclean data object structure
   // *****************************************************************************
   // pointer to processed extmic
   // format : integer
   .CONST $selfclean100.Z0_PROC_FIELD                        0;    
   // pointer to unprocessed extmic
   // format : integer
   .CONST $selfclean100.Z0_UNPROC_FIELD                      ADDR_PER_WORD + $selfclean100.Z0_PROC_FIELD;
   // pointer to selfclean params
   // format : integer
   .CONST $selfclean100.PARAM_PTR_FIELD                      ADDR_PER_WORD + $selfclean100.Z0_UNPROC_FIELD;
   // Pointer to cvc variant variable
   // format : integer
   .CONST $selfclean100.PTR_VARIANT_FIELD                    ADDR_PER_WORD + $selfclean100.PARAM_PTR_FIELD;
   // FFT length field
   // format : integer
   .CONST $selfclean100.FFTLEN_FIELD                         ADDR_PER_WORD + $selfclean100.PTR_VARIANT_FIELD;
   // low freq index
   // format : integer
   .CONST $selfclean100.LOW_FREQ_IDX                         ADDR_PER_WORD + $selfclean100.FFTLEN_FIELD;
   // number of frequency bands
   // format : integer
   .CONST $selfclean100.NUMBANDS_FIELD                       ADDR_PER_WORD + $selfclean100.LOW_FREQ_IDX;
   // smoothed ratio
   // format : q8.24
   .CONST $selfclean100.SMOOTHED_RATIO                       ADDR_PER_WORD + $selfclean100.NUMBANDS_FIELD;
   // smoothing factor
   // format : q1.31
   .CONST $selfclean100.SMOOTH_FACTOR                        ADDR_PER_WORD + $selfclean100.SMOOTHED_RATIO;
   // selfclean output flag
   // format : integer
   .CONST $selfclean100.SELF_CLEAN_FLAG                      ADDR_PER_WORD + $selfclean100.SMOOTH_FACTOR;
   .CONST $selfclean100.SELF_CLEAN_FLAG_0                    ADDR_PER_WORD + $selfclean100.SELF_CLEAN_FLAG;
      // Pointer to DMSS noise floor parameter
   // format : integer
   .CONST $selfclean100.PTR_DMSS_LPN_MIC_FIELD               ADDR_PER_WORD + $selfclean100.SELF_CLEAN_FLAG_0;
   // number of bins to average for silence checking
   // format : integer
   .CONST $selfclean100.POWER_RANGE_FIELD                    ADDR_PER_WORD + $selfclean100.PTR_DMSS_LPN_MIC_FIELD;
   .CONST $selfclean100.L2P_LOW_INPUT_FIELD                  ADDR_PER_WORD + $selfclean100.POWER_RANGE_FIELD;
   // power threshold for silence checking
   // format : 8.24
   .CONST $selfclean100.MIN_POWER_THRESHOLD_FIELD            ADDR_PER_WORD + $selfclean100.L2P_LOW_INPUT_FIELD;
   // flag that indicates silence (noise floor)
   // format : bool
   .CONST $selfclean100.LOW_INPUT_FLAG                       ADDR_PER_WORD + $selfclean100.MIN_POWER_THRESHOLD_FIELD;

   // selfclean structure size
   // format : integer
#ifndef CVC_INEAR_MAOR_ROM_HARDWARE_BUILD
   .CONST $selfclean100.STRUC_SIZE                           1 +  ($selfclean100.LOW_INPUT_FLAG >> LOG2_ADDR_PER_WORD);
#else
   .CONST $selfclean100.STRUC_SIZE_HW                        1 +  ($selfclean100.LOW_INPUT_FLAG >> LOG2_ADDR_PER_WORD);
#endif

   // *****************************************************************************
   //                               selfclean parameters object structure
   // *****************************************************************************
   // Time Constant for Power Ratio Smoothing
   // format : q7.25
   .CONST $selfclean100.param.SELF_CLEAN_TC                  0;
   // Lower frequency band
   // format : integer
   .CONST $selfclean100.param.LOWER_BAND_FREQ                ADDR_PER_WORD + $selfclean100.param.SELF_CLEAN_TC;
   // Higher frequency band
   // format : integer
   .CONST $selfclean100.param.HIGHER_BAND_FREQ               ADDR_PER_WORD + $selfclean100.param.LOWER_BAND_FREQ;
   // selfclean decision making threshold
   // format : q8.24
   .CONST $selfclean100.param.THRESH                         ADDR_PER_WORD + $selfclean100.param.HIGHER_BAND_FREQ;
   // low input decision making threshold
   // format : q8.24
   .CONST $selfclean100.param.LOWINPUT_THRESH                ADDR_PER_WORD + $selfclean100.param.THRESH;

   // selfclean params structure size
   // format : integer
#ifndef CVC_INEAR_MAOR_ROM_HARDWARE_BUILD
   .CONST $selfclean100.params.STRUC_SIZE                    1 +  ($selfclean100.param.LOWINPUT_THRESH >> LOG2_ADDR_PER_WORD);
#else
   .CONST $selfclean100.params.STRUC_SIZE_HW                 1 +  ($selfclean100.param.LOWINPUT_THRESH >> LOG2_ADDR_PER_WORD);
#endif
#endif //_SELFCLEAN_LIB_H
