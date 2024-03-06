/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#ifndef EAI_PREPROCESS_COMMON_H
#define EAI_PREPROCESS_COMMON_H

/******************************************************************************/
#include "macros.h"
#include "buffer/buffer.h"
#include "pmalloc/pl_malloc.h"

/******************************************************************************/



typedef struct ALGO_OUTPUT_INFO
{
    unsigned tensor_id;
    unsigned size;
    signed *output_data; /*!< This is neither allocated not released by the framework */
}ALGO_OUTPUT_INFO;



#endif /* EAI_PREPROCESS_COMMON_H */
