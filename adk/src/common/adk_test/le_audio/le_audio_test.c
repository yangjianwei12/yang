/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of common LE Audio specifc testing functions.
*/

#include "le_audio_test.h"
#include <logging.h>
#include <macros.h>

#include "focus_audio_source.h"
#include "focus_voice_source.h"
#include "le_advertising_manager_select_extended.h"
#ifdef INCLUDE_LE_AUDIO_BROADCAST
#include <gatt.h>
#include "le_broadcast_manager.h"
#include "le_broadcast_manager_source.h"
#include "le_audio_messages.h"
#endif
#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#include "le_audio_client.h"
#endif
#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
#include "le_broadcast_manager_self_scan.h"
#endif
#ifdef INCLUDE_TWS
#include "tws_topology.h"
#endif
#include "volume_messages.h"
#include "volume_service.h"

#include "connection_manager_list.h"
#include "bt_device.h"

#include "mirror_profile.h"


#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)

#ifndef LE_AUDIO_TEST_PA_DATA_MAX_SIZE
#define LE_AUDIO_TEST_PA_DATA_MAX_SIZE      0x08u
#endif

static void leAudioTest_taskHandler(Task task, MessageId id, Message msg);

static TaskData le_audio_test_taskdata = {leAudioTest_taskHandler};

#ifdef INCLUDE_LE_AUDIO_BROADCAST
static uint8 le_audio_test_pa_data_len;
static uint8 le_audio_test_pa_data[LE_AUDIO_TEST_PA_DATA_MAX_SIZE];
#endif /* INCLUDE_LE_AUDIO_BROADCAST */
#endif /* INCLUDE_LE_AUDIO_BROADCAST || INCLUDE_LE_AUDIO_UNICAST */

bool leAudioTest_IsExtendedAdvertisingActive(void)
{
#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    // needs revisiting, maybe remove
    return FALSE;
#else
    return LeAdvertisingManager_IsExtendedAdvertisingActive();
#endif
}

bool leAudioTest_IsBroadcastReceiveActive(void)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    return LeBroadcastManager_IsBroadcastReceiveActive();
#else
    return FALSE;
#endif
}

bool leAudioTest_IsAnyBroadcastSourceSyncedToPa(void)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    return LeBroadcastManager_IsAnySourceSyncedToPa();
#else
    return FALSE;
#endif
}

bool leAudioTest_IsAnyBroadcastSourceSyncedToBis(void)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    return LeBroadcastManager_IsAnySourceSyncedToBis();
#else
    return FALSE;
#endif
}

static bool leAudioTest_SetVolumeForLeaAudioSource(audio_source_t source, uint8 volume)
{
#ifdef INCLUDE_TWS
    if (TwsTopology_IsRoleSecondary())
    {
        DEBUG_LOG_ALWAYS("This Test API should never be called on the secondary Earbud");
        return FALSE;
    }
#endif
    audio_source_t focused_source = audio_source_none;
    if (!Focus_GetAudioSourceForContext(&focused_source))
    {
        DEBUG_LOG_ALWAYS("no focused audio source");
        return FALSE;
    }
    if (focused_source != source)
    {
        DEBUG_LOG_ALWAYS("focused audio source is not enum:audio_source_t:%d", source);
        return FALSE;
    }
    Volume_SendAudioSourceVolumeUpdateRequest(focused_source, event_origin_local, volume);
    return TRUE;
}

bool leAudioTest_SetVolumeForBroadcast(uint8 volume)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SetVolumeForBroadcast %d", volume);
    return leAudioTest_SetVolumeForLeaAudioSource(audio_source_le_audio_broadcast, volume);
}

bool leAudioTest_SetVolumeForUnicastMusic(uint8 volume)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SetVolumeForUnicast %d", volume);
    return leAudioTest_SetVolumeForLeaAudioSource(audio_source_le_audio_unicast_1, volume);
}

