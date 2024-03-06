/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  argmax_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for argmax operator
 *
 */

#ifndef ARGMAX_KALIMBA_PUBLIC_H
#define ARGMAX_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* Maximum possible dimensions for an input tensor */
#define NDIM_4 (4)
/* Compatible version information */
#define ARGMAX_COMPATIBLE_VERSION 0
/* Operator version information*/
#define ARGMAX_OPERATOR_VERSION 0

/* argmax operator specific structure */
typedef struct Argmax {
    int8 scale;              /* scale factor for input tensor */
    uint8 keep_dim;          /* flag to retain number of dimensions of input tensor in output tensor*/
    int8 axis;               /* axis along which argmax is to be applied*/
    uint8 select_last_index; /* flag to select index of last occurring maximum in input data*/
} Argmax;


/**
 * \brief argmax processing function
 *
 * \param kalimba layer_lite structure with input, output and argmax params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int argmax_forward(layer_lite *layer);

#endif /*ARGMAX_KALIMBA_PUBLIC_H*/
