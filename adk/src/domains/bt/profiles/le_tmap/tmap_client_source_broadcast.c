/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    tmap_profile
    \brief      Implementation of broadcast source
*/

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#include "tmap_client_source_broadcast.h"
#include "tmap_client_source_broadcast_private.h"
#include "tmap_client_source_private.h"

#define TMAP_LOG     DEBUG_LOG

/*! \brief Audio Location representing both left and right in a single BIS */
#define TMAP_AUDIO_LOCATION_LEFT_AND_RIGHT              (CAP_CLIENT_AUDIO_LOCATION_FL | CAP_CLIENT_AUDIO_LOCATION_FR)

/*! Returns the tmap client broadcast context data */
#define tmapClientSourceBroadcast_GetContext()          (&TmapClientSource_GetContext()->broadcast_data)

/*! \brief Default periodic advertisement interval to use (PA interval = N * 1.25ms) */
#define TMAP_BROADCAST_PA_INTERVAL_DEFAULT           360    /* 450ms */

static void tmapClientSourceBroadcast_SendCommonResp(tmap_client_broadcast_msg_id_t id, CapClientResult result)
{
    tmap_client_broadcast_msg_t msg;

    TMAP_LOG("tmapClientSourceBroadcast_SendCommonResp enum:tmap_client_broadcast_msg_id_t:%d, status:%d", id, result);

    msg.id = id;
    msg.body.common_cfm.status = result == TMAP_CLIENT_STATUS_SUCCESS ?
                                    TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS : TMAP_CLIENT_BROADCAST_MSG_STATUS_FAILED;

    tmapClientSourceBroadcast_GetContext()->callback_handler(&msg);
}

static void tmapClientSourceBroadcast_HandleTmapInitCfm(const TmapClientBroadcastSrcInitCfm *cfm)
{
    tmap_client_broadcast_msg_t msg;

    TMAP_LOG("tmapClientSourceBroadcast_HandleTmapInitCfm status:0x%x, handle: 0x%x", cfm->result, cfm->bcastSrcProfileHandle);

    msg.id = TMAP_CLIENT_MSG_ID_BROADCAST_INIT_COMPLETE;

    if (cfm->result == TMAP_CLIENT_STATUS_SUCCESS)
    {
        tmapClientSourceBroadcast_GetContext()->handle = cfm->bcastSrcProfileHandle;
        msg.body.init_complete.handle = cfm->bcastSrcProfileHandle;
        msg.body.init_complete.status = TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS;
    }
    else
    {
        tmapClientSourceBroadcast_GetContext()->handle = TMAP_CLIENT_SOURCE_INVALID_BROADCAST_HANDLE;
        msg.body.init_complete.status = TMAP_CLIENT_BROADCAST_MSG_STATUS_FAILED;
    }

    tmapClientSourceBroadcast_GetContext()->callback_handler(&msg);
}

static void tmapClientSourceBroadcast_SendConfigComplete(const TmapClientBroadcastSrcConfigCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_CONFIG_COMPLETE, cfm->result);
}

static void tmapClientSourceBroadcast_SendStreamingStartCfm(const TmapClientBroadcastSrcStartStreamCfm *cfm)
{
    tmap_client_broadcast_msg_t msg;
    uint8 index;

    TMAP_LOG("tmapClientSourceBroadcast_SendStreamingStartCfm status:%d", cfm->result);

    msg.id = TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_START_CFM;
    msg.body.stream_start.status = cfm->result == TMAP_CLIENT_STATUS_SUCCESS ?
                                       TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS : TMAP_CLIENT_BROADCAST_MSG_STATUS_FAILED;
    msg.body.stream_start.num_bis = cfm->subGroupInfo->numBis;
    msg.body.stream_start.frame_duration = cfm->subGroupInfo->audioConfig->frameDuaration;
    msg.body.stream_start.sampling_frequency = cfm->subGroupInfo->audioConfig->samplingFrequency;
    msg.body.stream_start.octets_per_frame = cfm->subGroupInfo->audioConfig->octetsPerFrame;

    PanicFalse(cfm->subGroupInfo->numBis <= TMAP_BROADCAST_MAX_SUPPORTED_BIS);

    for(index = 0; index < cfm->subGroupInfo->numBis; index++)
    {
        msg.body.stream_start.bis_handles[index] = cfm->subGroupInfo->bisHandles[index];
    }
    /* copy the BIG transport latency which is required to have audio sync in case of product supporting
       concurrency */
    msg.body.stream_start.transport_latency_big = cfm->bigParameters->transportLatencyBig;
    msg.body.stream_start.iso_interval = cfm->bigParameters->isoInterval;

    tmapClientSourceBroadcast_GetContext()->callback_handler(&msg);

    pfree(cfm->bigParameters);
    if (cfm->subGroupInfo != NULL)
    {
        pfree(cfm->subGroupInfo->audioConfig);
        pfree(cfm->subGroupInfo->bisHandles);
        pfree(cfm->subGroupInfo->metadata);
        pfree(cfm->subGroupInfo);
    }
}

