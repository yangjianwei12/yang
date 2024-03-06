/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  lstm_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for lstm operator
 *
 */

#ifndef LSTM_KALIMBA_PUBLIC_H
#define LSTM_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* min input tensor count */
#define LSTM_MIN_INPUT_TENSOR_COUNT (6)
/* max input tensor count */
#define LSTM_MAX_INPUT_TENSOR_COUNT (7)
/* output tensor count */
#define LSTM_OUTPUT_TENSOR_COUNT (5)
/* Compatible version information */
#define LSTM_COMPATIBLE_VERSION 0
/* Operator version information*/
#define LSTM_OPERATOR_VERSION 1

/* LSTM operator specific structure */
typedef struct {
    int8 input_scale;        /*!< Scale factor for input tensor */
    int8 initial_h_scale;    /*!< Scale factor for initial_h tensor */
    int8 initial_c_scale;    /*!< Scale factor for initial_c tensor */
    int8 beta_z;             /*!< Beta input + beta weight */
    int8 beta_output;        /*!< Beta output for hidden scaling */
    int8 beta_cell;          /*!< Beta for cell scaling */
    uint8 initialize_hidden; /**< Flag to indicate if hidden state is to be initialized with initial_h */
    uint8 initialize_cell;   /**< Flag to indicate if cell state is to be initialized with initial_c */
} lstm_params_t;

/**
 * \brief lstm processing function
 *
 * \param kalimba layer_lite structure with input, output and lstm params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int lstm_forward(layer_lite *layer);

#endif /*LSTM_KALIMBA_PUBLIC_H*/
