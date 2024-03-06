/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      USB dongle LE audio interface.

*/

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

#include "usb_dongle_logging.h"
#include "unexpected_message.h"
#include "audio_sources.h"
#include <voice_sources.h>
#include <kymera_adaptation_voice_protected.h>
#include <kymera_adaptation_audio_protected.h>
#include <kymera_usb_le_audio.h>
#include <kymera_analog_le_audio.h>

#include "usb_dongle_config.h"
#include "usb_dongle_lea.h"
#include "usb_dongle_lea_config.h"
#include "usb_dongle_le_voice.h"
#include "le_audio_client.h"

#include <device_types.h>
#include <ui.h>
#include <panic.h>
#include <local_addr.h>
#include <stdio.h>

#define usbDongle_IsLeaUnicastEnabled()                             (!usbDongleConfig_IsInBroadcastAudioMode())
#define usbDongle_IsLeAudioClientInUnicastStreaming()               (LeAudioClient_IsUnicastStreamingActive(usb_dongle_lea_data.group_handle))
#define usbDongle_IsLeAudioClientInBroadcastStreaming()             (LeAudioClient_IsBroadcastSourceStreamingActive())
#define usbDongle_IsLeAudioClientInStreaming()                      (usbDongle_IsLeAudioClientInUnicastStreaming() || \
                                                                     usbDongle_IsLeAudioClientInBroadcastStreaming())
/* Check if there is any active connection */
#define usbDongle_IsLeaConnected()                                  (usb_dongle_lea_data.group_handle != 0)

#define USB_LEA_AUDIO_PROCESSING_LATENCY_THRESHOLD_US                   (5000)
#define USB_LEA_AUDIO_PROCESSING_TIME_FOR_LOW_LATENCY_MODE_US           (19000)
#define USB_LEA_AUDIO_PROCESSING_TIME_FOR_MUSIC_MODE_US                 (25000)
#define USB_LEA_AUDIO_PROCESSING_TIME_FOR_ULL_MODE_US                   (9900)
#define USB_LEA_AUDIO_PROCESSING_TIME_FOR_ULL_MODE_MONO_US              (12340)
#define USB_LEA_AUDIO_PROCESSING_TIME_FOR_APTX_ADAPTIVE_MONO_US         (30000)


/*! \brief Delay in ms before we process the context change.
           Note: Recommended to keep this >= USB_DONGLE_SM_RESCAN_INPUTS_DELAY
 */
#define USB_DONGLE_LEA_PROCESS_CONTEXT_CHANGE_DELAY                     (50)

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! \brief Is USB dongle is in unicast streaming */
#define usbDongle_IsInLeaUnicastStreaming()     (usbDongle_LeaGetState() == APP_LEA_STATE_UNICAST_STREAMING_STARTING || \
                                                 usbDongle_LeaGetState() == APP_LEA_STATE_UNICAST_STREAMING)
#else

/*! \brief Is USB dongle is in unicast streaming */
#define usbDongle_IsInLeaUnicastStreaming()     FALSE

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

/*! \brief Is USB dongle is in broadcast streaming */
#define usbDongle_IsInLeaBroadcastStreaming()   (usbDongle_LeaGetState() == APP_LEA_STATE_BROADCAST_STREAMING_STARTING || \
                                                 usbDongle_LeaGetState() == APP_LEA_STATE_BROADCAST_STREAMING)

#else

/*! \brief Is USB dongle is in broadcast streaming */
#define usbDongle_IsInLeaBroadcastStreaming()     FALSE

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

typedef enum
{
    /* Idle or disconnected state */
    APP_LEA_STATE_DISCONNECTED = 0,

    /* LE Connected with a sink device */
    APP_LEA_STATE_CONNECTED,

    /* Unicast streaming is starting */
    APP_LEA_STATE_UNICAST_STREAMING_STARTING,

    /* Unicast streaming */
    APP_LEA_STATE_UNICAST_STREAMING,

    /* Broadcast streaming is starting (with/without connection) */
    APP_LEA_STATE_BROADCAST_STREAMING_STARTING,

    /* Broadcast streaming (with/without connection) */
    APP_LEA_STATE_BROADCAST_STREAMING,

    /* Unicast/Broadcast streaming stopping */
    APP_LEA_STATE_STREAMING_STOPPING,
} usb_dongle_lea_state_t;

/*! \brief USB Dongle LEA data structure */
typedef struct
{
    TaskData                        task;                   /*!< Local task */
    ServiceHandle                   group_handle;           /*!< Group handle */
    uint8                           audio_context_mask;     /*!< Bitfield of LEA context */
    usb_dongle_lea_context_t        active_context;         /*!< Active context: VBC/Audio/Voice */
    usb_dongle_lea_context_t        requested_context;      /*!< Pending context, in progess */
    usb_dongle_lea_state_t          state;                  /*!< Current state of USB LEA dongle */
    bool                            is_request_in_progress; /*!< True if request to start/stop stream is in progress */
    void (*connected_cb)(void);                             /*!< Callback for connected state. */
    void (*disconnected_cb)(void);                          /*!< Callback for disconnected state */
    void (*usb_stopped_handler)(Source source);             /*!< Supplied as part of the USB disconnect params. Must be
                                                                 called once the USB LE audio chain has been stopped by kymera. */
} usb_dongle_lea_data_t;

/*! Start LEA Unicast streaming with requested stream type. */
static void usbDongle_LeaStartStreaming(void);

/*! To get requested audio context for LEA unicast client. */
static uint16 usbDongle_LeaGetRequestedAudioContext(void);

/*! \brief USB Dongle LEA data instance */
static usb_dongle_lea_data_t usb_dongle_lea_data;

#define usbDongle_LeaGetState() (usb_dongle_lea_data.state)

static unsigned usbDongle_LeaGetStateContext(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_GetAudioStateContext");
    audio_source_provider_context_t context = BAD_CONTEXT;

    switch(usbDongle_LeaGetState())
    {
        case APP_LEA_STATE_UNICAST_STREAMING:
        case APP_LEA_STATE_BROADCAST_STREAMING:
            context = context_audio_is_streaming;
            break;

        case APP_LEA_STATE_CONNECTED:
            context = context_audio_connected;
            break;

        case APP_LEA_STATE_DISCONNECTED:
            context = context_audio_disconnected;
            break;

        default:
            context = context_audio_connected;
            break;
     }

    return (unsigned)context;
}

bool UsbDongle_LeaIsSourceActive(void)
{
    return (usb_dongle_lea_data.active_context != USB_DONGLE_LEA_NONE);
}

