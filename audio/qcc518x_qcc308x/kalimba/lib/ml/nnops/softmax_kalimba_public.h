/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  softmax_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for softmax operator
 *
 */

#ifndef SOFTMAX_KALIMBA_PUBLIC_H
#define SOFTMAX_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Number of input tensors for softmax operator */
#define SOFTMAX_INPUT_TENSOR_COUNT 1
/* Number of output tensors for softmax operator */
#define SOFTMAX_OUTPUT_TENSOR_COUNT 1
/* Compatible version information */
#define SOFTMAX_COMPATIBLE_VERSION 0
/* Operator version information*/
#define SOFTMAX_OPERATOR_VERSION 0

/* Softmax operator specific parameters
 * For performing softmax on all operators, use axis = number of dimensions in input tensor
 */
typedef struct softmax_params {
    int8 scale;       /**< Scale factor for layer */
    int8 axis;        /**< Axis to perform softmax operation */
    int8 beta;        /**< Scale factor for operator */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} softmax_params_t;

/**
 * \brief softmax processing function
 *
 * \param kalimba layer_lite structure with input, output and softmax params 
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int softmax_forward(layer_lite *layer);

#endif /*SOFTMAX_KALIMBA_PUBLIC_H*/
