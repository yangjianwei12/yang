/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  conv_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for conv operator
 *
 */

#ifndef CONV_KALIMBA_PUBLIC_H
#define CONV_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"
#include "ml_defines.h"

/* minimum input tensor count */
#define CONV_MIN_INPUT_TENSOR_COUNT (4)
/* maximum input tensor count */
#define CONV_MAX_INPUT_TENSOR_COUNT (5)
/* output tensor count */
#define CONV_OUTPUT_TENSOR_COUNT (1)
/* Compatible version information */
#define CONV_COMPATIBLE_VERSION 0
/* Operator version information*/
#define CONV_OPERATOR_VERSION 0

typedef enum conv_type_t {
    /* Conv with Kernel size of dimension 1x1 */
    CONV_1X1 = 1,
    /* Conv with Kernel size of dimension NxN, N>1 */
    CONV_NXN,
    /* Conv with Kernel size of dimension MxN, N!=M */
    CONV_MxN,
    /* Conv with Kernel size of dimension 1xN */
    CONV_1XN,
    /* Conv with Kernel size of dimension HxN */
    CONV_HXN_1D,
    /* Conv with precalculated offsets - single pattern of kernel size */
    CONV_GENERIC_SINGLE_PATTERN_KERNEL,
    /* Conv with precalculated offsets - multiple pattern of kernel size */
    CONV_GENERIC_MULTI_PATTERN_KERNEL,
    /* Conv with precalculated offsets - single pattern of col size */
    CONV_GENERIC_SINGLE_PATTERN_COL,
    /* Conv with precalculated offsets - multiple pattern of col size */
    CONV_GENERIC_MULTI_PATTERN_COL,
} conv_type_t;

/* Conv operator specific structure */
typedef struct conv_params_t {
    int8 scale;                     /**< scaling factor to be applied on input tensor */
    eActivationFunction activation; /*!< activation function to be applied over convolution result */
    conv_type_t conv_type;          /*<! type of convolution to be performed */
    uint8 do_runtime_pad;           /*<! flag to indicate if padding is required in runtime */
    uint8 kernel_h; /*!< Kernel Height */
    uint8 kernel_w; /*!< Kernel Width */
    uint8 stride_h; /*!< Stride in along tensor width */
    uint8 stride_w; /*!< Stride along tensor height */

    uint16 group;                     /*!< number of groups in convolution */
    uint16 input_channels_per_group;  /*!< Number of input channels in one convolution group */
    uint16 output_channels_per_group; /*!< Number of output channels in one convolution group */
    uint16 kernels_per_group;         /*!< Number of kernels in one convolution group */

    uint16 pattern_size;   /*<! Size of one pattern in the pattern tensor */
    uint16 pattern_offset; /*!< Offset to the start of pattern info in pattern tensor */
    uint16 pad_start_w; /*<! Pad left */
    uint16 pad_end_w;   /*<! Pad right */
    uint16 pad_start_h; /*<! Pad top */
    uint16 pad_end_h;   /*<! Pad bottom */
} conv_params_t;

/**
 * \brief conv processing function
 *
 * \param kalimba layer_lite structure with input, output and conv params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int conv_forward(layer_lite *layer);

#endif /*CONV_KALIMBA_PUBLIC_H*/
