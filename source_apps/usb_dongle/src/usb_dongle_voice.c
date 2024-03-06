/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      USB Dongle application voice interface

*/
#include "usb_dongle_logging.h"

#include "usb_dongle_voice.h"
#include "usb_dongle_sm_private.h"
#include "usb_dongle_config.h"

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)
#include "usb_dongle_hfp.h"
#endif

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)
#include "usb_dongle_lea.h"
#include "usb_dongle_le_voice.h"
#endif

#include <voice_sources.h>
#include <kymera.h>
#include <kymera_usb_sco.h>
#include <kymera_adaptation_voice_protected.h>
#include <panic.h>

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)

typedef struct
{
    Sink sco_sink;
} voice_data_t;

voice_data_t voice_data;

void UsbDongle_VoiceStart(void)
{
    uint32_t sco_sample_rate;
    source_defined_params_t source_params_usb;
    source_defined_params_t source_params_hfp;

    if (!usbDongleConfig_IsInBredrMode() && !usbDongleConfig_IsInDualModeWithBredrConnected())
    {
        return;
    }

    if (!VoiceSources_GetConnectParameters(voice_source_usb, &source_params_usb))
    {
        return;
    }
    if (!source_params_usb.data)
    {
        Panic();
    }
    usb_voice_connect_parameters_t usb_params_data = *(usb_voice_connect_parameters_t *)source_params_usb.data;

    DEBUG_LOG_VERBOSE("UsbDongle_VoiceStart, usb_params_data:");
    DEBUG_LOG_VERBOSE("  spkr_src %d",usb_params_data.spkr_src);
    DEBUG_LOG_VERBOSE("  kymera_stopped_handler %d",usb_params_data.kymera_stopped_handler);
    DEBUG_LOG_VERBOSE("  max_latency_ms %d",usb_params_data.max_latency_ms);
    DEBUG_LOG_VERBOSE("  mic_sample_rate %d",usb_params_data.mic_sample_rate);
    DEBUG_LOG_VERBOSE("  mute_status %d",usb_params_data.mute_status);
    DEBUG_LOG_VERBOSE("  mic_sink %d",usb_params_data.mic_sink);
    DEBUG_LOG_VERBOSE("  min_latency_ms %d",usb_params_data.min_latency_ms);
    DEBUG_LOG_VERBOSE("  mode %d",usb_params_data.mode);
    DEBUG_LOG_VERBOSE("  spkr_channels %d",usb_params_data.spkr_channels);
    DEBUG_LOG_VERBOSE("  spkr_sample_rate %d",usb_params_data.spkr_sample_rate);
    DEBUG_LOG_VERBOSE("  target_latency_ms %d",usb_params_data.target_latency_ms);
    DEBUG_LOG_VERBOSE("  volume %d",usb_params_data.volume);

    VoiceSources_GetConnectParameters(voice_source_hfp_1, &source_params_hfp);
    voice_connect_parameters_t hfp_params_data = *(voice_connect_parameters_t*)source_params_hfp.data;

    switch(hfp_params_data.codec_mode)
    {
    case hfp_codec_mode_narrowband:
        sco_sample_rate = 8000;
        break;
    case hfp_codec_mode_wideband:
        sco_sample_rate = 16000;
        break;
    case hfp_codec_mode_super_wideband:
        sco_sample_rate = 32000;
        break;
    default:
        DEBUG_LOG_ERROR("UsbDongle_VoiceStart, invalid SCO sample rate");
        return;
    }
    DEBUG_LOG_INFO("UsbDongle_VoiceStart, starting USB SCO chain with codec enum:hfp_codec_mode_t:%d",
                   hfp_params_data.codec_mode);

    DEBUG_LOG_VERBOSE("UsbDongle_VoiceStart, hfp_params_data:");
    DEBUG_LOG_VERBOSE("  channels %d",hfp_params_data.codec_mode);
    DEBUG_LOG_VERBOSE("  audio_sink %d",hfp_params_data.audio_sink);
    DEBUG_LOG_VERBOSE("  pre_start_delay %d",hfp_params_data.pre_start_delay);
    DEBUG_LOG_VERBOSE("  tesco %d",hfp_params_data.tesco);
    DEBUG_LOG_VERBOSE("  wesco %d",hfp_params_data.wesco);
    DEBUG_LOG_VERBOSE("  volume %d",hfp_params_data.volume);

    KYMERA_INTERNAL_USB_SCO_VOICE_START_T message;

    message.mode = usb_params_data.mode;
    message.spkr_channels = usb_params_data.spkr_channels;
	message.spkr_frame_size = usb_params_data.spkr_frame_size;
    message.spkr_src = usb_params_data.spkr_src;
    message.mic_sink = usb_params_data.mic_sink;
    message.sco_sink = hfp_params_data.audio_sink;
    message.spkr_sample_rate = usb_params_data.spkr_sample_rate;
    message.mic_sample_rate =  usb_params_data.mic_sample_rate;
    message.mute_status = usb_params_data.mute_status;
    message.sco_sample_rate = sco_sample_rate;
    message.min_latency_ms = 30;
    message.max_latency_ms = 70;
    message.target_latency_ms = 50;

    DEBUG_LOG_VERBOSE("UsbDongle_VoiceStart, message.mic_snk %p", message.mic_sink);
    DEBUG_LOG_VERBOSE("UsbDongle_VoiceStart, message.sco_snk %p", message.sco_sink);
    DEBUG_LOG_INFO("UsbDongle_VoiceStart, USB audio sample rate %d", message.spkr_sample_rate);
    DEBUG_LOG_INFO("UsbDongle_VoiceStart, USB mic input sample rate %d", message.mic_sample_rate);
    DEBUG_LOG_INFO("UsbDongle_VoiceStart, SCO sample rate %d", message.sco_sample_rate);

    voice_data.sco_sink = hfp_params_data.audio_sink;

    VoiceSources_ReleaseConnectParameters(voice_source_usb, &source_params_usb);
    VoiceSources_ReleaseConnectParameters(voice_source_hfp_1, &source_params_hfp);

    KymeraUsbScoVoice_Start(&message);
}

