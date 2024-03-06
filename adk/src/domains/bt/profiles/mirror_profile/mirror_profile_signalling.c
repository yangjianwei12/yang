/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Mirror profile channel for sending messages between Primary & Secondary.
*/

#ifdef INCLUDE_MIRRORING

#include <stdlib.h>

#include <bdaddr.h>
#include <sink.h>

#include "audio_sources.h"
#include "kymera_adaptation_audio_protected.h"
#include "volume_messages.h"
#include "kymera.h"
#include "av_instance.h"
#ifdef ENABLE_LEA_CIS_DELEGATION
#include "le_unicast_manager_peer_signal.h"
#include "kymera_le_mic_chain.h"
#endif

#include "mirror_profile_signalling.h"
#include "mirror_profile_typedef.h"
#include "mirror_profile_marshal_typedef.h"
#include "mirror_profile_private.h"
#include "mirror_profile_voice_source.h"
#include "mirror_profile_audio_source.h"

/*! The stream context rate is represented as Hz/25 */
#define STREAM_CONTEXT_RATE_MULTIPLIER 25

#define peerSigTx(message, type) appPeerSigMarshalledMsgChannelTx(\
    MirrorProfile_GetTask(), \
    PEER_SIG_MSG_CHANNEL_MIRROR_PROFILE, \
    (message), MARSHAL_TYPE(type))

#define peerSigCancelTx(type) appPeerSigMarshalledMsgChannelTxCancelAll(\
    MirrorProfile_GetTask(), \
    PEER_SIG_MSG_CHANNEL_MIRROR_PROFILE, \
    MARSHAL_TYPE(type))


/*! Flags in the mirror_profile_stream_context_t message */
#define MIRROR_PROFILE_STREAM_CONTEXT_FLAG_SEND_RESPONSE 0x01

/*
    Functions sending a mirror_profile channel message
*/

void MirrorProfile_SendHfpVolumeToSecondary(voice_source_t source, uint8 volume)
{
    mirror_profile_hfp_volume_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
    msg->voice_source = source;
    msg->volume = volume;
    peerSigCancelTx(mirror_profile_hfp_volume_ind_t);
    peerSigTx(msg, mirror_profile_hfp_volume_ind_t);
}

void MirrorProfile_SendHfpCodecAndVolumeToSecondary(voice_source_t voice_source, hfp_codec_mode_t codec_mode, uint8 volume)
{
    mirror_profile_hfp_codec_and_volume_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
    msg->codec_mode = codec_mode;
    msg->volume = volume;
    msg->voice_source = voice_source;
    peerSigTx(msg, mirror_profile_hfp_codec_and_volume_ind_t);
}

void MirrorProfile_SendA2dpVolumeToSecondary(audio_source_t source, uint8 volume)
{
    mirror_profile_a2dp_volume_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
    msg->audio_source = source;
    msg->volume = volume;
    peerSigCancelTx(mirror_profile_a2dp_volume_ind_t);
    peerSigTx(msg, mirror_profile_a2dp_volume_ind_t);
}