static bool leAudioTest_SetMuteForLeaAudioSource(audio_source_t source, bool mute_state)
{
#ifdef INCLUDE_TWS
    if (TwsTopology_IsRoleSecondary())
    {
        DEBUG_LOG_ALWAYS("This Test API should never be called on the secondary Earbud");
        return FALSE;
    }
#endif
    audio_source_t focused_source = audio_source_none;
    if (!Focus_GetAudioSourceForContext(&focused_source))
    {
        DEBUG_LOG_ALWAYS("no focused audio source");
        return FALSE;
    }
    if (focused_source != source)
    {
        DEBUG_LOG_ALWAYS("focused audio source is not enum:audio_source_t:%d", source);
        return FALSE;
    }
    Volume_SendAudioSourceMuteRequest(focused_source, event_origin_local, mute_state);
    return TRUE;
}

bool leAudioTest_SetMuteForBroadcast(bool mute_state)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SetMuteForBroadcast %d", mute_state);
    return leAudioTest_SetMuteForLeaAudioSource(audio_source_le_audio_broadcast, mute_state);
}

bool leAudioTest_SetMuteForUnicastMusic(bool mute_state)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SetMuteForUnicastMusic %d", mute_state);
    return leAudioTest_SetMuteForLeaAudioSource(audio_source_le_audio_unicast_1, mute_state);
}

bool leAudioTest_PauseBroadcast(void)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    DEBUG_LOG_ALWAYS("leAudioTest_PauseBroadcast");
    LeBroadcastManager_Pause(&le_audio_test_taskdata);
    return TRUE;
#else
    return FALSE;
#endif
}

bool leAudioTest_ResumeBroadcast(void)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    DEBUG_LOG_ALWAYS("leAudioTest_ResumeBroadcast");
    LeBroadcastManager_Resume(&le_audio_test_taskdata);
    return TRUE;
#else
    return FALSE;
#endif
}

bool leAudioTest_IsBroadcastPaused(void)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    bool paused = LeBroadcastManager_IsPaused();
    DEBUG_LOG_ALWAYS("leAudioTest_IsBroadcastPaused %d", paused);
    return paused;
#else
    return TRUE;
#endif
}

bool leAudioTest_SetVolumeForUnicastVoice(uint8 volume)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SetVolumeForUnicastVoice %d", volume);
#ifdef INCLUDE_TWS
    if (TwsTopology_IsRoleSecondary())
    {
        DEBUG_LOG_ALWAYS("This Test API should never be called on the secondary Earbud");
        return FALSE;
    }
#endif
    voice_source_t focused_source = voice_source_none;
    if (!Focus_GetVoiceSourceForContext(ui_provider_telephony, &focused_source))
    {
        DEBUG_LOG_ALWAYS("no focused voice source");
        return FALSE;
    }
    if (focused_source != voice_source_le_audio_unicast_1)
    {
        DEBUG_LOG_ALWAYS("focused audio source is not enum:voice_source_t:%d", voice_source_le_audio_unicast_1);
        return FALSE;
    }
    Volume_SendVoiceSourceVolumeUpdateRequest(focused_source, event_origin_local, volume);
    return TRUE;
}

bool leAudioTest_SetMuteForUnicastVoice(bool mute_state)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SetMuteForUnicastVoice %d", mute_state);
#ifdef INCLUDE_TWS
    if (TwsTopology_IsRoleSecondary())
    {
        DEBUG_LOG_ALWAYS("This Test API should never be called on the secondary Earbud");
        return FALSE;
    }
#endif
    voice_source_t focused_source = voice_source_none;
    if (!Focus_GetVoiceSourceForContext(ui_provider_telephony, &focused_source))
    {
        DEBUG_LOG_ALWAYS("no focused voice source");
        return FALSE;
    }
    if (focused_source != voice_source_le_audio_unicast_1)
    {
        DEBUG_LOG_ALWAYS("focused audio source is not enum:voice_source_t:%d", voice_source_le_audio_unicast_1);
        return FALSE;
    }
    Volume_SendVoiceSourceMuteRequest(focused_source, event_origin_local, mute_state);
    return TRUE;
}

int leAudioTest_GetVolumeForBroadcast(void)
{
    int volume = AudioSources_GetVolume(audio_source_le_audio_broadcast).value;

    DEBUG_LOG_ALWAYS("leAudioTest_GetVolumeForBroadcast Volume:%d", volume);

    return volume;
}

