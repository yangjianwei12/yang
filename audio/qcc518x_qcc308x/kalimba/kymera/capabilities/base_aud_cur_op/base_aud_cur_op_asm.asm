/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
#include "portability_macros.h"
#include "stack.h"

#define $aud_cur.FINE_GAIN_LOG2_SHIFT_AMT         24
#define $aud_cur.MAKEUP_GAIN_TO_Q8                -6
#define $aud_cur.MAKEUP_GAIN_TO_Q8_ROUND          0x20 // 1 << 6-1
#define $aud_cur.SUBTRACT_1_Q8                    Qfmt_(1.0, 8)
// pow2 returns Q1.31. Scale to 128 and double to compensate for -6dB
#define $aud_cur.NORMALIZED_TO_GAIN_STEP          -23
#define $aud_cur.NORMALIZED_TO_GAIN_STEP_ROUND    0x400000 // 1 << (23-1)

// *****************************************************************************
// MODULE:
//   $aud_cur.calc_target_gain
//
// DESCRIPTION:
//    Calculate the adjusted fine gain based on a static gain and dB gain
//    value.
//
// INPUTS:
//    - r0 = fine gain (uint16, ANC HW steps)
//    - r1 = dB gain value to adjust static gain (int, log2, Q2.N)
//
// OUTPUTS:
//    - r0 = adjusted fine gain
//
// TRASHED REGISTERS:
//    C compliant
//
// NOTES:
//    Algorithm sequence:
//    1. Convert fine gain to log2 value
//    2. Apply makeup gain
//    3. Convert back to fine gain
//
// *****************************************************************************

.MODULE $M.aud_cur.calc_adjusted_gain;
    .CODESEGMENT PM;

$_aud_cur_calc_adjusted_gain:

    // Round makeup gain before converting to Q8.24
    // Formula: gain = (gain + (1 << (shift-1))) >> shift
    r1 = r1 + $aud_cur.MAKEUP_GAIN_TO_Q8_ROUND;
    // Convert makeup gain to Q8.24
    r2 = r1 ASHIFT $aud_cur.MAKEUP_GAIN_TO_Q8;
    // If makeup gain is 0 then r0 is returned untouched
    if Z jump calc_target_gain_rts;

    pushm <r6, r7, rLink>;

    rMAC = r0; // Copy fine gain to rMAC, in Q40.32
    // Fine gain measured relative to 128, so needs to be scaled by 2^-7
    // log2_table takes input (rMAC) in Q9.63, so shift rMAC by 63-32-7=24
    rMAC = rMAC ASHIFT $aud_cur.FINE_GAIN_LOG2_SHIFT_AMT (72bit);

    // log2_table trashes rMAC, r0, r1, r6
    call $math.log2_table;
    // r0 is now log2 in Q8.24

    // Add makeup gain to static gain
    r0 = r0 + r2;

    // subtract (log10) 6dB to make sure the total gain is < 0
    r0 = r0 - $aud_cur.SUBTRACT_1_Q8;

    // Convert back to normal value
    call $math.pow2_table;

    // round r0 before converting to gain step value.
    r0 = r0 + $aud_cur.NORMALIZED_TO_GAIN_STEP_ROUND;

    // r0 now has the normalized value
    r0 = r0 ASHIFT $aud_cur.NORMALIZED_TO_GAIN_STEP;

    // Upper bounds check is provided in the pow2_table function and limits
    // the returned value to just less than 2.

    popm <r6, r7, rLink>;
calc_target_gain_rts:

    rts;

.ENDMODULE;