static void mirrorProfile_SendA2dpStreamContextToSecondaryImpl(audio_source_t source, bool request_response)
{
    mirror_profile_cached_context_t* cached_context = MirrorProfile_GetCachedContext(source);

    if (appPeerSigIsConnected())
    {
        mirror_profile_stream_context_t *context = PanicUnlessMalloc(sizeof(*context));
        avInstanceTaskData *av_inst = AvInstance_GetSinkInstanceForAudioSource(source);
        bdaddr* handset_addr = PanicNull(AvInstance_GetBdaddr(av_inst));

        memset(context, 0, sizeof(*context));

        context->addr = *handset_addr;
        PanicFalse(!BdaddrIsZero(&context->addr));
        context->cid = cached_context->cid;
        context->mtu = cached_context->mtu;
        context->seid = cached_context->seid;
        context->sample_rate = (uint16)((cached_context->sample_rate) / STREAM_CONTEXT_RATE_MULTIPLIER);
        context->content_protection_type = cached_context->content_protection ?
                    UINT16_BUILD(AVDTP_CP_TYPE_SCMS_MSB, AVDTP_CP_TYPE_SCMS_LSB) : 0;

        context->volume = mirrorProfile_GetMirroredAudioVolume(source);
        context->q2q_mode = cached_context->q2q_mode;
        context->aptx_features = cached_context->aptx_features;
        context->audio_source = source;

        if (request_response)
        {
            context->flags |= MIRROR_PROFILE_STREAM_CONTEXT_FLAG_SEND_RESPONSE;
        }

        /* If the mirrored instance is already streaming, force the audio state
           sent to active. This ensures the secondary starts in the
           MIRROR_PROFILE_A2DP_START_SECONDARY_SYNC_UNMUTE state meaning primary
           and secondary should start with a synchronised unmute. */
        if (av_inst && appA2dpIsStreaming(av_inst))
        {
            context->audio_state = AUDIO_SYNC_STATE_ACTIVE;
        }
        else
        {
            context->audio_state = cached_context->state;
        }

        peerSigTx(context, mirror_profile_stream_context_t);

        MIRROR_LOG("MirrorProfile_SendA2dpStreamContextToSecondary. enum:audio_source_t:%d %d", source, context->audio_state);
    }
    else
    {
        MirrorProfile_ClearStreamChangeLock();
    }
}

void MirrorProfile_SendA2dpStreamContextToSecondary(audio_source_t source)
{
    mirrorProfile_SendA2dpStreamContextToSecondaryImpl(source, FALSE);
}

void MirrorProfile_SendA2dpStreamContextToSecondaryRequestResponse(audio_source_t source)
{
    mirrorProfile_SendA2dpStreamContextToSecondaryImpl(source, TRUE);
}

void MirrorProfile_SendA2dpStreamContextToSecondaryBlockUntilResponse(audio_source_t source)
{
    MirrorProfile_SetStreamChangeLock();
    MirrorProfile_SendA2dpStreamContextToSecondaryRequestResponse(source);
    /* Kick the sm once the stream context message response is received */
    MessageSendConditionally(MirrorProfile_GetTask(),
                             MIRROR_INTERNAL_KICK_TARGET_STATE, NULL,
                             MirrorProfile_GetStreamChangeLockAddr());
}

void MirrorProfile_SendA2pdUnmuteTimeToPrimary(rtime_t unmute_time)
{
    if (MirrorProfile_IsSecondary())
    {
        mirror_profile_sync_a2dp_unmute_ind_t *ind = PanicUnlessMalloc(sizeof(*ind));
        // Clock domain conversion is done by peer signalling type conversion */
        ind->unmute_time = unmute_time;
        peerSigTx(ind, mirror_profile_sync_a2dp_unmute_ind_t);
    }
}

void MirrorProfile_HandleKymeraScoStarted(void)
{
    if (MirrorProfile_IsSecondary())
    {
        mirror_profile_sync_sco_unmute_ind_t *ind = PanicUnlessMalloc(sizeof(*ind));
        rtime_t unmute_time = rtime_add(VmGetTimerTime(), mirrorProfileConfig_ScoSyncUnmuteDelayUs());
        // Clock domain conversion is done by peer signalling type conversion */
        ind->unmute_time = unmute_time;
        peerSigTx(ind, mirror_profile_sync_sco_unmute_ind_t);
        Kymera_ScheduleScoSyncUnmute(RtimeTimeToMsDelay(unmute_time));
    }
}

static void mirrorProfile_UpdateAudioVolumeFromPeer(audio_source_t audio_source, int new_volume)
{
    /* only if we have valid audio_source from primary, we allow the volume update */
    if(audio_source != audio_source_none)
    {
        volume_t volume = AudioSources_GetVolume(audio_source);
        if (volume.value != new_volume)
        {
            Volume_SendAudioSourceVolumeUpdateRequest(audio_source, event_origin_peer, new_volume);
        }
    }
}

