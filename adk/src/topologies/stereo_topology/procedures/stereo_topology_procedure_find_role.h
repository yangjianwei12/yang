/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#ifndef STEREO_TOPOLOGY_PROC_FIND_ROLE_H
#define STEREO_TOPOLOGY_PROC_FIND_ROLE_H

#include "stereo_topology_procedures.h"
#include "stereo_topology_private.h"
#include "script_engine.h"

extern const procedure_fns_t stereo_proc_find_role_fns;

typedef enum
{
    PROC_SEND_STEREO_TOPOLOGY_MESSAGE_FIND_ROLE_FINISHED = STEREOTOP_INTERNAL_PROCEDURE_RESULTS_MSG_BASE | stereo_topology_procedure_find_role,
} stereo_proc_find_role_messages_t;


extern const procedure_script_t stereo_find_role_script;

#endif /* STEREO_TOPOLOGY_PROC_FIND_ROLE_H */
