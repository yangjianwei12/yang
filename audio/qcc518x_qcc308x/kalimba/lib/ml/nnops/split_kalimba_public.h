/****************************************************************************
* Copyright (c) 2023 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  split_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for split operator
 *
 */

#ifndef SPLIT_KALIMBA_PUBLIC_H
#define SPLIT_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* input tensor count */
#define SPLIT_INPUT_TENSOR_COUNT (1)
/* min output tensor count */
#define SPLIT_OUTPUT_TENSOR_COUNT_MIN (1)
/* max output tensor count */
#define SPLIT_OUTPUT_TENSOR_COUNT_MAX (10)
/* Compatible version information */
#define SPLIT_COMPATIBLE_VERSION 2
/* Operator version information*/
#define SPLIT_OPERATOR_VERSION 0

/* Split operator specific structure */
typedef struct Split {
    int8 scale; /*!< scale factor for input tensor */
    uint8 axis; /*!< axis along which split happens */
    uint16 reserved; /*!< reserved variable to make the param structure word aligned */
    uint32 split_size[SPLIT_OUTPUT_TENSOR_COUNT_MAX]; /*!< list of ints indicating the length of each output tensor along axis */
} Split;

/**
 * \brief split processing function
 *
 * \param kalimba layer_lite structure with input, output and split params
 *
 * \return ML_SUCCESS if successful, else ML_FAIL
 */
int split_forward(layer_lite *layer);

#endif /*SPLIT_KALIMBA_PUBLIC_H*/
