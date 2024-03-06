/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.                       *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#ifndef __VECTOR_OPS_FIXED_KALIMBA_H__
#define __VECTOR_OPS_FIXED_KALIMBA_H__
#include "utils_kalimba.h"

/**
 * @brief Function to calculate sigmoid of each element of a vector
 * @param out output vector
 * @param vec input vector
 * @param len length of vector
 */
int vector_sigmoid_kalimba(int32 *out, int32 *vec, int32 len);

/**
 * @brief Function to get exponential of input vector
 * @param out pointer to output data
 * @param vec pointer to input data
 * @param len number of elements required for operation
 * @param stride stride between two elements
 * @note All elements of input vector should be multiplied by -16  before calling this function
 */
int vector_exp_kalimba(int32 *out, int32 *vec, int32 len, uint32 stride);

/*
 * @brief Function to get exponential of the input data.
 * @param input data
 * @note  Input is assumed in the range [0, 1]. The output
 *        is in Q3.29.
 */
int exp_kalimba_positive(int input);

/**
 * @brief Function to calculate tanh of each element of a vector
 * @param out output vector
 * @param vec input vector
 * @param len length of vector
 */
int vector_tanh_kalimba(int32 *out, int32 *vec, int32 len);

#endif //__VECTOR_OPS_FIXED_KALIMBA_H__