static void tmapClientSourceBroadcast_SendStreamingStopCfm(const TmapClientBroadcastSrcStopStreamCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_STOP_CFM, cfm->result);
}

static void tmapClientSourceBroadcast_SendDeinitCfm(const TmapClientBroadcastSrcDeinitCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_DEINIT_COMPLETE, cfm->result);
}

static void tmapClientSourceBroadcast_HandleSrcUpdateStreamCfm(const TmapClientBroadcastSrcUpdateStreamCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_SRC_STREAM_UPDATE, cfm->result);
}


static void tmapClientSourceBroadcast_HandleSrcRemoveStreamCfm(const TmapClientBroadcastSrcRemoveStreamCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_SRC_STREAM_REMOVE, cfm->result);
}

static void tmapClientSourceBroadcast_HandleSetBcastConfigParamsCfm(const TmapClientSetParamsCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_SRC_BCAST_PARAM_SET, cfm->result);
}

static void tmapClientSourceBroadcast_SendScanningStartCfm(const TmapClientBroadcastAsstStartSrcScanCfm *cfm)
{
    tmap_client_broadcast_msg_t msg;

    TMAP_LOG("tmapClientSourceBroadcast_SendScanningStartCfm status:%d", cfm->result);

    msg.id = TMAP_CLIENT_MSG_ID_BROADCAST_ASST_START_SCAN_CFM;
    msg.body.scan_start.status = cfm->result == TMAP_CLIENT_STATUS_SUCCESS ?
                                     TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS : TMAP_CLIENT_BROADCAST_MSG_STATUS_FAILED;
    tmapClientSourceBroadcast_GetContext()->asst_params.scan_handle = cfm->scanHandle;

    tmapClientSourceBroadcast_GetContext()->callback_handler(&msg);
    pfree(cfm->status);
}

static void tmapClientSourceBroadcast_SendSourceScanningReport(const TmapClientBroadcastAsstSrcReportInd *report)
{
    tmap_client_broadcast_msg_t msg;

    TMAP_LOG("tmapClientSourceBroadcast_SendSourceScanningReport broadcast_id:%d adv_handle:%d adv_sid %d", report->broadcastId,
             report->advHandle, report->advSid);

    msg.id = TMAP_CLIENT_MSG_ID_BROADCAST_ASST_SCAN_REPORT;
    msg.body.src_scan_report.cid = report->cid;
    msg.body.src_scan_report.source_addr = report->sourceAddrt;
    msg.body.src_scan_report.adv_sid = report->advSid;
    msg.body.src_scan_report.adv_handle = report->advHandle;
    msg.body.src_scan_report.collocated = report->collocated;
    msg.body.src_scan_report.broadcast_id = report->broadcastId;
    msg.body.src_scan_report.num_subgroup = report->numSubgroup;
    msg.body.src_scan_report.subgroup_info = report->subgroupInfo;

    tmapClientSourceBroadcast_GetContext()->callback_handler(&msg);
    pfree(report->bigName);
}

