/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Start/stop audio from various inputs, keep track of active source.
*/

#include "charger_case_audio.h"

#include <audio_sources.h>
#include <kymera.h>
#include <power_manager.h>
#include <wired_audio_source.h>


/*! \brief Charger case audio data structure */
typedef struct
{
    audio_source_t active_source;   /*!< Currently active audio source if any */
    uint8 connected_inputs;         /*!< Bitfield of connected audio inputs */

    /*! Client-provided function to call once the audio chain has successfully
        stopped. Necessary because some chains stop asynchronously, e.g. USB. */
    ChargerCaseAudioStopped stopped_callback;

    /*! Configuration parameters last used to connect analogue wired line-in. */
    wired_audio_config_t wired_config;

} charger_case_audio_data_t;

/*! \brief Charger case audio data instance */
charger_case_audio_data_t charger_case_audio_data;


/*! \brief Handle callback from Kymera to say USB audio has stopped */
static void chargerCase_HandleKymeraUsbAudioStopComplete(Source source)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_HandleKymeraUsbAudioStopComplete");
    UNUSED(source);

    if (charger_case_audio_data.active_source == audio_source_usb)
    {
        charger_case_audio_data.active_source = audio_source_none;
    }

    if (charger_case_audio_data.stopped_callback)
    {
        charger_case_audio_data.stopped_callback();
        charger_case_audio_data.stopped_callback = NULL;
    }
}

/*! \brief Start analogue wired audio chain (line-in) */
static void chargerCase_AudioStartAnalogue(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_AudioStartAnalogue");

    if (ChargerCase_AudioIsActive())
    {
        DEBUG_LOG_ERROR("Can't start analogue chain - audio already running!");
        return;
    }

    DEBUG_LOG_INFO("ChargerCase: Starting Analogue Wired Audio Chain");
    /* Volume is set to 0 as it is not used */
    Kymera_StartWiredAnalogAudio(0,
                                 charger_case_audio_data.wired_config.rate,
                                 charger_case_audio_data.wired_config.min_latency,
                                 charger_case_audio_data.wired_config.max_latency,
                                 charger_case_audio_data.wired_config.target_latency);

    charger_case_audio_data.active_source = audio_source_line_in;
}

/*! \brief Stop analogue wired audio chain (line-in) */
static void chargerCase_AudioStopAnalogue(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_AudioStopAnalogue");

    if (charger_case_audio_data.active_source != audio_source_line_in)
    {
        DEBUG_LOG_WARN("Attempted to stop USB audio chain when not running");
        return;
    }

    DEBUG_LOG_INFO("ChargerCase: Stopping Analogue Wired Audio Chain");
    Kymera_StopWiredAnalogAudio();

    charger_case_audio_data.active_source = audio_source_none;
}

/*! \brief Start USB audio chain */
static void chargerCase_AudioStartUsb(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_AudioStartUsb");

    source_defined_params_t usb_params;
    usb_audio_connect_parameters_t usb_params_data;

    if (ChargerCase_AudioIsActive())
    {
        DEBUG_LOG_ERROR("Can't start USB chain, audio already running!");
        return;
    }

    AudioSources_GetConnectParameters(audio_source_usb, &usb_params);
    usb_params_data = *(usb_audio_connect_parameters_t *)usb_params.data;
    AudioSources_ReleaseConnectParameters(audio_source_usb, &usb_params);

    if (usb_params_data.spkr_src == NULL)
    {
        DEBUG_LOG_ERROR("Can't start USB chain, spkr_src is NULL!");
        return;
    }

    /* USB audio requires higher clock speeds, so request a switch to the
     * "performance" power profile */
    appPowerPerformanceProfileRequest();

    /* Latency hardcoded until configurable USB interface added */
    DEBUG_LOG_INFO("ChargerCase: Starting USB Audio Chain");
    appKymeraUsbAudioStart(usb_params_data.channels,
                           usb_params_data.frame_size,
                           usb_params_data.spkr_src,
                           usb_params_data.volume.value,
                           usb_params_data.mute_status,
                           usb_params_data.sample_freq,
                           usb_params_data.min_latency_ms,
                           usb_params_data.max_latency_ms,
                           usb_params_data.target_latency_ms);

    charger_case_audio_data.active_source = audio_source_usb;
}

/*! \brief Stop USB audio chain */
static void chargerCase_AudioStopUsb(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_AudioStopUsb");

    source_defined_params_t usb_params;
    usb_audio_disconnect_parameters_t usb_params_data;

    if (charger_case_audio_data.active_source != audio_source_usb)
    {
        DEBUG_LOG_WARN("Attempted to stop USB audio chain when not running");
        return;
    }

    AudioSources_GetDisconnectParameters(audio_source_usb, &usb_params);
    usb_params_data = *(usb_audio_disconnect_parameters_t *)usb_params.data;
    AudioSources_ReleaseDisconnectParameters(audio_source_usb, &usb_params);

    DEBUG_LOG_INFO("ChargerCase: Stopping USB Audio Chain");
    appKymeraUsbAudioStop(usb_params_data.source,
                          chargerCase_HandleKymeraUsbAudioStopComplete);

    /* No longer need to be in high performance power profile */
    appPowerPerformanceProfileRelinquish();
}

