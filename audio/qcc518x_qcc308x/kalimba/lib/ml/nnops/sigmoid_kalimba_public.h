/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  sigmoid_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for sigmoid operator
 *
 */

#ifndef SIGMOID_KALIMBA_PUBLIC_H
#define SIGMOID_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* number of input tensors */
#define SIGMOID_INPUT_TENSOR_COUNT 1
/* number of output tensors */
#define SIGMOID_OUTPUT_TENSOR_COUNT 1
/* Compatible version information */
#define SIGMOID_COMPATIBLE_VERSION 0
/* Operator version information*/
#define SIGMOID_OPERATOR_VERSION 0

/* Sigmoid operator specific structure */
typedef struct Sigmoid {
    int8 scale;       /**< Scale factor for input */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Sigmoid;

/**
 * \brief sigmoid processing function
 *
 * \param kalimba layer_lite structure with input, output and sigmoid params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int sigmoid_forward(layer_lite *layer);

#endif /*SIGMOID_KALIMBA_PUBLIC_H*/
