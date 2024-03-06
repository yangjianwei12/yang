/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.                       *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#ifndef _UTILS_KALIMBA_H
#define _UTILS_KALIMBA_H

#include <stdfix.h>
#include "log_utils_kalimba.h"
#include "ml_defines.h"
#include "macros.h"
#include "tensor_kalimba.h"
#ifdef INPUT_VALIDATION
#include "debug_utils_kalimba.h"
#endif /* INPUT_VALIDATION */

#ifdef __KALIMBA__
typedef fract weight_type;
typedef fract accu_type;
typedef fract bitvalue_type;
#else
typedef int32 weight_type;
typedef int32 accu_type;
typedef int32 bitvalue_type;
#endif

// suppress warning 'unused-variable'
#ifndef unused
#define unused UNUSED
#endif

#define TENSOR_FLAG_HAS_DATA (1 << 0)
#define TENSOR_FLAG_NEED_ALIGN_BY_128 (1 << 1)
#define TENSOR_FLAG_NEED_BUFFER (1 << 2) // initialize on runtime
#define DATATYPE_ACCU_TYPE EAI_DATATYPE_INT32

#define DIM_N(tensor) (((tensor)->ndim < 4) ? 1 : ((int)(tensor)->dims[(tensor)->ndim - 4]))

#define DIM_C(tensor) (((tensor)->ndim < 3) ? 1 : ((int)(tensor)->dims[(tensor)->ndim - 3]))
#define DIM_H(tensor) (((tensor)->ndim < 2) ? 1 : ((int)(tensor)->dims[(tensor)->ndim - 2]))
#define DIM_W(tensor) (((tensor)->ndim < 1) ? 0 : ((int)(tensor)->dims[(tensor)->ndim - 1]))

#define STRIDE_N(tensor) ((int)(tensor)->strides[((tensor)->ndim < 4) ? 0 : (tensor)->ndim - 4])
#define STRIDE_C(tensor) ((int)(tensor)->strides[((tensor)->ndim < 3) ? 0 : (tensor)->ndim - 3])
#define STRIDE_H(tensor) ((int)(tensor)->strides[((tensor)->ndim < 2) ? 0 : (tensor)->ndim - 2])
#define STRIDE_W(tensor) ((int)(tensor)->strides[((tensor)->ndim < 1) ? 0 : (tensor)->ndim - 1])

#define VERIFY_SHAPE_INFERENCE 0
#define NEED_SHAPE_INFERENCE(tensor) (((tensor) != NULL) && (VERIFY_SHAPE_INFERENCE || NEED_BUFFER(tensor)))
#define NEED_BUFFER(tensor) (((tensor) != NULL) && (((Tensor *)tensor)->flags & TENSOR_FLAG_NEED_BUFFER))

#define TENSOR_FLAG_DATA_LAYOUT_C_ROW_MAJOR (1 << 9)
#define TENSOR_FLAG_DATA_LAYOUT_HVX_64X_U8 (1 << 10)
#define TENSOR_FLAG_DATA_LAYOUT_HVX_4W_16X_U8 (1 << 11)
#define TENSOR_FLAG_DATA_LAYOUT_HVX_4W_32X_U8 (1 << 12)
#define TENSOR_FLAG_DATA_LAYOUT_HVX_4W_64X_U8 (1 << 13)
#define TENSOR_FLAG_DATA_LAYOUT_HVX_4W_128X_U8 (1 << 14)
#define TENSOR_FLAG_DATA_LAYOUT_HVX_2W_16X_S16 (1 << 15)
#define TENSOR_FLAG_DATA_LAYOUT_HVX_2W_32X_S16 (1 << 16)
#define TENSOR_FLAG_DATA_LAYOUT_HVX_2W_64X_S16 (1 << 17)
#define TENSOR_FLAG_DATA_LAYOUT_HVX_2W_128X_S16 (1 << 18)
#define TENSOR_FLAG_DATA_LAYOUT_HVX_NOT_SUPPORTED (1 << 19)
#define TENSOR_FLAG_DATA_LAYOUT_WRONG_FORMAT (1 << 20)