bool UsbDongle_LeaIsAudioActive(void)
{
    return (usb_dongle_lea_data.active_context & (USB_DONGLE_LEA_AUDIO |
                                                  USB_DONGLE_LEA_ANALOG_AUDIO |
                                                  USB_DONGLE_LEA_AUDIO_VBC)) ? TRUE : FALSE;
}

bool UsbDongle_LeaIsAnalogAudioActive(void)
{
    return (usb_dongle_lea_data.active_context == USB_DONGLE_LEA_ANALOG_AUDIO);
}

bool UsbDongle_LeaIsVbcActive(void)
{
    return (usb_dongle_lea_data.active_context == USB_DONGLE_LEA_AUDIO_VBC);
}

bool UsbDongle_LeaIsVoiceActive(void)
{
    return (usb_dongle_lea_data.active_context == USB_DONGLE_LEA_VOICE);
}

static bool usbDongle_LeaIsStreamRequested(void)
{
    return (usb_dongle_lea_data.requested_context != USB_DONGLE_LEA_NONE);
}

static usb_dongle_lea_context_t usbDongle_LeaDetermineNewContext(void)
{
    if(usb_dongle_lea_data.audio_context_mask & USB_DONGLE_LEA_VOICE)
    {
        return USB_DONGLE_LEA_VOICE;
    }
    else if(usb_dongle_lea_data.audio_context_mask & USB_DONGLE_LEA_AUDIO_VBC)
    {
        return USB_DONGLE_LEA_AUDIO_VBC;
    }
    else if(usb_dongle_lea_data.audio_context_mask & USB_DONGLE_LEA_ANALOG_AUDIO)
    {
        return USB_DONGLE_LEA_ANALOG_AUDIO;
    }
    else if(usb_dongle_lea_data.audio_context_mask & USB_DONGLE_LEA_AUDIO)
    {
        return USB_DONGLE_LEA_AUDIO;
    }

    return USB_DONGLE_LEA_NONE;
}

static bool usbDongle_LeaFocusedContextIsAudio(void)
{
    return (usbDongle_LeaDetermineNewContext() & (USB_DONGLE_LEA_AUDIO | USB_DONGLE_LEA_ANALOG_AUDIO)) ? TRUE : FALSE;
}

static void usbDongle_LeaEnterDisconnected(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaEnterDisconnected");
}

static void usbDongle_LeaEnterConnected(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaEnterConnected");
}

static void usbDongle_LeaEnterStreamingStarting(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaEnterStreamingStarting");

    PanicFalse(usb_dongle_lea_data.requested_context == USB_DONGLE_LEA_NONE ||
               usbDongle_IsInLeaBroadcastStreaming());

    usb_dongle_lea_data.requested_context = usbDongle_LeaDetermineNewContext();

    usbDongle_LeaStartStreaming();
}

static void usbDongle_LeaEnterAudioStreaming(usb_dongle_lea_state_t state)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaEnterAudioStreaming unicast_mode: %d", state == APP_LEA_STATE_UNICAST_STREAMING);
}

static void usbDongle_LeaEnterStreamingStopping(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaEnterStreamingStopping");
    UsbDongle_LeaAudioStop();
}

/* This function is called to change the applications state, it automatically
   calls the entry and exit functions for the new and old states.
*/
static void usbDongle_LeaSetState(usb_dongle_lea_state_t new_state)
{
    DEBUG_LOG_STATE("usbDongle_LeaSetState, enum:usb_dongle_lea_state_t:%d -> "
                    "enum:usb_dongle_lea_state_t:%d", usbDongle_LeaGetState(), new_state);

    /* Set the new state */
    usb_dongle_lea_data.state = new_state;

    /* Handle state entry functions */
    switch (new_state)
    {
        case APP_LEA_STATE_DISCONNECTED:
            usbDongle_LeaEnterDisconnected();
        break;

        case APP_LEA_STATE_CONNECTED:
            usbDongle_LeaEnterConnected();
        break;

        case APP_LEA_STATE_BROADCAST_STREAMING_STARTING:
        case APP_LEA_STATE_UNICAST_STREAMING_STARTING:
            usbDongle_LeaEnterStreamingStarting();
        break;

        case APP_LEA_STATE_UNICAST_STREAMING:
        case APP_LEA_STATE_BROADCAST_STREAMING:
            usbDongle_LeaEnterAudioStreaming(new_state);
        break;

        case APP_LEA_STATE_STREAMING_STOPPING:
            usbDongle_LeaEnterStreamingStopping();
        break;

        default:
            DEBUG_LOG_ERROR("usbDongle_LeaSetState, attempted to enter unsupported state "
                            "enum:usb_dongle_lea_state_t:0x%02x", new_state);
            Panic();
            break;
    }
}

static void usbDongle_LeaDongleSmProcessContextChange(void)
{
    MessageCancelAll(&usb_dongle_lea_data.task, USB_DONGLE_LEA_SM_INTERNAL_PROCESS_CONTEXT_CHANGE);
    MessageSendLater(&usb_dongle_lea_data.task, USB_DONGLE_LEA_SM_INTERNAL_PROCESS_CONTEXT_CHANGE, NULL,
                     USB_DONGLE_LEA_PROCESS_CONTEXT_CHANGE_DELAY);
}

/*! \brief Rescan available audio inputs.

    Connect/disconnect, suspend/resume, or switch as required.
*/
static void usbDongle_LeaSmProcessContextChangeHandler(void)
{
    usb_dongle_lea_context_t new_lea_context = usbDongle_LeaDetermineNewContext();

    DEBUG_LOG_DEBUG("usbDongle_LeaSmProcessContextChangeHandler broadcast_mode: %d, is_connected: %d,"
                    "new_lea_context enum:usb_dongle_lea_context_t:%d, lea_state enum:usb_dongle_lea_state_t:%d",
                    usbDongleConfig_IsInBroadcastAudioMode(), usbDongle_IsLeaConnected(), new_lea_context, usbDongle_LeaGetState());

    /* For unicast mode or connected device case, this is taken care by main dongle SM Handler */
    if (!usbDongleConfig_IsInBroadcastAudioMode() || usbDongle_IsLeaConnected())
    {
        return;
    }

    /* For broadcast mode, start/stop audio based on current state and new context */
    if (UsbDongle_LeaContextSwitchIsRequired())
    {
        /* In case of non colocated Broadcast, we need to stop broadcast stream when
           - Context is set to none (due to input source (USB/Line In) disconnection)
           - Context is changed (swicthing between sources like Voice, Line In <-> USB)
        */
        switch (usbDongle_LeaGetState())
        {
            case APP_LEA_STATE_BROADCAST_STREAMING_STARTING:
            case APP_LEA_STATE_BROADCAST_STREAMING:
                usbDongle_LeaSetState(APP_LEA_STATE_STREAMING_STOPPING);
            break;

            case APP_LEA_STATE_STREAMING_STOPPING:
                /* Still waiting for audio/voice chain to be torn down. */
                /* Do nothing. */
            break;

            default:
            break;
        }
    }
    else if(usbDongle_LeaFocusedContextIsAudio() && usbDongle_LeaGetState() == APP_LEA_STATE_DISCONNECTED)
    {
        /* Start the broadcast streaming as a new audio is available and we are in disconnected state */
        usbDongle_LeaSetState(APP_LEA_STATE_BROADCAST_STREAMING_STARTING);
    }
}

