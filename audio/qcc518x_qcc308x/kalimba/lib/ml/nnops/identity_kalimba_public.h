/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  identity_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for identity operator
 *
 */

#ifndef IDENTITY_KALIMBA_PUBLIC_H
#define IDENTITY_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Identity operator specific structure */
typedef struct Identity {
    int8 scale; /*!< scale factor for input tensor */
} Identity;

/**
 * \brief identity processing function
 *
 * \param kalimba layer_lite structure with input, output and identity params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int identity_forward(layer_lite *layer);

#endif /*IDENTITY_KALIMBA_PUBLIC_H*/
