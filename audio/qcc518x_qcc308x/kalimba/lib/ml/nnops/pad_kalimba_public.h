/****************************************************************************
* Copyright (c) 2023 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  pad_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for pad operator
 *
 */

#ifndef PAD_KALIMBA_PUBLIC_H
#define PAD_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Number of input tensors for pad operator.*/
#define PAD_INPUT_TENSOR_COUNT (1)

/* Number of output tensors for pad operator.*/
#define PAD_OUTPUT_TENSOR_COUNT (1)

/* Compatible version information */
#define PAD_COMPATIBLE_VERSION 2

/* Operator version information*/
#define PAD_OPERATOR_VERSION 0

/* pad operator specific structure */
typedef struct Pad {
    int8 scale;         /*!< scale factor for input tensor */
    uint8 reserved1;    /*!< unused field to make it word aligned */
    uint16 reserved2;   /*!< unused field to make it word aligned */
    int32 pad_value;    /*!< constant value used for padding */
    uint16 pad_n_start; /*!< start padding in n dimension */
    uint16 pad_n_end;   /*!< end padding in n dimension */
    uint16 pad_c_start; /*!< start padding in c dimension */
    uint16 pad_c_end;   /*!< end padding in c dimension */
    uint16 pad_h_start; /*!< start padding in h dimension */
    uint16 pad_h_end;   /*!< end padding in h dimension */
    uint16 pad_w_start; /*!< start padding in w dimension */
    uint16 pad_w_end;   /*!< end padding in w dimension */
} Pad;

/**
 * \brief pad processing function
 *
 * \param kalimba layer_lite structure with input, output and pad params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int pad_forward(layer_lite *layer);

#endif /*PAD_KALIMBA_PUBLIC_H*/
