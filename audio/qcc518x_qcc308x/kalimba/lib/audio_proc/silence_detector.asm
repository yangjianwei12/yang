// *****************************************************************************
// Copyright (c) 2014 - 2021 Qualcomm Technologies International, Ltd.
// %% version
//
// $Change$  $DateTime$
// *****************************************************************************

#include "silence_detector.h"
#include "stack.h"

#ifdef PATCH_LIBS
   #include "patch_library.h"
#endif

//******************************************************************************
// MODULE:
//    silence_detector.initialize
//
// DESCRIPTION:
//    silence_detector initialization
//
// INPUTS:
//    - r0  - silence_detector data object
//    - r1  - frame_size in samples
//    - r2  - sample rate in samples/second
// OUTPUTS:
//
// TRASHED REGISTERS:
//    - Assume everything
//
// CPU USAGE:
//    -
//******************************************************************************
.MODULE $M.silence_detector.initialize;

   .CODESEGMENT PM;

$_silence_detector_lib_initialize:
   PUSH_ALL_C
   
#if defined(PATCH_LIBS)
   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($silence_detector.SILENCE_DETECTOR_ASM.INITIALIZE.PATCH_ID_0,r10)
#endif

   // 1/length(N)
   rMAC = 1;
   r3 = r1 ASHIFT 1;
   div = rMAC/r3;
   r3 = DivResult;
   M[r0 + $M.SILENCE_DETECTOR.ONE_OVER_FRAME_SIZE] = r3;

   r3 = M[r0 + $M.SILENCE_DETECTOR.PARAMS_OBJ_PTR];
   r3 = M[r3 + $M.SILENCE_DETECTOR.PARAMS.TIME_CONSTANT];

   //alpha = 1 - exp(-1/(tau*(FS/FRAME_SIZE)));
   call $calc.tc_alfa;
   M[r0 + $M.SILENCE_DETECTOR.ALPHA] = r1;


   // set upper and lower thresholds for silence detector
   r3 = M[r0 + $M.SILENCE_DETECTOR.PARAMS_OBJ_PTR];
   r1 = M[r3 + $M.SILENCE_DETECTOR.PARAMS.HYSTERESIS_LEVEL];
   r10 = M[r3 + $M.SILENCE_DETECTOR.PARAMS.SILENCE_THRESHOLD_POWER];
   r2 = r10 * r1;
   r2 = ABS r2;

   r1 = r10 - r2;
   M[r0 + $M.SILENCE_DETECTOR.LOWER_THRESHOLD_POWER] = r1;

   r1 = r10 + r2;
   M[r0 + $M.SILENCE_DETECTOR.UPPER_THRESHOLD_POWER] = r1;

   // Intialize non-scratch fields to zero
   M[r0 + $M.SILENCE_DETECTOR.SMOOTHED_POWER_LINEAR] = Null;
   M[r0 + $M.SILENCE_DETECTOR.COMPUTED_POWER_LOG2] = Null;
   // Unknown silence detection event (2)
   r1 = 2;
   M[r0 + $M.SILENCE_DETECTOR.SILENCE_DETECTION_EVENT] = r1;

   POP_ALL_C
   rts;

.ENDMODULE;

//******************************************************************************
// MODULE:
//    $silence_detector.process
//
// DESCRIPTION:
//    silence_detector process function
//
// INPUTS:
//    - r0 - pointer to silence_detector data object
//    - r1 - number of samples to process (frame_size)
//    -
//
// OUTPUTS:
//
// TRASHED REGISTERS:
//    - Assume everything
//
// CPU USAGE:
//    -
//******************************************************************************
.MODULE $M.silence_detector.process;

   .CODESEGMENT PM;

