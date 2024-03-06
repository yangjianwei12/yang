// *****************************************************************************
// Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
// %%version
//
// $Change$  $DateTime$
// *****************************************************************************

// ASM function for VAD operator data processing
// The function(s) obey the C compiler calling convention (see documentation, CS-124812-UG)

#include "svad_capability_asm_defs.h"
#include "svad_gen_asm.h"
#include "portability_macros.h"
#include "stack.h"
#include "peq.h"
#include "filter_bank_library.h"
#include "math_library.h"
#include "cvc_stream_asm_defs.h"
#include "svad100_library.h"
#include "patch_library.h"

#define $SVAD.WB_NUM_SAMPLES_PER_FRAME          120

/* Do not edit without editing SVAD_ALGS_CONTAINER in svad_capability.h */
.CONST   $svad_algs_container.svad_object_ptr          0*ADDR_PER_WORD;
.CONST   $svad_algs_container.afb_ext_mic_object_ptr   1*ADDR_PER_WORD;
.CONST   $svad_algs_container.afb_int_mic_object_ptr   2*ADDR_PER_WORD;


// *****************************************************************************
// MODULE:
//    $_SVAD_PROCESS
//
// DESCRIPTION:
//    Performs SVAD initialization (per $_svad_init)
//    Performs SVAD data processing per ($_svad_process)
//
// TRASHED REGISTERS:
//    C compliant
//
// *****************************************************************************
.MODULE $M.SVAD_PROCESS; 
    .CODESEGMENT PM;
    .DATASEGMENT DM;

.VAR/CONST svad.filter_bank.config.fftsplit_table1[] = 
      // Table cos128: capable of 128/256/512 point real FFT
      //    N = 128;
      //    cos128=cos((1:N-1)*pi/2/N)
      $M.filter_bank.Parameters.FFT512_SCALE,
      #include "fftsplit_cos_128.dat"
      ;

.VAR/CONST h0[256] =
      #include "proto_hanning_16k_120_240_256.dat"
      ;

// *****************************************************************************
// FUNCTION:
//    $_svad_init
//
// DESCRIPTION:
//    If reinit flag is non-zero this function:
//      1) Initializes analysis filter bank for internal and external mics
//      2) Initializes self voice activity detector (SVAD)
//      3) Set reinit flag to zero
//
// INPUTS:
//    - r0 = pointer to SVAD_OP_DATA structure
//    - r1 = pointer to SVAD_ALGS_CONTAINER structure
//
// OUTPUTS:
//    - None
//
// TRASHED REGISTERS:
//    C compliant
//
// *****************************************************************************

$_svad_init:
    PUSH_ALL_C
    LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($svad_cap.SVAD_HELPER_FUNCTIONS_ASM.SVAD_INIT.PATCH_ID_0, r4)

    Null = M[r0 + $svad_capability.svad_extra_op_struct.REINITFLAG_FIELD];
    if Z jump done_afb_init;

        push r0;         // stack = svad op data
        push r1;         // stack = svad algs container, svad op data
        r8 = M[r1 + $svad_algs_container.afb_ext_mic_object_ptr];

        // Load proto_hanning window into memory used by ext and int mic AFBs
        r0 = &h0;
        r1 = M[r8 + $M.filter_bank.Parameters.OFFSET_CONFIG_OBJECT];
        M[r1 + $M.filter_bank.config.PTR_PROTO] = r0;

        // Load fft split table into memory used by ext and int mic AFBs
        r0 = &svad.filter_bank.config.fftsplit_table1;
        r1 = M[r8 + $filter_bank.FFT_OBJ_FIELD];
        M[r1 + $M.filter_bank.fft.PTR_FFTSPLIT] = r0;
        
        // Initialize external mic AFB
        r7 = Null;
        push FP;
        call $filter_bank.analysis.initialize;
        pop FP;

        pop r0;          // r0 = svad_alg_container
        push r0;         // save back to stack
        r8 = M[r0 + $svad_algs_container.afb_int_mic_object_ptr];

        // Initialize internal mic AFB
        r7 = Null;
        push FP;
        call $filter_bank.analysis.initialize;
        pop FP;

        pop r0;          // r0 = svad_alg_container
        r8 = M[r0 + $svad_algs_container.svad_object_ptr];

        // Initialize SVAD algorithm
        push FP;
        call $svad100.initialize;
        pop FP;

        pop r0;
        M[r0 + $svad_capability.svad_extra_op_struct.REINITFLAG_FIELD] = Null;

    done_afb_init:
    
    POP_ALL_C
    rts;

