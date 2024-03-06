/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
#include "portability_macros.h"
#include "stack.h"

/* Ramp duration calculation is Q12.20 but after multiplication needs to be
 * shifted to Q32.
 */
#define $anc_hw_manager_ramp.AHM_DURATION_SHIFT  11
#define $anc_hw_manager_ramp.AHM_TC_APPROX_SHIFT  2
// *****************************************************************************
// MODULE:
//   $anc_hw_manager_ramp.calc_duration_samples
//
// DESCRIPTION:
//    Calculate a timer duration in samples
//
// INPUTS:
//    - r0 = Duration of the ramp in seconds (Q12.20)
//    - r1 = Sample rate
//
// OUTPUTS:
//    - r0 = Duration of the ramp in samples
//
// TRASHED REGISTERS:
//    C compliant
//
// NOTES:
//
// *****************************************************************************

.MODULE $M.anc_hw_manager_ramp.calc_duration_samples;
    .CODESEGMENT PM;

$_ahm_calc_duration_samples:
    rMAC = r0 * r1;
    r0 = rMAC ASHIFT $anc_hw_manager_ramp.AHM_DURATION_SHIFT;

    rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//   $anc_hw_manager_ramp.calc_ramp_tc
//
// DESCRIPTION:
//    Calculate the time constant for a timer
//
// INPUTS:
//    - r0 = Duration of the ramp in seconds (Q12.20)
//    - r1 = Ramp rate
//
// OUTPUTS:
//    - r0 = time constant (Q1.31)
//
// TRASHED REGISTERS:
//    C compliant
//
// NOTES:
//
// *****************************************************************************

.MODULE $M.anc_hw_manager_ramp.calc_ramp_tc;
    .CODESEGMENT PM;

$_ahm_calc_ramp_tc:

    PUSH_ALL_C
    // Setup the inputs to the standard exp calculation. This gives the time
    // constant corresponding
    // r0: t (seconds, Q7.N)
    // r1: L (frame size)
    // r2: fs (sample rate)

    r2 = r1;
    r1 = 1; // Frame size effectively handled via sample rate input

    // calculating alpha based on frame size (r1) and sampling rate (r2)

    // 1/t
    r6 = SIGNDET r0;
    r0 = r0 ASHIFT r6;
    rMAC = 0.25;
    div = rMAC/r0;

    // t in Q12.20      (r6 = r6 - 12;)
    // div guard bit    (r6 = r6 + 1;)
    // Q8.24 pow2 input (r6 = r6 - 7;)
    // r6 = r6 + (-12 + 1 - 7)
    r6 = r6 - 18;

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

    // alfa_c = exp(-1/(t*(fs/L)))
    r3 = r0;
    // alfa = 1 - exp(-1/(t*(fs/L)))
    r1 = 1.0 - r3;

    POP_ALL_C

    // Multiply by 4 approximates the true settling time
    r0 = r1 ASHIFT $anc_hw_manager_ramp.AHM_TC_APPROX_SHIFT;

    rts;

.ENDMODULE;