static void mirrorProfile_HandleA2dpStreamContext(const mirror_profile_stream_context_t *context)
{
    mirror_profile_a2dp_t *a2dp_state = MirrorProfile_GetA2dpState();
    mirror_profile_cached_context_t* cached_context = MirrorProfile_GetCachedContext(context->audio_source);

    MIRROR_LOG("mirrorProfile_HandleA2dpStreamContext enum:audio_source_t:%d ind_state:%d q2q|seid:%02x cid 0x%x",
                    context->audio_source, context->audio_state, (context->q2q_mode<<4)|context->seid, context->cid);

    /* Update mirrored audio source only if this is the cid we are currently mirroring */
    if(context->cid == a2dp_state->cid)
    {
        a2dp_state->audio_source = context->audio_source;
    }

    cached_context->cid = context->cid;
    cached_context->mtu = context->mtu;
    cached_context->seid = context->seid;
    cached_context->sample_rate = context->sample_rate * STREAM_CONTEXT_RATE_MULTIPLIER;
    cached_context->content_protection = (context->content_protection_type != 0);
    cached_context->q2q_mode = context->q2q_mode;
    cached_context->aptx_features = context->aptx_features;

    mirrorProfile_SetAudioSyncState(context->audio_source, context->audio_state);
    mirrorProfile_UpdateAudioVolumeFromPeer(context->audio_source, context->volume);

    if (context->flags & MIRROR_PROFILE_STREAM_CONTEXT_FLAG_SEND_RESPONSE)
    {
        mirror_profile_stream_context_response_t *response = PanicUnlessMalloc(sizeof(*response));
        memset(response, 0, sizeof(*response));

        response->cid = context->cid;
        response->seid = context->seid;
        peerSigTx(response, mirror_profile_stream_context_response_t);
    }
}

static void mirrorProfile_HandleA2dpStreamContextResponse(const mirror_profile_stream_context_response_t *response)
{
    UNUSED(response);
    MirrorProfile_ClearStreamChangeLock();
    MIRROR_LOG("mirrorProfile_HandleA2dpStreamContextResponse clearing lock");
}

static void mirrorProfile_HandleA2dpSyncUnmute(const mirror_profile_sync_a2dp_unmute_ind_t *ind)
{
    if (MirrorProfile_IsPrimary())
    {
        appKymeraA2dpSetSyncUnmuteTime(ind->unmute_time);
    }
}

static void mirrorProfile_HandleScoSyncUnmute(const mirror_profile_sync_sco_unmute_ind_t *ind)
{
    if (MirrorProfile_IsPrimary())
    {
        Kymera_ScheduleScoSyncUnmute(RtimeTimeToMsDelay(ind->unmute_time));
    }
}

static void mirrorProfile_HandleHfpCodecAndVolume(const mirror_profile_hfp_codec_and_volume_ind_t *ind)
{
    mirror_profile_esco_t *esco = MirrorProfile_GetScoState();
    MirrorProfile_SetScoCodec(ind->codec_mode);
    esco->voice_source = ind->voice_source;
    MirrorProfile_SetScoVolume(ind->voice_source, ind->volume);
    if (MirrorProfile_IsEscoConnected() && esco->codec_mode != hfp_codec_mode_none)
    {
        MirrorProfile_StartScoAudio();
    }
}

#ifdef ENABLE_LEA_CIS_DELEGATION

static void mirrorProfile_HandleLeaMicSyncUnmute(const mirror_profile_sync_lea_mic_unmute_ind_t *ind)
{
    if (MirrorProfile_IsPrimary())
    {
        Kymera_ScheduleLeaMicSyncUnmute(RtimeTimeToMsDelay(ind->unmute_time));
    }
}

