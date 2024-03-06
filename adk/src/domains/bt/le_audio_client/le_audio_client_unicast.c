/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of le audio unicast client functionality.
*/

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

#include "le_audio_client_context.h"
#include "le_audio_client_unicast_music_source.h"
#include "le_audio_client_unicast_voice_source.h"

#include "tmap_client_source_unicast.h"
#include "tmap_server_role.h"
#include "volume_messages.h"
#include "gatt_connect.h"
#include "csip_client.h"
#include "tmap_profile.h"
#include "tmap_server_role.h"

#include "kymera.h"
#include "feature.h"
#include "device_properties.h"
#include "qualcomm_connection_manager.h"


#ifdef GC_SECTIONS
                                                      /* Move all functions in KEEP_PM section to ensure they are not removed during
                                                       * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#define LC3_EPC_FEATURE_ID                                      (LC3_EPC_HEADSET)

#define LE_AUDIO_CLIENT_CLIENT_MICROPHONE_COUNT                 (CAP_PROFILE_CLIENT_MICROPHONE_COUNT_ONE)

#define LE_AUDIO_CLIENT_DEFAULT_CONFIG_MODE                      (CAP_CLIENT_CIG_CONFIG_MODE_DEFAULT | CAP_CLIENT_MODE_JOINT_STEREO)
#define LE_AUDIO_CLIENT_CONFIG_MODE_QHS                          (LE_AUDIO_CLIENT_DEFAULT_CONFIG_MODE | CAP_CLIENT_CIG_CONFIG_MODE_QHS)

#define LeAudioClient_IsContextOfTypeMedia(context) (context == CAP_CLIENT_CONTEXT_TYPE_MEDIA)

#define LeAudioClient_IsContextOfTypeGaming(context) (context == CAP_CLIENT_CONTEXT_TYPE_GAME || \
                                                      context == CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC)

#define LeAudioClient_IsContextOfTypeGamingWithoutVBC(context) (context == CAP_CLIENT_CONTEXT_TYPE_GAME)

#define LeAudioClient_SnkAptxLite48kSupported(group_handle) \
    (bool) CapProfileClient_IsStreamCapabilitySupported(group_handle, CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_48_1)

#define LeAudioClient_SrcAptxLite16kSupported(group_handle) \
    (bool) CapProfileClient_IsStreamCapabilitySupported(group_handle, CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_16_1)

#define LeAudioClient_SnkAptxAdaptivesSupported(group_handle) \
    (bool) CapProfileClient_IsStreamCapabilitySupported(group_handle, CAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_48_1)

static bool leAudioClient_ConfigureUnicastStreaming(ServiceHandle group_handle, CapClientContext audio_context);

#ifdef INCLUDE_LE_APTX_ADAPTIVE
static bool leAudioClient_CanAptxAdaptiveCodecBeUsed(uint16 audio_context, ServiceHandle group_handle)
{
    bool aptx_supported = LeAudioClient_SnkAptxAdaptivesSupported(group_handle);

    DEBUG_LOG("leAudioClient_CanAptxAdaptiveCodecBeUsed, audio_context 0x%x, aptx_supported %d", aptx_supported);

    return (LeAudioClient_IsContextOfTypeMedia(audio_context) && LeAudioClient_SnkAptxAdaptivesSupported(group_handle));
}

static void leAudioClient_SetQhsMapReq(uint8 cig_id, uint16 audio_context)
{
    QCOM_CON_MANAGER_SET_CIG_QHS_MAP_REQ_T qhs_map_req;

    UNUSED(audio_context);

    DEBUG_LOG("leAudioClient_SetQhsMapReq audio_context:0x%x", audio_context);

    qhs_map_req.cig_id = cig_id;
    qhs_map_req.flags = QCOM_CON_MANAGER_FIXED_LENGTH_PDU;
    qhs_map_req.cis_count = QCOM_MAX_SUPPORTED_CIS;
    qhs_map_req.cis_ids[0] = 0x20; /* TBD: Limitation in profiles will be fixed later, hence using hardcoded values for CIS IDs */
    qhs_map_req.cis_ids[1] = 0x21;
    qhs_map_req.cis_qhs_map = leAudioClient_GetAptXAdaptiveCisQhsMapConfig();

    QcomConManagerSetQHSMapReq(&qhs_map_req);
}
#else
#define leAudioClient_CanAptxAdaptiveCodecBeUsed(audio_context, group_handle) (UNUSED(audio_context), UNUSED(group_handle), FALSE)
#define leAudioClient_SetQhsMapReq(cig_id, audio_context) (UNUSED(cig_id), UNUSED(audio_context))
#endif  /* INCLUDE_LE_APTX_ADAPTIVE */

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
static bool leAudioClient_CanAptxLiteCodecBeUsed(uint16 audio_context, ServiceHandle group_handle)
{
    return (LeAudioClient_IsContextOfTypeGaming(audio_context) && LeAudioClient_SnkAptxLite48kSupported(group_handle) &&
           (LeAudioClient_IsContextOfTypeGamingWithoutVBC(audio_context) || LeAudioClient_SrcAptxLite16kSupported(group_handle)));
}

static void leAudioClient_UpdateFlushTimeoutRange(ServiceHandle group_handle, cap_profile_ft_info_t * cap_ft_info)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClient_UpdateFlushTimeoutRange");

    if (leAudioClient_CanAptxLiteCodecBeUsed(client_ctxt->configured_audio_contexts, group_handle))
    {
        QCOM_CON_MANAGER_SET_FLUSH_TIMEOUT_RANGE_PARAM_T ft_info;

        ft_info.cig_id = CapProfileClient_GetCigId(group_handle);
        ft_info.enable = QCOM_CON_MANAGER_SET_FLUSH_TIMEOUT_ENABLE;
        ft_info.ll_mode = cap_ft_info->latency_mode;
        ft_info.min_ft_c_to_p = cap_ft_info->min_flush_timeout;
        ft_info.max_ft_c_to_p = cap_ft_info->max_flush_timeout;
        ft_info.min_ft_p_to_c = LE_AUDIO_FLUSH_TIMEOUT_MIN_P_TO_C_DEFAULT;
        ft_info.max_ft_p_to_c = LE_AUDIO_FLUSH_TIMEOUT_MAX_P_TO_C_DEFAULT;
        ft_info.ttp_adjust_rate = LE_AUDIO_FLUSH_TIMEOUT_TTP_ADJUST_RATE_DEFAULT;
        
        QcomConManagerSetFlushTimeoutRange(&ft_info);
    }
}

static void leAudioClient_SetDefaultFlushTimeoutRange(ServiceHandle group_handle, uint16 audio_context)
{
    DEBUG_LOG("leAudioClient_SetDefaultFlushTimeoutRange audio_context:0x%x", audio_context);

    if (leAudioClient_CanAptxLiteCodecBeUsed(audio_context, group_handle))
    {
        cap_profile_ft_info_t ft_info;

        ft_info.min_flush_timeout = LE_AUDIO_FLUSH_TIMEOUT_MIN_C_TO_P_DEFAULT;
        ft_info.max_flush_timeout = LE_AUDIO_FLUSH_TIMEOUT_MAX_C_TO_P_DEFAULT;
        ft_info.max_bit_rate = 0;
        ft_info.latency_mode = QCOM_CON_SET_FLUSH_LL_MODE_NO_CHANGE;
        ft_info.err_resilience = 0;

        leAudioClient_UpdateFlushTimeoutRange(group_handle, &ft_info);
    }
}

#else
#define leAudioClient_CanAptxLiteCodecBeUsed(audio_context, group_handle) (UNUSED(audio_context), UNUSED(group_handle), FALSE)
#define leAudioClient_UpdateFlushTimeoutRange(group_handle, cap_ft_info) (UNUSED(group_handle), UNUSED(cap_ft_info))
#define leAudioClient_SetDefaultFlushTimeoutRange(group_handle, audio_context) (UNUSED(group_handle), UNUSED(audio_context))
#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */

/* A unicast audio session is starting. Preserve the audio parameters used for the session */
static void leAudioClient_StartUnicastAudioSession(ServiceHandle group_handle, CapClientContext audio_context)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    uint8 codec = KYMERA_LE_AUDIO_CODEC_LC3;

    DEBUG_LOG("leAudioClient_StartUnicastAudioSession");

    if (leAudioClient_CanAptxLiteCodecBeUsed(audio_context, group_handle))
    {
        codec = KYMERA_LE_AUDIO_CODEC_APTX_LITE;
    }

    if (leAudioClient_CanAptxAdaptiveCodecBeUsed(audio_context, group_handle))
    {
        codec = KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE;
    }

    client_ctxt->session_data.audio_context = audio_context;
    client_ctxt->session_data.audio_config = leAudioClient_GetAudioConfig(audio_context, codec);
    PanicNull((void *) client_ctxt->session_data.audio_config);
}

/* A unicast audio session is ending. Clean the audio parameters used for the session */
static void leAudioClient_EndUnicastAudioSession(le_audio_unicast_session_data_t *session_data)
{
    uint8 cid_index;

    DEBUG_LOG("leAudioClient_EndUnicastAudioSession");
    memset(session_data, 0, sizeof(le_audio_unicast_session_data_t));

    for (cid_index = 0; cid_index < LE_AUDIO_CLIENT_MAX_DEVICES_SUPPORTED ; cid_index++)
    {
        session_data->devices_cis_info[cid_index].cid = INVALID_CID;
    }
}

/*! From the requested contexts bitmasks, find the next audio context to configure. This is done by taking the
    rightmost bit (audio context) set in the requested audio contexts.
*/
static CapClientContext leAudioClient_GetNextAudioContextToConfigure(CapClientContext requested_audio_contexts)
{
    CapClientContext next_audio_context = 1;

    if (requested_audio_contexts == 0)
    {
        DEBUG_LOG("leAudioClient_GetNextAudioContextToConfigure no pending context to configure");

        /* There is no bits set in given bitmask, so no context to configure */
        return CAP_CLIENT_CONTEXT_TYPE_PROHIBITED;
    }

    /* Get the rightmost bit set in the requested_audio_contexts */
    while ((requested_audio_contexts & next_audio_context) == 0)
    {
        next_audio_context = next_audio_context << 1;
    }

    DEBUG_LOG("leAudioClient_GetNextAudioContextToConfigure requested_audio_contexts: 0x%x audio_context_to_configure: 0x%x",
               requested_audio_contexts, next_audio_context);

    return next_audio_context;
}

static le_audio_client_profiles_t leAudioClient_GetProfileForAudioContext(CapClientContext audio_context)
{
    le_audio_client_profiles_t profile_to_use = LE_AUDIO_CLIENT_PROFILE_NONE;

    switch (audio_context)
    {
       case CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC:
       case CAP_CLIENT_CONTEXT_TYPE_GAME:
           profile_to_use = LE_AUDIO_CLIENT_PROFILE_CAP;
       break;

       case CAP_CLIENT_CONTEXT_TYPE_MEDIA:
       case CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL:
           profile_to_use = LE_AUDIO_CLIENT_PROFILE_TMAP ;
       break;

       default:
       break;
    }

    /* Panic if no matching profiles found for given context */
    PanicFalse(profile_to_use != LE_AUDIO_CLIENT_PROFILE_NONE);

    DEBUG_LOG("leAudioClient_GetProfileForAudioContext enum:le_audio_client_profiles_t:%d", profile_to_use);

    return profile_to_use;
}

