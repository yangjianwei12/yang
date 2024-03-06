
// *****************************************************************************
// Copyright (c) 2022 Qualcomm Technologies International, Ltd.
// *****************************************************************************

#include "cbops/cbops.h"
#include "cbops_aec_ref_llm_custom_op_asm_defs.h"

.MODULE $M.cbops.aec_ref_llm_custom_op;
    .CODESEGMENT PM;
    .DATASEGMENT DM;

    //function vector
    .VAR $cbops.aec_ref_llm_custom_op[$cbops.function_vector.STRUC_SIZE] = 
        $cbops.function_vector.NO_FUNCTION,          // RESET FUNCTION
        &$cbops.aec_ref_llm_custom_op.amount_to_use, // AMOUNT_TO_USE_FUNCTION
        &$cbops.aec_ref_llm_custom_op.main;          // MAIN FUNCTION

// Expose the location of this table to C
.set $_cbops_aec_ref_llm_custom_op_table, $cbops.aec_ref_llm_custom_op

// *****************************************************************************
// MODULE:
//   $cbops.aec_ref_llm_custom_op.amount_to_use
//
// DESCRIPTION:
//   Amount to use function of the custom cbops operator
//
// INPUTS:
//    - r4 = buffer table
//    - r8 = pointer to operator structure
//
// OUTPUTS:
//
// TRASHED REGISTERS:
//    Assume anything except r4 and r8
//
// *****************************************************************************
$cbops.aec_ref_llm_custom_op.amount_to_use:

    pushm<r4, r8, rLink>;

    // get number of input channels - transform it into addresses, so that we don't do the latter
    // for every channel. But let's keep calling it subsequently the "number of channels"
    r9 = M[r8 + $cbops.param_hdr.NR_INPUT_CHANNELS_FIELD];
    Words2Addr(r9);

    // get the input and output index for first channel
    // returns amount to process in r0
    r1 = r8 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;
    r0 = M[r1 + 0];     // input index (first channel)
    r1 = M[r1 + r9];    // Output index (first channel)

    // Get the transfer amount at input
    r2 = r4 + $cbops_c.cbops_buffer_struct.TRANSFER_PTR_FIELD;
    // Get transfer amount of input
    r0 = r0 * $CBOP_BUFTAB_ENTRY_SIZE_IN_ADDR (int);
    r0 = M[r2 + r0];
    // save the transfer pointer onto stack as we will need to 
    // update it.
    push r0;
    // Transfer amount at input
    r0 = M[r0];

    // Get the amount of space in the output
    r1 = r1 * $CBOP_BUFTAB_ENTRY_SIZE_IN_ADDR (int);
    r1 = M[r2 + r1];
    r1 = M[r1];
    // The amount of data that we can process is the minimum of
    // the amount of data at the input and the amount of space
    // at the output
    r0 = MIN r1;

    // call C function which can potentially change the TRANSFER_AMOUNT_FIELD
    call $_aec_ref_llm_custom_op_amount_to_process;

    r1 = r0;
    pop r0;
    M[r0] = r1;

    popm<r4, r8, rLink>;
    rts;

// *****************************************************************************
// MODULE:
//   $cbops.aec_ref_llm_custom_op.main
//
// DESCRIPTION:
//   Main processing function of the custom cbops operator
//
// INPUTS:
//    - r4 = buffer table
//    - r8 = pointer to operator structure
//
// OUTPUTS:
//
// TRASHED REGISTERS:
//    Assume anything except r4 and r8
//
// *****************************************************************************
$cbops.aec_ref_llm_custom_op.main:

    push rLink;

    // Get data pointer
    r7 = M[r8 + $cbops.param_hdr.OPERATOR_DATA_PTR_FIELD];
    // Number of input channels
    r9 = M[r8 + $cbops.param_hdr.NR_INPUT_CHANNELS_FIELD];
    Words2Addr(r9);
    r0 = r9 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;
    r0 = M[r8 + r0]; // output index
    // output buffer
    call $cbops.get_buffer_address_and_length;
    I0 = r0;
    L0 = r1;
    push r2;
    pop B0;


    // Get the input cbuffer
    r0 = M[r8 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD]; // first input channel idx
    r1 = r0; // save the first input channel index into r1
    // get the amount of data
    r2 = r4 + $cbops_c.cbops_buffer_struct.TRANSFER_PTR_FIELD;
    r0 = r0 * $CBOP_BUFTAB_ENTRY_SIZE_IN_ADDR (int);
    r0 = M[r2 + r0];
    // Transfer amount at input
    r0 = M[r0];
    // Fill in the amount of data field
    M[r7 + $cbops_aec_ref_llm_custom_op.cbops_aec_ref_llm_custom_op_struct.AMT_DATA_FIELD] = r0;


    // get the cbuffer corresponding to this index
    // retrieve the first input channel index from r1
    r0 = r1;
    call $cbops.get_buffer_address_and_length;
    I1 = r0;
    L1 = r1;
    push r2;
    pop B1;

    // Check if there is another input
    I4 = 0;
    r0 = M[r7 + $cbops_aec_ref_llm_custom_op.cbops_aec_ref_llm_custom_op_struct.NUM_INPUTS_FIELD];
    r0 = r0 - 1;
    if Z jump mono_input;
    // stereo input - get the buffer corresponding to the second mic
    r0 = $cbops.param_hdr.CHANNEL_INDEX_START_FIELD + ADDR_PER_WORD;
    r0 = M[r8 + r0];
    // get the buffer corrsponding to this index
    call $cbops.get_buffer_address_and_length;
    I4 = r0;
    L4 = r1;
    push r2;
    pop B4;


mono_input:

    // setup for calling the actual processing function
    // r0: Input linear buffer for channel 1 - present in I1
    // r1: Input linear buffer for channel 2 - present in I4
    // r2: Output linear buffer - present in I0
    // r3: Parameter - present in r7

    r0 = I1;
    r1 = I4;
    r2 = I0;
    r3 = r7;

    pushm <r4,r8>;


    // call the actual processing function for this
    // custom cbop operator.
    call $_aec_ref_llm_custom_op_main_processing;

    popm<r4,r8>;

    r9 = M[r8 + $cbops.param_hdr.NR_INPUT_CHANNELS_FIELD];
    Words2Addr(r9);

    // get the input and output index for first channel
    // returns amount to process in r0
    r1 = r8 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;
    r1 = M[r1 + r9];    // Output index (first channel)
    r2 = r4 + $cbops_c.cbops_buffer_struct.TRANSFER_PTR_FIELD;
    // Update output
    r1 = r1 * $CBOP_BUFTAB_ENTRY_SIZE_IN_ADDR (int);
    r1 = M[r2 + r1];
    M[r1] = r0;


    pop rLink;
    rts;
.ENDMODULE;