void MirrorProfile_HandleKymeraLeaMicStarted(void)
{
    if (MirrorProfile_IsSecondary())
    {
        mirror_profile_sync_lea_mic_unmute_ind_t *ind = PanicUnlessMalloc(sizeof(*ind));
        rtime_t unmute_time = rtime_add(VmGetTimerTime(), mirrorProfileConfig_ScoSyncUnmuteDelayUs());
        /* Clock domain conversion is done by peer signalling type conversion */
        ind->unmute_time = unmute_time;
        peerSigTx(ind, mirror_profile_sync_lea_mic_unmute_ind_t);
        Kymera_ScheduleLeaMicSyncUnmute(RtimeTimeToMsDelay(unmute_time));
    }
}

void MirrorProfile_SendMicMute(uint8 mute_state)
{
    if (MirrorProfile_IsPrimary())
    {
        mirror_profile_lea_mic_mute_state_ind_t *ind = PanicUnlessMalloc(sizeof(*ind));
        ind->mute_state = mute_state;
        peerSigCancelTx(mirror_profile_lea_mic_mute_state_ind_t);
        peerSigTx(ind, mirror_profile_lea_mic_mute_state_ind_t);
    }
}

static void mirrorProfile_HandleMicMute(const mirror_profile_lea_mic_mute_state_ind_t *ind)
{
    if (MirrorProfile_IsSecondary())
    {
        KymeraLeAudioVoice_SetMicMuteState(ind->mute_state);
    }
}

void MirrorProfile_SendUnicastConfigData(multidevice_side_t side)
{
    UNUSED(side);
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    if (appPeerSigIsConnected())
    {
        mirror_profile_lea_unicast_audio_conf_req_t *cfg_data = PanicUnlessMalloc(sizeof(*cfg_data));

        if (LeUnicastManagerPeerSignal_FillCodecAndQoSCfg(Multidevice_GetPairSide(), cfg_data))
        {
            peerSigTx(cfg_data, mirror_profile_lea_unicast_audio_conf_req_t);
        }
        else
        {
            free(cfg_data);
        }
    }

    MIRROR_LOG("MirrorProfile_SendUnicastConfigData Sending peer LEA unicast config");
    /* @TODO should we get out of sniff subrating here? */

    /* Store locally if it is for our side, might be useful post handover */
    MIRROR_LOG("MirrorProfile_SendUnicastConfigData Remember own LEA unicast config");
    LeUnicastManagerPeerSignal_FillCodecAndQoSCfg(Multidevice_GetSide(), &lea_unicast->audio_config);
    MirrorProfile_SetLeUnicastConfigRcvd();
}

void MirrorProfile_SendUnicastConfigDataClear(multidevice_side_t side)
{
    /* If inclides for peer (not same means either both or other side), do peer signalling. */
    if (Multidevice_IsSameAsPairSide(side))
    {
        MirrorProfile_ClearLeUnicastConfigSent();
        if (appPeerSigIsConnected())
        {
            mirror_profile_lea_unicast_audio_clear_req_t *cfg_data = PanicUnlessMalloc(sizeof(*cfg_data));
            peerSigTx(cfg_data, mirror_profile_lea_unicast_audio_clear_req_t);
            MIRROR_LOG("MirrorProfile_SendUnicastConfigDataClear Sending peer LEA unicast config clear");
        }
    }

    if (Multidevice_IsSameAsOurSide(side))
    {
        MirrorProfile_ClearLeUnicastConfigRcvd();
    }
}