int leAudioTest_GetVolumeForUnicastMusic(void)
{
    int volume = AudioSources_GetVolume(audio_source_le_audio_unicast_1).value;

    DEBUG_LOG_ALWAYS("leAudioTest_GetVolumeForUnicastMusic Volume:%d", volume);

    return volume;
}

int leAudioTest_GetVolumeForUnicastVoice(void)
{
    int volume = VoiceSources_GetVolume(voice_source_le_audio_unicast_1).value;

    DEBUG_LOG_ALWAYS("leAudioTest_GetVolumeForUnicastVoice Volume:%d", volume);

    return volume;
}

#ifdef INCLUDE_LE_AUDIO_UNICAST

/*! Increment/decrement the volume of unicast sink */
static void leAudioTest_UnicastSinkVolumeChange(source_type_t source_type, bool is_vol_down)
{
    int step_size;
    bool is_audio_src = source_type == source_type_audio;

    step_size = VolumeService_GetUiStepSize(is_audio_src ? AudioSources_GetVolume(audio_source_le_audio_unicast_1).config :
                                                           VoiceSources_GetVolume(voice_source_le_audio_unicast_1).config);

    if (is_vol_down)
    {
        step_size *= -1;
    }

    if (is_audio_src)
    {
        VolumeService_ChangeAudioSourceVolume(audio_source_le_audio_unicast_1, step_size);
    }
    else
    {
        VolumeService_ChangeVoiceSourceVolume(voice_source_le_audio_unicast_1, step_size);
    }
}

void leAudioTest_UnicastAudioSinkVolumeUp(void)
{
    DEBUG_LOG_ALWAYS("leAudioTest_UnicastAudioSinkVolumeUp");

    leAudioTest_UnicastSinkVolumeChange(source_type_audio, FALSE);
}

void leAudioTest_UnicastAudioSinkVolumeDown(void)
{
    DEBUG_LOG_ALWAYS("leAudioTest_UnicastAudioSinkVolumeDown");

    leAudioTest_UnicastSinkVolumeChange(source_type_audio, TRUE);
}

void leAudioTest_UnicastVoiceSinkVolumeUp(void)
{
    DEBUG_LOG_ALWAYS("leAudioTest_UnicastVoiceSinkVolumeUp");

    leAudioTest_UnicastSinkVolumeChange(source_type_voice, FALSE);
}

void leAudioTest_UnicastVoiceSinkVolumeDown(void)
{
    DEBUG_LOG_ALWAYS("leAudioTest_UnicastVoiceSinkVolumeDown");

    leAudioTest_UnicastSinkVolumeChange(source_type_voice, TRUE);
}

void leAudioTest_SendAseReleaseRequest(gatt_cid_t cid, uint8 ase_id)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SendAseReleaseRequest");
    LeUnicastManager_SendAseReleaseRequest(cid, ase_id);
}

void leAudioTest_SendAseReleasedRequest(gatt_cid_t cid, uint8 ase_id)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SendAseReleasedRequest");
    LeUnicastManager_SendAseReleasedRequest(cid, ase_id);
}

void leAudioTest_SendAseDisableRequest(gatt_cid_t cid, uint8 ase_id)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SendAseDisableRequest");
    LeUnicastManager_SendAseDisableRequest(cid, ase_id);
}

void leAudioTest_SendAseAseConfigureCodecReq(gatt_cid_t cid, uint8 ase_id, le_um_codec_config_set_t config_set)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SendAseAseConfigureCodecReq");
    LeUnicastManager_SendAseConfigureCodecReq(cid, ase_id, config_set);
}

void leAudioTest_SendAseMetadataUpdateReq(gatt_cid_t cid, uint8 ase_id)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SendAseMetadataUpdateReq");
    LeUnicastManager_SendAseMetadataUpdate(cid, ase_id);
}

bool leAudioTest_IsCcpConnected(void)
{
    bool is_connected = CallClientControl_IsCcpConnected();
    DEBUG_LOG_ALWAYS("leAudioTest_IsCcpConnected %d", is_connected);

    return is_connected;
}

bool appTestIsLeaUnicastStreaming(void)
{
    bool is_streaming = LeUnicastManager_IsStreamingActive();

    DEBUG_LOG_ALWAYS("appTestIsLeaUnicastStreaming %d", is_streaming);
    return is_streaming;
}

uint16 appTestLeaUnicastGetAudioContext(void)
{
    uint16 audio_context = LeUnicastManager_GetUnicastAudioContext();

    DEBUG_LOG_ALWAYS("appTestIsLeaUnicastGetAudioContext audio_context=enum:audio_context_t:%d", audio_context);
    return audio_context;
}

bool leAudioTest_ReadUnicastCisLinkQuality(void)
{
    hci_connection_handle_t handles[2] = {LE_INVALID_CIS_HANDLE};

#ifdef ENABLE_LEA_CIS_DELEGATION
    MirrorProfile_GetCisHandle(&handles[0], &handles[1]);

    DEBUG_LOG_ALWAYS("leAudioTest_ReadUnicastCisLinkQuality own-handle: 0x%x, peer-handle: 0x%x", handles[0], handles[1]);
#else
    DEBUG_LOG_ALWAYS("leAudioTest_ReadUnicastCisLinkQuality total handles: %d, handle-1: 0x%x, handle-2: 0x%x",
                     LeUnicastManager_GetCisHandles(ARRAY_DIM(handles), handles), handles[0], handles[1]);
#endif

    if (handles[0] != LE_INVALID_CIS_HANDLE)
    {
        CmIsocReadIsoLinkQualityReq(&le_audio_test_taskdata, handles[0]);
    }

    if (handles[1] != LE_INVALID_CIS_HANDLE)
    {
        CmIsocReadIsoLinkQualityReq(&le_audio_test_taskdata, handles[1]);
    }

    return handles[0] != LE_INVALID_CIS_HANDLE || handles[1] != LE_INVALID_CIS_HANDLE;
}

void leAudioTest_LeUnicastRemoveDataPaths(le_um_cis_t *cis_data)
{
    DEBUG_LOG_ALWAYS("leAudioTest_LeUnicastRemoveDataPaths: cis_data=%p", cis_data);
    LeUnicastManager_RemoveDataPaths(cis_data);
}

#endif /* INCLUDE_LE_AUDIO_UNICAST */

bool leAudioTest_AnyHandsetConnectedBothBredrAndLe(void)
{
    cm_connection_t *le_connection;

    le_connection = ConManagerFindFirstActiveLink(cm_transport_ble);
    while (le_connection)
    {
        const tp_bdaddr *address = ConManagerGetConnectionTpAddr(le_connection);
        if (address)
        {
            tp_bdaddr le_address = {0};

            if (ConManagerResolveTpaddr(address, &le_address))
            {
                tp_bdaddr bredr_address = le_address;
                bredr_address.transport = TRANSPORT_BREDR_ACL;
                if (ConManagerFindConnectionFromBdAddr(&bredr_address))
                {
                    DEBUG_LOG_ALWAYS("leAudioTest_AnyHandsetConnectedBothBredrAndLe. Found device with LE and BREDR addr:(0x%6lx)",
                                le_address.taddr.addr.lap);
                    return TRUE;
                }
            }
        }
        le_connection = ConManagerFindNextActiveLink(le_connection, cm_transport_ble);
    }

    DEBUG_LOG_ALWAYS("leAudioTest_AnyHandsetConnectedBothBredrAndLe. No devices.");

    return FALSE;
}

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
static void leAudioTest_taskHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

#ifdef INCLUDE_LE_AUDIO_UNICAST

    if (id == CM_PRIM && (*(CsrBtCmPrim *) msg) == CM_ISOC_READ_ISO_LINK_QUALITY_CFM)
    {
        CmIsocReadIsoLinkQualityCfm *cfm = (CmIsocReadIsoLinkQualityCfm *) msg;

        DEBUG_LOG_ALWAYS("leAudioTest_taskHandler::CM_ISOC_READ_ISO_LINK_QUALITY_CFM handle: 0x%x, result: {code: 0x%x, supplier: 0x%x}",
                         cfm->handle, cfm->resultCode, cfm->resultSupplier);
        DEBUG_LOG_ALWAYS("     tx_unacked_packets: 0x%x, tx_flushed_packets: 0x%x, tx_last_subevent_packets: 0x%x, retransmitted_packets: 0x%x",
                         cfm->tx_unacked_packets, cfm->tx_flushed_packets, cfm->tx_last_subevent_packets, cfm->retransmitted_packets);
        DEBUG_LOG_ALWAYS("     crc_error_packets: 0x%x, rx_unreceived_packets: 0x%x, duplicate_packets: 0x%x",
                         cfm->crc_error_packets, cfm->rx_unreceived_packets, cfm->duplicate_packets);
    }

#else /* INCLUDE_LE_AUDIO_UNICAST */

    UNUSED(id);
    UNUSED(msg);

#endif /* INCLUDE_LE_AUDIO_UNICAST */

#ifdef INCLUDE_LE_AUDIO_BROADCAST
    if (id == LE_AUDIO_BROADCAST_METADATA_PAYLOAD)
    {
        LE_AUDIO_BROADCAST_METADATA_PAYLOAD_T *message = (LE_AUDIO_BROADCAST_METADATA_PAYLOAD_T *) msg;

        DEBUG_LOG_ALWAYS("leAudioTest_taskHandler: LE_AUDIO_BROADCAST_METADATA_PAYLOAD metadata_len: 0x%x, ltv_1_len: 0x%x, ltv_1_type: 0x%x",
                         message->metadata_len, message->metadata[0], message->metadata[1]);

        le_audio_test_pa_data_len = message->metadata_len;
        if (le_audio_test_pa_data_len <= LE_AUDIO_TEST_PA_DATA_MAX_SIZE)
        {
            memcpy(le_audio_test_pa_data, message->metadata, message->metadata_len);
        }
    }
#endif /* INCLUDE_LE_AUDIO_BROADCAST */

#ifdef ENABLE_ACK_FOR_PA_TRANSMITTED
    if(id == LE_AUDIO_CLIENT_PA_TRANSMITTED_IND)
    {
        DEBUG_LOG_ALWAYS("leAudioTest_taskHandler: LE_AUDIO_CLIENT_PA_TRANSMITTED_IND");
        /* unregister, if new metadata is sent then it will register again */
        LeAudioClient_ClientUnregister(&le_audio_test_taskdata);
    }
#endif /* ENABLE_ACK_FOR_PA_TRANSMITTED */

}

#ifdef INCLUDE_LE_AUDIO_BROADCAST

#include "broadcast_sink_role.h"

uint16 leAudioTest_GetPastTimeout(void)
{
    uint16 timeout = LeBroadcastManager_GetPastTimeoutMs();

    DEBUG_LOG_ALWAYS("leAudioTest_GetPastTimeout timeout %ums", timeout);

    return timeout;
}

void leAudioTest_SetPastTimeout(uint16 timeout)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SetPastTimeout timeout %ums", timeout);

    LeBroadcastManager_SetPastTimeoutMs(timeout);
}

uint16 leAudioTest_GetFindTrainsTimeout(void)
{
    uint16 timeout = LeBroadcastManager_GetFindTrainsTimeout();

    DEBUG_LOG_ALWAYS("leAudioTest_GetFindTrainsTimeout timeout %ums", timeout);

    return timeout;
}

void leAudioTest_SetFindTrainsTimeout(uint16 timeout)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SetFindTrainsTimeout timeout %ums", timeout);

    LeBroadcastManager_SetFindTrainsTimeout(timeout);
}

uint16 leAudioTest_GetSyncToTrainTimeout(void)
{
    uint16 timeout = LeBapBroadcastSink_GetSyncToTrainTimeout();

    DEBUG_LOG_ALWAYS("leAudioTest_GetSyncToTrainTimeout timeout %ums", timeout);

    return timeout;
}

void leAudioTest_SetSyncToTrainTimeout(uint16 timeout)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SetSyncToTrainTimeout timeout %ums", timeout);

    LeBapBroadcastSink_SetSyncToTrainTimeout(timeout);
}

void leAudioTest_EnableBroadcastMetadataNotification(bool enable)
{
    DEBUG_LOG_ALWAYS("leAudioTest_EnableMetadataTransaction %d", enable);

    LeBroadcastManager_EnableMetadataNotification(enable);
    enable ? LeAudioMessages_ClientRegister(&le_audio_test_taskdata)
           : LeAudioMessages_ClientDeregister(&le_audio_test_taskdata);
}

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

/* Expected broadcast reciever as well as broadcast transmitter enabled to verify this functionality */
bool leAudioTest_UpdateBroadcastMetadata(void)
{
    bool status = FALSE;

    if (le_audio_test_pa_data_len > 0 && le_audio_test_pa_data_len <= LE_AUDIO_TEST_PA_DATA_MAX_SIZE)
    {
        status = LeAudioClientBroadcast_UpdateMetadataInPeriodicTrain(le_audio_test_pa_data_len, le_audio_test_pa_data);
        
#ifdef ENABLE_ACK_FOR_PA_TRANSMITTED
        LeAudioClient_ClientRegister(&le_audio_test_taskdata);
#endif /* ENABLE_ACK_FOR_PA_TRANSMITTED */
    }

    DEBUG_LOG_ALWAYS("leAudioTest_UpdateBroadcastMetadata metadata_len: 0x%x, ltv_1_len: 0x%x, ltv_1_type: 0x%x, status: %u",
                     le_audio_test_pa_data_len, le_audio_test_pa_data[0], le_audio_test_pa_data[1], status);

    return status;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

uint8 leAudioTest_BassAddSource(uint8 advertising_sid, uint32 broadcast_id, le_bm_pa_sync_t pa_sync)
{
    uint8 source_id = 0;

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)

    /* Minimal broadcast subgroup array; a single subgroup with no preferred BIS index and no metadata */
    le_bm_source_subgroup_t subgroups[] =
    {
        {
            .bis_sync = BIS_SYNC_NO_PREFERENCE
        }
    };

    le_bm_add_source_info_t add_source_info =
    {
        .advertising_sid = advertising_sid,
        .broadcast_id = broadcast_id,
        .pa_sync = pa_sync,
        .pa_interval = LE_BM_PA_INTERVAL_UNKNOWN,
        .num_subgroups = ARRAY_DIM(subgroups),
        .subgroups = subgroups
    };

    appDeviceGetMyBdAddr(&add_source_info.assistant_address.addr);
    LeBroadcastManager_BassAddSource(&source_id, &add_source_info);

    DEBUG_LOG_ALWAYS("leAudioTest_BassAddSource: source_id=%u", source_id);

#else
    UNUSED(advertising_sid);
    UNUSED(broadcast_id);
    UNUSED(pa_sync);

    DEBUG_LOG_ALWAYS("leAudioTest_BassAddSource: Feature Not Enabled");
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */

    return source_id;
}

le_bm_bass_status_t leAudioTest_BassModifySource(uint8 source_id, le_bm_pa_sync_t pa_sync)
{
    le_bm_bass_status_t result = le_bm_bass_status_failed;

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    /* Minimal broadcast subgroup array; a single subgroup with no preferred BIS index and no metadata */
    le_bm_source_subgroup_t subgroups[] =
    {
        {
            .bis_sync = BIS_SYNC_NO_PREFERENCE
        }
    };

    le_bm_modify_source_info_t modify_source_info =
    {
        .pa_sync = pa_sync,
        .pa_interval = LE_BM_PA_INTERVAL_UNKNOWN,
    };

    if (pa_sync != le_bm_pa_sync_none)
    {
        modify_source_info.num_subgroups = ARRAY_DIM(subgroups);
        modify_source_info.subgroups = subgroups;
    }

    result = LeBroadcastManager_BassModifySource(source_id, &modify_source_info);
    DEBUG_LOG_ALWAYS("leAudioTest_BassModifySource: source_id=%u pa_sync=enum:le_bm_pa_sync_t:%d result=enum:le_bm_bass_status_t:%d", source_id, pa_sync, result);
#else
    UNUSED(source_id);
    UNUSED(pa_sync);

    DEBUG_LOG_ALWAYS("leAudioTest_BassModifySource: feature not enabled");
#endif /* INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN */

    return result;
}

