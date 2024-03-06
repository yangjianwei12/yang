/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
#include "portability_macros.h"
#include "stack.h"

#define $ahm_gain.LOG2_TO_DB_CONV_FACTOR           Qfmt_(6.020599913279624, 5)
#define $ahm_gain.DB_TO_LOG2_CONV_FACTOR           Qfmt_(0.166096404744368, 12)
#define $ahm_gain.COARSE_GAIN_TO_DB                6
#define $ahm_gain.FINE_GAIN_LOG2_SHIFT_AMT         24
#define $ahm_gain.COARSE_GAIN_DB_SHIFT_AMT         20
#define $ahm_gain.MAKEUP_GAIN_TO_Q8                -6
#define $ahm_gain.MAKEUP_GAIN_TO_Q8_ROUND          0x20 // 1 << 6-1
#define $ahm_gain.SUBTRACT_1_Q8                    Qfmt_(1.0, 8)
// #define $ahm_gain.SUBTRACT_1_Q8                    0x01000000

// pow2 returns Q1.31. Scale to 128 and double to compensate for -6dB
#define $ahm_gain.NORMALIZED_TO_GAIN_STEP          -23
#define $ahm_gain.NORMALIZED_TO_GAIN_STEP_ROUND    0x400000 // 1 << (23-1)

// *****************************************************************************
// MODULE:
//   $ahm_gain.calc_gain_db
//
// DESCRIPTION:
//    Calculate dB representation of gain to be reported as statistic
//    Formula used : Gain (dB) = 6*coarse_gain + 20*log10(fine_gain/128)
//
// INPUTS:
//    - r0 = fine gain (uint16)
//    - r1 = coarse gain (int16)
//
// OUTPUTS:
//    - r0 = gain value in dB in Q12.20
//
// TRASHED REGISTERS:
//    C compliant
//
// NOTES: If fine gain is 0, gain value returned is INT_MIN
//
// *****************************************************************************

.MODULE $M.ahm_gain.calc_gain_db;
    .CODESEGMENT PM;

$_ahm_calc_gain_db:

    pushm <r6, rLink>;

    Null = r0;
    if Z jump return_early;

calc_fine_gain_db:
    rMAC = r0; // Copy fine gain to rMAC, in Q40.32
    r3 = r1 * $ahm_gain.COARSE_GAIN_TO_DB (int); // Store coarse gain dB value in r3

    // Fine gain measured relative to 128, so needs to be scaled by 2^-7
    // log2_table takes input (rMAC) in Q9.63, so shift rMAC by 63-32-7=24
    rMAC = rMAC ASHIFT $ahm_gain.FINE_GAIN_LOG2_SHIFT_AMT (72bit);
    r3 = r3 ASHIFT $ahm_gain.COARSE_GAIN_DB_SHIFT_AMT; // Coarse gain (dB) in Q12.20

    call $math.log2_table;

    // Convert to dB by using multiplying factor of 20/log2(10)
    rMAC = r0 * $ahm_gain.LOG2_TO_DB_CONV_FACTOR;
    r0 = rMAC + r3; // Add coarse gain (dB) and fine gain (dB)
    jump return_db_gain;

return_early:
    r0 = MININT;

return_db_gain:
    popm <r6, rLink>;

    rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//   $ahm_gain.convert_db_to_log2
//
// DESCRIPTION:
//    Convert gain in dB to log2 representation.
//
// INPUTS:
//    - r0 = gain in dB (Q12.20)
//
// OUTPUTS:
//    - r0 = gain in log2 (Q12.20)
//
// TRASHED REGISTERS:
//    C compliant
//
// *****************************************************************************

.MODULE $M.ahm_gain.convert_db_to_log2;
    .CODESEGMENT PM;

$_ahm_convert_db_to_log2:

    rMAC = r0 * $ahm_gain.DB_TO_LOG2_CONV_FACTOR;
    // Q12.20 * Q12.20 = Q24.40
    // Automatically get a left shift by 1 bit for 63 fractional bits (Q23.41)
    // Left shift by 11 recovers to Q12
    rMAC = rMAC ASHIFT 11 (72bit);

    r0 = rMAC;
    rts;

.ENDMODULE;