static uint32 usbDongle_LeaGetTargetLatency(uint16 frame_duration)
{
    uint32 target_latency = 0;

    switch (frame_duration)
    {
        case LE_LC3_10_MS_FRAME_DURATION:
        {
            target_latency = USB_LEA_AUDIO_PROCESSING_TIME_FOR_MUSIC_MODE_US;
        }
        break;

        case LE_LC3_7P5_MS_FRAME_DURATION:
        {
            target_latency = USB_LEA_AUDIO_PROCESSING_TIME_FOR_LOW_LATENCY_MODE_US;
        }
        break;

        case LE_APTX_LITE_DEFAULT_FRAME_DURATION:
        {
            target_latency = USB_LEA_AUDIO_PROCESSING_TIME_FOR_ULL_MODE_US;
        }
        break;

        case LE_APTX_LITE_FRAME_DURATION_6P25:
        {
            target_latency = USB_LEA_AUDIO_PROCESSING_TIME_FOR_ULL_MODE_MONO_US;
        }
        break;

        case LE_APTX_ADAPTIVE_FRAME_DURATION:
        {
            target_latency = USB_LEA_AUDIO_PROCESSING_TIME_FOR_APTX_ADAPTIVE_MONO_US;
        }
        break;

        default:
            DEBUG_LOG_ERROR("usbDongle_LeaGetTargetLatency, Unknown Frame duration %d", frame_duration);
        break;
    }

    DEBUG_LOG_VERBOSE(" usbDongle_LeaGetTargetLatency target_latency %d", target_latency);

    return target_latency;
}

static bool usbDongle_GetWiredSourceConnectParams(KYMERA_INTERNAL_ANALOG_LE_AUDIO_START_T *message, bool is_voice, uint16 frame_duration)
{
    source_defined_params_t wired_src_params = {0};

    PanicNotZero(is_voice);

    bool status = AudioSources_GetConnectParameters(audio_source_line_in, &wired_src_params);
    if (status)
    {
        wired_analog_connect_parameters_t *audio_wired_params = (wired_analog_connect_parameters_t *) wired_src_params.data;

        PanicNull(audio_wired_params);
        message->sample_rate = audio_wired_params->rate;

        uint32 target_latency = usbDongle_LeaGetTargetLatency(frame_duration);
        message->min_latency_us = target_latency - USB_LEA_AUDIO_PROCESSING_LATENCY_THRESHOLD_US;
        message->max_latency_us = target_latency + USB_LEA_AUDIO_PROCESSING_LATENCY_THRESHOLD_US;
        message->target_latency_us = target_latency;
    }
    else
    {
        DEBUG_LOG_ERROR("usbDongle_GetWiredSourceConnectParams, failed");
        Panic();
    }

    AudioSources_ReleaseConnectParameters(audio_source_line_in, &wired_src_params);

    DEBUG_LOG_VERBOSE("usbDongle_GetWiredSourceConnectParams, sample rate: %u, target latency: %u us",
                                                            message->sample_rate, message->target_latency_us);

    return status;
}

static bool usbDongle_GetUsbSourceConnectParam(KYMERA_INTERNAL_USB_LE_AUDIO_START_T *message, bool is_voice, uint16 frame_duration)
{
    uint32 target_latency = usbDongle_LeaGetTargetLatency(frame_duration);
    source_defined_params_t usb_src_params = {0};
    bool status;

    message->min_latency_us = target_latency - USB_LEA_AUDIO_PROCESSING_LATENCY_THRESHOLD_US;
    message->max_latency_us = target_latency + USB_LEA_AUDIO_PROCESSING_LATENCY_THRESHOLD_US;
    message->target_latency_us = target_latency;

    if (!is_voice)
    {
        status = AudioSources_GetConnectParameters(audio_source_usb, &usb_src_params);
        if (status)
        {
            usb_audio_connect_parameters_t *audio_usb_params = (usb_audio_connect_parameters_t *) usb_src_params.data;

            PanicNull(audio_usb_params);
            message->spkr_channels = audio_usb_params->channels;
            message->spkr_frame_size = audio_usb_params->frame_size;
            message->spkr_src = audio_usb_params->spkr_src;
            message->mic_sink = audio_usb_params->mic_sink;
            message->spkr_sample_rate = audio_usb_params->sample_freq;
            message->mic_sample_rate =  audio_usb_params->sample_freq;
        }
        else
        {
            DEBUG_LOG_ERROR("usbDongle_GetUsbAudioSourceConnectParam, can't start USB LE chain, USB Speaker interface is not selected");
        }

        AudioSources_ReleaseConnectParameters(audio_source_usb, &usb_src_params);
    }
    else
    {
        status = VoiceSources_GetConnectParameters(voice_source_usb, &usb_src_params);
        if (status)
        {
            usb_voice_connect_parameters_t *voice_usb_params = (usb_voice_connect_parameters_t *) usb_src_params.data;

            PanicNull(voice_usb_params);
            message->spkr_channels = voice_usb_params->spkr_channels;
            message->spkr_frame_size = voice_usb_params->spkr_frame_size;
            message->spkr_src = voice_usb_params->spkr_src;
            message->mic_sink = voice_usb_params->mic_sink;
            message->mute_status = voice_usb_params->mute_status;
            message->spkr_sample_rate = voice_usb_params->spkr_sample_rate;
            message->mic_sample_rate =  voice_usb_params->mic_sample_rate;
        }
        else
        {
            DEBUG_LOG_ERROR("usbDongle_GetUsbAudioSourceConnectParam, can't start USB LE chain, USB Mic interface is not selected");
        }

        VoiceSources_ReleaseConnectParameters(voice_source_usb, &usb_src_params);
    }

    DEBUG_LOG_VERBOSE("usbDongle_GetUsbAudioSourceConnectParam spkr (channels: %d, frame_size: 0x%x, sample_rate: %u) "
                          "mic (sample_rate %u, Mute: %d), target latency: %u us",
                          message->spkr_channels, message->spkr_frame_size, message->spkr_sample_rate,
                          message->mic_sample_rate, message->mute_status, message->target_latency_us);

    return status;
}

