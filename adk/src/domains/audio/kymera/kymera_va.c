/*!
\copyright  Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle Voice Assistant related internal APIs

*/

#ifdef INCLUDE_VOICE_UI
#include "kymera_va.h"
#include "kymera_va_handlers.h"
#include "kymera_va_mic_chain.h"
#include "kymera_va_encode_chain.h"
#include "kymera_va_wuw_chain.h"
#include "kymera_va_common.h"
#include "kymera_dsp_clock.h"
#include "timestamp_event.h"
#include <logging.h>

#define MAX_NUMBER_OF_ACTIONS 16

#define NO_ACTIONS {NULL}
#define ADD_ACTIONS(...) {__VA_ARGS__}

#define ADD_TRANSITIONS(state_transitions_array) ARRAY_DIM(state_transitions_array), state_transitions_array

typedef enum
{
    va_idle,
    va_live_capture_prepared,
    va_capturing,
    va_wuw_detect_prepared,
    va_wuw_detecting,
    va_wuw_detected,
    va_capturing_detect_pending,
    va_live_capture_prepared_detect_pending,
    va_wuw_detecting_paused,
} va_states_t;

typedef enum
{
    live_capture_prepare,
    live_capture_start,
    wuw_capture_start,
    capture_stop,
    wuw_detect_prepare,
    wuw_detect_start,
    wuw_detect_stop,
    wuw_detected,
    wuw_ignore_detected,
    mic_stop,
    mic_start
} va_events_t;

typedef void (* enterStateAction)(const void *event_params);

typedef struct
{
    va_events_t      event;
    va_states_t      new_state;
    enterStateAction actions[MAX_NUMBER_OF_ACTIONS];
} state_transition_t;

typedef struct
{
    va_states_t               state_id;
    unsigned                  is_capture_active;
    unsigned                  is_wuw_active;
    unsigned                  number_of_state_transitions;
    const state_transition_t *state_transitions;
} state_t;

static void kymera_SetAsInterruptibleMicUser(const void *params);
static void kymera_SetAsUninterruptibleMicUser(const void *params);

static const state_transition_t idle_state_transitions[] =
{
    {live_capture_prepare, va_live_capture_prepared, ADD_ACTIONS(
        KymeraVaHandler_EnterKeepDspOn, KymeraVaHandler_BoostClockForChainCreation, KymeraVaHandler_UpdateDspKickPeriod,
        KymeraVaHandler_SetLiveCaptureSampleRate,
        KymeraVaHandler_CreateMicChainForLiveCapture, KymeraVaHandler_CreateEncodeChainForLiveCapture,
        KymeraVaHandler_UpdateDspClock, KymeraVaHandler_ExitKeepDspOn
    )},
#ifdef INCLUDE_WUW
    {wuw_detect_prepare, va_wuw_detect_prepared, ADD_ACTIONS(
        KymeraVaHandler_EnterKeepDspOn, KymeraVaHandler_BoostClockForChainCreation, KymeraVaHandler_UpdateDspKickPeriod,
        KymeraVaHandler_SetWuwSampleRate,
        KymeraVaHandler_LoadDownloadableCapsForPrompt,
        KymeraVaHandler_CreateMicChainForWuw, KymeraVaHandler_CreateWuwChain,
        KymeraVaHandler_ConnectWuwChainToMicChain, KymeraVaHandler_BufferMicChainEncodeOutput, KymeraVaHandler_ActivateMicChainWuwOutput,
        KymeraVaHandler_UpdateDspClock, KymeraVaHandler_ExitKeepDspOn,
    )},
#endif /* INCLUDE_WUW */
};

static const state_transition_t live_capture_prepared_state_transitions[] =
{
    {live_capture_start, va_capturing, ADD_ACTIONS(
        KymeraVaHandler_BoostClockForChainCreation,
        kymera_SetAsUninterruptibleMicUser, KymeraVaHandler_ConnectToMics,
        KymeraVaHandler_StartEncodeChain, KymeraVaHandler_StartMicChain,
        KymeraVaHandler_UpdateDspClock
    )},
    {capture_stop, va_idle, ADD_ACTIONS(
        KymeraVaHandler_EnterKeepDspOn,
        KymeraVaHandler_DestroyEncodeChain, KymeraVaHandler_DestroyMicChain, kymera_SetAsInterruptibleMicUser,
        KymeraVaHandler_UpdateDspClock, KymeraVaHandler_ExitKeepDspOn
    )}
};

