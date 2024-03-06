/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  subtract_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for subtract operator
 *
 */

#ifndef SUBTRACT_KALIMBA_PUBLIC_H
#define SUBTRACT_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Number of input tensors for subtract operator */
#define SUBTRACT_INPUT_TENSOR_COUNT (2)
/* Number of output tensors for subtract operator */
#define SUBTRACT_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define SUBTRACT_COMPATIBLE_VERSION 0
/* Operator version information*/
#define SUBTRACT_OPERATOR_VERSION 0

/* Subtract operator specific structure */
typedef struct Subtract {
    int8 scale_A;     /*!< scale factor for input A tensor */
    int8 scale_B;     /*!< scale factor for input B tensor */
    int8 kernel;      /*!< Kernel to be used for subtraction */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Subtract;

/**
 * \brief subtract processing function
 *
 * \param kalimba layer_lite structure with input, output and subtract params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int subtract_forward(layer_lite *layer);

#endif /*SUBTRACT_KALIMBA_PUBLIC_H*/
