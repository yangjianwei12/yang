/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_defines.h
 * \ingroup  ml
 *
 * Header file containing common ML defines
 *
 */

#ifndef ML_DEFINES_H
#define ML_DEFINES_H

/****************************************************************************
Include Files
*/
#include "hydra_types.h"

typedef enum LAYER_TYPE {
    LAYER_TYPE_CONVOLUTION=0,           /* Convolution */
    LAYER_TYPE_INNER_PRODUCT,           /* Inner Product */
    LAYER_TYPE_RELU,                    /* Rectified Linear Function */
    LAYER_TYPE_LOGSOFTMAX,              /* Log of Softmax */
    LAYER_TYPE_MAX_OUT,                 /* Output max elements from input tensor */
    LAYER_TYPE_MAX_POOL,                /* Max pooling across tensor */

    LAYER_TYPE_RESERVED_6,
    LAYER_TYPE_RESHAPE,                 /* Reshape the input tensor */
    LAYER_TYPE_LSTM,                    /* Long-Short Term Memory */
    LAYER_TYPE_RESERVED_9,
    LAYER_TYPE_GRU,                     /* GRU */
    LAYER_TYPE_RNN,                     /* Recurrent NN */
    LAYER_TYPE_TRANSPOSE,               /* Transpose input tensor */
    LAYER_TYPE_RESERVED_13,

    LAYER_TYPE_BATCH_NORMALIZE,         /* Batch Normalization */
    LAYER_TYPE_QUANTIZE,                /* Quantization */
    LAYER_TYPE_ADD,                     /* Element-wise binary Addition */
    LAYER_TYPE_MATMUL,                  /* Matrix Multiplication */
    LAYER_TYPE_DIV,                     /* Element-wise binary division */
    LAYER_TYPE_CONSTANT,                /* Constant Tensor */
    LAYER_TYPE_NO_OP,                   /* No Op */

    LAYER_TYPE_REDUCEMEAN,              /* Mean of the input tensor's element along the provided axes */
    LAYER_TYPE_RESERVED_22,

    LAYER_TYPE_FRAMER,                  /* Output 2-D frame from feature vectors */
    LAYER_TYPE_SLICE,                   /* Produces a slice of the input tensor */
    LAYER_TYPE_SHAPE,                   /* Output the shape of the input tensor */
    LAYER_TYPE_SQUEEZE,                 /* Reduce */
    LAYER_TYPE_UNSQUEEZE,               /* Insert single-dimensional entries to the shape of a tensor */
    LAYER_TYPE_MUL,                     /* Element-wise binary multiplication */
    LAYER_TYPE_MAX,                     /* Element-wise max of each of the input tensors */
    LAYER_TYPE_CONCAT,                  /* Concatentate a list of tensors into a single tensor */

    LAYER_TYPE_RESERVED_31,

    LAYER_TYPE_SOFTMAX,                 /* Softmax */
    LAYER_TYPE_TANH,                    /* Tanh of the given input tensor, element-wise */
    LAYER_TYPE_REDUCE,                  /* Norm of the input tensor's element along the provided axes */
    LAYER_TYPE_CONV1D,                  /* Convolution 1D */
    LAYER_TYPE_SUB,                     /* Element-wise binary subtraction */
    LAYER_TYPE_POW,                     /* Power of the given input tensor, element-wise */
    LAYER_TYPE_CLIP,                    /* Limits (Clips) the given input within an interval */
    LAYER_TYPE_SIGMOID,                 /* Sigmoid */

    LAYER_TYPE_RESERVED_40,

    LAYER_TYPE_GATHER,                  /* Selectively gather entries from tensor */
    LAYER_TYPE_LOG,                     /* Natural log of the given input tensor, element-wise */
    LAYER_TYPE_SQRT,                    /* Square root of the given input tensor, element-wise */
    LAYER_TYPE_CONSTANT_FILL,           /* Constant Tensor */

    LAYER_TYPE_RESERVED_45,
    LAYER_TYPE_RESERVED_46,
    LAYER_TYPE_EXP,                     /* Exp */
    LAYER_TYPE_RESERVED_48,
    LAYER_TYPE_RESERVED_49,
    LAYER_TYPE_GEMM,                    /* General Matrix-matrix multiplication */
    LAYER_TYPE_RESERVED_51,

    LAYER_TYPE_GLOBAL_AVERAGE_POOL,     /* Global Average pooling across the values in the same channel */
    LAYER_TYPE_AVERAGE_POOL,            /* Average pooling across the values in the same channel */
    LAYER_TYPE_ARGMAX,                  /* Computes the indices of the max elements of the input tensor */
    LAYER_TYPE_ARGMIN,                  /* Computes the indices of the min elements of the input tensor */
    LAYER_TYPE_DEQUANTIZE,              /* DeQuantization */

    LAYER_TYPE_CONVTRANSPOSE,           /* Transposed Convolution */
    LAYER_TYPE_MIN,
    LAYER_TYPE_PAD,
    LAYER_TYPE_PRELU,
    LAYER_TYPE_SPLIT,
    LAYER_TYPE_EXPAND,
    LAYER_TYPE_NEG,
    LAYER_TYPE_TYPE_MAX = 0x3FFFFFFF
} LAYER_TYPE;


/* ML Data Types */
#define ML_DATATYPE_INT8           3    /* Signed 8 bit data    */
#define ML_DATATYPE_INT16          5    /* Signed 16 bit data   */
#define ML_DATATYPE_INT32          6    /* Signed 32 bit data   */

/* ML Result Codes */
typedef enum ML_RESULT {
    ML_SUCCESS = 0,                     /* Successful - No errors */
    ML_FAIL,                            /* Failure - Generic */
    ML_NEED_MORE,                       /* Need more data for processing */
} ML_RESULT;

/* Activation functions */
enum {
    ACTIVATION_FUNCTION_NONE,           /* No activation */
    ACTIVATION_FUNCTION_RELU,           /* ReLU activation function */
    ACTIVATION_FUNCTION_SOFTMAX,        /* Softmax activation function */
    ACTIVATION_FUNCTION_SIGMOID,        /* Sigmoid activation function */
};
typedef uint8 eActivationFunction;

/* Operator kernels */
enum {
    KERNEL_SS,                          /* Scalar Scalar Kernel */
    KERNEL_SV,                          /* Scalar Vector Kernel */
    KERNEL_VS,                          /* Vector Scalar Kernel */
    KERNEL_VV,                          /* Vector Vector Kernel */
    KERNEL_INVALID,                     /* Invalid Kernel */
};

#endif /* ML_DEFINES_H */
