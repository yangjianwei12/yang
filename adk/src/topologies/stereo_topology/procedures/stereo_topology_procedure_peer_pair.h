/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#ifndef STEREO_TOPOLOGY_PROC_PEER_PAIR_H
#define STEREO_TOPOLOGY_PROC_PEER_PAIR_H

#include "stereo_topology_procedures.h"
#include "stereo_topology_private.h"
#include "script_engine.h"

extern const procedure_fns_t stereo_proc_peer_pair_fns;

extern const procedure_script_t stereo_peer_pair_script;

typedef enum
{
    PROC_SEND_STEREO_TOPOLOGY_MESSAGE_PEER_PAIR_FINISHED = STEREOTOP_INTERNAL_PROCEDURE_RESULTS_MSG_BASE | stereo_topology_procedure_peer_pair,
} stereo_proc_peer_pair_messages_t;


#endif /* STEREO_TOPOLOGY_PROC_PEER_PAIR_H */
