/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_nnops_tensor_def.h
 * \ingroup capabilities
 *
 * ML_NNOPS Tensor defination header file. <br>
 *
 */

#ifndef ML_NNOPS_TENSOR_DEF_H 
#define ML_NNOPS_TENSOR_DEF_H

/****************************************************************************
Include Files
*/
#include "ml_defines.h"

/* Please note that all these compiler defines are specific to the
 * example MUL operator illustrated in this capability */

#define NUM_INPUT_TENSORS  2
#define NUM_OUTPUT_TENSORS 1

/* For Tensor ID 0 -> first input tensor for the mul nn operator */
#define TENSOR_ID_0_NUM_DIMS    1
#define TENSOR_ID_0_NUM_ELEMS   160
#define TENSOR_ID_0_DATA_TYPE   ML_DATATYPE_INT32

/* For Tensor ID 1 -> second input tensor for the mul nn operator */
#define TENSOR_ID_1_NUM_DIMS    1
#define TENSOR_ID_1_NUM_ELEMS   1
#define TENSOR_ID_1_DATA_TYPE   ML_DATATYPE_INT32

/* For Tensor ID 2 -> output tensor for the mul nn operator */
#define TENSOR_ID_2_NUM_DIMS    1
#define TENSOR_ID_2_NUM_ELEMS   160
#define TENSOR_ID_2_DATA_TYPE   ML_DATATYPE_INT32

#endif/* ML_NNOPS_TENSOR_DEF_H */