void UsbDongle_VoiceStop(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_VoiceStop");

    source_defined_params_t source_params_usb;

    if (!usbDongleConfig_IsInBredrMode() && !usbDongleConfig_IsInDualModeWithBredrConnected())
    {
        return;
    }

    if (!VoiceSources_GetDisconnectParameters(voice_source_usb, &source_params_usb))
    {
        return;
    }

    usb_voice_disconnect_parameters_t usb_params_data = *(usb_voice_disconnect_parameters_t *)source_params_usb.data;

    KYMERA_INTERNAL_USB_SCO_VOICE_STOP_T message;

    message.kymera_stopped_handler = usb_params_data.kymera_stopped_handler;
    message.mic_sink = usb_params_data.mic_sink;
    message.sco_sink = voice_data.sco_sink;
    message.spkr_src = usb_params_data.spkr_src;

    KymeraUsbScoVoice_Stop(&message);

    VoiceSources_ReleaseDisconnectParameters(voice_source_usb, &source_params_usb);
}

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

void UsbDongle_VoiceRestartGraph(void)
{
    /* first check that kymera has some chain running */
    if (Kymera_IsIdle())
    {
        DEBUG_LOG("UsbDongle_VoiceRestartGraph, kymera is idle, cannot restart graph");
        return;
    }

    DEBUG_LOG_INFO("UsbDongle_VoiceRestartGraph, restarting voice graph");

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        /* Stop and restart USB-voice graph synchronously */
        UsbDongle_VoiceStop();
        UsbDongle_VoiceStart();
    }

#endif  /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        UsbDongle_LeaRestartAudioGraph(TRUE);
    }

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

}

bool UsbDongle_VoiceIsSourceAvailable(void)
{
    bool is_src_available = FALSE;

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        is_src_available = UsbDongle_HfpInConnectedState();
    }

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        is_src_available = UsbDongle_LeaIsVoiceAvailable();
    }

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    return is_src_available;
}

bool UsbDongle_VoiceIsSourceConnected(void)
{
    bool is_src_connected = FALSE;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        is_src_connected = UsbDongle_VoiceIsSourceAvailable();
    }

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        is_src_connected = UsbDongle_LeaIsVoiceConnected();
    }

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    return is_src_connected;
}

void UsbDongle_VoiceConnect(const bdaddr *sink_addr)
{
#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        UsbDongle_HfpConnect(sink_addr);
    }

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        UNUSED(sink_addr);
        Panic();
    }

#else /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    UNUSED(sink_addr);

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
}

bool UsbDongle_VoiceStreamConnect(void)
{
#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        /* Start a call with the sink device. This will initiate a SCO connection,
           if there isn't one already. */
        UsbDongle_HfpOutgoingCall();

        if (VoiceSources_IsVoiceChannelAvailable(voice_source_hfp_1))
        {
            /* Pre-existing SCO was already connected, can immediately "answer" the
               call and skip straight to routing audio. */
            UsbDongle_HfpAnswerOutgoingCall();
            return TRUE;
        }
    }

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        UsbDongle_LeVoiceOutgoingCall();
        UsbDongle_LeaAudioStart();
    }

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    return FALSE;
}


bool UsbDongle_VoiceStreamDisconnect(void)
{
    /* Stop call with the sink device. This will initiate SCO disconnection in case of HFP,
       if there is currently one connected. In case of LE voice, this will terminate ongoing 
       call activity. */
    VoiceSources_TerminateOngoingCall(UsbDongle_VoiceGetCurrentVoiceSource());

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        /* Release Held call if present */
        UsbDongle_HfpReleaseHeldCall();

        if (AghfpProfile_IsScoDisconnectedForInstance(hfp_data.active_instance))
        {
            /* SCO was already disconnected, skip straight to rescanning inputs to
               decide the next state / appropriate action. */
            return TRUE;
        }
    }

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        UsbDongle_LeaAudioStop();
    }

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    return FALSE;
}

voice_source_t UsbDongle_VoiceGetCurrentVoiceSource(void)
{
    voice_source_t voice_source = voice_source_none;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        voice_source = voice_source_hfp_1;
    }

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        voice_source = voice_source_le_audio_unicast_1;
    }

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    return voice_source;
}

bool UsbDongle_VoiceIsCallActive(void)
{
    bool is_call_active = FALSE;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        is_call_active = (hfp_data.state == APP_HFP_STATE_INCOMING);
    }

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        is_call_active = (le_voice_data.state == APP_LE_VOICE_STATE_INCOMING);
    }

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    return is_call_active;
}

void UsbDongle_VoiceIncomingVoiceCall(void)
{

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        UsbDongle_HfpIncomingVoiceCall();
    }

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        UsbDongle_LeVoiceIncomingCall();
    }

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
}