/*! Check if compatible profile for the given context is connected or not */
static bool leAudioClient_IsProfileConnectedForAudioContext(CapClientContext audio_context)
{
    le_audio_client_profiles_t profile_to_use;

    profile_to_use = leAudioClient_GetProfileForAudioContext(audio_context);

    return profile_to_use == LE_AUDIO_CLIENT_PROFILE_TMAP ? TmapClientSource_IsTmapConnected() :
                                                            CapProfileClient_IsCapConnected();

}

static bool leAudioClient_ConfigureNextAudioContext(ServiceHandle group_handle)
{
    CapClientContext context_to_configure;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    bool status = FALSE;
    uint8 config_index = 0;

    for (config_index = 0; config_index < LE_AUDIO_CLIENT_MAX_AUDIO_CONFIGS_SUPPORTED ; config_index++)
    {
        context_to_configure = leAudioClient_GetNextAudioContextToConfigure(client_ctxt->requested_audio_contexts);

        if (context_to_configure == CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)
        {
            /* No more audio context to configure */
            break;
        }

        if (leAudioClient_IsProfileConnectedForAudioContext(context_to_configure))
        {
            /* Do a Codec & QoS config for the requested audio context */
            leAudioClient_ConfigureUnicastStreaming(group_handle, context_to_configure);
            status = TRUE;
            break;
        }
        else
        {
            DEBUG_LOG("leAudioClient_ConfigureNextAudioContext profile not connected for context 0x%x",
                       context_to_configure);

            /* Clear the ccontext from the requested list as not able to configure */
            client_ctxt->requested_audio_contexts &= (~context_to_configure);
        }
    }

    return status;
}

static CapClientSreamCapability leAudioClient_GetStreamCapability(uint32 stream_capability)
{
    if (stream_capability != CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)
    {
        /* Enable LC3 EPC when license is available and on valid stream */
        stream_capability |= FeatureVerifyLicense(LC3_EPC_FEATURE_ID) ? CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC : 0;
    }

    DEBUG_LOG("leAudioClient_GetStreamCapability stream_capability 0x%x", stream_capability);

    return stream_capability;
}

static bool leAudioClient_ConfigureUnicastStreaming(ServiceHandle group_handle, CapClientContext audio_context)
{
    bool config_req_sent = FALSE;
    le_audio_client_profiles_t profile;
    const le_audio_client_audio_config_t *audio_config;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    CapClientQhsConfig qhs_config;
    uint32 sink_stream_capability, src_stream_capability;

    if (client_ctxt->pts_mode != LE_AUDIO_CLIENT_PTS_MODE_OFF)
    {
        /* In PTS mode we should only allow configuration using PTS test APIs */
        return FALSE;
    }

    DEBUG_LOG("leAudioClient_ConfigureUnicastStreaming audio_context:0x%x, QHS_Enabled:%d",
               audio_context,
               client_ctxt->iso_qhs_supported);

    if(client_ctxt->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_CAP ||
       client_ctxt->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_TMAP)
    {
        return FALSE;
    }

    profile = leAudioClient_GetProfileForAudioContext(audio_context);

    if (leAudioClient_CanAptxLiteCodecBeUsed(audio_context, group_handle))
    {
        audio_config = leAudioClient_GetAudioConfig(audio_context, KYMERA_LE_AUDIO_CODEC_APTX_LITE);
        sink_stream_capability = audio_config->sink_stream_capability;
        src_stream_capability = audio_config->source_stream_capability;
    }
    else if (leAudioClient_CanAptxAdaptiveCodecBeUsed(audio_context, group_handle))
    {
        audio_config = leAudioClient_GetAudioConfig(audio_context, KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE);
        sink_stream_capability = audio_config->sink_stream_capability;
        src_stream_capability = audio_config->source_stream_capability;
    }
    else
    {
        audio_config = leAudioClient_GetAudioConfig(audio_context, KYMERA_LE_AUDIO_CODEC_LC3);
        sink_stream_capability = leAudioClient_GetStreamCapability(audio_config->sink_stream_capability);
        src_stream_capability = leAudioClient_GetStreamCapability(audio_config->source_stream_capability);
    }

    PanicNull((void *) audio_config);

    if (client_ctxt->iso_qhs_supported)
    {
        qhs_config.framing = audio_config->framing;
        qhs_config.phyCtoP = audio_config->phy_ctop;
        qhs_config.phyPtoC = audio_config->phy_ptoc;
        qhs_config.rtnCtoP = audio_config->rtn_ctop;
        qhs_config.rtnPtoC = audio_config->rtn_ptoc;
    }

    if (profile == LE_AUDIO_CLIENT_PROFILE_CAP)
    {
        config_req_sent = CapProfileClient_ConfigureForGaming(group_handle,
                                                     sink_stream_capability,
                                                     src_stream_capability,
                                                     audio_config->target_latency,
                                                     leAudioClient_IsMicrophoneNeededForContext(audio_context) ?
                                                     LE_AUDIO_CLIENT_CLIENT_MICROPHONE_COUNT : CAP_PROFILE_CLIENT_MICROPHONE_COUNT_NONE,
                                                     client_ctxt->iso_qhs_supported ? LE_AUDIO_CLIENT_CONFIG_MODE_QHS : LE_AUDIO_CLIENT_DEFAULT_CONFIG_MODE,
                                                     client_ctxt->iso_qhs_supported ? &qhs_config : NULL);

    }
    else if (profile == LE_AUDIO_CLIENT_PROFILE_TMAP)
    {
        config_req_sent = TmapClientSource_Configure(group_handle,
                                               sink_stream_capability,
                                               src_stream_capability,
                                               leAudioClient_IsMicrophoneNeededForContext(audio_context) ?
                                               LE_AUDIO_CLIENT_CLIENT_MICROPHONE_COUNT : CAP_PROFILE_CLIENT_MICROPHONE_COUNT_NONE,
                                               client_ctxt->iso_qhs_supported ? LE_AUDIO_CLIENT_CONFIG_MODE_QHS : LE_AUDIO_CLIENT_DEFAULT_CONFIG_MODE,
                                               client_ctxt->iso_qhs_supported ? &qhs_config : NULL);
    }

    if (config_req_sent)
    {
        client_ctxt->requested_audio_contexts |= audio_context;
    }

    return config_req_sent;
}

/*! Set the media or call state based on current audio context */
static void leAudioClient_SetMediaOrCallState(bool is_active)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    switch (client_ctxt->session_data.audio_context)
    {
        case CAP_CLIENT_CONTEXT_TYPE_MEDIA:
        case CAP_CLIENT_CONTEXT_TYPE_GAME:
            LeTmapServer_SetMediaState(is_active ? MCS_MEDIA_STATE_PLAYING : MCS_MEDIA_STATE_INACTIVE);
            break;

        case CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL:
            if (is_active)
            {
                LeTmapServer_SetCallState(TBS_CALL_STATE_ACTIVE);
            }
            break;

        default:
            /* Neither call nor media */
            break;
    }
}

/* Process the cancel start stream request */
static void leAudioClient_HandleCancelStartStreamRequest(ServiceHandle group_handle, uint16 audio_context_configured, bool config_success)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (config_success)
    {
        /* Remove the configured audio contexts before sending cancelled start stream indication to clients. */
        if (leAudioClient_GetProfileForAudioContext(audio_context_configured) == LE_AUDIO_CLIENT_PROFILE_CAP)
        {
            CapProfileClient_RemoveGamingConfiguration(group_handle, audio_context_configured);
        }
        else
        {
            TmapClientSource_RemoveConfiguration(group_handle, audio_context_configured);
        }
    }
    else
    {
        /* No configured audio context. Send cancelled start stream indcation to all registered clients */
        leAudioClientMessages_SendStreamStartCancelCompleteInd(group_handle, TRUE);
        leAudioClient_EndUnicastAudioSession(&client_ctxt->session_data);
    }
}

/* Codec & QoS configuration completed.Start the unicast streaming */
static void leAudioClient_StartStreamingIfConfigCompleted(ServiceHandle group_handle, uint16 audio_context_configured, bool config_success)
{
    CapClientContext requested_audio_context;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClient_StartStreamingIfConfigCompleted audio_context:0x%x success:%d", audio_context_configured, config_success);

    if (client_ctxt->requested_audio_contexts == 0 ||
        !config_success)
    {
        /* Either requested contexts have been cleared (ie, cancel start stream request have received) or
           the configuration failed. Send a Stream start failure indication to registered clients */
        leAudioClientMessages_SendUnicastStreamStartInd(group_handle, FALSE, audio_context_configured);

        if (client_ctxt->requested_audio_contexts == 0)
        {
            /* Handles the received cancel start stream request. */
            leAudioClient_HandleCancelStartStreamRequest(group_handle, audio_context_configured, config_success);
            return;
        }

        /* Reset the session data */
        leAudioClient_EndUnicastAudioSession(&client_ctxt->session_data);
    }
    else
    {
        /* The requested audio context have configured successfully and there is no cancel start stream command
           received in between. Proceed to start streaming after updating the configured audio contexts */
        client_ctxt->configured_audio_contexts |= audio_context_configured;

         /* Start the unicast streaming for the audio context */
        if (leAudioClient_GetProfileForAudioContext(audio_context_configured) == LE_AUDIO_CLIENT_PROFILE_CAP)
        {
            leAudioClient_SetDefaultFlushTimeoutRange(group_handle, audio_context_configured);
            CapProfileClient_StartUnicastStreaming(group_handle, audio_context_configured);
        }
        else
        {
            leAudioClient_SetQhsMapReq(TmapProfileClient_GetCigId(group_handle), audio_context_configured);
            TmapClientSource_StartUnicastStreaming(group_handle, audio_context_configured);
        }
    }

    /* Clear the audio context from requested contexts list */
    requested_audio_context = leAudioClient_GetNextAudioContextToConfigure(client_ctxt->requested_audio_contexts);
    client_ctxt->requested_audio_contexts &= (~requested_audio_context);
}

#ifdef INCLUDE_QCOM_CON_MANAGER
static void leAudioClient_EnableWBM(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (FeatureVerifyLicense(LC3_EPC_FEATURE_ID) && client_ctxt->session_data.codec_qos_config.codecVersionNum == 1)
    {
        le_audio_client_cis_devices_info_t *dev, *end_dev;
        le_audio_client_cis_info_t *cis, *end_cis;

        for (dev = &client_ctxt->session_data.devices_cis_info[0],
                end_dev = &dev[LE_AUDIO_CLIENT_MAX_DEVICES_SUPPORTED - 1]; dev <= end_dev; dev++)
        {
            if (dev->cid != INVALID_CID)
            {
                for (cis = &dev->cis_info[0], end_cis = &dev->cis_info[dev->cis_count]; cis < end_cis; cis++)
                {
                    DEBUG_LOG("leAudioClient_EnableWBM Enabling WBM for CIS handle 0x%x", cis->cis_handle);
                    QcomConManagerSetWBMFeature(cis->cis_handle, TRUE);
                }
            }
        }
    }
}
#endif

