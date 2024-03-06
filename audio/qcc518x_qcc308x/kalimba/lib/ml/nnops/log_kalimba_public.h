/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  log_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for log operator
 *
 */

#ifndef LOG_KALIMBA_PUBLIC_H
#define LOG_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* input tensor count */
#define LOG_INPUT_TENSOR_COUNT (1)
/* output tensor count */
#define LOG_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define LOG_COMPATIBLE_VERSION 0
/* Operator version information*/
#define LOG_OPERATOR_VERSION 0

typedef struct Log {
    int8 beta_x;      /*!< Beta_x scaling value */
    int8 scale;       /*!< scale factor for input tensor */
    uint8 is_inplace; /*!< Flag to indicate if the operation is inplace */
} Log;

/**
 * \brief log processing function
 *
 * \param kalimba layer_lite structure with input, output and log params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int log_forward(layer_lite *layer);

#endif /*LOG_KALIMBA_PUBLIC_H*/
