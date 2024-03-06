/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  maxpool_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for maxpool operator
 *
 */

#ifndef MAXPOOL_KALIMBA_PUBLIC_H
#define MAXPOOL_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Compatible version information */
#define MAXPOOL_COMPATIBLE_VERSION 0
/* Operator version information*/
#define MAXPOOL_OPERATOR_VERSION 0

/* Maxpool operator specific structure */
typedef struct MaxPool {
    uint8 dims;
    uint8 kernel_c;
    uint8 kernel_h;
    uint8 kernel_w;
    uint8 stride_c;
    uint8 stride_h;
    uint8 stride_w;
    uint8 dilation_c;
    uint8 dilation_h;
    uint8 dilation_w;
    int8 scale;
} MaxPool;

/**
 * \brief maxpool processing function
 *
 * \param kalimba layer_lite structure with input, output and maxpool params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int maxpool_forward(layer_lite *layer);

#endif /*MAXPOOL_KALIMBA_PUBLIC_H*/
