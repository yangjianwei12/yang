/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  relu_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for relu operator
 *
 */

#ifndef RELU_KALIMBA_PUBLIC_H
#define RELU_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* input tensor count */
#define RELU_INPUT_TENSOR_COUNT (1)
/* output tensor count */
#define RELU_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define RELU_COMPATIBLE_VERSION 0
/* Operator version information*/
#define RELU_OPERATOR_VERSION 0

typedef struct Relu {
    int8 scale;       /*!< scale factor for input tensor */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Relu;

/**
 * \brief relu processing function
 *
 * \param kalimba layer_lite structure with input, output and relu params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int relu_forward(layer_lite *layer);

#endif /*RELU_KALIMBA_PUBLIC_H*/