#define MAX_ALLOWED_SHIFT 31 /**< Maximum shift allowed for sat fract*/

/**
 * @brief Memory copy by word
 *
 * @param d destination address
 * @param s source address
 * @param num_words number of words to be copied
 */
void memwcpy(uint32 *d, uint32 *s, uint32 num_words);

/**
 * @brief Memory set by word
 *
 * @param d destination address
 * @param v value to be set
 * @param num_words number of words to be set
 */

void memwset(uint32 *d, uint32 v, uint32 num_words);

int32 is_same_dimension(Tensor *a, Tensor *b);

int32 dims_size(int ndim, dim_type *dims);

int get_pooling_size(int input, int kernel, int stride, int padding, int dilation);

void tensor_set_dimension(Tensor *t, int ndim, dim_type *dims, stride_type *strides);

/**
 * @brief Perform arithmetic left/right shift inplace. If scaling factor is:
 *     1). positive, then perform arithmetic right shift by scaling factor.
 *     2). negative, then perform arithmetic left shift shift by abs of scaling factor.
 *
 * @param input tensor's data which needs to be scale
 * @param len number of the element in the tensor's data
 * @param scaling_factor a signed integer on which shift will perform
 */
#ifdef __KALIMBA__
void scale_inplace(sat fract *input, int32 len, int8 scaling_factor);
#else
void scale_inplace(int32 *input, int32 len, int8 scaling_factor);
#endif
/**
 * @brief Pads empty dimensions of given input tensor with 1
 *
 * @param t input tensor
 * @param ndim number of output dimensions
 * @param dim output dimensions
 * @note The function does not copy padded dimensions to tensor dims
 */
void pad_dims(Tensor *t, uint32 ndim, dim_type *dim);

/**
 * @brief Sets the ndims, dims and strides of given tensor
 *
 * @param tensor given tensor
 * @param ndim number of dimensions to be set for the given tensor
 * @param dims dimensions to be updated for the given tensor
 * @param strides strides to be updated for the given tensor
 * @return int
 * @retval EAI_SUCCESS	Operation success
 * @retval EAI_FAIL operation fail
 */
int set_tensor_attributes(Tensor *tensor, int ndim, uint16 *dims, uint16 *strides);

/**
 * @brief Perform arithmetic left/right shift inplace. If scaling factor is:
 *     1). positive, then perform arithmetic right shift by scaling factor.
 *     2). negative, then perform arithmetic left shift shift by abs of scaling factor.
 *
 * @param input tensor's data which needs to be scale
 * @param output tensor's data where result stored after perform the scaling.
 * @param len number of the element in the tensor's data
 * @param scaling_factor a signed integer on which shift will perform
 */
#ifdef __KALIMBA__
void scale(sat fract *input, sat fract *output, int32 len, int8 scaling_factor);
#else
void scale(int32 *input, int32 *output, int32 len, int8 scaling_factor);
#endif

/**
 * @brief calculates number of elements in given tensor by multiplying its dimensions
 * @param t input tensor
 */
void calculate_tensor_size_words(Tensor *t);

#ifdef ENABLE_DEBUG_LOG
void print_tensor1d(const char *text, Tensor *t);
void print_tensor2d(const char *text, Tensor *t);
void print_tensor_info(const char *text, Tensor *t);
#endif

typedef struct {
    const char *name;
    uint32 index;
} TENSOR_INDEX_ENTRY;

/**
 * @brief calculates number of elements in given tensor by multiplying its dimensions
 * @param entry in-out structure pointer containing the tensor names and index
 * @param count the number of tensor names
 * @param file_input_buffer contents of the index file
 * @param file_buffer_len length of the index file
 */
int get_input_tensors(TENSOR_INDEX_ENTRY *entry, int count, const uint8 *file_input_buffer, int file_buffer_len);

#endif /* _UTILS_KALIMBA_H */