/*! \brief Get the audio source to use based on current mode */
static audio_source_t usbDongle_LeaGetAudioSource(void)
{
    return LeAudioClient_IsInUnicastMode() ? audio_source_le_audio_unicast_sender :
                                             audio_source_le_audio_broadcast_sender;
}

static bool usbDongle_LeaStartGraph(bool enable_vbc, bool is_restart)
{
    source_defined_params_t source_params_lea;
    le_audio_connect_parameters_t *lea_params_data;

    DEBUG_LOG_FN_ENTRY("usbDongle_LeaStartGraph, requested: enum:usb_dongle_lea_context_t:%u, enable_vbc: %d",
                                                    usb_dongle_lea_data.requested_context, enable_vbc);

    /* If start graph is called as part of restarting the graph, then do not check for
       active source as it will be active */
    if (!is_restart && UsbDongle_LeaIsSourceActive())
    {
        DEBUG_LOG_ERROR("usbDongle_LeaStartGraph, can't start USB chain, audio already running, active_source %d", usb_dongle_lea_data.active_context);
        return FALSE;
    }

    if (usb_dongle_lea_data.requested_context == USB_DONGLE_LEA_VOICE)
    {
        VoiceSources_GetConnectParameters(voice_source_le_audio_unicast_1, &source_params_lea);
    }
    else
    {
        AudioSources_GetConnectParameters(usbDongle_LeaGetAudioSource(), &source_params_lea);
    }

    PanicNull(source_params_lea.data);
    lea_params_data = source_params_lea.data;

    DEBUG_LOG_VERBOSE("usbDongle_LeaStartGraph, LEA params: microphone_present %d, media_present %d",
                      lea_params_data->microphone_present, lea_params_data->media_present);

    switch (usb_dongle_lea_data.requested_context)
    {
        case USB_DONGLE_LEA_AUDIO:
        case USB_DONGLE_LEA_AUDIO_VBC:
        case USB_DONGLE_LEA_VOICE:
        {
            KYMERA_INTERNAL_USB_LE_AUDIO_START_T message;
            memset(&message, 0, sizeof(message));

            message.vbc_enabled = lea_params_data->microphone_present;
            message.to_air_params = lea_params_data->media;
            message.from_air_params = lea_params_data->microphone;
            message.qhs_level = LeAudioClient_GetQhsLevel();
            usbDongle_GetUsbSourceConnectParam(&message, enable_vbc, lea_params_data->media.frame_duration);
            KymeraUsbLeAudio_Start(&message);
        }
        break;

        case USB_DONGLE_LEA_ANALOG_AUDIO:
        {
            KYMERA_INTERNAL_ANALOG_LE_AUDIO_START_T message;
            message.to_air_params = lea_params_data->media;
            usbDongle_GetWiredSourceConnectParams(&message, enable_vbc, lea_params_data->media.frame_duration);
            KymeraAnalogLeAudio_Start(&message);
        }
        break;

        default:
            Panic();
    }

    if (usb_dongle_lea_data.requested_context == USB_DONGLE_LEA_VOICE)
    {
        VoiceSources_ReleaseConnectParameters(voice_source_le_audio_unicast_1, &source_params_lea);
    }
    else
    {
        AudioSources_ReleaseConnectParameters(usbDongle_LeaGetAudioSource(), &source_params_lea);
    }

    return TRUE;
}

static bool usbDongle_GetUsbSourceDisconnectParams(KYMERA_INTERNAL_USB_LE_AUDIO_STOP_T *message, bool is_voice)
{
    source_defined_params_t usb_src_params = {0};
    bool status;

    if (!is_voice)
    {
        status = AudioSources_GetDisconnectParameters(audio_source_usb, &usb_src_params);
        if (status)
        {
            usb_audio_disconnect_parameters_t *audio_usb_params = (usb_audio_disconnect_parameters_t *) usb_src_params.data;

            PanicNull(audio_usb_params);
            usb_dongle_lea_data.usb_stopped_handler = audio_usb_params->kymera_stopped_handler;

            message->kymera_stopped_handler = audio_usb_params->kymera_stopped_handler;
            message->mic_sink = audio_usb_params->sink;
            message->spkr_src = audio_usb_params->source;
        }
        else
        {
            DEBUG_LOG_ERROR("usbDongle_GetUsbSourceDisconnectParams, can't stop USB LE Audio chain");
        }

        AudioSources_ReleaseDisconnectParameters(audio_source_usb, &usb_src_params);
    }
    else
    {
        status = VoiceSources_GetDisconnectParameters(voice_source_usb, &usb_src_params);
        if (status)
        {
            usb_voice_disconnect_parameters_t *voice_usb_params = (usb_voice_disconnect_parameters_t *) usb_src_params.data;

            PanicNull(voice_usb_params);
            usb_dongle_lea_data.usb_stopped_handler = voice_usb_params->kymera_stopped_handler;

            message->kymera_stopped_handler = voice_usb_params->kymera_stopped_handler;
            message->mic_sink = voice_usb_params->mic_sink;
            message->spkr_src = voice_usb_params->spkr_src;
        }
        else
        {
            DEBUG_LOG_ERROR("usbDongle_GetUsbSourceDisconnectParams, can't stop USB LE Voice chain");
        }

        VoiceSources_ReleaseDisconnectParameters(voice_source_usb, &usb_src_params);
    }

    return status;
}

/*! \brief Stop USB LE audio chain */
static void usbDongle_LeAudioStopGraph(void)
{
    if (!UsbDongle_LeaIsSourceActive())
    {
        DEBUG_LOG_WARN("usbDongle_LeAudioStopGraph, attempted to stop USB LE audio chain when not running");
        return;
    }

    switch (usb_dongle_lea_data.active_context)
    {
        case USB_DONGLE_LEA_AUDIO:
        case USB_DONGLE_LEA_AUDIO_VBC:
        case USB_DONGLE_LEA_VOICE:
        {
            DEBUG_LOG_INFO("usbDongle_LeAudioStopGraph, stopping USB LEA Chain");

            KYMERA_INTERNAL_USB_LE_AUDIO_STOP_T message;

            bool is_voice = !!(usb_dongle_lea_data.active_context & (USB_DONGLE_LEA_AUDIO_VBC | USB_DONGLE_LEA_VOICE));

            if (!usbDongle_GetUsbSourceDisconnectParams(&message, is_voice))
            {
                DEBUG_LOG_ERROR("usbDongle_LeAudioStopGraph, failed to disconnect USB LEA Chain");
                return;
            }

            KymeraUsbLeAudio_Stop(&message);
        }
        break;

        case USB_DONGLE_LEA_ANALOG_AUDIO:
        {
            DEBUG_LOG_INFO("usbDongle_LeAudioStopGraph, stopping Wired LE Audio Chain");

            KymeraAnalogLeAudio_Stop();
        }
        break;

        default:
        {
            DEBUG_LOG_INFO("usbDongle_LeAudioStopGraph, invalid context: "
                           "enum:usb_dongle_lea_context_t:%d", usb_dongle_lea_data.active_context);
        }
        break;
    }
}

static void usbDongle_LeaHandleConnected(LE_AUDIO_CLIENT_CONNECT_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaHandleConnected");

    if (msg->status == LE_AUDIO_CLIENT_STATUS_SUCCESS)
    {
        usb_dongle_lea_data.group_handle = msg->group_handle;

        /* Todo: Separate connected and streaming states */
        if (usbDongle_LeaGetState() == APP_LEA_STATE_DISCONNECTED)
        {
            usbDongle_LeaSetState(APP_LEA_STATE_CONNECTED);
        }

        if(usb_dongle_lea_data.connected_cb != NULL)
        {
            usb_dongle_lea_data.connected_cb();
        }

        Ui_InformContextChange(ui_provider_media_player, context_audio_connected);
    }
}

static void usbDongle_LeaHandleDisconnected(LE_AUDIO_CLIENT_DISCONNECT_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaHandleDisConnected");

    UNUSED(msg);

    /* If ACL disconnected while LE Audio streaming, LE_AUDIO_CLIENT_STREAM_STOP_IND may not be received
     * Need to stop audio chain, if not already stopped */
    if (usb_dongle_lea_data.active_context != USB_DONGLE_LEA_NONE)
    {
        DEBUG_LOG_STATE("usbDongle_LeaHandleDisconnected, audio chain active, "
                        "enum:usb_dongle_lea_context_t:%d", usb_dongle_lea_data.active_context);
        /* Stop Audio Chain */
        usbDongle_LeAudioStopGraph();
        usb_dongle_lea_data.active_context = USB_DONGLE_LEA_NONE;
    }

    usb_dongle_lea_data.group_handle = 0;

    if (usbDongle_LeaGetState() == APP_LEA_STATE_CONNECTED ||
        usbDongle_LeaGetState() == APP_LEA_STATE_UNICAST_STREAMING_STARTING ||
        usbDongle_LeaGetState() == APP_LEA_STATE_UNICAST_STREAMING)
    {
        usbDongle_LeaSetState(APP_LEA_STATE_DISCONNECTED);
    }

    usb_dongle_lea_data.is_request_in_progress = FALSE;
    usb_dongle_lea_data.requested_context = USB_DONGLE_LEA_NONE;

    if(usb_dongle_lea_data.disconnected_cb != NULL)
    {
        usb_dongle_lea_data.disconnected_cb();
    }

    Ui_InformContextChange(ui_provider_media_player, usbDongle_LeaGetStateContext());
}

static void usbDongle_LeaHandleStreamDisconnected(LE_AUDIO_CLIENT_STREAM_STOP_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaHandleStreamDisconnected");

    UNUSED(msg);
    PanicFalse(msg->group_handle == usb_dongle_lea_data.group_handle);

    /* Stop audio, if not already stopped */
    if (usb_dongle_lea_data.active_context != USB_DONGLE_LEA_NONE)
    {
        /* Stop Audio Chain */
        usbDongle_LeAudioStopGraph();
    }

    usb_dongle_lea_data.is_request_in_progress = FALSE;

    /* In broadcast, stop streaming can be get called when the state is not in streaming or
       connected. Eg. If a context change happened after a start broadcast stream request.
       In such cases also we need to inform the UI module. */
    if (usbDongle_LeaGetState() >= APP_LEA_STATE_CONNECTED ||
       (!LeAudioClient_IsInUnicastMode() && !usbDongle_IsLeaConnected()))
    {
        /* Move the state to connected only if there is a connection */
        usbDongle_LeaSetState(usbDongle_IsLeaConnected() ? APP_LEA_STATE_CONNECTED : APP_LEA_STATE_DISCONNECTED);

        if (usb_dongle_lea_data.active_context != USB_DONGLE_LEA_VOICE)
        {
            Ui_InformContextChange(ui_provider_media_player, usbDongle_LeaGetStateContext());
        }
        else
        {
            UsbDongle_LeVoiceCallEnded();
        }
    }

    usb_dongle_lea_data.active_context = USB_DONGLE_LEA_NONE;

    if(usb_dongle_lea_data.requested_context != USB_DONGLE_LEA_NONE)
    {
        DEBUG_LOG_STATE("usbDongle_LeaHandleStreamDisconnected, there is a pending request to connect, "
                        "enum:usb_dongle_lea_context_t:%d", usb_dongle_lea_data.requested_context);
        usbDongle_LeaStartStreaming();
    }
    else if (usbDongleConfig_IsInBroadcastAudioMode() && !usbDongle_IsLeaConnected() &&
             usbDongle_LeaFocusedContextIsAudio())
    {
        /* In case of non collocated broadcast, ensure to start broadcast when we have a focused audio source */
        usbDongle_LeaSetState(APP_LEA_STATE_BROADCAST_STREAMING_STARTING);
    }
}