static void tmapClientSourceBroadcast_SendScanningStopCfm(const TmapClientBroadcastAsstStopSrcScanCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_ASST_STOP_SCAN_CFM, cfm->result);
    pfree(cfm->status);
}

static void tmapClientSourceBroadcast_SendRegisterNotificationCfm(const TmapClientBroadcastAsstRegisterNotificationCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM, cfm->result);
    pfree(cfm->status);
}

static void tmapClientSourceBroadcast_AddSourceCfm(const TmapClientBroadcastAsstAddSrcCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_ASST_ADD_SOURCE_CFM, cfm->result);
    pfree(cfm->status);
}

static void tmapClientSourceBroadcast_ModifySourceCfm(const TmapClientBroadcastAsstModifySrcCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_ASST_MODIFY_SOURCE_CFM, cfm->result);
    pfree(cfm->status);
}

static void tmapClientSourceBroadcast_RemoveSourceCfm(const TmapClientBroadcastAsstRemoveSrcCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_ASST_REMOVE_SOURCE_CFM, cfm->result);
    pfree(cfm->status);
}

static void tmapClientSourceBroadcast_ReadBrsCfm(const TmapClientBroadcastAsstReadBrsCfm *cfm)
{
    tmapClientSourceBroadcast_SendCommonResp(TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_READ_CFM, cfm->result);
}

static void tmapClientSourceBroadcast_ReadBrsInd(const TmapClientBroadcastAsstReadBrsInd *ind)
{
    tmap_client_broadcast_msg_t msg;
    TYPED_BD_ADDR_T source_bd_address;

    TMAP_LOG("tmapClientSourceBroadcast_ReadBrsInd source_id:%d pa_sync_state:0x%x"
             " advSid :%d bcast_id:%d addtype:%d addr: %x %x %x",
              ind->sourceId, ind->paSyncState, ind->advSid, ind->broadcastId, ind->advertiseAddType,
              ind->sourceAddress.lap, ind->sourceAddress.nap, ind->sourceAddress.uap);

    msg.id = TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_READ_IND;
    msg.body.brs_read_ind.adv_sid = ind->advSid;
    msg.body.brs_read_ind.broadcast_id = ind->broadcastId;
    msg.body.brs_read_ind.source_id = ind->sourceId;
    msg.body.brs_read_ind.pa_sync_state = ind->paSyncState;
    msg.body.brs_read_ind.big_encryption = ind->bigEncryption;
    msg.body.brs_read_ind.status =  ind->result == TMAP_CLIENT_STATUS_SUCCESS ? TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS :
                                                                                TMAP_CLIENT_BROADCAST_MSG_STATUS_FAILED;

    source_bd_address.type = ind->advertiseAddType;
    source_bd_address.addr = ind->sourceAddress;
    BdaddrConvertTypedBluestackToVm(&msg.body.brs_read_ind.source_address, &source_bd_address);

    tmapClientSourceBroadcast_GetContext()->callback_handler(&msg);

    pfree(ind->badCode);
    if (ind->subGroupInfo != NULL)
    {
        pfree(ind->subGroupInfo->metadataValue);
        pfree(ind->subGroupInfo);
    }
}

static void tmapClientSourceBroadcast_BrsInd(const TmapClientBroadcastAsstBrsInd *ind)
{
    tmap_client_broadcast_msg_t msg;
    TYPED_BD_ADDR_T source_bd_address;

    TMAP_LOG("tmapClientSourceBroadcast_BrsInd source_id:%d pa_sync_state:0x%x bis_index:0x%x",
              ind->sourceId, ind->paSyncState, ind->subGroupInfo != NULL ? ind->subGroupInfo->bisIndex : 0);

    msg.id = TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_IND;
    msg.body.brs_ind.adv_sid = ind->advSid;
    msg.body.brs_ind.broadcast_id = ind->broadcastId;
    msg.body.brs_ind.source_id = ind->sourceId;
    msg.body.brs_read_ind.pa_sync_state = ind->paSyncState;
    msg.body.brs_read_ind.big_encryption = ind->bigEncryption;
    msg.body.brs_read_ind.status =  TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS;

    source_bd_address.type = ind->advertiseAddType;
    source_bd_address.addr = ind->sourceAddress;
    BdaddrConvertTypedBluestackToVm(&msg.body.brs_ind.source_address, &source_bd_address);

    tmapClientSourceBroadcast_GetContext()->callback_handler(&msg);

    pfree(ind->badCode);
    if (ind->subGroupInfo != NULL)
    {
        pfree(ind->subGroupInfo->metadataValue);
        pfree(ind->subGroupInfo);
    }
}

