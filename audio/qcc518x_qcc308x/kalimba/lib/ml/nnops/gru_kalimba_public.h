/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  gru_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for gru operator
 *
 */

#ifndef GRU_KALIMBA_PUBLIC_H
#define GRU_KALIMBA_PUBLIC_H


#include "layer_lite_kalimba.h"

/* min input tensor count */
#define GRU_MIN_INPUT_TENSOR_COUNT (6)
/* max input tensor count */
#define GRU_MAX_INPUT_TENSOR_COUNT (8)
/* output tensor count */
#define GRU_OUTPUT_TENSOR_COUNT (3)
/* Compatible version information */
#define GRU_COMPATIBLE_VERSION 0
/* Operator version information*/
#define GRU_OPERATOR_VERSION 1

/* GRU operator specific structure */
typedef struct {
    int8 scale_input;         /*!< Scale factor for input tensor */
    int8 scale_initial_h;     /*!< Scale factor for initial h tensor */
    int8 scale_x_r;           /*!< Beta input + beta weight */
    int8 linear_before_reset; /*!< Apply linear operation before reset gate */
    int8 beta_output;         /*!< Beta for output */
    uint8 initialize_hidden;  /**< Flag to indicate if hidden state is to be initialized with initial_h */
} gru_params_t;

/**
 * \brief gru processing function
 *
 * \param kalimba layer_lite structure with input, output and gru params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int gru_forward(layer_lite *layer);

#endif /*GRU_KALIMBA_PUBLIC_H*/