/*! \brief Internal callback used when switching between audio inputs

    Passed to ChargerCase_AudioStop by ChargerCase_AudioSwitchSourceIfRequired
    as a convenience when the intention is to stop one audio chain in order to
    immediatly start another.
 */
static void chargerCase_AudioReadyToSwitchToNewSource(void)
{
    /* Previous source stopped, can now start new source to complete switch.
     * Pass NULL config to reuse parameters from previous chain connect. */
    ChargerCase_AudioStart(NULL);
}


bool ChargerCase_AudioIsActive(void)
{
    return (charger_case_audio_data.active_source != audio_source_none);
}

void ChargerCase_AudioInputAdd(charger_case_audio_input_t input)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_AudioInputAdd,"
                       " enum:charger_case_audio_input_t:%d", input);
    charger_case_audio_data.connected_inputs |= input;
}

void ChargerCase_AudioInputRemove(charger_case_audio_input_t input)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_AudioInputRemove,"
                       " enum:charger_case_audio_input_t:%d", input);
    charger_case_audio_data.connected_inputs &= ~input;
}

audio_source_t ChargerCase_AudioDetermineNewSource(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_AudioDetermineNewSource");

    audio_source_t audio_source = audio_source_none;

    /* Analogue line-in audio has the highest priority, since USB could also be
     * connected for charging purposes. */
    if (charger_case_audio_data.connected_inputs & charger_case_audio_input_analogue)
    {
        audio_source = audio_source_line_in;
    }
    else if (charger_case_audio_data.connected_inputs & charger_case_audio_input_usb)
    {
        audio_source = audio_source_usb;
    }
    return audio_source;
}

bool ChargerCase_AudioStart(const wired_audio_config_t *config)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_AudioStart");

    if (config)
    {
        charger_case_audio_data.wired_config = *config;
    }
    else if (charger_case_audio_data.wired_config.rate == 0)
    {
        DEBUG_LOG_WARN("ChargerCase: Wired audio config not available yet");
        return FALSE;
    }

    if (ChargerCase_AudioIsActive())
    {
        if (config && charger_case_audio_data.wired_config.rate != 0)
        {
            DEBUG_LOG_INFO("Audio already started, checking sources...");
            ChargerCase_AudioSwitchSourceIfRequired();
            return TRUE;
        }
        else
        {
            DEBUG_LOG_INFO("Audio already started, ignoring start request");
            return FALSE;
        }
    }

    switch (ChargerCase_AudioDetermineNewSource())
    {
        case audio_source_line_in:
            chargerCase_AudioStartAnalogue();
            break;

        case audio_source_usb:
            chargerCase_AudioStartUsb();
            break;

        default:
            DEBUG_LOG_INFO("No inputs connected, ignoring audio start request");
            break;
    }

    return ChargerCase_AudioIsActive();
}

void ChargerCase_AudioStop(ChargerCaseAudioStopped callback)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_AudioStop");

    if (charger_case_audio_data.stopped_callback)
    {
        DEBUG_LOG_ERROR("Already a previous audio stop request in progress!");
        return;
    }

    switch (charger_case_audio_data.active_source)
    {
        case audio_source_line_in:
            chargerCase_AudioStopAnalogue();
            break;

        case audio_source_usb:
            charger_case_audio_data.stopped_callback = callback;
            chargerCase_AudioStopUsb();
            break;

        default:
            DEBUG_LOG_DEBUG("No active source, ignoring audio stop request");
            break;
    }

    if (!ChargerCase_AudioIsActive())
    {
        /* Source either stopped synchronously, or there wasn't one to begin
         * with. Notify the caller immediately that audio is now stopped.
         * Otherwise, caller will be notified later once asynchronous stop has
         * completed (e.g. USB - chargerCase_HandleKymeraUsbAudioStopComplete).
         */
        if (callback)
        {
            callback();
        }
    }
}

void ChargerCase_AudioSwitchSourceIfRequired(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_AudioSwitchActiveSource");

    audio_source_t new_source = ChargerCase_AudioDetermineNewSource();

    if (charger_case_audio_data.active_source == new_source)
    {
        DEBUG_LOG_DEBUG("ChargerCase: Highest priority source already active,"
                        " not switching (enum:audio_source_t:%d)", new_source);
        return;
    }

    if (!ChargerCase_AudioIsActive())
    {
        DEBUG_LOG_DEBUG("ChargerCase: No active source, ignore switch request");
        return;
    }

    /* Stop the current source, switch to the new source once complete */
    DEBUG_LOG_INFO("ChargerCase: Switching to enum:audio_source_t:%d ...",
                   new_source);
    ChargerCase_AudioStop(chargerCase_AudioReadyToSwitchToNewSource);
}

bool ChargerCase_AudioInit(Task init_task)
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

    charger_case_audio_data.active_source = audio_source_none;
    charger_case_audio_data.connected_inputs = charger_case_audio_input_none;
    charger_case_audio_data.stopped_callback = NULL;
    charger_case_audio_data.wired_config.rate = 0;

    return TRUE;
}

bool ChargerCase_AudioInputIsConnected(charger_case_audio_input_t audio_input)
{
    return (charger_case_audio_data.connected_inputs & audio_input);
}
