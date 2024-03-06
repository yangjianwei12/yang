// *****************************************************************************
// Copyright (c) 2007 - 2020 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************


// *****************************************************************************
// NAME:
//    Sidetone filter operator
//
// DESCRIPTION:
//    It is desirable to apply a filter to the sidetone.  Also, clipping may need
//    to be applied to handle special saturation requirements of IO
//
//
// *****************************************************************************

#include "stack.h"
#include "cbops.h"
#include "cbuffer_asm.h"
#include "cbops_sidetone_filter_asm_defs.h"

#ifdef PATCH_LIBS
#include "patch_library.h"
#endif

// Private Library Exports
.PUBLIC $cbops.sidetone_filter_op;


.MODULE $M.cbops.sidetone_filter_op;
   .DATASEGMENT DM;

   // ** function vector **
   .VAR $cbops.sidetone_filter_op[$cbops.function_vector.STRUC_SIZE] =
      // reset function
      $cbops.function_vector.NO_FUNCTION,
      // amount to use function
      $cbops.basic_multichan_amount_to_use,
      // main function
      &$cbops.sidetone_filter_op.main;

.ENDMODULE;

// 2 entries exist in the index table, as this only ever works on 1 in & out channel
.CONST $cbops_sidetone_filter.NR_INDEX_ENTRIES		2*ADDR_PER_WORD;

/* *****************************************************************************
* MODULE:
*    $cbops.sidetone_filter_op.main
*
* DESCRIPTION:
*    Operator that provides gains and EQs for sidetone
*
// INPUTS:
//    - r4 = pointer to list of buffers
//    - r8 = pointer to cbops object
*
* OUTPUTS:
*    none
*
* TRASHED REGISTERS:
*    r0, r1, r2, r3, r5, r6, r7, r10, rMAC, M1, I0, I1, I4, L0, L1, L4, DoLoop
*
*****************************************************************************/

.MODULE $M.cbops.sidetone_filter_op.main;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $cbops.sidetone_filter_op.main:

   // push rLink onto stack
   push rLink;

#if defined(PATCH_LIBS)
   LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbops.CBOPS_SIDETONE_FILTER_OPERATOR_ASM.MAIN.PATCH_ID_0)
#endif

   call $cbops.get_transfer_and_update_multi_channel;
   r10 = r0;
   if LE jump jp_done_sidetone;
   // r10=data to process,  r9=num_chans in addresses

   // Get operator data pointer
   r7 = M[r8 + $cbops.param_hdr.OPERATOR_DATA_PTR_FIELD];

   // If gain is zero don't generate output.   mixer will zero insert
   NULL = M[r7 + $cbops_sidetone_filter.sidetone_filter_op_struct.APPLY_GAIN_FIELD];
   if Z jump jp_skip_sidetone;

   // Setup Input Buffer
   r0 = M[r8 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD];     // input index
   call $cbops.get_buffer_address_and_length;
   I4 = r0;
   if Z jump jp_skip_sidetone;
   L4 = r1;
   push r2;
   pop B4;

   // Setup Output Buffer
   r0 = r9 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;
   r0 = M[r8 + r0];                                            // output index
   call $cbops.get_buffer_address_and_length;
   I0 = r0;
   if Z jump jp_skip_sidetone;
   L0 = r1;
   push r2;
   pop B0;

   NULL = M[r7 + $cbops_sidetone_filter.sidetone_filter_op_struct.APPLY_FILTER_FIELD];
   if Z jump ApplyGain;
      // Save Transfer amount, data pointer, and output buffer

      // Save output buffer on stack
      push B0;
      pushm <I0,L0>;
      pushm <r8,r10>;

      // get PEQ filter data object
      r7 = r7 + $cbops_sidetone_filter.sidetone_filter_op_struct.PEQ_FIELD;
      // Samples to process
      r4 = r10;
      // PEQ execution
      // Input:  I4,L4,B4      Output:    I0,L0,B0
      call $audio_proc.peq.process_op;

      // Restore Registers
      popm <r8,r10>;
      popm <I0,L0>;
      // Restore output base address (Also, use output as input)
      B4 = M[SP - 1*ADDR_PER_WORD];
      pop B0;

      // Use output as input for Gain operation (i.e. inplace)
      I4 = I0;
      L4 = L0;