le_bm_bass_status_t leAudioTest_BassSetBroadcastCode(uint8 source_id, const char *code_string)
{
    le_bm_bass_status_t result = le_bm_bass_status_failed;

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    size_t code_len = strlen(code_string);

    if (code_len <= SCAN_DELEGATOR_BROADCAST_CODE_SIZE)
    {
        uint8 *broadcast_code = PanicNull(calloc(1, SCAN_DELEGATOR_BROADCAST_CODE_SIZE));

        memcpy(broadcast_code, code_string, code_len);
        result = LeBroadcastManager_BassSetBroadcastCode(source_id, broadcast_code);

        DEBUG_LOG_ALWAYS("leAudioTest_BassSetBroadcastCode: result=enum:le_bm_bass_status_t:%d", result);
        DEBUG_LOG_DATA_VERBOSE(broadcast_code, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);

        free(broadcast_code);
    }
    else
    {
        DEBUG_LOG_ALWAYS("leAudioTest_BassSetBroadcastCode: code string too long (%u > %u)", code_len, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);
    }

#else
    UNUSED(source_id);
    UNUSED(code_string);

    DEBUG_LOG_ALWAYS("leAudioTest_BassSetBroadcastCode: feature not enabled");
#endif /* INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN */

    return result;
}

le_bm_bass_status_t leAudioTest_BassRemoveSource(uint8 source_id)
{
    le_bm_bass_status_t result = le_bm_bass_status_failed;

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    result = LeBroadcastManager_BassRemoveSource(source_id);

    DEBUG_LOG_ALWAYS("leAudioTest_BassRemoveSource: source_id=%u result=enum:le_bm_bass_status_t:%d", source_id, result);
#else
    UNUSED(source_id);

    DEBUG_LOG_ALWAYS("leAudioTest_BassRemoveSource: feature not enabled");
#endif /* INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN */

    return result;
}

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
static void leAudioTest_SelfScanHandleCmExtendedScannerGlobalParamsCfm(CmExtScanGetGlobalParamsCfm *cfm)
{
    DEBUG_LOG_ALWAYS("leAudioTest_SelfScanHandleCmExtendedScannerGlobalParamsCfm Result 0x%x", cfm->resultCode);

    if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        DEBUG_LOG_ALWAYS("  flags 0x%x own_addr_type 0x%x scanning_filter_policy 0x%x",
                         cfm->flags, cfm->own_address_type, cfm->scanning_filter_policy);
        DEBUG_LOG_ALWAYS("  filter_duplicates 0x%x scanning_phys %u ",
                         cfm->filter_duplicates, cfm->scanning_phys);

        for (uint32 i = 0; i < cfm->scanning_phys; i++)
        {
            CmScanningPhy *phy = &cfm->phys[i];

            DEBUG_LOG_ALWAYS("    scan_type 0x%x scan_interval 0x%x scan_window 0x%x",
                             phy->scan_type, phy->scan_interval, phy->scan_window);
        }
    }

}

static bool leAudioTest_SelfScanHandleExtendedCmMessages(void *msg)
{
    CsrBtCmPrim *primType = (CsrBtCmPrim *) msg;
    bool status = TRUE;

    switch (*primType)
    {
        case CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_CFM:
            leAudioTest_SelfScanHandleCmExtendedScannerGlobalParamsCfm((CmExtScanGetGlobalParamsCfm *) msg);
        break;

        default:
        {
            status = FALSE;
        }
        break;
    }

    CmFreeUpstreamMessageContents((void *) msg);

    return status;
}


