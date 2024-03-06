/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Script to stop the system from a topology perspective

            The script bans activity, stops anything that topology controls and then
            sends a message to topology to indicate this has completed.
*/

#include "script_engine.h"

#include "stereo_topology_procedure_stop_handset_reconnect.h"
#include "stereo_topology_procedure_allow_handset_connect.h"
#include "stereo_topology_procedure_enable_connectable_handset.h"
#include "stereo_topology_procedure_allow_le_connection.h"
#include "stereo_topology_procedure_send_topology_message.h"
#include "stereo_topology_procedure_start_stop_script.h"
#include "stereo_topology_procedure_disconnect_handset.h"
#include "stereo_topology_procedure_disconnect_le.h"
#include "stereo_topology_procedure_system_stop.h"
#include "stereo_topology_procedure_stop_le_broadcast.h"

#include <logging.h>

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(stereo_proc_system_stop_messages_t)


const STEREOTOP_PROC_SEND_MESSAGE_PARAMS_T stereo_system_stop_finished = {PROC_SEND_STEREO_TOPOLOGY_MESSAGE_SYSTEM_STOP_FINISHED, (Message)NULL, 0};

#define PROC_SEND_STEREO_TOPOLOGY_MESSAGE_SYSTEM_STOP_FINISHED_MESSAGE ((Message)&stereo_system_stop_finished)

#define STEREO_SYSTEM_STOP_SCRIPT(ENTRY) \
    ENTRY(stereo_proc_start_stop_script_fns, NO_DATA), \
    ENTRY(stereo_proc_stop_handset_reconnect_fns, NO_DATA), \
    ENTRY(stereo_proc_allow_handset_connect_fns, PROC_ALLOW_HANDSET_CONNECT_DATA_DISABLE), \
    ENTRY(stereo_proc_enable_connectable_handset_fns, PROC_ENABLE_CONNECTABLE_HANDSET_DATA_DISABLE), \
    ENTRY(stereo_proc_allow_le_connection_fns,PROC_ALLOW_LE_CONNECTION_DATA_DISABLE), \
    ENTRY(stereo_proc_stop_le_broadcast_fns, NO_DATA), \
    ENTRY(stereo_proc_disconnect_handset_fns, NO_DATA), \
    ENTRY(stereo_proc_disconnect_le_fns, NO_DATA), \
    ENTRY(stereo_proc_send_topology_message_fns, PROC_SEND_STEREO_TOPOLOGY_MESSAGE_SYSTEM_STOP_FINISHED_MESSAGE)

DEFINE_TOPOLOGY_SCRIPT(stereo_system_stop, STEREO_SYSTEM_STOP_SCRIPT);


