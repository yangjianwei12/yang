/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  concat_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for concat operator
 *
 */

#ifndef CONCAT_KALIMBA_PUBLIC_H
#define CONCAT_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* min input tensor count */
#define CONCAT_INPUT_TENSOR_COUNT_MIN (2)
/* max input tensor count */
#define CONCAT_INPUT_TENSOR_COUNT_MAX (8)
/* output tensor count */
#define CONCAT_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define CONCAT_COMPATIBLE_VERSION 0
/* Operator version information*/
#define CONCAT_OPERATOR_VERSION 0

/* Concat operator specific structure */
typedef struct Concat {
    int8 scale[CONCAT_INPUT_TENSOR_COUNT_MAX]; /*!< scale factor for input tensors */
    uint8 axis;
} Concat;

/**
 * \brief concat processing function
 *
 * \param kalimba layer_lite structure with input, output and concat params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int concat_forward(layer_lite *layer);

#endif /*CONCAT_KALIMBA_PUBLIC_H*/
