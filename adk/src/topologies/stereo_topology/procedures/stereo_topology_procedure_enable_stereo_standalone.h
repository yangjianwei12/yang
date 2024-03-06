/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Procedure to work as standalone stereo topolgy.
*/

#ifndef STEREO_TOPOLOGY_PROC_ENABLE_STEREO_STANDALONE_H
#define STEREO_TOPOLOGY_PROC_ENABLE_STEREO_STANDALONE_H

#include "stereo_topology_procedures.h"
#include "stereo_topology_private.h"
#include "script_engine.h"

typedef enum
{
    PROC_SEND_STEREO_TOPOLOGY_MESSAGE_ENABLE_STANDALONE_FINISHED = STEREOTOP_INTERNAL_PROCEDURE_RESULTS_MSG_BASE | stereo_topology_procedure_enable_stereo_standalone,
} stereo_proc_enable_standalone_messages_t;


extern const procedure_script_t stereo_enable_stereo_standalone_script;

#endif /* STEREO_TOPOLOGY_PROC_ENABLE_STEREO_STANDALONE_H */
