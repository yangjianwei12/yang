/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  unsqueeze_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for unsqueeze operator
 *
 */

#ifndef UNSQUEEZE_KALIMBA_PUBLIC_H
#define UNSQUEEZE_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* Number of input tensors for unsqueeze operator */
#define UNSQUEEZE_INPUT_TENSOR_COUNT (1)
/* Number of output tensors for unsqueeze operator */
#define UNSQUEEZE_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define UNSQUEEZE_COMPATIBLE_VERSION 0
/* Operator version information*/
#define UNSQUEEZE_OPERATOR_VERSION 0

/* Unsqueeze operator specific parameters
 * For performing unsqueeze operation on input tensor in case of inplace operation
 */
typedef struct Unsqueeze {
    int8 scale;       /*!< scale factor for input tensor */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Unsqueeze;

/**
 * \brief unsqueeze processing function
 *
 * \param kalimba layer_lite structure with input, output and unsqueeze params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int unsqueeze_forward(layer_lite *layer);

#endif /*UNSQUEEZE_KALIMBA_PUBLIC_H*/
