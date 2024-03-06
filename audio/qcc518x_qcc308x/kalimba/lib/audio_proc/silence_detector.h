// *****************************************************************************
// Copyright (c) 2014 - 2021 Qualcomm Technologies International, Ltd.
// %% version
//
// $Change: 3688507 $  $DateTime: 2021/06/22 17:22:46 $
// *****************************************************************************

#include "portability_macros.h"

#ifndef _SILENCE_DETECTOR_LIB_H
#define _SILENCE_DETECTOR_LIB_H
   
   // *****************************************************************************
   //                               silence_detector data object structure
   // *****************************************************************************

   // pointer to Input buffer
   // format : integer
   .CONST $M.SILENCE_DETECTOR.INPUT_BUFFER_PTR              0;

   // pointer to Parameters Object
   // format : integer
   .CONST $M.SILENCE_DETECTOR.PARAMS_OBJ_PTR                ADDR_PER_WORD + $M.SILENCE_DETECTOR.INPUT_BUFFER_PTR;

   // Alpha based on time_constant
   // format : Q1.N
   .CONST $M.SILENCE_DETECTOR.ALPHA                         ADDR_PER_WORD + $M.SILENCE_DETECTOR.PARAMS_OBJ_PTR;

   // Smoothed input power as measured in the linear domain
   // format : Q8.N
   .CONST $M.SILENCE_DETECTOR.SMOOTHED_POWER_LINEAR         ADDR_PER_WORD + $M.SILENCE_DETECTOR.ALPHA;

   // Computed power in the log2 domain
   // format : Q8.N
   .CONST $M.SILENCE_DETECTOR.COMPUTED_POWER_LOG2           ADDR_PER_WORD + $M.SILENCE_DETECTOR.SMOOTHED_POWER_LINEAR;

   // Threshold power minus hysteresis band in the log2 domain
   // format : Q8.N
   .CONST $M.SILENCE_DETECTOR.LOWER_THRESHOLD_POWER         ADDR_PER_WORD + $M.SILENCE_DETECTOR.COMPUTED_POWER_LOG2;

   // Threshold power plus hysteresis band in the log2 domain
   // format : Q8.N
   .CONST $M.SILENCE_DETECTOR.UPPER_THRESHOLD_POWER         ADDR_PER_WORD + $M.SILENCE_DETECTOR.LOWER_THRESHOLD_POWER;

   // (1/frame_size)
   // format : Q1.N
   .CONST $M.SILENCE_DETECTOR.ONE_OVER_FRAME_SIZE           ADDR_PER_WORD + $M.SILENCE_DETECTOR.UPPER_THRESHOLD_POWER;

   // Flag that indicates the event detected (silence 1, audio - 0)
   // format : integer
   .CONST $M.SILENCE_DETECTOR.SILENCE_DETECTION_EVENT        ADDR_PER_WORD + $M.SILENCE_DETECTOR.ONE_OVER_FRAME_SIZE;

   // silence_detector data object structure size
   // format : integer
   .CONST $M.SILENCE_DETECTOR.STRUC_SIZE                    1 +  ($M.SILENCE_DETECTOR.SILENCE_DETECTION_EVENT >> LOG2_ADDR_PER_WORD);


   // *****************************************************************************
   //                               silence_detector data object structure
   // *****************************************************************************

   // time_constant used for smoothing
   // format : Q(7.N)
   .CONST $M.SILENCE_DETECTOR.PARAMS.TIME_CONSTANT                    0;
   
   // Threshold power used to determine silence
   // format : Q8.N
   .CONST $M.SILENCE_DETECTOR.PARAMS.SILENCE_THRESHOLD_POWER          ADDR_PER_WORD + $M.SILENCE_DETECTOR.PARAMS.TIME_CONSTANT;

   // Amount of hysteresis to be applied on threshold power
   // format : Q1.N
   .CONST $M.SILENCE_DETECTOR.PARAMS.HYSTERESIS_LEVEL                 ADDR_PER_WORD + $M.SILENCE_DETECTOR.PARAMS.SILENCE_THRESHOLD_POWER;

   // silence_detector parameter object structure size
   // format : integer
   .CONST $M.SILENCE_DETECTOR.PARAMS.STRUC_SIZE                       1 +  ($M.SILENCE_DETECTOR.PARAMS.HYSTERESIS_LEVEL >> LOG2_ADDR_PER_WORD);
   
#endif  