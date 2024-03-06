/****************************************************************************
* Copyright (c) 2023 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  prelu_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for prelu operator
 *
 */

#ifndef PRELU_KALIMBA_PUBLIC_H
#define PRELU_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* input tensor count */
#define PRELU_INPUT_TENSOR_COUNT (2)
/* output tensor count */
#define PRELU_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define PRELU_COMPATIBLE_VERSION 1
/* Operator version information*/
#define PRELU_OPERATOR_VERSION 0

typedef struct PRelu {
    int8 scale_in;       /*!< scale factor for input tensor */
    int8 scale_slp;      /*!< scale factor for slope tensor */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
    uint8 kernel;      /*!< Kernel to be used for multiplication */
} PRelu;

/**
 * \brief prelu processing function
 *
 * \param kalimba layer_lite structure with input, output and prelu params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int prelu_forward(layer_lite *layer);

#endif /*PRELU_KALIMBA_PUBLIC_H*/
