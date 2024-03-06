/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    tmap_profile
    \brief      Implementation of unicast client
*/

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

#include "tmap_client_source_unicast.h"
#include "tmap_client_source_unicast_private.h"
#include "tmap_client_source_private.h"
#include "tmap_profile_mcs_tbs_private.h"
#include "cap_profile_client.h"

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#define INVALID_TMAP_CIG_ID 0xff

#define TMAP_LOG     DEBUG_LOG

/*! Returns the tmap client broadcast context data */
#define tmapClientSourceUnicast_GetContext() (&TmapClientSource_GetContext()->unicast_data)

#define tmapProfileClient_IsCapabilitySupported(capability_data, requested_capability) \
            (((capability_data) & (requested_capability)) == requested_capability)

void TmapClientSourceUnicast_RegisterCallback(tmap_client_source_unicast_callback_handler_t handler)
{
    tmapClientSourceUnicast_GetContext()->callback_handler = handler;
}

/*! \brief Converts and returns the stream capability */
static uint32 tmapProfileClient_GetSamplingRate(CapClientSreamCapability cap)
{
    switch (cap)
    {
        case CAP_CLIENT_STREAM_CAPABILITY_16_1:
        case CAP_CLIENT_STREAM_CAPABILITY_16_2:
        {
            return 16000;
        }

        case CAP_CLIENT_STREAM_CAPABILITY_24_1:
        case CAP_CLIENT_STREAM_CAPABILITY_24_2:
        {
            return 24000;
        }

        case CAP_CLIENT_STREAM_CAPABILITY_32_1:
        case CAP_CLIENT_STREAM_CAPABILITY_32_2:
        {
            return 32000;
        }

        case CAP_CLIENT_STREAM_CAPABILITY_48_1:
        case CAP_CLIENT_STREAM_CAPABILITY_48_2:
        case CAP_CLIENT_STREAM_CAPABILITY_48_3:
        case CAP_CLIENT_STREAM_CAPABILITY_48_4:
        case CAP_CLIENT_STREAM_CAPABILITY_48_5:
        case CAP_CLIENT_STREAM_CAPABILITY_48_6:
        {
            return 48000;
        }

        default:
            return 0;
    }
}

