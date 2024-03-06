/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    pbp_client_source
    \brief      Implementation of Public Broadcast
*/

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#include "pbp_client_source_private.h"

#define PBP_LOG    DEBUG_LOG

/*! \brief Audio Location representing both left and right in a single BIS */
#define PBP_AUDIO_LOCATION_LEFT_AND_RIGHT              (CAP_CLIENT_AUDIO_LOCATION_FL | CAP_CLIENT_AUDIO_LOCATION_FR)

/*! \brief PBP client source task data. */
pbp_client_source_task_data_t pbp_client_source_taskdata;

/*! \brief Default periodic advertisement interval to use (PA interval = N * 1.25ms) */
#define PBP_PA_INTERVAL_DEFAULT           360    /* 450ms */

#define PBP_CLIENT_SOURCE_FLAGS_ENCRYPTED   1

static void pbpClientSource_SendCommonResp(pbp_client_msg_id_t id, CapClientResult result)
{
    pbp_client_msg_t msg;

    PBP_LOG("pbpClientSource_SendCommonResp enum:pbp_client_msg_id_t:%d, status:%d", id, result);

    msg.id = id;
    msg.body.common_cfm.status = result == PBP_STATUS_SUCCESS ?
                                    PBP_CLIENT_MSG_STATUS_SUCCESS : PBP_CLIENT_MSG_STATUS_FAILED;

    pbp_client_source_taskdata.callback_handler(&msg);
}

static void pbpClientSource_HandlePbpInitCfm(const PbpInitCfm *cfm)
{
    pbp_client_msg_t msg;

    PBP_LOG("pbpClientSource_HandlePbpInitCfm status:0x%x, handle: 0x%x", cfm->status, cfm->prflHndl);

    if (cfm->status == PBP_STATUS_SUCCESS)
    {
        pbp_client_source_taskdata.profile_handle = cfm->prflHndl;

        PbpBroadcastSrcInitReq(cfm->prflHndl);
    }
    else
    {
        msg.id = PBP_CLIENT_MSG_ID_INIT_COMPLETE;
        msg.body.init_complete.handle = PBP_CLIENT_SOURCE_INVALID_HANDLE;
        msg.body.init_complete.status = PBP_CLIENT_MSG_STATUS_FAILED;

        pbp_client_source_taskdata.callback_handler(&msg);
    }
}

static void pbpClientSource_HandlePbpBroadcastInitCfm(const PbpBroadcastSrcInitCfm *cfm)
{
    pbp_client_msg_t msg;

    PBP_LOG("pbpClientSource_HandlePbpBroadcastInitCfm status:0x%x, handle: 0x%x", cfm->result, cfm->bcastSrcProfileHandle);

    msg.id = PBP_CLIENT_MSG_ID_INIT_COMPLETE;

    if (cfm->result == PBP_STATUS_SUCCESS)
    {
        pbp_client_source_taskdata.bcast_handle = cfm->bcastSrcProfileHandle;
        msg.body.src_init_complete.handle = cfm->bcastSrcProfileHandle;
        msg.body.src_init_complete.status = PBP_CLIENT_MSG_STATUS_SUCCESS;
    }
    else
    {
        msg.body.src_init_complete.handle = PBP_CLIENT_SOURCE_INVALID_HANDLE;
        msg.body.src_init_complete.status = PBP_CLIENT_MSG_STATUS_FAILED;
    }

    pbp_client_source_taskdata.callback_handler(&msg);
}

static void pbpClientSource_HandlePbpBroadcastCapRegisterCfm(const PbpRegisterTaskCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_REGISTER_CAP_CFM, cfm->result);
}

static void pbpClientSource_SendConfigComplete(const PbpBroadcastSrcConfigCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_SRC_CONFIG_COMPLETE, cfm->result);
}