// *****************************************************************************
// FUNCTION:
//    $_svad_process
//
// DESCRIPTION:
//     Calls analysis filter bank for internal and external mics
//     Calls self voice activity detector (SVAD) processing
//
// INPUTS:
//    - r0 = internal mic cbuffer struc
//    - r1 = external mic cbuffer struc
//    - r2 = pointer to SVAD_ALGS_CONTAINER structure
//    - r3 = pointer to SVAD_OP_DATA structure
//
// OUTPUTS:
//    - None
//
// TRASHED REGISTERS:
//    C compliant
//
// *****************************************************************************
$_svad_process:

    PUSH_ALL_C
    LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($svad_cap.SVAD_HELPER_FUNCTIONS_ASM.SVAD_PROCESS.PATCH_ID_0, r4)

    push r3; // svad op data

    r8 = M[r2 +  $svad_algs_container.afb_int_mic_object_ptr];
    r4 = M[r2 + $svad_algs_container.svad_object_ptr];
    push r4; // 

    r4 = M[r2 +  $svad_algs_container.afb_ext_mic_object_ptr];
    push r4;

    push r1; // ext mic cbuffer struc
    push r0; // int mic cbuffer struc
    
    /* load internal mic afb's input stream object from int mic cbuffer structure */
    call $cbuffer.get_read_address_and_size_and_start_address;
    r9 = $SVAD.WB_NUM_SAMPLES_PER_FRAME;
    r6 = M[r8 + $M.filter_bank.Parameters.OFFSET_PTR_FRAME];
    M[r6 + $cvc_stream.frmbuffer_struct.FRAME_PTR_FIELD] = r0;
    M[r6 + $cvc_stream.frmbuffer_struct.BUFFER_SIZE_FIELD] = r1;
    M[r6 + $cvc_stream.frmbuffer_struct.BUFFER_ADDR_FIELD] = r2;
    M[r6 + $cvc_stream.frmbuffer_struct.FRAME_SIZE_FIELD] = r9;

    // r8 = int mic afb object
    r7 = Null;
    push FP;
    call $filter_bank.analysis.process;
    pop FP;

    // Update read address for int mic input buffer
    pop r0; // r0 = int mic cbuffer struc
    r1 = $SVAD.WB_NUM_SAMPLES_PER_FRAME;
    call $cbuffer.advance_read_ptr;

    /* load external mic afb's input stream object from cbuffer structure */
    pop r0; // r0 = ext mic cbuffer struc
    call $cbuffer.get_read_address_and_size_and_start_address;
   
    r9 = $SVAD.WB_NUM_SAMPLES_PER_FRAME;
    pop r8;
    r6 = M[r8 + $M.filter_bank.Parameters.OFFSET_PTR_FRAME];
    M[r6 + $cvc_stream.frmbuffer_struct.FRAME_PTR_FIELD] = r0;
    M[r6 + $cvc_stream.frmbuffer_struct.BUFFER_SIZE_FIELD] = r1;
    M[r6 + $cvc_stream.frmbuffer_struct.BUFFER_ADDR_FIELD] = r2;
    M[r6 + $cvc_stream.frmbuffer_struct.FRAME_SIZE_FIELD] = r9;

    r7 = Null;
    push FP;
    call $filter_bank.analysis.process;
    pop FP;

    // No need to update read address for external mic cbuffer struc as that's
    // done in svad.c when its data is copied to the output buffer
    pop r8;
    pop r0; 
    r4 = M[r0 + $svad_capability.svad_extra_op_struct.F_HANDLE_FIELD];
    push FP;
    call $svad100.process;
    pop FP;

    POP_ALL_C
    rts;

.ENDMODULE;