static bool leAudioClient_IsCisInfoAvailableForSession(void)
{
    bool status = FALSE;
    le_audio_client_cis_devices_info_t *dev, *end_dev;

    for (dev = &leAudioClient_GetContext()->session_data.devices_cis_info[0],
            end_dev = &dev[LE_AUDIO_CLIENT_MAX_DEVICES_SUPPORTED - 1]; dev <= end_dev; dev++)
    {
        if (dev->cid != INVALID_CID)
        {
            if (dev->cis_count == 0)
            {
                status = FALSE;
                break;
            }

            status = TRUE;
        }
    }

    DEBUG_LOG("leAudioClient_IsCisInfoAvailableForSession %d", status);

    return status;
}

/* If Unicast audio streaming started, inform the registered clients */
static void leAudioClient_HandleUnicastStreamStartInd(bool stream_started)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClient_HandleUnicastStreamStartInd Streaming Start success: %d", stream_started);

#ifdef INCLUDE_QCOM_CON_MANAGER
    leAudioClient_EnableWBM();
#endif

    leAudioClientMessages_SendUnicastStreamStartInd(client_ctxt->group_handle,
                                                    stream_started,
                                                    client_ctxt->session_data.audio_context);

    if (stream_started && leAudioClient_IsCisInfoAvailableForSession())
    {
        leAudioClient_SetStreamingState();
        leAudioClient_SetMediaOrCallState(TRUE);
    }
    else
    {
        /* Cleanup the session data */
        leAudioClient_EndUnicastAudioSession(&client_ctxt->session_data);
    }
}

static void leAudioClient_HandleUnicastVolumeStateInd(uint8 volume_state)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClient_HandleUnicastVolumeStateInd Le Audio state %d, Volume state: %d", LeAudioClient_GetState(), volume_state);

    if (leAudioClient_IsInConnectedState())
    {
        if (client_ctxt->session_data.audio_context == CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL)
        {
            Volume_SendVoiceSourceVolumeUpdateRequest(voice_source_le_audio_unicast_1, event_origin_external, volume_state);
        }
        else
        {
            /* If session context is not yet set also (it can happen if in connected state), consider it as audio */
            Volume_SendAudioSourceVolumeUpdateRequest(audio_source_le_audio_unicast_sender, event_origin_external, volume_state);
        }
    }
}

/* CIS Isochronous streams have got connected for a audio session Store them in context */
static void leAudioClient_HandleUnicastCisConnectInd(gatt_cid_t cid, uint8 cis_count, void *cis_handles, void *codec_qos_config,
                                                     bool cis_connect_status)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    CapClientCisHandles *cis_info = (CapClientCisHandles *) cis_handles;

    DEBUG_LOG("leAudioClient_HandleUnicastCisConnectInd CisCount : %d, success : %d", cis_count, cis_connect_status);

    if (cis_connect_status)
    {
        le_audio_client_cis_devices_info_t *dev, *end_dev;
        le_audio_client_cis_info_t *cis, *end_cis;

        client_ctxt->session_data.codec_qos_config = *(CapClientAudioConfig *) codec_qos_config;

        DEBUG_LOG("leAudioClient_HandleUnicastCisConnectInd vendorCodecId : 0x%x, codecId : 0x%x",
                  client_ctxt->session_data.codec_qos_config.vendorCodecId, client_ctxt->session_data.codec_qos_config.codecId);

        for (dev = &client_ctxt->session_data.devices_cis_info[0],
                end_dev = &dev[LE_AUDIO_CLIENT_MAX_DEVICES_SUPPORTED - 1]; dev <= end_dev; dev++)
        {
            /* See if any free slot to populate the received cis info */
            if (dev->cid == INVALID_CID)
            {
                /* Populate the cid and cis count */
                dev->cid = cid;
                dev->cis_count = cis_count;

                PanicFalse(cis_count <= LE_AUDIO_CLIENT_MAX_CIS_PER_DEVICE);

                for (cis = &dev->cis_info[0], end_cis = &dev->cis_info[dev->cis_count]; cis < end_cis; cis++)
                {
                    if (cis_info->direction == LE_CIS_DIRECTION_BOTH)
                    {
                        client_ctxt->session_data.enable_vbc = TRUE;
                    }

                    cis->cis_handle = cis_info->cisHandle;
                    cis->audio_location = cis_info->audioLocation;
                    cis->direction = cis_info->direction;
                    cis_info++;
                }
                break;
            }
        }
    }
}

/* Inform registered clients with a stream stopped indication */
static void leAudioClient_HandleUnicastStreamStopInd(ServiceHandle group_handle, le_audio_client_profiles_t profile, bool stream_stopped)
{
    bool stop_streaming = FALSE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClient_HandleUnicastStreamStopInd success:%d", stream_stopped);

    if (client_ctxt->session_data.release_config)
    {
        if (leAudioClient_IsInUnicastStreaming())
        {
            stop_streaming = profile == LE_AUDIO_CLIENT_PROFILE_TMAP ? TmapClientSource_StopUnicastStreaming(group_handle, TRUE) :
                                                                       CapProfileClient_StopUnicastStreaming(group_handle, TRUE);
            if (stop_streaming)
            {
                client_ctxt->session_data.release_config = FALSE;
            }
        }
    }
    else
    {
        leAudioClientMessages_SendUnicastStreamStopInd(group_handle, stream_stopped,
                                                        client_ctxt->session_data.audio_context);

        leAudioClient_SetMediaOrCallState(FALSE);

        /* Cleanup the session data */
        leAudioClient_EndUnicastAudioSession(&client_ctxt->session_data);
        leAudioClient_ClearStreamingState();
    }
}

/* Inform registered clients with a stream start cancelled indication */
static void leAudioClient_HandleUnicastConfigRemovedInd(ServiceHandle group_handle, bool config_remove_success)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClient_HandleUnicastConfigRemovedInd success:%d", config_remove_success);

    /* Send indication that stream request cancelled successfully */
    leAudioClientMessages_SendStreamStartCancelCompleteInd(group_handle, TRUE);
    /* Cleanup the session data */
    leAudioClient_EndUnicastAudioSession(&client_ctxt->session_data);
}

/* Inform registered clients with a stream stopped indication */
static void leAudioClient_HandleCisLinkLossInd(const CAP_PROFILE_CLIENT_CIS_LINK_LOSS_T *message)
{
    DEBUG_LOG("leAudioClient_HandleCisLinkLossInd 0x%04X", message->group_handle);
    /* ToDo: CIS link loss */
}

static void leAudioClient_HandleConfigComplete(ServiceHandle group_handle, uint16 audio_context_configured, bool config_complete_success)
{
    CapClientContext requested_audio_context;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClient_HandleConfigComplete: context:0x%x success %d", audio_context_configured, config_complete_success);

    requested_audio_context = leAudioClient_GetNextAudioContextToConfigure(client_ctxt->requested_audio_contexts);

    if (config_complete_success)
    {
        /* Configuration is successfull. Update the configured list */
        PanicFalse(requested_audio_context == audio_context_configured);
        client_ctxt->configured_audio_contexts |= audio_context_configured;
    }

    /* Clear the audio context from requested list */
    client_ctxt->requested_audio_contexts &= (~requested_audio_context);

    /* Configure the next audio context in the requested list (if any) */
    if (!leAudioClient_ConfigureNextAudioContext(group_handle))
    {
        /* There is no more configuration needs to be done. Move the state to connected */
        LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_CONNECTED);
        leAudioClientMessages_SendConnectInd(group_handle, LE_AUDIO_CLIENT_STATUS_SUCCESS,
                                             leAudioClient_GetTotalDeviceCount(client_ctxt),
                                             leAudioClient_GetConnectedDeviceCount(client_ctxt));
    }
}

/*! Handles register with CAP confirmation message for TMAP */
static void leAudioClient_HandleTmapRegisterCapCfm(tmap_client_msg_status_t status)
{
    bool move_to_connected = TRUE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClient_HandleTmapRegisterCapCfm: status %d", status);

    /* Now we need to configure profiles according to the contexts */
    if (client_ctxt->requested_audio_contexts != CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)
    {
        if (MessagePendingFirst(leAudioClient_GetTask(), LE_AUDIO_CLIENT_INTERNAL_QLL_CONNECT_TIMEOUT, NULL))
        {
            LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_WAITING_FOR_QLL_TO_CONNECT);
            return;
        }
        else
        {
            /* Configure the first audio context from the requested contexts list */
            if (leAudioClient_ConfigureNextAudioContext(leAudioClient_GetContext()->group_handle))
            {
                /* Configuring is in progress, move to connected state only after its completion */
                move_to_connected = FALSE;
            }
        }
    }

    if (move_to_connected)
    {
        /* There is nothing to configure for TMAP/CAP. Move the state to connected */
        LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_CONNECTED);
        leAudioClientMessages_SendConnectInd(leAudioClient_GetContext()->group_handle, LE_AUDIO_CLIENT_STATUS_SUCCESS,
                                             leAudioClient_GetTotalDeviceCount(client_ctxt),
                                             leAudioClient_GetConnectedDeviceCount(client_ctxt));
    }
}

/* Handle TMAP Profile generic messages in connecting state */
static void leAudioClient_HandleTmapMessagesInConnectingState(const tmap_client_msg_t *message)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    switch (message->id)
    {
        case TMAP_CLIENT_MSG_ID_INIT_COMPLETE:
        {
            const TMAP_CLIENT_MSG_ID_INIT_COMPLETE_T *cfm = &message->body.init_complete;

            DEBUG_LOG("leAudioClient_HandleTmapMessagesInConnectingState: TMAP_CLIENT_MSG_ID_INIT_COMPLETE Status %d", cfm->status);

            /* Treat TMAS service not present also as success because profiles will be internally routing all TMAP operation
               through CAP if TMAS service is not present. */
            if (cfm->status == TMAP_CLIENT_MSG_STATUS_SUCCESS || cfm->status == TMAP_CLIENT_MSG_STATUS_SUCCESS_TMAS_SRVC_NOT_FOUND)
            {

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
                /* Need to register task for broadcast assistant related APIs to work */
                PbpClientSource_RegisterTaskWithCap(cfm->group_handle);
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

                TmapClientSource_RegisterTaskWithCap(cfm->group_handle);

                if (leAudioClient_GetConnectedDeviceCount(client_ctxt) > 1)
                {
                    /* If there is more than one member connected, then need to add those devices into the TMAP group */
                    TmapClientSource_AddDeviceToGroup(&client_ctxt->gatt_cid_list[0], leAudioClient_GetConnectedDeviceCount(client_ctxt));
                }
            }
            else
            {
                DEBUG_LOG("leAudioClient_HandleTmapMessagesInConnectingState: TMAP init failed");

                /* Reaching here indicates that TMAP init got failed but CAP init was successful.
                   Eg. LE ACL gets disconnected immediately after CAP init got successful.
                   So destroy the CAP instance and send connect failure indication */
                PanicFalse(CapProfileClient_DestroyInstance(client_ctxt->group_handle,
                                                            leAudioClient_GetFirstConnectedGattCid(client_ctxt)));
            }
        }
        break;

        case TMAP_CLIENT_MSG_ID_REGISTER_CAP_CFM:
        {
            const TMAP_CLIENT_MSG_ID_REGISTER_CAP_CFM_T *cfm = &message->body.cap_register_cfm;

            leAudioClient_HandleTmapRegisterCapCfm(cfm->status);
        }
        break;

        default:
        break;
    }
}

