/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#ifndef EAI_POSTPROCESS_COMMON_H
#define EAI_POSTPROCESS_COMMON_H

/******************************************************************************/
#include "macros.h"
#include "buffer/buffer.h"
#include "pmalloc/pl_malloc.h"

/******************************************************************************/



typedef struct ALGO_INPUT_INFO
{
    unsigned tensor_id;
    unsigned size;
    signed *input_data; /*!< This is neither allocated not released by the framework */
}ALGO_INPUT_INFO;



#endif /* EAI_POSTPROCESS_COMMON_H */
