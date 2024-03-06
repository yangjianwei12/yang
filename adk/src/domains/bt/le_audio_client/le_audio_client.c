/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief    Interfaces for the LE Audio Client
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "le_audio_client_context.h"
#include "qualcomm_connection_manager.h"

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#define LE_AUDIO_CLIENT_DEFAULT_MODE        LE_AUDIO_CLIENT_MODE_UNICAST
#else
#define LE_AUDIO_CLIENT_DEFAULT_MODE        LE_AUDIO_CLIENT_MODE_BROADCAST
#endif

le_audio_client_context_t le_audio_client_context;

static void leAudioClient_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    DEBUG_LOG("leAudioClient_HandleMessage Received Message Id : 0x%x", id);

    if (leAudioClientBroadcast_ProcessMsgIfFromBcastRouter(id, message))
    {
        return;
    }

    leAudioClient_HandleUnicastMessage(id, message);
}

static bool leAudioClient_GetSpeakerPathConfig(gatt_cid_t cid, void *media_config)
{
    bool status = FALSE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    switch (client_ctxt->pts_mode)
    {
        case LE_AUDIO_CLIENT_PTS_MODE_BAP:
        {
            status = BapProfileClient_GetSpeakerPathConfig(cid, (bap_media_config_t*) media_config);
        }
        break;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        case LE_AUDIO_CLIENT_PTS_MODE_TMAP:
        {
            status = TmapClientSource_PtsGetSpeakerPathConfig(cid, (tmap_media_config_t*) media_config);
        }
        break;

        case LE_AUDIO_CLIENT_PTS_MODE_CAP:
        {
            status = CapProfileClient_PtsGetSpeakerPathConfig((cap_profile_client_media_config_t*) media_config);
        }
        break;
#endif

        default:
        break;
    }

    return status;
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
static bool leAudioClient_GetMicPathConfig(gatt_cid_t cid, void *mic_config)
{
    bool status = FALSE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    switch (client_ctxt->pts_mode)
    {
        case LE_AUDIO_CLIENT_PTS_MODE_BAP:
        {
            status = BapProfileClient_GetMicPathConfig(cid, (bap_microphone_config_t*) mic_config);
        }
        break;

        case LE_AUDIO_CLIENT_PTS_MODE_TMAP:
        {
            status = TmapClientSource_PtsGetMicPathConfig(cid, (tmap_microphone_config_t*) mic_config);
        }
        break;

        default:
        break;
    }

    return status;
}
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

void leAudioClient_ResetContext(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    client_ctxt->group_handle = INVALID_GROUP_HANDLE;
    client_ctxt->requested_audio_contexts = CAP_CLIENT_CONTEXT_TYPE_PROHIBITED;
    client_ctxt->configured_audio_contexts = CAP_CLIENT_CONTEXT_TYPE_PROHIBITED;
    client_ctxt->pts_mode = LE_AUDIO_CLIENT_PTS_MODE_OFF;
    LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_INITIALIZED);

    leAudioClient_ResetUnicastContext();
}

bool LeAudioClient_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("LeAudioClient_Init");

    memset(&le_audio_client_context, 0, sizeof(le_audio_client_context));

    leAudioClient_GetTask()->handler = leAudioClient_HandleMessage;
    le_audio_client_context.client_tasks = TaskList_Create();
    le_audio_client_context.group_handle = INVALID_GROUP_HANDLE;

    TmapProfile_Init();
    TmapClientSource_Init(NULL);

    leAudioClient_InitUnicast();
    leAudioClient_AudioConfigInit();
    leAudioClientBroadcast_Init();

    /* If INCLUDE_QCOM_CON_MANAGER is defined then LE Audio client shall receive
       VSDM messages */
    QcomConManagerRegisterClient(leAudioClient_GetTask());

    le_audio_client_context.mode = LE_AUDIO_CLIENT_DEFAULT_MODE;

#ifndef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_INITIALIZED);

    /* Inform application that LE audio client init is completed */
    MessageSend(SystemState_GetTransitionTask(), LE_AUDIO_CLIENT_INIT_CFM, NULL);
#endif

    return TRUE;
}

void LeAudioClient_SetState(le_audio_client_state_t state)
{
    leAudioClient_GetContext()->state = state;

    if (state == LE_AUDIO_CLIENT_STATE_CONNECTED &&
        !LeAudioClient_IsInUnicastMode() &&
         leAudioClient_IsStreamingEnabled())
    {
        /* Add source to assistant if not already added */
        leAudioClientBroadcast_AddSourceToAssistant();
    }
}