ApplyGain:
    // Input:  I4,L4,B4      Output:    I0,L0,B0

    r9 = M[r8 + $cbops.param_hdr.OPERATOR_DATA_PTR_FIELD];

    // Parameters for sidetone
    r7 = M[r9 + $cbops_sidetone_filter.sidetone_filter_op_struct.PARAMS_FIELD];

    // Get gain mantisa/exponent for sidetone
    r2 = M[r7 + $cbops_sidetone_filter.sidetone_params_struct.STF_GAIN_EXP_FIELD];
    r3 = M[r7 + $cbops_sidetone_filter.sidetone_params_struct.STF_GAIN_MANTISA_FIELD];

   // Get inverse of Current DAC gain
    r5 = M[r8 + ($cbops.param_hdr.CHANNEL_INDEX_START_FIELD + $cbops_sidetone_filter.NR_INDEX_ENTRIES +
         $cbops_sidetone_filter.sidetone_filter_op_struct.INV_DAC_GAIN_FIELD)];
    // Verify that inverse gain is not greater than the limit value
    // limit is Q9.15, same as the inverse gain.
    r1 = M[r7 + $cbops_sidetone_filter.sidetone_params_struct.ST_ADJUST_LIMIT_FIELD];
    Null = r5 - r1;
    if GT r5 = r1;

    // Consolidated target gains in just one value.
    rMAC = r3 * r5;
    rMAC = rMAC ASHIFT r2(56bit);
    r0 = rMAC;

    // get the current gain, if ramping is disabled this will be the target config gain
    // with ramping this will gradually be moving towards the target gain.
    r5 = M[r9 + $cbops_sidetone_filter.sidetone_filter_op_struct.EXP_RAMPING_STEP_FIELD];
    r2 = M[r9 + $cbops_sidetone_filter.sidetone_filter_op_struct.LIN_RAMPING_STEP_FIELD];
    r1 = M[r9 + $cbops_sidetone_filter.sidetone_filter_op_struct.CURRENT_GAIN_FIELD];

    // r5 = exponential ramping step
    // r2 = linear ramping step
    // if both are 0, no ramping required
    NULL = r2 OR r5;
    if Z r1 = r0;
    // store current gain in r3, this will be the gain that
    // will apply to first sample of this block
    r3 = r1;

    // optimisation: if we have already reached
    // target gain no fading step is needed to calculate
    r1 = r0 - r1;
    if Z jump calc_step_done;

    // Ramping is enabled, we need gradually move current gain
    // towards target gain, need to work out the fading step
    // r0 = target gain:     Q:9.x
    // r3 = current gain  Q:9.x
    // r5 = exp ramping factor
    // r2 = lin ramping factor
    // r1 = diff = target_gain - current gain
    r5 = r5 * r3(frac);       // r5 = exponential_step
    r5 = r5 + r2;             // r5 = exponential_step + linear_step
    r5 = r5 + 1;              // to avoid getting stuck in 0
    r2 = ABS r1;              // limit change to 24dB less than diff, this will
    r2 = r2 * (1/16.0) (frac);//     provide smooth curve at the end of ramping time
    r2 = r2 + 1;              // to avoid getting stuck in 0
    r5 = MIN r2;              // r5 = ramping gain step
    r1 = MIN r5;              // limit change to +ramping_step
    r5 = -r5;
    r1 = MAX r5;              // limit change to -ramping_step
    // r0 = gain change for r10 samples
    // workout gain change for 1 sample so we will have
    // per sample smooth gains
    rMAC = r1 ASHIFT 0 (LO); // whole rMAC integer-wise = r1
    DIV = rMAC / r10;
    r0 = r1 + r3;            // current gain + ramping_step
    r1 = DivResult;          // step per sample for this run
    calc_step_done:

    // update current gain (will be used next run)
    M[r9 + $cbops_sidetone_filter.sidetone_filter_op_struct.CURRENT_GAIN_FIELD] = r0;

    // exponent, gain is Q9.x
    r6 = 8;

    // Get PeakSideTone Address
    r2 = M[r9 + $cbops_sidetone_filter.sidetone_filter_op_struct.PEAK_LEVEL_FIELD];

    // Get clipper limits
    // r7 = clip point, r5 = -clip point
    r7 = M[r7 + $cbops_sidetone_filter.sidetone_params_struct.ST_CLIP_POINT_FIELD];
    r5 = -r7;

    // apply gain and ramp if needed
    // At this point
    //   r3: gain mantissa
    //   r6: gain exponent
    //   r7: sidetone clip point (+)
    //   r5: sidetone clip point (-)
    //   I4/L4/B4: input buffer
    //   I0/L0/B0: output buffer
    //   r2: current peak level
    //   r10: number of samples to process( > 0)
    //   r1: gain step per sample

    // read first sample out of the loop
    r0 = M[I4, MK1];
    do gain_ramp_clip_loop;

       // apply gain: sample = sample*mantissa*(2.0**exponent)
       rMAC = r0 * r3, r0 = M[I4, MK1]; // also reads next input sample
       rMAC = rMAC ASHIFT r6 (56bit);

       // limit sidetone sample between r5=-st_clip_point and  r4=+st_clip_point
       rMAC = MIN r7;
       rMAC = MAX r5;

       // ramp gain, and write output
       r3 = r3 + r1, M[I0, MK1] = rMAC;

       // update peak value
       r2 = MAX rMAC;
    gain_ramp_clip_loop:
    M[r9 + $cbops_sidetone_filter.sidetone_filter_op_struct.PEAK_LEVEL_FIELD] = r2;
jp_done_sidetone:
   // reset L registers
   L0 = 0;
   L4 = 0;
   // clear base registers
   push Null;
   B0 = M[SP - 1*ADDR_PER_WORD];
   pop B4;

   pop rLink;
   rts;

