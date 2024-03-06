/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  squeeze_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for squeeze operator
 *
 */

#ifndef SQUEEZE_KALIMBA_PUBLIC_H
#define SQUEEZE_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Number of input tensors for squeeze operator */
#define SQUEEZE_INPUT_TENSOR_COUNT (1)
/* Number of output tensors for squeeze operator */
#define SQUEEZE_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define SQUEEZE_COMPATIBLE_VERSION 0
/* Operator version information*/
#define SQUEEZE_OPERATOR_VERSION 0

/* Squeeze operator specific parameters
 * For performing squeeze operation on input tensor in case of inplace operation
 */
typedef struct Squeeze {
    int8 scale;       /*!< scale factor for input tensor */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Squeeze;

/**
 * \brief squeeze processing function
 *
 * \param kalimba layer_lite structure with input, output and squeeze params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int squeeze_forward(layer_lite *layer);

#endif /*SQUEEZE_KALIMBA_PUBLIC_H*/
