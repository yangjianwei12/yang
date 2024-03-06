/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  transpose_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for transpose operator
 *
 */

#ifndef TRANSPOSE_KALIMBA_PUBLIC_H
#define TRANSPOSE_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Max permutation count supported */
#define MAX_PERMUTE_COUNT (5)

/* Number of input tensors for transpose operator */
#define TRANSPOSE_INPUT_TENSOR_COUNT (1)
/* Number of output tensors for transpose operator */
#define TRANSPOSE_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define TRANSPOSE_COMPATIBLE_VERSION 0
/* Operator version information*/
#define TRANSPOSE_OPERATOR_VERSION 0

/* Transpose operator specific structure */
typedef struct Transpose {
    int8 permute_count;              /*!< permutation count of input tensor */
    int8 permute[MAX_PERMUTE_COUNT]; /*!< permutation of input tensor dimensions*/
    int8 scale;                      /*!< scale factor for input tensor */
    uint8 copy_data;                 /*!< flag to indicate if memcpy is to be performed */
} Transpose;

/**
 * \brief transpose processing function
 *
 * \param kalimba layer_lite structure with input, output and transpose params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int transpose_forward(layer_lite *layer);

#endif /*TRANSPOSE_KALIMBA_PUBLIC_H*/
