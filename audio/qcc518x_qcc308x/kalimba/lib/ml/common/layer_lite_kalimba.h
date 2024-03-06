/****************************************************************************
 * Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/

#ifndef _LAYER_LITE_H_
#define _LAYER_LITE_H_

#include "tensor_kalimba.h"

/* Initialization state of layer for the first inference*/
#define LAYER_INIT_STATE 1
/* Rest state of layer for the from the second inference onwards */
#define LAYER_RESET_STATE 2
/* Flag to indicate if the layer performs an inplace operation.
 * This means that the data buffer for the output tensor is
 * same as one of the input tensor.
 */
#define LAYER_IS_IN_PLACE 3

/*! @struct tensorlist_lite
    @brief Tensor list data structure storing the array of tensor pointers
*/
typedef struct {
    uint32 count;      /*!< Number of tensors. */
    Tensor *tensors[]; /*!< Zero-length, so zero bytes in memory. */
} tensorlist_lite;

/*! @struct layer_lite
    @brief layer data structure for operator interface
*/
typedef struct {
    uint32 flags;                      /*!< Indicates layer properties like - LAYER_INIT_STATE, LAYER_RESET_STATE. */
    tensorlist_lite *p_input_tensors;  /*!< Pointer to input tensor list for this operator. */
    tensorlist_lite *p_output_tensors; /*!< Pointer to output tensor list for this operator. */
    void *params;                      /*!< Pointer to parameters for this operator. */
} layer_lite;

#endif /*_LAYER_LITE_H_*/