static void usbDongle_LeaHandleStreamConnected(LE_AUDIO_CLIENT_STREAM_START_IND_T *msg)
{
    bool result = FALSE;
    uint16 requested_audio_context = usbDongle_LeaGetRequestedAudioContext();

    DEBUG_LOG_FN_ENTRY("usbDongle_LeaHandleStreamConnected, connected audio_context 0x%x requested_audio_context 0x%x",
                       msg->audio_context, requested_audio_context);

    PanicFalse(msg->group_handle == usb_dongle_lea_data.group_handle);

    if(msg->status != LE_AUDIO_CLIENT_STATUS_SUCCESS)
    {
        DEBUG_LOG_ERROR("usbDongle_LeaHandleStreamConnected, unicast stream start failed");
        usb_dongle_lea_data.is_request_in_progress = FALSE;
        return;
    }

    /* Its possible that requested_context could be changed after calling
     * LeAudioClient_StartStreaming. If usb_dongle_lea_data.requested_context
     * doesn't match with stream_type, need to reconfigure unicast stream.*/
    if(msg->audio_context != requested_audio_context)
    {
        DEBUG_LOG_WARN("usbDongle_LeaHandleStreamConnected, LE context changed.");
        if(!LeAudioClient_StopStreaming(usb_dongle_lea_data.group_handle, TRUE))
        {
            DEBUG_LOG_ERROR("usbDongle_LeaHandleStreamConnected, LeAudioClient_StopStreaming Failed");
            usb_dongle_lea_data.is_request_in_progress = FALSE;
        }
        return;
    }

    if(usb_dongle_lea_data.requested_context != usbDongle_LeaDetermineNewContext())
    {
        DEBUG_LOG_WARN("usbDongle_LeaHandleStreamConnected, LE context changed, chain not started");
        usb_dongle_lea_data.is_request_in_progress = FALSE;
        return;
    }

    /* In broadcast, it is possible that LE_AUDIO_CLIENT_STREAM_START_IND is getting received twice.
       Eg. First when broadcast source only streaming starts. Second when gets connnected with sink and
       source gets added */
    if (usbDongle_LeaGetState() == APP_LEA_STATE_BROADCAST_STREAMING &&
        usb_dongle_lea_data.requested_context == usb_dongle_lea_data.active_context)
    {
        /* Already in broadcast steaming state. No need to start the graph again. Just
           inform the UI module */
        DEBUG_LOG_INFO("usbDongle_LeaHandleStreamConnected, streaming already");

        usb_dongle_lea_data.is_request_in_progress = FALSE;
        Ui_InformContextChange(ui_provider_media_player, usbDongle_LeaGetStateContext());
        return;
    }

    switch(usb_dongle_lea_data.requested_context)
    {
        case USB_DONGLE_LEA_VOICE:
        case USB_DONGLE_LEA_AUDIO_VBC:
        {
            result = usbDongle_LeaStartGraph(TRUE, FALSE);
            break;
        }

        case USB_DONGLE_LEA_AUDIO:
        case USB_DONGLE_LEA_ANALOG_AUDIO:
        {
            result = usbDongle_LeaStartGraph(FALSE, FALSE);
            break;
        }

        default:
        {
            DEBUG_LOG_ERROR("usbDongle_LeaHandleStreamConnected, LE context not supported, "
                            "enum:usb_dongle_lea_context_t:%d", usb_dongle_lea_data.requested_context);
            Panic();
        }
    }

    usb_dongle_lea_data.is_request_in_progress = FALSE;
    if (result)
    {
        usbDongle_LeaSetState(msg->source.type == source_type_audio && msg->source.u.audio == audio_source_le_audio_broadcast_sender ?
                                APP_LEA_STATE_BROADCAST_STREAMING : APP_LEA_STATE_UNICAST_STREAMING);
        usb_dongle_lea_data.active_context = usb_dongle_lea_data.requested_context;

        if (usb_dongle_lea_data.active_context == USB_DONGLE_LEA_VOICE)
        {
            UsbDongle_LeVoiceCallStart();
        }
        else
        {
            Ui_InformContextChange(ui_provider_media_player, usbDongle_LeaGetStateContext());
        }
    }
    else
    {
        DEBUG_LOG_ERROR("usbDongle_LeaHandleStreamConnected, failed to start LE audio chain");
    }
}

static void usbDongle_LeaHandleStreamStartCancelComplete(LE_AUDIO_CLIENT_STREAM_START_CANCEL_COMPLETE_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaHandleStreamStartCancelComplete, status:%d", msg->status);

    PanicFalse(msg->group_handle == usb_dongle_lea_data.group_handle);

    usb_dongle_lea_data.is_request_in_progress = FALSE;
    if(usbDongle_LeaGetState() >= APP_LEA_STATE_CONNECTED)
    {
        usbDongle_LeaSetState(APP_LEA_STATE_CONNECTED);

        /* Stream start got cancelled. Notify this to UI module. */
        if (usb_dongle_lea_data.active_context != USB_DONGLE_LEA_VOICE)
        {
            Ui_InformContextChange(ui_provider_media_player, usbDongle_LeaGetStateContext());
        }
        else
        {
            UsbDongle_LeVoiceCallEnded();
        }
    }
}

static void usbDongle_LeaHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG_FN_ENTRY("usbDongle_LeaHandleMessage %d", id);

    switch (id)
    {
        case LE_AUDIO_CLIENT_CONNECT_IND:
            usbDongle_LeaHandleConnected((LE_AUDIO_CLIENT_CONNECT_IND_T *) message);
            break;

        case LE_AUDIO_CLIENT_DISCONNECT_IND:
            usbDongle_LeaHandleDisconnected((LE_AUDIO_CLIENT_DISCONNECT_IND_T *) message);
            break;

        case LE_AUDIO_CLIENT_STREAM_START_IND:
            usbDongle_LeaHandleStreamConnected((LE_AUDIO_CLIENT_STREAM_START_IND_T *) message);
            break;

        case LE_AUDIO_CLIENT_STREAM_STOP_IND:
            usbDongle_LeaHandleStreamDisconnected((LE_AUDIO_CLIENT_STREAM_STOP_IND_T *) message);
        break;

        case LE_AUDIO_CLIENT_STREAM_START_CANCEL_COMPLETE_IND:
            usbDongle_LeaHandleStreamStartCancelComplete((LE_AUDIO_CLIENT_STREAM_START_CANCEL_COMPLETE_IND_T *) message);
        break;

        case USB_DONGLE_LEA_SM_INTERNAL_PROCESS_CONTEXT_CHANGE:
            usbDongle_LeaSmProcessContextChangeHandler();
        break;

        default:
            UnexpectedMessage_HandleMessage(id);
        break;

    }
}

/*! \brief Return TRUE if any of the given context is available */
static bool usbDongle_LeaIsContextAvailable(uint8 audio_contexts)
{
    return (usb_dongle_lea_data.audio_context_mask & audio_contexts) != 0;
}

void UsbDongle_LeaAddContext(usb_dongle_lea_context_t lea_ctx)
{
    DEBUG_LOG_STATE("UsbDongle_LeaAddCtx, enum:usb_dongle_lea_context_t:%d", lea_ctx);
    usb_dongle_lea_data.audio_context_mask |= lea_ctx;

    usbDongle_LeaDongleSmProcessContextChange();
}

void UsbDongle_LeaRemoveContext(usb_dongle_lea_context_t lea_ctx)
{
    DEBUG_LOG_STATE("UsbDongle_LeaRemoveCtx, enum:usb_dongle_lea_context_t:%d", lea_ctx);
    usb_dongle_lea_data.audio_context_mask &= ~lea_ctx;

    usbDongle_LeaDongleSmProcessContextChange();
}

usb_dongle_lea_context_t UsbDongle_LeaGetContext(void)
{
    return usb_dongle_lea_data.audio_context_mask;
}

