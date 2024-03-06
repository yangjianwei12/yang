/****************************************************************************
* Copyright (c) 2023 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  expand_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for expand operator
 *
 */

#ifndef EXPAND_KALIMBA_PUBLIC_H
#define EXPAND_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Compatible version information */
#define EXPAND_COMPATIBLE_VERSION 2
/* Operator version information*/
#define EXPAND_OPERATOR_VERSION 0

/* Expand operator specific structure */
typedef struct Expand {
    int8 scale;              /*!< scale factor for input data tensor */
    uint8 reserved1;         /*!< unused field to make it word aligned */
    uint16 reserved2;        /*!< unused field to make it word aligned */
    uint32 shape[MAX_NDIM];  /*!< list of 5 elements indicating given shape tensor */
} Expand;

/**
 * \brief expand processing function
 *
 * \param kalimba layer_lite structure with input, output and expand params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int expand_forward(layer_lite *layer);

#endif /*EXPAND_KALIMBA_PUBLIC_H*/