/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  reshape_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for reshape operator
 *
 */

#ifndef RESHAPE_KALIMBA_PUBLIC_H
#define RESHAPE_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Number of input tensors for reshape operator */
#define RESHAPE_INPUT_TENSOR_COUNT_MIN (1)
/* Number of optional input tensors for reshape operator */
#define RESHAPE_INPUT_TENSOR_COUNT_MAX (2)
/*  Number of output tensors for reshape operator */
#define RESHAPE_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define RESHAPE_COMPATIBLE_VERSION 0
/* Operator version information*/
#define RESHAPE_OPERATOR_VERSION 0

typedef struct Reshape {
    int8 scale;       /*!< scale factor for input tensor */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Reshape;

/**
 * \brief reshape processing function
 *
 *\param kalimba layer_lite structure with input, output and reshape params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int reshape_forward(layer_lite *layer);

#endif /*RESHAPE_KALIMBA_PUBLIC_H*/
