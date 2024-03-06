/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  argmin_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for argmin operator
 *
 */

#ifndef ARGMIN_KALIMBA_PUBLIC_H
#define ARGMIN_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* Maximum possible dimensions for an input tensor*/
#define NDIM_4 (4)
/* Compatible version information */
#define ARGMIN_COMPATIBLE_VERSION 0
/* Operator version information*/
#define ARGMAX_OPERATOR_VERSION 0

/* argmin operator specific structure */
typedef struct Argmin {
    int8 scale;              /* scale factor for input tensor */
    uint8 keep_dim;          /* flag to retain number of dimensions of input tensor in output tensor*/
    int8 axis;               /* axis along which argmin is to be applied*/
    uint8 select_last_index; /* flag to select index of last occurring minimum in input data*/
} Argmin;

/**
 * \brief argmin processing function
 *
 * \param kalimba layer_lite structure with input, output and argmin params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int argmin_forward(layer_lite *layer);

#endif /*ARGMIN_KALIMBA_PUBLIC_H*/