static const state_transition_t capturing_state_transitions[] =
{
    {capture_stop, va_idle, ADD_ACTIONS(
        KymeraVaHandler_StopMicChain, KymeraVaHandler_StopEncodeChain, KymeraVaHandler_DisconnectFromMics,
        KymeraVaHandler_EnterKeepDspOn,
        KymeraVaHandler_DestroyEncodeChain, KymeraVaHandler_DestroyMicChain, kymera_SetAsInterruptibleMicUser,
        KymeraVaHandler_UpdateDspClock, KymeraVaHandler_ExitKeepDspOn
    )},
};

#ifdef INCLUDE_WUW
static const state_transition_t wuw_detect_prepared_state_transitions[] =
{
    {wuw_detect_start, va_wuw_detecting, ADD_ACTIONS(
        KymeraVaHandler_BoostClockForChainCreation,
        KymeraVaHandler_ConnectToMics, KymeraVaHandler_StartWuwChain, KymeraVaHandler_StartMicChain,
        KymeraVaHandler_UpdateDspClock,
        KymeraVaHandler_StartGraphManagerDelegation
    )},
    {wuw_detect_stop, va_idle, ADD_ACTIONS(
        KymeraVaHandler_EnterKeepDspOn,
        KymeraVaHandler_DestroyWuwChain, KymeraVaHandler_DestroyMicChain,
        KymeraVaHandler_UnloadDownloadableCapsForPrompt,
        KymeraVaHandler_UpdateDspClock, KymeraVaHandler_ExitKeepDspOn
    )},
};

static const state_transition_t wuw_detecting_state_transitions[] =
{
    {live_capture_prepare, va_live_capture_prepared_detect_pending, ADD_ACTIONS(
        kymera_SetAsUninterruptibleMicUser, KymeraVaHandler_StopGraphManagerDelegation, KymeraVaHandler_DeactivateMicChainWuwOutput, KymeraVaHandler_StopWuwChain,
        KymeraVaHandler_DeactivateMicChainEncodeOutput,
        KymeraVaHandler_BoostClockForChainCreation, KymeraVaHandler_UpdateDspKickPeriod,
        KymeraVaHandler_CreateEncodeChainForLiveCapture,
        KymeraVaHandler_UpdateDspClock,
    )},
    {wuw_detect_stop, va_idle, ADD_ACTIONS(
        KymeraVaHandler_StopGraphManagerDelegation, KymeraVaHandler_StopMicChain, KymeraVaHandler_StopWuwChain, KymeraVaHandler_DisconnectFromMics,
        KymeraVaHandler_EnterKeepDspOn,
        KymeraVaHandler_DestroyEncodeChain, KymeraVaHandler_DestroyWuwChain, KymeraVaHandler_DestroyMicChain,
        KymeraVaHandler_UnloadDownloadableCapsForPrompt,
        KymeraVaHandler_UpdateDspClock, KymeraVaHandler_ExitKeepDspOn
    )},
    {wuw_detected, va_wuw_detected, ADD_ACTIONS(kymera_SetAsUninterruptibleMicUser, KymeraVaHandler_StopGraphManagerDelegation, KymeraVaHandler_DeactivateMicChainWuwOutput)},
    {mic_stop, va_wuw_detecting_paused, ADD_ACTIONS(KymeraVaHandler_StopGraphManagerDelegation, KymeraVaHandler_StopMicChain)},
};

static const state_transition_t wuw_detecting_paused_transitions[] =
{
    {mic_start, va_wuw_detecting, ADD_ACTIONS(KymeraVaHandler_StartMicChain, KymeraVaHandler_StartGraphManagerDelegation)},
};

static const state_transition_t wuw_detected_state_transitions[] =
{
    {wuw_capture_start, va_capturing_detect_pending, ADD_ACTIONS(
        KymeraVaHandler_StopWuwChain,
        KymeraVaHandler_BoostClockForChainCreation, KymeraVaHandler_UpdateDspKickPeriod,
        KymeraVaHandler_CreateEncodeChainForWuwCapture,
        KymeraVaHandler_UpdateDspClock,
        KymeraVaHandler_StartEncodeChain, KymeraVaHandler_ActivateMicChainEncodeOutputForWuwCapture
    )},
    {wuw_ignore_detected, va_wuw_detecting,                ADD_ACTIONS(KymeraVaHandler_ActivateMicChainWuwOutput, KymeraVaHandler_StartGraphManagerDelegation, kymera_SetAsInterruptibleMicUser)},
};

static const state_transition_t capturing_detect_pending_state_transitions[] =
{
    {capture_stop, va_wuw_detecting, ADD_ACTIONS(
        KymeraVaHandler_DeactivateMicChainEncodeOutput, KymeraVaHandler_StopEncodeChain,
        KymeraVaHandler_UpdateDspClock, KymeraVaHandler_UpdateDspKickPeriod,
        KymeraVaHandler_BufferMicChainEncodeOutput, KymeraVaHandler_StartWuwChain, KymeraVaHandler_ActivateMicChainWuwOutput, KymeraVaHandler_StartGraphManagerDelegation,
        kymera_SetAsInterruptibleMicUser
    )},
    {wuw_detect_stop, va_capturing, ADD_ACTIONS(KymeraVaHandler_UnloadDownloadableCapsForPrompt, KymeraVaHandler_DestroyWuwChain, KymeraVaHandler_UpdateDspClock)},
};

static const state_transition_t live_capture_prepared_detect_pending_state_transitions[] =
{
    {live_capture_start, va_capturing_detect_pending, ADD_ACTIONS(
        KymeraVaHandler_UpdateDspClock,
        KymeraVaHandler_StartEncodeChain, KymeraVaHandler_ActivateMicChainEncodeOutputForLiveCapture
    )},
    {capture_stop, va_wuw_detecting, ADD_ACTIONS(
        KymeraVaHandler_UpdateDspClock, KymeraVaHandler_UpdateDspKickPeriod,
        KymeraVaHandler_BufferMicChainEncodeOutput, KymeraVaHandler_StartWuwChain, KymeraVaHandler_ActivateMicChainWuwOutput, KymeraVaHandler_StartGraphManagerDelegation,
        kymera_SetAsInterruptibleMicUser
    )},
    {wuw_detect_stop, va_live_capture_prepared, ADD_ACTIONS(
        KymeraVaHandler_StopMicChain, KymeraVaHandler_DisconnectFromMics, KymeraVaHandler_ActivateMicChainEncodeOutputForLiveCapture,
        KymeraVaHandler_DestroyWuwChain,
        KymeraVaHandler_UnloadDownloadableCapsForPrompt,
        KymeraVaHandler_UpdateDspClock
    )},
};

#endif /* INCLUDE_WUW */

static const state_t states[] =
{
/*  {state,                                   is_capture_active, is_wuw_active, state_transitions */
    {va_idle,                                 FALSE,             FALSE,         ADD_TRANSITIONS(idle_state_transitions)},
    {va_live_capture_prepared,                FALSE,             FALSE,         ADD_TRANSITIONS(live_capture_prepared_state_transitions)},
    {va_capturing,                            TRUE,              FALSE,         ADD_TRANSITIONS(capturing_state_transitions)},
#ifdef INCLUDE_WUW
    {va_wuw_detect_prepared,                  FALSE,             FALSE,         ADD_TRANSITIONS(wuw_detect_prepared_state_transitions)},
    {va_wuw_detecting,                        FALSE,             TRUE,          ADD_TRANSITIONS(wuw_detecting_state_transitions)},
    {va_wuw_detected,                         FALSE,             TRUE,          ADD_TRANSITIONS(wuw_detected_state_transitions)},
    {va_capturing_detect_pending,             TRUE,              FALSE,         ADD_TRANSITIONS(capturing_detect_pending_state_transitions)},
    /* Set wuw state as active even though it is not, mic chain still requires the resources */
    {va_live_capture_prepared_detect_pending, FALSE,             TRUE,          ADD_TRANSITIONS(live_capture_prepared_detect_pending_state_transitions)},
    {va_wuw_detecting_paused,                 FALSE,             TRUE,          ADD_TRANSITIONS(wuw_detecting_paused_transitions)},
#endif /* INCLUDE_WUW */
};

static bool kymera_VaMicDisconnectInd(const mic_change_info_t *info);
static void kymera_VaMicReconnectedInd(void);
static mic_user_state_t kymera_VaMicGetState(void);
static bool kymera_VaMicGetAecRefUsage(void);

static const mic_callbacks_t mic_callbacks =
{
    .MicDisconnectIndication = kymera_VaMicDisconnectInd,
    .MicGetConnectionParameters = Kymera_GetVaMicChainMicConnectionParams,
    .MicReconnectedIndication = kymera_VaMicReconnectedInd,
    .MicGetUserState = kymera_VaMicGetState,
    .MicGetAecRefUsage = kymera_VaMicGetAecRefUsage,
};

static mic_user_state_t mic_user_state = mic_user_state_interruptible;
#ifdef KYMERA_VA_USE_CHAIN_WITHOUT_CVC
static const bool mic_use_aec_ref = FALSE;
#else
static const bool mic_use_aec_ref = TRUE;
#endif

static const mic_registry_per_user_t mic_registration =
{
    .user = mic_user_va,
    .callbacks = &mic_callbacks,
    .permanent.mandatory_mic_ids = NULL,
    .permanent.num_of_mandatory_mics = 0,
    .permanent.activate_timestamps = TRUE,
};

static va_states_t current_state = va_idle;

static const state_t * kymera_GetStateInfo(va_states_t state)
{
    unsigned i;
    for(i = 0; i < ARRAY_DIM(states); i++)
    {
        if (states[i].state_id == state)
            return &states[i];
    }
    Panic();

    return NULL;
}

static const state_transition_t * kymera_GetStateTransition(va_states_t state, va_events_t event)
{
    unsigned i;
    const state_t *state_info = kymera_GetStateInfo(state);

    if (state_info)
    {
        for(i = 0; i < state_info->number_of_state_transitions; i++)
        {
            if (state_info->state_transitions[i].event == event)
            {
                return &state_info->state_transitions[i];
            }
        }
    }

    return NULL;
}

static void kymera_ExecuteActions(const void *event_params, const enterStateAction *actions, unsigned number_of_actions)
{
    unsigned i;
    for(i = 0; i < number_of_actions; i++)
    {
        if (actions[i])
        {
            actions[i](event_params);
        }
    }
}

static bool kymera_UpdateVaState(va_events_t event, const void *event_params)
{
    const state_transition_t *transition = kymera_GetStateTransition(current_state, event);

    if (transition)
    {
        DEBUG_LOG_STATE("kymera_UpdateVaState: enum:va_events_t:%d, transition from enum:va_states_t:%d to enum:va_states_t:%d",
                        event, current_state, transition->new_state);
        /* Updating the state first since the new state must be used for any DSP clock adjustments */
        current_state = transition->new_state;
        kymera_ExecuteActions(event_params, transition->actions, ARRAY_DIM(transition->actions));
        return TRUE;
    }
    else
    {
        DEBUG_LOG_WARN("kymera_UpdateVaState: enum:va_events_t:%d, NO TRANSITION FOUND from enum:va_states_t:%d",
                        event, current_state);
        return FALSE;
    }
}

static bool kymera_VaMicDisconnectInd(const mic_change_info_t *info)
{
    UNUSED(info);
    PanicFalse(kymera_UpdateVaState(mic_stop, NULL));
    return TRUE;
}

static void kymera_VaMicReconnectedInd(void)
{
    PanicFalse(kymera_UpdateVaState(mic_start, NULL));
}

static mic_user_state_t kymera_VaMicGetState(void)
{
    return mic_user_state;
}

static bool kymera_VaMicGetAecRefUsage(void)
{
    return mic_use_aec_ref;
}

static void kymera_SetAsInterruptibleMicUser(const void *params)
{
    UNUSED(params);
    mic_user_state = mic_user_state_interruptible;
    Kymera_MicUserUpdatedState(mic_user_va);
}

static void kymera_SetAsUninterruptibleMicUser(const void *params)
{
    UNUSED(params);
    mic_user_state = mic_user_state_non_interruptible;
    Kymera_MicUserUpdatedState(mic_user_va);
}

