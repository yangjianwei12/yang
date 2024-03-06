/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Start/stop audio from various inputs, keep track of active source.
*/

#include "usb_dongle_logging.h"

#include "usb_dongle_audio.h"

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)
#include "usb_dongle_a2dp.h"
#endif

#if defined(INCLUDE_SOURCE_APP_LE_AUDIO)
#include "usb_dongle_lea.h"
#endif

#include <av.h>
#include <audio_sources.h>
#include <kymera.h>
#include <kymera_adaptation.h>
#include <power_manager.h>
#include <wired_audio_source.h>

/*! \brief Charger case audio data structure */
typedef struct
{
    audio_source_t active_source;   /*!< Currently active audio source if any */
    uint8 connected_inputs;         /*!< Bitfield of connected audio inputs */

    /*! Client-provided function to call once the audio chain has successfully
        stopped. Necessary because some chains stop asynchronously, e.g. USB. */
    UsbDongleAudioStopped stopped_callback;

    /*! Supplied as part of the USB disconnect params. Must be called once the
        USB audio chain has been stopped by kymera. */
    void (*usb_stopped_handler)(Source source);

    /*! Configuration parameters last used to connect analogue wired line-in. */
    wired_audio_config_t wired_config;
} usb_dongle_audio_data_t;

/*! \brief Charger case audio data instance */
static usb_dongle_audio_data_t usb_dongle_audio_data;


#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)
/*! \brief Handle callback from Kymera to say USB audio has stopped */
static void usbDongle_HandleKymeraUsbAudioStopComplete(Source source)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleKymeraUsbAudioStopComplete");

    PanicZero(usb_dongle_audio_data.usb_stopped_handler);

    /* Call handler to free any USB resources as required. */
    usb_dongle_audio_data.usb_stopped_handler(source);
    usb_dongle_audio_data.usb_stopped_handler = NULL;

    if (usb_dongle_audio_data.active_source == audio_source_usb)
    {
        usb_dongle_audio_data.active_source = audio_source_none;
    }

    if (usb_dongle_audio_data.stopped_callback)
    {
        usb_dongle_audio_data.stopped_callback();
        usb_dongle_audio_data.stopped_callback = NULL;
    }
}

/*! \brief Start analogue wired audio chain (line-in) */
static void usbDongle_AudioStartAnalogue(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_AudioStartAnalogue");

    if (UsbDongle_AudioIsActive())
    {
        DEBUG_LOG_ERROR("usbDongle_AudioStartAnalogue, can't start analogue chain, audio already running");
        return;
    }

    DEBUG_LOG_INFO("usbDongle_AudioStartAnalogue, starting analogue wired audio chain");
    /* Volume is set to 0 as it is not used */
    Kymera_StartWiredAnalogAudio(0,
                                 usb_dongle_audio_data.wired_config.rate,
                                 usb_dongle_audio_data.wired_config.min_latency,
                                 usb_dongle_audio_data.wired_config.max_latency,
                                 usb_dongle_audio_data.wired_config.target_latency
                                 );

    usb_dongle_audio_data.active_source = audio_source_line_in;
}

/*! \brief Stop analogue wired audio chain (line-in) */
static void usbDongle_AudioStopAnalogue(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_AudioStopAnalogue");

    if (usb_dongle_audio_data.active_source != audio_source_line_in)
    {
        DEBUG_LOG_WARN("usbDongle_AudioStopAnalogue, attempted to stop USB audio chain when not running");
        return;
    }

    DEBUG_LOG_INFO("usbDongle_AudioStopAnalogue, stopping analogue wired audio chain");
    Kymera_StopWiredAnalogAudio();

    usb_dongle_audio_data.active_source = audio_source_none;
}