static void mirrorProfile_HandleLeaUnicastConfig(const mirror_profile_lea_unicast_audio_conf_req_t *cfg_data)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();
    uint8 current_mirror_type = lea_unicast->audio_config.mirror_type;
    bool config_existed = MirrorProfile_GetLeUnicastConfigRcvd();

    MIRROR_LOG("mirrorProfile_HandleLeaUnicastConfig LEA unicast config rcvd, current config state: %d, current_mirror_type enum:le_um_cis_mirror_type_t:%d, New mirror type enum:le_um_cis_mirror_type_t:%d",
               MirrorProfile_GetLeUnicastConfigRcvd(), current_mirror_type, cfg_data->mirror_type);

    lea_unicast->audio_config = *cfg_data;
    MirrorProfile_SetLeUnicastConfigRcvd();
    MirrorProfile_SetLeUnicastConfigSent(); /* if handover happens, we have flag set appropriately */

    /* Soon LEA audio streaming would start, so power on Audio SS */
    appKymeraProspectiveDspPowerOn(KYMERA_POWER_ACTIVATION_MODE_ASYNC);

    if (!config_existed)
    {
        /* If this peerSig message is delayed, then check and try to establish data if possible  */
        MirrorProfile_CheckAndEstablishDataPath(lea_unicast);
    }
    else if (current_mirror_type != cfg_data->mirror_type && MirrorProfile_GetState() == MIRROR_PROFILE_STATE_CIS_CONNECTED)
    {
        /*  This is a reconfiguration scenario. The following possibilities can occur:
         *  1) We are transitioning from "Mirror type" to "Delegate" OR "Delegate with Src/Sink CIS Shared"
         *  2) We are transitioning from "Mirror Delegate type" to "Delegate with Src/Sink CIS Shared"
         *  With the above condition, if both Peer CIS & Own CIS are connected already, reconfigure the audio
         *  graph, else record that a reconfigure is pending 
         */
        if ((current_mirror_type == le_um_cis_mirror_type_mirror && cfg_data->mirror_type != le_um_cis_mirror_type_mirror) ||
            (current_mirror_type == le_um_cis_mirror_type_delegate &&
             cfg_data->mirror_type != le_um_cis_mirror_type_delegate && cfg_data->mirror_type != le_um_cis_mirror_type_mirror))
        {
            if (lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED &&
                lea_unicast->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED)
            {
                if (MirrorProfile_GetLeAudioContext() == AUDIO_CONTEXT_TYPE_COVERSATIONAL)
                {
                    MirrorProfile_ReconfigureLeVoiceGraph();
                }
                else
                {
                    MirrorProfile_ReconfigureLeaAudioGraph();
                }
            }
            else
            {
                /* Either of the cis is pending setup */
                lea_unicast->reconfig_pending = TRUE;
            }
        }
    }
}

static void mirrorProfile_ClearLeaAudioConfig(void)
{
    mirror_profile_lea_unicast_audio_conf_req_t *lea_audio_config = MirrorProfile_GetLeaAudioConfig();

    memset(lea_audio_config, 0, sizeof(*lea_audio_config));
}

static void mirrorProfile_HandleLeaUnicastConfigClear(const mirror_profile_lea_unicast_audio_clear_req_t *cfg_data)
{
    UNUSED(cfg_data);

    MIRROR_LOG("mirrorProfile_HandleLeaUnicastConfigClear LEA unicast config rcvd, current config state: %d",
               MirrorProfile_GetLeUnicastConfigRcvd());

    if (MirrorProfile_GetLeUnicastConfigRcvd())
    {
        mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();
        lea_unicast->reconfig_pending = FALSE;
    }

    MirrorProfile_ClearLeUnicastConfigRcvd();
    MirrorProfile_ClearLeUnicastConfigSent();  /* if handover happens, we have flag set appropriately */

    mirrorProfile_ClearLeaAudioConfig();
}

#endif /* ENABLE_LEA_CIS_DELEGATION */

/*
    Handlers for receiving mirror_profile channel messages.
*/

