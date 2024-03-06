/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  clip_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for clip operator
 *
 */

#ifndef CLIP_KALIMBA_PUBLIC_H
#define CLIP_KALIMBA_PUBLIC_H

#include <stdfix.h>
#include "layer_lite_kalimba.h"

/* maximum input tensor count */
#define CLIP_INPUT_TENSOR_COUNT (1)
/* maximum output tensor count */
#define CLIP_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define CLIP_COMPATIBLE_VERSION 0
/* Operator version information*/
#define CLIP_OPERATOR_VERSION 0

/* Clip operator specific data structure */
typedef struct Clip {
    sat fract clip_min; /* param for minimum value to limit input tensor */
    sat fract clip_max; /* param for maximum value to limit input tensor */
    int8 scale;         /* scale factor for input tensor */
    uint8 is_inplace;   /* flag to indicate if the operation is inplace */
} Clip;

/**
 * \brief clip processing function
 *
 * \param kalimba layer_lite structure with input, output and clip params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int clip_forward(layer_lite *layer);

#endif /*CLIP_KALIMBA_PUBLIC_H*/