/*! \brief Handle TMAP unicast start streaming indication */
static void tmapClientSource_HandleCisConnectInd(const TmapClientUnicastStartStreamInd *start_stream_ind)
{
    tmap_client_unicast_msg_t msg;
    int dev_count = 0;
    int cis_count = 0;
    tmap_client_source_group_instance_t *group_instance;
    CapClientAudioConfig *config;

    TMAP_LOG("tmapClientSource_HandleCisConnectInd status 0x%x", start_stream_ind->result);

    PanicFalse(TmapClientSource_GetGroupInstance()->cap_group_handle == start_stream_ind->groupId);

    /* Send message TMAP_CLIENT_MSG_ID_UNICAST_CIS_CONNECT to registered client */
    msg.id = TMAP_CLIENT_MSG_ID_UNICAST_CIS_CONNECT;
    msg.body.unicast_cis_connect.cid = start_stream_ind->cid;
    msg.body.unicast_cis_connect.group_handle = TmapClientSource_GetGroupInstance()->cap_group_handle;
    msg.body.unicast_cis_connect.cis_handles = (void *) start_stream_ind->cishandles;
    msg.body.unicast_cis_connect.codec_qos_config = (void *) start_stream_ind->audioConfig;
    msg.body.unicast_cis_connect.cis_count = start_stream_ind->cisCount;
    msg.body.unicast_cis_connect.status = start_stream_ind->result == CAP_CLIENT_RESULT_SUCCESS ?
                                          TMAP_CLIENT_UNICAST_MSG_STATUS_SUCCESS : TMAP_CLIENT_UNICAST_MSG_STATUS_FAILED;
    tmapClientSourceUnicast_GetContext()->callback_handler(&msg);

    if (TmapClientSource_IsInPtsMode())
    {
        group_instance = TmapClientSource_GetGroupInstance();
        config = start_stream_ind->audioConfig;

        for (dev_count = 0; dev_count < MAX_TMAP_DEVICES_SUPPORTED; dev_count++)
        {
            if (group_instance->tmap_client_instance[dev_count].cid == start_stream_ind->cid)
            {
                tmap_media_config_t *spk = &group_instance->tmap_client_instance[dev_count].spkr_audio_path;
                tmap_microphone_config_t *mic = &group_instance->tmap_client_instance[dev_count].mic_audio_path;

                TMAP_LOG("tmapClientSource_HandleCisConnectInd ciscount 0x%x", start_stream_ind->cisCount);

                for (cis_count = 0; cis_count < start_stream_ind->cisCount; cis_count++)
                {
                    CapClientCisHandles  *cis = &start_stream_ind->cishandles[cis_count];

                    if (cis->audioLocation & (CAP_CLIENT_AUDIO_LOCATION_FL | CAP_CLIENT_AUDIO_LOCATION_MONO))
                    {
                        TMAP_LOG("tmapClientSource_HandleCisConnectInd Location Left or Mono ");
                        if (cis->direction & CAP_CLIENT_DATAPATH_INPUT)
                        {
                            TMAP_LOG("tmapClientSource_HandleCisConnectInd Direction Input ");
                            spk->source_iso_handle = cis->cisHandle;
                            spk->codec_frame_blocks_per_sdu = config->sinkLc3BlocksPerSdu;
                            spk->frame_duration = (config->sinkFrameDuaration == 0x2) ? 10000 : 7500;
                            spk->frame_length = config->sinkOctetsPerFrame;
                            spk->presentation_delay = config->sinkPdelay;
                            spk->sample_rate = tmapProfileClient_GetSamplingRate(config->sinkSamplingFrequency);
                        }

                        if (cis->direction & CAP_CLIENT_DATAPATH_OUTPUT)
                        {
                            TMAP_LOG("tmapClientSource_HandleCisConnectInd Direction Output ");
                            mic->source_iso_handle = cis->cisHandle;
                            mic->codec_frame_blocks_per_sdu = config->srcLc3BlocksPerSdu;
                            mic->frame_duration = (config->srcFrameDuaration == 0x2) ? 10000 : 7500;
                            mic->frame_length = config->srcOctetsPerFrame;
                            mic->presentation_delay = config->srcPdelay;
                            mic->sample_rate = tmapProfileClient_GetSamplingRate(config->srcSamplingFrequency);
                        }
                    }

                    if (cis->audioLocation & CAP_CLIENT_AUDIO_LOCATION_FR)
                    {
                        TMAP_LOG("tmapClientSource_HandleCisConnectInd Location Right");

                        if (cis->direction & CAP_CLIENT_DATAPATH_INPUT)
                        {
                            TMAP_LOG("tmapClientSource_HandleCisConnectInd Direction Input ");
                            spk->source_iso_handle_right = cis->cisHandle;
                            spk->codec_frame_blocks_per_sdu = config->sinkLc3BlocksPerSdu;
                            spk->frame_duration = (config->sinkFrameDuaration == 0x2) ? 10000 : 7500;
                            spk->frame_length = config->sinkOctetsPerFrame;
                            spk->presentation_delay = config->sinkPdelay;
                            spk->sample_rate = tmapProfileClient_GetSamplingRate(config->sinkSamplingFrequency);
                        }

                        if (cis->direction & CAP_CLIENT_DATAPATH_OUTPUT)
                        {
                            TMAP_LOG("tmapClientSource_HandleCisConnectInd Direction output ");
                            mic->source_iso_handle_right = cis->cisHandle;
                            mic->codec_frame_blocks_per_sdu = config->srcLc3BlocksPerSdu;
                            mic->frame_duration = (config->srcFrameDuaration == 0x2) ? 10000 : 7500;
                            mic->frame_length = config->srcOctetsPerFrame;
                            mic->presentation_delay = config->srcPdelay;
                            mic->sample_rate = tmapProfileClient_GetSamplingRate(config->srcSamplingFrequency);
                        }
                    }
                }
            }
        }
    }

    if (start_stream_ind->cisCount != 0)
    {
        pfree(start_stream_ind->cishandles);
    }

    if (start_stream_ind->audioConfig != NULL)
    {
        pfree(start_stream_ind->audioConfig);
    }
}

/*! \brief Handle TMAP unicast start streaming confirmation */
static void tmapClientSource_HandleUnicastStartStreamCfm(const TmapClientUnicastStartStreamCfm *start_stream_cfm)
{
    tmap_client_unicast_msg_t msg;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TMAP_LOG("tmapClientSource_HandleUnicastStartStreamCfm status 0x%x", start_stream_cfm->result);

    PanicFalse(group_instance->cap_group_handle == start_stream_cfm->groupId);

    msg.id = TMAP_CLIENT_MSG_ID_UNICAST_STREAM_START;
    msg.body.unicast_stream_start.group_handle = group_instance->cap_group_handle;
    msg.body.unicast_stream_start.status = start_stream_cfm->result == CAP_CLIENT_RESULT_SUCCESS ?
                                           TMAP_CLIENT_UNICAST_MSG_STATUS_SUCCESS : TMAP_CLIENT_UNICAST_MSG_STATUS_FAILED ;
    tmapClientSourceUnicast_GetContext()->callback_handler(&msg);
}

