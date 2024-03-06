/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
    All Rights Reserved.
    Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup tmap_profile
    \brief   TMAP client source message handler
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "tmap_client_source_private.h"
#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#include "tmap_client_source_broadcast_private.h"
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#include "tmap_client_source_unicast_private.h"
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#define TMAP_LOG     DEBUG_LOG

/*! \brief Process notifications received from TMAP library */
static void tmapClientSourceMessageHandler_HandleTmapMessage(Message message)
{
    CsrBtCmPrim tmap_id = *(CsrBtCmPrim *)message;

    switch (tmap_id)
    {
        case TMAP_CLIENT_INIT_CFM:
        case TMAP_CLIENT_DESTROY_CFM:
        case TMAP_CLIENT_ROLE_CFM:
        case TMAP_CLIENT_REGISTER_CAP_CFM:
        case TMAP_CLIENT_VOLUME_STATE_IND:
            tmapClientSource_HandleTmapProfileMessage(message);
        break;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        case TMAP_CLIENT_UNICAST_CONNECT_CFM:
        case TMAP_CLIENT_UNICAST_START_STREAM_IND:
        case TMAP_CLIENT_UNICAST_START_STREAM_CFM:
        case TMAP_CLIENT_UNICAST_STOP_STREAM_CFM:
        case TMAP_CLIENT_UNICAST_DISCONNECT_CFM:
            tmapClientSource_HandleTmapUnicastMessage(message);
        break;
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
        case TMAP_CLIENT_BROADCAST_SRC_INIT_CFM:
        case TMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM:
        case TMAP_CLIENT_BROADCAST_SRC_START_STREAM_CFM:
        case TMAP_CLIENT_BROADCAST_SRC_UPDATE_STREAM_CFM:
        case TMAP_CLIENT_BROADCAST_SRC_STOP_STREAM_CFM:
        case TMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_START_SRC_SCAN_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND:
        case TMAP_CLIENT_BROADCAST_ASST_STOP_SRC_SCAN_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_READ_BRS_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_BRS_IND:
        case TMAP_CLIENT_BROADCAST_ASST_START_SYNC_TO_SRC_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_CANCEL_SYNC_TO_SRC_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_SET_CODE_IND:
        case TMAP_CLIENT_BROADCAST_SRC_DEINIT_CFM:
        case TMAP_CLIENT_BROADCAST_ASST_READ_BRS_IND:
        case TMAP_CLIENT_SET_PARAMS_CFM:
            tmapClientSourceBroadcast_HandleTmapMessage(message);
        break;
#endif  /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

        default:
            TMAP_LOG("tmapClientSourceMessageHandler_HandleTmapMessage Unhandled message id: 0x%x", tmap_id);
        break;
    }
}

/*! \brief Common handler that receives all message from TMAP library */
void tmapClientSourceMessageHandler_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    TMAP_LOG("tmapClientSource_HandleMessage Received Message Id : 0x%x", id);

    switch (id)
    {
        case TMAP_CLIENT_PROFILE_PRIM:
            tmapClientSourceMessageHandler_HandleTmapMessage(message);
        break;

        default:
        break;
    }
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */
