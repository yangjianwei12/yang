/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    bap_profile_client
    \brief      BAP Profile Client
*/

#include "bap_profile_client_private.h"

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "bap_client_lib.h"
#include "synergy.h"
#include "stdlib.h"
#include "gatt_connect.h"

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#define BAP_SCAN_PARAM_DEFAULT 0x0
#define BAP_BROADCAST_PA_INTERVAL_DEFAULT 360 /* 450ms */
#define BAP_BROADCAST_NUMBER_OF_SUBGROUP_SUPPORTED  0x1

/*! \brief BAP client task data. */
bap_profile_client_task_data_t *bap_profile_client_taskdata = NULL;

/*! \brief Connect to maximum supported BAP devices */
bool bap_profile_client_connect_all_servers = FALSE;

/*! Sends BAP_CLIENT_MSG_ID_INIT_COMPLETE to registered clients */
static void bapProfileClient_SendInitCfm(bap_profile_client_status_t status)
{
    bap_profile_client_msg_t msg;
    bool discovery_needed = FALSE;

    msg.id = BAP_PROFILE_CLIENT_MSG_ID_INIT_COMPLETE;
    msg.body.init_complete.status = status;

    if (bap_profile_client_connect_all_servers &&
        bap_profile_client_taskdata->number_of_connected_servers < MAX_BAP_DEVICES_SUPPORTED)
    {
        discovery_needed = TRUE;
    }
    msg.body.init_complete.more_devices_needed = discovery_needed;

    bap_profile_client_taskdata->callback_handler(&msg);
}

/*! Sends BAP_CLIENT_MSG_ID_PROFILE_DISCONNECT to registered clients */
static void bapProfileClient_DisconnectCfm(bap_profile_client_status_t status)
{
    bap_profile_client_msg_t msg;

    if (status == BAP_PROFILE_CLIENT_STATUS_SUCCESS)
    {
        bap_profile_client_taskdata->number_of_connected_servers--;
    }

    msg.id = BAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT;
    msg.body.disconnected.status = status;
    msg.body.disconnected.connected_server_cnt = bap_profile_client_taskdata->number_of_connected_servers;

    bap_profile_client_taskdata->callback_handler(&msg);
}

static void BapProfileClient_ResetBapInstance(bap_profile_client_device_instance_t *instance)
{
    if (instance != NULL)
    {
        memset(instance, 0, sizeof(bap_profile_client_device_instance_t));
    }
}

/*! \brief Function that checks whether the BAP client instance matches based on the compare type */
static bool bapProfileClient_Compare(bap_profile_client_instance_compare_by_type_t type,
                              unsigned compare_value,
                              bap_profile_client_device_instance_t *instance)
{
    bool found = FALSE;

    switch (type)
    {
        case bap_profile_client_compare_by_cid:
            found = instance->cid == (gatt_cid_t) compare_value;
        break;

        case bap_profile_client_compare_by_state:
            found = instance->state == (bap_profile_client_state_t) compare_value;
        break;

        case bap_profile_client_compare_by_profile_handle:
            found = instance->bap_profile_handle == (BapProfileHandle) compare_value;
        break;

        case bap_profile_client_compare_by_valid_invalid_cid:
            found = instance->state == bap_profile_client_state_connected &&
                    (instance->cid == (gatt_cid_t) compare_value || compare_value == INVALID_CID);
        break;

        default:
        break;
    }

    return found;
}

/*! \brief Get the BAP instance based on the compare type */
static bap_profile_client_device_instance_t * bapProfileClient_GetDeviceInstance(bap_profile_client_instance_compare_by_type_t type,
                                                                          unsigned cmp_value)
{
    bap_profile_client_device_instance_t *instance = NULL;
    bap_profile_client_task_data_t *client_context = BapProfileClient_GetContext();

    for (instance = &client_context->device_instance[0];
         instance < &client_context->device_instance[MAX_BAP_DEVICES_SUPPORTED];
         instance++)
    {
        if (bapProfileClient_Compare(type, cmp_value, instance))
        {
            return instance;
        }
    }

    return NULL;
}

/*! \brief Handle the BAP initialisation confirm message */
static void bapProfileClient_HandleBapInitCfm(BapInitCfm *cfm)
{
    bap_profile_client_device_instance_t *instance;

    DEBUG_LOG("bapProfileClient_HandleBapInitCfm role 0x%04x prflHndl 0x%04x result %d",
               cfm->role, cfm->handle, cfm->result);

    instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_state, (unsigned)bap_profile_client_state_discovery);
    PanicNull(instance);

    if (cfm->result == BAP_RESULT_SUCCESS)
    {
        instance->bap_profile_handle = cfm->handle;
        instance->role = cfm->role;
        instance->state = bap_profile_client_state_connected;

        bap_profile_client_taskdata->number_of_connected_servers++;

        /* Send indication that all instance are initialized successfully */
        bapProfileClient_SendInitCfm(BAP_PROFILE_CLIENT_STATUS_SUCCESS);

        memset(&instance->spkr_audio_path, 0, sizeof(bap_media_config_t));
        memset(&instance->mic_audio_path, 0, sizeof(bap_microphone_config_t));
        instance->spkr_audio_path.source_iso_handle = 0xFFFF;
        instance->spkr_audio_path.source_iso_handle_right = 0xFFFF;
        instance->mic_audio_path.source_iso_handle = 0xFFFF;
        instance->mic_audio_path.source_iso_handle_right = 0xFFFF;
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        BapUnicastRegisterAseNotificationReq(instance->bap_profile_handle, ASE_ID_ALL, TRUE);
#endif
    }
    else if(cfm->result == BAP_RESULT_INPROGRESS)
    {
    }
    else
    {
        BapProfileClient_ResetBapInstance(instance);

        /* Send indication that all instance are initialized successfully */
        bapProfileClient_SendInitCfm(BAP_PROFILE_CLIENT_STATUS_FAILED);
    }
}

static void bapProfileClient_HandleBapDestroyCfm(BapDeinitCfm *cfm)
{
    bap_profile_client_device_instance_t *instance;

    if (cfm->result == BAP_RESULT_INPROGRESS)
    {
        return;
    }

    instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle, (unsigned)cfm->handle);
    PanicNull(instance);

    DEBUG_LOG("bapProfileClient_GetDeviceInstance prflHndl 0x%04x status %d", cfm->handle, cfm->result);

    BapProfileClient_ResetBapInstance(instance);
    bapProfileClient_DisconnectCfm(BAP_PROFILE_CLIENT_STATUS_SUCCESS);

    pfree(cfm->handles);

    /* free instance data if all servers disconnected */
    if (bap_profile_client_taskdata->number_of_connected_servers == 0)
    {
        CsrPmemFree(bap_profile_client_taskdata);
    }
}

static void bapProfileClient_HandleGetRemoteAudioLocationCfm(BapGetRemoteAudioLocationCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleGetRemoteAudioLocationCfm status %d, Audio Location %d", cfm->result, cfm->location);
}

static void bapProfileClient_HandleDiscoverAudioContextCfm(BapDiscoverAudioContextCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleDiscoverAudioContextCfm status %d, Sink Context %d Source Context %d",
               cfm->result,
               cfm->contextValue.sinkContext,
               cfm->contextValue.sourceContext);
}

static void bapProfileClient_HandleReadAseInfoCfm(BapUnicastClientReadAseInfoCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleReadAseCfm status %d", cfm->result);

    if (cfm->aseInfo != NULL)
    {
        pfree(cfm->aseInfo);
    }
}

static void bapProfileClient_HandleAseEnableCfm(BapUnicastClientEnableCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleSetOpControlCfm status %d", cfm->result);
}

static void bapProfileClient_HandleAseEnableInd(BapUnicastClientEnableInd *ind)
{
    int i;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle, (unsigned)ind->handle);

    DEBUG_LOG("bapProfileClient_HandleSetOpControlCfm status %d", ind->result);

    if (instance != NULL)
    {
        for (i = 0; i < MAX_BAP_ASE_SUPPORTED; i++)
        {
            if (instance->ase_info[i].ase_id == ind->aseId && 
                instance->ase_info[i].ase_state == BAP_ASE_STATE_QOS_CONFIGURED)
            {
                instance->ase_info[i].ase_state = ind->aseState;
            }
        }
    }

    if (ind->metadata != NULL)
    {
        pfree(ind->metadata);
    }
}

static void bapProfileClient_HandleAseCodecConfigInd(BapUnicastClientCodecConfigureInd *ind)
{
    int i;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle, (unsigned)ind->handle);

    DEBUG_LOG("bapProfileClient_HandleAseCodecConfigInd status %d", ind->result);

    if (instance != NULL)
    {
        for (i = 0; i < MAX_BAP_ASE_SUPPORTED; i++)
        {
            if (instance->ase_info[i].ase_state == BAP_ASE_STATE_IDLE)
            {
                instance->ase_info[i].ase_id = ind->aseId;
                instance->ase_info[i].ase_state = ind->aseState;
                instance->ase_info[i].presentationDelayMax = ind->presentationDelayMax;
                instance->ase_info[i].presentationDelayMin = ind->presentationDelayMin;
                instance->ase_info[i].framing = ind->framing;
                instance->ase_info[i].phy = ind->phy;
                instance->ase_info[i].rtn = ind->rtn;
                instance->ase_info[i].transportLatency = ind->transportLatency;
                memcpy(&instance->ase_info[i].codec_config, &ind->codecConfiguration, sizeof(BapCodecConfiguration));
                DEBUG_LOG("bapProfileClient_HandleAseCodecConfigInd sf %d fd %d", ind->codecConfiguration.samplingFrequency, ind->codecConfiguration.frameDuaration);
                break;
            }
        }
    }
}

static void bapProfileClient_HandleAseCodecConfigCfm(BapUnicastClientCodecConfigureCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleAseCodecConfigCfm status %d", cfm->result);
}

static void bapProfileClient_HandleCapabilityCfm(BapDiscoverRemoteAudioCapabilityCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleCapabilityCfm status %d", cfm->result);
}

static void bapProfileClient_HandleCigConfigCfm(BapUnicastClientCigConfigureCfm *cfm)
{
    int i;

    DEBUG_LOG("bapProfileClient_HandleCigConfigCfm status %d", cfm->result);

    bap_profile_client_taskdata->cig_info.cig_id = cfm->cigId;

    for (i = 0; i < cfm->cisCount; i++)
    {
        bap_profile_client_taskdata->cig_info.cis_info[i].cis_handle = cfm->cisHandles[i];
        DEBUG_LOG("bapProfileClient_HandleCigConfigCfm status Cis Id %d, Cis Handle %d", bap_profile_client_taskdata->cig_info.cis_info[i].cis_id, cfm->cisHandles[i]);
    }
}

static bool bapProfileClient_GetCisInfoById(uint16 cis_id, bap_profile_client_cis_info_t *cis_info)
{
    int i;
    bool status = FALSE;

    for (i = 0; i < MAX_BAP_CIS_SUPPORTED; i++)
    {
        if (bap_profile_client_taskdata->cig_info.cis_info[i].cis_id == cis_id)
        {
            memcpy(cis_info, &bap_profile_client_taskdata->cig_info.cis_info[i], sizeof(bap_profile_client_cis_info_t));
            status = TRUE;
        }
    }

    return status;
}

static uint16 bapProfileClient_GetCisHandle(uint8 cis_id)
{
    int i;

    for (i = 0; i < MAX_BAP_CIS_SUPPORTED; i++)
    {
        if (bap_profile_client_taskdata->cig_info.cis_info[i].cis_id == cis_id)
        {
            return bap_profile_client_taskdata->cig_info.cis_info[i].cis_handle;
        }
    }

    DEBUG_LOG("bapProfileClient_GetCisHandle failed");

    return 0;
}

static void bapProfileClient_HandleQosConfigInd(BapUnicastClientQosConfigureInd *cfm)
{
    int i;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle, (unsigned)cfm->handle);

    DEBUG_LOG("bapProfileClient_HandleQosConfigInd status %d", cfm->result);

    if (instance != NULL)
    {
        for (i = 0; i < MAX_BAP_ASE_SUPPORTED; i++)
        {
            if (instance->ase_info[i].ase_id == cfm->aseId)
            {
                instance->ase_info[i].ase_state = cfm->aseState;
                instance->ase_info[i].cis.cis_id = cfm->cisId;
                instance->ase_info[i].cis.cis_handle = bapProfileClient_GetCisHandle(cfm->cisId);
            }
        }
    }
}

static void bapProfileClient_HandleQosConfigCfm(BapUnicastClientQosConfigureCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleQosConfigCfm status %d", cfm->result);
}

static void bapProfileClient_HandleCisConnectInd(BapUnicastClientCisConnectInd *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleCisConnectInd status %d", cfm->result);
}

static void bapProfileClient_HandleCisConnectCfm(BapUnicastClientCisConnectCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleCisConnectCfm status %d", cfm->result);
}

static void bapProfileClient_HandleSetupDataPathCfm(BapSetupDataPathCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleSetupDataPathCfm status %d", cfm->result);
}

static void bapProfileClient_HandleReceiverReadyInd(BapUnicastClientReceiverReadyInd *ind)
{
    int i;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle, (unsigned)ind->handle);

    DEBUG_LOG("bapProfileClient_HandleReceiverReadyInd status %d", ind->result);

    if (instance != NULL)
    {
        for (i = 0; i < MAX_BAP_ASE_SUPPORTED; i++)
        {
            if (instance->ase_info[i].ase_id == ind->aseId)
            {
                instance->ase_info[i].ase_state = ind->aseState;
                break;
            }
        }
    }
}

static void bapProfileClient_HandleAseDisableInd(BapUnicastClientDisableInd *ind)
{
    int i;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle,
                                                                                        (unsigned)ind->handle);

    DEBUG_LOG("bapProfileClient_HandleAseDisableInd status %d", ind->result);
    PanicFalse(instance != NULL);

    for (i = 0; i < MAX_BAP_ASE_SUPPORTED; i++)
    {
        if (instance->ase_info[i].ase_id == ind->aseId)
        {
            instance->ase_info[i].ase_state = ind->aseState;
            break;
        }
    }
}

static void bapProfileClient_HandleAseDisableCfm(BapUnicastClientDisableCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleAseDisableCfm status %d", cfm->result);
}

static void bapProfileClient_HandleAseReleaseInd(BapUnicastClientReleaseInd *ind)
{
    int i;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle,
                                                                                        (unsigned)ind->handle);

    DEBUG_LOG("bapProfileClient_HandleAseReleaseInd status %d", ind->result);

    PanicFalse(instance != NULL);

    for (i = 0; i < MAX_BAP_ASE_SUPPORTED; i++)
    {
        if (instance->ase_info[i].ase_id == ind->aseId)
        {
            instance->ase_info[i].ase_state = ind->aseState;
            break;
        }
    }
}

static void bapProfileClient_HandleAseReleaseCfm(BapUnicastClientReleaseCfm *cfm)
{
    DEBUG_LOG("bapProfileClient_HandleAseReleaseCfm status %d", cfm->result);
}

static void bapProfileClient_StartScanCfm(BapBroadcastAssistantStartScanCfm *cfm)
{
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle,
                                                                                        (unsigned)cfm->handle);

    DEBUG_LOG("bapProfileClient_StartScanCfm status %d", cfm->result);

    PanicFalse(instance != NULL);

    instance->scan_handle = cfm->scanHandle;
}

static void bapProfileClient_SourceReport(BapBroadcastAssistantSrcReportInd * ind)
{
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle,
                                                                                        (unsigned)ind->handle);

    DEBUG_LOG("bapProfileClient_SourceReport");

    PanicFalse(instance != NULL);

    if (!instance->source_found)
    {
        instance->adv_handle = ind->advHandle;
        instance->bcast_id = ind->broadcastId;
        instance->adv_sid = ind->advSid;

        if (instance->add_collocated && ind->collocated)
        {
            instance->source_found = TRUE;
        }
        else if (ind->sourceAddrt.addr.lap ==  instance->source_addr.addr.lap &&
                 ind->sourceAddrt.addr.nap ==  instance->source_addr.addr.nap &&
                 ind->sourceAddrt.addr.uap ==  instance->source_addr.addr.uap)
        {
             instance->source_found = TRUE;
        }
    }

    pfree(ind->serviceData);
    pfree(ind->bigName);

    if (ind->subgroupInfo != NULL)
    {
        pfree(ind->subgroupInfo->bisInfo);
        pfree(ind->subgroupInfo);
    }
}

static void bapProfileClient_StartSyncToSrcCfm(BapBroadcastAssistantSyncToSrcStartCfm * cfm)
{
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle,
                                                                                        (unsigned)cfm->handle);

    DEBUG_LOG("bapProfileClient_StartSyncToSrcCfm status %d", cfm->result);

    PanicFalse(instance != NULL);

    instance->adv_handle = cfm->syncHandle;
    instance->adv_sid = cfm->advSid;
}

static void bapProfileClient_ReadBrsCfm(BapBroadcastAssistantReadBrsCfm * cfm)
{
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle,
                                                                                        (unsigned)cfm->handle);

    DEBUG_LOG("bapProfileClient_ReadBrsCfm status %d", cfm->result);

    PanicFalse(instance != NULL);

    instance->source_id = cfm->sourceId;

    pfree(cfm->badCode);

    if (cfm->subGroupInfo != NULL)
    {
        pfree(cfm->subGroupInfo->metadataValue);
        pfree(cfm->subGroupInfo);
    }
}

static void bapProfileClient_BrsInd(BapBroadcastAssistantBrsInd *ind)
{
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle,
                                                                                        (unsigned)ind->handle);

    DEBUG_LOG("bapProfileClient_BrsInd status 0x%x, source_id:0x%x, pa_state:0x%x", ind->sourceId, ind->paSyncState);

    PanicFalse(instance != NULL);

    if (ind->subGroupInfo != NULL)
    {
        instance->bis_index = ind->subGroupInfo->bisSyncState;
    }

    pfree(ind->badCode);

    if (ind->subGroupInfo != NULL)
    {
        pfree(ind->subGroupInfo->metadataValue);
        pfree(ind->subGroupInfo);
    }
}

static void bapProfileClient_HandleBapMessage(Message message)
{
    CsrBtGattPrim *bap_id = (CsrBtGattPrim *)message;

    switch(*bap_id)
    {
        case BAP_INIT_CFM:
            bapProfileClient_HandleBapInitCfm((BapInitCfm *) message);
        break;

        case BAP_DEINIT_CFM:
            bapProfileClient_HandleBapDestroyCfm((BapDeinitCfm *) message);
        break;

        case BAP_GET_REMOTE_AUDIO_LOCATION_CFM:
            bapProfileClient_HandleGetRemoteAudioLocationCfm((BapGetRemoteAudioLocationCfm *) message);
        break;

        case BAP_DISCOVER_AUDIO_CONTEXT_CFM:
            bapProfileClient_HandleDiscoverAudioContextCfm((BapDiscoverAudioContextCfm *) message);
        break;

        case BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM:
            bapProfileClient_HandleReadAseInfoCfm((BapUnicastClientReadAseInfoCfm *) message);
        break;

        case BAP_UNICAST_CLIENT_ENABLE_CFM:
            bapProfileClient_HandleAseEnableCfm((BapUnicastClientEnableCfm *) message);
        break;

        case BAP_UNICAST_CLIENT_ENABLE_IND:
            bapProfileClient_HandleAseEnableInd((BapUnicastClientEnableInd *) message);
        break;

        case BAP_UNICAST_CLIENT_CODEC_CONFIGURE_IND:
            bapProfileClient_HandleAseCodecConfigInd((BapUnicastClientCodecConfigureInd *) message);
        break;

        case BAP_UNICAST_CLIENT_CODEC_CONFIGURE_CFM:
            bapProfileClient_HandleAseCodecConfigCfm((BapUnicastClientCodecConfigureCfm *) message);
        break;

        case BAP_DISCOVER_REMOTE_AUDIO_CAPABILITY_CFM:
            bapProfileClient_HandleCapabilityCfm((BapDiscoverRemoteAudioCapabilityCfm *) message);
        break;

        case BAP_UNICAST_CLIENT_CIG_CONFIGURE_CFM:
            bapProfileClient_HandleCigConfigCfm((BapUnicastClientCigConfigureCfm *) message);
        break;

        case BAP_UNICAST_CLIENT_QOS_CONFIGURE_IND:
            bapProfileClient_HandleQosConfigInd((BapUnicastClientQosConfigureInd *) message);
        break;

        case BAP_UNICAST_CLIENT_QOS_CONFIGURE_CFM:
            bapProfileClient_HandleQosConfigCfm((BapUnicastClientQosConfigureCfm *) message);
        break;

        case BAP_UNICAST_CLIENT_CIS_CONNECT_IND:
            bapProfileClient_HandleCisConnectInd((BapUnicastClientCisConnectInd *) message);
        break;

        case BAP_UNICAST_CLIENT_CIS_CONNECT_CFM:
            bapProfileClient_HandleCisConnectCfm((BapUnicastClientCisConnectCfm *) message);
        break;

        case BAP_CLIENT_SETUP_DATA_PATH_CFM:
            bapProfileClient_HandleSetupDataPathCfm((BapSetupDataPathCfm *) message);
        break;

        case BAP_UNICAST_CLIENT_RECEIVER_READY_IND:
            bapProfileClient_HandleReceiverReadyInd((BapUnicastClientReceiverReadyInd *) message);
        break;

        case BAP_UNICAST_CLIENT_DISABLE_IND:
            bapProfileClient_HandleAseDisableInd((BapUnicastClientDisableInd *) message);
        break;

        case BAP_UNICAST_CLIENT_DISABLE_CFM:
            bapProfileClient_HandleAseDisableCfm((BapUnicastClientDisableCfm *) message);
        break;

        case BAP_UNICAST_CLIENT_RELEASE_IND:
            bapProfileClient_HandleAseReleaseInd((BapUnicastClientReleaseInd *) message);
        break;

        case BAP_UNICAST_CLIENT_RELEASE_CFM:
            bapProfileClient_HandleAseReleaseCfm((BapUnicastClientReleaseCfm *) message);
        break;

        case BAP_BROADCAST_ASSISTANT_START_SCAN_CFM:
            bapProfileClient_StartScanCfm((BapBroadcastAssistantStartScanCfm *) message);
        break;

        case BAP_BROADCAST_ASSISTANT_SRC_REPORT_IND:
            bapProfileClient_SourceReport((BapBroadcastAssistantSrcReportInd *) message);
        break;

        case BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_CFM:
            bapProfileClient_StartSyncToSrcCfm((BapBroadcastAssistantSyncToSrcStartCfm *) message);
        break;

        case BAP_BROADCAST_ASSISTANT_READ_BRS_CFM:
            bapProfileClient_ReadBrsCfm((BapBroadcastAssistantReadBrsCfm *) message);
        break;

        case BAP_BROADCAST_ASSISTANT_BRS_IND:
            bapProfileClient_BrsInd((BapBroadcastAssistantBrsInd *) message);
        break;

        default:
            DEBUG_LOG("bapProfileClient_HandleBapMessage Unhandled Message Id : 0x%x", bap_id);
        break;
    }
}

/*! \brief Handler that receives notification from BAP Profile library */
static void bapProfileClient_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case BAP_CLIENT_PRIM:
            bapProfileClient_HandleBapMessage(message);
        break;

        default:
            DEBUG_LOG("bapProfileClient_HandleMessage Unhandled Message Id : 0x%x", id);
        break;
    }
}

static void bapProfileClient_Init(bap_profile_client_callback_handler_t handler)
{
    DEBUG_LOG("BapProfileClient_Init");

    if (bap_profile_client_taskdata == NULL)
    {
        bap_profile_client_taskdata = (bap_profile_client_task_data_t *) CsrPmemAlloc(sizeof(bap_profile_client_task_data_t));

        memset(bap_profile_client_taskdata, 0, sizeof(bap_profile_client_task_data_t));
        bap_profile_client_taskdata->task_data.handler = bapProfileClient_HandleMessage;
        bap_profile_client_taskdata->callback_handler = handler;
    }
    else
    {
        DEBUG_LOG("BapProfileClient_Init : Already Initialized");
    }
}

bool BapProfileClient_CreateInstance(gatt_cid_t cid, bap_profile_client_callback_handler_t handler)
{
    bool status = FALSE;
    BapInitData bap_init_data;
    bap_profile_client_device_instance_t *instance;

    bapProfileClient_Init(handler);

    instance =  bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_state, bap_profile_client_state_idle);

    DEBUG_LOG("BapProfileClient_CreateInstance instance 0x%x", instance);

    if (instance != NULL)
    {
        instance->cid = cid;
        instance->state = bap_profile_client_state_discovery;
        bap_init_data.cid = cid;
        bap_init_data.role = BAP_ROLE_UNICAST_CLIENT | BAP_ROLE_BROADCAST_SOURCE |BAP_ROLE_BROADCAST_ASSISTANT;
        bap_init_data.handles = NULL;

        BapInitReq(TrapToOxygenTask((Task) &bap_profile_client_taskdata->task_data), bap_init_data);
        status = TRUE;
    }

    return status;
}

bool BapProfileClient_DestroyInstance(gatt_cid_t cid)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance;

    DEBUG_LOG("BapProfileClient_DestroyInstance cid 0x%x", cid);

    instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        BapDeinitReq(instance->bap_profile_handle, instance->role);
        status = TRUE;
    }

    return status;
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
void BapProfileClient_ReadAudioLocation(gatt_cid_t cid, uint8 location_type)
{
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_ReadAudioLocation location 0x%x", location_type);
        BapGetRemoteAudioLocationReq(instance->bap_profile_handle, location_type);
    }
}

void BapProfileClient_ReadAudioContext(gatt_cid_t cid, uint8 audio_context_type)
{
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_ReadAudioContext location 0x%x", audio_context_type);
        BapDiscoverAudioContextReq(instance->bap_profile_handle, audio_context_type);
    }
}

void BapProfileClient_ReadAseInfo(gatt_cid_t cid, uint8 ase_id, uint8 ase_type)
{
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_ReadAseInfo ase_id %d, ase_type %d", ase_id, ase_type);
        BapUnicastClientReadAseInfoReq(instance->bap_profile_handle, ase_id, ase_type);
    }
}

void BapProfileClient_ConfigureCodecForSinkAse(gatt_cid_t cid,
                                               uint8 sink_ase_id,
                                               uint8 channel_alloc,
                                               uint8 frame_duration,
                                               uint16 samp_freq,
                                               uint16 octets_per_frame,
                                               uint8 target_latency,
                                               uint8 target_phy)
{
    BapAseCodecConfiguration sink_codec_config;

    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_ConfigureCodecForSinkAse");

        sink_codec_config.aseId = sink_ase_id;
        sink_codec_config.serverDirection = BAP_SERVER_DIRECTION_SINK;
        sink_codec_config.targetLatency = target_latency; //3
        sink_codec_config.targetPhy = target_phy;
        sink_codec_config.codecId.codecId = BAP_CODEC_ID_LC3;
        sink_codec_config.codecId.companyId = 0;
        sink_codec_config.codecId.vendorCodecId = 0;
        sink_codec_config.codecConfiguration.audioChannelAllocation = channel_alloc;
        sink_codec_config.codecConfiguration.frameDuaration = frame_duration;
        sink_codec_config.codecConfiguration.lc3BlocksPerSdu = BAP_DEFAULT_LC3_BLOCKS_PER_SDU;
        sink_codec_config.codecConfiguration.octetsPerFrame = octets_per_frame;
        sink_codec_config.codecConfiguration.samplingFrequency = samp_freq;

        BapUnicastClientCodecConfigReq(instance->bap_profile_handle, 1, &sink_codec_config);
    }
}

void BapProfileClient_ConfigureCodecForSourceAse(gatt_cid_t cid,
                                                 uint8 source_ase_id,
                                                 uint8 channel_alloc,
                                                 uint8 frame_duration,
                                                 uint16 samp_freq,
                                                 uint16 octets_per_frame,
                                                 uint8 target_latency,
                                                 uint8 target_phy)
{
    BapAseCodecConfiguration source_codec_config;

    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);


    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_ConfigureCodecForSinkAse");

        source_codec_config.aseId = source_ase_id;
        source_codec_config.serverDirection = BAP_SERVER_DIRECTION_SOURCE;
        source_codec_config.targetLatency = target_latency; //3
        source_codec_config.targetPhy = target_phy;
        source_codec_config.codecId.codecId = BAP_CODEC_ID_LC3;
        source_codec_config.codecId.companyId = 0;
        source_codec_config.codecId.vendorCodecId = 0;
        source_codec_config.codecConfiguration.audioChannelAllocation = channel_alloc;
        source_codec_config.codecConfiguration.frameDuaration = frame_duration;
        source_codec_config.codecConfiguration.lc3BlocksPerSdu = BAP_DEFAULT_LC3_BLOCKS_PER_SDU;
        source_codec_config.codecConfiguration.octetsPerFrame = octets_per_frame;
        source_codec_config.codecConfiguration.samplingFrequency = samp_freq;

        BapUnicastClientCodecConfigReq(instance->bap_profile_handle, 1, &source_codec_config);
    }
}

void BapProfileClient_AseEnableRequest(gatt_cid_t cid, uint8 ase_id, uint32 streaming_context)
{
    BapAseEnableParameters params;

    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_AseEnableRequest");

        params.aseId = ase_id;
        params.metadata = NULL;
        params.metadataLen = 0;
        params.streamingAudioContexts = streaming_context;

        BapUnicastClientEnableReq(instance->bap_profile_handle, 1, &params);
    }
}

void BapProfileClient_DiscoverCapabilityRequest(gatt_cid_t cid, uint8 pac_record_type)
{
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_DiscoverCapabilityRequest");

        BapDiscoverRemoteAudioCapabilityReq(instance->bap_profile_handle, pac_record_type);
    }

}

void BapProfileClient_ConfigureCig(uint8 cis_count,
                                   uint16 trans_lat_m_to_s,
                                   uint16 trans_lat_s_to_m,
                                   uint32 sdu_int_m_to_s,
                                   uint32 sdu_int_s_to_m,
                                   uint16 maxSduMtoS,
                                   uint16 maxSduStoM,
                                   uint8 rtn)
{
    int i;
    BapUnicastClientCigParameters params = {0};

    DEBUG_LOG("BapProfileClient_ConfigureCig");

    params.cigId = 0;
    params.cisCount = cis_count;
    params.framing = 0; 
    params.maxTransportLatencyMtoS = trans_lat_m_to_s;
    params.maxTransportLatencyStoM = trans_lat_s_to_m;
    params.packing = 1; //interleaved
    params.sca = 0;
    params.sduIntervalMtoS = sdu_int_m_to_s;
    params.sduIntervalStoM = sdu_int_s_to_m;
    params.cisConfig = (BapUnicastClientCisConfig *)PanicNull(malloc(sizeof(BapUnicastClientCisConfig) * cis_count));

    for (i = 0; i < cis_count; i++)
    {
        params.cisConfig[i].cisId = i + 1;
        params.cisConfig[i].maxSduMtoS = maxSduMtoS;
        params.cisConfig[i].maxSduStoM = maxSduStoM;
        params.cisConfig[i].phyMtoS = 0x02;
        params.cisConfig[i].phyStoM = 0x02;
        params.cisConfig[i].rtnMtoS = rtn;
        params.cisConfig[i].rtnStoM = rtn;
        bap_profile_client_taskdata->cig_info.cis_info[i].cis_id = i + 1;

        if (maxSduMtoS != 0 && maxSduStoM != 0)
        {
            bap_profile_client_taskdata->cig_info.cis_info[i].direction = DATAPATH_DIRECTION_INPUT | DATAPATH_DIRECTION_OUTPUT;
        }
        else if (maxSduMtoS != 0)
        {
            bap_profile_client_taskdata->cig_info.cis_info[i].direction = DATAPATH_DIRECTION_INPUT;
        }
        else if (maxSduStoM != 0)
        {
            bap_profile_client_taskdata->cig_info.cis_info[i].direction = DATAPATH_DIRECTION_OUTPUT;
        }
    }

    BapUnicastClientCigConfigReq(TrapToOxygenTask((Task) &bap_profile_client_taskdata->task_data),
                                 &params);
    pfree(params.cisConfig);
}

void BapProfileClient_ConfigureQoSForAse(gatt_cid_t cid,
                                         uint8 ase_id,
                                         uint8 cis_id,
                                         uint32 sdu_interval,
                                         uint16 sdu_size,
                                         uint8 rtn,
                                         uint16 trans_lat,
                                         uint8 phy)
{
    int ase_index;
    BapAseQosConfiguration qos;

    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        for (ase_index = 0; ase_index < MAX_BAP_ASE_SUPPORTED; ase_index++)
        {
            if (instance->ase_info[ase_index].ase_id == ase_id &&
                instance->ase_info[ase_index].ase_state == BAP_ASE_STATE_CODEC_CONFIGURED)
            {
                qos.aseId = instance->ase_info[ase_index].ase_id;
                qos.cisId = cis_id;
                qos.cigId = bap_profile_client_taskdata->cig_info.cig_id;
                qos.cisHandle = bapProfileClient_GetCisHandle(cis_id);
                qos.qosConfiguration.framing = 0;
                qos.qosConfiguration.phy = phy;
                qos.qosConfiguration.presentationDelay = instance->ase_info[ase_index].presentationDelayMin;
                qos.qosConfiguration.rtn = rtn;
                qos.qosConfiguration.sduInterval = sdu_interval;
                qos.qosConfiguration.sduSize = sdu_size;
                qos.qosConfiguration.transportLatency = trans_lat;

                BapUnicastClientQosConfigReq(instance->bap_profile_handle,
                                             1,
                                             &qos);
                break;
            }
        }
    }
}

void BapProfileClient_CreateCis(gatt_cid_t cid, uint8 cis_id)
{
    BapUnicastClientCisConnection cis_info;
    TP_BD_ADDR_T  bs_addr;
    tp_bdaddr tp_addr;

    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    PanicFalse(instance != NULL);

    GattConnect_GetTpaddrFromConnectionId(instance->cid, &tp_addr);
    BdaddrConvertTpVmToBluestack(&bs_addr, &tp_addr);

    cis_info.cisId = cis_id;
    cis_info.cisHandle = bapProfileClient_GetCisHandle(cis_id);
    memcpy(&cis_info.tpAddrt, &bs_addr, sizeof(TP_BD_ADDR_T));

    DEBUG_LOG("BapProfileClient_CreateCis cis_id %d, cis_handle %d", cis_info.cisId, cis_info.cisHandle);

    BapUnicastClientCisConnectReq(instance->bap_profile_handle, 1, &cis_info);
}

void BapProfileClient_ExecuteAseReceiverready(gatt_cid_t cid, uint8 ase_id, uint8 ready_req)
{
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    PanicFalse(instance != NULL);
    DEBUG_LOG("BapProfileClient_ExecuteAseReceiverready");
    BapUnicastClientReceiverReadyReq(instance->bap_profile_handle, ready_req, 1, &ase_id);
}

void BapProfileClient_DisableAse(gatt_cid_t cid, uint8 ase_id)
{
    BapAseParameters ase;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    PanicFalse(instance != NULL);
    ase.aseId = ase_id;

    BapUnicastClientDisableReq(instance->bap_profile_handle, 1, &ase);
}

void BapProfileClient_ReleaseAse(gatt_cid_t cid, uint8 ase_id)
{
    BapAseParameters ase;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    PanicFalse(instance != NULL);

    ase.aseId = ase_id;
    BapUnicastClientReleaseReq(instance->bap_profile_handle, 1, &ase);
}

void BapProfileClient_UpdateMetadataRequest(gatt_cid_t cid, uint8 ase_id, uint32 streaming_contexts)
{
    BapAseMetadataParameters update_metadata;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    PanicFalse(instance != NULL);

    update_metadata.aseId = ase_id;
    update_metadata.streamingAudioContexts = streaming_contexts;
    update_metadata.metadataLen = 0;
    update_metadata.metadata = NULL;

    BapUnicastClientUpdateMetadataReq(instance->bap_profile_handle, 1, &update_metadata);
}

bool BapProfileClient_StartScanningForSource(gatt_cid_t cid, bool add_collocated)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_StartScanningForSource");

        instance->source_found = FALSE;
        instance->add_collocated = add_collocated;
        instance->cid = cid;

        BapBroadcastAssistantStartScanReq(instance->bap_profile_handle,
                                      instance->add_collocated ? BROADCAST_SRC_COLLOCATED: BROADCAST_SRC_NON_COLLOCATED,
                                      BAP_CONTEXT_TYPE_MEDIA,
                                      BAP_SCAN_PARAM_DEFAULT,
                                      BAP_SCAN_PARAM_DEFAULT,
                                      BAP_SCAN_PARAM_DEFAULT);
        status = TRUE;
    }

    return status;
}

bool BapProfileClient_SyncToSource(gatt_cid_t cid)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL && instance->source_found)
    {
        DEBUG_LOG("BapProfileClient_SyncToSource");

        BapBroadcastAssistantSyncToSrcStartReq(instance->bap_profile_handle, &instance->source_addr, instance->adv_sid);
        status = TRUE;
    }

    return status;
}

bool BapProfileClient_StopScanningForSource(gatt_cid_t cid)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle,
                                                                                        (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_StopScanningForSource");

        BapBroadcastAssistantStopScanReq(instance->bap_profile_handle, instance->scan_handle);
        status = TRUE;
    }

    return status;
}

bool BapProfileClient_ReadBrs(gatt_cid_t cid)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_ReadBrs");

        instance->source_id = 0x0;

        BapBroadcastAssistantReadBRSReq(instance->bap_profile_handle, instance->source_id, TRUE);
        status = TRUE;
    }

    return status;
}

bool BapProfileClient_RegisterForGattNotification(gatt_cid_t cid)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_RegisterForGattNotification");

        BapBroadcastAssistantBRSRegisterForNotificationReq(instance->bap_profile_handle, instance->source_id, TRUE, TRUE);
        status = TRUE;
    }
    return status;
}

bool BapProfileClient_AddSource(gatt_cid_t cid, bool is_collocated, uint32 bis_index, uint8 pa_sync_state)
{
    bool status = FALSE;
    BapSubgroupInfo subgroup_info;

    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid,
                                                                                        (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_AddSource");

        subgroup_info.bisSyncState = bis_index;
        subgroup_info.metadataLen = 0x0;
        subgroup_info.metadataValue = NULL;

        BapBroadcastAssistantAddSrcReq(instance->bap_profile_handle,
                                       &instance->source_addr.addr,
                                       instance->source_addr.type,
                                       is_collocated,
                                       instance->adv_handle,
                                       instance->adv_sid,
                                       pa_sync_state,
                                       BAP_BROADCAST_PA_INTERVAL_DEFAULT,
                                       instance->bcast_id,
                                       BAP_BROADCAST_NUMBER_OF_SUBGROUP_SUPPORTED,
                                       &subgroup_info);
        status = TRUE;
    }

    return status;
}

bool BapProfileClient_ModifySource(gatt_cid_t cid, bool is_collocated, uint32 bis_index, uint8 pa_sync_state)
{
    bool status = FALSE;
    BapSubgroupInfo subgroup_info;

    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_profile_handle,
                                                                                        (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_ModifySource");

        subgroup_info.bisSyncState = bis_index;
        subgroup_info.metadataLen = 0x0;
        subgroup_info.metadataValue = NULL;

        BapBroadcastAssistantModifySrcReq(instance->bap_profile_handle,
                                          instance->source_id,
                                          is_collocated,
                                          instance->adv_handle,
                                          instance->adv_sid,
                                          pa_sync_state,
                                          BAP_BROADCAST_PA_INTERVAL_DEFAULT,
                                          BAP_BROADCAST_NUMBER_OF_SUBGROUP_SUPPORTED,
                                          &subgroup_info);
        status = TRUE;
    }

    return status;
}

bool BapProfileClient_RemoveSource(gatt_cid_t cid)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_RemoveSource");

        BapBroadcastAssistantRemoveSrcReq(instance->bap_profile_handle, instance->source_id);
        status = TRUE;
    }

    return status;
}

bool BapProfileClient_AssistantBcastSetCode(gatt_cid_t cid, uint8 *broadcast_code)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_AssistantBcastSetCode");

        BapBroadcastAssistantSetCodeRsp(instance->bap_profile_handle, instance->source_id, broadcast_code);
        status = TRUE;
    }

    return status;
}
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

void BapProfileClient_CreateDataPathForCis(gatt_cid_t cid, uint8 ase_id, uint8 cis_id, uint8 direction)
{
    int ase_index;
    BapSetupDataPath data_path;
    bap_profile_client_cis_info_t cis_info;

    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    PanicFalse(instance != NULL);

    DEBUG_LOG("BapProfileClient_CreateDataPathForCis");

    for (ase_index = 0; ase_index < MAX_BAP_ASE_SUPPORTED; ase_index++)
    {
        if (instance->ase_info[ase_index].ase_id == ase_id)
        {
            PanicFalse(bapProfileClient_GetCisInfoById(cis_id, &cis_info));

            memset(&data_path, 0, sizeof(BapSetupDataPath));
            data_path.isoHandle = instance->ase_info[ase_index].cis.cis_handle;
            data_path.dataPathId = DATAPATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
            data_path.codecId.codecId = BAP_CODEC_ID_LC3;
            data_path.codecId.companyId = 0;
            data_path.codecId.vendorCodecId = 0;
            data_path.dataPathDirection = direction;
            data_path.controllerDelay = 0x1;
            memcpy(&data_path.codecConfiguration, &instance->ase_info[ase_index].codec_config, sizeof(BapCodecConfiguration));

            BapClientSetupDataPathReq(instance->bap_profile_handle, &data_path);
            break;
        }
    }
}

void BapProfileClient_PopulateSpeakerPathConfig(gatt_cid_t cid, uint8 ase_id, uint32 channel, uint16 frame_duration, uint32 sample_rate)
{
    int ase_index;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    PanicFalse(instance != NULL);

    DEBUG_LOG("BapProfileClient_PopulateSpeakerPathConfig");

    for (ase_index = 0; ase_index < MAX_BAP_ASE_SUPPORTED; ase_index++)
    {
        if (instance->ase_info[ase_index].ase_id == ase_id)
        {
            if (channel & BAP_AUDIO_LOCATION_FL || channel & BAP_AUDIO_LOCATION_MONO)
            {
                instance->spkr_audio_path.source_iso_handle = instance->ase_info[ase_index].cis.cis_handle;
            }

            if (channel & BAP_AUDIO_LOCATION_FR)
            {
                instance->spkr_audio_path.source_iso_handle_right = instance->ase_info[ase_index].cis.cis_handle;
            }

            instance->spkr_audio_path.codec_frame_blocks_per_sdu = instance->ase_info[ase_index].codec_config.lc3BlocksPerSdu;
            /* TBD: There seems an issue where BAP frame duration returned in instance is not correct, either remote is sending it 
                wrongly or is not returned properly from locally, untill then we can populate the values directrly from pydbg inrerface
            instance->spkr_audio_path.frame_duration = instance->ase_info[ase_index].codec_config.frameDuaration;*/
            instance->spkr_audio_path.frame_duration = frame_duration;
            instance->spkr_audio_path.frame_length = instance->ase_info[ase_index].codec_config.octetsPerFrame;
            instance->spkr_audio_path.gaming_mode = FALSE;
            /* TBD: There seems an issue where BAP sample rate returned in instance is not correct, either remote is sending it 
                wrongly or is not returned properly from locally, untill then we can populate the values directrly from pydbg inrerface
            instance->spkr_audio_path.sample_rate = instance->ase_info[ase_index].codec_config.samplingFrequency;*/
            instance->spkr_audio_path.sample_rate = sample_rate;
            break;
        }
    }
}

void BapProfileClient_PopulateMicPathConfig(gatt_cid_t cid, uint8 ase_id, uint32 channel, uint16 frame_duration, uint32 sample_rate)
{
    int ase_index;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    PanicFalse(instance != NULL);

    DEBUG_LOG("BapProfileClient_PopulateMicPathConfig");

    for (ase_index = 0; ase_index < MAX_BAP_ASE_SUPPORTED; ase_index++)
    {
        if (instance->ase_info[ase_index].ase_id == ase_id)
        {
            if (channel & BAP_AUDIO_LOCATION_FL || channel & BAP_AUDIO_LOCATION_MONO)
            {
                instance->mic_audio_path.source_iso_handle = instance->ase_info[ase_index].cis.cis_handle;
            }

            if (channel & BAP_AUDIO_LOCATION_FR)
            {
                instance->mic_audio_path.source_iso_handle_right = instance->ase_info[ase_index].cis.cis_handle;
            }

            instance->mic_audio_path.codec_frame_blocks_per_sdu = instance->ase_info[ase_index].codec_config.lc3BlocksPerSdu;
            /* TBD: There seems an issue where BAP frame duration returned in instance is not correct, either remote is sending it 
                wrongly or is not returned properly from locally, untill then we can populate the values directrly from pydbg inrerface
            instance->mic_audio_path.frame_duration = instance->ase_info[ase_index].codec_config.frameDuaration;*/
            instance->mic_audio_path.frame_duration = frame_duration;
            instance->mic_audio_path.frame_length = instance->ase_info[ase_index].codec_config.octetsPerFrame;
            //instance->mic_audio_path.gaming_mode = FALSE;
            /* TBD: There seems an issue where BAP sample rate returned in instance is not correct, either remote is sending it 
                wrongly or is not returned properly from locally, untill then we can populate the values directrly from pydbg inrerface
            instance->spkr_audio_path.sample_rate = instance->ase_info[ase_index].codec_config.samplingFrequency;*/
            instance->mic_audio_path.sample_rate = sample_rate;
            break;
        }
    }
}

void BapProfileClient_ConnectToAllSupportedBapServers(bool connect_all)
{
    bap_profile_client_connect_all_servers = connect_all;
}

bool BapProfileClient_GetSpeakerPathConfig(gatt_cid_t cid, bap_media_config_t *media_config)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    PanicFalse(media_config != NULL);

    if (instance != NULL &&
        (instance->spkr_audio_path.source_iso_handle != 0xFFFF ||
         instance->spkr_audio_path.source_iso_handle_right != 0xFFFF))
    {
        memcpy(media_config, &instance->spkr_audio_path, sizeof(bap_media_config_t));
        status = TRUE;
    }

    return status;
}
bool BapProfileClient_GetMicPathConfig(gatt_cid_t cid, bap_microphone_config_t *mic_config)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    PanicFalse(mic_config != NULL);

    if (instance != NULL &&
        (instance->mic_audio_path.source_iso_handle != 0xFFFF ||
         instance->mic_audio_path.source_iso_handle_right != 0xFFFF))
    {
        memcpy(mic_config, &instance->mic_audio_path, sizeof(bap_microphone_config_t));
        status = TRUE;
    }

    return status;
}

bool BapProfileClient_SetDeviceBdAddressToAdd(gatt_cid_t cid, BD_ADDR_T *bd_addr)
{
    bool status = FALSE;
    bap_profile_client_device_instance_t *instance = bapProfileClient_GetDeviceInstance(bap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        DEBUG_LOG("BapProfileClient_SetDeviceBdAddressToAdd");

        instance->source_addr.addr.nap = bd_addr->nap;
        instance->source_addr.addr.uap = bd_addr->uap;
        instance->source_addr.addr.lap = bd_addr->lap;
        instance->source_addr.type = 0x0;
        status = TRUE;
    }

    return status;
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