/*! \brief Start USB audio chain */
static void usbDongle_AudioStartUsb(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_AudioStartUsb");

    source_defined_params_t usb_params;
    usb_audio_connect_parameters_t usb_params_data;

    if (UsbDongle_AudioIsActive())
    {
        DEBUG_LOG_ERROR("usbDongle_AudioStartUsb, can't start USB chain, audio already running");
        return;
    }

    AudioSources_GetConnectParameters(audio_source_usb, &usb_params);
    usb_params_data = *(usb_audio_connect_parameters_t *)usb_params.data;
    AudioSources_ReleaseConnectParameters(audio_source_usb, &usb_params);

    if (usb_params_data.spkr_src == NULL)
    {
        DEBUG_LOG_ERROR("usbDongle_AudioStartUsb, can't start USB chain, spkr_src is NULL");
        return;
    }

    /* USB audio requires higher clock speeds, so request a switch to the
     * "performance" power profile */
    appPowerPerformanceProfileRequest();

    /* ignore latency values provided by the USB source, use ones defined by the application */
    DEBUG_LOG_INFO("usbDongle_AudioStartUsb, starting USB audio chain");
    usb_params_data.min_latency_ms = usb_dongle_audio_data.wired_config.min_latency;
    usb_params_data.max_latency_ms = usb_dongle_audio_data.wired_config.max_latency;
    usb_params_data.target_latency_ms = usb_dongle_audio_data.wired_config.target_latency;
    connect_parameters_t connect_parameters;
    connect_parameters.source.type = source_type_audio;
    connect_parameters.source.u.audio = audio_source_usb;
    connect_parameters.source_params.data_length = sizeof(usb_audio_connect_parameters_t);
    connect_parameters.source_params.data = &usb_params_data;
    KymeraAdaptation_Connect(&connect_parameters);

    usb_dongle_audio_data.active_source = audio_source_usb;
}

/*! \brief Stop USB audio chain */
static void usbDongle_AudioStopUsb(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_AudioStopUsb");

    source_defined_params_t usb_params;
    usb_audio_disconnect_parameters_t usb_params_data;

    if (usb_dongle_audio_data.active_source != audio_source_usb)
    {
        DEBUG_LOG_WARN("usbDongle_AudioStopUsb, attempted to stop USB audio chain when not running");
        return;
    }

    if (!AudioSources_GetDisconnectParameters(audio_source_usb, &usb_params))
    {
        DEBUG_LOG_ERROR("usbDongle_AudioStopUsb, failed to get disconnect params");
        return;
    }
    usb_params_data = *(usb_audio_disconnect_parameters_t *)usb_params.data;
    AudioSources_ReleaseDisconnectParameters(audio_source_usb, &usb_params);

    DEBUG_LOG_INFO("usbDongle_AudioStopUsb, stopping USB Audio Chain");
    usb_dongle_audio_data.usb_stopped_handler = usb_params_data.kymera_stopped_handler;
    appKymeraUsbAudioStop(usb_params_data.source,
                          usbDongle_HandleKymeraUsbAudioStopComplete);

    /* No longer need to be in high performance power profile */
    appPowerPerformanceProfileRelinquish();
}

static void usbDongle_AudioRestartUsbStoppedCallback(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_AudioRestartUsbStoppedCallback");
    usbDongle_AudioStartUsb();
}

#endif  /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

bool UsbDongle_IsGraphRestartNeededForUsbAudioConfigChange(bool is_sampling_rate_changed)
{
    bool restart_needed = TRUE;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsConnectedToBredrSink())
    {
        /* Restart USB audio immediately if USB sampling rate not changed or A2DP renegotiation not required.
           Otherwise, the renegotiation process will take care of restarting. */
        restart_needed = !is_sampling_rate_changed || !UsbDongle_A2dpUpdatePreferredSampleRate();
    }

#else

    UNUSED(is_sampling_rate_changed);

#endif  /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

    /* In case of LE audio, streaming can be active without a connection also (broadcast).
       So just return TRUE here */

    return restart_needed;
}

void UsbDongle_AudioRestartUsbGraph(void)
{
    /* first check that kymera has some chain running */
    if (Kymera_IsIdle())
    {
        DEBUG_LOG("UsbDongle_AudioRestartUsbGraph, kymera is idle, cannot restart graph");
        return;
    }

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    /* Restart the graph if connected with a BREDR sink and streaming is ongoing */
    if (usbDongleConfig_IsConnectedToBredrSink())
    {
        /* Check if active chain is not usb */
        if (usb_dongle_audio_data.active_source != audio_source_usb)
        {
            DEBUG_LOG_WARN("UsbDongle_AudioRestartUsbGraph, USB audio graph not running");
            return;
        }

        UsbDongle_AudioStop(usbDongle_AudioRestartUsbStoppedCallback);
    }

#endif  /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

    /* Connection is not relevant for LE audio as broadcast can be connectionless. So just
       check if BREDR is not connected */
    if (!usbDongleConfig_IsConnectedToBredrSink())
    {
        /* No need to restart if there is no active LE audio streaming or if gaming + VBC is streaming (which
           uses voice graph) or if active chain is line in instead of usb */
        if (!UsbDongle_LeaIsAudioActive() || UsbDongle_LeaIsVbcActive() || UsbDongle_LeaIsAnalogAudioActive())
        {
            DEBUG_LOG_WARN("UsbDongle_AudioRestartUsbGraph, USB audio graph not running");
            return;
        }

        UsbDongle_LeaRestartAudioGraph(FALSE);
    }

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */
}

