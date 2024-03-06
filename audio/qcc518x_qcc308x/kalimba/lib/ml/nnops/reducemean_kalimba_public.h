/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  reducemean_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for reducemean operator
 *
 */

#ifndef REDUCEMEAN_KALIMBA_PUBLIC_H
#define REDUCEMEAN_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Number of input tensors for REDUCEMEAN operator */
#define REDUCEMEAN_INPUT_TENSOR_COUNT (1)
/*  Number of output tensors for REDUCEMEAN operator */
#define REDUCEMEAN_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define REDUCEMEAN_COMPATIBLE_VERSION 0
/* Operator version information*/
#define REDUCEMEAN_OPERATOR_VERSION 0

/* Reducemean operator parameter specific structure */
typedef struct Reducemean {
    int8 scale;                    /*!< scale factor for input tensor */
    uint8 pad_field;               /*!< unused field to support packing and unpacking of the struct*/
    uint16 reset_count;            /*!< count for the accumulation along axis to get the mean*/
    sat fract inv_reset_count;     /*!< inverse of count for the accumulation along axis to get the mean*/
    uint16 axes_dims[MAX_NDIM];    /*!< sequence of dims according to axes */
    uint16 axes_strides[MAX_NDIM]; /*!< sequence of strides according to axes*/
} Reducemean;

/**
 * \brief reducemean processing function
 *
 *\param kalimba layer_lite structure with input, output and reducemean params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int reducemean_forward(layer_lite *layer);

#endif /*REDUCEMEAN_KALIMBA_PUBLIC_H*/
