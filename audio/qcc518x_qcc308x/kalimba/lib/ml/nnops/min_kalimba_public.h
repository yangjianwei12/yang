/****************************************************************************
* Copyright (c) 2023 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  min_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for min operator
 *
 */

#ifndef MIN_KALIMBA_PUBLIC_H
#define MIN_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"
/* Minimum number of input tensors for min operator.*/
#define MIN_INPUT_TENSOR_COUNT_MIN (2)
/* Maximum number of input tensors for min operator.*/
#define MIN_INPUT_TENSOR_COUNT_MAX (8)
/* Number of output tensors for min operator.*/
#define MIN_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define MIN_COMPATIBLE_VERSION 2
/* Operator version information*/
#define MIN_OPERATOR_VERSION 0

/* min operator specific structure */
typedef struct Min {
    int8 scale[MIN_INPUT_TENSOR_COUNT_MAX]; /*!< scale factor for input tensor */
    uint8 is_inplace;                       /*!< Flag to indicate if the operation is inplace */
} Min;

/**
 * \brief min processing function
 *
 * \param kalimba layer_lite structure with input, output and min params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int min_forward(layer_lite *layer);
#endif /*MIN_KALIMBA_PUBLIC_H*/