static void pbpClientSource_SendStreamingStartCfm(const PbpBroadcastSrcStartStreamCfm *cfm)
{
    pbp_client_msg_t msg;
    uint8 index;

    PBP_LOG("pbpClientSource_SendStreamingStartCfm status:%d", cfm->result);

    msg.id = PBP_CLIENT_MSG_ID_SRC_STREAM_START_CFM;
    msg.body.stream_start.status = cfm->result == PBP_STATUS_SUCCESS ?
                                       PBP_CLIENT_MSG_STATUS_SUCCESS : PBP_CLIENT_MSG_STATUS_FAILED;
    msg.body.stream_start.num_bis = cfm->subGroupInfo->numBis;
    msg.body.stream_start.frame_duration = cfm->subGroupInfo->audioConfig->frameDuaration;
    msg.body.stream_start.sampling_frequency = cfm->subGroupInfo->audioConfig->samplingFrequency;
    msg.body.stream_start.octets_per_frame = cfm->subGroupInfo->audioConfig->octetsPerFrame;

    PanicFalse(cfm->subGroupInfo->numBis <= PBP_MAX_SUPPORTED_BIS);

    for(index = 0; index < cfm->subGroupInfo->numBis; index++)
    {
        msg.body.stream_start.bis_handles[index] = cfm->subGroupInfo->bisHandles[index];
    }
    /* copy the BIG transport latency which is required to have audio sync in case of product supporting
       concurrency */
    msg.body.stream_start.transport_latency_big = cfm->bigParameters->transportLatencyBig;
    msg.body.stream_start.iso_interval = cfm->bigParameters->isoInterval;

    pbp_client_source_taskdata.callback_handler(&msg);

    pfree(cfm->bigParameters);

    if (cfm->subGroupInfo != NULL)
    {
        pfree(cfm->subGroupInfo->audioConfig);
        pfree(cfm->subGroupInfo->bisHandles);
        pfree(cfm->subGroupInfo->metadata);
        pfree(cfm->subGroupInfo);
    }
}

static void pbpClientSource_SendStreamingStopCfm(const PbpBroadcastSrcStopStreamCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_SRC_STREAM_STOP_CFM, cfm->result);
}

static void pbpClientSource_SendDeinitCfm(const PbpBroadcastSrcDeinitCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_SRC_DEINIT_COMPLETE, cfm->result);
}

static void pbpClientSource_HandleSrcUpdateStreamCfm(const PbpBroadcastSrcUpdateAudioCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_SRC_STREAM_UPDATE, cfm->result);
}

static void pbpClientSource_HandleSrcRemoveStreamCfm(const PbpBroadcastSrcRemoveStreamCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_SRC_STREAM_REMOVE, cfm->result);
}

static void pbpClientSource_HandleSrcSetBcastParamsConfigCfm(const PbpBroadcastSrcSetParamCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_SRC_BCAST_PARAM_SET, cfm->result);
}

static void pbpClientSource_SendScanningStartCfm(const PbpBroadcastAssistantScanSrcStartCfm *cfm)
{
    pbp_client_msg_t msg;

    PBP_LOG("pbpClientSource_SendScanningStartCfm status:%d", cfm->result);

    msg.id = PBP_CLIENT_MSG_ID_ASST_START_SCAN_CFM;
    msg.body.scan_start.status = cfm->result == PBP_STATUS_SUCCESS ?
                                     PBP_CLIENT_MSG_STATUS_SUCCESS : PBP_CLIENT_MSG_STATUS_FAILED;
    pbp_client_source_taskdata.scan_handle = cfm->scanHandle;

    pbp_client_source_taskdata.callback_handler(&msg);
    pfree(cfm->status);
}

static void pbpClientSource_SendSourceScanningReport(const PbpBroadcastAssistantSrcReportInd *report)
{
    pbp_client_msg_t msg;

    PBP_LOG("pbpClientSource_SendSourceScanningReport broadcast_id:%d adv_handle:%d adv_sid %d", report->broadcastId,
             report->advHandle, report->advSid);

    msg.id = PBP_CLIENT_MSG_ID_ASST_SCAN_REPORT;
    msg.body.src_scan_report.source_addr = report->sourceAddrt;
    msg.body.src_scan_report.adv_sid = report->advSid;
    msg.body.src_scan_report.adv_handle = report->advHandle;
    msg.body.src_scan_report.collocated = report->collocated;
    msg.body.src_scan_report.broadcast_id = report->broadcastId;
    msg.body.src_scan_report.num_subgroup = report->numSubgroup;
    msg.body.src_scan_report.subgroup_info = report->subgroupInfo;

    pbp_client_source_taskdata.callback_handler(&msg);
    pfree(report->bigName);
}

static void pbpClientSource_SendScanningStopCfm(const PbpBroadcastAssistantScanSrcStopCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_ASST_STOP_SCAN_CFM, cfm->result);
    pfree(cfm->status);
}

static void pbpClientSource_SendRegisterNotificationCfm(const PbpBroadcastAssistantRegForNotificationCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_ASST_REGISTER_NOTIFICATION_CFM, cfm->result);
    pfree(cfm->status);
}

static void pbpClientSource_AddSourceCfm(const PbpBroadcastAssistantAddSrcCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_ASST_ADD_SOURCE_CFM, cfm->result);
    pfree(cfm->status);
}

static void pbpClientSource_ModifySourceCfm(const PbpBroadcastAssistantModifySrcCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_ASST_MODIFY_SOURCE_CFM, cfm->result);
    pfree(cfm->status);
}

static void pbpClientSource_RemoveSourceCfm(const PbpBroadcastAssistantRemoveSrcCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_ASST_REMOVE_SOURCE_CFM, cfm->result);
    pfree(cfm->status);
}

static void pbpClientSource_ReadBrsCfm(const PbpBroadcastAssistantReadReceiveStateCfm *cfm)
{
    pbpClientSource_SendCommonResp(PBP_CLIENT_MSG_ID_ASST_BRS_READ_CFM, cfm->result);
}

static void pbpClientSource_ReadBrsInd(const PbpBroadcastAssistantReadReceiveStateInd *ind)
{
    pbp_client_msg_t msg;
    TYPED_BD_ADDR_T source_bd_address;

    PBP_LOG("pbpClientSource_ReadBrsInd source_id:%d pa_sync_state:0x%x"
             " advSid :%d bcast_id:%d addtype:%d addr: %x %x %x",
              ind->sourceId, ind->paSyncState, ind->advSid, ind->broadcastId, ind->sourceAddrt.type,
              ind->sourceAddrt.addr.lap, ind->sourceAddrt.addr.nap, ind->sourceAddrt.addr.uap);

    msg.id = PBP_CLIENT_MSG_ID_ASST_BRS_READ_IND;
    msg.body.brs_read_ind.adv_sid = ind->advSid;
    msg.body.brs_read_ind.broadcast_id = ind->broadcastId;
    msg.body.brs_read_ind.source_id = ind->sourceId;
    msg.body.brs_read_ind.pa_sync_state = ind->paSyncState;
    msg.body.brs_read_ind.big_encryption = ind->bigEncryption;
    msg.body.brs_read_ind.status =  ind->result == PBP_STATUS_SUCCESS ? PBP_CLIENT_MSG_STATUS_SUCCESS :
                                                                                PBP_CLIENT_MSG_STATUS_FAILED;

    source_bd_address = ind->sourceAddrt;
    BdaddrConvertTypedBluestackToVm(&msg.body.brs_read_ind.source_address, &source_bd_address);

    pbp_client_source_taskdata.callback_handler(&msg);

    pfree(ind->badCode);
    if (ind->subGroupInfo != NULL)
    {
        pfree(ind->subGroupInfo->metadataValue);
        pfree(ind->subGroupInfo);
    }
}

static void pbpClientSource_BrsInd(const PbpBroadcastAssistantBrsInd *ind)
{
    pbp_client_msg_t msg;
    TYPED_BD_ADDR_T source_bd_address;

    PBP_LOG("pbpClientSource_BrsInd source_id:%d pa_sync_state:0x%x bis_index:0x%x",
              ind->sourceId, ind->paSyncState, ind->subGroupInfo != NULL ? ind->subGroupInfo->bisIndex : 0);

    msg.id = PBP_CLIENT_MSG_ID_ASST_BRS_IND;
    msg.body.brs_ind.adv_sid = ind->advSid;
    msg.body.brs_ind.broadcast_id = ind->broadcastId;
    msg.body.brs_ind.source_id = ind->sourceId;
    msg.body.brs_read_ind.pa_sync_state = ind->paSyncState;
    msg.body.brs_read_ind.big_encryption = ind->bigEncryption;
    msg.body.brs_read_ind.status =  PBP_CLIENT_MSG_STATUS_SUCCESS;

    source_bd_address = ind->sourceAddrt;
    BdaddrConvertTypedBluestackToVm(&msg.body.brs_ind.source_address, &source_bd_address);

    pbp_client_source_taskdata.callback_handler(&msg);

    pfree(ind->badCode);
    if (ind->subGroupInfo != NULL)
    {
        pfree(ind->subGroupInfo->metadataValue);
        pfree(ind->subGroupInfo);
    }
}

static void pbpClientSource_HandlePbpBroadcastMessage(Message message)
{
    CsrBtCmPrim pbp_id = *(CsrBtCmPrim *)message;

    switch (pbp_id)
    {
        case PBP_INIT_CFM:
            pbpClientSource_HandlePbpInitCfm((const PbpInitCfm *) message);
        break;

        case PBP_BROADCAST_SRC_INIT_CFM:
             pbpClientSource_HandlePbpBroadcastInitCfm((const PbpBroadcastSrcInitCfm *) message);
        break;

        case PBP_REGISTER_CAP_CFM:
            pbpClientSource_HandlePbpBroadcastCapRegisterCfm((const PbpRegisterTaskCfm *) message);
        break;

        case PBP_BROADCAST_SRC_CONFIG_CFM:
             pbpClientSource_SendConfigComplete((const PbpBroadcastSrcConfigCfm *) message);
        break;

        case PBP_BROADCAST_SRC_START_STREAM_CFM:
             pbpClientSource_SendStreamingStartCfm((const PbpBroadcastSrcStartStreamCfm *) message);
        break;

        case PBP_BROADCAST_SRC_UPDATE_AUDIO_CFM:
             pbpClientSource_HandleSrcUpdateStreamCfm((const PbpBroadcastSrcUpdateAudioCfm *) message);
        break;

        case PBP_BROADCAST_SRC_STOP_STREAM_CFM:
             pbpClientSource_SendStreamingStopCfm((const PbpBroadcastSrcStopStreamCfm *) message);
        break;

        case PBP_BROADCAST_SRC_REMOVE_STREAM_CFM:
             pbpClientSource_HandleSrcRemoveStreamCfm((const PbpBroadcastSrcRemoveStreamCfm *) message);
        break;

        case PBP_BROADCAST_SRC_DEINIT_CFM:
             pbpClientSource_SendDeinitCfm((const PbpBroadcastSrcDeinitCfm *) message);
        break;

        case PBP_BROADCAST_SRC_SET_PARAM_CFM:
             pbpClientSource_HandleSrcSetBcastParamsConfigCfm((const PbpBroadcastSrcSetParamCfm *) message);
        break;

        case PBP_BROADCAST_ASSISTANT_SCAN_SRC_START_CFM:
            pbpClientSource_SendScanningStartCfm((const PbpBroadcastAssistantScanSrcStartCfm *) message);
        break;

        case PBP_BROADCAST_ASSISTANT_SRC_REPORT_IND:
            pbpClientSource_SendSourceScanningReport((const PbpBroadcastAssistantSrcReportInd *) message);
        break;

        case PBP_BROADCAST_ASSISTANT_SCAN_SRC_STOP_CFM:
            pbpClientSource_SendScanningStopCfm((const PbpBroadcastAssistantScanSrcStopCfm *) message);
        break;

        case PBP_BROADCAST_ASSISTANT_REG_FOR_NOTIFICATION_CFM:
            pbpClientSource_SendRegisterNotificationCfm((const PbpBroadcastAssistantRegForNotificationCfm *) message);
        break;

        case PBP_BROADCAST_ASSISTANT_ADD_SRC_CFM:
            pbpClientSource_AddSourceCfm((const PbpBroadcastAssistantAddSrcCfm *) message);
        break;

        case PBP_BROADCAST_ASSISTANT_MODIFY_SRC_CFM:
            pbpClientSource_ModifySourceCfm((const PbpBroadcastAssistantModifySrcCfm *) message);
        break;

        case PBP_BROADCAST_ASSISTANT_REMOVE_SRC_CFM:
            pbpClientSource_RemoveSourceCfm((const PbpBroadcastAssistantRemoveSrcCfm *) message);
        break;

        case PBP_BROADCAST_ASSISTANT_READ_RECEIVE_STATE_IND:
            pbpClientSource_ReadBrsInd((const PbpBroadcastAssistantReadReceiveStateInd *) message);
        break;

        case PBP_BROADCAST_ASSISTANT_READ_RECEIVE_STATE_CFM:
            pbpClientSource_ReadBrsCfm((const PbpBroadcastAssistantReadReceiveStateCfm *) message);
        break;

        case PBP_BROADCAST_ASSISTANT_BRS_IND:
            pbpClientSource_BrsInd((const PbpBroadcastAssistantBrsInd *) message);
        break;

        default:
            PBP_LOG("pbpClientSource_HandlePbpBroadcastMessage Unhandled message id: 0x%x", (*(CsrBtCmPrim *) message));
        break;
    }
}

static void pbpClientSource_HandlePbpMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case PBP_PRIM_BASE:
            pbpClientSource_HandlePbpBroadcastMessage(message);
        break;

        default:
            PBP_LOG("pbpClientSource_HandlePbpMessage Unhandled Message Id : 0x%x", id);
        break;
    }
}

static PbpBigConfigMode pbpClientSource_GetBigConfigMode(const PbpBigSubGroups * sub_group_info)
{
    PbpBigConfigMode big_config_mode = PBP_BIG_CONFIG_MODE_DEFAULT;

    if (sub_group_info->numBis == 1 && sub_group_info->bisInfo[0].audioLocation == PBP_AUDIO_LOCATION_LEFT_AND_RIGHT)
    {
        big_config_mode |= 2 /* PBP_BIG_CONFIG_MODE_JOINT_STEREO*/;
    }

    return big_config_mode;
}

void PbpClientSource_UpdateAdvSetting(const CapClientBcastSrcAdvParams *pbp_client_adv_param)
{
    PBP_LOG("PbpClientSource_UpdateAdvSetting");

    PbpBroadcastSrcSetAdvParamsReq(pbp_client_source_taskdata.bcast_handle, pbp_client_adv_param);
}

void PbpClientSource_Init(void)
{
    PBP_LOG("PbpClientSource_Init");

    pbp_client_source_taskdata.task_data.handler = pbpClientSource_HandlePbpMessage;
    PbpInitReq(TrapToOxygenTask((Task) &pbp_client_source_taskdata.task_data));
}