jp_skip_sidetone:
   M[r7 + $cbops_sidetone_filter.sidetone_filter_op_struct.CURRENT_GAIN_FIELD] = NULL;
   M[r7 + $cbops_sidetone_filter.sidetone_filter_op_struct.PEAK_LEVEL_FIELD]=NULL;

   // Set amount of output produced to zero
   r0 = r9 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;
   r0 = M[r8 + r0];                                            // output index
   call $cbops.get_amount_ptr;
   M[r0]=NULL;

   jump jp_done_sidetone;

.ENDMODULE;


/******************************************************************************
* MODULE:
*    $cbops.sidetone_filter_op.SetMode
*
* DESCRIPTION:
*    Setup the sidetone mode
*
*    APPLY_GAIN_FIELD:
*             1.0 = sidetone enabled
*               0 = sidetone disabled
*    APPLY_FILTER:
*             0 = Number of PEQ stages is zero or (OFFSET_STF_SWITCH&1==zereo)
*             1 = Noise Level is above upper threshold
*             0 = Noise Level is below lower threshold
*
* INPUTS:
*    - r0 = pointer to operator structure:
*
* OUTPUTS:
*    - none
*
* TRASHED REGISTERS:
*
*
******************************************************************************/

.MODULE $M.cbops.sidetone_filter_op.SetMode;
   .CODESEGMENT PM;

// void      update_sidetone_filter_op(void *params,unsigned enable,unsigned noise_level);
// inv_dac_gain ???
$_update_sidetone_filter_op:

#if defined(PATCH_LIBS)
   LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbops.CBOPS_SIDETONE_FILTER_OPERATOR_ASM.UPDATE_SIDETONE_FILTER_OP.PATCH_ID_0)
#endif
                            
   // Advance to sidetone data
   r0 = M[r0 + ($cbops_c.cbops_op_struct.PARAMETER_AREA_START_FIELD+$cbops.param_hdr.OPERATOR_DATA_PTR_FIELD)];
    
   // ST disabled if 'enable' is zero. Params are after 2 locations of index table as it
   // always ever uses single in and out channel.
   M[r0 + $cbops_sidetone_filter.sidetone_filter_op_struct.APPLY_GAIN_FIELD] = r1;
   if Z rts;

   push r7;
   
   r1 = Null;
   // Check number of stages.  If zero then bypass filter
   NULL = M[r0 + ($cbops_sidetone_filter.sidetone_filter_op_struct.PEQ_FIELD +
                  $audio_proc.peq.MAX_STAGES_FIELD)];
   if Z jump exit_filter_update;

   // Parameter Pointer
   r7 = M[r0 + $cbops_sidetone_filter.sidetone_filter_op_struct.PARAMS_FIELD];
   
   // if switch==0, bypass filter
   // if switch==1, apply filter
   r1 = M[r7 + $cbops_sidetone_filter.sidetone_params_struct.STF_SWITCH_FIELD];
   NULL = r1 AND -2;             // 0xFFFFFE (arch4: 0xFFFFFFFE)
   if Z jump exit_filter_update;
   // SP.  rz is non-zero here

   // noise level is input parameter (r2)
   // if noise level is above high threshold, apply PEQ filter
   r3 = M[r7 + $cbops_sidetone_filter.sidetone_params_struct.STF_NOISE_HIGH_THRES_FIELD];
   Null = r2 - r3;
   if GT jump exit_filter_update;

   // if noise level is below low threshold, bypass filter
   r1 = 0;
   r3 = M[r7 + $cbops_sidetone_filter.sidetone_params_struct.STF_NOISE_LOW_THRES_FIELD];
   Null = r2 - r3;
   if POS jump done_sidetone_mode;

   exit_filter_update:
   M[r0 + $cbops_sidetone_filter.sidetone_filter_op_struct.APPLY_FILTER_FIELD] = r1;

   done_sidetone_mode:
   pop r7;
   rts;

// void      initialize_sidetone_filter_op(void *params);
$_initialize_sidetone_filter_op:
#if defined(PATCH_LIBS)
   LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbops.CBOPS_SIDETONE_FILTER_OPERATOR_ASM.INITIALIZE_SIDETONE_FILTER_OP.PATCH_ID_0)
#endif
                            
   push rlink;
   pushm <r7,r8>;
   push I0;
   
   // Advance to sidetone data
   r0 = M[r0 + ($cbops_c.cbops_op_struct.PARAMETER_AREA_START_FIELD+$cbops.param_hdr.OPERATOR_DATA_PTR_FIELD)];
   
   // initialize sidetone PEQ filter
   r7 = r0 + $cbops_sidetone_filter.sidetone_filter_op_struct.PEQ_FIELD;
   call $audio_proc.peq.initialize;
   pop I0;
   popm <r7,r8>;
   jump $pop_rLink_and_rts;

.ENDMODULE;

// Expose the location of this table to C
.set $_cbops_sidetone_filter_table,  $cbops.sidetone_filter_op