/* \brief Handle PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND */
void MirrorProfile_HandlePeerSignallingMessage(const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *ind)
{
    MIRROR_LOG("MirrorProfile_HandlePeerSignallingMessage. Channel 0x%x, type %d", ind->channel, ind->type);

    switch (ind->type)
    {
    case MARSHAL_TYPE(mirror_profile_hfp_volume_ind_t):
        {
            const mirror_profile_hfp_volume_ind_t* vol_ind = (const mirror_profile_hfp_volume_ind_t*)ind->msg;

            MirrorProfile_SetScoVolume(vol_ind->voice_source, vol_ind->volume);
        }
        break;

    case MARSHAL_TYPE(mirror_profile_hfp_codec_and_volume_ind_t):
        mirrorProfile_HandleHfpCodecAndVolume((const mirror_profile_hfp_codec_and_volume_ind_t*)ind->msg);
        break;

    case MARSHAL_TYPE(mirror_profile_a2dp_volume_ind_t):
        {
            const mirror_profile_a2dp_volume_ind_t* vol_ind = (const mirror_profile_a2dp_volume_ind_t*)ind->msg;

            MIRROR_LOG("MirrorProfile_HandlePeerSignallingMessage enum:audio_source_t:%d volume %d", vol_ind->audio_source, vol_ind->volume);
            mirrorProfile_UpdateAudioVolumeFromPeer(vol_ind->audio_source, vol_ind->volume);
        }
        break;

    case MARSHAL_TYPE(mirror_profile_stream_context_t):
        mirrorProfile_HandleA2dpStreamContext((const mirror_profile_stream_context_t*)ind->msg);
        break;

    case MARSHAL_TYPE(mirror_profile_stream_context_response_t):
        mirrorProfile_HandleA2dpStreamContextResponse((const mirror_profile_stream_context_response_t*)ind->msg);
        break;

    case MARSHAL_TYPE(mirror_profile_sync_a2dp_unmute_ind_t):
        mirrorProfile_HandleA2dpSyncUnmute((const mirror_profile_sync_a2dp_unmute_ind_t *)ind->msg);
        break;

    case MARSHAL_TYPE(mirror_profile_sync_sco_unmute_ind_t):
        mirrorProfile_HandleScoSyncUnmute((const mirror_profile_sync_sco_unmute_ind_t *)ind->msg);
        break;

#ifdef ENABLE_LEA_CIS_DELEGATION
    case MARSHAL_TYPE(mirror_profile_sync_lea_mic_unmute_ind_t):
        mirrorProfile_HandleLeaMicSyncUnmute((const mirror_profile_sync_lea_mic_unmute_ind_t *)ind->msg);
        break;

    case MARSHAL_TYPE(mirror_profile_lea_unicast_audio_conf_req_t):
        mirrorProfile_HandleLeaUnicastConfig((const mirror_profile_lea_unicast_audio_conf_req_t *)ind->msg);
        break;

    case MARSHAL_TYPE(mirror_profile_lea_unicast_audio_clear_req_t):
        mirrorProfile_HandleLeaUnicastConfigClear((const mirror_profile_lea_unicast_audio_clear_req_t *)ind->msg);
        break;

    case MARSHAL_TYPE(mirror_profile_lea_mic_mute_state_ind_t):
        mirrorProfile_HandleMicMute((const mirror_profile_lea_mic_mute_state_ind_t *)ind->msg);
        break;
#endif

    default:
        MIRROR_LOG("MirrorProfile_HandlePeerSignallingMessage unhandled type 0x%x", ind->type);
        break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

/* \brief Handle PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM */
void MirrorProfile_HandlePeerSignallingMessageTxConfirm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *cfm)
{
    UNUSED(cfm);

    switch (cfm->type)
    {
        case MARSHAL_TYPE(mirror_profile_stream_context_t):
            if (cfm->status != peerSigStatusSuccess)
            {
                MirrorProfile_ClearStreamChangeLock();
            }
        break;

#ifdef ENABLE_LEA_CIS_DELEGATION
        case MARSHAL_TYPE(mirror_profile_lea_unicast_audio_conf_req_t):
            MIRROR_LOG("MirrorProfile_HandlePeerSignallingMessageTxConfirm status: %d", cfm->status);
            if (cfm->status == peerSigStatusSuccess)
            {
                MirrorProfile_SetLeUnicastConfigSent();
            }
        break;
#endif

        default:
        break;
    }
}

#endif /* INCLUDE_MIRRORING */
