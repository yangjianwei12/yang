/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  batchnorm_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for batchnorm operator
 *
 */

#ifndef BATCHNORM_KALIMBA_PUBLIC_H
#define BATCHNORM_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* maximum input tensor count */
#define BATCH_NORM_INPUT_TENSOR_COUNT (3)
/* maximum output tensor count */
#define BATCH_NORM_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define BATCH_NORM_COMPATIBLE_VERSION 0
/* Operator version information*/
#define BATCH_NORM_OPERATOR_VERSION 0

/* Batch normalization operator specific structure */
typedef struct {
    int8 scale;            /* Scale factor for input tensor */
    uint8 is_inplace;      /* Flag to indicate if the operation is inplace */
    uint16 elems_per_loop; /* Count of elements in every batchnorm loop */
} batch_norm_params_t;

/**
 * \brief batchnorm processing function
 *
 * \param kalimba layer_lite structure with input, output and batch norm params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int batchnorm_forward(layer_lite *layer);

#endif /*BATCHNORM_KALIMBA_PUBLIC_H*/
