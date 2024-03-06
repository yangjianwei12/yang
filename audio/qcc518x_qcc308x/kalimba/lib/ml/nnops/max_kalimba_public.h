/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  max_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for max operator
 *
 */

#ifndef MAX_KALIMBA_PUBLIC_H
#define MAX_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* min input tensor count */
#define MAX_INPUT_TENSOR_COUNT_MIN (2)
/* max input tensor count */
#define MAX_INPUT_TENSOR_COUNT_MAX (8)
/* output tensor count */
#define MAX_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define MAX_COMPATIBLE_VERSION 0
/* Operator version information*/
#define MAX_OPERATOR_VERSION 0

/* max operator specific structure */
typedef struct Max {
    int8 scale[MAX_INPUT_TENSOR_COUNT_MAX]; /*!< scale factor for input tensor */
    uint8 is_inplace;                       /*!< Flag to indicate if the operation is inplace */
} Max;

/**
 * \brief max processing function
 *
 * \param kalimba layer_lite structure with input, output and max params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int max_forward(layer_lite *layer);

#endif /*MAX_KALIMBA_PUBLIC_H*/
