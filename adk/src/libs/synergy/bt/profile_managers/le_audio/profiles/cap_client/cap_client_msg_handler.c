/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "cap_client_vcp_handler.h"
#include "cap_client_bap_handler.h"
#include "cap_client_csip_handler.h"
#include "cap_client_micp_handler.h"
#include "cap_client_init.h"
#include "cap_client_add_new_dev.h"
#include "cap_client_service_discovery_handler.h"
#include "cap_client_init_stream_and_control_req.h"
#include "cap_client_init_optional_service_req.h"
#include "cap_client_micp_operation_req.h"
#include "cap_client_remove_device_req.h"
#include "cap_client_discover_audio_capabilities_req.h"
#include "cap_client_unicast_connect_req.h"
#include "cap_client_start_stream_req.h"
#include "cap_client_available_audio_context_req.h"
#include "cap_client_vcp_operation_req.h"
#include "cap_client_micp_operation_req.h"
#include "cap_client_stop_stream_req.h"
#include "cap_client_broadcast_src.h"
#include "cap_client_msg_handler.h"
#include "cap_client_update_audio_req.h"
#include "cap_client_broadcast_assistant_periodic_scan.h"
#include "cap_client_broadcast_assistant_sync_to_adv.h"
#include "cap_client_broadcast_assistant_add_modify_src_req.h"
#include "cap_client_broadcast_assistant_remove_src_req.h"
#include "cap_client_unicast_disconnect_req.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
static void capClientFreeInternalMsgContent(CapClientPrim type,Msg msg)
{
    uint8 i;

    switch (type)
    {
        case CAP_CLIENT_INTERNAL_BCAST_ASST_ADD_SRC_REQ:
        { 
            CapClientInternalBcastAsstAddSrcReq *req =
                      (CapClientInternalBcastAsstAddSrcReq*)msg;

            for (i = 0; i < req->numbSubGroups; i++)
            {
                if (req->subgroupInfo[i])
                {
                    if (req->subgroupInfo[i]->metadataLen &&
                        req->subgroupInfo[i]->metadataValue)
                    {
                        CsrPmemFree(req->subgroupInfo[i]->metadataValue);
                        req->subgroupInfo[i]->metadataValue = NULL;
                    }
                    CsrPmemFree(req->subgroupInfo[i]);
                    req->subgroupInfo[i] = NULL;
                }
            }
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SRC_REQ:
        {
            CapClientInternalBcastAsstModifySrcReq* req =
                          (CapClientInternalBcastAsstModifySrcReq*)msg;

            for (i = 0; i < req->numbSubGroups; i++)
            {
                if (req->subgroupInfo[i])
                {
                    if (req->subgroupInfo[i]->metadataLen &&
                        req->subgroupInfo[i]->metadataValue)
                    {
                        CsrPmemFree(req->subgroupInfo[i]->metadataValue);
                        req->subgroupInfo[i]->metadataValue = NULL;
                    }
                    CsrPmemFree(req->subgroupInfo[i]);
                    req->subgroupInfo[i] = NULL;
                }
            }
        }
        break;
    }
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

static void capClientHandleInternalMessage(CAP_INST *const inst, Msg msg)
{
    CsrBtGattPrim *prim = (CsrBtGattPrim*)msg;
    CsrBtGattPrim type = *(CsrBtGattPrim*)prim;

    switch(type)
    {
#ifdef INSTALL_LEA_UNICAST_CLIENT
        case CAP_CLIENT_INTERNAL_INIT_REQ:
        {
            handleCapClientInitReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_ADD_NEW_DEV_REQ:
        {
            handleCapClientAddnewDevReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_REMOVE_DEV_REQ:
        {
            handleCapClientRemoveDeviceReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_INIT_STREAM_CONTROL_REQ:
        {
            handleCapClientInitializeStreamControlReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_DISCOVER_STREAM_CAP_REQ:
        {
            handleCapClientDiscoverStreamCapabilitiesReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ:
        {
            handleCapClientDiscoverAvailableAudioContextReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ:
        {
            handleUnicastConnectionReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_UNICAST_DISCONNECT_REQ:
        {
            handleUnicastDisconnectReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ:
        {
            handleUnicastStartStreamReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ:
        {
            handleUnicastUpdateAudioReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ:
        {
            handleUnicastStopStreamReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_MUTE_REQ:
        {
            handleMuteReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ:
        {
            handleChangeVolumeReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_INIT_OPTIONAL_SERVICES_REQ:
        {
            handleCapClientInitOptionalServicesReq(inst, msg);
        }
        break;

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        case CAP_CLIENT_INTERNAL_SET_MICP_PROFILE_ATTRIB_HANDLES_REQ:
        {
            handleCapClientSetMicpProfileAttribHandlesReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_SET_MIC_STATE_REQ:
        {
            handleCapClientSetMicStateReq(inst, msg);
        }
        break;
		
		case CAP_CLIENT_INTERNAL_READ_MIC_STATE_REQ:
        {
            handleCapClientReadMicStateReq(inst, msg);
        }
        break;
#endif

        case CAP_CLIENT_INTERNAL_REGISTER_TASK_REQ:
        {
            handleRegisterTaskReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_DEREGISTER_TASK_REQ:
        {
            handleDeRegisterTaskReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_CSIP_READ_REQ:
        {
            handleCsipReadLock(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_READ_VOLUME_STATE_REQ:
        {
            handleReadVolumeStateReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_UNICAST_CIG_TEST_CONFIG_REQ:
        {
            handleUnicastCigTestConfigReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_UNICAST_VS_SET_CONFIG_DATA_REQ:
        {
            handleUnicastSetVsConfigDataReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_PENDING_OP_REQ:
        {
            CapClientGroupInstance* cap = (CapClientGroupInstance*)
                                           CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
            CapClientInternalPendingOpReq* req = (CapClientInternalPendingOpReq*)msg;
            
            /* All the other required details will be already populated during initial validation */

            if (cap && req->pendingOp == CAP_CLIENT_UNICAST_DISCONNECT)
            {
                cap->pendingOp = req->pendingOp;
                capClientUnicastDisableReleaseReq(cap, inst);
            }
        }
        break;
#endif /*INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
        case CAP_CLIENT_INTERNAL_BCAST_SRC_INIT_REQ:
        {
            handleBroadcastSrcInitReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_SRC_DEINIT_REQ:
        {
            handleBroadcastSrcDeinitReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_SRC_CONFIG_REQ:
        {
            handleBroadcastSrcConfigReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_SRC_START_STREAM_REQ:
        {
            handleBroadcastSrcStartStreamReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_SRC_STOP_STREAM_REQ:
        {
            handleBroadcastSrcStopStreamReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_SRC_REMOVE_STREAM_REQ:
        {
            handleBroadcastSrcRemoveStreamReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_SRC_UPDATE_STREAM_REQ:
        {
            handleBroadcastSrcUpdateStreamReq(inst, msg);
        }
        break;
#endif /* INSTALL_LEA_BROADCAST_SOURCE */
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        case CAP_CLIENT_INTERNAL_BCAST_ASST_START_SRC_SCAN_REQ:
        {
            handleBroadcastAssistantStartSrcScanReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_STOP_SRC_SCAN_REQ:
        {
            handleBroadcastAssistantStopSrcScanReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_START_SYNC_TO_SRC_REQ:
        {
            handleBroadcastAssistantSyncToSrcStartReq( inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_CANCEL_SYNC_TO_SRC_REQ:
        {
            handleBroadcastAssistantSyncToSrcCancelReq( inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_TERMINATE_SYNC_TO_SRC_REQ:
        {
            handleBroadcastAssistantTerminateSyncToSrcReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_ADD_SRC_REQ:
        {
            handleBroadcastAssistantAddSrcReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SRC_REQ:
        {
            handleBroadcastAssistantModifySrcReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SRC_REQ:
        {
            handleBroadcastAssistantRemoveSrcReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_UNLOCK_COORDINATED_SET_REQ:
        {
            handleCoordinatedSetUnlockReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_REG_FOR_NOTIFICATION_REQ:
        {
            handleBassRegisterNotificationReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_READ_RECEIVE_STATE_REQ:
        {
            handleReadBroadcastReceiveStateReq(inst, msg);
        }
        break;

        case CAP_CLIENT_INTERNAL_BCAST_ASST_SET_CODE_RSP:
        {
            handleBroadcastAssistantSetBroadcastCodeRsp(inst, msg);
        }
        break;
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

        case CAP_CLIENT_INTERNAL_SET_PARAM_REQ:
        {
            handleSetParamReq(inst, msg);
        }
        break;

        default:
        {
            CAP_CLIENT_WARNING("\n(CAP)capClientHandleInternalMessage: Unrecognized Prim: 0x%x \n\n", *prim);
        }
        break;
    }

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    capClientFreeInternalMsgContent(type, msg);
#endif
}


static void CapClientFreeMsg(uint16 eventClass, Msg msg)
{
    CsrBtGattPrim type = *(CsrBtGattPrim*)msg;


    if (eventClass == CAP_CLIENT_PRIM)
    { 
        /*
         * Free all the CAP Internal Messages other than Unicast requests like
         * CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ, CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ,
         * CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ, CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ,
         * CAP_CLIENT_INTERNAL_UNICAST_CIG_TEST_CONFIG_REQ , CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ,
         * CAP_CLIENT_INTERNAL_UNICAST_DISCONNECT_REQ, CAP_CLIENT_INTERNAL_UNICAST_VS_SET_CONFIG_DATA_REQ,
         * CAP_CLIENT_INTERNAL_INIT_STREAM_CONTROL_REQ, CAP_CLIENT_INTERNAL_DISCOVER_STREAM_CAP_REQ,
         * CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ and CAP_CLIENT_INTERNAL_MUTE_REQ
         * These messages will get queued and will be freed by CAP once they are serviced.
         */

        if ((type >= CAP_CLIENT_INTERNAL_BASE)
            && (type <= CAP_CLIENT_INTERNAL_TOP))
        {
            switch (type)
            {
                 case CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_CIG_TEST_CONFIG_REQ:
                 case CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ:
                 case CAP_CLIENT_INTERNAL_MUTE_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_DISCONNECT_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_VS_SET_CONFIG_DATA_REQ:
                 case CAP_CLIENT_INTERNAL_INIT_STREAM_CONTROL_REQ:
                 case CAP_CLIENT_INTERNAL_DISCOVER_STREAM_CAP_REQ:
                 case CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ:
                    return;
            }

            SynergyMessageFree(eventClass, msg);
            msg = NULL;
        }
    }
    else
    {
        /* Free all the non Internal CAP messages recieved by CAP*/
        SynergyMessageFree(eventClass, msg);
        msg = NULL;
    }
}

void CapClientMsgHandler(void **gash)
{
    uint16 eventClass = 0;
    void *msg = NULL;
    CAP_INST *inst = (CAP_INST*)*gash;

    if(inst == NULL)
        return;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CAP_CLIENT_PRIM:
                capClientHandleInternalMessage(inst, msg);
                break;
            case BAP_PRIM:
                capClientHandleBapMsg(inst, msg);
                break;

#ifdef INSTALL_LEA_UNICAST_CLIENT
            case GATT_SRVC_DISC_PRIM:
                capClientHandleGattSrvcDiscMsg(inst, msg);
                break;
            case VCP_PRIM:
                capClientHandleVcpMsg(inst, msg);
                break;
            case CSIP_PRIM:
                capClientHandleCsipMsg(inst, msg);
                break;

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
            case MICP_PRIM:
                capClientHandleMicpMsg(inst, msg);
                break;
#endif /* EXCLUDE_CSR_BT_MICP_MODULE */
#endif /* INSTALL_LEA_UNICAST_CLIENT */
            default:
                CAP_CLIENT_WARNING("(CAP) CapClientMsgHandler :Profile Msg not handled \n");
               break;
        }

        CapClientFreeMsg(eventClass, msg);
    }
}

