/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  add_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for add operator
 *
 */

#ifndef ADD_KALIMBA_PUBLIC_H
#define ADD_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Input tensor count for add operator */
#define ADD_INPUT_TENSOR_COUNT  2
/* Output tensot count for add operator */
#define ADD_OUTPUT_TENSOR_COUNT 2
/* Compatible version information */
#define ADD_COMPATIBLE_VERSION 0
/* Operator version information*/
#define ADD_OPERATOR_VERSION 0

/* Add operator specific structure */
typedef struct Add {
    int8 scale_A;     /* scale factor for input A tensor */
    int8 scale_B;     /* scale factor for input B tensor */
    int8 kernel;      /* Kernel to be used for addition  */
    uint8 is_inplace; /* Flag to indicate if the operation is inplace */
} Add;

/**
 *
 * \brief add processing function
 * \param kalimba layer_lite structure with input, output and add params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int add_forward(layer_lite *layer);

#endif /*ADD_KALIMBA_PUBLIC_H*/