void PbpClientSource_RegisterCallback(pbp_client_source_callback_handler_t handler)
{
    pbp_client_source_taskdata.callback_handler = handler;
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
void PbpClientSource_RegisterTaskWithCap(ServiceHandle group_handle)
{
    PbpRegisterTaskReq(group_handle, pbp_client_source_taskdata.profile_handle);
}
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

void PbpClientSource_Configure(PbpProfileHandle handle,
                               uint32 presentation_delay,
                               uint8 num_subgroup,
                               const PbpBigSubGroups *subgroup_info,
                               const uint8 source_name_len,
                               const char *source_name,
                               const BroadcastType bcast_type,
                               bool encryption)
{
    PbpBroadcastInfo bcast_info;

    bcast_info.broadcast = bcast_type;
    bcast_info.flags = encryption ? PBP_CLIENT_SOURCE_FLAGS_ENCRYPTED : 0;
    bcast_info.appearanceValue = CSR_BT_APPEARANCE_GENERIC_PHONE;
    bcast_info.bigNameLen = source_name_len;
    bcast_info.bigName = (uint8*)source_name;

    PbpBroadcastSrcConfigReq(handle, CSR_BT_ADDR_PUBLIC, presentation_delay, num_subgroup,
                             subgroup_info,
                             &bcast_info,
                             pbpClientSource_GetBigConfigMode(subgroup_info), NULL);
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
void PbpClientSource_AddSource(ServiceHandle cap_group_id,
                               uint32 cid,
                               typed_bdaddr source_taddr,
                               uint8 adv_handle,
                               uint8 adv_sid,
                               uint32 broadcast_id,
                               uint32 bis_index)
{
    TYPED_BD_ADDR_T source_bd_address;
    PbpSubgroupInfo subgroup_info[PBP_NUMBER_OF_SUBGROUP_SUPPORTED];

    subgroup_info[0].bisIndex = bis_index;
    subgroup_info[0].metadataLen = 0;
    subgroup_info[0].metadataValue = NULL;
    BdaddrConvertTypedVmToBluestack(&source_bd_address, &source_taddr);

    PBP_LOG("PbpClientSource_AddSource cid 0x%x broadcast_id:%d",
              cid, broadcast_id);

    PbpBroadcastAssistantAddSrcReq(pbp_client_source_taskdata.profile_handle,
                                   cap_group_id,
                                   cid,
                                   &source_bd_address,
                                   PBP_BCAST_SRC_COLLOCATED,
                                   adv_handle,
                                   adv_sid,
                                   CAP_CLIENT_PA_SYNC_SYNCHRONIZE_PAST,
                                   PBP_PA_INTERVAL_DEFAULT,
                                   broadcast_id,
                                   PBP_NUMBER_OF_SUBGROUP_SUPPORTED,
                                   subgroup_info);
}

void PbpClientSource_ModifySource(ServiceHandle cap_group_id,
                                  uint32 cid,
                                  uint8 adv_handle,
                                  uint8 adv_sid,
                                  uint32 bis_index,
                                  uint8 source_id,
                                  bool pa_sync_enable)
{
    PbpBroadcastDelegatorInfo brcast_sink_info[PBP_NUMBER_OF_SINK_INFO_SUPPORTED];
    PbpSubgroupInfo subgroup_info[PBP_NUMBER_OF_SUBGROUP_SUPPORTED];

    subgroup_info[0].bisIndex = bis_index;
    subgroup_info[0].metadataLen = 0;
    subgroup_info[0].metadataValue = NULL;
    brcast_sink_info[0].cid = cid;
    brcast_sink_info[0].sourceId = source_id;

    PBP_LOG("PbpClientSource_ModifySource cid 0x%x source_id:%d",
              cid, source_id);

    PbpBroadcastAssistantModifySrcReq(pbp_client_source_taskdata.profile_handle,
                                      cap_group_id,
                                      PBP_BCAST_SRC_COLLOCATED,
                                      adv_handle,
                                      adv_sid,
                                      pa_sync_enable ? CAP_CLIENT_PA_SYNC_SYNCHRONIZE_PAST :
                                                       CAP_CLIENT_PA_SYNC_NOT_SYNCHRONIZE,
                                      PBP_PA_INTERVAL_DEFAULT,
                                      PBP_NUMBER_OF_SINK_INFO_SUPPORTED,
                                      brcast_sink_info,
                                      PBP_NUMBER_OF_SUBGROUP_SUPPORTED,
                                      subgroup_info);
}

void PbpClientSource_RemoveSource(ServiceHandle cap_group_id, uint32 cid, uint8 source_id)
{
    PbpBroadcastDelegatorInfo brcast_sink_info[PBP_NUMBER_OF_SINK_INFO_SUPPORTED];

    PBP_LOG("PbpClientSource_RemoveSource cid 0x%x source_id:%d",
              cid, source_id);

    brcast_sink_info[0].cid = cid;
    brcast_sink_info[0].sourceId = source_id;
    PbpBroadcastAssistantRemoveSrcReq(pbp_client_source_taskdata.profile_handle,
                                      cap_group_id,
                                      PBP_NUMBER_OF_SINK_INFO_SUPPORTED,
                                      brcast_sink_info);
}

void PbpClientSource_StartScanningForSource(ServiceHandle cap_group_id, uint32 cid,
                                            PbpBcastType bcast_type, CapClientContext audio_context)
{
    PbpBroadcastAssistantScansSrcStartSrcReq(pbp_client_source_taskdata.profile_handle,
                                             cap_group_id,
                                             cid,
                                             PBP_BCAST_SRC_COLLOCATED,
                                             bcast_type,
                                             audio_context,
                                             PBP_SCAN_PARAM_DEFAULT,
                                             PBP_SCAN_PARAM_DEFAULT,
                                             PBP_SCAN_PARAM_DEFAULT);
}

void PbpClientSource_StopScanningForSource(ServiceHandle cap_group_id, uint32 cid)
{
    PbpBroadcastAssistantStopSrcScanReq(pbp_client_source_taskdata.profile_handle,
                                        cap_group_id,
                                        cid,
                                        pbp_client_source_taskdata.scan_handle);
}

void PbpClientSource_RegisterForGattNotification(ServiceHandle cap_group_id, uint32 cid, uint8 source_id)
{
    PbpBroadcastAssistantRegisterForNotificationReq(pbp_client_source_taskdata.profile_handle,
                                                    cap_group_id,
                                                    cid,
                                                    source_id,
                                                    TRUE,
                                                    TRUE);
}

void PbpClientSource_ReadReceiverSinkState(ServiceHandle cap_group_id, uint32 cid)
{
    PbpBroadcastAssistantReadReceiveStateReq(pbp_client_source_taskdata.profile_handle,
                                             cap_group_id, cid);
}
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