static TmapClientBigConfigMode tmapClientSourceBroadcast_GetBigConfigMode(const TmapClientBigSubGroup * sub_group_info)
{
    TmapClientBigConfigMode big_config_mode = TMAP_CLIENT_BIG_CONFIG_MODE_DEFAULT;

    if (sub_group_info->numBis == 1 && sub_group_info->bisInfo[0].audioLocation == TMAP_AUDIO_LOCATION_LEFT_AND_RIGHT)
    {
        big_config_mode |= 2 /* TMAP_CLIENT_BIG_CONFIG_MODE_JOINT_STEREO*/;
    }

    return big_config_mode;
}

/*! \brief Handler to process broadcast related messages received from TMAP library */
void tmapClientSourceBroadcast_HandleTmapMessage(Message message)
{
    CsrBtCmPrim tmap_id = *(CsrBtCmPrim *)message;

    switch (tmap_id)
    {
        case TMAP_CLIENT_BROADCAST_SRC_INIT_CFM:
             tmapClientSourceBroadcast_HandleTmapInitCfm((const TmapClientBroadcastSrcInitCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM:
             tmapClientSourceBroadcast_SendConfigComplete((const TmapClientBroadcastSrcConfigCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_SRC_START_STREAM_CFM:
             tmapClientSourceBroadcast_SendStreamingStartCfm((const TmapClientBroadcastSrcStartStreamCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_SRC_UPDATE_STREAM_CFM:
             tmapClientSourceBroadcast_HandleSrcUpdateStreamCfm((const TmapClientBroadcastSrcUpdateStreamCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_SRC_STOP_STREAM_CFM:
             tmapClientSourceBroadcast_SendStreamingStopCfm((const TmapClientBroadcastSrcStopStreamCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM:
             tmapClientSourceBroadcast_HandleSrcRemoveStreamCfm((const TmapClientBroadcastSrcRemoveStreamCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_SRC_DEINIT_CFM:
             tmapClientSourceBroadcast_SendDeinitCfm((const TmapClientBroadcastSrcDeinitCfm *) message);
        break;

        case TMAP_CLIENT_SET_PARAMS_CFM:
             tmapClientSourceBroadcast_HandleSetBcastConfigParamsCfm((const TmapClientSetParamsCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_ASST_START_SRC_SCAN_CFM:
            tmapClientSourceBroadcast_SendScanningStartCfm((const TmapClientBroadcastAsstStartSrcScanCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND:
            tmapClientSourceBroadcast_SendSourceScanningReport((const TmapClientBroadcastAsstSrcReportInd *) message);
        break;

        case TMAP_CLIENT_BROADCAST_ASST_STOP_SRC_SCAN_CFM:
            tmapClientSourceBroadcast_SendScanningStopCfm((const TmapClientBroadcastAsstStopSrcScanCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM:
            tmapClientSourceBroadcast_SendRegisterNotificationCfm((const TmapClientBroadcastAsstRegisterNotificationCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM:
            tmapClientSourceBroadcast_AddSourceCfm((const TmapClientBroadcastAsstAddSrcCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM:
            tmapClientSourceBroadcast_ModifySourceCfm((const TmapClientBroadcastAsstModifySrcCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM:
            tmapClientSourceBroadcast_RemoveSourceCfm((const TmapClientBroadcastAsstRemoveSrcCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_ASST_READ_BRS_IND:
            tmapClientSourceBroadcast_ReadBrsInd((const TmapClientBroadcastAsstReadBrsInd *) message);
        break;

        case TMAP_CLIENT_BROADCAST_ASST_READ_BRS_CFM:
            tmapClientSourceBroadcast_ReadBrsCfm((const TmapClientBroadcastAsstReadBrsCfm *) message);
        break;

        case TMAP_CLIENT_BROADCAST_ASST_BRS_IND:
            tmapClientSourceBroadcast_BrsInd((const TmapClientBroadcastAsstBrsInd *) message);
        break;

        default:
            TMAP_LOG("tmapClientSourceBroadcast_HandleTmapMessage Unhandled message id: 0x%x", (*(CsrBtCmPrim *) message));
        break;
    }
}

void TmapClientSourceBroadcast_Init(void)
{
    TMAP_LOG("TmapClientSourceBroadcast_Init");

    TmapClientSource_GetContext()->task_data.handler = tmapClientSourceMessageHandler_HandleMessage;
    TmapClientBroadcastSrcInitReq(TrapToOxygenTask((Task) &TmapClientSource_GetContext()->task_data));
}

void TmapClientSourceBroadcast_RegisterCallback(tmap_client_source_broadcast_callback_handler_t handler)
{
    tmapClientSourceBroadcast_GetContext()->callback_handler = handler;
}

void TmapClientSourceBroadcast_Configure(TmapClientProfileHandle handle,
                                         uint32 presentation_delay,
                                         uint8 num_subgroup,
                                         const TmapClientBigSubGroup *subgroup_info,
                                         uint8 source_name_len,
                                         const char *source_name)
{
    TmapClientBroadcastSrcConfigReq(handle, CSR_BT_ADDR_PUBLIC, presentation_delay, num_subgroup,
                                    subgroup_info,
                                    source_name_len,
                                    (const uint8 *) source_name,
                                    tmapClientSourceBroadcast_GetBigConfigMode(subgroup_info), NULL);
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
void TmapClientSourceBroadcast_AddSource(ServiceHandle cap_group_id,
                                         uint32 cid,
                                         typed_bdaddr source_taddr,
                                         uint8 adv_handle,
                                         uint8 adv_sid,
                                         uint32 broadcast_id,
                                         uint32 bis_index)
{
    TYPED_BD_ADDR_T source_bd_address;
    TmapClientSubgroupInfo subgroup_info[TMAP_BROADCAST_NUMBER_OF_SUBGROUP_SUPPORTED];

    subgroup_info[0].bisIndex = bis_index;
    subgroup_info[0].metadataLen = 0;
    subgroup_info[0].metadataValue = NULL;
    BdaddrConvertTypedVmToBluestack(&source_bd_address, &source_taddr);


    TMAP_LOG("TmapClientSourceBroadcast_AddSource cid 0x%x broadcast_id:%d",
              cid, broadcast_id);

    TmapClientBroadcastAsstAddSrcReq(TmapClientSource_GetProfileHandle(),
                                     cap_group_id,
                                     cid,
                                     &source_bd_address,
                                     TMAP_CLIENT_BROADCAST_SRC_COLLOCATED,
                                     adv_handle,
                                     adv_sid,
                                     CAP_CLIENT_PA_SYNC_SYNCHRONIZE_PAST,
                                     TMAP_BROADCAST_PA_INTERVAL_DEFAULT,
                                     broadcast_id,
                                     TMAP_BROADCAST_NUMBER_OF_SUBGROUP_SUPPORTED,
                                     subgroup_info);
}

void TmapClientSourceBroadcast_ModifySource(ServiceHandle cap_group_id,
                                            uint32 cid,
                                            uint8 adv_handle,
                                            uint8 adv_sid,
                                            uint32 bis_index,
                                            uint8 source_id,
                                            bool pa_sync_enable)
{
    TmapClientBroadcastSinkInfo brcast_sink_info[TMAP_BROADCAST_NUMBER_OF_SINK_INFO_SUPPORTED];
    TmapClientSubgroupInfo subgroup_info[TMAP_BROADCAST_NUMBER_OF_SUBGROUP_SUPPORTED];

    subgroup_info[0].bisIndex = bis_index;
    subgroup_info[0].metadataLen = 0;
    subgroup_info[0].metadataValue = NULL;
    brcast_sink_info[0].cid = cid;
    brcast_sink_info[0].sourceId = source_id;

    TMAP_LOG("TmapClientSourceBroadcast_ModifySource cid 0x%x source_id:%d",
              cid, source_id);

    TmapClientBroadcastAsstModifySrcReq(TmapClientSource_GetProfileHandle(),
                                        cap_group_id,
                                        TMAP_CLIENT_BROADCAST_SRC_COLLOCATED,
                                        adv_handle,
                                        adv_sid,
                                        pa_sync_enable ? CAP_CLIENT_PA_SYNC_SYNCHRONIZE_PAST :
                                                         CAP_CLIENT_PA_SYNC_NOT_SYNCHRONIZE,
                                        TMAP_BROADCAST_PA_INTERVAL_DEFAULT,
                                        TMAP_BROADCAST_NUMBER_OF_SINK_INFO_SUPPORTED,
                                        brcast_sink_info,
                                        TMAP_BROADCAST_NUMBER_OF_SUBGROUP_SUPPORTED,
                                        subgroup_info);
}

void TmapClientSourceBroadcast_RemoveSource(ServiceHandle cap_group_id, uint32 cid, uint8 source_id)
{
    TmapClientBroadcastSinkInfo brcast_sink_info[TMAP_BROADCAST_NUMBER_OF_SINK_INFO_SUPPORTED];

    TMAP_LOG("TmapClientSourceBroadcast_RemoveSource cid 0x%x source_id:%d",
              cid, source_id);

    brcast_sink_info[0].cid = cid;
    brcast_sink_info[0].sourceId = source_id;
    TmapClientBroadcastAsstRemoveSrcReq(TmapClientSource_GetProfileHandle(),
                                        cap_group_id,
                                        TMAP_BROADCAST_NUMBER_OF_SINK_INFO_SUPPORTED,
                                        brcast_sink_info);
}


void TmapClientSourceBroadcast_StartScanningForSource(ServiceHandle cap_group_id, uint32 cid,
                                                      CapClientContext audio_context)
{
    TmapClientBroadcastAsstStartSrcScanReq(TmapClientSource_GetProfileHandle(),
                                           cap_group_id,
                                           cid,
                                           TMAP_CLIENT_BROADCAST_SRC_COLLOCATED,
                                           audio_context,
                                           TMAP_BROADCAST_SCAN_PARAM_DEFAULT,
                                           TMAP_BROADCAST_SCAN_PARAM_DEFAULT,
                                           TMAP_BROADCAST_SCAN_PARAM_DEFAULT);
}

void TmapClientSourceBroadcast_StopScanningForSource(ServiceHandle cap_group_id, uint32 cid)
{
    TmapClientBroadcastAsstStopSrcScanReq(TmapClientSource_GetProfileHandle(),
                                          cap_group_id,
                                          cid,
                                          tmap_client_source_taskdata.broadcast_data.asst_params.scan_handle);
}

void TmapClientSourceBroadcast_RegisterForGattNotification(ServiceHandle cap_group_id, uint32 cid, uint8 source_id)
{
    TmapClientBroadcastAsstRegisterNotificationReq(TmapClientSource_GetProfileHandle(),
                                                   cap_group_id,
                                                   cid,
                                                   source_id,
                                                   TRUE);
}

void TmapClientSourceBroadcast_ReadReceiverSinkState(ServiceHandle cap_group_id, uint32 cid)
{
    TmapClientBroadcastAsstReadReceiverSinkStateReq(TmapClientSource_GetProfileHandle(),
                                                    cap_group_id, cid);
}
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

void TmapClientSource_UpdateAdvSetting(const CapClientBcastSrcAdvParams *tmap_client_adv_param)
{
    TMAP_LOG("TmapClientSource_UpdateAdvSetting");

    TmapClientBroadcastSrcSetAdvParamsReq(tmapClientSourceBroadcast_GetContext()->handle, tmap_client_adv_param);
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
