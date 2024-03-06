/****************************************************************************
 * Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/

#ifndef TENSOR_KALIMBA_H
#define TENSOR_KALIMBA_H

#include "hydra_types.h"

/*
 * This structure is the internal (Kalimba) representation of a Tensor, which is shared with the offline tool
 * If running on Kalimba, this is the same as 'Tensor'. Tensor can have maximum 5 dimensions. Currently 
 * tensor supports only XNCHW data format, where N, C, H and W represents the batch, channel, height and
 * width of the tensor and X represents the fifth dimensions of the tensor.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
#ifndef MAX_NDIM
#define MAX_NDIM (5)
#endif
#ifndef dim_type
typedef int32 dim_type;
typedef int32 stride_type;
#endif

typedef struct KalimbaTensor {
    int32 size;                    /*!< Actual size of tensor = sizeof(KalimbaTensor) + size of all subsequent data. */
    uint32 flags;                  /*!< Flags indicate the tensor's property, currently reserved for the future use */

    uint32 ndim : 4;               /*!< Number of dimensions, max 5 dimensions are supported */
    uint32 elem_size : 4;          /*!< Size of each tensor element */
    uint32 data_type : 8;          /*!< The type of the data pointed to the data pointer of tensor */
    uint32 zero : 16;              /*!< Unused */

    uint32 step_size_q : 8;        /*!< Unused */
    uint32 num_elems : 24;         /*!< Number of tensor elements */

    int32 step_size;               /*!< Unused */
    int32 inverse_scale;           /*!< Unused */
    int16 min;                     /*!< Unused */
    int16 max;                     /*!< Unused */

    dim_type dims_filled;          /*!< Unused */
    dim_type dims[MAX_NDIM];       /*!< Values of dimension, supported sequence is  xnchw */
    stride_type strides[MAX_NDIM]; /*!< Values of strides for each dimension in xnchw sequence */

    uint8 *data;                   /*!< Tensor data pointer */

} KalimbaTensor;

typedef struct KalimbaTensor Tensor;

#ifdef __cplusplus
}
#endif
#endif /* TENSOR_KALIMBA_H */