/*! \brief Handle TMAP unicast stop streaming confirmation */
static void tmapClientSource_HandleUnicastStopStreamCfm(const TmapClientUnicastStopStreamCfm *stop_stream_cfm)
{
    tmap_client_unicast_msg_t msg;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TMAP_LOG("tmapClientSource_HandleUnicastStopStreamCfm: 0x%x", stop_stream_cfm->result);

    PanicFalse(group_instance->cap_group_handle == stop_stream_cfm->groupId);

    msg.id = TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP;
    msg.body.unicast_stream_stop.group_handle = group_instance->cap_group_handle;
    msg.body.unicast_stream_stop.status = stop_stream_cfm->result == CAP_CLIENT_RESULT_SUCCESS ?
                                          TMAP_CLIENT_UNICAST_MSG_STATUS_SUCCESS : TMAP_CLIENT_UNICAST_MSG_STATUS_FAILED ;
    tmapClientSourceUnicast_GetContext()->callback_handler(&msg);

    if (stop_stream_cfm->deviceStatusLen != 0)
    {
        pfree(stop_stream_cfm->deviceStatus);
    }
}

/*! \brief Handle TMAP unicast disconnect confirmation */
static void tmapClientSource_HandleUnicastDisconnectCfm(const TmapClientUnicastDisconnectCfm *disconnect_cfm)
{
    tmap_client_unicast_msg_t msg;
    tmap_src_unicast_task_data_t *unicast_data = tmapClientSourceUnicast_GetContext();

    TMAP_LOG("tmapClientSource_HandleUnicastDisconnectCfm: 0x%x", disconnect_cfm->result);

    PanicFalse(TmapClientSource_GetGroupInstance()->cap_group_handle == disconnect_cfm->groupId);

    msg.id = TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_REMOVED;
    msg.body.unicast_config_removed.group_handle = TmapClientSource_GetGroupInstance()->cap_group_handle;
    msg.body.unicast_config_removed.status = disconnect_cfm->result == CAP_CLIENT_RESULT_SUCCESS ?
                                             TMAP_CLIENT_UNICAST_MSG_STATUS_SUCCESS : TMAP_CLIENT_UNICAST_MSG_STATUS_FAILED ;

    unicast_data->cig_id = INVALID_TMAP_CIG_ID;

    tmapClientSourceUnicast_GetContext()->callback_handler(&msg);
}

/*! \brief Handle TMAP unicast connect confirmation message */
static void tmapClientSource_HandleTmapUnicastConnectConfirmation(const TmapClientUnicastConnectCfm* message)
{
    tmap_client_unicast_msg_t msg;
    tmap_src_unicast_task_data_t *unicast_data = tmapClientSourceUnicast_GetContext();

    TMAP_LOG("tmapClientSource_HandleTmapUnicastConnectConfirmation group_id 0x%x status %d context:%d",
              message->groupId, message->result, message->context);

    PanicFalse(TmapClientSource_GetGroupInstance()->cap_group_handle == message->groupId);

    unicast_data->cig_id = message->cigId;

    /* Send message TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE to registered client */
    msg.id = TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE;
    msg.body.unicast_config_complete.group_handle = TmapClientSource_GetGroupInstance()->cap_group_handle;
    msg.body.unicast_config_complete.status = message->result == CAP_CLIENT_RESULT_SUCCESS ?
                                              TMAP_CLIENT_UNICAST_MSG_STATUS_SUCCESS : TMAP_CLIENT_UNICAST_MSG_STATUS_FAILED ;
    msg.body.unicast_config_complete.audio_context = message->context;
    tmapClientSourceUnicast_GetContext()->callback_handler(&msg);

    if (message->deviceStatusLen != 0)
    {
        pfree(message->deviceStatus);
    }
}

/*! \brief Handler that process the unicast related messages alone */
void tmapClientSource_HandleTmapUnicastMessage(Message message)
{
    CsrBtCmPrim tmap_id = *(CsrBtCmPrim *)message;

    switch (tmap_id)
    {
        case TMAP_CLIENT_UNICAST_CONNECT_CFM:
            tmapClientSource_HandleTmapUnicastConnectConfirmation((const TmapClientUnicastConnectCfm*)message);
        break;

        case TMAP_CLIENT_UNICAST_START_STREAM_IND:
            tmapClientSource_HandleCisConnectInd((const TmapClientUnicastStartStreamInd*)message);
        break;

        case TMAP_CLIENT_UNICAST_START_STREAM_CFM:
            tmapClientSource_HandleUnicastStartStreamCfm((const TmapClientUnicastStartStreamCfm*) message);
        break;

        case TMAP_CLIENT_UNICAST_STOP_STREAM_CFM:
            tmapClientSource_HandleUnicastStopStreamCfm((const TmapClientUnicastStopStreamCfm *) message);
        break;

        case TMAP_CLIENT_UNICAST_DISCONNECT_CFM:
            tmapClientSource_HandleUnicastDisconnectCfm((const TmapClientUnicastDisconnectCfm *) message);
        break;

        default:
            TMAP_LOG("tmapClientSource_HandleTmapUnicastMessage Unhandled message id: 0x%x", tmap_id);
        break;
    }
}

bool TmapClientSource_Configure(ServiceHandle group_handle,
                                uint32 sink_capability,
                                uint32 source_capability,
                                uint8 mic_count,
                                TmapClientCigConfigMode tmap_cig_config_mode,
                                TmapClientQhsConfig *cig_qhs_config)
{
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TMAP_LOG("TmapClientSource_Configure: group_handle=0x%04X, mic_count:%d sink_freq:%x source_freq:%x",
              group_handle, mic_count, sink_capability, source_capability);

    PanicFalse(group_instance->cap_group_handle == group_handle);

#ifdef INCLUDE_LE_APTX_ADAPTIVE
    if (tmapProfileClient_IsCapabilitySupported(sink_capability, CAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_48_1) && (mic_count == 0))
    {
        DEBUG_LOG("Aptx Adaptive Configured for Media");
    }
#endif

    TmapClientUnicastConnectReq(group_instance->tmap_profile_handle,
                                group_handle,
                                sink_capability,
                                source_capability,
                                mic_count != 0 ? TMAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL : TMAP_CLIENT_CONTEXT_TYPE_MEDIA,
                                CAP_CLIENT_AUDIO_LOCATION_FL | CAP_CLIENT_AUDIO_LOCATION_FR,
                                CapProfileClient_GetSourceAudioLocation(group_handle),
                                mic_count,
                                tmap_cig_config_mode,
                                cig_qhs_config);

    return TRUE;
}

bool TmapClientSource_StartUnicastStreaming(ServiceHandle group_handle, TmapClientContext audio_context)
{
    bool status = FALSE;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TMAP_LOG("TmapClientSource_StartUnicastStreaming: group_handle=0x%04X context %d", group_handle, audio_context);

    if (audio_context != TMAP_CLIENT_CONTEXT_TYPE_MEDIA &&
        audio_context != TMAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL)
    {
        /* Audio context not supported */
        return FALSE;
    }

    if (group_instance->cap_group_handle == group_handle)
    {
        group_instance->audio_context = audio_context;

        TmapClientUnicastStartStreamReq(group_instance->tmap_profile_handle,
                                        group_handle,
                                        audio_context,
                                        audio_context == TMAP_CLIENT_CONTEXT_TYPE_MEDIA ?
                                        TMAP_PROFILE_MCS_CCID : TMAP_PROFILE_TBS_CCID,
                                        0, NULL);
        status = TRUE;
    }

    return status;
}

bool TmapClientSource_StopUnicastStreaming(ServiceHandle group_handle, bool remove_configured_context)
{
    bool status = FALSE;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TMAP_LOG("TmapClientSource_StopUnicastStreaming: group_handle=0x%04X", group_handle);

    if (group_instance->cap_group_handle == group_handle)
    {
        TmapClientUnicastStopStreamReq(group_instance->tmap_profile_handle,
                                       group_handle,
                                       remove_configured_context);
        status = TRUE;
    }

    return status;
}

bool TmapClientSource_RemoveConfiguration(ServiceHandle group_handle, TmapClientContext use_case)
{
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    DEBUG_LOG("TmapClientSource_RemoveConfiguration: group_handle=0x%04X, use_case: %x", group_handle, use_case);

    PanicFalse(group_instance->cap_group_handle == group_handle);

    TmapClientUnicastDisconnectReq(group_instance->tmap_profile_handle,
                                   group_handle,
                                   use_case);

    return TRUE;
}

bool TmapClientSource_PtsGetSpeakerPathConfig(gatt_cid_t cid, tmap_media_config_t *media_config)
{
    bool status = FALSE;
    tmap_client_source_instance_t *instance = TmapClientSource_GetInstance(tmap_client_source_compare_by_cid,
                                                                           (unsigned) cid);

    if (instance != NULL &&
        (instance->spkr_audio_path.source_iso_handle != 0xFFFF ||
         instance->spkr_audio_path.source_iso_handle_right != 0xFFFF))
    {
        memcpy(media_config, &instance->spkr_audio_path, sizeof(tmap_media_config_t));
        status = TRUE;
    }

    return status;
}

bool TmapClientSource_PtsGetMicPathConfig(gatt_cid_t cid, tmap_microphone_config_t *mic_config)
{
    bool status = FALSE;
    tmap_client_source_instance_t *instance = TmapClientSource_GetInstance(tmap_client_source_compare_by_cid,
                                                                           (unsigned) cid);
    PanicFalse(mic_config != NULL);

    if (instance != NULL &&
        (instance->mic_audio_path.source_iso_handle != 0xFFFF ||
         instance->mic_audio_path.source_iso_handle_right != 0xFFFF))
    {
        memcpy(mic_config, &instance->mic_audio_path, sizeof(tmap_microphone_config_t));
        status = TRUE;
    }

    return status;
}

gatt_cid_t TmapClientSource_PtsGetFirstGattDeviceCid(void)
{
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();
    return group_instance->tmap_client_instance[0].cid;
}

gatt_cid_t TmapClientSource_PtsGetSecondGattDeviceCid(void)
{
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();
    return group_instance->tmap_client_instance[1].cid;
}

bool TmapClientSource_PtsIsSpkrPresent(void)
{
    int dev_count;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    for (dev_count = 0; dev_count < MAX_TMAP_DEVICES_SUPPORTED; dev_count++)
    {
        tmap_media_config_t *spk = &group_instance->tmap_client_instance[dev_count].spkr_audio_path;

        if (spk->source_iso_handle != 0xFFFF || spk->source_iso_handle_right != 0xFFFF)
        {
            return TRUE;
        }
    }

    return FALSE;
}

bool TmapClientSource_PtsIsMicPresent(void)
{
    int dev_count;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    for (dev_count = 0; dev_count < MAX_TMAP_DEVICES_SUPPORTED; dev_count++)
    {
        tmap_microphone_config_t *mic = &group_instance->tmap_client_instance[dev_count].mic_audio_path;

        if (mic->source_iso_handle != 0xFFFF || mic->source_iso_handle_right != 0xFFFF)
        {
            return TRUE;
        }
    }

    return FALSE;
}

void TmapClientSource_PtsConfigureForStreaming(uint32 sink_capability,
                                                uint32 source_capability,
                                                uint16 use_case,
                                                uint8 mic_count,
                                                TmapClientCigConfigMode tmap_config_mode,
                                                uint32 sink_audio_location,
                                                uint32 src_audio_location)
{
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TmapClientUnicastConnectReq(group_instance->tmap_profile_handle,
                                group_instance->cap_group_handle,
                                sink_capability,
                                source_capability,
                                use_case,
                                sink_audio_location,
                                src_audio_location,
                                mic_count,
                                tmap_config_mode,
                                NULL);
}

void TmapClientSource_PtsStartUnicastStreaming(TmapClientContext audio_context, int ccid_count, int ccid_type)
{
    UNUSED(ccid_count);
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TmapClientUnicastStartStreamReq(group_instance->tmap_profile_handle,
                                    group_instance->cap_group_handle,
                                    audio_context,
                                    ccid_type,
                                    0,
                                    NULL);
}

void TmapClientSource_PtsStopUnicastStreaming(bool remove_configured_context)
{
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TmapClientUnicastStopStreamReq(group_instance->tmap_profile_handle,
                                   group_instance->cap_group_handle,
                                   remove_configured_context);
}

uint8 TmapProfileClient_GetCigId(ServiceHandle group_handle)
{
    tmap_src_unicast_task_data_t *unicast_data = tmapClientSourceUnicast_GetContext();

    UNUSED(group_handle);

    return unicast_data->cig_id;
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