static void leAudioTest_SelfScanHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

    DEBUG_LOG_ALWAYS("leAudioTest_SelfScanHandler id enum:le_broadcast_manager_self_scan_msg_t:%u", id);

    switch (id)
    {
    case LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM:
        {
            LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM_T *cfm = (LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM_T *)msg;
            DEBUG_LOG_ALWAYS("  LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM status enum:le_broadcast_manager_self_scan_status_t:%u", cfm->status);
        }
        break;

    case LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM:
        {
            LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM_T *cfm = (LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM_T *)msg;
            DEBUG_LOG_ALWAYS("  LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM status enum:le_broadcast_manager_self_scan_status_t:%u", cfm->status);
        }
        break;

    case LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND:
        {
            LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND_T *ind = (LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND_T *)msg;
            DEBUG_LOG_ALWAYS("  LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND status enum:le_broadcast_manager_self_scan_status_t:%u", ind->status);
        }
        break;

    case LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND:
        {
            LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND_T *ind = (LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND_T *)msg;
            DEBUG_LOG_ALWAYS("  LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND broadcast_id 0x%x ", ind->broadcast_id);
            DEBUG_LOG_ALWAYS("    advertising_sid 0x%x", ind->adv_sid);
            DEBUG_LOG_ALWAYS("    rssi %d", ind->rssi);
            if (ind->broadcast_name)
            {
                DEBUG_LOG_ALWAYS("    broadcast_name");
                DEBUG_LOG_DATA_ALWAYS(ind->broadcast_name, ind->broadcast_name_len);
            }
        }
        break;

#ifdef USE_SYNERGY
    case CM_PRIM:
        leAudioTest_SelfScanHandleExtendedCmMessages((void *)msg);
        break;
#endif

    default:
        DEBUG_LOG_WARN("leAudioTest_SelfScanHandler unexpected id MESSAGE:0x%x", id);
        break;
    }
}

static TaskData le_audio_test_self_scan_task = {
    .handler = leAudioTest_SelfScanHandler
};
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */

void leAudioTest_SelfScanStart(uint32 timeout, uint32 broadcast_id)
{
#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    DEBUG_LOG_ALWAYS("leAudioTest_SelfScanStart timeout %u broadcast_id 0x%x", timeout, broadcast_id);

    self_scan_params_t scan_params = {
        .timeout = timeout,
        .filter = {
            .broadcast_id = broadcast_id,
        }
    };

    LeBroadcastManager_SelfScanStart(&le_audio_test_self_scan_task, &scan_params);
#else
    UNUSED(timeout);
    UNUSED(broadcast_id);

    DEBUG_LOG_ALWAYS("leAudioTest_SelfScanStart: feature not enabled");
#endif
}

void leAudioTest_SelfScanStop(void)
{
#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    DEBUG_LOG_ALWAYS("leAudioTest_SelfScanStop");

    LeBroadcastManager_SelfScanStop(&le_audio_test_self_scan_task);
#else
    DEBUG_LOG_ALWAYS("leAudioTest_SelfScanStop: feature not enabled");
#endif
}

void leAudioTest_SelfScanStopAll(void)
{
#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    DEBUG_LOG_ALWAYS("leAudioTest_SelfScanStopAll");

    LeBroadcastManager_SelfScanStopAll(&le_audio_test_self_scan_task);
#else
    DEBUG_LOG_ALWAYS("leAudioTest_SelfScanStopAll: feature not enabled");
#endif
}

le_bm_bass_status_t leAudioTest_BassSetSourceMatchAddress(uint8 source_id, typed_bdaddr *taddr)
{
    le_bm_bass_status_t result = le_bm_bass_status_failed;

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    result = LeBroadcastManager_BassSetSourceMatchAddress(source_id, taddr);

    DEBUG_LOG_ALWAYS("leAudioTest_BassSetSourceMatchAddress: result=enum:le_bm_bass_status_t:%d", result);
#else
    UNUSED(source_id);
    UNUSED(taddr);
    DEBUG_LOG_ALWAYS("leAudioTest_BassSetSourceMatchAddress: feature not enabled");
#endif

    return result;
}

void leAudioTest_GetGlobalLeScanParameters(void)
{
#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    DEBUG_LOG_ALWAYS("leAudioTest_GetGlobalLeScanParameters");

    CmExtScanGetGlobalParamsRequest(&le_audio_test_self_scan_task);
#else
    DEBUG_LOG_ALWAYS("leAudioTest_GetGlobalLeScanParameters: feature not enabled");
#endif
}
#endif /* INCLUDE_LE_AUDIO_BROADCAST */

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST) */

