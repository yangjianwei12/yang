// *****************************************************************************
// Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
// *****************************************************************************

#ifndef _EQ100_LIB_H
#define _EQ100_LIB_H

   // *****************************************************************************
   //                               eq100 data object structure
   // *****************************************************************************
   // pointer to internal mic data
   // format : integer
   .CONST $eq100.INTMIC_PTR_FIELD                     0;    
   // pointer to external mic data
   // format : integer
   .CONST $eq100.EXTMIC_PTR_FIELD                     ADDR_PER_WORD + $eq100.INTMIC_PTR_FIELD;
   // pointer to eq100 params
   // format : integer
   .CONST $eq100.PARAM_PTR_FIELD                      ADDR_PER_WORD + $eq100.EXTMIC_PTR_FIELD;
   // Pointer to cvc variant variable
   // format : integer
   .CONST $eq100.PTR_VARIANT_FIELD                    ADDR_PER_WORD + $eq100.PARAM_PTR_FIELD;
   // Pointer to nproto
   // format : integer
   .CONST $eq100.NPROTO_FRAME_FIELD                   ADDR_PER_WORD + $eq100.PTR_VARIANT_FIELD;
   // Pointer to hold flag
   // format : integer
   .CONST $eq100.LOW_INPUT_FLAG_PTR_FIELD             ADDR_PER_WORD + $eq100.NPROTO_FRAME_FIELD;
   // @DOC_FIELD_TEXT Pointer to map coefficients
   // @DOC_FIELD_FORMAT q.24
   .CONST $eq100.SMOOTHED_RATIO_PTR_FIELD             ADDR_PER_WORD + $eq100.LOW_INPUT_FLAG_PTR_FIELD;
   // @DOC_FIELD_TEXT Pointer to scratch
   // @DOC_FIELD_FORMAT integer
   .CONST $eq100.EQ100_SCRATCH_1_PTR                  ADDR_PER_WORD + $eq100.SMOOTHED_RATIO_PTR_FIELD;;
   .CONST $eq100.EQ100_SCRATCH_2_PTR                  ADDR_PER_WORD + $eq100.EQ100_SCRATCH_1_PTR;
   .CONST $eq100.BLEND_DIM_PTR_FIELD                  ADDR_PER_WORD + $eq100.EQ100_SCRATCH_2_PTR;
   .CONST $eq100.MAX_AMPLIFICATION                    ADDR_PER_WORD + $eq100.BLEND_DIM_PTR_FIELD;
   .CONST $eq100.NULL_FREQ                            ADDR_PER_WORD + $eq100.MAX_AMPLIFICATION;
   .CONST $eq100.HOLD_ADAPT_FLAG_FIELD                ADDR_PER_WORD + $eq100.NULL_FREQ;

   // =========================== START OF INTERNAL FIELDS ============================================

   // FFT length field
   // format : integer
   .CONST $eq100.FFTLEN_FIELD                         ADDR_PER_WORD + $eq100.HOLD_ADAPT_FLAG_FIELD;
   //initial_alpha field
   .CONST $eq100.INITIAL_ALPHA_FIELD                  ADDR_PER_WORD + $eq100.FFTLEN_FIELD;
   //steady_alpha field
   .CONST $eq100.STEADY_ALPHA_FIELD                   ADDR_PER_WORD + $eq100.INITIAL_ALPHA_FIELD;
   //initial frame field
   .CONST $eq100.NUM_INITIAL_FRAMES_FIELD             ADDR_PER_WORD + $eq100.STEADY_ALPHA_FIELD;
   .CONST $eq100.PTR_INT_FLOOR_FIELD                  ADDR_PER_WORD + $eq100.NUM_INITIAL_FRAMES_FIELD;

   // eq100 structure size
   // format : integer
   .CONST $eq100.STRUC_SIZE                           1 +  ($eq100.PTR_INT_FLOOR_FIELD >> LOG2_ADDR_PER_WORD);


   // *****************************************************************************
   //                               eq100 parameters object structure
   // *****************************************************************************
   //EQ initial_tau 
   // format : q7.25
   .CONST $eq100.param.INIT_TAU                       0;
   //EQ steady_tau 
   // format : q7.25
   .CONST $eq100.param.STEADY_TAU                     ADDR_PER_WORD + $eq100.param.INIT_TAU;
   //EQ L_initial 
   // format : q7.25
   .CONST $eq100.param.L_INITIAL                      ADDR_PER_WORD + $eq100.param.STEADY_TAU;
   //EQ Null Frequency
   // format : Integer
   .CONST $eq100.param.NULL_FREQ                      ADDR_PER_WORD + $eq100.param.L_INITIAL;
   //EQ Maximum Amplification
   // format : Q8.24 in LOG2 DB
   .CONST $eq100.param.MAX_AMPLIFICATION              ADDR_PER_WORD + $eq100.param.NULL_FREQ;

   // eq100 params structure size
   // format : integer
   .CONST $eq100.params.STRUC_SIZE                    1 +  ($eq100.param.MAX_AMPLIFICATION >> LOG2_ADDR_PER_WORD);

#endif // _EQ100_LIB_H