bool Kymera_PrepareVaLiveCapture(const va_audio_voice_capture_params_t *params)
{
    return kymera_UpdateVaState(live_capture_prepare, params);
}

Source Kymera_StartVaLiveCapture(const va_audio_voice_capture_params_t *params)
{
    Source source = NULL;

    if (kymera_UpdateVaState(live_capture_start, params))
    {
        source = PanicNull(Kymera_GetVaEncodeChainOutput());
    }

    return source;
}

bool Kymera_StopVaCapture(void)
{
    return kymera_UpdateVaState(capture_stop, NULL);
}

bool Kymera_PrepareVaWuwDetection(Task wuw_detection_handler, const va_audio_wuw_detection_params_t *params)
{
    wuw_detection_start_t wuw_params = {.handler = wuw_detection_handler, .params = params};
    return kymera_UpdateVaState(wuw_detect_prepare, &wuw_params);
}

bool Kymera_StartVaWuwDetection(Task wuw_detection_handler, const va_audio_wuw_detection_params_t *params)
{
    wuw_detection_start_t wuw_params = {.handler = wuw_detection_handler, .params = params};
    return kymera_UpdateVaState(wuw_detect_start, &wuw_params);
}

bool Kymera_StopVaWuwDetection(void)
{
    return kymera_UpdateVaState(wuw_detect_stop, NULL);
}

bool Kymera_VaWuwDetected(void)
{
    TimestampEvent(TIMESTAMP_EVENT_WUW_DETECTED);
    return kymera_UpdateVaState(wuw_detected, NULL);
}

Source Kymera_StartVaWuwCapture(const va_audio_wuw_capture_params_t *params)
{
    Source source = NULL;

    if (kymera_UpdateVaState(wuw_capture_start, params))
    {
        source = PanicNull(Kymera_GetVaEncodeChainOutput());
    }

    return source;
}

bool Kymera_IgnoreDetectedVaWuw(void)
{
    return kymera_UpdateVaState(wuw_ignore_detected, NULL);
}

bool Kymera_IsVaCaptureActive(void)
{
    return kymera_GetStateInfo(current_state)->is_capture_active;
}

bool Kymera_IsVaWuwDetectionActive(void)
{
    return kymera_GetStateInfo(current_state)->is_wuw_active;
}

bool Kymera_IsVaActive(void)
{
    return Kymera_IsVaCaptureActive() || Kymera_IsVaWuwDetectionActive();
}

audio_dsp_clock_type Kymera_VaGetMinDspClock(void)
{
    audio_dsp_clock_type min_dsp_clock = AUDIO_DSP_VERY_SLOW_CLOCK;

    if (Kymera_IsVaCaptureActive())
    {
        min_dsp_clock = AUDIO_DSP_TURBO_CLOCK;
    }
    else if (Kymera_IsVaWuwDetectionActive() && !Kymera_VaIsLowPowerEnabled())
    {
        min_dsp_clock = AUDIO_DSP_BASE_CLOCK;
    }

    return min_dsp_clock;
}

void Kymera_VaGetMinLpClockSpeedMhz(uint8 *min_lp_clk_speed_mhz, uint8 *min_very_lp_clk_speed_mhz)
{
    *min_lp_clk_speed_mhz = DEFAULT_LOW_POWER_CLK_SPEED_MHZ;
    *min_very_lp_clk_speed_mhz = DEFAULT_VERY_LOW_POWER_CLK_SPEED_MHZ;

    if (!Kymera_WuwEngineSupportsDefaultLpClock() && Kymera_VaIsLowPowerEnabled())
    {
        *min_lp_clk_speed_mhz = BOOSTED_LOW_POWER_CLK_SPEED_MHZ;
        *min_very_lp_clk_speed_mhz = BOOSTED_VERY_LOW_POWER_CLK_SPEED_MHZ;
    }
}

void Kymera_VaInit(void)
{
    Kymera_MicRegisterUser(&mic_registration);
}

bool Kymera_VaSetDspClock(audio_dsp_clock_type active_mode, audio_dsp_clock_type trigger_mode)
{
    return Kymera_VaWuwChainSetDspClock(active_mode, trigger_mode);
}

#endif /*#ifdef INCLUDE_VOICE_UI */
