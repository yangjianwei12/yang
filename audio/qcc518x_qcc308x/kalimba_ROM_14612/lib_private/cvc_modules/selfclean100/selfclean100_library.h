// *****************************************************************************
// %%fullcopyright(2008)        http://www.csr.com
// %%version
//
// $Change: 3572284 $  $DateTime: 2020/12/11 19:12:21 $
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
   // selfclean structure size
   // format : integer
   .CONST $selfclean100.STRUC_SIZE                           1 +  ($selfclean100.SELF_CLEAN_FLAG >> LOG2_ADDR_PER_WORD);
   
   
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
   // decision making threshold
   // format : q8.24
   .CONST $selfclean100.param.THRESH                         ADDR_PER_WORD + $selfclean100.param.HIGHER_BAND_FREQ;
   // selfclean params structure size
   // format : integer
   .CONST $selfclean100.params.STRUC_SIZE                    1 +  ($selfclean100.param.THRESH >> LOG2_ADDR_PER_WORD);
   
   
   
   
#endif  