static void leAudioClient_ProcessTmapUnicastMessageInPtsMode(const tmap_client_unicast_msg_t *ind)
{
    switch (ind->id)
    {
        case TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE_T *config_complete = &ind->body.unicast_config_complete;
            DEBUG_LOG("leAudioClient_ProcessTmapUnicastMessageInPtsMode Config Status 0x%x", config_complete->status);
        }
        break;

        case TMAP_CLIENT_MSG_ID_UNICAST_CIS_CONNECT:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_CIS_CONNECT_T *cis_connect = &ind->body.unicast_cis_connect;
            DEBUG_LOG("leAudioClient_ProcessTmapUnicastMessageInPtsMode CIS Status %d", cis_connect->status);
        }
        break;

        case TMAP_CLIENT_MSG_ID_UNICAST_STREAM_START:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_STREAM_START_T *stream_start =  &ind->body.unicast_stream_start;
            DEBUG_LOG("leAudioClient_ProcessTmapUnicastMessageInPtsMode Stream Start %d", stream_start->status);

            leAudioClient_SetMediaOrCallState(TRUE);

            LeAudioClient_StartPtsStreamingToDevices(TmapClientSource_PtsGetFirstGattDeviceCid(),
                                                     TmapClientSource_PtsGetSecondGattDeviceCid(),
                                                     TmapClientSource_PtsIsSpkrPresent(),
                                                     TmapClientSource_PtsIsMicPresent());
         }
        break;

        case TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP_T *stream_stop =  &ind->body.unicast_stream_stop;
            DEBUG_LOG("leAudioClient_ProcessTmapUnicastMessageInPtsMode Stream Stopt %d", stream_stop->status);

            leAudioClient_SetMediaOrCallState(FALSE);
        }
        break;

        default:
        break;
    }
}

/* Handle TMAP Profile unicast messages in connecting state */
static void leAudioClient_HandleTmapUnicastMessagesInConnectingState(const tmap_client_unicast_msg_t *message)
{
    switch (message->id)
    {
        case TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE_T *cfm = &message->body.unicast_config_complete;

            leAudioClient_HandleConfigComplete(cfm->group_handle, cfm->audio_context, cfm->status == TMAP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        default:
        break;
    }
}

/* Handle TMAP Profile generic messages in disconnecting state */
static void leAudioClient_HandleTmapMessagesInDisconnectingState(const tmap_client_msg_t *message)
{
    switch (message->id)
    {
        case TMAP_CLIENT_MSG_ID_PROFILE_DISCONNECT:
        {
            const TMAP_CLIENT_MSG_ID_PROFILE_DISCONNECT_T *cfm = &message->body.disconnect_complete;

            DEBUG_LOG("leAudioClient_HandleTmapMessagesInDisconnectingState: TMAP_CLIENT_MSG_ID_PROFILE_DISCONNECT Status %d", cfm->status);

            if (CapProfileClient_IsCapConnected())
            {
                /* TMAP disconnected. Now disconnect CAP */
                PanicFalse(CapProfileClient_DestroyInstance(leAudioClient_GetContext()->group_handle,
                                                            leAudioClient_GetFirstConnectedGattCid(leAudioClient_GetContext())));
            }
            else
            {
                /* Both profiles disconnected */
                leAudioClientMessages_SendDisconnectInd(leAudioClient_GetFirstConnectedGattCid(leAudioClient_GetContext()),
                                                                                               cfm->status == TMAP_CLIENT_MSG_STATUS_SUCCESS);
                leAudioClient_ResetContext();
            }

            /* TMAP profile gets disconnected. Reset the broadcast source related parameters */
            leAudioClientBroadcast_ResetSourceContext();
        }
        break;

        case TMAP_CLIENT_MSG_ID_PROFILE_DEVICE_REMOVED:
        {
            const TMAP_CLIENT_MSG_ID_PROFILE_DEVICE_REMOVED_T *cfm = &message->body.device_removed;

            DEBUG_LOG("leAudioClient_HandleTmapMessagesInDisconnectingState: TMAP_CLIENT_MSG_ID_PROFILE_DEVICE_REMOVED Status %d", cfm->status);

            leAudioClient_GetContext()->connected_devices--;

            /* Send indication to registered clients that device is removed only if CAP is not connected. If CAP is connected,
               this is done when corresponding CAP instance gets removed */
            if (!CapProfileClient_IsCapConnectedForCid(cfm->cid))
            {
                /* Destroy the instance of given cid alone */
                leAudioClientMessages_SendDeviceRemovedInd(cfm->cid, cfm->status == TMAP_CLIENT_MSG_STATUS_SUCCESS, cfm->more_devices_present);
            }
            else
            {
                if (!cfm->more_devices_present)
                {
                     /* All CAP instances in the group is removed. TMAP disconnect will be trigered when we receive
                        TMAP_CLIENT_MSG_ID_PROFILE_DISCONNECT. */
                    return;
                }

                /* Disconnect the CAP instance as TMAP is now disconected */
                PanicFalse(CapProfileClient_DestroyInstance(leAudioClient_GetContext()->group_handle, cfm->cid));
            }
        }
        break;

        default:
        break;
    }
}

/* Handle TMAP Profile unicast messages in disconnecting state */
static void leAudioClient_HandleTmapUnicastMessagesInDisconnectingState(const tmap_client_unicast_msg_t *message)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    switch (message->id)
    {
        case TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP_T *cfm = &message->body.unicast_stream_stop;

            DEBUG_LOG("leAudioClient_HandleTmapMessagesInDisconnectingState: TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP_T Status %d", cfm->status);

            if (client_ctxt->session_data.release_config)
            {
                PanicFalse(TmapClientSource_StopUnicastStreaming(client_ctxt->group_handle, TRUE));
                client_ctxt->session_data.release_config = FALSE;
            }
            else
            {
                leAudioClient_SetMediaOrCallState(FALSE);
                /* Destroy the instance of given cid alone */
                PanicFalse(TmapClientSource_DestroyInstance(client_ctxt->group_handle, client_ctxt->gatt_cid));
            }
        }
        break;

        default:
        break;
    }
}

/* Process all the TMAP profile generic messages */
static void leAudioClient_ProcessTmapMessage(const tmap_client_msg_t *message)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (client_ctxt->state < LE_AUDIO_CLIENT_STATE_CONNECTED)
    {
        return;
    }

    switch (message->id)
    {
        case TMAP_CLIENT_MSG_ID_VOLUME_STATE_IND:
        {
            leAudioClient_HandleUnicastVolumeStateInd(message->body.volume_state_ind.volumeState);
        }
        break;

        default:
        break;
    }
}