bool UsbDongle_IsLeaBroadcastModeActive(void)
{
    return usbDongle_LeaGetState() == APP_LEA_STATE_BROADCAST_STREAMING;
}

/*! \brief Check if broadcast streaming needs to be started or not */
static bool usbDongle_IsLeaBroadcastModeRequired(void)
{
    /* If voice context is active, broadcast mode will be switched to high quality mode */
    return usbDongleConfig_IsInBroadcastAudioMode() &&
           (usbDongle_LeaDetermineNewContext() & (USB_DONGLE_LEA_AUDIO | USB_DONGLE_LEA_ANALOG_AUDIO));
}

static uint16 usbDongle_LeaGetRequestedAudioContext(void)
{
    uint16 audio_context = CAP_CLIENT_CONTEXT_TYPE_PROHIBITED;

    if (usbDongle_IsLeaBroadcastModeRequired())
    {
        /* In broadcast mode, we only support media context */
        if ((usb_dongle_lea_data.requested_context == USB_DONGLE_LEA_AUDIO) ||
            (usb_dongle_lea_data.requested_context == USB_DONGLE_LEA_ANALOG_AUDIO))
        {
            audio_context = CAP_CLIENT_CONTEXT_TYPE_MEDIA;
        }

        return audio_context;
    }

    switch(usb_dongle_lea_data.requested_context)
    {
        case USB_DONGLE_LEA_AUDIO_VBC:
        {
            audio_context = usbDongleConfig_IsInGamingAudioMode() ? CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC
                                                                  : CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL;
            break;
        }

        case USB_DONGLE_LEA_AUDIO:
        case USB_DONGLE_LEA_ANALOG_AUDIO:
        {
            audio_context = usbDongleConfig_IsInGamingAudioMode() ? CAP_CLIENT_CONTEXT_TYPE_GAME
                                                                  : CAP_CLIENT_CONTEXT_TYPE_MEDIA;
            break;
        }

        case USB_DONGLE_LEA_VOICE:
        {
            audio_context = CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL;
            break;
        }

        default:
        {
            audio_context = CAP_CLIENT_CONTEXT_TYPE_PROHIBITED;
            break;
        }
    }

    return audio_context;
}

static void usbDongle_LeaStartStreaming(void)
{
    uint16 audio_context = usbDongle_LeaGetRequestedAudioContext();

    if(audio_context == CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)
    {
        DEBUG_LOG_ERROR("usbDongle_LeaStartStreaming, LE Context not supported "
                        "enum:usb_dongle_lea_context_t:%d", usbDongle_LeaDetermineNewContext());
        Panic();
    }

    DEBUG_LOG_STATE("usbDongle_LeaStartStreaming, audio_context:0x%x", audio_context);

    if(usbDongle_IsLeaUnicastEnabled() && !LeAudioClient_IsUnicastConnected(usb_dongle_lea_data.group_handle))
    {
        DEBUG_LOG_ERROR("usbDongle_LeaStartStreaming, LEA unicast not connected");
        return;
    }
    else if(usbDongle_IsLeAudioClientInUnicastStreaming())
    {
        DEBUG_LOG_STATE("usbDongle_LeaStartStreaming, Unicast Streaming is active already");
        return;
    }
    else if(usbDongle_IsLeAudioClientInBroadcastStreaming())
    {
        DEBUG_LOG_STATE("usbDongle_LeaStartStreaming, Broadcast streaming is active already");
        return;
    }

    if(usb_dongle_lea_data.is_request_in_progress)
    {
        DEBUG_LOG_STATE("usbDongle_LeaStartStreaming, request in progress");
        return;
    }

    LeAudioClient_SetMode(usbDongle_IsLeaBroadcastModeRequired() ? LE_AUDIO_CLIENT_MODE_BROADCAST : LE_AUDIO_CLIENT_MODE_UNICAST);

    if(LeAudioClient_StartStreaming(usb_dongle_lea_data.group_handle, audio_context))
    {
        usb_dongle_lea_data.is_request_in_progress = TRUE;
    }

    DEBUG_LOG_STATE("usbDongle_LeaStartStreaming, audio_context:0x%x, status: %d", audio_context, usb_dongle_lea_data.is_request_in_progress);
}

/* Based on LEA context, LE Audio/VBC path will be started */
void UsbDongle_LeaAudioStart( void )
{
    if (usbDongle_LeaGetState() == APP_LEA_STATE_BROADCAST_STREAMING && usbDongle_IsLeaBroadcastModeRequired())
    {
        /* If we are already in broadcast streaming state, inform the main dongle SM via UI module */
        Ui_InformContextChange(ui_provider_media_player, context_audio_is_streaming);
    }
    else
    {
        /* Start the streaming */
        usbDongle_LeaSetState(usbDongle_IsLeaBroadcastModeRequired() ? APP_LEA_STATE_BROADCAST_STREAMING_STARTING
                                                                     : APP_LEA_STATE_UNICAST_STREAMING_STARTING);
    }
}

void UsbDongle_LeaAudioStop( void )
{
    DEBUG_LOG_STATE("UsbDongle_LeaAudioStop, enum:usb_dongle_lea_context_t:%d",
                    usb_dongle_lea_data.active_context);

    usb_dongle_lea_data.requested_context = USB_DONGLE_LEA_NONE;
    if(usb_dongle_lea_data.is_request_in_progress)
    {
        (void) LeAudioClient_StartStreamingCancelRequest(usb_dongle_lea_data.group_handle);
    }
    else if(usbDongle_IsLeAudioClientInStreaming())
    {
        if(LeAudioClient_StopStreaming(usb_dongle_lea_data.group_handle, TRUE))
        {
            usb_dongle_lea_data.is_request_in_progress = TRUE;
        }
        else
        {
            DEBUG_LOG_ERROR("UsbDongle_LeaAudioStop, LeAudioClient_StopStreaming failed");
            Panic();
        }
    }
    else
    {
        DEBUG_LOG_STATE("UsbDongle_LeaAudioStop streaming not active, "
                        "enum:usb_dongle_lea_state_t:%d, "
                        "enum:usb_dongle_lea_context_t:%d",
                        usbDongle_LeaGetState(), usb_dongle_lea_data.active_context);

        Ui_InformContextChange(ui_provider_media_player, usbDongle_LeaGetStateContext());
    }
}

bool UsbDongle_LeaIsAudioAvailable(void)
{
    uint8 allowed_context = USB_DONGLE_LEA_AUDIO | USB_DONGLE_LEA_ANALOG_AUDIO;

    if (!usbDongleConfig_IsInBroadcastAudioMode())
    {
        if (!LeAudioClient_IsUnicastConnected(usb_dongle_lea_data.group_handle))
        {
            return FALSE;
        }

        allowed_context |= USB_DONGLE_LEA_AUDIO_VBC;
    }

    return usbDongle_LeaIsContextAvailable(allowed_context);
}

