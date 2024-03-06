/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  rnn_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for rnn operator
 *
 */

#ifndef RNN_KALIMBA_PUBLIC_H
#define RNN_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Number of input tensors for RNN operator */
#define RNN_MIN_INPUT_TENSOR_COUNT 4

/* Number of input tensors for RNN operator */
#define RNN_MAX_INPUT_TENSOR_COUNT 5

/* Number of output tensors for RNN operator */
#define RNN_OUTPUT_TENSOR_COUNT 3

/* TanH scaling factor */
#define TANH_GATE_INPUT_SCALING 3

/* Sigmoid  scaling factor */
#define SIGMOID_GATE_INPUT_SCALING 4

/* Relu scaling factor */
#define RELU_GATE_INPUT_SCALING 0

/* Compatible version information */
#define RNN_COMPATIBLE_VERSION 0
/* Operator version information*/
#define RNN_OPERATOR_VERSION 1

/* Enumeration for activation function */
enum rnn_activation
{ 
    ACTIVATION_NONE = 0,
    ACTIVATION_RELU,
    ACTIVATION_TANH,
    ACTIVATION_SIGMOID
};

/* RNN operator specific parameters */
typedef struct rnn_params {
    int8 activation;         /**< activation function to be used in RNN */
    int8 scale_input;        /**< Scale factor for layer input */
    int8 scale_initial_h;    /**< Scale factor for initial hidden tensor */
    int8 beta_output;        /**< Beta value to be applied to output */
    int8 beta_weight;        /**< Beta value to be applied to weight */
    int8 beta_input;         /**< Beta value applied on input */
    uint8 initialize_hidden; /**< Flag to indicate if hidden state is to be initialized with initial_h */
} rnn_params_t;

/**
 * \brief rnn processing function
 *
 * \param kalimba layer_lite structure with input, output and rnn params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int rnn_forward(layer_lite *layer);

#endif /*RNN_KALIMBA_PUBLIC_H*/
