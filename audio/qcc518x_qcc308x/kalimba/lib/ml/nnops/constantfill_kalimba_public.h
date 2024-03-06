/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  constantfill_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for constantfill operator
 *
 */

#ifndef CONSTANTFILL_KALIMBA_PUBLIC_H
#define CONSTANTFILL_KALIMBA_PUBLIC_H

#include <stdfix.h>
#include "layer_lite_kalimba.h"

/* maximum input tensor count */
#define CONSTANT_FILL_INPUT_TENSOR_COUNT (1)
/* maximum output tensor count */
#define CONSTANT_FILL_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define CONSTANT_FILL_COMPATIBLE_VERSION 0
/* Operator version information*/
#define CONSTANT_FILL_OPERATOR_VERSION 0

/* Constant fill operator specific structure */
typedef struct ConstantFill {
    sat fract const_value; /* value to fill */
    int8 scale;            /* scale factor for input A tensor */
    int8 is_inplace;       /*!< flag to indicate inplace operation */
} ConstantFill;

/**
 * \brief constantfill processing function
 *
 * \param kalimba layer_lite structure with input, output and constantfill params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int constantfill_forward(layer_lite *layer);

#endif /*CONSTANTFILL_KALIMBA_PUBLIC_H*/
