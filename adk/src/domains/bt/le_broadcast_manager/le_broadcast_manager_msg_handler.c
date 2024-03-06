/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      Message Handler for LE Broadcast Manager.
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "le_broadcast_manager_msg_handler.h"

#include "le_broadcast_manager_scan_delegator.h"
#include "le_broadcast_manager_source.h"
#include "le_broadcast_manager_sync.h"
#include "le_broadcast_manager.h"

#include "domain_message.h"
#include "le_audio_messages.h"

#include "av.h"
#include "mirror_profile.h"
#include "telephony_messages.h"

#include <connection.h>
#include <logging.h>

#ifdef USE_SYNERGY
#include "cm_lib.h"
#include "gatt_lib.h"
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif


#define BROADCAST_MANAGER_MSG_LOG     DEBUG_LOG

void LeBroadcastManager_MsgHandlerLeabmMessages(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    BROADCAST_MANAGER_MSG_LOG("LeBroadcastManager_MsgHandlerLeabmMessages ID:0x%x", id);

    switch (id)
    {
        case BROADCAST_MANAGER_INTERNAL_MSG_PAST_TIMEOUT:
            LeBroadcastManager_HandleInternalMsgPastTimeout();
            break;

		case BROADCAST_MANAGER_INTERNAL_MSG_SOURCES_SYNC_CHECK:
            LeBroadcastManager_HandleInternalMsgSourcesSyncCheck();
            break;
            
        case BROADCAST_MANAGER_INTERNAL_MSG_ADD_SOURCE:
            LeBroadcastManager_SourceAdd((scan_delegator_client_add_broadcast_source_t *) message);
            LeBroadcastManager_ScanDelegatorFreeSourceAddMemory((scan_delegator_client_add_broadcast_source_t *) message);
            break;
            
        case BROADCAST_MANAGER_INTERNAL_MSG_RESYNC_TO_LOST_PA:
            LeBroadcastManager_HandleInternalMsgResyncToLostPa();
            break;

        case BROADCAST_MANAGER_INTERNAL_UNMUTE_TIMEOUT:
            LeBroadcastManager_SyncHandleInternalUnmuteTimeout();
            break;

        case LE_BROADCAST_MANAGER_STOP_CFM:
            LeBroadcastManager_HandleStopConfirm();
            break;

        default:
            /* Use hfp_profile and av notifications to know when a voice call
               or a2dp music starts and ends. This should be replaced by the
               audio router interrupting and resuming the broadcast source. */
            if (   ID_IN_MSG_GRP(AV, id)
                || ID_IN_MSG_GRP(MIRROR_PROFILE, id)
                || ID_IN_MSG_GRP(LE_AUDIO, id)
                || ID_IN_MSG_GRP(TELEPHONY, id))
            {
                LeBroadcastManager_MsgHandlerAvMessages(id, message);
            }
            else
            {
                LeBroadcastManager_MsgHandlerBapCmClPrim(id, (void*)message);
            }
            break;
    }
}

void LeBroadcastManager_MsgHandlerAvMessages(MessageId id, Message message)
{
    UNUSED(message);
    
    BROADCAST_MANAGER_MSG_LOG("LeBroadcastManager_MsgHandlerAvMessages MESSAGE:0x%x", id);

    switch (id)
    {
#ifdef INCLUDE_LE_AUDIO_UNICAST
    case LE_AUDIO_UNICAST_MEDIA_CONNECTED:
#endif
    case TELEPHONY_AUDIO_CONNECTED:
    case MIRROR_PROFILE_ESCO_CONNECT_IND:
    case AV_STREAMING_ACTIVE_IND:
    case MIRROR_PROFILE_A2DP_STREAM_ACTIVE_IND:
        LeBroadcastManager_SourceHandleAudioOrVoiceConnectedInd();
        break;

    case TELEPHONY_INCOMING_CALL:
        LeBroadcastManager_SourceHandleIncomingCallStarted();
        break;

#ifdef INCLUDE_LE_AUDIO_UNICAST
    case LE_AUDIO_UNICAST_MEDIA_DISCONNECTED:
#endif
    case TELEPHONY_AUDIO_DISCONNECTED:
    case TELEPHONY_CALL_ENDED:
    case MIRROR_PROFILE_ESCO_DISCONNECT_IND:
    case AV_STREAMING_INACTIVE_IND:
    case MIRROR_PROFILE_A2DP_STREAM_INACTIVE_IND:
        LeBroadcastManager_SourceHandleAudioOrVoiceDisconnectedInd();
        break;

    case TELEPHONY_INCOMING_CALL_ENDED:
        LeBroadcastManager_SourceHandleIncomingCallEnded();
        break;

    default:
        break;
    }
}

void LeBroadcastManager_MsgHandlerBapCmClPrim(MessageId id, void* message)
{
    BROADCAST_MANAGER_MSG_LOG("LeBroadcastManager_MsgHandlerBapCmClPrim ID:0x%x", id);
#ifdef USE_SYNERGY
    uint16 msgId = 0;
    if ((id == CM_PRIM) || (id == BAP_SRVR_PRIM))
    {
        msgId = *((uint16*) message);
    }
    switch(msgId)
#else
    switch(id)
#endif
    {
        case BAP_SERVER_SETUP_DATA_PATH_CFM:
            {
                LeBroadcastManager_HandleSetupIsoDataPathCfm((BapServerSetupDataPathCfm *)message);
            }
            break;
        case BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM:
            {
                LeBroadcastManager_HandleIsocBigCreateSyncCfm((BapServerIsocBigCreateSyncCfm *)message);

                ((BapServerIsocBigCreateSyncCfm *)message)->bisHandles = NULL;
            }
            break;
        case BAP_SERVER_BIGINFO_ADV_REPORT_IND:
             LeBroadcastManager_SourceBigInfoReportReceived(((BapServerBigInfoAdvReportInd *)message)->syncHandle,
                                ((BapServerBigInfoAdvReportInd *)message)->maxSdu,
                                ((BapServerBigInfoAdvReportInd *)message)->encryption,
                                ((BapServerBigInfoAdvReportInd *)message)->isoInterval);
             break;
        case BAP_SERVER_ISOC_BIG_TERMINATE_SYNC_IND:
            LeBroadcastManager_SourceBigSyncLoss(((BapServerIsocBigTerminateSyncInd *)message)->bigHandle,
                ((BapServerIsocBigTerminateSyncInd *)message)->status == hci_error_conn_terminated_due_to_mic_failure ? TRUE : FALSE);
            break;
#ifdef USE_SYNERGY
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_CFM:
            {
                CL_DM_BLE_PERIODIC_SCAN_SYNC_TERMINATE_CFM_T cfm;
                cfm.status = ((CmPeriodicScanSyncTerminateCfm *)message)->resultCode;
                cfm.sync_handle= ((CmPeriodicScanSyncTerminateCfm *)message)->syncHandle;
                LeBroadcastManager_SourcePaSyncTerminateCfm(&cfm);
                CmFreeUpstreamMessageContents(message);
            }
            break;
#else
        case CL_DM_BLE_PERIODIC_SCAN_SYNC_TERMINATE_CFM:
            LeBroadcastManager_SourcePaSyncTerminateCfm((CL_DM_BLE_PERIODIC_SCAN_SYNC_TERMINATE_CFM_T *)message);
            break;
#endif
        default:
            break;
    }
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) */
