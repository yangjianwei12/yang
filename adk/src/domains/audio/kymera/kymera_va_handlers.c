/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module that implements basic build block functions to handle Voice Assistant related actions
*/

#ifdef INCLUDE_VOICE_UI
#include "kymera_va_handlers.h"
#include "kymera_va_common.h"
#include "kymera_va_encode_chain.h"
#include "kymera_va_mic_chain.h"
#include "kymera_va_wuw_chain.h"
#include "kymera_dsp_clock.h"
#include "kymera_tones_prompts.h"

#include <vmal.h>

static struct
{
    unsigned engine_supports_default_low_power_clock:1;
    unsigned low_power_mode_enabled:1;
} state;

static void kymeraVaHandler_CreateMicChain(const va_audio_mic_config_t *mic_config, bool support_wuw, uint32 pre_roll_needed_in_ms)
{
    va_mic_chain_create_params_t params = {0};
    bool mic_chain_supported = FALSE;

#ifdef KYMERA_VA_USE_CHAIN_WITHOUT_CVC
    params.chain_params.clear_voice_capture = FALSE;
#else
    params.chain_params.clear_voice_capture = TRUE;
#endif
    params.chain_params.wake_up_word_detection = support_wuw;
    params.operators_params.max_pre_roll_in_ms = pre_roll_needed_in_ms;

#ifdef KYMERA_VA_USE_1MIC
    params.chain_params.number_of_mics = 1;
    mic_chain_supported = (mic_config->min_number_of_mics <= 1) && Kymera_IsVaMicChainSupported(&params.chain_params);
#else
    for(int i = MIN(mic_config->max_number_of_mics, Microphones_MaxSupported()); i >= mic_config->min_number_of_mics; i--)
    {
        params.chain_params.number_of_mics = i;
        if (Kymera_IsVaMicChainSupported(&params.chain_params))
        {
            mic_chain_supported = TRUE;
            break;
        }
    }
#endif

    PanicFalse(mic_chain_supported);
    bool using_multi_mic_cvc = (params.chain_params.number_of_mics > 1) && params.chain_params.clear_voice_capture;
    state.low_power_mode_enabled = !using_multi_mic_cvc;

    Kymera_CreateVaMicChain(&params);
}

static void kymeraVaHandler_CreateEncodeChain(const va_audio_encode_config_t *encoder_config)
{
    va_encode_chain_create_params_t chain_params = {0};
    chain_params.chain_params.encoder = encoder_config->encoder;
    chain_params.operators_params.encoder_params = &encoder_config->encoder_params;
    Kymera_CreateVaEncodeChain(&chain_params);
}

static void kymeraVaHandler_CreateVaWuwChain(Task detection_handler, const va_audio_wuw_config_t *wuw_config)
{
    va_wuw_chain_create_params_t wuw_params = {0};
    wuw_params.chain_params.wuw_engine = wuw_config->engine;
    wuw_params.operators_params.wuw_model = wuw_config->model;
    wuw_params.operators_params.wuw_detection_handler = detection_handler;
    wuw_params.operators_params.LoadWakeUpWordModel = wuw_config->LoadWakeUpWordModel;
    wuw_params.operators_params.engine_init_preroll_ms = wuw_config->engine_init_preroll_ms;
    Kymera_CreateVaWuwChain(&wuw_params);
}

static void kymeraVaHandler_WuwDetectionChainSleep(void)
{
    Kymera_VaWuwChainSleep();
    Kymera_VaMicChainSleep();
    Kymera_VaEncodeChainSleep();
}

static void kymeraVaHandler_WuwDetectionChainWake(void)
{
    Kymera_VaMicChainWake();
    Kymera_VaWuwChainWake();
    Kymera_VaEncodeChainWake();
}

void KymeraVaHandler_CreateMicChainForLiveCapture(const void *params)
{
    const va_audio_voice_capture_params_t *capture = params;
    kymeraVaHandler_CreateMicChain(&capture->mic_config, FALSE, 0);
}

void KymeraVaHandler_CreateMicChainForWuw(const void *params)
{
    const wuw_detection_start_t *wuw_detection = params;
#if defined (__QCC516X__) || defined (__QCC517X__)
    state.engine_supports_default_low_power_clock = FALSE;
#else
    state.engine_supports_default_low_power_clock = (wuw_detection->params->wuw_config.engine == va_wuw_engine_apva) ? FALSE : TRUE;
#endif
    kymeraVaHandler_CreateMicChain(&wuw_detection->params->mic_config, TRUE, wuw_detection->params->max_pre_roll_in_ms);
}

void KymeraVaHandler_ConnectToMics(const void *params)
{
    UNUSED(params);
    Kymera_ConnectVaMicChainToMics();
}

void KymeraVaHandler_DisconnectFromMics(const void *params)
{
    UNUSED(params);
    Kymera_DisconnectVaMicChainFromMics();
}

void KymeraVaHandler_StartMicChain(const void *params)
{
    UNUSED(params);
    Kymera_StartVaMicChain();
}

void KymeraVaHandler_StopMicChain(const void *params)
{
    UNUSED(params);
    Kymera_StopVaMicChain();
}

void KymeraVaHandler_DestroyMicChain(const void *params)
{
    UNUSED(params);
    Kymera_DestroyVaMicChain();
}

void KymeraVaHandler_ActivateMicChainEncodeOutputForLiveCapture(const void *params)
{
    UNUSED(params);
    Kymera_ActivateVaMicChainEncodeOutput();
}

