/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  div_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for div operator
 *
 */

#ifndef DIV_KALIMBA_PUBLIC_H
#define DIV_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* maximum input tensor count */
#define DIV_INPUT_TENSOR_COUNT (2)
/* maximum output tensor count */
#define DIV_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define DIV_COMPATIBLE_VERSION 0
/* Operator version information*/
#define DIV_OPERATOR_VERSION 0

/* Division operator specific structure */
typedef struct Division {
    int8 scale_A;     /*!< scale factor for input A tensor */
    int8 scale_B;     /*!< scale factor for input B tensor */
    int8 kernel;      /*!< Kernel to be used for division */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Division;

/**
 * \brief div processing function
 *
 * \param kalimba layer_lite structure with input, output and div params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int div_forward(layer_lite *layer);

#endif /*DIV_KALIMBA_PUBLIC_H*/
