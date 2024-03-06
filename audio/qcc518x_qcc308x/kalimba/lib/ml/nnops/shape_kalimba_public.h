/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  shape_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for shape operator
 *
 */

#ifndef SHAPE_KALIMBA_PUBLIC_H
#define SHAPE_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* Number of input tensors for shape operator */
#define SHAPE_INPUT_TENSOR_COUNT (1)
/* Number of output tensors for shape operator */
#define SHAPE_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define SHAPE_COMPATIBLE_VERSION 0
/* Operator version information*/
#define SHAPE_OPERATOR_VERSION 0

/* shape operator specific structure */
typedef struct {
    int8 scale; /*!< Scale factor for input tensor */
} shape_params_t;

/**
 * \brief shape processing function
 *
 * \param kalimba layer_lite structure with input, output and shape params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int shape_forward(layer_lite *layer);

#endif /*SHAPE_KALIMBA_PUBLIC_H*/
