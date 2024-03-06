/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  tanh_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for tanh operator
 *
 */

#ifndef TANH_KALIMBA_PUBLIC_H
#define TANH_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* input tensor count */
#define TANH_INPUT_TENSOR_COUNT 1
/* output tensor count */
#define TANH_OUTPUT_TENSOR_COUNT 1
/* Compatible version information */
#define TANH_COMPATIBLE_VERSION 0
/* Operator version information*/
#define TANH_OPERATOR_VERSION 0

/* Tanh operator specific structure */
typedef struct Tanh {
    int8 scale;       /*!< scale factor for input tensor */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Tanh;

/**
 * \brief tanh processing function
 *
 * \param kalimba layer_lite structure with input, output and tanh params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int tanh_forward(layer_lite *layer);

#endif /*TANH_KALIMBA_PUBLIC_H*/
