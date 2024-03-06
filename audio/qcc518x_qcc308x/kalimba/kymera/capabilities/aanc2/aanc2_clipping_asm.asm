/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
#include "portability_macros.h"
#include "stack.h"
#include "aanc2_gen_asm.h"
#include "aanc2_clipping_asm_defs.h"

// *****************************************************************************
// MODULE:
//   $aanc2_clipping.clipping_peak_detect_dual
//
// DESCRIPTION:
//    AANC2 clipping and peak detection on two input buffers
//
// INPUTS:
//    - r0 = Pointer to aanc2 clipping struct in DM1
//    - r1 = Pointer to aanc2 clipping struct in DM2
//    - r2 = Number of samples to process
//
// OUTPUTS:
//    - r0 = Success/failure
//
// TRASHED REGISTERS:
//    C compliant
//
// NOTES:
//
// *****************************************************************************

.MODULE $M.aanc2_clipping.clipping_peak_detect;
    .CODESEGMENT PM;

$_aanc2_clipping_peak_detect_dual:

    PUSH_ALL_C;

    r8 = r0;    // DM1 struct
    r9 = r1;    // DM2 struct
    r10 = r2;   // Number of samples to process

    r3 = M[r8 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.THRESHOLD_FIELD];
    r4 = M[r9 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.THRESHOLD_FIELD];

    // Get cbuffer details for DM1 input
    r0 = M[r8 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.P_DATA_FIELD];
    call $cbuffer.get_read_address_and_size_and_start_address;
    push r2;
    pop B0;
    I0 = r0;
    L0 = r1;

    // Get cbuffer details for DM2 input
    r0 = M[r9 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.P_DATA_FIELD];
    call $cbuffer.get_read_address_and_size_and_start_address;
    push r2;
    pop B4;
    I4 = r0;
    L4 = r1;

    r5 = 0; // DM1 mic clipping detection
    r6 = 0; // DM2 mic clipping detection

    r7 = M[r8 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.PEAK_VALUE_FIELD];
    r1 = M[r9 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.PEAK_VALUE_FIELD];

    do detect_mic_clipping;
        r0 = M[I0, MK1], r2 = M[I4, MK1];
        r0 = ABS r0; // Absolute value on DM1
        r7 = MAX r0; // Peak detect DM1
        r2 = ABS r2; // Absolute value on DM2
        r1 = MAX r2; // Peak detect DM2
        Null = r0 - r3;
        if GE r5 = 1;
        Null = r2 - r4;
        if GE r6 = 1;
    detect_mic_clipping:

    MB[r8 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.FRAME_DETECT_FIELD] = r5;
    MB[r9 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.FRAME_DETECT_FIELD] = r6;
    M[r8 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.PEAK_VALUE_FIELD] = r7;
    M[r9 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.PEAK_VALUE_FIELD] = r1;

    POP_ALL_C;
    r0 = 1;

    rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//   $aanc2_clipping.clipping_peak_detect_single
//
// DESCRIPTION:
//    AANC2 clipping and peak detection on a single input buffer in DM1
//
// INPUTS:
//    - r0 = Pointer to aanc2 clipping struct in dm1
//    - r1 = Number of samples to process
//
// OUTPUTS:
//    - r0 = Success/failure
//
// TRASHED REGISTERS:
//    C compliant
//
// NOTES:
//
// *****************************************************************************

.MODULE $M.aanc2_clipping.clipping_peak_detect_single;
    .CODESEGMENT PM;

$_aanc2_clipping_peak_detect_single:

    PUSH_ALL_C;

    r4 = r0;    // DM1 struct
    r10 = r1;   // Number of samples to process

    r5 = M[r4 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.THRESHOLD_FIELD];

    r0 = M[r4 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.P_DATA_FIELD];
    call $cbuffer.get_read_address_and_size_and_start_address;
    push r2;
    pop B0;
    I0 = r0;
    L0 = r1;

    r3 = 0;

    r1 = M[r4 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.PEAK_VALUE_FIELD];

    do detect_pb_clipping;
        r0 = M[I0, MK1];
        r0 = ABS r0;        // Absolute value
        r1 = MAX r0;        // Peak detect
        Null = r0 - r5;     // Threshold detect for clipping flag
        if GE r3 = 1;
    detect_pb_clipping:

    MB[r4 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.FRAME_DETECT_FIELD] = r3;
    M[r4 + $aanc2_clipping._AANC2_CLIP_DETECT_struct.PEAK_VALUE_FIELD] = r1;

    POP_ALL_C;
    r0 = 1;

    rts;

.ENDMODULE;
