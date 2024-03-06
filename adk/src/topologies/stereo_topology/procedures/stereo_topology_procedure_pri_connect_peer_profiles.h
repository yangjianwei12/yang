/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      This procedure connects the speaker to spreaker profiles.
*/

#ifndef STEREO_TOPOLOGY_PROC_PRI_CONNECT_PEER_PROFILES_H
#define STEREO_TOPOLOGY_PROC_PRI_CONNECT_PEER_PROFILES_H

#include "stereo_topology_procedures.h"
#include "stereo_topology_procedure_send_topology_message.h"
#include "stereo_topology_private.h"

/*! Structure definining the parameters for the procedure to connect/disconnect
    peer profiles
*/
typedef struct
{
    bool connect;
} PEER_PROFILE_PARAMS_T;


extern const procedure_fns_t proc_pri_connect_peer_profiles_fns;

typedef enum
{
    PROC_SEND_STEREO_TOPOLOGY_MESSAGE_PEER_PROFILE_CONN_FINISHED = STEREOTOP_INTERNAL_PROCEDURE_RESULTS_MSG_BASE | stereo_topology_procedure_pri_connect_peer_profiles,
} stereo_proc_peer_profile_conn_messages_t;

extern STEREOTOP_PROC_SEND_MESSAGE_PARAMS_T stereo_peer_profile_conn_finished;
#define PROC_SEND_STEREO_TOPOLOGY_MESSAGE_PEER_PROFILE_CONN_FINISHED_MESSAGE ((Message)&stereo_peer_profile_conn_finished)

/*! Parameter definition for connecting peer profiles */
extern const PEER_PROFILE_PARAMS_T stereo_topology_procedure_peer_prof_connect;
#define PROC_PEER_PROFILE_CONNECT  ((Message)&stereo_topology_procedure_peer_prof_connect)

/*! Parameter definition for disconnecting peer profiles */
extern const PEER_PROFILE_PARAMS_T stereo_topology_procedure_peer_prof_disconnect;
#define PROC_PEER_PROFILE_DISCONNECT  ((Message)&stereo_topology_procedure_peer_prof_disconnect)



#endif /* STEREO_TOPOLOGY_PROC_PRI_CONNECT_PEER_PROFILES_H */

