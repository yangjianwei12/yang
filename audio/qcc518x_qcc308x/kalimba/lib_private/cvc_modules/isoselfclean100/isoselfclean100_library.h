// *****************************************************************************
// Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
// *****************************************************************************
#include "portability_macros.h"

#ifndef _ISOSELFCLEAN_LIB_H
#define _ISOSELFCLEAN_LIB_H
    
   // *****************************************************************************
   //                               isoselfclean data object structure
   // *****************************************************************************
   // pointer to isoselfclean params
   // format : integer
   .CONST $isoselfclean100.PARAM_PTR_FIELD                      0;
   // Pointer to intmic VAD
   // format : integer
   .CONST $isoselfclean100.INTVAD_PTR_FIELD                     ADDR_PER_WORD + $isoselfclean100.PARAM_PTR_FIELD;
   // Pointer to noise power in log2(8.24)
   // format : integer
   .CONST $isoselfclean100.NOISE_POWER_LOG2_PTR                 ADDR_PER_WORD + $isoselfclean100.INTVAD_PTR_FIELD;
   // Isoselfclean flag
   // format : integer
   .CONST $isoselfclean100.ISO_SELF_CLEAN_FLAG                  ADDR_PER_WORD + $isoselfclean100.NOISE_POWER_LOG2_PTR;
   // isoselfclean structure size
   // format : integer
   .CONST $isoselfclean100.STRUC_SIZE                           1 +  ($isoselfclean100.ISO_SELF_CLEAN_FLAG  >> LOG2_ADDR_PER_WORD);
   
   
   // *****************************************************************************
   //                               isoselfclean parameters object structure
   // *****************************************************************************
   // Low Noise Threshold
   // format : q8.24
   .CONST $isoselfclean100.param.LOWNOISE_THRESH                0;
   
#endif  
