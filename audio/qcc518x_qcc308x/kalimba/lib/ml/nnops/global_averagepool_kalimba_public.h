/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  global_averagepool_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for global_averagepool operator
 *
 */

#ifndef GLOBAL_AVERAGEPOOL_KALIMBA_PUBLIC_H
#define GLOBAL_AVERAGEPOOL_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* maximum input tensor count */
#define GAP_INPUT_TENSOR_COUNT 1
/* maximum output tensor count */
#define GAP_OUTPUT_TENSOR_COUNT 1
/* Compatible version information */
#define GAP_COMPATIBLE_VERSION 0
/* Operator version information*/
#define GAP_OPERATOR_VERSION 0

/* GAP operator specific parameters */
typedef struct gap_params {
    sat fract inv_len; /**< Inverse of number of elements per channel of input */
    int8 scale;        /**< Scale factor for layer */
} gap_params_t;

/**
 * \brief global_averagepool processing function
 *
 * \param kalimba layer_lite structure with input, output and gap params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int gap_forward(layer_lite *layer);

#endif /*GLOBAL_AVERAGEPOOL_KALIMBA_PUBLIC_H*/
