/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      DCF manager intialises the individual engines according to the supplied configuration
*/

#include "dcf.h"
#include "dcf_config.h"
#include "dcf_engine.h"

bool Dcf_Init(Task init_task)
{
#ifdef INCLUDE_DCF_OPEN_GARDEN
    DcfOpenGarden_Init();
#endif

    UNUSED(init_task);
    return TRUE;
}
