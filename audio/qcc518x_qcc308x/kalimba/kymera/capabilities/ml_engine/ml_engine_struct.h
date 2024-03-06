/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_engine_struct.h
 * \ingroup capabilities
 *
 *
 */

#ifndef ML_ENGINE_STRUCT_H
#define ML_ENGINE_STRUCT_H

#include "capabilities.h"
#include "const_data/const_data.h"
#include "pmalloc/pl_malloc.h"
#include "ml_runtime_if.h"

#define MAX_CHANNELS (1)
#define MAX_TENSORS (10)

/*********************************************Configuration Flags*****************************/
#define REMOVE_PERSISTENT_TENSOR(flag) flag & 0x00000001

/****************************************************************************
Public Type Definitions
*/

typedef struct ML_ENGINE_OP_DATA{
    ML_ENGINE_NODE *use_cases;              /*!< Linked list of active use cases */
    void *ml_engine_private_data;           /*!< Private data specific to ML Engine Capability only */
                                            /*   Not to be used by other ML Capabilities */
    unsigned uc_id;                         /*!< use case id associated with the input tensors */
    unsigned frames_processed;              /*!< total numbers of frames processed by ML Engine */
} ML_ENGINE_OP_DATA;
#endif /* ML_ENGINE_H */
