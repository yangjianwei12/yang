/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  maxout_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for maxout operator
 *
 */

#ifndef MAXOUT_KALIMBA_PUBLIC_H
#define MAXOUT_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Compatible version information */
#define MAXOUT_COMPATIBLE_VERSION 0
/* Operator version information*/
#define MAXOUT_OPERATOR_VERSION 0

/* Maxout operator specific structure */
typedef struct Maxout {
    uint8 is_transpose; /*!< Input/Output tensor transpose shape indicator for maxout */
    int8 scale;         /*!< Scale factor for input Tensor */
} Maxout;

/**
 * \brief maxout processing function
 *
 * \param kalimba layer_lite structure with input, output and maxout params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int maxout_forward(layer_lite *layer);

#endif /*MAXOUT_KALIMBA_PUBLIC_H*/
