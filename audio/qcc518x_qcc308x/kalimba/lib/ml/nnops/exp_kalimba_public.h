/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  exp_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for exp operator
 *
 */

#ifndef EXP_KALIMBA_PUBLIC_H
#define EXP_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* maximum input tensor count */
#define EXP_INPUT_TENSOR_COUNT 1
/* maximum output tensor count */
#define EXP_OUTPUT_TENSOR_COUNT 1
/* Compatible version information */
#define EXP_COMPATIBLE_VERSION 0
/* Operator version information*/
#define EXP_OPERATOR_VERSION 0

/*Exp operator specific structure */
typedef struct Exp {
    int8 scale; /*!< scale factor for input tensor */
} Exp;

/**
 * \brief exp processing function
 *
 * \param kalimba layer_lite structure with input, output and exp params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int exp_forward(layer_lite *layer);

#endif /*EXP_KALIMBA_PUBLIC_H*/
