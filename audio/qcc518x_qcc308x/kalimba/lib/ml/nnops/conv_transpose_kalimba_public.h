/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  conv_transpose_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for conv_transpose operator
 *
 */

#ifndef CONV_TRANSPOSE_KALIMBA_PUBLIC_H
#define CONV_TRANSPOSE_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* maximum input tensor count */
#define CONV_TRANSPOSE_INPUT_TENSOR_COUNT (4)
/* maximum output tensor count */
#define CONV_TRANSPOSE_OUTPUT_TENSOR_COUNT (1)
/* max dimensions */
#define CONV_TRANSPOSE_MAX_DIMS (3)
/* Compatible version information */
#define CONV_TRANSPOSE_COMPATIBLE_VERSION 0
/* Operator version information*/
#define CONV_TRANSPOSE_OPERATOR_VERSION 0

/* ConvTranspose operator specific structure */
typedef struct ConvTranspose {
    uint8 kernel[CONV_TRANSPOSE_MAX_DIMS];        /*!< kernel dimensions */
    uint8 stride[CONV_TRANSPOSE_MAX_DIMS];        /*!< strides dimension */
    uint8 dilation[CONV_TRANSPOSE_MAX_DIMS];      /*!< dilation dimensions */
    uint8 padding_start[CONV_TRANSPOSE_MAX_DIMS]; /*!< padding dimensions */
    int8 scale;                                   /*!< scale factor for input tensor */
} ConvTranspose;

/**
 * \brief conv_transpose processing function
 *
 * \param kalimba layer_lite structure with input, output and convtranspose params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int conv_transpose_forward(layer_lite *layer);

#endif /*CONV_TRANSPOSE_KALIMBA_PUBLIC_H*/
