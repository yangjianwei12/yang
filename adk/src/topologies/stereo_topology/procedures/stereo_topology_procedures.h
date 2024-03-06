/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Definition of STEREO topology specific procedures.
*/

#ifndef STEREO_TOPOLOGY_PROCEDURES_H
#define STEREO_TOPOLOGY_PROCEDURES_H

#include "procedures.h"


/*! Definition of STEREO Topology procedures. 

    A naming convention is followed for important procedures. 
    Following the convention and using in the recommended order
    reduces the possibility of unexpected behaviour.

    The convention applies to functions that enable or disable 
    activity.
    \li allow changes the response if an event happens.
    \li permit starts or stops an activity temporarily
    \li enable Starts or stops an activity permanently

    If several of these functions are called with DISABLE parameters
    it is recommended that they are called in the order 
    allow - permit - enable.
*/

typedef enum
{
    stereo_topology_procedure_enable_connectable_handset = 1,

    stereo_topology_procedure_allow_handset_connection,

    stereo_topology_procedure_disconnect_handset,

    stereo_topology_procedure_allow_le_connection,

    stereo_topology_procedure_system_stop,

    stereo_topology_procedure_start_stop_script,

    stereo_topology_procedure_send_message_to_topology,

    stereo_topology_procedure_disconnect_le,

    /*! Procedure to stop handset reconnection */
    stereo_topology_procedure_stop_handset_reconnect,

    stereo_topology_proc_stop_le_broadcast,

    stereo_topology_procedure_peer_pair,

    stero_topology_procedure_setup_le_peer_pair,

    stereo_topology_procedure_find_role,

    stereo_topology_procedure_become_primary,

    /*! Procedure to connect BREDR ACL to Primary */
    stereo_topology_procedure_sec_connect_peer,

    /*! Procedure to connect BREDR profiles to Secondary. */
    stereo_topology_procedure_pri_connect_peer_profiles,

    /*! Procedure to enable page scan for Secondary to establish BREDR ACL to Primary. */
    stereo_topology_procedure_enable_connectable_peer,

    stereo_topology_procedure_enable_stereo_standalone,

} stereo_topology_procedure;


#endif /* STEREO_TOPOLOGY_PROCEDURES_H */
