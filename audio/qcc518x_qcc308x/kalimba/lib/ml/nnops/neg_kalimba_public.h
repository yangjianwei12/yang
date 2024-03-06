/****************************************************************************
* Copyright (c) 2023 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  neg_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for neg operator
 *
 */

#ifndef NEG_KALIMBA_PUBLIC_H
#define NEG_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* input tensor count */
#define NEG_INPUT_TENSOR_COUNT (1)
/* output tensor count */
#define NEG_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define NEG_COMPATIBLE_VERSION 2
/* Operator version information*/
#define NEG_OPERATOR_VERSION 0

typedef struct Neg {
    int8 scale;          /*!< scale factor for input tensor */
    uint8 is_inplace;    /*!< Flag to indicate if the operation is inplace */
} Neg;

/**
 * \brief neg processing function
 *
 * \param kalimba layer_lite structure with input, output and neg params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int neg_forward(layer_lite *layer);

#endif /*NEG_KALIMBA_PUBLIC_H*/
