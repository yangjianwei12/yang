/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  pow_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for pow operator
 *
 */

#ifndef POW_KALIMBA_PUBLIC_H
#define POW_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* input tensor count */
#define POW_INPUT_TENSOR_COUNT (2)
/* output tensor count */
#define POW_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define POW_COMPATIBLE_VERSION 0
/* Operator version information*/
#define POW_OPERATOR_VERSION 0

/* Pow operator specific structure */
typedef struct Pow {
    int8 scale_a;     /*!< scale factor for input A tensor */
    int8 scale_b;     /*!< scale factor for input B tensor */
    uint8 kernel;     /*!< Kernel to be used for pow operation */
    int8 beta_a;      /*!< beta value for input A tensor */
    int8 beta_b;      /*!< beta value for input B tensor */
    int8 beta_z;      /*!< beta value for output tensor */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
    uint8 vs_scalar;  /*!< scalar value of tensor b if it is a whole number */
} Pow;

/**
 * \brief pow processing function
 *
 * \param kalimba layer_lite structure with input, output and pow params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int pow_forward(layer_lite *layer);

#endif /*POW_KALIMBA_PUBLIC_H*/
