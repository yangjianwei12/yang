/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  mul_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for mul operator
 *
 */

#ifndef MUL_KALIMBA_PUBLIC_H
#define MUL_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* input tensor count */
#define MUL_INPUT_TENSOR_COUNT (2)
/* output tensor count */
#define MUL_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define MUL_COMPATIBLE_VERSION 0
/* Operator version information*/
#define MUL_OPERATOR_VERSION 0

/* Multiply operator specific structure */
typedef struct Multiply {
    int8 scale_A;     /*!< scale factor for input A tensor */
    int8 scale_B;     /*!< scale factor for input B tensor */
    int8 kernel;      /*!< Kernel to be used for subtraction */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Multiply;

/**
 * \brief mul processing function
 *
 * \param kalimba layer_lite structure with input, output and mul params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int mul_forward(layer_lite *layer);

#endif /*MUL_KALIMBA_PUBLIC_H*/