void KymeraVaHandler_ActivateMicChainEncodeOutputForWuwCapture(const void *params)
{
    const va_audio_wuw_capture_params_t *capture = params;
    Kymera_ActivateVaMicChainEncodeOutputAfterTimestamp(capture->start_timestamp);
}

void KymeraVaHandler_DeactivateMicChainEncodeOutput(const void *params)
{
    UNUSED(params);
    Kymera_DeactivateVaMicChainEncodeOutput();
}

void KymeraVaHandler_BufferMicChainEncodeOutput(const void *params)
{
    UNUSED(params);
    Kymera_BufferVaMicChainEncodeOutput();
}

void KymeraVaHandler_ActivateMicChainWuwOutput(const void *params)
{
    UNUSED(params);
    Kymera_ActivateVaMicChainWuwOutput();
}

void KymeraVaHandler_DeactivateMicChainWuwOutput(const void *params)
{
    UNUSED(params);
    Kymera_DeactivateVaMicChainWuwOutput();
}

void KymeraVaHandler_CreateEncodeChainForLiveCapture(const void *params)
{
    const va_audio_voice_capture_params_t *capture = params;
    kymeraVaHandler_CreateEncodeChain(&capture->encode_config);
}

void KymeraVaHandler_CreateEncodeChainForWuwCapture(const void *params)
{
    const va_audio_wuw_capture_params_t *capture = params;
    kymeraVaHandler_CreateEncodeChain(&capture->encode_config);
}

void KymeraVaHandler_StartEncodeChain(const void *params)
{
    UNUSED(params);
    Kymera_StartVaEncodeChain();
}

void KymeraVaHandler_StopEncodeChain(const void *params)
{
    UNUSED(params);
    Kymera_StopVaEncodeChain();
}

void KymeraVaHandler_DestroyEncodeChain(const void *params)
{
    UNUSED(params);
    Kymera_DestroyVaEncodeChain();
}

void KymeraVaHandler_CreateWuwChain(const void *params)
{
    const wuw_detection_start_t *wuw_detection = params;
    kymeraVaHandler_CreateVaWuwChain(wuw_detection->handler, &wuw_detection->params->wuw_config);
}

void KymeraVaHandler_StartWuwChain(const void *params)
{
    UNUSED(params);
    Kymera_StartVaWuwChain();
}

void KymeraVaHandler_StopWuwChain(const void *params)
{
    UNUSED(params);
    Kymera_StopVaWuwChain();
}

void KymeraVaHandler_DestroyWuwChain(const void *params)
{
    UNUSED(params);
    Kymera_DestroyVaWuwChain();
}

void KymeraVaHandler_ConnectWuwChainToMicChain(const void *params)
{
    UNUSED(params);
    Kymera_ConnectVaWuwChainToMicChain();
}

void KymeraVaHandler_StartGraphManagerDelegation(const void *params)
{
    UNUSED(params);
    Kymera_VaWuwChainStartGraphManagerDelegation();

    if (state.low_power_mode_enabled)
    {
        kymeraVaHandler_WuwDetectionChainSleep();
    }
}

void KymeraVaHandler_StopGraphManagerDelegation(const void *params)
{
    UNUSED(params);
    if (state.low_power_mode_enabled)
    {
        kymeraVaHandler_WuwDetectionChainWake();
    }
    Kymera_VaWuwChainStopGraphManagerDelegation();
}

void KymeraVaHandler_EnterKeepDspOn(const void *params)
{
    UNUSED(params);
    VmalOperatorFrameworkEnableMainProcessor();
}

void KymeraVaHandler_ExitKeepDspOn(const void *params)
{
    UNUSED(params);
    VmalOperatorFrameworkDisableMainProcessor();
}

void KymeraVaHandler_UpdateDspClock(const void *params)
{
    UNUSED(params);
    appKymeraConfigureDspPowerMode();
}

void KymeraVaHandler_UpdateDspKickPeriod(const void *params)
{
    UNUSED(params);
    OperatorsFrameworkSetKickPeriod(KICK_PERIOD_VOICE);
}

void KymeraVaHandler_BoostClockForChainCreation(const void *params)
{
    UNUSED(params);
    DEBUG_LOG("Kymera_BoostClockForChainCreation");
    appKymeraBoostDspClockToMax();
}

void KymeraVaHandler_SetWuwSampleRate(const void *params)
{
    const wuw_detection_start_t *wuw_detection = params;
    Kymera_SetVaSampleRate(wuw_detection->params->mic_config.sample_rate);
}

void KymeraVaHandler_SetLiveCaptureSampleRate(const void *params)
{
    const va_audio_voice_capture_params_t *capture = params;
    Kymera_SetVaSampleRate(capture->mic_config.sample_rate);
}

void KymeraVaHandler_LoadDownloadableCapsForPrompt(const void *params)
{
    UNUSED(params);
    Kymera_PromptLoadDownloadableCaps();
}

void KymeraVaHandler_UnloadDownloadableCapsForPrompt(const void *params)
{
    UNUSED(params);
    Kymera_PromptUnloadDownloadableCaps();
}

bool Kymera_VaIsLowPowerEnabled(void)
{
    return state.low_power_mode_enabled;
}

bool Kymera_WuwEngineSupportsDefaultLpClock(void)
{
    return state.engine_supports_default_low_power_clock;
}

#endif /*#ifdef INCLUDE_VOICE_UI */