bool UsbDongle_AudioIsActive(void)
{
    bool is_active = FALSE;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        is_active = (usb_dongle_audio_data.active_source != audio_source_none);
    }

#endif  /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        is_active = UsbDongle_LeaIsAudioActive();
    }

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    return is_active;
}

void UsbDongle_AudioInputAdd(usb_dongle_audio_input_t input)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_AudioInputAdd,"
                       " enum:usb_dongle_audio_input_t:%d", input);
    usb_dongle_audio_data.connected_inputs |= input;
}

void UsbDongle_AudioInputRemove(usb_dongle_audio_input_t input)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_AudioInputRemove,"
                       " enum:usb_dongle_audio_input_t:%d", input);
    usb_dongle_audio_data.connected_inputs &= ~input;
}

voice_source_t UsbDongle_VoiceDetermineNewSource(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_VoiceDetermingNewSource");

    voice_source_t voice_source = voice_source_none;

    if (usb_dongle_audio_data.connected_inputs & usb_dongle_audio_input_usb_voice)
    {
        voice_source = voice_source_usb;
    }

    return voice_source;
}

audio_source_t UsbDongle_AudioDetermineNewSource(void)
{
    audio_source_t audio_source = audio_source_none;

    if (!(usb_dongle_audio_data.connected_inputs & usb_dongle_audio_input_usb_voice))
    {
        if (usb_dongle_audio_data.connected_inputs & usb_dongle_audio_input_analogue)
        {
            audio_source = audio_source_line_in;
        }
        else if (usb_dongle_audio_data.connected_inputs & usb_dongle_audio_input_usb)
        {
            audio_source = audio_source_usb;
        }
    }
    DEBUG_LOG_FN_ENTRY("UsbDongle_AudioDetermineNewSource, enum:audio_source_t:%d", audio_source);

    return audio_source;
}

bool UsbDongle_AudioSourceSwitchIsRequired(void)
{
    bool is_switch_required = FALSE;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        is_switch_required = (UsbDongle_AudioIsActive() &&
                             (usb_dongle_audio_data.active_source != UsbDongle_AudioDetermineNewSource()));
    }

#endif  /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        is_switch_required = (UsbDongle_LeaContextSwitchIsRequired() || UsbDongle_LeaAudioTransportSwitchRequired());
    }

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    return is_switch_required;
}

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)

bool UsbDongle_AudioStart(const wired_audio_config_t *config)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_AudioStart");

    if (!usbDongleConfig_IsInBredrMode() && !usbDongleConfig_IsInDualModeWithBredrConnected())
    {
        return FALSE;
    }

    if (config)
    {
        usb_dongle_audio_data.wired_config = *config;
    }
    else if (usb_dongle_audio_data.wired_config.rate == 0)
    {
        DEBUG_LOG_WARN("UsbDongle_AudioStart, wired audio config not available yet");
        return FALSE;
    }

    if (UsbDongle_AudioIsActive())
    {
        DEBUG_LOG_ERROR("UsbDongle_AudioStart, audio chain already running");
        return FALSE;
    }

    switch (UsbDongle_AudioDetermineNewSource())
    {
        case audio_source_line_in:
            usbDongle_AudioStartAnalogue();
            break;

        case audio_source_usb:
            usbDongle_AudioStartUsb();
            break;

        default:
            DEBUG_LOG_INFO("UsbDongle_AudioStart, no inputs connected, ignoring audio start request");
            break;
    }

    return UsbDongle_AudioIsActive();
}

void UsbDongle_AudioStop(UsbDongleAudioStopped callback)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_AudioStop");

    if (!usbDongleConfig_IsInBredrMode() && !usbDongleConfig_IsInDualModeWithBredrConnected())
    {
        return;
    }

    if (usb_dongle_audio_data.stopped_callback)
    {
        DEBUG_LOG_ERROR("UsbDongleAudioStopped, already a previous audio stop request in progress");
        return;
    }

    switch (usb_dongle_audio_data.active_source)
    {
        case audio_source_line_in:
            usbDongle_AudioStopAnalogue();
            break;

        case audio_source_usb:
            usb_dongle_audio_data.stopped_callback = callback;
            usbDongle_AudioStopUsb();
            break;

        default:
            DEBUG_LOG_DEBUG("UsbDongleAudioStopped, no active source, ignoring audio stop request");
            break;
    }

    if (!UsbDongle_AudioIsActive())
    {
        /* Source either stopped synchronously, or there wasn't one to begin
         * with. Notify the caller immediately that audio is now stopped.
         * Otherwise, caller will be notified later once asynchronous stop has
         * completed (e.g. USB - usbDongle_HandleKymeraUsbAudioStopComplete).
         */
        if (callback)
        {
            callback();
        }
    }
}

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

