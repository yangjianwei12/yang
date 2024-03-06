/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  logsoftmax_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for logsoftmax operator
 *
 */

#ifndef LOGSOFTMAX_KALIMBA_PUBLIC_H
#define LOGSOFTMAX_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* input tensor count */
#define LOG_SOFTMAX_INPUT_TENSOR_COUNT 1
/* output tensor count */
#define LOG_SOFTMAX_OUTPUT_TENSOR_COUNT 1
/* Compatible version information */
#define LOG_SOFTMAX_COMPATIBLE_VERSION 0
/* Operator version information*/
#define LOG_SOFTMAX_OPERATOR_VERSION 0

/* log softmax operator data structure
 * For performing log softmax on all operators, use axis = number of dimensions in input tensor
 */
typedef struct logsoftmax_params {
    int8 scale;       /**< Scale factor for layer */
    int8 axis;        /**< Axis to perform softmax operation */
    int8 beta;        /**< Scale factor for operator */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} logsoftmax_params_t;

/**
 * \brief logsoftmax processing function
 *
 * \param kalimba layer_lite structure with input, output and logsoftmax params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int logsoftmax_forward(layer_lite *layer);

#endif /*LOGSOFTMAX_KALIMBA_PUBLIC_H*/
