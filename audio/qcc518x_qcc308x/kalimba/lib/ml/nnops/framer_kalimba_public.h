/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2019-2022 Qualcomm Technologies, Inc.                       *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#ifndef _FRAMER_KALIMBA_PUBLIC_H_
#define _FRAMER_KALIMBA_PUBLIC_H_

#include "layer_lite_kalimba.h"

/**
 * @def FRAMER_INPUT_TENSOR_COUNT
 * @brief Number of input tensors for framer operator.
 */

#define FRAMER_INPUT_TENSOR_COUNT (1)
/**
 * @def FRAMER_OUTPUT_TENSOR_COUNT
 * @brief Number of output tensors for framer operator.
 */
#define FRAMER_OUTPUT_TENSOR_COUNT (2)
/* Compatible version information */
#define FRAMER_COMPATIBLE_VERSION 0
/* Operator version information*/
#define FRAMER_OPERATOR_VERSION 0

enum FRAMER_BLOCK_MODE {
    FRAMER_BLOCK = 0, // 0: return EAI_NEED_MORE if not ready
    FRAMER_NO_BLOCK,  // 1: forward always and fill data from the beginning
    FRAMER_NO_BLOCK2, // 2: forward always and fill data from the last positon
};

typedef struct Framer {
    int8 scale;        /*!< scale factor for input tensor */
    uint8 transpose;   /*!< Flag to indicate transpose operation when framing */
    uint8 block_mode;  /*!< Frame blocking mode */
    uint8 initialized; /*!< Flag to indicate if frame pointer has been initialized */
    uint32 frame_pos;  /*!< Current frame position */
    uint32 stride;     /*!< Stride for frame reset */
} Framer;


/**
 * @fn int framer_forward(layer_lite *layer)
 * @brief It processes the Layer and performs framer operation.
 * @details layer_lite must contain the list of input and output tensors.
 * @param[in,out] layer layer_lite to be processed
 * @return int error code
 * @retval EAI_SUCCESS sqrt success
 * @retval EAI_FAIL sqrt fail
 */
int framer_forward(layer_lite *layer);

#endif // _FRAMER_KALIMBA_PUBLIC_H_