$_silence_detector_lib_process:
   
   PUSH_ALL_C

   #if defined(PATCH_LIBS)
      LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($silence_detector.SILENCE_DETECTOR_ASM.PROCESS.PATCH_ID_0,r10)
   #endif

   r4 = r0;
   // r10 -> frame_size
   r10 = r1;

   // get read pointer, size and start addresses of input buffer
   r0 = M[r4 +$M.SILENCE_DETECTOR.INPUT_BUFFER_PTR];
   call $cbuffer.get_read_address_and_size_and_start_address;
   push r2;
   pop B0;
   I0 = r0;
   L0 = r1;

   // sum(input_buffer.^2)
   rMAC = 0, r3 = M[I0,MK1];
   do sum_loop;
      rMAC = rMAC + r3*r3, r3 = M[I0,MK1];
   sum_loop:

   //power_linear = sum(input_buffer.^2)/frame_size;
   r1 = M[r4 + $M.SILENCE_DETECTOR.ONE_OVER_FRAME_SIZE];
   r3 = SIGNDET rMAC;
   rMAC = rMAC ASHIFT r3 (56bit);
   rMAC = rMAC * r1;
   r3 = -1 * r3 (int);
   r5 = rMAC ASHIFT r3;

   //**********************************************************************************
   // smoothed_power_linear = (1-alpha)*smoothed_power_linear + alpha*power_linear
   //**********************************************************************************
   r2 = M[r4 + $M.SILENCE_DETECTOR.ALPHA];                               // load alpha
   r3 = 1.0 - r2;                                                        // r3:(1-alpha)
   rMAC = M[r4 + $M.SILENCE_DETECTOR.SMOOTHED_POWER_LINEAR];             // load smoothed power
   rMAC = rMAC * r3 ;                                                    // (1-alpha) * smoothed_power_linear
   rMAC = rMAC + r2 * r5;                                                // smoothed_power_linear = (1-alpha)*smoothed_power_linear + alpha*power_linear
   M[r4 +$M.SILENCE_DETECTOR.SMOOTHED_POWER_LINEAR] = rMAC;              // save smoothed_power_linear

   call $math.log2_table;
   M[r4 + $M.SILENCE_DETECTOR.COMPUTED_POWER_LOG2] = r0;

   // if ( power < lower_threshold) we have silence
   r2 = M[r4 + $M.SILENCE_DETECTOR.SILENCE_DETECTION_EVENT];
   r1 = M[r4 + $M.SILENCE_DETECTOR.LOWER_THRESHOLD_POWER];
   Null = r0 - r1;
   if LT r2 = 1;

   // if ( power > upper_threshold) we have audio
   r1 = M[r4 + $M.SILENCE_DETECTOR.UPPER_THRESHOLD_POWER];
   Null = r0 - r1;
   if GT r2 = 0;

   M[r4 + $M.SILENCE_DETECTOR.SILENCE_DETECTION_EVENT] = r2;

   POP_ALL_C
   rts;
   
.ENDMODULE;

//******************************************************************************
// MODULE:
//    $calc.tc_alfa
//
// DESCRIPTION:
//    alfa = 1 - exp(-1/(tau*(fs/L)))
//
//       fs : sampling rate
//       L  : frame size
//       tau: time constant
//
// INPUTS:
//       r1  : frame size
//       r2  : sampling rate
//       r3  : time constant
//
// OUTPUTS:
//    - r1 - alpha
//
// TRASHED REGISTERS:
//    - Assume everything
//
// CPU USAGE:
//    -
//******************************************************************************
.MODULE $M.calc.tc_alfa;

   .CODESEGMENT PM;

$calc.tc_alfa:
   
   pushm <r0, rLink>;  
   
#if defined(PATCH_LIBS)
   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($silence_detector.SILENCE_DETECTOR_ASM.CALC.TC_ALPHA.PATCH_ID_0,r10)
#endif

   // 1/t
   r6 = SIGNDET r3;
   r3 = r3 ASHIFT r6;
   rMAC = 0.25;
   div = rMAC/r3;

   // t in Q7.17
   r6 = r6 - 6;
   // div guard bit
   r6 = r6 + 1;
   // Q8.16 pow2 input
   r6 = r6 - 7;

   // 1/t result
   r3 = divResult;

   //r2 = fs
   //r1 = L
   r0 = -r1;
   rMAC = r0 ASHIFT -1;
   Div = rMAC/r2;
   //r0 = (-L/fs);
   r0 = divResult;  

   // -log2(e)/(fs/L)
   r2 = Qfmt_(1.442695040888963, 8);
   r0 = r2 *r0;
   r0 = r0 ASHIFT 7;

   // -log2(e)/(fs/L)*(1/t)
   rMAC = r0 * r3;
   r0 = rMAC ASHIFT r6;

   // exp(-1/(t*(fs/L)))
   call $math.pow2_taylor;

   // r0 = alfa_c = exp(-1/(t*(fs/L)))
   // alfa = 1 - exp(-1/(t*(fs/L)))
   r1 = 1.0 - r0;

   popm <r0, rLink>;
   rts;
   
.ENDMODULE;