bool UsbDongle_AudioInit(Task init_task)
{
    UNUSED(init_task);

    wired_audio_pio_t source_pio;
    source_pio.line_in_pio = WIRED_AUDIO_LINE_IN_PIO;
#ifdef WIRED_AUDIO_LINE_IN_PIO_DETECT_ON_HIGH
    source_pio.line_in_detect_pio_on_high = TRUE;
#else
    source_pio.line_in_detect_pio_on_high = FALSE;
#endif
    WiredAudioSource_Init(&source_pio);

    usb_dongle_audio_data.active_source = audio_source_none;
    usb_dongle_audio_data.connected_inputs = usb_dongle_audio_input_none;
    usb_dongle_audio_data.stopped_callback = NULL;
    usb_dongle_audio_data.usb_stopped_handler = NULL;
    usb_dongle_audio_data.wired_config.rate = 0;

    return TRUE;
}

bool UsbDongle_AudioInputIsConnected(usb_dongle_audio_input_t audio_input)
{
    return (usb_dongle_audio_data.connected_inputs & audio_input);
}

bool UsbDongle_AudioGetLatencies(uint32 *min, uint32 *max, uint32 *target )
{
    if ( (usb_dongle_audio_data.active_source == audio_source_line_in) ||
         (usb_dongle_audio_data.active_source == audio_source_usb) )
    {
        *min = usb_dongle_audio_data.wired_config.min_latency;
        *max = usb_dongle_audio_data.wired_config.max_latency;
        *target = usb_dongle_audio_data.wired_config.target_latency;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

uint32 UsbDongle_AudioGetCurrentPreferredSampleRate(void)
{
    uint32 preferred_rate;

    switch (UsbDongle_AudioDetermineNewSource())
    {
        case audio_source_usb:
        {
            /* USB host dictates the sample rate, we have no control over it.
               Preferred rate is therefore the USB sample rate. */
            source_defined_params_t usb_params;
            usb_audio_connect_parameters_t *usb_params_data;

            if (AudioSources_GetConnectParameters(audio_source_usb, &usb_params))
            {
                usb_params_data = (usb_audio_connect_parameters_t *)usb_params.data;
                preferred_rate = usb_params_data->sample_freq;
                AudioSources_ReleaseConnectParameters(audio_source_usb, &usb_params);
            }
            else
            {
                /* USB parameters not known/available yet. */
                preferred_rate = 0;
            }
        }
        break;

        case audio_source_line_in:
        default:
        {
            /* Input sample rate can be configured, so no preferred rate. */
            preferred_rate = 0;
        }
        break;
    }

    return preferred_rate;
}

bool UsbDongle_AudioSourceConnect(void)
{
    bool status = FALSE;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        status = UsbDongle_A2dpSourceConnect();
    }

#endif  /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

    return status;
}

void UsbDongle_AudioStreamConnect(void)
{

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        UsbDongle_A2dpSourceResume();
    }

#endif  /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        UsbDongle_LeaAudioStart();
    }

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

}

void UsbDongle_AudioStreamDisconnect(void)
{

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        UsbDongle_A2dpSourceSuspend();
    }

#endif  /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        UsbDongle_LeaAudioStop();
    }

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

}

bool UsbDongle_AudioIsSourceAvailable(void)
{
    bool is_available = FALSE;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        is_available = (appAvGetA2dpSource() != NULL);
    }

#endif  /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        is_available = UsbDongle_LeaIsAudioAvailable();
    }

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    return is_available;
}

bool UsbDongle_AudioIsVbcAvailable(void)
{
    bool is_available = FALSE;

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        is_available = UsbDongle_LeaIsVbcAvailable();
    }

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    return is_available;
}

bool UsbDongle_AudioIsVbcActive(void)
{
    bool is_available = FALSE;

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        is_available = UsbDongle_LeaIsVbcActive();
    }

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    return is_available;
}
