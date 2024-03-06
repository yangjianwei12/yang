/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.                       *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#include <stdfix.h>
#include <stdio.h>
#include "const_data/const_data.h"
#include "vector_ops_fixed_kalimba.h"
#include "exp_lut_kalimba.h"
#include "sigmoid_fixed_coef_kalimba.h"

/**Scaling factor for Tanh after Sigmoid operation*/
#define SCALE_SIGMOID_TO_TANH (-1)
/**Offset to minus after Sigmoid*/
#define TANH_OFFSET_FROM_SIGMOID 0.5r

int vector_sigmoid_kalimba(int32 *out, int32 *vec, int32 len) {
    sat fract __DM1 *input;
    sat fract __DM2 *output;

    input = (sat fract __DM1 *)vec;
    output = (sat fract __DM2 *)out;

    uint32 __DM2 *out_ptr = (uint32 __DM2 *)out;

#pragma loop minitercount(1)
    for (int i = 0; i < len; i++) {
        // Get sign and abs of input data
        int32 sign = (*(int32 *)(input)) >> SIGMOID_SIGN_SHIFT;
        *out_ptr = __builtin_abs(*(int32 *)(input++));

        /**
         * 32 bit fract data can be broken into
         * <1 sign bit>|<8 index bits>|<23 delta>
         */
        uint32 index = (*out_ptr >> SIGMOID_INDEX_SHIFT) & SIGMOID_INDEX_MASK;
        *out_ptr = *out_ptr & SIGMOID_DELTA_MASK;
        sat fract delta = *output;

        // Apply rounding to increase precision
        if (delta > SIGMOID_SPC_BY_2) {
            index++;
            delta = delta - SIGMOID_LUT_SPC;
        }
        // Multiply delta by log2(max_val) = 4
        delta = delta << SIGMOID_LUT_SCALING_FACTOR;

        /**
         * Read LUT and generate second order expression
         * sigmoid = lut[0] + lut[1] * delta + lut[2] * delta * delta
         */
        sat fract second = sigmoid_lut[index][2] * delta;
        sat fract first = (sigmoid_lut[index][1] + second) * delta;
        sat fract zero = sigmoid_lut[index][0] + first;

        fract sigmoid = zero;
        if (sign) {
            sigmoid = -sigmoid;
        }
        *output++ = sigmoid + SIGMOID_OFFSET;
        out_ptr++;
    }
    return ML_SUCCESS;
}

int vector_exp_kalimba(int32 *out, int32 *vec, int32 len, uint32 stride) {
    sat fract __DM1 *input;
    sat fract __DM2 *output;

    input = (sat fract __DM1 *)vec;
    output = (sat fract __DM2 *)out;

    stride--;
#pragma loop minitercount(1)
    for (int i = 0; i < len; i++) {
        int index = *(uint32 *)input >> EXP_INDEX_SHIFT;
        sat fract delta = __builtin_and_fract(*input, EXP_DELTA_MASK);
        input++;
        // Round data to increase precision
        if (delta > EXP_SPC_BY_2) {
            index++;
            delta = delta - EXP_SPC;
        }
        // Scale up by log base 2 of max_val(16)
        delta = delta << EXP_LUT_SCALING_FACTOR;

        // Get value from LUT
        sat fract exp_terms = exp_lut[index];

        // Calculate second order exponential
        exp_terms = exp_terms - exp_terms * delta + exp_terms * delta * (delta >> 1);
        *output++ = exp_terms;
        output = output + stride;
        input = input + stride;
    }
    return ML_SUCCESS;
}

int exp_kalimba_positive(int input)
{
    int index = input >> EXP_INDEX_SHIFT; /* Shift right by 22*/
    int delta = input & EXP_DELTA_MASK;
    if (delta > EXP_SPC_BY_2)
    {
        index++;
        delta -= __builtin_reinterpret_fract_to_int(EXP_SPC);
    }
    /* Get value from LUT */
    int exp_terms = exp_lut_positive[index];
    /* Calculate second order terms */
    sat fract data = __builtin_reinterpret_int_to_fract(exp_terms) - \
                     __builtin_reinterpret_int_to_fract(exp_terms) * __builtin_reinterpret_int_to_fract(delta) + \
                     __builtin_reinterpret_int_to_fract(exp_terms) * __builtin_reinterpret_int_to_fract(delta) * __builtin_reinterpret_int_to_fract(delta >> 1);
    return __builtin_reinterpret_fract_to_int(data);
}

int vector_tanh_kalimba(int32 *out, int32 *vec, int32 len) {

    sat fract *output = (sat fract *)out;

    // At this point input is already 2*x so as next step sigmoid is called on input.
    vector_sigmoid_kalimba(out, vec, len);

// Performing (sigmoid result-0.5)
#pragma loop minitercount(1)
    for (int i = 0; i < len; i++) {
        output[i] = output[i] - TANH_OFFSET_FROM_SIGMOID;
    }

    // Multiply the result by 2
    scale_inplace(output, len, SCALE_SIGMOID_TO_TANH);
    return ML_SUCCESS;
}