bool LeAudioClient_StartStreaming(ServiceHandle group_handle, uint16 audio_context)
{
    bool stream_start_req = FALSE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if(client_ctxt->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_CAP ||
       client_ctxt->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_TMAP)
    {
        return FALSE;
    }

    switch (client_ctxt->mode)
    {
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        case LE_AUDIO_CLIENT_MODE_UNICAST:
            stream_start_req = leAudioClient_StartUnicastStreaming(group_handle, audio_context);
        break;
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
        case LE_AUDIO_CLIENT_MODE_BROADCAST:
            stream_start_req = leAudioClientBroadcast_StartStreaming(audio_context);
        break;
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

        default:
            UNUSED(group_handle);
        break;
    }

    return stream_start_req;
}

bool LeAudioClient_StartStreamingCancelRequest(ServiceHandle group_handle)
{
    bool status = FALSE;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    /* Check the below conditions
       1. Check if there is a pending unicast streaming request for the given cid
       2. We can only cancel the start request if it is in configuring the stage. Verify this by
          checking if there is an audio context pending to configure */
    if (client_ctxt->mode == LE_AUDIO_CLIENT_MODE_UNICAST &&
        !leAudioClient_IsInUnicastStreaming() &&
        client_ctxt->group_handle == group_handle &&
        client_ctxt->session_data.audio_context != CAP_CLIENT_CONTEXT_TYPE_PROHIBITED &&
        client_ctxt->requested_audio_contexts != 0)
    {
        DEBUG_LOG("LeAudioClient_StartStreamingCancelRequest");
        client_ctxt->requested_audio_contexts = 0;
        status = TRUE;
    }
#else
    UNUSED(group_handle);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    return status;
}

bool LeAudioClient_StopStreaming(ServiceHandle group_handle, bool remove_config)
{
    bool stop_streaming = FALSE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if(client_ctxt->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_CAP ||
       client_ctxt->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_TMAP)
    {
        return FALSE;
    }

    switch (client_ctxt->mode)
    {
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        case LE_AUDIO_CLIENT_MODE_UNICAST:
            stop_streaming = leAudioClient_StopUnicastStreaming(group_handle, remove_config);
        break;
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
        case LE_AUDIO_CLIENT_MODE_BROADCAST:
            stop_streaming = leAudioClientBroadcast_StopStreaming(FALSE);
        break;
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

        default:
            UNUSED(group_handle);
            UNUSED(remove_config);
        break;
    }

    return stop_streaming;
}

void LeAudioClient_ClientRegister(Task client_task)
{
    PanicNull((void *) client_task);
    TaskList_AddTask(le_audio_client_context.client_tasks, client_task);
}

void LeAudioClient_ClientUnregister(Task client_task)
{
    PanicNull((void *) client_task);
    TaskList_RemoveTask(le_audio_client_context.client_tasks, client_task);
}

void LeAudioClient_SetDefaultAudioContext(uint16 default_context)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    client_ctxt->requested_audio_contexts = default_context;
}

uint16 LeAudioClient_GetConfiguredCapAudioContext(void)
{
    return leAudioClient_GetContext()->configured_audio_contexts;
}

uint32 LeAudioClient_GetActiveSpkrPathSampleRate(void)
{
    uint32 sample_rate = 0;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    if (leAudioClient_IsInUnicastStreaming())
    {
        sample_rate = leAudioClient_GetSampleRate(client_ctxt->session_data.audio_config->sink_stream_capability);
    }
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    if (LeAudioClient_IsBroadcastSourceStreamingActive())
    {
        sample_rate = leAudioClient_GetSampleRate(client_ctxt->broadcast_session_data.audio_config->sub_group_info->config);
    }
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

    return sample_rate;
}

uint32 LeAudioClient_GetActiveMicPathSampleRate(void)
{
    uint32 sample_rate = 0;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (leAudioClient_IsInUnicastStreaming())
    {
        sample_rate = leAudioClient_GetSampleRate(client_ctxt->session_data.audio_config->source_stream_capability);
    }
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    return sample_rate;
}