bool UsbDongle_LeaIsVbcAvailable(void)
{
    return LeAudioClient_IsUnicastConnected(usb_dongle_lea_data.group_handle) &&
            (usb_dongle_lea_data.audio_context_mask & USB_DONGLE_LEA_AUDIO_VBC);
}

bool UsbDongle_LeaIsVoiceConnected(void)
{
    return LeAudioClient_IsUnicastConnected(usb_dongle_lea_data.group_handle);
}

bool UsbDongle_LeaIsVoiceAvailable(void)
{
    return UsbDongle_LeaIsVoiceConnected() &&
           (usb_dongle_lea_data.audio_context_mask & USB_DONGLE_LEA_VOICE);
}

/* Source is still active but lea context is changed */
bool UsbDongle_LeaContextSwitchIsRequired(void)
{
    usb_dongle_lea_context_t new_ctx = usbDongle_LeaDetermineNewContext();

    if(UsbDongle_LeaIsSourceActive())
    {
        return (usb_dongle_lea_data.active_context != new_ctx);
    }
    else if(usbDongle_LeaIsStreamRequested())
    {
        return (usb_dongle_lea_data.requested_context != new_ctx);
    }

    return FALSE;
}

void UsbDongle_LeaRegisterConnectionCallbacks(void (*connected_cb)(void),
                                               void (*disconnected_cb)(void))
{
    usb_dongle_lea_data.connected_cb = connected_cb;
    usb_dongle_lea_data.disconnected_cb = disconnected_cb;
}

bool UsbDongle_LeaInit(Task task)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeaInit");

    UNUSED(task);

    UsbDongleConfigInit();

    memset(&usb_dongle_lea_data, 0, sizeof(usb_dongle_lea_data));
    usb_dongle_lea_data.task.handler = usbDongle_LeaHandleMessage;

    /* Register as ui provider */
    Ui_RegisterUiProvider(ui_provider_media_player, usbDongle_LeaGetStateContext);

    LeAudioClient_ClientRegister(&usb_dongle_lea_data.task);
    LeAudioClient_RegisterUsbSourceConnectParamCallback(usbDongle_GetUsbSourceConnectParam);
    UsbDongle_LeaConfigInit();
    LeAudioClient_SetBootGameModeConfig(usbDongleConfig_IsInGamingAudioMode());

    UsbDongle_LeVoiceInit();

    return TRUE;
}

static bool usbDongle_LeaIsGamingContextActive(void)
{
    bool gaming_mode_active = FALSE;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    if (UsbDongle_LeaIsSourceActive())
    {
        switch (LeAudioClient_GetUnicastSessionCapAudioContext())
        {
            case CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC:
            case CAP_CLIENT_CONTEXT_TYPE_GAME:
                gaming_mode_active = TRUE;
            break;

            default:
                break;
        }
    }
#endif

    return gaming_mode_active;
}

bool UsbDongle_LeaIsContextChangeRequired(void)
{
    bool isGamingModeReqd = usbDongleConfig_IsInGamingAudioMode();
    bool isGamingCtxActive = !!usbDongle_LeaIsGamingContextActive();

    return UsbDongle_LeaIsSourceActive() && isGamingModeReqd != isGamingCtxActive;
}

bool UsbDongle_LeaSwitchToVoiceContextIfRequired(void)
{
    bool status = FALSE;

    /* Check if dongle context is gaming with VBC */
    if (UsbDongle_LeaGetContext() & USB_DONGLE_LEA_AUDIO_VBC)
    {
        /* Change the context to voice */
        UsbDongle_LeaRemoveContext(USB_DONGLE_LEA_AUDIO_VBC);
        UsbDongle_LeaAddContext(USB_DONGLE_LEA_VOICE);

        status = TRUE;
    }

    return status;
}

void UsbDongle_LeaRestartAudioGraph(bool enable_mic)
{
    if (UsbDongle_LeaIsSourceActive())
    {
        usbDongle_LeAudioStopGraph();
        usbDongle_LeaStartGraph(enable_mic, TRUE);
    }
}

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

bool UsbDongle_LeaAudioTransportSwitchRequired(void)
{
    bool switch_required;

    /* Switching is needed if mode changed to broadcast mode while unicast is streaming or
       if mode changed to unicast mode while broadcast is streaming */
    switch_required = usbDongleConfig_IsInBroadcastAudioMode() ? usbDongle_IsInLeaUnicastStreaming()
                                                               : usbDongle_IsInLeaBroadcastStreaming();

    return switch_required;
}

/*! \brief Handle request to toggle the broadcast mode */
bool UsbDongle_LeaHandleBroadcastModeToggle(bool is_connected)
{
    bool isBroadcastModeEnabled = usbDongle_IsLeaBroadcastModeRequired();

    DEBUG_LOG_FN_ENTRY("UsbDongle_LeaHandleBroadcastModeToggle BroadcastModeEnabled: %d, isLeaConnected: %d, state enum:usb_dongle_lea_state_t:%d",
                       isBroadcastModeEnabled, is_connected, usbDongle_LeaGetState());

    if (is_connected)
    {
        /* Main dongle SM has to handle the change in mode */
        return TRUE;
    }

    switch (usbDongle_LeaGetState())
    {
        case APP_LEA_STATE_BROADCAST_STREAMING_STARTING:
        case APP_LEA_STATE_BROADCAST_STREAMING:
        {
            /* Broadcast mode is now disabled. Stop the streaming to exit. */
            if (!isBroadcastModeEnabled)
            {
                usbDongle_LeaSetState(APP_LEA_STATE_STREAMING_STOPPING);
            }
        }
        break;

        case APP_LEA_STATE_DISCONNECTED:
        {
            /* Broadcast mode is now enabled. Start broadcast streaming if audio context is available  */
            if (isBroadcastModeEnabled)
            {
                DEBUG_LOG_INFO("UsbDongle_LeaHandleBroadcastModeToggle, starting the streaming");

                usbDongle_LeaSetState(APP_LEA_STATE_BROADCAST_STREAMING_STARTING);
            }
        }
        break;

        default:
        break;
    }

    return FALSE;
}

#endif  /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#else

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#error "INCLUDE_SOURCE_APP_LE_AUDIO not defined but INCLUDE_LE_AUDIO_BROADCAST_SOURCE defined"
#endif

#endif  /* INCLUDE_SOURCE_APP_LE_AUDIO */
