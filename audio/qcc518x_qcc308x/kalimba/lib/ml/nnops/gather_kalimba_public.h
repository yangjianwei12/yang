/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  gather_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for gather operator
 *
 */

#ifndef GATHER_KALIMBA_PUBLIC_H
#define GATHER_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* maximum input tensor count */
#define GATHER_INPUT_TENSOR_COUNT (2)
/* maximum output tensor count */
#define GATHER_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define GATHER_COMPATIBLE_VERSION 0
/* Operator version information*/
#define GATHER_OPERATOR_VERSION 0

/* Gather operator specific structure */
typedef struct Gather {
    int8 scale_A; /*!< scale factor for input A tensor */
    int8 axis;    /*!< axis in Tensor A to gather indices from */
} Gather;

/**
 * \brief gather processing function
 *
 * \param kalimba layer_lite structure with input, output and gather params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int gather_forward(layer_lite *layer);

#endif /*GATHER_KALIMBA_PUBLIC_H*/
