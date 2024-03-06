/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  gemm_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for gemm operator
 *
 */

#ifndef GEMM_KALIMBA_PUBLIC_H
#define GEMM_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* maximum input tensor count */
#define GEMM_INPUT_COUNT (3)
/* maximum output tensor count */
#define GEMM_OUTPUT_COUNT (1)
/* Compatible version information */
#define GEMM_COMPATIBLE_VERSION 0
/* Operator version information*/
#define GEMM_OPERATOR_VERSION 0

#define TRANS_NONE (0x00)
#define TRANS_A (0x01)
#define TRANS_B (0x02)
#define TRANS_A_B (0x03)
#define TRANS_C (0x04)
#define TRANS_A_C (0x05)
#define TRANS_B_C (0x06)
#define TRANS_A_B_C (0x07)

/* GeMM operator specific structure */
typedef struct Gemm {
    uint8 trans_mask; /*!< Transpose mask of I/O tensors */
    int8 scale_A;     /*!< scale factor for input A tensor */
    int8 scale_B;     /*!< Scale factor for input B Tensor */
} Gemm;

/**
 * \brief gemm processing function
 *
 * \param kalimba layer_lite structure with input, output and gemm params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int gemm_forward(layer_lite *layer);

#endif /*GEMM_KALIMBA_PUBLIC_H*/