bool LeAudioClient_EnablePtsMode(le_audio_client_pts_mode_t mode)
{
    bool status = FALSE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (client_ctxt->state == LE_AUDIO_CLIENT_STATE_INITIALIZED)
    {
        client_ctxt->pts_mode = mode;
        status = TRUE;

        if (mode == LE_AUDIO_CLIENT_PTS_MODE_CAP || mode == LE_AUDIO_CLIENT_PTS_MODE_TMAP)
        {
            LeAudioClient_SetDefaultAudioContext(0);
            TmapClientSource_SetPtsMode(1);
        }
    }

    return status;
}

void LeAudioClient_SetMode(le_audio_client_mode_t mode)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG_INFO("LeAudioClient_SetMode mode = %d", mode);

    client_ctxt->mode = mode;
}

void LeAudioClient_RegisterUsbSourceConnectParamCallback(LeAudioClient_UsbSourceParamCallback usb_src_param_cb)
{
    leAudioClient_GetContext()->usb_src_param_cb = usb_src_param_cb;
}

void LeAudioClient_StartPtsStreamingToDevices(gatt_cid_t cid_1, gatt_cid_t cid_2, bool spkr_path, bool mic_path)
{
    bap_media_config_t media_config;
    KYMERA_INTERNAL_USB_LE_AUDIO_START_T message;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    uint16 frame_duration = 0;

    memset(&message, 0, sizeof(message));

    message.to_air_params.source_iso_handle = INVALID_CIS_HANDLE;
    message.to_air_params.source_iso_handle_right = INVALID_CIS_HANDLE;
    message.from_air_params.source_iso_handle = INVALID_CIS_HANDLE;
    message.from_air_params.source_iso_handle_right = INVALID_CIS_HANDLE;
    message.pts_mode = TRUE;

    /* Ensure this function is called only in PTS mode */
    PanicFalse(client_ctxt->pts_mode != LE_AUDIO_CLIENT_PTS_MODE_OFF);

    if (spkr_path)
    {
        /* Fill in To Air params */
        message.to_air_params.volume = AudioSources_CalculateOutputVolume(audio_source_le_audio_unicast_sender);
        message.to_air_params.codec_type = KYMERA_LE_AUDIO_CODEC_LC3;
        message.to_air_params.presentation_delay = 0;
        message.to_air_params.codec_version = 0;

        if (cid_1 != INVALID_CID &&
            leAudioClient_GetSpeakerPathConfig(cid_1, (void*) &media_config) &&
            media_config.frame_length != 0)
        {
            if (media_config.source_iso_handle != INVALID_CIS_HANDLE)
            {
                message.to_air_params.source_iso_handle = media_config.source_iso_handle;
            }

            if (media_config.source_iso_handle_right != INVALID_CIS_HANDLE)
            {
                message.to_air_params.source_iso_handle_right = media_config.source_iso_handle_right;
            }
        }

        if (cid_2 != INVALID_CID &&
            leAudioClient_GetSpeakerPathConfig(cid_2, (void*) &media_config) &&
            media_config.frame_length != 0)
        {
            if (media_config.source_iso_handle != INVALID_CIS_HANDLE)
            {
                message.to_air_params.source_iso_handle = media_config.source_iso_handle;
            }

            if (media_config.source_iso_handle_right != INVALID_CIS_HANDLE)
            {
                message.to_air_params.source_iso_handle_right = media_config.source_iso_handle_right;
            }
        }

        if (message.to_air_params.source_iso_handle == INVALID_CIS_HANDLE)
        {
            message.to_air_params.source_iso_handle = message.to_air_params.source_iso_handle_right;
            message.to_air_params.source_iso_handle_right = INVALID_CIS_HANDLE;
            message.to_air_params.stream_type = KYMERA_LE_STREAM_MONO;
        }
        else if(message.to_air_params.source_iso_handle == message.to_air_params.source_iso_handle_right)
        {
            message.to_air_params.source_iso_handle_right = INVALID_CIS_HANDLE;
            message.to_air_params.stream_type = KYMERA_LE_STREAM_STEREO_USE_BOTH;
        }
        else if(message.to_air_params.source_iso_handle_right != INVALID_CIS_HANDLE)
        {
            message.to_air_params.stream_type = KYMERA_LE_STREAM_DUAL_MONO;
        }
        else
        {
            message.to_air_params.stream_type = KYMERA_LE_STREAM_MONO;
        }

        PanicFalse(message.to_air_params.source_iso_handle != INVALID_CIS_HANDLE);

        message.to_air_params.sample_rate = media_config.sample_rate;
        message.to_air_params.frame_length = media_config.frame_length;
        frame_duration = message.to_air_params.frame_duration = media_config.frame_duration;
        message.to_air_params.codec_frame_blocks_per_sdu = media_config.codec_frame_blocks_per_sdu;

        DEBUG_LOG_INFO("LeAudioClient_StartPtsStreamingToDevices To Air: sample_rate %d codec_frame_blocks_per_sdu %d, ",
                       message.to_air_params.sample_rate,
                       message.to_air_params.codec_frame_blocks_per_sdu);
        DEBUG_LOG_INFO("LeAudioClient_StartPtsStreamingToDevices To Air: frame_dur %d frame_len %d",
                        message.to_air_params.frame_duration,
                        message.to_air_params.frame_length);

        DEBUG_LOG_INFO("LeAudioClient_StartPtsStreamingToDevices To Air: Left Iso %d Right Iso %d",
                        message.to_air_params.source_iso_handle,
                        message.to_air_params.source_iso_handle_right);
    }

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

    if (mic_path)
    {
        bap_microphone_config_t mic_config;

        message.from_air_params.codec_type = KYMERA_LE_AUDIO_CODEC_LC3;
        message.from_air_params.presentation_delay = 0;
        message.from_air_params.codec_version = 0;

        if (cid_1 != INVALID_CID &&
            leAudioClient_GetMicPathConfig(cid_1, (void*) &mic_config) &&
            mic_config.frame_length != 0)
        {
            if (mic_config.source_iso_handle != INVALID_CIS_HANDLE)
            {
                 message.from_air_params.source_iso_handle = mic_config.source_iso_handle;
            }

            if (mic_config.source_iso_handle_right != INVALID_CIS_HANDLE)
            {
                message.from_air_params.source_iso_handle_right = mic_config.source_iso_handle_right;
            }
        }

        if (cid_2 != INVALID_CID &&
            leAudioClient_GetMicPathConfig(cid_2, (void*) &mic_config) &&
            mic_config.frame_length != 0)
        {
             if (mic_config.source_iso_handle != INVALID_CIS_HANDLE)
             {
                 message.from_air_params.source_iso_handle = mic_config.source_iso_handle;
             }

             if (mic_config.source_iso_handle_right != INVALID_CIS_HANDLE)
             {
                 message.from_air_params.source_iso_handle_right = mic_config.source_iso_handle_right;
             }
        }

        if (message.from_air_params.source_iso_handle == INVALID_CIS_HANDLE)
        {
            message.from_air_params.source_iso_handle = message.from_air_params.source_iso_handle_right;
            message.from_air_params.source_iso_handle_right = INVALID_CIS_HANDLE;
        }
        else if(message.from_air_params.source_iso_handle == message.from_air_params.source_iso_handle_right)
        {
            message.from_air_params.source_iso_handle_right = INVALID_CIS_HANDLE;
        }

        PanicFalse(message.from_air_params.source_iso_handle != INVALID_CIS_HANDLE);

        message.from_air_params.codec_frame_blocks_per_sdu = mic_config.codec_frame_blocks_per_sdu;
        message.from_air_params.frame_duration = mic_config.frame_duration;
        message.from_air_params.frame_length = mic_config.frame_length;
        message.from_air_params.mic_mute_state = FALSE;
        message.from_air_params.sample_rate = mic_config.sample_rate;
        message.vbc_enabled = TRUE;

        if (!spkr_path)
        {
            frame_duration = message.from_air_params.frame_duration;
        }

        DEBUG_LOG_INFO("LeAudioClient_StartPtsStreamingToDevices From Air: sample_rate %d codec_frame_blocks_per_sdu %d, ",
                       message.from_air_params.sample_rate,
                       message.from_air_params.codec_frame_blocks_per_sdu);
        DEBUG_LOG_INFO("LeAudioClient_StartPtsStreamingToDevices From Air: frame_dur %d frame_len %d",
                        message.from_air_params.frame_duration,
                        message.from_air_params.frame_length);

        DEBUG_LOG_INFO("LeAudioClient_StartPtsStreamingToDevices From Air: Left Iso %d Right Iso %d",
                        message.from_air_params.source_iso_handle,
                        message.from_air_params.source_iso_handle_right);
    }

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    client_ctxt->usb_src_param_cb(&message, mic_path, frame_duration);

    KymeraUsbLeAudio_Start(&message);
}

uint8 LeAudioClient_GetQhsLevel(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    return client_ctxt->qhs_level;
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */
