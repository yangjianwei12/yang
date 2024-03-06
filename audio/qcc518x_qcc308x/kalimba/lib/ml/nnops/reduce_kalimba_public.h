/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  reduce_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for reduce operator
 *
 */

#ifndef REDUCE_KALIMBA_PUBLIC_H
#define REDUCE_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* Number of input tensors for REDUCE operator */
#define REDUCE_INPUT_TENSOR_COUNT (1)
/* Number of output tensors for REDUCE operator */
#define REDUCE_OUTPUT_TENSOR_COUNT (1)
/*  Maximum possible dimensions for an input tensor */
#define NDIM_4 (4)
/* Compatible version information */
#define REDUCE_COMPATIBLE_VERSION 0
/* Operator version information*/
#define REDUCE_OPERATOR_VERSION 0

/* Reduce operator parameter specific structure */
typedef struct Reduce {
    int8 scale;                    /*!< scale factor for input tensor */
    uint8 reduce_type;             /*!< flag to select the type of reduce*/
    uint16 reset_count;            /*!< count for the accumulation along axis to get the mean*/
    uint16 axes_dims[MAX_NDIM];    /*!< sequence of dims according to axes */
    uint16 axes_strides[MAX_NDIM]; /*!< sequence of strides according to axes*/
} Reduce;

/* Type of reduce to be performed */
typedef enum REDUCE_TYPE {
    REDUCE_L1 = 1,
    REDUCE_L2,
    REDUCE_SUM,
    REDUCE_SUM_SQUARE,
} REDUCE_TYPE;

/**
 * \brief reduce processing function
 *
 * \param kalimba layer_lite structure with input, output and reduce params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int reduce_forward(layer_lite *layer);

#endif /*REDUCE_KALIMBA_PUBLIC_H*/