/* Process all the TMAP profile unicast messages */
static void leAudioClient_ProcessTmapUnicastMessage(const tmap_client_unicast_msg_t *message)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (client_ctxt->state < LE_AUDIO_CLIENT_STATE_CONNECTED)
    {
        return;
    }

    /* If in PTS mode just by pass all the TMAP messages to the TMAP PTS message handler */
    if (client_ctxt->pts_mode != LE_AUDIO_CLIENT_PTS_MODE_OFF)
    {
        leAudioClient_ProcessTmapUnicastMessageInPtsMode(message);
        return;
    }

    switch (message->id)
    {
        case TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE_T *config_complete = &message->body.unicast_config_complete;

            leAudioClient_StartStreamingIfConfigCompleted(config_complete->group_handle, config_complete->audio_context,
                                                          config_complete->status == TMAP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_UNICAST_CIS_CONNECT:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_CIS_CONNECT_T *cis_connect = &message->body.unicast_cis_connect;

            leAudioClient_HandleUnicastCisConnectInd(cis_connect->cid, cis_connect->cis_count, cis_connect->cis_handles,
                                                     cis_connect->codec_qos_config,
                                                     cis_connect->status == TMAP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_UNICAST_STREAM_START:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_STREAM_START_T *stream_start =  &message->body.unicast_stream_start;

            leAudioClient_HandleUnicastStreamStartInd(stream_start->status == TMAP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP_T *stream_stop =  &message->body.unicast_stream_stop;

            leAudioClient_HandleUnicastStreamStopInd(stream_stop->group_handle, LE_AUDIO_CLIENT_PROFILE_TMAP,
                                                     stream_stop->status == TMAP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_REMOVED:
        {
            const TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_REMOVED_T *config_removed =  &message->body.unicast_config_removed;

            leAudioClient_HandleUnicastConfigRemovedInd(config_removed->group_handle,
                                                        config_removed->status == TMAP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        default:
        break;
    }
}

/*! \brief Process TMAP Domain generic Messages */
static void leAudioClient_ProcessTmapClientMessage(const tmap_client_msg_t *message)
{
    switch (LeAudioClient_GetState())
    {
        case LE_AUDIO_CLIENT_STATE_CONNECTING:
        case LE_AUDIO_CLIENT_STATE_WAITING_FOR_QLL_TO_CONNECT:
            leAudioClient_HandleTmapMessagesInConnectingState(message);
        break;

        case LE_AUDIO_CLIENT_STATE_DISCONNECTING:
            leAudioClient_HandleTmapMessagesInDisconnectingState(message);
        break;

        case LE_AUDIO_CLIENT_STATE_CONNECTED:
            leAudioClient_ProcessTmapMessage(message);
        break;

        default:
        break;
    }
}

/*! \brief Process TMAP Domain unicast Messages */
static void leAudioClient_ProcessTmapClientUnicastMessage(const tmap_client_unicast_msg_t *message)
{
    switch (LeAudioClient_GetState())
    {
        case LE_AUDIO_CLIENT_STATE_CONNECTING:
            leAudioClient_HandleTmapUnicastMessagesInConnectingState(message);
        break;

        case LE_AUDIO_CLIENT_STATE_DISCONNECTING:
            leAudioClient_HandleTmapUnicastMessagesInDisconnectingState(message);
        break;

        case LE_AUDIO_CLIENT_STATE_CONNECTED:
            leAudioClient_ProcessTmapUnicastMessage(message);
        break;

        default:
        break;
    }
}

/* Handles the messages from CAP profile while in PTS mode */
static void leAudioClient_ProcessCapUnicastMessageInPtsMode(const cap_profile_client_msg_t *ind)
{
    switch (ind->id)
    {
        case CAP_PROFILE_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE:
        {
            const CAP_PROFILE_CLIENT_UNICAST_CONFIG_COMPLETE_T *config_complete = &ind->body.unicast_config_complete;
            DEBUG_LOG("leAudioClient_ProcessCapUnicastMessageInPtsMode Config Status %d", config_complete->status);
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_UNICAST_CIS_CONNECT:
        {
            const CAP_PROFILE_CLIENT_UNICAST_CIS_CONNECT_T *cis_connect = &ind->body.unicast_cis_connect;
            DEBUG_LOG("leAudioClient_ProcessCapUnicastMessageInPtsMode Cis Status %d", cis_connect->status);
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_UNICAST_STREAM_START:
        {
            const CAP_PROFILE_CLIENT_UNICAST_STREAM_START_T *stream_start =  &ind->body.unicast_stream_start;
            DEBUG_LOG("leAudioClient_ProcessCapUnicastMessageInPtsMode Stream Start %d", stream_start->status);
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_UNICAST_STREAM_STOP:
        {
            const CAP_PROFILE_CLIENT_UNICAST_STREAM_STOP_T *stream_stop =  &ind->body.unicast_stream_stop;
           DEBUG_LOG("leAudioClient_ProcessCapUnicastMessageInPtsMode Strea Stop %d", stream_stop->status);
        }
        break;

        default:
        break;
    }
}

/* Process all the CAP profile messages */
static void leAudioClient_ProcessCapUnicastMessage(const cap_profile_client_msg_t *message)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (client_ctxt->state < LE_AUDIO_CLIENT_STATE_CONNECTED)
    {
        return;
    }

    if (client_ctxt->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_CAP)
    {
        leAudioClient_ProcessCapUnicastMessageInPtsMode(message);
        return;
    }

    switch (message->id)
    {
        case CAP_PROFILE_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE:
        {
            const CAP_PROFILE_CLIENT_UNICAST_CONFIG_COMPLETE_T *config_complete = &message->body.unicast_config_complete;

            leAudioClient_StartStreamingIfConfigCompleted(config_complete->group_handle, config_complete->audio_context,
                                                          config_complete->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS);
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_UNICAST_CIS_CONNECT:
        {
            const CAP_PROFILE_CLIENT_UNICAST_CIS_CONNECT_T *cis_connect = &message->body.unicast_cis_connect;

            leAudioClient_HandleUnicastCisConnectInd(cis_connect->cid, cis_connect->cis_count, cis_connect->cis_handles,
                                                     cis_connect->codec_qos_config,
                                                     cis_connect->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS);
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_UNICAST_STREAM_START:
        {
            const CAP_PROFILE_CLIENT_UNICAST_STREAM_START_T *stream_start =  &message->body.unicast_stream_start;

            leAudioClient_HandleUnicastStreamStartInd(stream_start->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS);
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_UNICAST_STREAM_STOP:
        {
            const CAP_PROFILE_CLIENT_UNICAST_STREAM_STOP_T *stream_stop =  &message->body.unicast_stream_stop;

            leAudioClient_HandleUnicastStreamStopInd(stream_stop->group_handle, LE_AUDIO_CLIENT_PROFILE_CAP,
                                                     stream_stop->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS);
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_CIS_LINK_LOSS:
            leAudioClient_HandleCisLinkLossInd(&message->body.cis_link_loss);
        break;

        case CAP_PROFILE_CLIENT_MSD_ID_VOLUME_STATE:
        {
            leAudioClient_HandleUnicastVolumeStateInd(message->body.volume_state.volumeState);
        }
        break;

        case CAP_PROFILE_CLIENT_MSD_ID_UNICAST_CONFIG_REMOVED:
        {
            const CAP_PROFILE_CLIENT_UNICAST_CONFIG_REMOVED_T *config_removed =  &message->body.unicast_config_removed;

            leAudioClient_HandleUnicastConfigRemovedInd(config_removed->group_handle,
                                                        config_removed->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS);
        }
        break;

        case CAP_PROFILE_CLIENT_MSD_ID_FLUSH_TIMEOUT_INFO:
        {
            const CAP_PROFILE_CLIENT_MSD_ID_FLUSH_TIMEOUT_INFO_T *flush_timeout_info =  &message->body.flush_timeout_info;

            leAudioClient_UpdateFlushTimeoutRange(flush_timeout_info->group_handle, (cap_profile_ft_info_t*)&flush_timeout_info->ft_info);
        }
        break;

        default:
        break;
    }
}


/* Handle CAP Profile messages in connecting state */
static void leAudioClient_HandleCapMessagesInConnectingState(const cap_profile_client_msg_t *message)
{
    uint8 device_count;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    switch (message->id)
    {
        case CAP_PROFILE_CLIENT_MSG_ID_INIT_COMPLETE:
        {
            const CAP_PROFILE_CLIENT_INIT_COMPLETE_T *cfm = &message->body.init_complete;

            DEBUG_LOG("leAudioCLient_HandleCapMessagesInConnectingState: CAP_PROFILE_CLIENT_INIT_COMPLETE Status %d", cfm->status);

            if (cfm->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS)
            {
                /* All devices in the group got added successfully. */
                leAudioClient_GetContext()->group_handle = cfm->group_handle;
                leAudioClient_GetContext()->total_devices = cfm->total_devices;
                leAudioClient_GetContext()->connected_devices = cfm->connected_devices;

                /* CAP is initialised, now try to initialise TMAP. Panic if TMAP init request not able to place */
                PanicFalse(TmapClientSource_CreateInstance(leAudioClient_GetFirstConnectedGattCid(leAudioClient_GetContext())));
                TmapClientSource_SetGroupId(leAudioClient_GetFirstConnectedGattCid(leAudioClient_GetContext()), cfm->group_handle);

                /* Send indication that device got added successfully and no other device pending to add */
                leAudioClientMessages_SendDeviceAddedInd(leAudioClient_GetContext()->gatt_cid, TRUE, FALSE);
            }
            else
            {
                /* LE Audio Client failed to connect. Reset the context */
                leAudioClient_ResetContext();

                /* Send connection failure indication */
                leAudioClientMessages_SendDeviceAddedInd(leAudioClient_GetContext()->gatt_cid, FALSE, TRUE);
                leAudioClientMessages_SendConnectInd(cfm->group_handle, LE_AUDIO_CLIENT_STATUS_FAILED, 0, 0);
            }
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_DEVICE_ADDED:
        {
            const CAP_PROFILE_CLIENT_DEVICE_ADDED_T *cfm = &message->body.device_added;

            DEBUG_LOG("leAudioCLient_HandleCapMessagesInConnectingState: CAP_PROFILE_CLIENT_MSG_ID_DEVICE_ADDED Status %d", cfm->status);

            if (cfm->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS)
            {
                leAudioClient_GetContext()->group_handle = cfm->group_handle;

                for(device_count = 0; device_count < LE_AUDIO_CLIENT_MAX_DEVICES_SUPPORTED; device_count++)
                {
                    /* Add the CID of the new added device to an empty slot */
                    if (client_ctxt->gatt_cid_list[device_count] == INVALID_CID)
                    {
                        client_ctxt->gatt_cid_list[device_count] = cfm->cid;
                        break;
                    }
                }
            }
            leAudioClientMessages_SendDeviceAddedInd(leAudioClient_GetContext()->gatt_cid, cfm->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS,
                                                     cfm->more_devices_needed);
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_DEVICE_REMOVED:
        {
            const CAP_PROFILE_CLIENT_DEVICE_REMOVED_T *cfm = &message->body.device_removed;

            DEBUG_LOG("leAudioCLient_HandleCapMessagesInConnectingState: CAP_PROFILE_CLIENT_MSG_ID_DEVICE_REMOVED Status %d", cfm->status);

            leAudioClientMessages_SendDeviceRemovedInd(cfm->cid, cfm->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS,
                                                       cfm->more_devices_present);
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT:
        {
            const CAP_PROFILE_CLIENT_DISCONNECT_T *cfm = &message->body.disconnect_complete;

            DEBUG_LOG("leAudioCLient_HandleCapMessagesInConnectingState: CAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT Status %d", cfm->status);

            /* Both profiles disconnected */
            leAudioClientMessages_SendConnectInd(cfm->group_handle, LE_AUDIO_CLIENT_STATUS_FAILED, 0, 0);
            leAudioClient_ResetContext();
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE:
        {
            const CAP_PROFILE_CLIENT_UNICAST_CONFIG_COMPLETE_T *cfm = &message->body.unicast_config_complete;

            leAudioClient_HandleConfigComplete(cfm->group_handle, cfm->audio_context, cfm->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS);
        }
        break;

        default:
        break;
    }
}

/* Handle CAP Profile messages in disconnecting state */
static void leAudioClient_HandleCapMessagesInDisconnectingState(const cap_profile_client_msg_t *message)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    switch (message->id)
    {
        case CAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT:
        {
            const CAP_PROFILE_CLIENT_DISCONNECT_T *cfm = &message->body.disconnect_complete;

            DEBUG_LOG("leAudioClient_HandleCapMessagesInDisconnectingState: CAP_PROFILE_CLIENT_DISCONNECT_T Status %d", cfm->status);

            if (TmapClientSource_IsTmapConnected())
            {
                /* CAP disconnected. Now disconnect TMAP */
                PanicFalse(TmapClientSource_DestroyInstance(cfm->group_handle, 0));
            }
            else
            {
                /* Both profiles disconnected */
                leAudioClientMessages_SendDisconnectInd(cfm->group_handle, cfm->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS);
                leAudioClient_ResetContext();
            }

            /* On disconnection, reset the streaming flag(if active) in unicast mode */
            if (leAudioClient_IsInUnicastStreaming())
            {
                leAudioClient_ClearStreamingState();
            }
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_DEVICE_REMOVED:
        {
            const CAP_PROFILE_CLIENT_DEVICE_REMOVED_T *cfm = &message->body.device_removed;

            DEBUG_LOG("leAudioClient_HandleCapMessagesInDisconnectingState: CAP_PROFILE_CLIENT_MSG_ID_DEVICE_REMOVED Status %d", cfm->status);

            /* Send indication to registered clients that device is removed only if TMAP is not connected. If TMAP is connected,
               this is done when corresponding TMAP instance gets removed */
            if (!TmapClientSource_IsTmapConnectedForCid(cfm->cid))
            {
                leAudioClientMessages_SendDeviceRemovedInd(cfm->cid, cfm->status == CAP_PROFILE_CLIENT_STATUS_SUCCESS, cfm->more_devices_present);
            }
            else
            {
                if (!cfm->more_devices_present)
                {
                     /* All CAP instances in the group is removed. TMAP disconnect will be trigered when we receive
                        CAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT. */
                    return;
                }

                /* Disconnect the TMAP instance as CAP is now disconected */
                PanicFalse(TmapClientSource_DestroyInstance(client_ctxt->group_handle, cfm->cid));
            }
        }
        break;

        case CAP_PROFILE_CLIENT_MSG_ID_UNICAST_STREAM_STOP:
        {
            const CAP_PROFILE_CLIENT_UNICAST_STREAM_STOP_T *cfm = &message->body.unicast_stream_stop;

            DEBUG_LOG("leAudioClient_HandleCapMessagesInDisconnectingState: CAP_PROFILE_CLIENT_MSG_ID_UNICAST_STREAM_STOP Status %d", cfm->status);

            if (client_ctxt->session_data.release_config)
            {
                PanicFalse(CapProfileClient_StopUnicastStreaming(client_ctxt->group_handle, TRUE));
                client_ctxt->session_data.release_config = FALSE;
            }
            else
            {
                leAudioClient_SetMediaOrCallState(FALSE);
                CapProfileClient_DestroyInstance(client_ctxt->group_handle, client_ctxt->gatt_cid);
            }
        }
        break;

        default:
        break;
    }
}

/*! \brief Process CAP Domain Messages */
static void leAudioClient_ProcessCapClientMessage(ServiceHandle group_handle, const cap_profile_client_msg_t *message)
{
    UNUSED(group_handle);

    switch (LeAudioClient_GetState())
    {
        case LE_AUDIO_CLIENT_STATE_CONNECTING:
            leAudioClient_HandleCapMessagesInConnectingState(message);
        break;

        case LE_AUDIO_CLIENT_STATE_DISCONNECTING:
            leAudioClient_HandleCapMessagesInDisconnectingState(message);
        break;

        case LE_AUDIO_CLIENT_STATE_CONNECTED:
            leAudioClient_ProcessCapUnicastMessage(message);
        break;

        default:
        break;
    }
}

/* Handle CSIP Profile messages in connecting CSIP state */
static void leAudioClient_HandleCsipMessagesInConnectingState(const csip_client_msg_t *message)
{
    switch (message->id)
    {
        case CSIP_CLIENT_MSG_ID_DEVICE_ADDED:
        {
            const CSIP_CLIENT_DEVICE_ADDED_T *cfm = &message->body.device_added;

            DEBUG_LOG("leAudioClient_HandleCapMessagesInConnectingCsipState: CSIP_CLIENT_MSG_ID_DEVICE_ADDED Status %d",
                       cfm->status);

            leAudioClientMessages_SendDeviceAddedInd(cfm->cid, cfm->status == CSIP_CLIENT_STATUS_SUCCESS,
                                                     cfm->more_devices_needed);
        }
        break;

        case CSIP_CLIENT_MSG_ID_INIT_COMPLETE:
        {
            const CSIP_CLIENT_INIT_COMPLETE_T *cfm = &message->body.init_complete;

            DEBUG_LOG("leAudioClient_HandleCapMessagesInConnectingCsipState: CSIP_CLIENT_MSG_ID_INIT_COMPLETE Status %d",
                       cfm->status);

            leAudioClientMessages_SendConnectInd(0,
                                                 cfm->status == CSIP_CLIENT_STATUS_SUCCESS ? LE_AUDIO_CLIENT_STATUS_SUCCESS :
                                                                                             LE_AUDIO_CLIENT_STATUS_FAILED,
                                                 cfm->total_devices,
                                                 cfm->connected_devices);
        }
        break;

        case CSIP_CLIENT_MSG_ID_DEVICE_REMOVED:
        {
            const CSIP_CLIENT_DEVICE_REMOVED_T *cfm = &message->body.device_removed;

            DEBUG_LOG("leAudioClient_HandleCapMessagesInConnectingCsipState: CSIP_CLIENT_MSG_ID_DEVICE_REMOVED Status %d",
                       cfm->status);

            leAudioClientMessages_SendDeviceRemovedInd(cfm->cid, cfm->status == CSIP_CLIENT_STATUS_SUCCESS, cfm->more_devices_present);
        }
        break;

        case CSIP_CLIENT_MSG_ID_PROFILE_DISCONNECT:
        {
            const CSIP_CLIENT_DISCONNECT_T *cfm = &message->body.disconnected;

            DEBUG_LOG("leAudioClient_HandleCapMessagesInConnectingCsipState: CSIP_CLIENT_MSG_ID_PROFILE_DISCONNECT Status %d",
                       cfm->status);

            leAudioClientMessages_SendDisconnectInd(0, cfm->status == CSIP_CLIENT_STATUS_SUCCESS);
        }
        break;

        default:
            DEBUG_LOG("leAudioClient_HandleCapMessagesInConnectingCsipState: unhandled message 0x%x", message->id);
        break;
    }
}

/*! \brief Process CSIP Domain Messages */
static void leAudioClient_ProcessCsipClientMessage(const csip_client_msg_t *message)
{
    switch (LeAudioClient_GetState())
    {
        case LE_AUDIO_CLIENT_STATE_CONNECTING:
            leAudioClient_HandleCsipMessagesInConnectingState(message);
        break;

        default:
        break;
    }
}

static void leAudioClient_ConfigureStreamIfWaitingForQllEvent(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (le_audio_client_context.state == LE_AUDIO_CLIENT_STATE_WAITING_FOR_QLL_TO_CONNECT)
    {
        LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_CONNECTING);

        /* Configure the first default audio context from the requested contexts list */
        if (!leAudioClient_ConfigureNextAudioContext(client_ctxt->group_handle))
        {
            /* There is no pending configuration. Move the state to connected */
            LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_CONNECTED);
            leAudioClientMessages_SendConnectInd(client_ctxt->group_handle, LE_AUDIO_CLIENT_STATUS_SUCCESS,
                                                 leAudioClient_GetTotalDeviceCount(client_ctxt),
                                                 leAudioClient_GetConnectedDeviceCount(client_ctxt));
        }
    }
}

static void leAudioClient_HandleQcomRemoteIsoCapableMessage(const QCOM_CON_MANAGER_REMOTE_ISO_QHS_CAPABLE_IND_T *msg)
{
    DEBUG_LOG("leAudioClient_HandleQcomRemoteIsoCapableMessage lap=%06lX", msg->tp_addr.taddr.addr.lap);

    le_audio_client_context.iso_qhs_supported = TRUE;

    MessageCancelFirst(leAudioClient_GetTask(), LE_AUDIO_CLIENT_INTERNAL_QLL_CONNECT_TIMEOUT);
    leAudioClient_ConfigureStreamIfWaitingForQllEvent();
}

static uint16 leAudioClient_GetQhsLevelFromMask(uint8 qhs_mask)
{
    uint16 qhs_level;

    switch(qhs_mask)
    {
        case QCOM_QHS_2_MASK :
            qhs_level = QCOM_QHS_LEVEL_2;
        break;

        case QCOM_QHS_3_MASK :
            qhs_level = QCOM_QHS_LEVEL_3;
        break;

        case QCOM_QHS_4_MASK :
            qhs_level = QCOM_QHS_LEVEL_4;
        break;

        case QCOM_QHS_5_MASK :
            qhs_level = QCOM_QHS_LEVEL_5;
        break;

        case QCOM_QHS_6_MASK :
            qhs_level = QCOM_QHS_LEVEL_6;
        break;

        default :
            qhs_level = QCOM_QHS_LEVEL_INVALID;
        break;
    }

    return qhs_level;
}

static void leAudioClient_HandleQcomQhsRateChangeInd(const QCOM_CON_MANAGER_QHS_RATE_CHANGED_IND_T *msg)
{
    uint16 qhs_level = leAudioClient_GetQhsLevelFromMask(msg->qhs_rate);

    DEBUG_LOG("leAudioClient_HandleQcomQhsRateChangeInd cig_id =%d, QHS Level = %d", msg->cig_id, qhs_level);

    le_audio_client_context.qhs_level = qhs_level;

    appKymeraUpdateQhsLevel(qhs_level);
}

static void leAudioClient_HandleQllConnectionTimeout(void)
{
    DEBUG_LOG("leAudioClient_HandleQllConnectionTimeout");
    leAudioClient_ConfigureStreamIfWaitingForQllEvent();
}

/* Handle BAP Profile messages in connecting BAP state */
static void leAudioClient_HandleBapMessagesInConnectingState(const bap_profile_client_msg_t *message)
{
    switch (message->id)
    {
        case BAP_PROFILE_CLIENT_MSG_ID_INIT_COMPLETE:
        {
            const BAP_PROFILE_CLIENT_INIT_COMPLETE_T *cfm = &message->body.init_complete;

            DEBUG_LOG("leAudioClient_HandleBapMessagesInConnectingBapState: BAP_PROFILE_CLIENT_MSG_ID_INIT_COMPLETE Status %d",
                       cfm->status);

            if (cfm->status == BAP_PROFILE_CLIENT_STATUS_SUCCESS)
            {
                if (cfm->more_devices_needed)
                {
                    leAudioClientMessages_SendDeviceAddedInd(leAudioClient_GetFirstConnectedGattCid(leAudioClient_GetContext()),
                                                             TRUE,
                                                             cfm->more_devices_needed);
                }
                else
                {
                    leAudioClientMessages_SendConnectInd(0, LE_AUDIO_CLIENT_STATUS_SUCCESS, 1, 1);
                    LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_CONNECTED);
                }
            }
            else
            {
                leAudioClientMessages_SendConnectInd(0, LE_AUDIO_CLIENT_STATUS_FAILED, 1, 1);
                leAudioClient_ResetContext();
            }
        }
        break;

        default:
            DEBUG_LOG("leAudioClient_HandleBapMessagesInConnectingState: unhandled message 0x%x", message->id);
        break;
    }
}

/* Handle BAP Profile messages in connected BAP state */
static void leAudioClient_HandleBapMessagesInConnectedState(const bap_profile_client_msg_t *message)
{
    switch (message->id)
    {
        case BAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT:
        {
            const BAP_PROFILE_CLIENT_DISCONNECT_T *cfm = &message->body.disconnected;

            DEBUG_LOG("leAudioClient_HandleBapMessagesInConnectedState: BAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT Status %d",
                       cfm->status);

            if (cfm->connected_server_cnt == 0)
            {
                leAudioClientMessages_SendDisconnectInd(0, cfm->status == BAP_PROFILE_CLIENT_STATUS_SUCCESS);
                leAudioClient_ResetContext();
            }
        }
        break;

        default:
            DEBUG_LOG("leAudioClient_HandleBapMessagesInConnectedState: unhandled message 0x%x", message->id);
        break;
    }
}

/*! \brief Process BAP Domain Messages */
static void leAudioClient_ProcessBapClientMessage(const bap_profile_client_msg_t *message)
{
    switch (LeAudioClient_GetState())
    {
        case LE_AUDIO_CLIENT_STATE_CONNECTING:
            leAudioClient_HandleBapMessagesInConnectingState(message);
        break;

        case LE_AUDIO_CLIENT_STATE_CONNECTED:
            leAudioClient_HandleBapMessagesInConnectedState(message);
        break;

        default:
        break;
    }
}

static bool leAudioClient_CapConnect(gatt_cid_t cid)
{
    bool status = FALSE;

    /* Panic if all devices in the group is already connected */
    PanicFalse(leAudioClient_GetContext()->connected_devices == 0 ||
               leAudioClient_GetContext()->connected_devices != leAudioClient_GetContext()->total_devices);

    if (leAudioClient_GetContext()->group_handle != INVALID_GROUP_HANDLE)
    {
        if (CapProfileClient_AddDeviceToGroup(le_audio_client_context.group_handle, cid))
        {
            le_audio_client_context.gatt_cid = cid;
            status = TRUE;
        }
    }
    else
    {
        PanicFalse(le_audio_client_context.gatt_cid == INVALID_CID);

        le_audio_client_context.gatt_cid = cid;

        /* Set the default audio contexts to configure */
        le_audio_client_context.configured_audio_contexts = 0;

#ifdef INCLUDE_QCOM_CON_MANAGER
        if (!le_audio_client_context.iso_qhs_supported)
        {
            MessageCancelFirst(leAudioClient_GetTask(), LE_AUDIO_CLIENT_INTERNAL_QLL_CONNECT_TIMEOUT);
            MessageSendLater(leAudioClient_GetTask(), LE_AUDIO_CLIENT_INTERNAL_QLL_CONNECT_TIMEOUT, NULL,
                             leAudioClient_QllConnectTimeout());
        }
#endif

        if (CapProfileClient_CreateInstance(cid))
        {
            LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_CONNECTING);
            status = TRUE;
        }

        DEBUG_LOG_INFO("leAudioClient_CapConnect: Connecting & Configuring for audio_contexts 0x%x status %d",
                       le_audio_client_context.requested_audio_contexts, status);
    }

    return status;
}

static bool leAudioClient_ConnectLeaProfileForPts(gatt_cid_t cid)
{
    bool status = FALSE;

    switch (leAudioClient_GetContext()->pts_mode)
    {
        case LE_AUDIO_CLIENT_PTS_MODE_CSIP:
        {
            if (CsipClient_CreateInstance(cid))
            {
                LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_CONNECTING);
                status = TRUE;
            }
        }
        break;

        case LE_AUDIO_CLIENT_PTS_MODE_VCP:
        {
            if (VcpProfileClient_CreateInstance(cid))
            {
                leAudioClientMessages_SendConnectInd(0, LE_AUDIO_CLIENT_STATUS_SUCCESS, 1, 1);
                LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_CONNECTED);
                status = TRUE;
            }
        }
        break;

        case LE_AUDIO_CLIENT_PTS_MODE_GMCS:
        case LE_AUDIO_CLIENT_PTS_MODE_GTBS:
        {
           LeTmapServer_EnablePtsMode(TRUE);
           /* Send connected indication to sink service */
           leAudioClientMessages_SendConnectInd(0, LE_AUDIO_CLIENT_STATUS_SUCCESS, 1, 1);
           LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_CONNECTED);
           status = TRUE;
        }
        break;

        case LE_AUDIO_CLIENT_PTS_MODE_BAP:
        {
            if (BapProfileClient_CreateInstance(cid, leAudioClient_ProcessBapClientMessage))
            {
                LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_CONNECTING);
                status = TRUE;
            }
        }
        break;

        default:
        break;
    }

    DEBUG_LOG_INFO("leAudioClient_ConnectLeaProfileForPts: enum:le_audio_client_pts_mode_t:%d connection status %d",
                   leAudioClient_GetContext()->pts_mode, status);

    return status;
}

static void leAudioClient_HandleDisconnectRequestInPtsMode(gatt_cid_t cid)
{
    switch (leAudioClient_GetContext()->pts_mode)
    {
        case LE_AUDIO_CLIENT_PTS_MODE_CSIP:
        {
           if (!CsipClient_DestroyInstance(cid))
           {
                /* Todo: Destroy failed mostly because group disconnect is not implemented in CSIP.
                   Needs to handle it. */
                leAudioClientMessages_SendDisconnectInd(0, LE_AUDIO_CLIENT_STATUS_FAILED);
           }
        }
        break;

        case LE_AUDIO_CLIENT_PTS_MODE_BAP:
        {
            if (!BapProfileClient_DestroyInstance(cid))
            {
                leAudioClientMessages_SendDisconnectInd(0, LE_AUDIO_CLIENT_STATUS_FAILED);
                leAudioClient_ResetContext();
            }
        }
        break;

        case LE_AUDIO_CLIENT_PTS_MODE_VCP:
        {
            if (!VcpProfileClient_DestroyInstance(cid))
            {
                leAudioClientMessages_SendDisconnectInd(0, LE_AUDIO_CLIENT_STATUS_FAILED);
                leAudioClient_ResetContext();
            }
            else
            {
                leAudioClientMessages_SendDisconnectInd(0, BAP_PROFILE_CLIENT_STATUS_SUCCESS);
                leAudioClient_ResetContext();
            }
        }
        break;

        case LE_AUDIO_CLIENT_PTS_MODE_GMCS:
        case LE_AUDIO_CLIENT_PTS_MODE_GTBS:
        {
            leAudioClientMessages_SendDisconnectInd(0, TRUE);
            leAudioClient_ResetContext();
        }
        break;

        default:
        break;
    }
}

static uint16 leAudioClient_GetCisHandleForAudioLocation(uint16 audio_location, uint8 direction)
{
    le_audio_client_cis_devices_info_t *dev, *end_dev;
    le_audio_client_cis_info_t *cis, *end_cis;

    for (dev = &leAudioClient_GetContext()->session_data.devices_cis_info[0],
            end_dev = &dev[LE_AUDIO_CLIENT_MAX_DEVICES_SUPPORTED - 1]; dev <= end_dev; dev++)
    {
        if (dev->cid != INVALID_CID)
        {
            for (cis = &dev->cis_info[0], end_cis = &dev->cis_info[dev->cis_count]; cis < end_cis; cis++)
            {
                if (cis->audio_location == audio_location &&
                    (cis->direction == LE_CIS_DIRECTION_BOTH || cis->direction == direction))
                {
                    DEBUG_LOG_INFO("leAudioClient_GetCisHandleForAudioLocation Handle 0x%x for location 0x%x, direction %d",
                                   cis->cis_handle, audio_location, cis->direction);
                    return cis->cis_handle;
                }
            }
        }
    }

    return INVALID_CIS_HANDLE;
}

void leAudioClient_ResetUnicastContext(void)
{
    uint8 device_count;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    MessageCancelFirst(leAudioClient_GetTask(), LE_AUDIO_CLIENT_INTERNAL_QLL_CONNECT_TIMEOUT);
    leAudioClient_EndUnicastAudioSession(&client_ctxt->session_data);

    client_ctxt->gatt_cid = INVALID_CID;
    client_ctxt->total_devices = 0;
    client_ctxt->connected_devices = 0;
    for(device_count = 0; device_count < LE_AUDIO_CLIENT_MAX_DEVICES_SUPPORTED; device_count++)
    {
        client_ctxt->gatt_cid_list[device_count] = INVALID_CID;
    }
}

void leAudioClient_InitUnicast(void)
{
    leAudioClient_ResetUnicastContext();

    /* Register as observer to CAP profile to receive CAP events */
    CapProfileClient_Init(leAudioClient_ProcessCapClientMessage);

    CsipClient_Init(leAudioClient_ProcessCsipClientMessage);

    LeTmapServer_RegisterForRemoteCallControls(leAudioClient_GetTask());
    TmapClientSource_RegisterCallback(leAudioClient_ProcessTmapClientMessage);
    TmapClientSourceUnicast_RegisterCallback(leAudioClient_ProcessTmapClientUnicastMessage);

    leAudioClient_InitMusicSource();
    leAudioClient_InitVoiceSource();
    LeAudioClientVolume_Init();
}

bool leAudioClient_StartUnicastStreaming(ServiceHandle group_handle, uint16 audio_context)
{
    bool stream_start_req = FALSE;
    le_audio_client_profiles_t profile;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (client_ctxt->state < LE_AUDIO_CLIENT_STATE_CONNECTED ||
        leAudioClient_IsInUnicastStreaming())
    {
        DEBUG_LOG("leAudioClient_StartUnicastStreaming Failed State : %d", client_ctxt->state);
        return FALSE;
    }

    DEBUG_LOG("leAudioClient_StartUnicastStreaming audio_context:0x%x configured_audio_contexts 0x%x ", audio_context, client_ctxt->configured_audio_contexts);

    profile = leAudioClient_GetProfileForAudioContext(audio_context);

    if (!client_ctxt->pts_mode)
    {
        leAudioClient_StartUnicastAudioSession(group_handle, audio_context);

        /* If the requested audio context is already configured, start streaming it directly */
        if (client_ctxt->configured_audio_contexts & audio_context)
        {
            if (profile == LE_AUDIO_CLIENT_PROFILE_CAP)
            {
                leAudioClient_SetDefaultFlushTimeoutRange(group_handle, audio_context);
                stream_start_req = CapProfileClient_StartUnicastStreaming(group_handle, audio_context);
            }
            else
            {
                leAudioClient_SetQhsMapReq(TmapProfileClient_GetCigId(group_handle), audio_context);
                stream_start_req = TmapClientSource_StartUnicastStreaming(group_handle, audio_context);
            }
        }
        else
        {
            /* Requested audio context is not configured, configure it */
            stream_start_req = leAudioClient_ConfigureUnicastStreaming(group_handle, audio_context);
        }
    }

    return stream_start_req;
}

bool leAudioClient_StopUnicastStreaming(ServiceHandle group_handle, bool remove_config)
{
    bool stop_streaming = FALSE;
    le_audio_client_profiles_t profile_streaming;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (leAudioClient_IsInUnicastStreaming())
    {
        profile_streaming = leAudioClient_GetProfileForAudioContext(client_ctxt->session_data.audio_context);

        stop_streaming = profile_streaming == LE_AUDIO_CLIENT_PROFILE_TMAP ? TmapClientSource_StopUnicastStreaming(group_handle, FALSE) :
                                                                             CapProfileClient_StopUnicastStreaming(group_handle, FALSE);

        if (stop_streaming)
        {
            DEBUG_LOG("LeAudioClient_StopStreaming profile:%d group_handle:0x%x", profile_streaming, group_handle);
            if (remove_config)
            {
                client_ctxt->configured_audio_contexts &= (~client_ctxt->session_data.audio_context);
            }

            /* Remove the config later once the streaming has stopped and all the underlying
             * ASEs are disabled and CIS is disconnected by CAP.
             */
            client_ctxt->session_data.release_config = remove_config;
        }
    }

    return stop_streaming;
}

void leAudioClient_HandleUnicastMessage(MessageId id, Message message)
{
    switch (id)
    {
        case QCOM_CON_MANAGER_REMOTE_ISO_QHS_CAPABLE_IND:
            leAudioClient_HandleQcomRemoteIsoCapableMessage(message);
        break;

        case QCOM_CON_MANAGER_QHS_RATE_CHANGED_IND:
            leAudioClient_HandleQcomQhsRateChangeInd(message);
        break;

        case LE_AUDIO_CLIENT_INTERNAL_QLL_CONNECT_TIMEOUT:
            leAudioClient_HandleQllConnectionTimeout();
        break;

        case TMAP_SERVER_REMOTE_CALL_CONTROL_CALL_ACCEPT:
            leAudioClientMessages_HandleRemoteCallControlAccept();
        break;

        case TMAP_SERVER_REMOTE_CALL_CONTROL_CALL_TERMINATE:
            leAudioClientMessages_HandleRemoteCallControlTerminate();
        break;

        default:
        break;
    }
}

device_t leAudioClient_GetDeviceForSource(generic_source_t source)
{
    bool source_found;
    device_t device = GattConnect_GetBtLeDevice(leAudioClient_GetFirstConnectedGattCid(leAudioClient_GetContext()));

    source_found = (source.type == source_type_audio) ? (DeviceProperties_GetAudioSource(device) == source.u.audio) :
                                                        (DeviceProperties_GetVoiceSource(device) == source.u.voice);

    if (source_found)
    {
        return device;
    }
    else
    {
        return NULL;
    }
}

uint16 leAudioClient_GetSinkCisHandleForAudioLocation(uint16 audio_location)
{
    return leAudioClient_GetCisHandleForAudioLocation(audio_location, LE_CIS_DIRECTION_DL);
}

uint16 leAudioClient_GetSrcCisHandleForAudioLocation(uint16 audio_location)
{
    return leAudioClient_GetCisHandleForAudioLocation(audio_location, LE_CIS_DIRECTION_UL);
}

appKymeraLeStreamType leAudioClient_GetUnicastIsoHandles(uint8 dir, uint16 *iso_handle, uint16 *iso_handle_right)
{
    appKymeraLeStreamType stream_type;

    *iso_handle = leAudioClient_GetCisHandleForAudioLocation(BAP_AUDIO_LOCATION_FL, dir);
    *iso_handle_right = leAudioClient_GetCisHandleForAudioLocation(BAP_AUDIO_LOCATION_FR, dir);

    /* If iso handle is not present for FL alone location */
    if (*iso_handle == INVALID_CIS_HANDLE)
    {
        if (*iso_handle_right != INVALID_CIS_HANDLE)
        {
            /* Get iso handle for FR only location */
            *iso_handle = *iso_handle_right;
            *iso_handle_right = INVALID_CIS_HANDLE;

            stream_type = KYMERA_LE_STREAM_MONO;
        }
        else
        {
            /* Get the iso handle for FR+FL location (ie, stereo stream type) */
            *iso_handle = leAudioClient_GetCisHandleForAudioLocation(BAP_AUDIO_LOCATION_FL | BAP_AUDIO_LOCATION_FR, dir);
            stream_type = KYMERA_LE_STREAM_STEREO_USE_BOTH;
        }
    }
    else
    {
        /* Stream type should be dual mono if iso handle is present for both FR and FL alone locations */
        stream_type = *iso_handle_right != INVALID_CIS_HANDLE ? KYMERA_LE_STREAM_DUAL_MONO : KYMERA_LE_STREAM_MONO;
    }

    /* Panic if unable to get iso handle */
    PanicFalse(*iso_handle != INVALID_CIS_HANDLE);

    return stream_type;
}

bool LeAudioClient_Connect(gatt_cid_t cid)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    switch (client_ctxt->pts_mode)
    {
        case LE_AUDIO_CLIENT_PTS_MODE_CAP:
        case LE_AUDIO_CLIENT_PTS_MODE_TMAP:
        {
            /* Set the default audio context to zero so that it will not get
               configured as part of connection */
            LeAudioClient_SetDefaultAudioContext(0);
        }
        /* Fall through */
        case LE_AUDIO_CLIENT_PTS_MODE_OFF:
        {
            return leAudioClient_CapConnect(cid);
        }

        default:
        {
            return leAudioClient_ConnectLeaProfileForPts(cid);
        }
    }
}

bool LeAudioClient_Disconnect(gatt_cid_t cid, bool disconnect_gracefully)
{
    bool disconnect_req_sent = FALSE;
    le_audio_client_state_t client_state = LeAudioClient_GetState();
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG_INFO("LeAudioClient_Disconnect: CID: cid=0x%04X, Client State %d, graceful %d",
                    cid, client_state, disconnect_gracefully);

    switch (client_state)
    {
        case LE_AUDIO_CLIENT_STATE_CONNECTED:
        case LE_AUDIO_CLIENT_STATE_CONNECTING:
        case LE_AUDIO_CLIENT_STATE_WAITING_FOR_QLL_TO_CONNECT:
        {
            disconnect_req_sent = TRUE;

            if (!leAudioClient_IsInPtsMode())
            {
                /* Stream stop only needs to be called for graceful disconnection only for unicast */
                if (disconnect_gracefully && leAudioClient_IsInUnicastStreaming())
                {
                    /* Do not call the stream stop API if there is one already in progress. Absence of currently streaming
                       audio context in the configured audio context list indicates that a stop stream request is in progress.
                       In such cases, update the LE audio client state and just wait for the stream stop confirm to receive */
                    if (client_ctxt->session_data.audio_context & client_ctxt->configured_audio_contexts)
                    {
                        LeAudioClient_StopStreaming(leAudioClient_GetContext()->group_handle, TRUE);
                    }
                    else
                    {
                        DEBUG_LOG_INFO("LeAudioClient_Disconnect waiting");
                    }
                }
                else
                {
                    if (TmapClientSource_IsTmapConnected())
                    {
                        PanicFalse(TmapClientSource_DestroyInstance(leAudioClient_GetContext()->group_handle, cid));
                    }
                    else
                    {
                        CapProfileClient_DestroyInstance(leAudioClient_GetContext()->group_handle, cid);
                    }
                }

                LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_DISCONNECTING);
            }
            else
            {
                leAudioClient_HandleDisconnectRequestInPtsMode(cid);
            }
        }
        break;

        default:
        break;
    }

    if (disconnect_req_sent)
    {
        le_audio_client_context.gatt_cid = cid;
    }

    return disconnect_req_sent;
}

bool LeAudioClient_IsAdvertFromSetMember(uint8 *adv_data, uint16 adv_data_len)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    /* In BAP mode PTS mode, just return TRUE */
    if (client_ctxt->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_BAP || client_ctxt->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_TMAP)
    {
        return TRUE;
    }

    if (LeAudioClient_GetState() == LE_AUDIO_CLIENT_STATE_CONNECTING && client_ctxt->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_CSIP)
    {
        return CsipClient_IsAdvertFromSetMember(adv_data, adv_data_len);
    }

    return CapProfileClient_IsAdvertFromSetMember(leAudioClient_GetContext()->group_handle, adv_data, adv_data_len);
}

bool LeAudioClient_IsUnicastConnected(ServiceHandle group_handle)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    return (client_ctxt->group_handle == group_handle &&
            client_ctxt->state > LE_AUDIO_CLIENT_STATE_CONNECTING);
}

bool leAudioClient_IsInUnicastStreaming(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    return (leAudioClient_IsStreamingEnabled() && client_ctxt->mode == LE_AUDIO_CLIENT_MODE_UNICAST);
}

bool LeAudioClient_IsUnicastStreamingActive(ServiceHandle group_handle)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    return (client_ctxt->group_handle == group_handle &&
            leAudioClient_IsInUnicastStreaming());
}

bool LeAudioClient_DeviceDiscoveryFailed(void)
{
    bool status = FALSE;

    if (leAudioClient_GetContext()->pts_mode == LE_AUDIO_CLIENT_PTS_MODE_CSIP)
    {
        /* Todo : Need to handle this scenario */
        return TRUE;
    }

    DEBUG_LOG_INFO("LeAudioClient_DeviceDiscoveryFailed");

    status = CapProfileClient_CompleteInitWithExistingDevices(leAudioClient_GetContext()->group_handle);

    return status;
}

uint16 LeAudioClient_GetUnicastSessionCapAudioContext(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    return client_ctxt->session_data.audio_context;
}

uint32 LeAudioClient_GetConfiguredCodecId(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    le_audio_unicast_session_data_t *unicast_session_data = &client_ctxt->session_data;

    return ((uint32) unicast_session_data->codec_qos_config.codecId) |
            ((uint32) unicast_session_data->codec_qos_config.codecVersionNum << 8u) |
            ((uint32) unicast_session_data->codec_qos_config.vendorCodecId << 16u);
}

bool LeAudioClient_IsInUnicastMode(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    return client_ctxt->mode == LE_AUDIO_CLIENT_MODE_UNICAST;
}

ServiceHandle LeAudioClient_GetGroupHandle(gatt_cid_t cid)
{
    ServiceHandle group_handle = INVALID_GROUP_HANDLE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (cid == INVALID_CID || leAudioClient_GetFirstConnectedGattCid(client_ctxt) == cid)
    {
        group_handle = client_ctxt->group_handle;
    }

    return group_handle;
}

void LeAudioClient_SetBootGameModeConfig(bool enable)
{
    le_audio_client_context.requested_audio_contexts = enable ? CAP_CLIENT_CONTEXT_TYPE_GAME : CAP_CLIENT_CONTEXT_TYPE_MEDIA;
}

/*! Sets the absolute volume */
void LeAudioClient_SetAbsoluteVolume(ServiceHandle group_handle, uint8 volume)
{
    CapProfileClient_SetAbsoluteVolume(group_handle, volume);
}

uint8 LeAudioClient_GetAbsoluteVolume(ServiceHandle group_handle)
{
    volume_t volume = { 0 };
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    if (group_handle == client_ctxt->group_handle)
    {
        if (leAudioClient_IsInUnicastStreaming() &&
            client_ctxt->session_data.audio_context == CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL)
        {
            volume = VoiceSources_GetVolume(voice_source_le_audio_unicast_1);
        }
        else
        {
            volume = AudioSources_GetVolume(audio_source_le_audio_unicast_sender);
        }
    }

    DEBUG_LOG_INFO("LeAudioClient_GetAbsoluteVolume volume: %d", volume.value);

    return volume.value;
}

/*! Mute/Unmute the volume */
void LeAudioClient_SetMute(ServiceHandle group_handle, bool mute)
{
    CapProfileClient_SetMute(group_handle, mute);
}

bool LeAudioClient_CreateIncomingCall(void)
{
    return LeTmapServer_CreateCall(TBS_CALL_STATE_INCOMING, TBS_CALL_FLAG_CALL_DIRECTION_INCOMING);
}

bool LeAudioClient_CreateActiveOutgoingCall(void)
{
   return LeTmapServer_CreateCall(TBS_CALL_STATE_ACTIVE, TBS_CALL_FLAG_CALL_DIRECTION_OUTGOING);
}

bool LeAudioClient_IsCallActive(void)
{
    return LeTmapServer_IsCallPresent();
}

uint8 LeAudioClient_GetCurrentMediaState(void)
{
    return  LeTmapServer_GetCurrentMediaState();
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
