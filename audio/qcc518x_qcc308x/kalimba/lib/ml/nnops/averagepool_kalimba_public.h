/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  averagepool_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for averagepool operator
 *
 */

#ifndef AVERAGEPOOL_KALIMBA_PUBLIC_H
#define AVERAGEPOOL_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

#define AVERAGE_POOL_INPUT_TENSOR_COUNT (1)
#define AVERAGE_POOL_OUTPUT_TENSOR_COUNT (1)
#define AVERAGE_POOL_MAX_DIMS (3)
/* Compatible version information */
#define AVERAGE_POOL_COMPATIBLE_VERSION 0
/* Operator version information*/
#define AVERAGE_POOL_OPERATOR_VERSION 0

/* Averagepool operator specific structure */
typedef struct Averagepool {
    uint8 kernel[AVERAGE_POOL_MAX_DIMS];        /* dimensions of pooling window */
    uint8 stride[AVERAGE_POOL_MAX_DIMS];        /* strides of pooling window */
    uint8 padding_start[AVERAGE_POOL_MAX_DIMS]; /* padding top and left */
    uint8 padding_end[AVERAGE_POOL_MAX_DIMS];   /* padding bottom and right */
    uint8 kernel_index;                         /* Kernel index to be used for performing averagepool */
    int8 scale;                                 /* scale factor for input tensor */
} Averagepool;

/**
 * \brief averagepool processing function
 *
 * \param kalimba layer_lite structure with input, output and averagepool params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 *
 * \note size of input and output tensors should be same
 */
int averagepool2d_forward(layer_lite *layer);

#endif /*AVERAGEPOOL_KALIMBA_PUBLIC_H*/
