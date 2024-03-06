/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  sqrt_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for sqrt operator
 *
 */

#ifndef SQRT_KALIMBA_PUBLIC_H
#define SQRT_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Number of input tensors for SQRT operator */
#define SQRT_INPUT_TENSOR_COUNT (1)
/* Number of output tensors for SQRT operator */
#define SQRT_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define SQRT_COMPATIBLE_VERSION 0
/* Operator version information*/
#define SQRT_OPERATOR_VERSION 0

typedef struct Sqrt {
    int8 scale;       /*!< scale factor for input tensor */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Sqrt;

/**
 * \brief sqrt processing function
 *
 * \param kalimba layer_lite structure with input, output and sqrt params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 *
 * \note Size of input and output tensors should be same 
 */
int sqrt_forward(layer_lite *layer);

#endif /*SQRT_KALIMBA_PUBLIC_H*/
