/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_anc_common.c
\brief      Implementation of kymera anc common related functionality, controls AANCv2, Adaptive Ambient,
            Static ANC and Static leakthrough audio graphs for both standalone and concurrency use cases.
*/
#include <app/audio/audio_if.h>
#include <operators.h>
#include <cap_id_prim.h>
#include "kymera_anc_common.h"
#include "kymera.h"
#include "macros.h"
#include "kymera_aah.h"
#include "kymera_basic_passthrough.h"
#include "kymera_ahm.h"
#include "kymera_hcgr.h"
#include "kymera_data.h"
#include "kymera_dsp_clock.h"
#include "kymera_state.h"
#include "kymera_mic_if.h"
#include "kymera_output_if.h"
#include "kymera_setup.h"
#include "kymera_internal_msg_ids.h"
#include "panic.h"
#include "anc_state_manager.h"
#include "kymera_wind_detect.h"
#include "kymera_echo_canceller.h"
#include "kymera_adaptive_anc.h"
#include "kymera_adaptive_ambient.h"
#include "kymera_output_ultra_quiet_dac.h"
#include "kymera_anc.h"
#include "stream.h"
#include "wind_detect.h"
#include "kymera_self_speech_detect.h"
#include "kymera_noise_id.h"
#include "kymera_va.h"
#include "anc_config.h"
#include "kymera_fit_test.h"

#ifdef ENABLE_ADAPTIVE_ANC
#define KYMERA_ANC_COMMON_SAMPLE_RATE (16000U)
#define MAX_ANC_COMMON_MICS (2U)
#define MAKE_MESSAGE(TYPE)  TYPE##_T *message = PanicUnlessNew(TYPE##_T);
#define GET_REQUIRED_ANC_MICS (2U)
#define NONE_DB (-200 * 512)
#define SPLITTER_BUFFER_SIZE 256

/*! \brief AANC config type. */
typedef enum
{
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    aanc_config_none,
    aanc_config_anc,
    aanc_config_leakthrough,
    aanc_config_fit_test,
#else
    aanc_config_none,
    aanc_config_adaptive_anc,
    aanc_config_adaptive_ambient,
    aanc_config_static_anc,
    aanc_config_static_leakthrough,
    aanc_config_fit_test,
#endif
} aanc_config_t;

typedef enum
{
    aanc_state_disable_initiated,
    aanc_state_idle,
    aanc_state_enable_initiated,
    aanc_state_enabled
}aanc_state_t;

typedef enum
{
    aanc_ucid_offset_default = 0,                       /* 2-mic: FF, FB */
    aanc_ucid_offset_auto_transparency = 1,             /* 3-mic: BCM, FF, FB */
    aanc_ucid_offset_wind = 2,                          /* 3-mic: FF, FB, Voice */
    aanc_ucid_offset_wind_and_auto_transparency = 3,    /* 4-mic: BCM, FF, FB, Voice */
} aanc_ucid_offset;

typedef struct
{
   uint8 lpf_shift_1;
   uint8 lpf_shift_2;
} kymera_anc_common_lpf_config_t;

typedef struct
{
   uint8 filter_shift;
   uint8 filter_enable;
} kymera_anc_common_dc_config_t;

typedef kymera_anc_common_dc_config_t kymera_anc_common_small_lpf_config_t;

typedef struct
{
   kymera_anc_common_lpf_config_t lpf_config;
   kymera_anc_common_dc_config_t dc_filter_config;
} kymera_anc_common_feed_forward_config_t;

typedef struct
{
   kymera_anc_common_lpf_config_t lpf_config;
} kymera_anc_common_feed_back_config_t;

typedef struct
{
   kymera_anc_common_feed_forward_config_t ffa_config;
   kymera_anc_common_feed_forward_config_t ffb_config;
   kymera_anc_common_feed_back_config_t fb_config;
   kymera_anc_common_small_lpf_config_t small_lpf_config;
} kymera_anc_common_anc_instance_config_t;

typedef struct
{
   anc_mode_t mode;
   kymera_anc_common_anc_instance_config_t anc_instance_0_config;
   kymera_anc_common_anc_instance_config_t anc_instance_1_config;
} kymera_anc_common_anc_filter_config_t;

typedef enum
{
    KYMERA_ANC_COMMON_INTERNAL_AHM_TRANSITION_TIMEOUT = INTERNAL_MESSAGE_BASE,
    KYMERA_ANC_COMMON_INTERNAL_AHM_RAMP_TIMEOUT,
    KYMERA_ANC_COMMON_INTERNAL_MODE_TRANSITION_AHM_RAMP_DOWN_TIMEOUT,
    KYMERA_ANC_COMMON_INTERNAL_ENABLE_BYPASS_ANCCOMPANDER_TIMEOUT,
    KYMERA_ANC_COMMON_INTERNAL_DISABLE_BYPASS_ANCCOMPANDER_TIMEOUT,
    KYMERA_ANC_COMMON_INTERNAL_START_EFT_DELAYED
} kymera_anc_common_internal_message_ids;

static void kymeraAncCommon_HandleMessage(Task task, MessageId id, Message message);
static const TaskData kymera_anc_common_task = { .handler=kymeraAncCommon_HandleMessage };

#define kymeraAncCommon_GetTask() (Task)&kymera_anc_common_task

typedef struct
{
   aanc_config_t aanc_config;
   KYMERA_INTERNAL_AANC_ENABLE_T mode_settings;
   bool concurrency_req; /* Flag indicating standalone/concurrency anc chain has to be brought up during anc enable user-event*/
   bool transition_in_progress;/* Flag indicating standalone to concurrency or concurrency to standalone chain setup is in progress*/
   bool wind_detect_in_stage2;/* Can be in Stage 2 if there is Stage 1 Attack or in SCO concurrency*/
   bool wind_confirmed;/* Wind is detected and AHM sysmode is set to Wind */
   bool mode_change_in_progress; /*Flag indicating mode change progress to disallow self speech mic disconnection and connection */
   /*Flag indicating enable in progress to allow ramping during sequences other than enable eg., concurrrency transition, self speech enable/disable, WND 1-mic detection */
   bool enable_in_progress;
   kymera_anc_common_anc_filter_config_t* prev_filter_config;
   Operator op_consumer_ff, op_consumer_fb;
   output_users_t connecting_user;/*Used to check if self speech can be enabled or not based on the connecting concurrency user*/
   aanc_state_t aanc_state;
   anc_filter_topology_t filter_topology; /* ANC filter topology */
   Operator ref_splitter_op; /* To split reference signal and pass it to AANC2 operator */
   bool adaptive_mode; /* Flag indicating if Adaptive ANC or adaptive ambient is set */
} kymera_anc_common_data_t;

static kymera_anc_common_data_t data =
{
    .filter_topology = ancConfigFilterTopology(),
};
#define getData() (&data)

typedef void (*kymeraAncCommonCallback)(void);
kymeraAncCommonCallback ahmRampingCompleteCallback = {NULL};

/***************************************
 ********* Static Functions ************
 ***************************************/
static void kymeraAncCommon_RampGainsToMute(kymeraAncCommonCallback func);
static void kymeraAncCommon_RampGainsToQuiet(kymeraAncCommonCallback func);
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
static void kymeraAncCommon_RampGainsToStatic(kymeraAncCommonCallback func);
#endif
static void kymeraAncCommon_SetAancCurrentConfig(aanc_config_t aanc_config);
static aanc_config_t kymeraAncCommon_GetAancCurrentConfig(void);
static aanc_config_t kymeraAncCommon_IsAancCurrentConfigValid(void);
static void kymeraAncCommon_UpdateConcurrencyRequest(bool enable);
static bool kymeraAncCommon_isTransitionInProgress(void);
static void kymeraAncCommon_AhmDestroy(void);
static void kymeraAncCommon_AhmCreate(void);
static bool kymeraAncCommon_IsConcurrencyRequested(void);
static void kymeraAncCommon_StoreModeSettings(const KYMERA_INTERNAL_AANC_ENABLE_T *mode_params);
static void kymeraAncCommon_SetKymeraState(void);
static void kymeraAncCommon_ResetKymeraState(void);
static uint16 kymeraAncCommon_GetAncFeedforwardMicConfig(void);
static uint16 kymeraAncCommon_GetAncFeedbackMicConfig(void);
static Sink kymeraAncCommon_GetFeedforwardPathSink(fbc_config_t fbc_config);
static Sink kymeraAncCommon_GetFeedbackPathSink(void);
static Sink kymeraAncCommon_GetPlaybackPathSink(void);
static bool kymeraAncCommon_IsAncModeFitTest(anc_mode_t mode);
static aanc_config_t kymeraAncCommon_GetAancConfigFromMode(anc_mode_t mode);
static void kymeraAncCommon_FreezeAdaptivity(void);
static void kymeraAncCommon_StartAdaptivity(void);
static void kymeraAncCommon_Create(void);
static void kymeraAncCommon_Configure(const KYMERA_INTERNAL_AANC_ENABLE_T *msg);
static void kymeraAncCommon_WindDetectConnect(fbc_config_t fbc_config);
static void kymeraAncCommon_BasicPassthroughConnect(void);
static void kymeraAncCommon_Connect(void);
static void kymeraAncCommon_Start(void);
static void kymeraAncCommon_Stop(void);
static void kymeraAncCommon_Destroy(void);
static void kymeraAncCommon_Disconnect(void);
static void kymeraAncCommon_Enable(const KYMERA_INTERNAL_AANC_ENABLE_T *msg);
static void kymeraAncCommon_EnableOnUserRequest(const KYMERA_INTERNAL_AANC_ENABLE_T *msg);
static void kymeraAncCommon_Disable(void);
static void kymeraAncCommon_DisableOnUserRequest(void);
static void kymeraAncCommon_StandaloneToConcurrencyChain(void);
static void kymeraAncCommon_ConcurrencyToStandaloneChain(void);
static void kymeraAncCommon_ApplyModeChangeSettings(KYMERA_INTERNAL_AANC_ENABLE_T *mode_params);
static bool kymeraAncCommon_MicGetConnectionParameters(uint16 *mic_ids, Sink *mic_sinks, uint8 *num_of_mics, uint32 *sample_rate, Sink *aec_ref_sink);
static bool kymeraAncCommon_MicDisconnectIndication(const mic_change_info_t *info);
static void kymeraAncCommon_MicReconnectedIndication(void);
static void kymeraAncCommon_AdaptiveAncOutputConnectingIndication(output_users_t connecting_user, output_connection_t connection_type);
static void kymeraAncCommon_AdaptiveAncOutputIdleIndication(void);

#ifndef ENABLE_UNIFIED_ANC_GRAPH
static void KymeraAncCommon_StandaloneToConcurrencyReq(void);
static void KymeraAncCommon_ConcurrencyToStandaloneReq(void);
#endif

static void kymeraAncCommon_SetAancState(aanc_state_t state);
static aanc_state_t kymeraAncCommon_GetAancState(void);

static void kymeraAncCommon_AncStopOperators(void);
static void kymeraAncCommon_AncMicDisconnect(void);
static uint16 kymeraAncCommon_GetTargetGain(void);

#if defined(ENABLE_WIND_DETECT)
static void kymeraAncCommon_SetWindConfirmed(void);
static void kymeraAncCommon_ResetWindConfirmed(void);
static bool kymeraAncCommon_IsWindConfirmed(void);
#endif

#ifndef ENABLE_ANC_FAST_MODE_SWITCH
static void kymeraAncCommon_ModeChangeActionPostGainRamp(void);
#endif

#ifdef ENABLE_ANC_FAST_MODE_SWITCH
static void kymeraAncCommon_ResetFilterConfig(kymera_anc_common_anc_filter_config_t* filter_config);
static kymera_anc_common_anc_filter_config_t* kymeraAncCommon_GetPrevFilterConfig(void);
static void kymeraAncCommon_SetPrevFilterConfig(kymera_anc_common_anc_filter_config_t* filter_config);

static void KymeraAncCommon_ReadLpfCoefficients(audio_anc_instance instance, audio_anc_path_id path, uint8* lpf_shift_1, uint8* lpf_shift_2);
static void KymeraAncCommon_ReadDcFilterCoefficients(audio_anc_instance instance, audio_anc_path_id path, uint8* filter_shift, uint8* filter_enable);
static void KymeraAncCommon_ReadSmallLpfCoefficients(audio_anc_instance instance, uint8* filter_shift, uint8* filter_enable);

static kymera_anc_common_anc_instance_config_t* KymeraAncCommon_GetInstanceConfig(kymera_anc_common_anc_filter_config_t* filter_config, audio_anc_instance instance);

static void kymeraAncCommon_RampDownCompleteActionDuringModeTransition(void);
#endif

#if defined(ENABLE_SELF_SPEECH) && defined (ENABLE_AUTO_AMBIENT)
static void kymeraAncCommon_SelfSpeechDetect_MicReConnect(void);
static void kymeraAncCommon_SelfSpeechDetect_MicDisconnect(void);
#endif
static bool kymeraAncCommon_IsModeChangeInProgress(void);

#ifdef ENABLE_WIND_DETECT
static bool kymeraAncCommon_IsWindDetectInStage2(void);
static void kymeraAncCommon_SetWindDetectInStage2(void);
static void kymeraAncCommon_ResetWindDetectInStage2(void);
static void kymeraAncCommon_RampGainsToTarget(kymeraAncCommonCallback func);
static void kymeraAncCommon_WindConfirmed(wind_detect_intensity_t intensity);
static void kymeraAncCommon_StartOnWindRelease(void);
static void kymeraAncCommon_MoveToStage1(void);
static void kymeraAncCommon_MoveToStage2(void);
#endif

static Sink kymeraAncCommon_GetRefSplitterSink(void);
static void kymeraAncCommon_CreateRefSplitter(void);
static void kymeraAncCommon_ConfigureRefSplitter(void);
static void kymeraAncCommon_ConnectRefSplitter(void);
static void kymeraAncCommon_StartRefSplitter(void);
static void kymeraAncCommon_StopRefSplitter(void);
static void kymeraAncCommon_DisconnectRefSplitter(void);
static void kymeraAncCommon_DestroyRefSplitter(void);

static void kymeraAncCommon_RampGainsToMute(kymeraAncCommonCallback func)
{
    DEBUG_LOG("kymeraAncCommon_RampGainsToMute");
    ahmRampingCompleteCallback = func;
    KymeraAhm_SetSysMode(ahm_sysmode_mute_anc);
}

#ifndef ENABLE_ANC_FAST_MODE_SWITCH
static void kymeraAncCommon_RampGainsToStatic(kymeraAncCommonCallback func)
{
    ahmRampingCompleteCallback = func;
    KymeraAhm_SetSysMode(ahm_sysmode_full);
}

static void kymeraAncCommon_RampupGains(kymeraAncCommonCallback func)
{
    ahmRampingCompleteCallback = func;

#if defined(ENABLE_WIND_DETECT)
    if(kymeraAncCommon_IsWindConfirmed())
    {
        KymeraAhm_SetSysMode(ahm_sysmode_wind);
    }
    else
#endif
    {
        KymeraAhm_SetSysMode(ahm_sysmode_full);
    }
}
#endif

static void kymeraAncCommon_RampGainsToQuiet(kymeraAncCommonCallback func)
{
    DEBUG_LOG("kymeraAncCommon_RampGainsToQuiet");
    ahmRampingCompleteCallback = func;
}

static void kymeraAncCommon_SetAancCurrentConfig(aanc_config_t aanc_config)
{
    data.aanc_config = aanc_config;
}

static void kymeraAncCommon_SetAdaptiveMode(bool mode)
{
    data.adaptive_mode = mode;
}

static bool kymeraAncCommon_IsAdaptiveMode(void)
{
    return data.adaptive_mode;
}

static aanc_config_t kymeraAncCommon_GetAancCurrentConfig(void)
{
    return (data.aanc_config);
}

static aanc_config_t kymeraAncCommon_IsAancCurrentConfigValid(void)
{
    return (data.aanc_config != aanc_config_none);
}

static void kymeraAncCommon_UpdateConcurrencyRequest(bool enable)
{
    data.concurrency_req = enable;
}

#ifndef ENABLE_UNIFIED_ANC_GRAPH
static void kymeraAncCommon_UpdateTransitionInProgress(bool enable)
{
   data.transition_in_progress=enable;
}
#endif

static bool kymeraAncCommon_isTransitionInProgress(void)
{
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    return FALSE;
#else
    if(data.transition_in_progress)
    {
        DEBUG_LOG_INFO("kymeraAncCommon_isTransitionInProgress: 1");
    }
    return data.transition_in_progress;
#endif
}

static void kymeraAncCommon_SetModeChangeInProgress(bool enable)
{
   data.mode_change_in_progress=enable;
}

static bool kymeraAncCommon_IsModeChangeInProgress(void)
{
    if(data.mode_change_in_progress)
    {
        DEBUG_LOG_INFO("kymeraAncCommon_IsModeChangeInProgress: 1");
    }
    return data.mode_change_in_progress;
}

static void kymeraAncCommon_SetEnableInProgress(bool enable)
{
   data.enable_in_progress = enable;
}

static bool kymeraAncCommon_IsEnableInProgress(void)
{
    return data.enable_in_progress;
}

static void kymeraAncCommon_AhmDestroy(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() && KymeraAhm_IsActive())
    {
        if (!kymeraAncCommon_IsModeChangeInProgress())
        {
            DEBUG_LOG("kymeraAncCommon_AhmDestroy");
            KymeraAhm_Destroy();
        }
    }
}

static void kymeraAncCommon_AANCDestroy(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() && KymeraAdaptiveAnc_IsActive())
    {
        KymeraAdaptiveAnc_DestroyChain();
    }
}

static void kymeraAncCommon_HcgrDestroy(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() && KymeraHcgr_IsActive())
    {
        KymeraHcgr_Destroy();
    }
}

static void kymeraAncCommon_WindDetectDestroy(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() && !kymeraAncCommon_IsModeChangeInProgress() && KymeraWindDetect_IsActive())
    {
        DEBUG_LOG("kymeraAncCommon_WindDetectDestroy");
        KymeraWindDetect_Destroy();
    }
}

static void kymeraAncCommon_NoiseIDDestroy(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() && KymeraNoiseID_IsActive())
    {
        KymeraNoiseID_Destroy();
    }
}

static void kymeraAncCommon_AdaptiveAmbientDestroy(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() && KymeraAdaptiveAmbient_IsActive())
    {
        KymeraAdaptiveAmbient_DestroyChain();
    }
}

static void kymeraAncCommon_AhmCreate(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() && !KymeraAhm_IsActive())
    {
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
        if (!kymeraAncCommon_IsModeChangeInProgress())
#endif
        {
            KymeraAhm_Create();
        }
    }
}

static void kymeraAncCommon_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    DEBUG_LOG_INFO("kymeraAncCommon_HandleMessage, id: enum:kymera_anc_common_internal_message_ids:%d", id);

    switch(id)
    {
    case KYMERA_ANC_COMMON_INTERNAL_AHM_TRANSITION_TIMEOUT:
        DEBUG_LOG_INFO("kymeraAncCommon_HandleMessage, Transition timer expiry");
        KymeraAncCommon_TransitionCompleteAction();
        break;

    case KYMERA_ANC_COMMON_INTERNAL_MODE_TRANSITION_AHM_RAMP_DOWN_TIMEOUT:
        DEBUG_LOG_INFO("kymeraAncCommon_HandleMessage, Mode Transition Ramp Down timer expiry");
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
        KymeraAncCommon_RampCompleteAction();
#endif
        break;

    case KYMERA_ANC_COMMON_INTERNAL_AHM_RAMP_TIMEOUT:
         KymeraAncCommon_AhmRampExpiryAction();
    break;

    case KYMERA_ANC_COMMON_INTERNAL_ENABLE_BYPASS_ANCCOMPANDER_TIMEOUT:
        KymeraAdaptiveAmbient_EnableBypassAncCompanderParam();
    break;

    case KYMERA_ANC_COMMON_INTERNAL_DISABLE_BYPASS_ANCCOMPANDER_TIMEOUT:
        if (KymeraAdaptiveAmbient_GetBypassAncCompanderParam())
        {
            KymeraAdaptiveAmbient_DisableBypassAncCompanderParam();
        }
    break;

    case KYMERA_ANC_COMMON_INTERNAL_START_EFT_DELAYED:
        KymeraFitTest_ContinuousEnableEftMicClient();
    break;
    }
}

static void kymeraAncCommon_HcgrCreate(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() &&!KymeraHcgr_IsActive())
    {
        KymeraHcgr_Create();
    }
}

static void kymeraAncCommon_WindDetectCreate(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() && !kymeraAncCommon_IsModeChangeInProgress() && !KymeraWindDetect_IsActive())
    {
        KymeraWindDetect_Create();
    }
}

static void kymeraAncCommon_WindDetectConfigure(const KYMERA_INTERNAL_AANC_ENABLE_T *msg)
{
    if(!kymeraAncCommon_isTransitionInProgress() && !kymeraAncCommon_IsModeChangeInProgress() && KymeraWindDetect_IsActive())
    {
        KymeraWindDetect_Configure(msg);
    }
}

static void kymeraAncCommon_NoiseIDCreate(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() && !KymeraNoiseID_IsActive() && AncNoiseId_IsFeatureSupported())
    {
        KymeraNoiseID_Create();
    }
}

static void kymeraAncCommon_AANCCreate(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() && !KymeraAdaptiveAnc_IsActive())
    {
        KymeraAdaptiveAnc_CreateChain();
    }
}

static void kymeraAncCommon_AdaptiveAmbientCreate(void)
{
    if(!kymeraAncCommon_isTransitionInProgress() &&!KymeraAdaptiveAmbient_IsActive())
    {
        KymeraAdaptiveAmbient_CreateChain();
    }
}

static bool kymeraAncCommon_IsConcurrencyRequested(void)
{
    return data.concurrency_req;
}

/*Mode settings are used during concurrency/standalone transitions*/
static void kymeraAncCommon_StoreModeSettings(const KYMERA_INTERNAL_AANC_ENABLE_T *mode_params)
{
    if(mode_params)
    {
        getData()->mode_settings = *mode_params;
    }
}

static void kymeraAncCommon_SetKymeraState(void)
{
    if(!appKymeraIsBusy())
        appKymeraSetState(KYMERA_STATE_ADAPTIVE_ANC_STARTED);
}

static void kymeraAncCommon_ResetKymeraState(void)
{
    if((appKymeraGetState() == KYMERA_STATE_ADAPTIVE_ANC_STARTED))
        appKymeraSetState(KYMERA_STATE_IDLE);
}

static uint16 kymeraAncCommon_GetAncFeedforwardMicConfig(void)
{
    return appConfigAncFeedForwardMic();
}

static uint16 kymeraAncCommon_GetAncFeedbackMicConfig(void)
{
    return appConfigAncFeedBackMic();
}

/*! \brief Get the Adaptive ANC Feed Forward mic Sink
*/
static Sink kymeraAncCommon_GetAancFFMicPathSink(void)
{
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    return ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_IN);
#else
    if(KymeraEchoCanceller_IsActive() && ancConfigIncludeFFPathFbc())
    {
        return KymeraEchoCanceller_GetFFMicPathSink();
    }
    else
    {
        return ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_IN);
    }
#endif
}

static Sink kymeraAncCommon_GetFeedforwardPathSink(fbc_config_t fbc_config)
{
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
        case aanc_config_anc:
            DEBUG_LOG("kymeraAncCommon_GetFeedforwardPathSink: ANC");
            if(KymeraAah_IsFeatureSupported())
            {
                return KymeraAah_GetFFMicPathSink();
            }
            else if(fbc_config != fb_path_only)
            {
                return KymeraEchoCanceller_GetFFMicPathSink();
            }
            else if(KymeraWindDetect_IsFeatureSupported())
            {
                return KymeraWindDetect_GetFFMicPathSink();
            }
            else if(AncNoiseId_IsFeatureSupported())
            {
                return KymeraNoiseID_GetFFMicPathSink();
            }
            else
            {
                return kymeraAncCommon_GetAancFFMicPathSink();
            }

        case aanc_config_leakthrough:
            DEBUG_LOG("kymeraAncCommon_GetFeedforwardPathSink Transparency");
            if(KymeraAah_IsFeatureSupported())
            {
                return KymeraAah_GetFFMicPathSink();
            }
            else if(fbc_config != fb_path_only)
            {
                return KymeraEchoCanceller_GetFFMicPathSink();
            }
            else if(KymeraWindDetect_IsFeatureSupported())
            {
                return KymeraWindDetect_GetFFMicPathSink();
            }
            else
            {
                return KymeraAdaptiveAmbient_GetFFMicPathSink();
            }

        case aanc_config_fit_test:
            return kymeraAncCommon_GetAancFFMicPathSink();

        default:
            return NULL;
    }
#else
    UNUSED(fbc_config);

    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
        case aanc_config_adaptive_anc:
#if defined(ENABLE_ANC_AAH)
        return KymeraAah_GetFFMicPathSink();
#else
        return kymeraAncCommon_GetAancFFMicPathSink();
#endif

        case aanc_config_static_anc:
#if defined(ENABLE_ANC_AAH)
        return KymeraAah_GetFFMicPathSink();
#else
#ifdef ENABLE_WIND_DETECT
    return KymeraWindDetect_GetFFMicPathSink();
#else
    if(AncNoiseId_IsFeatureSupported())
    {
        return KymeraNoiseID_GetFFMicPathSink();
    }
    else
    {
        return NULL;
    }
#endif
#endif
        case aanc_config_adaptive_ambient:
#if defined(ENABLE_ANC_AAH)
        return KymeraAah_GetFFMicPathSink();
#else
        return KymeraAdaptiveAmbient_GetFFMicPathSink();
#endif

        case aanc_config_static_leakthrough:
#if defined(ENABLE_ANC_AAH)
            return KymeraAah_GetFFMicPathSink();

#elif defined(ENABLE_WIND_DETECT)
            return KymeraWindDetect_GetFFMicPathSink();
#else
            return NULL;
#endif

        case aanc_config_fit_test:
            return kymeraAncCommon_GetAancFFMicPathSink();

        default:
            return NULL;
    }
#endif
}

static Sink kymeraAncCommon_GetFeedbackPathSink(void)
{
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
        case aanc_config_leakthrough:
            DEBUG_LOG("kymeraAncCommon_GetFeedbackPathSink ANC or transparency");
            if(KymeraAah_IsFeatureSupported())
            {
                return KymeraAah_GetFBMicPathSink();
            }
            else if(KymeraEchoCanceller_IsActive())
            {
                return KymeraEchoCanceller_GetFBMicPathSink();
            }
            else
            {
                return KymeraHcgr_GetFbMicPathSink();
            }
#else
        case aanc_config_adaptive_anc:
#if defined(ENABLE_ANC_AAH)
        return KymeraAah_GetFBMicPathSink();
#else
        if(KymeraEchoCanceller_IsActive())
        {
            return KymeraEchoCanceller_GetFBMicPathSink();
        }
        else
        {
            return ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_ERR_MIC_IN);
        }
#endif

        case aanc_config_static_anc:
        case aanc_config_adaptive_ambient:
        case aanc_config_static_leakthrough:
#if defined(ENABLE_ANC_AAH)
        return KymeraAah_GetFBMicPathSink();
#else
        if(KymeraEchoCanceller_IsActive())
        {
            return KymeraEchoCanceller_GetFBMicPathSink();
        }
        else
        {
            return KymeraHcgr_GetFbMicPathSink();
        }

#endif
#endif
        case aanc_config_fit_test:
            return ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_ERR_MIC_IN);

        default:
            return NULL;
    }
}

static Sink kymeraAncCommon_GetPlaybackPathSink(void)
{
    DEBUG_LOG("kymeraAncCommon_GetPlaybackPathSink");
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    if(KymeraAah_IsActive())
    {
        return KymeraAah_GetRefPathSink();
    }
    else if(KymeraEchoCanceller_IsActive())
    {
        if (kymeraAncCommon_GetAancCurrentConfig() == aanc_config_anc)
        {
            return kymeraAncCommon_GetRefSplitterSink();
        }
        else
        {
            return KymeraEchoCanceller_GetSpkRefPathSink();
        }
    }
    return NULL;
#else
    if(KymeraEchoCanceller_IsActive())
    {
#if defined(ENABLE_ANC_AAH)
        return KymeraAah_GetRefPathSink();
#else
        if (kymeraAncCommon_GetAancCurrentConfig() == aanc_config_adaptive_anc)
        {
            return kymeraAncCommon_GetRefSplitterSink();
        }
        else
        {
            return KymeraEchoCanceller_GetSpkRefPathSink();
        }
#endif
    }
    return NULL;
#endif
}

static bool kymeraAncCommon_IsAncModeFitTest(anc_mode_t mode)
{
    return (mode == anc_mode_fit_test);
}

static aanc_config_t kymeraAncCommon_GetAancConfigFromMode(anc_mode_t mode)
{
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    if(kymeraAncCommon_IsAncModeFitTest(mode))
    {
        DEBUG_LOG("kymeraAncCommon_GetAancConfigFromMode: FitTest");
        return aanc_config_fit_test;
    }
    else if(!AncConfig_IsAncModeLeakThrough(mode))
    {
        DEBUG_LOG("kymeraAncCommon_GetAancConfigFromMode: ANC");
        return aanc_config_anc;
    }
    else
    {
        DEBUG_LOG("kymeraAncCommon_GetAancConfigFromMode: Transparency");
        return aanc_config_leakthrough;
    }
#else
    if(kymeraAncCommon_IsAncModeFitTest(mode))
    {
        return aanc_config_fit_test;
    }
    else if(AncConfig_IsAncModeAdaptive(mode))
    {
        if(!AncConfig_IsAncModeLeakThrough(mode))
            return aanc_config_adaptive_anc;
        else
            return aanc_config_adaptive_ambient;
    }
    else
    {
        if(!AncConfig_IsAncModeLeakThrough(mode))
            return aanc_config_static_anc;
        else
            return aanc_config_static_leakthrough;
    }
#endif
}

static void kymeraAncCommon_AdaptiveAncFreezeAdaptivity(void)
{
    if (KymeraAdaptiveAnc_IsActive())
    {
        KymeraAdaptiveAnc_SetSysMode(adaptive_anc_sysmode_freeze);
    }
}

static void kymeraAncCommon_AdaptiveAncStartAdaptivity(void)
{
    adaptive_ancv2_sysmode_t sys_mode = adaptive_anc_sysmode_full;

    if (kymeraAncCommon_IsConcurrencyRequested())
    {
        sys_mode = adaptive_anc_sysmode_concurrency;
    }

    if (KymeraAdaptiveAnc_IsActive())
    {
        KymeraAdaptiveAnc_SetSysMode(sys_mode);
    }
}

static void kymeraAncCommon_EnableBypassAncCompanderParam(bool enable, uint16 timeout)
{
    kymera_anc_common_internal_message_ids msg_id = (enable)?(KYMERA_ANC_COMMON_INTERNAL_ENABLE_BYPASS_ANCCOMPANDER_TIMEOUT):(KYMERA_ANC_COMMON_INTERNAL_DISABLE_BYPASS_ANCCOMPANDER_TIMEOUT);

    MessageCancelAll(kymeraAncCommon_GetTask(), msg_id);
    /*Inform Kymera module to configure AHM and update filters after timeout */
    MessageSendLater(kymeraAncCommon_GetTask(), msg_id, NULL, timeout);
}

static void kymeraAncCommon_SetAncCompanderSysMode(anc_compander_sysmode_t mode)
{
    if(mode == anc_compander_sysmode_full)
    {
        if(kymeraAncCommon_isTransitionInProgress())
        {
            /* Audio workaround: allow Anc compander to stabilize */
            KymeraAdaptiveAmbient_EnableBypassAncCompanderParam();
            KymeraAdaptiveAmbient_SetAncCompanderSysMode(anc_compander_sysmode_full);
            kymeraAncCommon_EnableBypassAncCompanderParam(FALSE, ANC_COMPANDER_BYPASS_RELEASE_TIMEOUT_MS);
        }
        else
        {
            KymeraAdaptiveAmbient_SetAncCompanderSysMode(anc_compander_sysmode_full);
        }
    }
    else
    {
        KymeraAdaptiveAmbient_SetAncCompanderSysMode(mode);
    }
}

static void kymeraAncCommon_FreezeAdaptivity(void)
{
    DEBUG_LOG("kymeraAncCommon_FreezeAdaptivity");
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
            kymeraAncCommon_AdaptiveAncFreezeAdaptivity();
        break;

        case aanc_config_leakthrough:
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
            if (kymeraAncCommon_IsModeChangeInProgress())
            {
                KymeraAdaptiveAmbient_EnableBypassAncCompanderParam();
            }
#endif
            kymeraAncCommon_SetAncCompanderSysMode(anc_compander_sysmode_passthrough);
        break;
#else
        case aanc_config_adaptive_anc:
            kymeraAncCommon_AdaptiveAncFreezeAdaptivity();
        break;

        case aanc_config_adaptive_ambient:
            kymeraAncCommon_SetAncCompanderSysMode(anc_compander_sysmode_passthrough);
        break;
#endif
        default:
        /* do nothing */
            break;
    }
}

static void kymeraAncCommon_StartAdaptivity(void)
{
    DEBUG_LOG("kymeraAncCommon_StartAdaptivity");
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
            kymeraAncCommon_AdaptiveAncStartAdaptivity();
        break;
        case aanc_config_leakthrough:
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
            if (kymeraAncCommon_IsModeChangeInProgress())
            {
                if (KymeraAdaptiveAmbient_GetBypassAncCompanderParam())
                {
                    KymeraAdaptiveAmbient_DisableBypassAncCompanderParam();
                }
            }
#endif
            kymeraAncCommon_SetAncCompanderSysMode(anc_compander_sysmode_full);
        break;
#else
        case aanc_config_adaptive_anc:
            kymeraAncCommon_AdaptiveAncStartAdaptivity();
        break;

        case aanc_config_adaptive_ambient:
            kymeraAncCommon_SetAncCompanderSysMode(anc_compander_sysmode_full);
        break;
#endif
        default:
        /* do nothing */
            break;
    }
}

#if defined(ENABLE_UNIFIED_ANC_GRAPH) || defined(ENABLE_WIND_DETECT) || !defined(ENABLE_ANC_FAST_MODE_SWITCH)
static void kymeraAncCommon_StartOrFreezeAdaptivity(void)
{
    if(kymeraAncCommon_IsAdaptiveMode())
    {
        kymeraAncCommon_StartAdaptivity();
    }
    else
    {
        kymeraAncCommon_FreezeAdaptivity();
    }
}
#endif

static anc_filter_topology_t kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(aanc_config_t current_config)
{
    /* So far dual gain control functionality has supported only by AANC capability. Hence, rest of ANC modes will be updated with either parallel/single topology */
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    if (current_config == aanc_config_anc)
    {
        return (data.filter_topology);
    }
    else
    {
        return ((data.filter_topology == anc_dual_filter_topology) ? (anc_parallel_filter_topology) : (data.filter_topology));
    }
#else

    if (current_config == aanc_config_adaptive_anc)
    {
        return (data.filter_topology);
    }
    else
    {
        return ((data.filter_topology == anc_dual_filter_topology) ? (anc_parallel_filter_topology) : (data.filter_topology));
    }
#endif
}

#ifdef ENABLE_ANC_FAST_MODE_SWITCH

static void kymeraAncCommon_AhmConfigure(const KYMERA_INTERNAL_AANC_ENABLE_T *msg, anc_filter_topology_t filter_topology)
{
    if (!kymeraAncCommon_IsModeChangeInProgress())
    {
        KymeraAhm_Configure(msg, filter_topology);

        /* Cancel any pending AHM Transition timeout messages */
        MessageCancelAll(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_AHM_TRANSITION_TIMEOUT);

        /*Inform Kymera module to put AHM in full proc after timeout */
        MessageSendLater(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_AHM_TRANSITION_TIMEOUT, NULL, AHM_TRANSITION_TIMEOUT_MS);
    }
}

#else

static void kymeraAncCommon_AhmConfigure(const KYMERA_INTERNAL_AANC_ENABLE_T *msg, anc_filter_topology_t filter_topology)
{
    KymeraAhm_Configure(msg, filter_topology);
}

#endif

static void kymeraAncCommon_AhmConnect(void)
{
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
    if (!kymeraAncCommon_IsModeChangeInProgress())
#endif
    {
        KymeraAhm_Connect();
    }
}

static void kymeraAncCommon_AhmStart(void)
{
    /* AHM need to be started only during AANC Enable on user request scenario */
    if(kymeraAncCommon_GetAancState() == aanc_state_enable_initiated)
    {
        KymeraAhm_Start();
    }
}

static void kymeraAncCommon_AhmStop(void)
{
    /* AHM need to be stopped only during AANC Disable on user request scenario */
    if(kymeraAncCommon_GetAancState() == aanc_state_disable_initiated)
    {
        KymeraAhm_Stop();
    }
}

static Operator kymeraAncCommon_GetRefSplitterOperator(void)
{
    return data.ref_splitter_op;
}

static void kymeraAncCommon_SetRefSplitterOperator(Operator op)
{
    data.ref_splitter_op = op;
}

static Sink kymeraAncCommon_GetRefSplitterSink(void)
{
    Operator op = kymeraAncCommon_GetRefSplitterOperator();

    if (op)
    {
        return StreamSinkFromOperatorTerminal(data.ref_splitter_op, 0);
    }
    else
    {
        return NULL;
    }
}

static void kymeraAncCommon_CreateRefSplitter(void)
{
    PanicNotZero(data.ref_splitter_op);

    Operator op = CustomOperatorCreate(CAP_ID_SPLITTER, OPERATOR_PROCESSOR_ID_0,
                                  operator_priority_lowest, NULL);

    kymeraAncCommon_SetRefSplitterOperator(op);
}

static void kymeraAncCommon_ConfigureRefSplitter(void)
{
    Operator op = kymeraAncCommon_GetRefSplitterOperator();

    if (op)
    {
        OperatorsStandardSetBufferSize(op, SPLITTER_BUFFER_SIZE);
        OperatorsSplitterSetWorkingMode(op, splitter_mode_clone_input);
        OperatorsSplitterEnableSecondOutput(op, FALSE);
        OperatorsSplitterSetDataFormat(op, operator_data_format_pcm);
    }
}

static void kymeraAncCommon_ConnectRefSplitter(void)
{
    Operator op = kymeraAncCommon_GetRefSplitterOperator();

    if (op)
    {
        PanicNull(StreamConnect(StreamSourceFromOperatorTerminal(op, 0), KymeraEchoCanceller_GetSpkRefPathSink()));
        PanicNull(StreamConnect(StreamSourceFromOperatorTerminal(op, 1), KymeraAdaptiveAnc_GetRefPathSink()));

        OperatorsSplitterEnableSecondOutput(op, TRUE);
    }
}

static void kymeraAncCommon_StartRefSplitter(void)
{
    Operator op = kymeraAncCommon_GetRefSplitterOperator();

    if (op)
    {
        OperatorStart(op);
    }
}

static void kymeraAncCommon_StopRefSplitter(void)
{
    Operator op = kymeraAncCommon_GetRefSplitterOperator();

    if (op)
    {
        OperatorStop(op);
    }
}

static void kymeraAncCommon_DisconnectRefSplitter(void)
{
    Operator op = kymeraAncCommon_GetRefSplitterOperator();

    if (op)
    {
        StreamDisconnect(StreamSourceFromOperatorTerminal(op, 0), NULL);
        StreamDisconnect(StreamSourceFromOperatorTerminal(op, 1), NULL);
    }
}

static void kymeraAncCommon_DestroyRefSplitter(void)
{
    Operator op = kymeraAncCommon_GetRefSplitterOperator();

    if (op)
    {
        CustomOperatorDestroy(&op, 1);
        kymeraAncCommon_SetRefSplitterOperator(INVALID_OPERATOR);
    }
}

static Sink kymeraAncCommon_CreateClientConsumer(Operator *op)
{
    if(*op)
    {
        return StreamSinkFromOperatorTerminal(*op, 0);
    }
    else
    {
        DEBUG_LOG("kymeraAncCommon_CreateClientConsumer");
        *op = CustomOperatorCreate(capability_id_switched_passthrough_consumer, OPERATOR_PROCESSOR_ID_0,
                                   operator_priority_lowest, NULL);
        OperatorsSetSwitchedPassthruEncoding(*op, spc_op_format_pcm);
        return StreamSinkFromOperatorTerminal(*op, 0);

    }

}

static void kymeraAncCommon_StartClientConsumer(Operator op_ff, Operator op_fb)
{
    if(op_ff)
    {
        OperatorStart(op_ff);
    }
    if(op_fb)
    {
        OperatorStart(op_fb);
    }
}

static void kymeraAncCommon_StopClientConsumer(Operator op_ff, Operator op_fb)
{
    if(op_ff)
    {
        OperatorStop(op_ff);
    }
    if(op_fb)
    {
        OperatorStop(op_fb);
    }
}

static void kymeraAncCommon_DestroyClientConsumer(Operator *op_ff, Operator *op_fb)
{
    if(*op_ff)
    {
        CustomOperatorDestroy(op_ff, 1);
        *op_ff = INVALID_OPERATOR;
    }
    if(*op_fb)
    {
        CustomOperatorDestroy(op_fb, 1);
        *op_fb = INVALID_OPERATOR;
    }
}

static void kymeraAncCommon_Create(void)
{
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
            DEBUG_LOG("kymeraAncCommon_Create: ANC");
            KymeraBasicPassthrough_Create();
            KymeraAah_Create();
            kymeraAncCommon_HcgrCreate();
            kymeraAncCommon_AhmCreate();
            kymeraAncCommon_WindDetectCreate();
            kymeraAncCommon_NoiseIDCreate();
            kymeraAncCommon_AANCCreate();
            KymeraEchoCanceller_Create(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_CreateRefSplitter();
        break;
        case aanc_config_leakthrough:
            DEBUG_LOG("kymeraAncCommon_Create: Transparency");
            KymeraBasicPassthrough_Create();
            KymeraAah_Create();
            kymeraAncCommon_HcgrCreate();
            kymeraAncCommon_AhmCreate();
            kymeraAncCommon_WindDetectCreate();
            kymeraAncCommon_AdaptiveAmbientCreate();
            KymeraEchoCanceller_Create(ancConfigIncludeFFPathFbc());
        break;
#else
        case aanc_config_adaptive_anc:
            KymeraBasicPassthrough_Create();
            KymeraAah_Create();
            kymeraAncCommon_HcgrCreate();
            kymeraAncCommon_AhmCreate();
            kymeraAncCommon_WindDetectCreate();
            kymeraAncCommon_NoiseIDCreate();
            kymeraAncCommon_AANCCreate();
            if(kymeraAncCommon_IsConcurrencyRequested())
            {
                KymeraEchoCanceller_Create(ancConfigIncludeFFPathFbc());
                kymeraAncCommon_CreateRefSplitter();
            }
        break;

        case aanc_config_adaptive_ambient:
            KymeraBasicPassthrough_Create();
            KymeraAah_Create();
            kymeraAncCommon_HcgrCreate();
            kymeraAncCommon_AhmCreate();
            kymeraAncCommon_WindDetectCreate();
            kymeraAncCommon_AdaptiveAmbientCreate();
            if(kymeraAncCommon_IsConcurrencyRequested())
            {
                KymeraEchoCanceller_Create(fb_path_only);
            }
        break;

        case aanc_config_static_anc:
            KymeraBasicPassthrough_Create();
            KymeraAah_Create();
            kymeraAncCommon_HcgrCreate();
            kymeraAncCommon_AhmCreate();
            kymeraAncCommon_WindDetectCreate();
            kymeraAncCommon_NoiseIDCreate();
            if(kymeraAncCommon_IsConcurrencyRequested())
            {
                KymeraEchoCanceller_Create(fb_path_only);
            }
        break;

        case aanc_config_static_leakthrough:
            KymeraBasicPassthrough_Create();
            KymeraAah_Create();
            kymeraAncCommon_HcgrCreate();
            kymeraAncCommon_AhmCreate();
            kymeraAncCommon_WindDetectCreate();
            if(kymeraAncCommon_IsConcurrencyRequested())
            {
                KymeraEchoCanceller_Create(fb_path_only);
            }
        break;
#endif
        case aanc_config_fit_test:
            KymeraBasicPassthrough_Create();
            KymeraAdaptiveAnc_CreateChain();
        break;

        default:
            break;
    }

    DEBUG_LOG("kymeraAncCommon_Create, enum:aanc_config_t:%d chain created", kymeraAncCommon_GetAancCurrentConfig());
}

static void kymeraAncCommon_Configure(const KYMERA_INTERNAL_AANC_ENABLE_T *msg)
{
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
             DEBUG_LOG("kymeraAncCommon_Configure: ANC");
             KymeraBasicPassthrough_Configure(msg);
             KymeraAah_Configure(msg);
             if(!kymeraAncCommon_isTransitionInProgress())
             {
                 kymeraAncCommon_AhmConfigure(msg, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
                 KymeraHcgr_Configure(msg);
                 KymeraNoiseID_Configure();
                 KymeraAdaptiveAnc_ConfigureChain(msg, data.filter_topology);
             }
             kymeraAncCommon_WindDetectConfigure(msg);
             KymeraEchoCanceller_Configure();
             kymeraAncCommon_ConfigureRefSplitter();
         break;

         case aanc_config_leakthrough:
             DEBUG_LOG("kymeraAncCommon_Configure: Transparency");
             KymeraBasicPassthrough_Configure(msg);
             KymeraAah_Configure(msg);
             if(!kymeraAncCommon_isTransitionInProgress())
             {
                 kymeraAncCommon_AhmConfigure(msg, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
                 KymeraHcgr_Configure(msg);
                 KymeraAdaptiveAmbient_ConfigureChain(msg);
             }
             kymeraAncCommon_WindDetectConfigure(msg);
             KymeraEchoCanceller_Configure();
         break;
#else
        case aanc_config_adaptive_anc:
            KymeraBasicPassthrough_Configure(msg);
            KymeraAah_Configure(msg);
            if(!kymeraAncCommon_isTransitionInProgress())
            {
                kymeraAncCommon_AhmConfigure(msg, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
                KymeraHcgr_Configure(msg);
                KymeraNoiseID_Configure();
                KymeraAdaptiveAnc_ConfigureChain(msg, data.filter_topology);
            }
            kymeraAncCommon_WindDetectConfigure(msg);
            KymeraEchoCanceller_Configure();
            kymeraAncCommon_ConfigureRefSplitter();
        break;

        case aanc_config_adaptive_ambient:
            KymeraBasicPassthrough_Configure(msg);
            KymeraAah_Configure(msg);
            if(!kymeraAncCommon_isTransitionInProgress())
            {
                kymeraAncCommon_AhmConfigure(msg, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
                KymeraHcgr_Configure(msg);
                KymeraAdaptiveAmbient_ConfigureChain(msg);
            }
            kymeraAncCommon_WindDetectConfigure(msg);
            KymeraEchoCanceller_Configure();
        break;

        case aanc_config_static_anc:
            KymeraBasicPassthrough_Configure(msg);
            KymeraAah_Configure(msg);
            if(!kymeraAncCommon_isTransitionInProgress())
            {
                kymeraAncCommon_AhmConfigure(msg, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
                KymeraHcgr_Configure(msg);
                KymeraNoiseID_Configure();
            }
            kymeraAncCommon_WindDetectConfigure(msg);
            KymeraEchoCanceller_Configure();
        break;

        case aanc_config_static_leakthrough:
            KymeraBasicPassthrough_Configure(msg);
            KymeraAah_Configure(msg);
            if(!kymeraAncCommon_isTransitionInProgress())
            {
                kymeraAncCommon_AhmConfigure(msg, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
                KymeraAhm_SetTargetGain(kymeraAncCommon_GetTargetGain());
                KymeraHcgr_Configure(msg);
            }
            kymeraAncCommon_WindDetectConfigure(msg);
            KymeraEchoCanceller_Configure();
        break;
#endif
        case aanc_config_fit_test:
            KymeraAdaptiveAnc_ConfigureChain(msg, data.filter_topology);
            KymeraBasicPassthrough_Configure(msg);
        break;

        default:
            break;
    }
    DEBUG_LOG("kymeraAncCommon_Configure, enum:aanc_config_t:%d chain configured", kymeraAncCommon_GetAancCurrentConfig());
}

void KymeraAncCommon_SetAncFilterTopology(anc_filter_topology_t filter_topology)
{
    DEBUG_LOG("KymeraAncCommon_SetAncFilterTopology: enum:anc_filter_topology_t:%d", filter_topology);
    data.filter_topology = filter_topology;
}

static void kymeraAncCommon_EchoCancellerConnect(fbc_config_t fbc_connect)
{
    if(KymeraEchoCanceller_IsActive())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        switch(kymeraAncCommon_GetAancCurrentConfig())
        {
            case aanc_config_anc:
                if(KymeraAah_IsFeatureSupported())
                {
                    if(fbc_connect != fb_path_only)
                    {
                        PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),KymeraEchoCanceller_GetFFMicPathSink()));
                    }
                    PanicNull(StreamConnect(KymeraAah_GetRefPathSource(),kymeraAncCommon_GetRefSplitterSink()));
                    PanicNull(StreamConnect(KymeraAah_GetFBMicPathSource(),KymeraEchoCanceller_GetFBMicPathSink()));
                }
            break;

            case aanc_config_leakthrough:
                if(KymeraAah_IsFeatureSupported())
                {
                    if(fbc_connect != fb_path_only)
                    {
                        PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),KymeraEchoCanceller_GetFFMicPathSink()));
                    }
                    PanicNull(StreamConnect(KymeraAah_GetRefPathSource(),KymeraEchoCanceller_GetSpkRefPathSink()));
                    PanicNull(StreamConnect(KymeraAah_GetFBMicPathSource(),KymeraEchoCanceller_GetFBMicPathSink()));
                }
            break;

            default:
            break;
        }
#else
        UNUSED(fbc_connect);
#ifdef ENABLE_ANC_AAH
        if (kymeraAncCommon_GetAancCurrentConfig() == aanc_config_adaptive_anc)
        {
            PanicNull(StreamConnect(KymeraAah_GetRefPathSource(), kymeraAncCommon_GetRefSplitterSink()));
        }
        else
        {
            PanicNull(StreamConnect(KymeraAah_GetRefPathSource(), KymeraEchoCanceller_GetSpkRefPathSink()));
        }
#endif
#endif
        kymeraAncCommon_ConnectRefSplitter();
        KymeraEchoCanceller_Connect();
    }
}

static void kymeraAncCommon_HcgrConnect(void)
{
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
        case aanc_config_leakthrough:
            DEBUG_LOG("kymeraAncCommon_HcgrConnect: ANC + Transparency");
            if((KymeraEchoCanceller_IsActive()))
            {
                PanicNull(StreamConnect(KymeraEchoCanceller_GetFBMicPathSource(),
                                        KymeraHcgr_GetFbMicPathSink()));
            }
            else if(KymeraAah_IsFeatureSupported())
            {
                PanicNull(StreamConnect(KymeraAah_GetFBMicPathSource(),
                                        KymeraHcgr_GetFbMicPathSink()));
            }
        break;
#else
        case aanc_config_adaptive_anc:
        PanicNull(StreamConnect(ChainGetOutput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_ERR_MIC_OUT),
                        KymeraHcgr_GetFbMicPathSink()));
        break;

        case aanc_config_adaptive_ambient:
        case aanc_config_static_leakthrough:
        case aanc_config_static_anc:
        if((KymeraEchoCanceller_IsActive()))
        {
#if defined(ENABLE_ANC_AAH)
            /* Connect AAH FB output to HCGR input through FBC */
            PanicNull(StreamConnect(KymeraAah_GetFBMicPathSource(),
                                    KymeraEchoCanceller_GetFBMicPathSink()));
#endif
            PanicNull(StreamConnect(KymeraEchoCanceller_GetFBMicPathSource(),
                        KymeraHcgr_GetFbMicPathSink()));
        }
        else
        {
#if defined(ENABLE_ANC_AAH)
            /* Connect AAH FB output to HCGR input directly*/
            PanicNull(StreamConnect(KymeraAah_GetFBMicPathSource(),
                                    KymeraHcgr_GetFbMicPathSink()));
#endif
        }
        break;
#endif
        default:
            break;
    }
    KymeraHcgr_Connect();
}

static void kymeraAncCommon_WindDetectConnect(fbc_config_t fbc_config)
{
#ifndef ENABLE_WIND_DETECT
    UNUSED(fbc_config);
#else

#ifdef ENABLE_UNIFIED_ANC_GRAPH
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
        case aanc_config_anc:
        case aanc_config_leakthrough:
            DEBUG_LOG("kymeraAncCommon_WindDetectConnect: ANC + Transparency");
            if(KymeraEchoCanceller_IsActive() && (fbc_config != fb_path_only))
            {
                PanicNull(StreamConnect(KymeraEchoCanceller_GetFFMicPathSource(),
                                        KymeraWindDetect_GetFFMicPathSink()));
            }
            else if(KymeraAah_IsFeatureSupported())
            {
                PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                                        KymeraWindDetect_GetFFMicPathSink()));
            }
            KymeraWindDetect_Connect();
        break;

        default:
        break;
    }
#else
    UNUSED(fbc_config);
    DEBUG_LOG("kymeraAncCommon_WindDetectConnect");

    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
        case aanc_config_adaptive_anc:
            PanicNull(StreamConnect(ChainGetOutput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_OUT),
                        ChainGetInput(KymeraWindDetect_GetChain(), EPR_WIND_DETECT_FF_MIC_IN)));
            KymeraWindDetect_Connect();
        break;

        case aanc_config_adaptive_ambient:
            PanicNull(StreamConnect(ChainGetOutput(KymeraAdaptiveAmbient_GetChain(),EPR_ADV_ANC_COMPANDER_OUT),
                                ChainGetInput(KymeraWindDetect_GetChain(), EPR_WIND_DETECT_FF_MIC_IN)));
            KymeraWindDetect_Connect();
        break;

        case aanc_config_static_anc:
#if defined(ENABLE_ANC_AAH)
            /* AAH FF -> WND */
            PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                                    ChainGetInput(KymeraWindDetect_GetChain(), EPR_WIND_DETECT_FF_MIC_IN)));
#endif
            KymeraWindDetect_Connect();
        break;

        case aanc_config_static_leakthrough:
#if defined(ENABLE_ANC_AAH)
        /* AAH FF -> WND */
        PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                                ChainGetInput(KymeraWindDetect_GetChain(), EPR_WIND_DETECT_FF_MIC_IN)));
#endif
            KymeraWindDetect_Connect();
        break;

        default:
            break;
    }
#endif
#endif
}

#ifdef ENABLE_UNIFIED_ANC_GRAPH
static void kymeraAncCommon_AahConnect(void)
{
    DEBUG_LOG("kymeraAncCommon_AahConnect");
    KymeraAah_Connect();
}
#endif

static void kymeraAncCommon_BasicPassthroughConnect(void)
{
    DEBUG_LOG("kymeraAncCommon_BasicPassthroughConnect");

    if (KymeraAncCommon_AdaptiveAncIsEnabled())
    {
        Sink ff_path = (kymeraAncCommon_GetFeedforwardPathSink(ancConfigIncludeFFPathFbc()) != NULL) ? (kymeraAncCommon_GetFeedforwardPathSink(ancConfigIncludeFFPathFbc())) : (kymeraAncCommon_CreateClientConsumer(&data.op_consumer_ff));
        Sink fb_path = (kymeraAncCommon_GetFeedbackPathSink() != NULL) ? (kymeraAncCommon_GetFeedbackPathSink()) : (kymeraAncCommon_CreateClientConsumer(&data.op_consumer_fb));

        PanicNull(StreamConnect(KymeraBasicPassthrough_GetFFMicPathSource(), ff_path));
        PanicNull(StreamConnect(KymeraBasicPassthrough_GetFBMicPathSource(), fb_path));

        if (KymeraEchoCanceller_IsActive())
        {
            PanicNull(StreamConnect(KymeraBasicPassthrough_GetRefPathSource(), kymeraAncCommon_GetPlaybackPathSink()));
        }

        DEBUG_LOG("kymeraAncCommon_BasicPassthroughConnect for ANC mics");
#if defined(ENABLE_WIND_DETECT)
        if (kymeraAncCommon_IsWindDetectInStage2())
        {
            DEBUG_LOG("kymeraAncCommon_BasicPassthroughConnect WNR");
            PanicNull(StreamConnect(KymeraBasicPassthrough_GetVoiceMicPathSource(), KymeraWindDetect_GetDiversityMicPathSink()));
        }
#endif
    }

#if defined(ENABLE_AUTO_AMBIENT)
    if (KymeraSelfSpeechDetect_IsActive())
    {
        DEBUG_LOG("kymeraAncCommon_BasicPassthroughConnect Auto ambient");
        PanicNull(StreamConnect(KymeraBasicPassthrough_GetBCMPathSource(), KymeraSelfSpeechDetect_GetMicPathSink()));
    }
#endif
}

static void kymeraAncCommon_AancConnect(fbc_config_t fbc_config)
{
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    DEBUG_LOG("kymeraAncCommon_AancConnect");
    // FF Path
    if(AncNoiseId_IsFeatureSupported())
    {
        PanicNull(StreamConnect(KymeraNoiseID_GetFFMicPathSource(),
                                ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_IN)));
    }
    else if(KymeraWindDetect_IsFeatureSupported())
    {
        PanicNull(StreamConnect(KymeraWindDetect_GetFFMicPathSource(),
                                ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_IN)));
    }
    else if(KymeraEchoCanceller_IsActive() && (fbc_config != fb_path_only))
    {
        PanicNull(StreamConnect(KymeraEchoCanceller_GetFFMicPathSource(),
                                ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_IN)));
    }
    else if(KymeraAah_IsFeatureSupported())
    {
         PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                                ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_IN)));
    }

    // FB Path
    PanicNull(StreamConnect(KymeraHcgr_GetFbMicPathSource(),
                                ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_ERR_MIC_IN)));
#else
    if(KymeraEchoCanceller_IsActive())
    {
        switch(fbc_config)
        {
        case fb_path_only:
#if defined(ENABLE_ANC_AAH)
            /* AAH (FF) -> AANCv2 */
            PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                                    ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_IN)));
            /* AAH (FB) -> FBC -> AANCv2 */
            PanicNull(StreamConnect(KymeraAah_GetFBMicPathSource(),
                                    KymeraEchoCanceller_GetFBMicPathSink()));
#endif
            PanicNull(StreamConnect(KymeraEchoCanceller_GetFBMicPathSource(),
                                    ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_ERR_MIC_IN)));
            break;
        case ff_fb_paths:
#if defined(ENABLE_ANC_AAH)
            /* AAH (FF) -> FBC -> AANCv2 */
            PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                                    KymeraEchoCanceller_GetFFMicPathSink()));
#endif
            PanicNull(StreamConnect(KymeraEchoCanceller_GetFFMicPathSource(),
                                    ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_IN)));

#if defined(ENABLE_ANC_AAH)
            /* AAH (FB) -> FBC -> AANCv2 */
            PanicNull(StreamConnect(KymeraAah_GetFBMicPathSource(),
                                    KymeraEchoCanceller_GetFBMicPathSink()));
#endif
            PanicNull(StreamConnect(KymeraEchoCanceller_GetFBMicPathSource(),
                                    ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_ERR_MIC_IN)));
            break;
        default:
            break;
        }
    }
#if defined(ENABLE_ANC_AAH)
    else
    {
        /* AAH (FF) -> AANCv2 */
        PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                                ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_IN)));
        /* AAH (FB) -> AANCv2 */
        PanicNull(StreamConnect(KymeraAah_GetFBMicPathSource(),
                                ChainGetInput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_ERR_MIC_IN)));
    }
    KymeraAah_Connect();
#endif
#endif
    KymeraAdaptiveAnc_ConnectChain();
}

static void kymeraAncCommon_AdaptiveAmbientConnect(fbc_config_t fbc_config)
{
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    if(KymeraWindDetect_IsFeatureSupported())
    {
        PanicNull(StreamConnect(KymeraWindDetect_GetFFMicPathSource(),
                                KymeraAdaptiveAmbient_GetFFMicPathSink()));
    }
    else if(KymeraEchoCanceller_IsActive() && (fbc_config != fb_path_only))
    {
        PanicNull(StreamConnect(KymeraEchoCanceller_GetFFMicPathSource(),
                                KymeraAdaptiveAmbient_GetFFMicPathSink()));
    }
    else if(KymeraAah_IsFeatureSupported())
    {
        PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                                KymeraAdaptiveAmbient_GetFFMicPathSink()));
    }
#else
    UNUSED(fbc_config);

#if defined(ENABLE_ANC_AAH)
    PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                            KymeraAdaptiveAmbient_GetFFMicPathSink()));
#endif
#endif
    KymeraAdaptiveAmbient_ConnectChain();
}

static void kymeraAncCommon_NoiseIDConnect(fbc_config_t fbc_config)
{
    DEBUG_LOG("kymeraAncCommon_NoiseIDConnect");
    if(KymeraNoiseID_IsActive())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        if(KymeraWindDetect_IsFeatureSupported())
        {
            PanicNull(StreamConnect(KymeraWindDetect_GetFFMicPathSource(),
                                    KymeraNoiseID_GetFFMicPathSink()));
        }
        else if(KymeraEchoCanceller_IsActive() && (fbc_config != fb_path_only))
        {
            PanicNull(StreamConnect(KymeraEchoCanceller_GetFFMicPathSource(),
                                    KymeraNoiseID_GetFFMicPathSink()));
        }
        else if(KymeraAah_IsFeatureSupported())
        {
            PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                                    KymeraNoiseID_GetFFMicPathSink()));
        }
        else
        {
            PanicNull(StreamConnect(KymeraBasicPassthrough_GetFFMicPathSource(),
                                    KymeraNoiseID_GetFFMicPathSink()));
        }
        KymeraNoiseID_Connect();
#else
        UNUSED(fbc_config);

        switch(kymeraAncCommon_GetAancCurrentConfig())
        {
            case aanc_config_adaptive_anc:
#ifdef ENABLE_WIND_DETECT
                PanicNull(StreamConnect(ChainGetOutput(KymeraWindDetect_GetChain(), EPR_WIND_DETECT_FF_MIC_OUT),
                            ChainGetInput(KymeraNoiseID_GetChain(), EPR_NOISE_ID_IN)));
#else
                PanicNull(StreamConnect(ChainGetOutput(KymeraAdaptiveAnc_GetChain(), EPR_AANCV2_FF_MIC_OUT),
                        ChainGetInput(KymeraNoiseID_GetChain(), EPR_NOISE_ID_IN)));
#endif
                KymeraNoiseID_Connect();
            break;
            case aanc_config_static_anc:
#ifdef ENABLE_WIND_DETECT
                PanicNull(StreamConnect(ChainGetOutput(KymeraWindDetect_GetChain(), EPR_WIND_DETECT_FF_MIC_OUT),
                            ChainGetInput(KymeraNoiseID_GetChain(), EPR_NOISE_ID_IN)));
#elif ENABLE_ANC_AAH
            /* AAH FF -> Noise ID */
            PanicNull(StreamConnect(KymeraAah_GetFFMicPathSource(),
                                    ChainGetInput(KymeraNoiseID_GetChain(), EPR_NOISE_ID_IN)));
#endif
            default:
                break;
        }
#endif
    }
}

static void kymeraAncCommon_Connect(void)
{
    DEBUG_LOG("kymeraAncCommon_Connect");

    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
            /*
             * Mic Framework (FF Mic)-> AAH -> WIND -> Noise ID -> AANCV2
             * Mic Framework (FB Mic)-> AAH -> HCGR -> FBC      -> AANCV2
             */
            DEBUG_LOG("kymeraAncCommon_Connect: ANC");
            kymeraAncCommon_BasicPassthroughConnect();
            kymeraAncCommon_AhmConnect();
            kymeraAncCommon_AahConnect();
            kymeraAncCommon_EchoCancellerConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_WindDetectConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_NoiseIDConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_HcgrConnect();
            kymeraAncCommon_AancConnect(ancConfigIncludeFFPathFbc());
            KymeraSelfSpeechDetect_Connect();
        break;

        case aanc_config_leakthrough:
            /*
             * Mic Framework (FF Mic)-> AAH -> WIND -> ANC compander
             * Mic Framework (FB Mic)-> AAH -> HCGR
             */
            DEBUG_LOG("kymeraAncCommon_Connect: Transparency");
            kymeraAncCommon_BasicPassthroughConnect();
            kymeraAncCommon_AhmConnect();
            kymeraAncCommon_AahConnect();
            kymeraAncCommon_EchoCancellerConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_WindDetectConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_HcgrConnect();
            kymeraAncCommon_AdaptiveAmbientConnect(ancConfigIncludeFFPathFbc());
            KymeraSelfSpeechDetect_Connect();
        break;

        case aanc_config_fit_test:
            kymeraAncCommon_BasicPassthroughConnect();
        break;
#else
        case aanc_config_adaptive_anc:
        /* Standalone graph:
         * Mic Framework (FF Mic)  -> AAH -> AANCV2 -> WIND -> Noise ID
         * Mic Framework (FB Mic)  -> AAH -> AANCv2 -> HCGR
         * Concurrency graph:
         * Mic Framework (FF Mic)  -> AAH -> FBC1 -> AANCv2 -> WIND
         * Mic Framework (FB Mic)  -> AAH -> FBC2 -> AANCv2 -> HCGR
         */
          kymeraAncCommon_BasicPassthroughConnect();
          kymeraAncCommon_HcgrConnect();
          kymeraAncCommon_WindDetectConnect(ancConfigIncludeFFPathFbc());
          kymeraAncCommon_NoiseIDConnect(ancConfigIncludeFFPathFbc());
          kymeraAncCommon_AancConnect(ancConfigIncludeFFPathFbc());
          kymeraAncCommon_EchoCancellerConnect(ancConfigIncludeFFPathFbc());
          kymeraAncCommon_AhmConnect();
          KymeraSelfSpeechDetect_Connect();
        break;

        case aanc_config_adaptive_ambient:
            kymeraAncCommon_BasicPassthroughConnect();
            kymeraAncCommon_HcgrConnect();
            kymeraAncCommon_AhmConnect();
            kymeraAncCommon_WindDetectConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_AdaptiveAmbientConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_EchoCancellerConnect(ancConfigIncludeFFPathFbc());
            KymeraSelfSpeechDetect_Connect();
        break;

        case aanc_config_static_anc:
            kymeraAncCommon_BasicPassthroughConnect();
            kymeraAncCommon_HcgrConnect();
            kymeraAncCommon_WindDetectConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_NoiseIDConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_EchoCancellerConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_AhmConnect();
            KymeraSelfSpeechDetect_Connect();
        break;

        case aanc_config_static_leakthrough:
            kymeraAncCommon_BasicPassthroughConnect();
            kymeraAncCommon_HcgrConnect();
            kymeraAncCommon_AhmConnect();
            kymeraAncCommon_WindDetectConnect(ancConfigIncludeFFPathFbc());
            kymeraAncCommon_EchoCancellerConnect(ancConfigIncludeFFPathFbc());
            KymeraSelfSpeechDetect_Connect();
        break;

        case aanc_config_fit_test:
            kymeraAncCommon_BasicPassthroughConnect();
        break;
#endif
        default:
            kymeraAncCommon_BasicPassthroughConnect();
            KymeraSelfSpeechDetect_Connect();
        break;
    }
}

static void kymeraAncCommon_BasicPassthroughDisconnect(void)
{
    DEBUG_LOG("kymeraAncCommon_BasicPassthroughDisconnect");

    if (KymeraAncCommon_AdaptiveAncIsEnabled())
    {
        StreamDisconnect(KymeraBasicPassthrough_GetFFMicPathSource(), NULL);
        StreamDisconnect(KymeraBasicPassthrough_GetFBMicPathSource(), NULL);

        if (KymeraEchoCanceller_IsActive())
        {
            StreamDisconnect(KymeraBasicPassthrough_GetRefPathSource(), NULL);
        }

        DEBUG_LOG("kymeraAncCommon_BasicPassthroughConnect for ANC mics");
#if defined(ENABLE_WIND_DETECT)
        if (kymeraAncCommon_IsWindDetectInStage2())
        {
            DEBUG_LOG("kymeraAncCommon_BasicPassthroughConnect WNR");
            StreamDisconnect(KymeraBasicPassthrough_GetVoiceMicPathSource(), NULL);
        }
#endif
    }

#if defined(ENABLE_AUTO_AMBIENT)
    if (KymeraSelfSpeechDetect_IsActive())
    {
        DEBUG_LOG("kymeraAncCommon_BasicPassthroughConnect Auto ambient");
        StreamDisconnect(KymeraBasicPassthrough_GetBCMPathSource(), NULL);
    }
#endif
}

static bool kymeraAncCommon_IsLeVoiceConcurrencyActive(void)
{
    bool status = FALSE;
    output_users_t connected_users = Kymera_OutputGetConnectedUsers();

    if ( ((connected_users & output_user_le_srec)==output_user_le_srec) ||
        ((connected_users & output_user_le_voice_mono)==output_user_le_voice_mono) ||
        ((connected_users & output_user_le_voice_stereo)==output_user_le_voice_stereo) ||
        (data.connecting_user==output_user_le_srec) ||
        (data.connecting_user==output_user_le_voice_mono) ||
        (data.connecting_user==output_user_le_voice_stereo) )
    {
        status = TRUE;
    }
    return status;
}

static aah_sysmode_t kymeraAncCommon_GetAahConcurrencySysmode(void)
{
    if(kymeraAncCommon_IsLeVoiceConcurrencyActive())
    {
        return (aah_sysmode_standby);
    }
    return (aah_sysmode_full);
}

static void kymeraAncCommon_AahStart(void)
{
    aah_sysmode_t sysmode = aah_sysmode_full;

    if(Kymera_OutputIsChainInUse())
    {
        sysmode = kymeraAncCommon_GetAahConcurrencySysmode();
        KymeraAah_SetLimitsForConcurrency();
    }

    KymeraAah_SetSysMode(sysmode);
    KymeraAah_Start();
}


#ifndef ENABLE_ANC_FAST_MODE_SWITCH
static void kymeraAncCommon_AhmRampGainsToStatic(void)
{
#ifdef ENABLE_WIND_DETECT
    if (kymeraAncCommon_IsWindDetectInStage2())
    {
        kymeraAncCommon_RampGainsToStatic(NULL);
    }
    else
#endif
    {
        kymeraAncCommon_RampGainsToStatic(kymeraAncCommon_StartOrFreezeAdaptivity);
    }
}
#endif

/*
If 'fast mode switch' flag included
    If ANC enable,
        Set AANC2/ADRC sysmode to full proc
    Else if Mode change
        If AANC
            Freeze
        Else if ADRC
            Full proc
    Else
        Set AANC2/ADRC sysmode to full proc
Else
    If ANC enable,
        If AANC
            Set AANC2 sysmode to freeze
            Set AHM to 'full proc' and Register 'Start Adaptivity' callback
        Else if AAMB
            Set AHM to 'full proc' and Register 'Start Adaptivity' callback
        Else
            Set AHM to 'full proc'
    Else if ANC mode change,
        If AANC
            Set AANC2 sysmode to freeze
            Set AHM to 'full proc' and Register 'Start Adaptivity' callback
        Else if AAMB
            Set AHM to 'full proc' and Register 'Start Adaptivity' callback
        Else
            Set AHM to 'full proc'
    Else
        Set AANC2/ADRC sysmode to full proc

*/

static void kymeraAncCommon_SetOperatorsSysmode(void)
{
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
    if (kymeraAncCommon_IsModeChangeInProgress())
    {
        aanc_config_t aanc_config = kymeraAncCommon_GetAancCurrentConfig();
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        if (aanc_config == aanc_config_anc)
#else
        if (aanc_config == aanc_config_adaptive_anc)
#endif
        {
            KymeraAdaptiveAnc_SetSysMode(adaptive_anc_sysmode_freeze);
        }
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        else if (aanc_config == aanc_config_leakthrough)
#else
        else if (aanc_config == aanc_config_adaptive_ambient)
#endif
        {
            if(kymeraAncCommon_IsAdaptiveMode())
            {
                kymeraAncCommon_SetAncCompanderSysMode(anc_compander_sysmode_full);
            }
            else
            {
                kymeraAncCommon_SetAncCompanderSysMode(anc_compander_sysmode_passthrough);
            }
        }
    }
    else
    {
#ifdef ENABLE_WIND_DETECT
        if (!kymeraAncCommon_IsWindDetectInStage2())
#endif
        {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
            kymeraAncCommon_StartOrFreezeAdaptivity();
#else
            kymeraAncCommon_StartAdaptivity();
#endif
        }
    }
#else
    if (kymeraAncCommon_GetAancState() == aanc_state_enable_initiated || kymeraAncCommon_IsModeChangeInProgress())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        if (kymeraAncCommon_GetAancCurrentConfig() == aanc_config_anc)
#else
        if (kymeraAncCommon_GetAancCurrentConfig() == aanc_config_adaptive_anc)
#endif
        {
            KymeraAdaptiveAnc_SetSysMode(adaptive_anc_sysmode_freeze);
        }
        kymeraAncCommon_AhmRampGainsToStatic();
    }
    else
    {
#ifdef ENABLE_WIND_DETECT
        if (!kymeraAncCommon_IsWindDetectInStage2())
#endif
        {
            kymeraAncCommon_StartAdaptivity();
        }
    }
#endif
}

static void kymeraAncCommon_EchoCancellerStart(void)
{
    KymeraEchoCanceller_UpdateBypassFbc(!Kymera_OutputIsChainInUse());
    KymeraEchoCanceller_Start();
}

static void kymeraAncCommon_Start(void)
{
    kymeraAncCommon_StartClientConsumer(data.op_consumer_ff, data.op_consumer_fb);

    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
       case aanc_config_anc:
            DEBUG_LOG("kymeraAncCommon_Start: ANC");
            kymeraAncCommon_SetOperatorsSysmode();
            kymeraAncCommon_AhmStart();
            KymeraAdaptiveAnc_StartChain();
            KymeraHcgr_Start();
            kymeraAncCommon_EchoCancellerStart();
            kymeraAncCommon_StartRefSplitter();
            KymeraNoiseID_Start();
            KymeraAncCommon_NoiseIDEnableOrDisable();
            KymeraWindDetect_Start();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
        break;

        case aanc_config_leakthrough:
            DEBUG_LOG("kymeraAncCommon_Start: Transparency");
            kymeraAncCommon_SetOperatorsSysmode();
            kymeraAncCommon_AhmStart();
            KymeraAdaptiveAmbient_StartChain();
            KymeraHcgr_Start();
            kymeraAncCommon_EchoCancellerStart();
            KymeraWindDetect_Start();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
        break;
#else
        case aanc_config_adaptive_anc:
            kymeraAncCommon_SetOperatorsSysmode();
            kymeraAncCommon_AhmStart();
            KymeraHcgr_Start();
            KymeraWindDetect_Start();
            KymeraNoiseID_Start();
            KymeraAncCommon_NoiseIDEnableOrDisable();
            KymeraAdaptiveAnc_StartChain();
            kymeraAncCommon_EchoCancellerStart();
            kymeraAncCommon_StartRefSplitter();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
        break;

        case aanc_config_adaptive_ambient:
            kymeraAncCommon_SetOperatorsSysmode();
            kymeraAncCommon_AhmStart();
            KymeraHcgr_Start();
            KymeraWindDetect_Start();
            KymeraAdaptiveAmbient_StartChain();
            kymeraAncCommon_EchoCancellerStart();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
        break;

        case aanc_config_static_anc:
            kymeraAncCommon_SetOperatorsSysmode();
            kymeraAncCommon_AhmStart();
            KymeraHcgr_Start();
            KymeraWindDetect_Start();
            KymeraNoiseID_Start();
            KymeraAncCommon_NoiseIDEnableOrDisable();
            kymeraAncCommon_EchoCancellerStart();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
        break;

        case aanc_config_static_leakthrough:
            kymeraAncCommon_SetOperatorsSysmode();
            kymeraAncCommon_AhmStart();
            KymeraHcgr_Start();
            KymeraWindDetect_Start();
            kymeraAncCommon_AahStart();
            kymeraAncCommon_EchoCancellerStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
        break;
#endif
        case aanc_config_fit_test:
            KymeraAdaptiveAnc_SetSysMode(adaptive_anc_sysmode_freeze);
            KymeraAdaptiveAnc_StartChain();
            KymeraBasicPassthrough_Start();
        break;

        default:
            /*For concurrency use cases when VAD only is enabled*/
            KymeraSelfSpeechDetect_Start();
            break;
    }
    DEBUG_LOG("kymeraAncCommon_Start, enum:aanc_config_t:%d chain active now", kymeraAncCommon_GetAancCurrentConfig());
}

static void kymeraAncCommon_Stop(void)
{
    kymeraAncCommon_StopClientConsumer(data.op_consumer_ff, data.op_consumer_fb);

    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
       case aanc_config_anc:
            DEBUG_LOG("kymeraAncCommon_Stop: ANC");
            KymeraBasicPassthrough_Stop();
            KymeraAah_Stop();
            KymeraWindDetect_Stop();
            KymeraNoiseID_Stop();
            kymeraAncCommon_StopRefSplitter();
            KymeraEchoCanceller_Stop();
            KymeraHcgr_Stop();
            KymeraAdaptiveAnc_StopChain();
            kymeraAncCommon_AhmStop();
            KymeraSelfSpeechDetect_Stop();
        break;

        case aanc_config_leakthrough:
            DEBUG_LOG("kymeraAncCommon_Stop: Transparency");
            KymeraBasicPassthrough_Stop();
            KymeraAah_Stop();
            KymeraWindDetect_Stop();
            KymeraEchoCanceller_Stop();
            KymeraHcgr_Stop();
            KymeraAdaptiveAmbient_StopChain();
            kymeraAncCommon_AhmStop();
            KymeraSelfSpeechDetect_Stop();
        break;
#else
        case aanc_config_adaptive_anc:
            KymeraBasicPassthrough_Stop();
            KymeraAah_Stop();
            kymeraAncCommon_StopRefSplitter();
            KymeraEchoCanceller_Stop();
            KymeraAdaptiveAnc_StopChain();
            kymeraAncCommon_AhmStop();
            KymeraNoiseID_Stop();
            KymeraWindDetect_Stop();
            KymeraHcgr_Stop();
            KymeraSelfSpeechDetect_Stop();
        break;

        case aanc_config_adaptive_ambient:
            KymeraBasicPassthrough_Stop();
            KymeraAah_Stop();
            KymeraEchoCanceller_Stop();
            KymeraAdaptiveAmbient_StopChain();
            kymeraAncCommon_AhmStop();
            KymeraWindDetect_Stop();
            KymeraHcgr_Stop();
            KymeraSelfSpeechDetect_Stop();
        break;

        case aanc_config_static_anc:
            KymeraBasicPassthrough_Stop();
            KymeraAah_Stop();
            KymeraEchoCanceller_Stop();
            kymeraAncCommon_AhmStop();
            KymeraNoiseID_Stop();
            KymeraWindDetect_Stop();
            KymeraHcgr_Stop();
            KymeraSelfSpeechDetect_Stop();

        break;

        case aanc_config_static_leakthrough:
            KymeraBasicPassthrough_Stop();
            KymeraAah_Stop();
            KymeraEchoCanceller_Stop();
            kymeraAncCommon_AhmStop();
            KymeraWindDetect_Stop();
            KymeraHcgr_Stop();
            KymeraSelfSpeechDetect_Stop();
        break;
#endif
        case aanc_config_fit_test:
            KymeraBasicPassthrough_Stop();
            KymeraAdaptiveAnc_StopChain();
        break;

        default:
            /*For concurrency use cases when VAD only is enabled*/
            KymeraSelfSpeechDetect_Stop();
            break;
    }

    DEBUG_LOG("kymeraAncCommon_Stop, enum:aanc_config_t:%d chain stopped", kymeraAncCommon_GetAancCurrentConfig());
}

static void kymeraAncCommon_Destroy(void)
{
    DEBUG_LOG("kymeraAncCommon_Destroy");

    kymeraAncCommon_DestroyClientConsumer(&data.op_consumer_ff, &data.op_consumer_fb);

    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
            DEBUG_LOG("kymeraAncCommon_Destroy: ANC");
            KymeraBasicPassthrough_Destroy();
            KymeraAah_Destroy();
            kymeraAncCommon_DestroyRefSplitter();
            KymeraEchoCanceller_Destroy();
            kymeraAncCommon_AANCDestroy();
            kymeraAncCommon_AhmDestroy();
            kymeraAncCommon_NoiseIDDestroy();
            kymeraAncCommon_WindDetectDestroy();
            kymeraAncCommon_HcgrDestroy();
        break;

        case aanc_config_leakthrough:
            DEBUG_LOG("kymeraAncCommon_Destroy: Transparency");
            KymeraBasicPassthrough_Destroy();
            KymeraAah_Destroy();
            KymeraEchoCanceller_Destroy();
            kymeraAncCommon_AdaptiveAmbientDestroy();
            kymeraAncCommon_AhmDestroy();
            kymeraAncCommon_WindDetectDestroy();
            kymeraAncCommon_HcgrDestroy();
        break;

#else
        case aanc_config_adaptive_anc:
            KymeraBasicPassthrough_Destroy();
            KymeraAah_Destroy();
            kymeraAncCommon_DestroyRefSplitter();
            KymeraEchoCanceller_Destroy();
            kymeraAncCommon_AANCDestroy();
            kymeraAncCommon_AhmDestroy();
            kymeraAncCommon_NoiseIDDestroy();
            kymeraAncCommon_WindDetectDestroy();
            kymeraAncCommon_HcgrDestroy();
        break;

        case aanc_config_adaptive_ambient:
            KymeraBasicPassthrough_Destroy();
            KymeraAah_Destroy();
            KymeraEchoCanceller_Destroy();
            kymeraAncCommon_AdaptiveAmbientDestroy();
            kymeraAncCommon_AhmDestroy();
            kymeraAncCommon_WindDetectDestroy();
            kymeraAncCommon_HcgrDestroy();
        break;

        case aanc_config_static_anc:
            KymeraBasicPassthrough_Destroy();
            KymeraAah_Destroy();
            KymeraEchoCanceller_Destroy();
            kymeraAncCommon_AhmDestroy();
            kymeraAncCommon_NoiseIDDestroy();
            kymeraAncCommon_WindDetectDestroy();
            kymeraAncCommon_HcgrDestroy();
        break;

        case aanc_config_static_leakthrough:
            KymeraBasicPassthrough_Destroy();
            KymeraAah_Destroy();
            KymeraEchoCanceller_Destroy();
            kymeraAncCommon_AhmDestroy();
            kymeraAncCommon_WindDetectDestroy();
            kymeraAncCommon_HcgrDestroy();
        break;
#endif
        case aanc_config_fit_test:
            KymeraBasicPassthrough_Destroy();
            KymeraAdaptiveAnc_DestroyChain();
        break;

        default:
            break;
    }

    DEBUG_LOG("kymeraAncCommon_Destroy, enum:aanc_config_t:%d chain destroyed", kymeraAncCommon_GetAancCurrentConfig());
}

static void kymeraAncCommon_Disconnect(void)
{
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
            DEBUG_LOG("kymeraAncCommon_Disconnect: ANC");
            kymeraAncCommon_BasicPassthroughDisconnect();
            KymeraAah_Disconnect();
            kymeraAncCommon_DisconnectRefSplitter();
            KymeraEchoCanceller_Disconnect();
            KymeraAdaptiveAnc_DisconnectChain();
            KymeraNoiseID_Disconnect();
            KymeraHcgr_Disconnect();
            KymeraWindDetect_Disconnect();
            KymeraSelfSpeechDetect_Disconnect();
        break;

        case aanc_config_leakthrough:
            DEBUG_LOG("kymeraAncCommon_Disconnect: Transparency");
            kymeraAncCommon_BasicPassthroughDisconnect();
            KymeraAah_Disconnect();
            KymeraEchoCanceller_Disconnect();
            KymeraAdaptiveAmbient_DisconnectChain();
            KymeraHcgr_Disconnect();
            KymeraWindDetect_Disconnect();
            KymeraSelfSpeechDetect_Disconnect();
        break;

        case aanc_config_fit_test:
            kymeraAncCommon_BasicPassthroughDisconnect();
        break;
#else
        case aanc_config_adaptive_anc:
            kymeraAncCommon_BasicPassthroughDisconnect();
            KymeraAah_Disconnect();
            kymeraAncCommon_DisconnectRefSplitter();
            KymeraEchoCanceller_Disconnect();
            /*Stream Disconnect AANC with AHM*/
            KymeraAdaptiveAnc_DisconnectChain();
            KymeraNoiseID_Disconnect();
            KymeraHcgr_Disconnect();
            KymeraSelfSpeechDetect_Disconnect();
        break;

        case aanc_config_adaptive_ambient:
            kymeraAncCommon_BasicPassthroughDisconnect();
            KymeraAah_Disconnect();
            KymeraEchoCanceller_Disconnect();
            StreamDisconnect(ChainGetOutput(KymeraAdaptiveAmbient_GetChain(),EPR_ADV_ANC_COMPANDER_OUT), NULL);
            KymeraAdaptiveAmbient_DisconnectChain();
            KymeraHcgr_Disconnect();
            KymeraSelfSpeechDetect_Disconnect();
        break;

        case aanc_config_static_anc:
            kymeraAncCommon_BasicPassthroughDisconnect();
            KymeraAah_Disconnect();
            KymeraEchoCanceller_Disconnect();
            KymeraNoiseID_Disconnect();
            KymeraHcgr_Disconnect();
            KymeraSelfSpeechDetect_Disconnect();
        break;

        case aanc_config_static_leakthrough:
            kymeraAncCommon_BasicPassthroughDisconnect();
            KymeraAah_Disconnect();
            KymeraEchoCanceller_Disconnect();
            KymeraHcgr_Disconnect();
            KymeraSelfSpeechDetect_Disconnect();
        break;

        case aanc_config_fit_test:
            kymeraAncCommon_BasicPassthroughDisconnect();
        /* Do nothing */
        break;
#endif
        default:
            /*Stream Disconnect between PEQ and VAD to avoid false triggers from VAD*/
            kymeraAncCommon_BasicPassthroughDisconnect();
            KymeraSelfSpeechDetect_Disconnect();
            break;
    }
}

static void kymeraAncCommon_AncStopOperators(void)
{
    if (KymeraAncCommon_AdaptiveAncIsEnabled())/*if any ANC mode is active*/
    {
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
        if (!kymeraAncCommon_IsModeChangeInProgress())
#endif
        {
            kymeraAncCommon_FreezeAdaptivity();
        }
        kymeraAncCommon_Stop();
        kymeraAncCommon_Disconnect();
    }
}

static void kymeraAncCommon_AncMicDisconnect(void)
{
    if (KymeraAncCommon_AdaptiveAncIsEnabled())/*if any ANC mode is active*/
    {
        kymeraAncCommon_AncStopOperators();
        Kymera_MicDisconnect(mic_user_aanc);
    }
}

static void kymeraAncCommon_Enable(const KYMERA_INTERNAL_AANC_ENABLE_T *msg)
{
    DEBUG_LOG("kymeraAncCommon_Enable");

    /* Store mode settings to apply again in transitional case */
    kymeraAncCommon_StoreModeSettings(msg);
    kymeraAncCommon_SetAancCurrentConfig(kymeraAncCommon_GetAancConfigFromMode(msg->current_mode));
    kymeraAncCommon_SetAdaptiveMode(AncConfig_IsAncModeAdaptive(msg->current_mode));

    if(Kymera_OutputIsChainInUse())
    {
        kymeraAncCommon_UpdateConcurrencyRequest(TRUE);
    }

    /* Use maximum audio clock speed in start of ANC graph setup */
    appKymeraBoostDspClockToMax();

#if defined(ENABLE_SELF_SPEECH) && defined (ENABLE_AUTO_AMBIENT)
    /*If Self speech is active, Disconnect Mic*/
    if (!kymeraAncCommon_IsModeChangeInProgress())
    {
        kymeraAncCommon_SelfSpeechDetect_MicDisconnect();
    }
#endif

    if (!KymeraAncCommon_AdaptiveAncIsEnabled())
    {
        kymeraAncCommon_SetEnableInProgress(TRUE);
    }

    /*Create and Enable ANC chain*/
    kymeraAncCommon_Create();
    kymeraAncCommon_Configure(msg);

    /*kymeraAncCommon_MicGetConnectionParameters() takes care of ANC and VAD connection to mic framework*/
    if (!Kymera_MicConnect(mic_user_aanc))
    {
        kymeraAncCommon_Destroy();
        kymeraAncCommon_SetAancState(aanc_state_idle);
        kymeraAncCommon_SetAancCurrentConfig(aanc_config_none);
        kymeraAncCommon_SetAdaptiveMode(FALSE);
        kymeraAncCommon_UpdateConcurrencyRequest(FALSE);
        MAKE_MESSAGE(KYMERA_INTERNAL_AANC_ENABLE);

        memcpy(message, msg, sizeof(KYMERA_INTERNAL_AANC_ENABLE_T));
        MessageSendLater(KymeraGetTask(), KYMERA_INTERNAL_MIC_CONNECTION_TIMEOUT_ANC,
                         message, MIC_CONNECT_RETRY_MS);
    }
    else
    {
        kymeraAncCommon_Connect();
        kymeraAncCommon_Start();
        kymeraAncCommon_SetAancState(aanc_state_enabled);
        kymeraAncCommon_SetKymeraState();
    }

    /* Use optimal audio clock speed after ANC graph setup */
    appKymeraConfigureDspPowerMode();

    if (kymeraAncCommon_IsEnableInProgress())
    {
        kymeraAncCommon_SetEnableInProgress(FALSE);
    }

}

static void kymeraAncCommon_EnableOnUserRequest(const KYMERA_INTERNAL_AANC_ENABLE_T *msg)
{
    kymeraAncCommon_SetAancState(aanc_state_enable_initiated);
    kymeraAncCommon_Enable(msg);
}

static void kymeraAncCommon_Disable(void)
{
    DEBUG_LOG("kymeraAncCommon_Disable, tearing down AANC graph");

    /* Disable ultra quiet mode with ongoing adaptive anc capability being on mute */
    Kymera_CancelUltraQuietDac();
    kymeraAncCommon_AncMicDisconnect();
    kymeraAncCommon_Destroy();
    kymeraAncCommon_SetAancCurrentConfig(aanc_config_none);
    kymeraAncCommon_SetAdaptiveMode(FALSE);

#if defined(ENABLE_SELF_SPEECH) && defined (ENABLE_AUTO_AMBIENT)
    /*Reconnect Self Speech if active*/
    if (!kymeraAncCommon_IsModeChangeInProgress())
    {
        kymeraAncCommon_SelfSpeechDetect_MicReConnect();
    }
#endif

    kymeraAncCommon_SetAancState(aanc_state_idle);
    kymeraAncCommon_ResetKymeraState();

#ifdef ENABLE_ANC_FAST_MODE_SWITCH
    if (!kymeraAncCommon_IsModeChangeInProgress())
#endif
    {
        /* Update default DSP clock */
        appKymeraConfigureDspPowerMode();
    }
}

static void kymeraAncCommon_DisableOnUserRequest(void)
{
    DEBUG_LOG("kymeraAncCommon_DisableOnUserRequest");

    kymeraAncCommon_SetAancState(aanc_state_disable_initiated);

    /* Cancel any pending AHM Ramp-up timeout messages */
    MessageCancelAll(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_AHM_RAMP_TIMEOUT);

    /*Tear down the chains*/
    kymeraAncCommon_Disable();
#ifdef ENABLE_WIND_DETECT
    kymeraAncCommon_ResetWindDetectInStage2();
#endif

    /*Inform ANC SM to disable HW*/
    MessageSend(AncStateManager_GetTask(), KYMERA_ANC_COMMON_CAPABILITY_DISABLE_COMPLETE, NULL);
}

static void kymeraAncCommon_StandaloneToConcurrencyChain(void)
{
#ifndef ENABLE_UNIFIED_ANC_GRAPH
    DEBUG_LOG_INFO("kymeraAncCommon_StandaloneToConcurrencyChain");
    kymeraAncCommon_UpdateTransitionInProgress(TRUE); /* Standalone to concurrency transition is in progress- ahm should not be destroyed */
    kymeraAncCommon_FreezeAdaptivity();
    kymeraAncCommon_Disable(); /* Disable Standalone Chain with no ramp */
    DEBUG_LOG_INFO("kymeraAncCommon_StandaloneToConcurrencyChain, disabled Standalone chain");

    kymeraAncCommon_UpdateConcurrencyRequest(TRUE); /* Update concurrency flag TRUE */
    KYMERA_INTERNAL_AANC_ENABLE_T mode_settings = getData()->mode_settings;
    kymeraAncCommon_Enable(&mode_settings); /* Enable Concurrency chain */
    kymeraAncCommon_UpdateTransitionInProgress(FALSE);/* Standalone to concurrency transition is over- user can destroy ahm using anc disable event*/
#endif
}

static void kymeraAncCommon_ConcurrencyToStandaloneChain(void)
{
#ifndef ENABLE_UNIFIED_ANC_GRAPH
    DEBUG_LOG_INFO("kymeraAncCommon_ConcurrencyToStandaloneChain");
    kymeraAncCommon_UpdateTransitionInProgress(TRUE);/* Concurrency to standalone transition is in progress- ahm should not be destroyed */
    kymeraAncCommon_FreezeAdaptivity();
    kymeraAncCommon_Disable(); /* Disable Concurrency chain with no ramp */
    DEBUG_LOG_INFO("kymeraAncCommon_ConcurrencyToStandaloneChain, disabled Concurrency chain");

    kymeraAncCommon_UpdateConcurrencyRequest(FALSE); /* Update concurrency flag to FALSE */
    KYMERA_INTERNAL_AANC_ENABLE_T mode_settings = getData()->mode_settings;
    kymeraAncCommon_Enable(&mode_settings); /* Enable Standalone chain */
    kymeraAncCommon_UpdateTransitionInProgress(FALSE);/* Concurrency to standalone transition is over- user can destroy ahm using anc disable event*/
#endif
}

static void kymeraAncCommon_ApplyModeChangeSettings(KYMERA_INTERNAL_AANC_ENABLE_T *mode_params)
{
    UNUSED(mode_params);
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            KymeraAhm_ApplyModeChange(mode_params, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
#endif
            KymeraWindDetect_ApplyModeChange(mode_params);
            KymeraAdaptiveAnc_ApplyModeChange(mode_params);
            KymeraAah_ApplyModeChange(mode_params);
            KymeraNoiseID_SetCategoryBasedOnCurrentMode();
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            kymeraAncCommon_RampupGains(kymeraAncCommon_StartOrFreezeAdaptivity);
#endif
        break;

        case aanc_config_leakthrough:
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            KymeraAhm_ApplyModeChange(mode_params, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
#endif
            KymeraWindDetect_ApplyModeChange(mode_params);
            KymeraAdaptiveAmbient_ApplyModeChange(mode_params);
            KymeraAah_ApplyModeChange(mode_params);
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            kymeraAncCommon_RampupGains(kymeraAncCommon_StartOrFreezeAdaptivity);
#endif
        break;
#else
        case aanc_config_adaptive_anc:
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            KymeraAhm_ApplyModeChange(mode_params, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
#endif
            KymeraWindDetect_ApplyModeChange(mode_params);
            KymeraAdaptiveAnc_ApplyModeChange(mode_params);
            KymeraAah_ApplyModeChange(mode_params);
            KymeraNoiseID_SetCategoryBasedOnCurrentMode();
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            kymeraAncCommon_RampupGains(kymeraAncCommon_StartAdaptivity);
#endif
        break;

        case aanc_config_adaptive_ambient:
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            KymeraAhm_ApplyModeChange(mode_params, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
#endif
            KymeraWindDetect_ApplyModeChange(mode_params);
            KymeraAdaptiveAmbient_ApplyModeChange(mode_params);
            KymeraAah_ApplyModeChange(mode_params);
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            kymeraAncCommon_RampupGains(kymeraAncCommon_StartAdaptivity);
#endif
        break;

        case aanc_config_static_anc:
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            KymeraAhm_ApplyModeChange(mode_params, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
#endif
            KymeraWindDetect_ApplyModeChange(mode_params);
            KymeraAah_ApplyModeChange(mode_params);
            KymeraNoiseID_SetCategoryBasedOnCurrentMode();
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            kymeraAncCommon_RampupGains(kymeraAncCommon_StartAdaptivity);
#endif
        break;

        case aanc_config_static_leakthrough:
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            KymeraAhm_ApplyModeChange(mode_params, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
#endif
            KymeraWindDetect_ApplyModeChange(mode_params);
            KymeraAah_ApplyModeChange(mode_params);
#ifndef ENABLE_ANC_FAST_MODE_SWITCH
            kymeraAncCommon_RampupGains(NULL);
#endif
#endif
        default:
        /* Do nothing */
            break;
    }
}

#ifndef ENABLE_ANC_FAST_MODE_SWITCH

static void kymeraAncCommon_ModeChangeActionPostGainRamp(void)
{
    DEBUG_LOG("kymeraAncCommon_ModeChangeActionPostGainRamp");

    /* Cancel any pending AHM Ramp-up timeout messages */
    MessageCancelAll(AncStateManager_GetTask(), KYMERA_ANC_COMMON_CAPABILITY_MODE_CHANGE_TRIGGER);

    /*Inform ANC SM to Change mode in HW*/
    MessageSend(AncStateManager_GetTask(), KYMERA_ANC_COMMON_CAPABILITY_MODE_CHANGE_TRIGGER, NULL);
}

#endif /* ENABLE_ANC_FAST_MODE_SWITCH */

static bool kymeraAncCommon_IsConcurrencyChainActive(void)
{
    return (KymeraEchoCanceller_IsActive());
}

/***************************************
 *** Mic framework callbacks ***
 ***************************************/
static uint32 kymera_AncCommonMicTaskPeriod = AEC_REF_TASK_PERIOD_1MS;

static uint16 kymera_AncCommonMics[MAX_ANC_COMMON_MICS] =
{
    appConfigAncFeedForwardMic(),
    appConfigAncFeedBackMic()
};

/*! For a reconnection the mic parameters are sent to the mic interface.
 *  return TRUE to reconnect with the given parameters
 */
static bool kymeraAncCommon_MicGetConnectionParameters(uint16 *mic_ids, Sink *mic_sinks, uint8 *num_of_mics, uint32 *sample_rate, Sink *aec_ref_sink)
{
    DEBUG_LOG("kymeraAncCommon_MicGetConnectionParameters");

    *sample_rate = KYMERA_ANC_COMMON_SAMPLE_RATE;
    *num_of_mics = 0;
    aec_ref_sink[0]=NULL;

    if (KymeraAncCommon_AdaptiveAncIsEnabled())
    {
        *num_of_mics = GET_REQUIRED_ANC_MICS;
        if (*num_of_mics > 0)
        {
            mic_ids[0] = kymeraAncCommon_GetAncFeedforwardMicConfig();
            mic_ids[1] = kymeraAncCommon_GetAncFeedbackMicConfig();

            mic_sinks[0] = KymeraBasicPassthrough_GetFFMicPathSink();
            mic_sinks[1] = KymeraBasicPassthrough_GetFBMicPathSink();
        }

        if (KymeraEchoCanceller_IsActive())
        {
            aec_ref_sink[0] = KymeraBasicPassthrough_GetRefPathSink();
        }

        DEBUG_LOG("kymeraAncCommon_MicGetConnectionParameters for ANC mics");
#if defined(ENABLE_WIND_DETECT)
        if (kymeraAncCommon_IsWindDetectInStage2())
        {
            uint16 mic_index = *num_of_mics;
            *num_of_mics = mic_index+1;/*Include Diversity mic for wind detect*/
            DEBUG_LOG("kymeraAncCommon_MicGetConnectionParameters WNR *num_of_mics %d, mic_index %d", *num_of_mics, mic_index);
            mic_ids[mic_index] = appConfigWindDetectDiversityMic();
            mic_sinks[mic_index] = KymeraBasicPassthrough_GetVoiceMicPathSink();
        }
#endif
    }

#if defined(ENABLE_AUTO_AMBIENT)
    if (KymeraSelfSpeechDetect_IsActive())
    {
        uint16 mic_index = *num_of_mics;
        *num_of_mics = mic_index+1;/*Include Self Speech mic for Ambient mode auto enable*/
        DEBUG_LOG("kymeraAncCommon_MicGetConnectionParameters Auto ambient *num_of_mics %d, mic_index %d ", *num_of_mics, mic_index);
        mic_ids[mic_index] = appConfigSelfSpeechDetectMic();
        mic_sinks[mic_index] =  KymeraBasicPassthrough_GetBCMPathSink();
    }
#endif
    return TRUE;
}


static bool kymeraAncCommon_MicDisconnectIndication(const mic_change_info_t *info)
{
    UNUSED(info);
    /* Stop & disconnect Adaptive ANC audio graph - due to client disconnection request */
    kymeraAncCommon_FreezeAdaptivity();
    kymeraAncCommon_Stop();
    kymeraAncCommon_Disconnect();
    return TRUE;
}

static void kymeraAncCommon_MicReconnectedIndication(void)
{
    kymeraAncCommon_Connect();
    kymeraAncCommon_Start();
}

static mic_user_state_t kymeraAncCommon_GetUserState(void)
{
    return mic_user_state_interruptible;
}

static bool kymeraAncCommon_GetAecRefUsage(void)
{
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    return TRUE;
#else
    return (KymeraEchoCanceller_IsActive() == TRUE);
#endif
}

static uint8 kymeraAncCommon_GetUcidOffset(void)
{
    aanc_ucid_offset ucid_offset = aanc_ucid_offset_default;
    if(KymeraSelfSpeechDetect_IsActive())
    {

#ifdef ENABLE_WIND_DETECT
        if(kymeraAncCommon_IsWindDetectInStage2())
        {
            ucid_offset = aanc_ucid_offset_wind_and_auto_transparency;
        }
        else
#endif
        {
            ucid_offset = aanc_ucid_offset_auto_transparency;
        }
    }
    else
    {
#ifdef ENABLE_WIND_DETECT
        if(kymeraAncCommon_IsWindDetectInStage2())
        {
            ucid_offset = aanc_ucid_offset_wind;
        }
#endif
    }
    return ucid_offset;
}

static bool kymeraAncCommon_IsConcurrencySwitchRequired(aanc_config_t config)
{
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    return ((config == aanc_config_anc) ||
            (config == aanc_config_leakthrough));
#else
    return ((config == aanc_config_adaptive_anc) ||
            (config == aanc_config_adaptive_ambient) ||
            (config == aanc_config_static_anc) ||
            (config == aanc_config_static_leakthrough));
#endif
}

static const mic_callbacks_t kymera_AncCommonCallbacks =
{
    .MicGetConnectionParameters = kymeraAncCommon_MicGetConnectionParameters,
    .MicDisconnectIndication = kymeraAncCommon_MicDisconnectIndication,
    .MicReconnectedIndication = kymeraAncCommon_MicReconnectedIndication,
    .MicGetUserState = kymeraAncCommon_GetUserState,
    .MicGetAecRefUsage = kymeraAncCommon_GetAecRefUsage,
    .MicGetUcidOffset = kymeraAncCommon_GetUcidOffset,
};

static const mic_registry_per_user_t kymera_AncCommonRegistry =
{
    .user = mic_user_aanc,
    .callbacks = &kymera_AncCommonCallbacks,
    .permanent.mandatory_mic_ids = &kymera_AncCommonMics[0],
    .permanent.num_of_mandatory_mics = MAX_ANC_COMMON_MICS,
    .permanent.mandatory_task_period_us = &kymera_AncCommonMicTaskPeriod,
};

/***************************************
 *** Kymera Output Manager callbacks ***
 ***************************************/
/*! Notifies registered user that another user is about to connect to the output chain. */
static void kymeraAncCommon_AdaptiveAncOutputConnectingIndication(output_users_t connecting_user, output_connection_t connection_type)
{
    UNUSED(connection_type);
    DEBUG_LOG_INFO("kymeraAncCommon_AdaptiveAncOutputConnectingIndication connecting user: enum:output_users_t:%d",connecting_user);

    data.connecting_user = connecting_user;

    KymeraAah_SetSysMode(kymeraAncCommon_GetAahConcurrencySysmode());

    if(kymeraAncCommon_IsAancCurrentConfigValid())
    {
        kymeraAncCommon_UpdateConcurrencyRequest(TRUE);

        /* Update optimum DSP clock for concurrency usecase */
        appKymeraConfigureDspPowerMode();

#ifndef ENABLE_UNIFIED_ANC_GRAPH
        if((!kymeraAncCommon_IsConcurrencyChainActive()) && (kymeraAncCommon_IsConcurrencySwitchRequired(kymeraAncCommon_GetAancCurrentConfig())))
        {
            KymeraAncCommon_StandaloneToConcurrencyReq();
        }
#else
        if(kymeraAncCommon_IsAdaptiveMode())
        {
            KymeraAdaptiveAnc_SetSysMode(adaptive_anc_sysmode_concurrency);
        }
#endif
    }
    else if (KymeraSelfSpeechDetect_IsActive())
    {
            kymeraAncCommon_UpdateConcurrencyRequest(TRUE);
    }
    KymeraEchoCanceller_UpdateBypassFbc(FALSE);
}

/*! Notifies the user the output chain is idle (no active users/the chain is destroyed). */
static void kymeraAncCommon_AdaptiveAncOutputIdleIndication(void)
{
    DEBUG_LOG_INFO("kymeraAncCommon_AdaptiveAncOutputIdleIndication Kymera_OutputIsChainInUse():%d",Kymera_OutputIsChainInUse());

    if(!Kymera_OutputIsChainInUse())
    {
        data.connecting_user = output_user_none;

        KymeraEchoCanceller_UpdateBypassFbc(TRUE);
        KymeraAah_SetSysMode(aah_sysmode_full);
        KymeraAah_SetLimitsForStandalone();

#ifndef ENABLE_UNIFIED_ANC_GRAPH
        if(kymeraAncCommon_IsConcurrencyChainActive())
        {
            KymeraAncCommon_ConcurrencyToStandaloneReq();
        }
        else
        {
            kymeraAncCommon_UpdateConcurrencyRequest(FALSE);
        }
#else
        if(kymeraAncCommon_IsAdaptiveMode())
        {
            KymeraAdaptiveAnc_SetSysMode(adaptive_anc_sysmode_full);
        }
        kymeraAncCommon_UpdateConcurrencyRequest(FALSE);
#endif

        /* Update optimum DSP clock for standalone usecase */
        appKymeraConfigureDspPowerMode();
    }
}

static void kymeraAncCommon_SetAancState(aanc_state_t state)
{
    data.aanc_state = state;
}

static aanc_state_t kymeraAncCommon_GetAancState(void)
{
    return (data.aanc_state);
}

static const output_indications_registry_entry_t aanc_user_info =
{
    .OutputConnectingIndication = kymeraAncCommon_AdaptiveAncOutputConnectingIndication,
    .OutputIdleIndication = kymeraAncCommon_AdaptiveAncOutputIdleIndication,
};



/***************************************
 ********* Global functions ************
 ***************************************/
void KymeraAncCommon_Init(void)
{
    Kymera_OutputRegisterForIndications(&aanc_user_info);
    kymera_AncCommonMicTaskPeriod = AEC_REF_TASK_PERIOD_1MS;
    Kymera_MicRegisterUser(&kymera_AncCommonRegistry);
    ahmRampingCompleteCallback = NULL;
    kymeraAncCommon_SetAancCurrentConfig(aanc_config_none);
    kymeraAncCommon_SetAdaptiveMode(FALSE);
#ifdef ENABLE_WIND_DETECT
    kymeraAncCommon_ResetWindDetectInStage2();
#endif
    KymeraHcgr_UpdateUserState(howling_detection_enabled);
    kymeraAncCommon_SetAancState(aanc_state_idle);
    kymeraAncCommon_SetModeChangeInProgress(FALSE);

    getData()->prev_filter_config = NULL;
}

void KymeraAncCommon_AncEnable(const KYMERA_INTERNAL_AANC_ENABLE_T *msg)
{
    DEBUG_LOG("KymeraAncCommon_AncEnable");

    if (kymeraAncCommon_GetAancCurrentConfig() == aanc_config_fit_test)
    {
        kymeraAncCommon_Enable(msg);
    }
    else
    {
        kymeraAncCommon_EnableOnUserRequest(msg);
    }
}

void KymeraAncCommon_AdaptiveAncDisable(void)
{
    if(kymeraAncCommon_GetAancCurrentConfig() == aanc_config_fit_test)
    {
        DEBUG_LOG("KymeraAncCommon_AdaptiveAncDisable");
        kymeraAncCommon_Disable();
    }
}
bool KymeraAncCommon_AdaptiveAncIsEnabled(void)
{
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
        case aanc_config_leakthrough:
            return KymeraAhm_IsActive();

        case aanc_config_fit_test:
            return KymeraAdaptiveAnc_IsActive();
#else
        case aanc_config_adaptive_anc:
        case aanc_config_adaptive_ambient:
        case aanc_config_static_anc:
        case aanc_config_static_leakthrough:
            return KymeraAhm_IsActive();

        case aanc_config_fit_test:
            return KymeraAdaptiveAnc_IsActive();
#endif
        default:
        /* Do nothing */
            break;
    }
    return FALSE;
}

void KymeraAncCommon_EnableQuietMode(void)
{
    if(kymeraAncCommon_IsAdaptiveMode())
    {
        DEBUG_LOG_FN_ENTRY("KymeraAncCommon_EnableQuietMode");
        kymeraAncCommon_AdaptiveAncFreezeAdaptivity();
        KymeraAhm_SetSysMode(ahm_sysmode_quiet);
#ifdef INCLUDE_ULTRA_QUIET_DAC_MODE
        kymeraAncCommon_RampGainsToQuiet(Kymera_RequestUltraQuietDac);
#else
        kymeraAncCommon_RampGainsToQuiet(NULL);
#endif
    }
}

void KymeraAncCommon_DisableQuietMode(void)
{
    if(kymeraAncCommon_IsAdaptiveMode())
    {
        DEBUG_LOG_FN_ENTRY("KymeraAncCommon_DisableQuietMode");
        Kymera_CancelUltraQuietDac();
        KymeraAhm_SetSysMode(ahm_sysmode_full);
        kymeraAncCommon_AdaptiveAncStartAdaptivity();
    }
}

static void KymeraAncCommon_CalculateTotalGainDb(int8 coarse_gain, uint8 fine_gain, int16 *total_gain_db, int16 *total_gain_fract)
{
    int total_gain = AncConvertGainTo16BitQFormat((uint16)fine_gain) + ((int16)coarse_gain * 512 * 6);
    if(fine_gain == 0)
    {
        total_gain = NONE_DB;
    }
    *total_gain_db = total_gain / 512;
    *total_gain_fract = abs((total_gain - *total_gain_db * 512) * 100 / 512);
}

void KymeraAncCommon_GetFFGain(uint8 *gain)
{
    int8 coarse_gain_inst0, coarse_gain_inst1;
    uint8 fine_gain_inst0, fine_gain_inst1;
    int16 total_gain_db_inst0, total_gain_fract_inst0, total_gain_db_inst1, total_gain_fract_inst1;
    KymeraAncCommon_GetCoarseGain(&coarse_gain_inst0, &coarse_gain_inst1, AUDIO_ANC_PATH_ID_FFB);
    KymeraAncCommon_GetFineGain(&fine_gain_inst0, &fine_gain_inst1, AUDIO_ANC_PATH_ID_FFB);
    KymeraAncCommon_CalculateTotalGainDb(coarse_gain_inst0, fine_gain_inst0, &total_gain_db_inst0, &total_gain_fract_inst0);
    KymeraAncCommon_CalculateTotalGainDb(coarse_gain_inst1, fine_gain_inst1, &total_gain_db_inst1, &total_gain_fract_inst1);
    DEBUG_LOG("KymeraAncCommon_GetFFGain: Inst0: coarse %d fine %d total %d.%02ddB   Inst1: coarse %d fine %d total %d.%02ddB",
               coarse_gain_inst0, fine_gain_inst0, total_gain_db_inst0, total_gain_fract_inst0,
               coarse_gain_inst1, fine_gain_inst1, total_gain_db_inst1, total_gain_fract_inst1);
    *gain = fine_gain_inst0;
}

void KymeraAncCommon_GetFBGain(uint8 *gain)
{
    int8 coarse_gain_inst0, coarse_gain_inst1;
    uint8 fine_gain_inst0, fine_gain_inst1;
    int16 total_gain_db_inst0, total_gain_fract_inst0, total_gain_db_inst1, total_gain_fract_inst1;
    KymeraAncCommon_GetCoarseGain(&coarse_gain_inst0, &coarse_gain_inst1, AUDIO_ANC_PATH_ID_FFA);
    KymeraAncCommon_GetFineGain(&fine_gain_inst0, &fine_gain_inst1, AUDIO_ANC_PATH_ID_FFA);
    KymeraAncCommon_CalculateTotalGainDb(coarse_gain_inst0, fine_gain_inst0, &total_gain_db_inst0, &total_gain_fract_inst0);
    KymeraAncCommon_CalculateTotalGainDb(coarse_gain_inst1, fine_gain_inst1, &total_gain_db_inst1, &total_gain_fract_inst1);
    DEBUG_LOG("KymeraAncCommon_GetFBGain: Inst0: coarse %d fine %d total %d.%02ddB   Inst1: coarse %d fine %d total %d.%02ddB",
               coarse_gain_inst0, fine_gain_inst0, total_gain_db_inst0, total_gain_fract_inst0,
               coarse_gain_inst1, fine_gain_inst1, total_gain_db_inst1, total_gain_fract_inst1);
    *gain = fine_gain_inst0;
}

void KymeraAncCommon_GetFineGain(uint8 *gain_inst0, uint8 *gain_inst1, audio_anc_path_id audio_anc_path)
{
    uint16 read_gain_inst0 = 0;
    uint16 read_gain_inst1 = 0;
    KymeraAhm_GetFineGain(&read_gain_inst0, &read_gain_inst1, audio_anc_path);
    *gain_inst0 = (read_gain_inst0 & 0xFF);
    *gain_inst1 = (read_gain_inst1 & 0xFF);
}

void KymeraAncCommon_GetCoarseGain(int8 *gain_inst0, int8 *gain_inst1, audio_anc_path_id audio_anc_path)
{
    int16 read_gain_inst0 = 0;
    int16 read_gain_inst1 = 0;
    KymeraAhm_GetCoarseGain(&read_gain_inst0, &read_gain_inst1, audio_anc_path);
    *gain_inst0 = (read_gain_inst0 & 0xFF);
    *gain_inst1 = (read_gain_inst1 & 0xFF);
}

audio_dsp_clock_type KymeraAncCommon_GetOptimalAudioClockAancScoConcurrency(appKymeraScoMode mode)
{
    audio_dsp_clock_type dsp_clock = AUDIO_DSP_TURBO_CLOCK;
    DEBUG_LOG("KymeraAncCommon_GetOptimalAudioClockAancScoConcurrency");

    if(appKymeraInConcurrency())
    {
        switch(mode)
        {
        case SCO_WB:
#if defined (KYMERA_SCO_USE_3MIC) || (defined (KYMERA_SCO_USE_2MIC) && defined(ENABLE_ANC_AAH))
            dsp_clock = AUDIO_DSP_TURBO_PLUS_CLOCK;
#endif
        break;
        case SCO_SWB:
        case SCO_UWB:
            dsp_clock = AUDIO_DSP_TURBO_PLUS_CLOCK;
        break;
        default:
        break;
        }
    }
    return dsp_clock;
}

audio_dsp_clock_type KymeraAncCommon_GetOptimalAudioClockAancMusicConcurrency(int seid)
{
    DEBUG_LOG("KymeraAncCommon_GetOptimalAudioClockAancMusicConcurrency");
#ifndef INCLUDE_DECODERS_ON_P1
    if(seid == AV_SEID_APTX_ADAPTIVE_SNK)
    {
        return AUDIO_DSP_TURBO_PLUS_CLOCK;
    }
#else
    UNUSED(seid);
#endif
#ifdef ENABLE_CONTINUOUS_EARBUD_FIT_TEST
    return AUDIO_DSP_TURBO_PLUS_CLOCK;
#else
    return AUDIO_DSP_TURBO_CLOCK;
#endif
}

void KymeraAncCommon_AdaptiveAncSetGainValues(uint32 mantissa, uint32 exponent)
{
    UNUSED(mantissa);
    UNUSED(exponent);
}

void KymeraAncCommon_UpdateInEarStatus(void)
{
    KymeraAhm_UpdateInEarStatus(TRUE);
}

void KymeraAncCommon_UpdateOutOfEarStatus(void)
{
    KymeraAhm_UpdateInEarStatus(FALSE);
}


void KymeraAncCommon_ExitAdaptiveAncTuning(const adaptive_anc_tuning_disconnect_parameters_t *param)
{
    UNUSED(param);

}

#ifdef ENABLE_ANC_FAST_MODE_SWITCH

static void kymeraAncCommon_ResetFilterConfig(kymera_anc_common_anc_filter_config_t* filter_config)
{
    if (filter_config != NULL)
    {
        free(filter_config);
    }
}

static void kymeraAncCommon_ResetPrevFilterConfig(void)
{
    kymeraAncCommon_ResetFilterConfig(data.prev_filter_config);
    data.prev_filter_config = NULL;
}

static kymera_anc_common_anc_filter_config_t* kymeraAncCommon_GetPrevFilterConfig(void)
{
    return data.prev_filter_config;
}

static void kymeraAncCommon_SetPrevFilterConfig(kymera_anc_common_anc_filter_config_t* filter_config)
{
    kymeraAncCommon_ResetFilterConfig(kymeraAncCommon_GetPrevFilterConfig());
    data.prev_filter_config = filter_config;
}

static void KymeraAncCommon_SetLpfCoefficients(audio_anc_instance instance, audio_anc_path_id path, uint8 lpf_shift_1, uint8 lpf_shift_2)
{
    DEBUG_LOG("KymeraAncCommon_SetLpfCoefficients, instance: enum:audio_anc_instance:%d, path: enum:audio_anc_path_id:%d, shift1: %d, shift2: %d", instance, path, lpf_shift_1, lpf_shift_2);

    PanicFalse(AncConfigureLPFCoefficients(instance, path, lpf_shift_1, lpf_shift_2));

}

static void KymeraAncCommon_SetDcFilterCoefficients(audio_anc_instance instance, audio_anc_path_id path, uint8 filter_shift, uint8 filter_enable)
{
    DEBUG_LOG("KymeraAncCommon_SetDcFilterCoefficients, instance: enum:audio_anc_instance:%d, path: enum:audio_anc_path_id:%d, shift: %d, shift: %d", instance, path, filter_shift, filter_enable);

    PanicFalse(AncConfigureDcFilterCoefficients(instance, path, filter_shift, filter_enable));

}

static void KymeraAncCommon_SetSmallLpfCoefficients(audio_anc_instance instance, uint8 filter_shift, uint8 filter_enable)
{
    DEBUG_LOG("KymeraAncCommon_SetSmallLpfCoefficients, instance: enum:audio_anc_instance:%d, shift: %d, shift: %d", instance, filter_shift, filter_enable);

    PanicFalse(AncConfigureSmLPFCoefficients(instance, filter_shift, filter_enable));

}

static kymera_anc_common_lpf_config_t* KymeraAncCommon_GetLpfCoefficientsForPath(audio_anc_instance instance, audio_anc_path_id path, kymera_anc_common_anc_filter_config_t* filter_config)
{
    DEBUG_LOG("KymeraAncCommon_GetLpfCoefficientsForPath, instance: enum:audio_anc_instance:%d, path: enum:audio_anc_path_id:%d", instance, path);

    kymera_anc_common_anc_instance_config_t* instance_config = KymeraAncCommon_GetInstanceConfig(filter_config, instance);

    kymera_anc_common_lpf_config_t *lpf_config = NULL;

    if (instance_config)
    {
        switch (path)
        {
        case AUDIO_ANC_PATH_ID_FFA:
        {
            lpf_config = &instance_config->ffa_config.lpf_config;
        }
        break;

        case AUDIO_ANC_PATH_ID_FFB:
        {
            lpf_config = &instance_config->ffb_config.lpf_config;
        }
        break;

        case AUDIO_ANC_PATH_ID_FB:
        {
            lpf_config = &instance_config->fb_config.lpf_config;
        }
        break;

        default:
        {
            lpf_config = NULL;
        }
        break;
        }
    }

    return lpf_config;
}

static kymera_anc_common_dc_config_t* KymeraAncCommon_GetDcFilterCoefficientsForPath(audio_anc_instance instance, audio_anc_path_id path, kymera_anc_common_anc_filter_config_t* filter_config)
{
    DEBUG_LOG("KymeraAncCommon_GetDcFilterCoefficientsForPath, instance: enum:audio_anc_instance:%d, path: enum:audio_anc_path_id:%d", instance, path);

    kymera_anc_common_anc_instance_config_t* instance_config = KymeraAncCommon_GetInstanceConfig(filter_config, instance);

    kymera_anc_common_dc_config_t *dc_config = NULL;

    if (instance_config)
    {
        switch (path)
        {
        case AUDIO_ANC_PATH_ID_FFA:
        {
            dc_config = &instance_config->ffa_config.dc_filter_config;
        }
        break;

        case AUDIO_ANC_PATH_ID_FFB:
        {
            dc_config = &instance_config->ffb_config.dc_filter_config;
        }
        break;

        default:
        {
            dc_config = NULL;
        }
        break;
        }
    }

    return dc_config;
}

static void KymeraAncCommon_SetLpfCoefficientsForPath(audio_anc_instance instance, audio_anc_path_id path)
{
    DEBUG_LOG("KymeraAncCommon_SetLpfCoefficientsForPath, instance: enum:audio_anc_instance:%d, path: enum:audio_anc_path_id:%d", instance, path);

    kymera_anc_common_lpf_config_t* curr_lpf_config = PanicUnlessMalloc(sizeof(kymera_anc_common_lpf_config_t));
    bool compare = kymeraAncCommon_IsModeChangeInProgress();
    bool update = TRUE;

    memset(curr_lpf_config, 0, sizeof(kymera_anc_common_lpf_config_t));

    KymeraAncCommon_ReadLpfCoefficients(instance, path, &curr_lpf_config->lpf_shift_1, &curr_lpf_config->lpf_shift_2);

    if (compare)
    {
        kymera_anc_common_anc_filter_config_t* prev_filter_config = kymeraAncCommon_GetPrevFilterConfig();
        kymera_anc_common_lpf_config_t* prev_lpf_config = KymeraAncCommon_GetLpfCoefficientsForPath(instance, path, prev_filter_config);

        if(prev_lpf_config && prev_lpf_config->lpf_shift_1 == curr_lpf_config->lpf_shift_1 &&
                prev_lpf_config->lpf_shift_2 == curr_lpf_config->lpf_shift_2)
        {
            update = FALSE;
        }
    }

    if (update)
    {
        KymeraAncCommon_SetLpfCoefficients(instance, path, curr_lpf_config->lpf_shift_1, curr_lpf_config->lpf_shift_2);
    }

    free(curr_lpf_config);
}

static void KymeraAncCommon_SetDcFilterCoefficientsForPath(audio_anc_instance instance, audio_anc_path_id path)
{
    DEBUG_LOG("KymeraAncCommon_SetDcFilterCoefficientsForPath, instance: enum:audio_anc_instance:%d, path: enum:audio_anc_path_id:%d", instance, path);

    kymera_anc_common_dc_config_t* curr_dc_config = PanicUnlessMalloc(sizeof(kymera_anc_common_dc_config_t));
    bool compare = kymeraAncCommon_IsModeChangeInProgress();
    bool update = TRUE;

    memset(curr_dc_config, 0, sizeof(kymera_anc_common_lpf_config_t));

    KymeraAncCommon_ReadDcFilterCoefficients(instance, path, &curr_dc_config->filter_shift, &curr_dc_config->filter_enable);

    if (compare)
    {
        kymera_anc_common_anc_filter_config_t* prev_filter_config = kymeraAncCommon_GetPrevFilterConfig();
        kymera_anc_common_dc_config_t* prev_dc_config = KymeraAncCommon_GetDcFilterCoefficientsForPath(instance, path, prev_filter_config);

        if(prev_dc_config && prev_dc_config->filter_shift == curr_dc_config->filter_shift &&
                prev_dc_config->filter_enable == curr_dc_config->filter_enable)
        {
            update = FALSE;
        }
    }

    if (update)
    {
        KymeraAncCommon_SetDcFilterCoefficients(instance, path, curr_dc_config->filter_shift, curr_dc_config->filter_enable);
    }

    free(curr_dc_config);
}

static void KymeraAncCommon_SetLpfCoefficientsForInstance(audio_anc_instance instance)
{
    DEBUG_LOG("KymeraAncCommon_SetLpfCoefficientsForInstance, instance: enum:audio_anc_instance:%d", instance);

    KymeraAncCommon_SetLpfCoefficientsForPath(instance, AUDIO_ANC_PATH_ID_FFA);
    KymeraAncCommon_SetLpfCoefficientsForPath(instance, AUDIO_ANC_PATH_ID_FFB);
    KymeraAncCommon_SetLpfCoefficientsForPath(instance, AUDIO_ANC_PATH_ID_FB);
}

static void KymeraAncCommon_SetDcFilterCoefficientsForInstance(audio_anc_instance instance)
{
    DEBUG_LOG("KymeraAncCommon_SetDcFilterCoefficientsForInstance, instance: enum:audio_anc_instance:%d", instance);

    KymeraAncCommon_SetDcFilterCoefficientsForPath(instance, AUDIO_ANC_PATH_ID_FFA);
    KymeraAncCommon_SetDcFilterCoefficientsForPath(instance, AUDIO_ANC_PATH_ID_FFB);
}

static void KymeraAncCommon_SetSmallLpfCoefficientsForInstance(audio_anc_instance instance)
{

    DEBUG_LOG("KymeraAncCommon_SetSmallLpfCoefficientsForInstance, instance: enum:audio_anc_instance:%d", instance);

    kymera_anc_common_small_lpf_config_t* curr_small_lpf_config = PanicUnlessMalloc(sizeof(kymera_anc_common_small_lpf_config_t));
    bool compare = kymeraAncCommon_IsModeChangeInProgress();
    bool update = TRUE;

    memset(curr_small_lpf_config, 0, sizeof(kymera_anc_common_lpf_config_t));

    KymeraAncCommon_ReadSmallLpfCoefficients(instance, &curr_small_lpf_config->filter_shift, &curr_small_lpf_config->filter_enable);

    if (compare)
    {
        kymera_anc_common_anc_filter_config_t* prev_filter_config = kymeraAncCommon_GetPrevFilterConfig();
        kymera_anc_common_anc_instance_config_t* instance_config = KymeraAncCommon_GetInstanceConfig(prev_filter_config, instance);
        kymera_anc_common_small_lpf_config_t* prev_small_lpf_config = instance_config != NULL ? &instance_config->small_lpf_config : NULL;

        if(prev_small_lpf_config && prev_small_lpf_config->filter_shift == curr_small_lpf_config->filter_shift &&
                prev_small_lpf_config->filter_enable == curr_small_lpf_config->filter_enable)
        {
            update = FALSE;
        }
    }

    if (update)
    {
        KymeraAncCommon_SetSmallLpfCoefficients(instance, curr_small_lpf_config->filter_shift, curr_small_lpf_config->filter_enable);
    }

    free(curr_small_lpf_config);
}

static void KymeraAncCommon_SetFilterCoefficientsForInstance(audio_anc_instance instance)
{
    DEBUG_LOG("KymeraAncCommon_SetFilterCoefficientsForInstance");

    KymeraAncCommon_SetLpfCoefficientsForInstance(instance);
    KymeraAncCommon_SetDcFilterCoefficientsForInstance(instance);
    KymeraAncCommon_SetSmallLpfCoefficientsForInstance(instance);
}

static void KymeraAncCommon_ReadLpfCoefficients(audio_anc_instance instance, audio_anc_path_id path, uint8* lpf_shift_1, uint8* lpf_shift_2)
{
    DEBUG_LOG("KymeraAncCommon_ReadLpfCoefficients, inst: enum:audio_anc_instance:%d, path: enum:audio_anc_path_id:%d", instance, path);

    PanicFalse(AncReadLpfCoefficients(instance, path, lpf_shift_1, lpf_shift_2));

    DEBUG_LOG("KymeraAncCommon_ReadLpfCoefficients, shift1: %d, shift2: %d", *lpf_shift_1, *lpf_shift_2);
}

static void KymeraAncCommon_ReadDcFilterCoefficients(audio_anc_instance instance, audio_anc_path_id path, uint8* filter_shift, uint8* filter_enable)
{
    DEBUG_LOG("KymeraAncCommon_ReadDcFilterCoefficients, inst: enum:audio_anc_instance:%d, path: enum:audio_anc_path_id:%d", instance, path);

    PanicFalse(AncReadDcFilterCoefficients(instance, path, filter_shift, filter_enable));

    DEBUG_LOG("KymeraAncCommon_ReadDcFilterCoefficients, filter_shift: %d, filter_enable: %d", *filter_shift, *filter_enable);

}

static void KymeraAncCommon_ReadSmallLpfCoefficients(audio_anc_instance instance, uint8* filter_shift, uint8* filter_enable)
{
    DEBUG_LOG("KymeraAncCommon_ReadSmallLpfCoefficients, inst: enum:audio_anc_instance:%d", instance);

    PanicFalse(AncReadSmallLpfCoefficients(instance, filter_shift, filter_enable));

    DEBUG_LOG("KymeraAncCommon_ReadSmallLpfCoefficients, filter_shift: %d, filter_enable: %d", *filter_shift, *filter_enable);
}

static void KymeraAncCommon_ReadLpfCoefficientsForPath(audio_anc_instance instance, audio_anc_path_id path, kymera_anc_common_anc_filter_config_t* filter_config)
{
    DEBUG_LOG("KymeraAncCommon_ReadLpfCoefficientsForPath, inst: enum:audio_anc_instance:%d, path: enum:audio_anc_path_id:%d", instance, path);

    kymera_anc_common_lpf_config_t* lpf_config = KymeraAncCommon_GetLpfCoefficientsForPath(instance, path, filter_config);

    if (lpf_config)
    {
        KymeraAncCommon_ReadLpfCoefficients(instance, path, &lpf_config->lpf_shift_1, &lpf_config->lpf_shift_2);
    }
}

static void KymeraAncCommon_ReadDcFilterCoefficientsForPath(audio_anc_instance instance, audio_anc_path_id path, kymera_anc_common_anc_filter_config_t* filter_config)
{
    DEBUG_LOG("KymeraAncCommon_ReadLpfCoefficientsForPath, inst: enum:audio_anc_instance:%d, path: enum:audio_anc_path_id:%d", instance, path);

    kymera_anc_common_dc_config_t* dc_config = KymeraAncCommon_GetDcFilterCoefficientsForPath(instance, path, filter_config);

    if (dc_config)
    {
        KymeraAncCommon_ReadDcFilterCoefficients(instance, path, &dc_config->filter_shift, &dc_config->filter_enable);
    }
}



static void KymeraAncCommon_ReadLpfCoefficientsForInstance(audio_anc_instance instance, kymera_anc_common_anc_filter_config_t* filter_config)
{
    DEBUG_LOG("KymeraAncCommon_ReadFilterCoefficientsForInstance, instance: enum:audio_anc_instance:%d", instance);

    KymeraAncCommon_ReadLpfCoefficientsForPath(instance, AUDIO_ANC_PATH_ID_FFA, filter_config);
    KymeraAncCommon_ReadLpfCoefficientsForPath(instance, AUDIO_ANC_PATH_ID_FFB, filter_config);
    KymeraAncCommon_ReadLpfCoefficientsForPath(instance, AUDIO_ANC_PATH_ID_FB, filter_config);
}

static void KymeraAncCommon_ReadDcFilterCoefficientsForInstance(audio_anc_instance instance, kymera_anc_common_anc_filter_config_t* filter_config)
{
    DEBUG_LOG("KymeraAncCommon_ReadDcFilterCoefficientsForInstance, instance: enum:audio_anc_instance:%d", instance);

    KymeraAncCommon_ReadDcFilterCoefficientsForPath(instance, AUDIO_ANC_PATH_ID_FFA, filter_config);
    KymeraAncCommon_ReadDcFilterCoefficientsForPath(instance, AUDIO_ANC_PATH_ID_FFB, filter_config);
}

static void KymeraAncCommon_ReadSmallLpfCoefficientsForInstance(audio_anc_instance instance, kymera_anc_common_anc_filter_config_t* filter_config)
{
    DEBUG_LOG("KymeraAncCommon_ReadSmallLpfCoefficients");

    kymera_anc_common_anc_instance_config_t* instance_config = KymeraAncCommon_GetInstanceConfig(filter_config, instance);
    kymera_anc_common_small_lpf_config_t* small_lpf_config = instance_config != NULL ? &instance_config->small_lpf_config : NULL;

    if (small_lpf_config)
    {
        KymeraAncCommon_ReadSmallLpfCoefficients(instance, &small_lpf_config->filter_shift, &small_lpf_config->filter_enable);
    }
}

static void KymeraAncCommon_ReadFilterCoefficientsForInstance(audio_anc_instance instance, kymera_anc_common_anc_filter_config_t* filter_config)
{
    DEBUG_LOG("KymeraAncCommon_ReadFilterCoefficientsForInstance, instance: enum:audio_anc_instance:%d", instance);

    KymeraAncCommon_ReadLpfCoefficientsForInstance(instance, filter_config);
    KymeraAncCommon_ReadDcFilterCoefficientsForInstance(instance, filter_config);
    KymeraAncCommon_ReadSmallLpfCoefficientsForInstance(instance, filter_config);
}

static void KymeraAncCommon_GetFilterConfig(kymera_anc_common_anc_filter_config_t* filter_config)
{
    DEBUG_LOG("KymeraAncCommon_GetFilterConfig");

    filter_config->mode = AncStateManager_GetCurrentMode();

    KymeraAncCommon_ReadFilterCoefficientsForInstance(AUDIO_ANC_INSTANCE_0, filter_config);
    if (appKymeraIsParallelAncFilterEnabled())
    {
        KymeraAncCommon_ReadFilterCoefficientsForInstance(AUDIO_ANC_INSTANCE_1, filter_config);
    }
}

static bool KymeraAncCommon_IsAncModeLeakthrough(anc_mode_t mode)
{
    bool is_leakthrough = AncConfig_IsAncModeLeakThrough(mode);
    DEBUG_LOG("KymeraAncCommon_IsAncModeLeakthrough: %d", is_leakthrough);

    return is_leakthrough;
}

static bool KymeraAncCommon_IsAncModeNoiseCancellation(anc_mode_t mode)
{
    bool is_anc = !AncConfig_IsAncModeLeakThrough(mode);
    DEBUG_LOG("KymeraAncCommon_IsAncModeNoiseCancellation: %d", is_anc);

    return is_anc;
}

static ahm_trigger_transition_ctrl_t KymeraAncCommon_GetTransition(anc_mode_t prev_mode, anc_mode_t curr_mode)
{
    DEBUG_LOG("KymeraAncCommon_GetTransition");

    ahm_trigger_transition_ctrl_t transition = ahm_trigger_transition_ctrl_dissimilar_filters;

    if (KymeraAncCommon_IsAncModeLeakthrough(prev_mode) == KymeraAncCommon_IsAncModeLeakthrough(curr_mode) ||
            KymeraAncCommon_IsAncModeNoiseCancellation(prev_mode) == KymeraAncCommon_IsAncModeNoiseCancellation(curr_mode))
    {
        transition = ahm_trigger_transition_ctrl_similar_filters;
    }

    return transition;
}

static kymera_anc_common_anc_instance_config_t* KymeraAncCommon_GetInstanceConfig(kymera_anc_common_anc_filter_config_t* filter_config, audio_anc_instance instance)
{
    DEBUG_LOG("KymeraAncCommon_GetInstanceConfig");

    kymera_anc_common_anc_instance_config_t* instance_config = NULL;

    if (filter_config != NULL)
    {
        if (instance == AUDIO_ANC_INSTANCE_0)
        {
            instance_config = &filter_config->anc_instance_0_config;
        }
        else if (instance == AUDIO_ANC_INSTANCE_1)
        {
            instance_config = &filter_config->anc_instance_1_config;
        }
    }

    return instance_config;
}

static void KymeraAncCommon_AhmTriggerTransitionWithFilterUpdate(audio_anc_path_id anc_path, adaptive_anc_hw_channel_t anc_hw_channel, ahm_trigger_transition_ctrl_t transition)
{
    DEBUG_LOG("KymeraAncCommon_AhmTriggerTransitionWithFilterUpdate");

    if (transition == ahm_trigger_transition_ctrl_similar_filters ||
            transition == ahm_trigger_transition_ctrl_dissimilar_filters)
    {
        /* Configure AHM, start adaptivity and update filters on ramp down complete during mode change */
        ahmRampingCompleteCallback = kymeraAncCommon_RampDownCompleteActionDuringModeTransition;

        /* To handle scenarios where IIR filters are exactly same that there won't be any ramps and wouldn't receive ramp down complete message. */
        if (transition == ahm_trigger_transition_ctrl_similar_filters)
        {
            /* Cancel any pending Mode Transition Ramp Down timeout messages */
            MessageCancelAll(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_MODE_TRANSITION_AHM_RAMP_DOWN_TIMEOUT);

            /*Inform Kymera module to configure AHM and update filters after timeout */
            MessageSendLater(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_MODE_TRANSITION_AHM_RAMP_DOWN_TIMEOUT, NULL, AHM_RAMP_DOWN_TIMEOUT_MS);
        }
    }

    KymeraAhm_TriggerTransitionWithFilterUpdate(anc_path, anc_hw_channel, transition);

    /* Cancel any pending AHM Transition timeout messages */
    MessageCancelAll(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_AHM_TRANSITION_TIMEOUT);

    /*Inform Kymera module to put AHM in full proc after timeout */
    MessageSendLater(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_AHM_TRANSITION_TIMEOUT, NULL, AHM_TRANSITION_TIMEOUT_MS);
}

/* Sequence for Mode change is
i) KymeraAncCommon_PreAncModeChange: Store previous mode LPF, DC Filter, Small LPF coefficients
ii) Update ANC lib with new mode coefficients
iii) KymeraAncCommon_ApplyModeChange: Triggered from ANC SM
    - Update IIR coefficients and gains to AHM and trigger transition
    - Update LPF, DC, Small LPF coefficients to ANC HW if they differ from previous mode
    - Disconnect and Destroy ANC chains in capability except AHM
    - Enable new mode with new configuration in capabilities
*/

/* Transition the AHM in preparation for Mode change*/
void KymeraAncCommon_PreAncModeChange(void)
{
    DEBUG_LOG("KymeraAncCommon_PreAncModeChange");

    kymera_anc_common_anc_filter_config_t* filter_config = PanicUnlessMalloc((sizeof(kymera_anc_common_anc_filter_config_t)));
    memset(filter_config, 0, sizeof(kymera_anc_common_anc_filter_config_t));

    KymeraAncCommon_GetFilterConfig(filter_config);

    kymeraAncCommon_SetPrevFilterConfig(filter_config);
}

#else

/* Sequence for Mode change is
i) KymeraAncCommon_PreAncModeChange: Ramp gains
ii) kymeraAncCommon_ModeChangeActionPostGainRamp: Inform ANC SM to change mode in HW
iii) KymeraAncCommon_ApplyModeChange: Triggered from ANC SM
    - Disconnect and Destroy ANC chains in capability except AHM
    - Enable new mode with new configuration in capabilities
*/

/*Ramp down the gains in preparation for Mode change*/
void KymeraAncCommon_PreAncModeChange(void)
{
    DEBUG_LOG("KymeraAncCommon_PreAncModeChange");
    kymeraAncCommon_FreezeAdaptivity();
    kymeraAncCommon_RampGainsToMute(kymeraAncCommon_ModeChangeActionPostGainRamp);

    /* Handle Anc Mode change action if the AHM ramp completion message is not received */
    MessageSendLater(AncStateManager_GetTask(), KYMERA_ANC_COMMON_CAPABILITY_MODE_CHANGE_TRIGGER,
                     NULL, AHM_RAMP_TIMEOUT_MS);
    Kymera_CancelUltraQuietDac();
}

#endif /* ENABLE_ANC_FAST_MODE_SWITCH */

/*Apply Mode change to capabilties */
bool KymeraAncCommon_ApplyModeChange(anc_mode_t mode, audio_anc_path_id anc_path, adaptive_anc_hw_channel_t anc_hw_channel)
{
    bool chain_rebuild_required = FALSE;
    KYMERA_INTERNAL_AANC_ENABLE_T settings;
    settings.control_path = anc_path;
    settings.current_mode = mode;
    settings.hw_channel = anc_hw_channel;
    settings.in_ear = TRUE;

    kymeraAncCommon_SetModeChangeInProgress(TRUE);

    kymeraAncCommon_StoreModeSettings(&settings);

#ifdef ENABLE_ANC_FAST_MODE_SWITCH
    /* Set AHM sysmode to 'standby' before the audio reconstruction or new similar filter update */
    KymeraAhm_SetSysMode(ahm_sysmode_standby);

    kymeraAncCommon_FreezeAdaptivity();
#endif

    DEBUG_LOG("KymeraAncCommon_ApplyModeChange, Current config:enum:aanc_config_t:%d, New Config:enum:aanc_config_t:%d chain created",
    kymeraAncCommon_GetAancCurrentConfig(), kymeraAncCommon_GetAancConfigFromMode(mode));

    /* if current and new aanc config same type */
    if(kymeraAncCommon_GetAancCurrentConfig() == kymeraAncCommon_GetAancConfigFromMode(mode))
    {
        DEBUG_LOG("KymeraAncCommon_ApplyModeChange, Same AANC config type");
        kymeraAncCommon_SetAancCurrentConfig(kymeraAncCommon_GetAancConfigFromMode(mode));
        kymeraAncCommon_SetAdaptiveMode(AncConfig_IsAncModeAdaptive(mode));

#ifdef ENABLE_ANC_FAST_MODE_SWITCH
        /* Set Static gains */
        KymeraAhm_SetStaticGain(anc_path, anc_hw_channel);
#endif /* ENABLE_ANC_FAST_MODE_SWITCH */

        kymeraAncCommon_ApplyModeChangeSettings(&settings);
    }
    else
    {
        /* Tear down current audio graph */
        kymeraAncCommon_Disable();

        DEBUG_LOG("KymeraAncCommon_ApplyModeChange, Disabled current mode");
        kymeraAncCommon_SetAancCurrentConfig(kymeraAncCommon_GetAancConfigFromMode(mode));
        kymeraAncCommon_SetAdaptiveMode(AncConfig_IsAncModeAdaptive(mode));

#ifdef ENABLE_ANC_FAST_MODE_SWITCH
        /* Set Static gains */
        KymeraAhm_SetStaticGain(anc_path, anc_hw_channel);
#endif /* ENABLE_ANC_FAST_MODE_SWITCH */

        /* Bring new audio graph */
        kymeraAncCommon_Enable(&settings);
        chain_rebuild_required = TRUE;
    }

#ifdef ENABLE_ANC_FAST_MODE_SWITCH
    ahm_trigger_transition_ctrl_t transition;

    transition = KymeraAncCommon_GetTransition(kymeraAncCommon_GetPrevFilterConfig()->mode,
                                                mode);

    KymeraAncCommon_AhmTriggerTransitionWithFilterUpdate(anc_path, anc_hw_channel, transition);

#else
    /* When Fast mode switch is enabled, mode change gets completed on receiving Transition complete message. */
    kymeraAncCommon_SetModeChangeInProgress(FALSE);
#endif /* ENABLE_ANC_FAST_MODE_SWITCH */
    return chain_rebuild_required;
}

/* Sequence for ANC Disable is
i) KymeraAncCommon_PreAncDisable: Ramp gains
ii) kymeraAncCommon_DisableOnUserRequest:
   - kymeraAncCommon_Disable: Disconnect and Destroy ANC chains in capability
   - Inform ANC SM to disable ANC HW
*/
void KymeraAncCommon_PreAncDisable(void)
{
    if(kymeraAncCommon_GetAancCurrentConfig() != aanc_config_fit_test)
    {
        DEBUG_LOG("KymeraAncCommon_PreAncDisable");

        kymeraAncCommon_FreezeAdaptivity();
        /* Register callback action after gain ramping complete */
        kymeraAncCommon_RampGainsToMute(kymeraAncCommon_DisableOnUserRequest);

        /* Handle Anc disable action if the AHM ramp completion message is not received */
        MessageSendLater(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_AHM_RAMP_TIMEOUT,
                         NULL, AHM_RAMP_TIMEOUT_MS);
    }
}

void KymeraAncCommon_AdaptiveAncEnableGentleMute(void)
{
    /*No action for ANC Advanced as Gentle mute is now happening as ramp down in AHM*/
}

void KymeraAncCommon_EnterAdaptiveAncTuning(const adaptive_anc_tuning_connect_parameters_t *param)
{
    UNUSED(param);
}

void KymeraAncCommon_EnableAdaptivity(void)
{
    kymeraAncCommon_StartAdaptivity();
}

void KymeraAncCommon_DisableAdaptivity(void)
{
    kymeraAncCommon_FreezeAdaptivity();
}

void KymeraAncCommon_AdaptiveAncSetUcid(anc_mode_t mode)
{
    UNUSED(mode);
}

bool KymeraAncCommon_GetApdativeAncCurrentMode(adaptive_anc_mode_t *aanc_mode)
{
    UNUSED(aanc_mode);
    return FALSE;
}

bool KymeraAncCommon_GetApdativeAncV2CurrentMode(adaptive_ancv2_sysmode_t *aancv2_mode)
{
    PanicNull(aancv2_mode);
    return KymeraAdaptiveAnc_GetSysMode(aancv2_mode);
}

bool KymeraAncCommon_GetAhmMode(ahm_sysmode_t *ahm_mode)
{
    PanicNull(ahm_mode);
    return KymeraAhm_GetSysMode(ahm_mode);
}

bool KymeraAncCommon_AdaptiveAncIsNoiseLevelBelowQmThreshold(void)
{
    return KymeraAdaptiveAnc_IsNoiseLevelBelowQuietModeThreshold();
}

bool KymeraAncCommon_AdaptiveAncIsConcurrencyActive(void)
{
    return ((kymeraAncCommon_IsAancCurrentConfigValid() || (KymeraSelfSpeechDetect_IsActive())) && kymeraAncCommon_IsConcurrencyRequested());
}

void KymeraAncCommon_CreateAdaptiveAncTuningChain(const KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_START_T *msg)
{
    UNUSED(msg);
}

void KymeraAncCommon_DestroyAdaptiveAncTuningChain(const KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_STOP_T *msg)
{
    UNUSED(msg);
}

void KymeraAncCommon_RampCompleteAction(void)
{
    DEBUG_LOG("KymeraAncCommon_RampCompleteAction!");

    if(ahmRampingCompleteCallback)
    {
        ahmRampingCompleteCallback();
        ahmRampingCompleteCallback = NULL;
    }
}

#ifdef ENABLE_ANC_FAST_MODE_SWITCH

static void kymeraAncCommon_UpdateFilter(void)
{
    /* Update LPF, DC, SmLPF coefficients */
    KymeraAncCommon_SetFilterCoefficients();
    kymeraAncCommon_ResetPrevFilterConfig();

    /* Enable RxMix */
    AncSetRxMixEnables();
}

static void kymeraAncCommon_RampDownCompleteActionDuringModeTransition(void)
{
    DEBUG_LOG_INFO("kymeraAncCommon_RampDownCompleteActionDuringModeTransition");
    KYMERA_INTERNAL_AANC_ENABLE_T mode_settings = getData()->mode_settings;

    /* Cancel any pending Mode Transition Ramp Down timeout messages */
    MessageCancelAll(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_MODE_TRANSITION_AHM_RAMP_DOWN_TIMEOUT);

    KymeraAhm_ApplyModeChange(&mode_settings, kymeraAncCommon_GetAhmTopologyBasedOnCurrentConfig(kymeraAncCommon_GetAancCurrentConfig()));
#ifdef ENABLE_UNIFIED_ANC_GRAPH
    kymeraAncCommon_StartOrFreezeAdaptivity();
    KymeraAncCommon_NoiseIDEnableOrDisable();
#else
    if (kymeraAncCommon_GetAancCurrentConfig() == aanc_config_adaptive_anc)
    {
        kymeraAncCommon_AdaptiveAncStartAdaptivity();
    }
#endif
    kymeraAncCommon_UpdateFilter();
}

#endif /* ENABLE_ANC_FAST_MODE_SWITCH */

void KymeraAncCommon_AhmRampExpiryAction(void)
{
    DEBUG_LOG("KymeraAncCommon_AhmRampExpiryAction");
    if(KymeraAncCommon_AdaptiveAncIsEnabled())
    {
        DEBUG_LOG("Action, In the case of AHM Ramp completion message failed to reach appliation");
        kymeraAncCommon_DisableOnUserRequest();
    }
    /* Unregister the ANC disable callback because it has already been handled. */
    ahmRampingCompleteCallback = NULL;
}

void KymeraAncCommon_AncCompanderMakeupGainVolumeDown(void)
{
    KymeraAdaptiveAmbient_AncCompanderMakeupGainVolumeDown();
}

void KymeraAncCommon_AncCompanderMakeupGainVolumeUp(void)
{
    KymeraAdaptiveAmbient_AncCompanderMakeupGainVolumeUp();
}

bool KymeraAncCommon_UpdateAhmFfPathFineTargetGain(uint8 ff_fine_gain)
{
    bool status = FALSE;
    ahm_sysmode_t curr_ahm_mode;

    if (KymeraAhm_IsActive())
    {
        KymeraAncCommon_GetAhmMode(&curr_ahm_mode);

        if (curr_ahm_mode != ahm_sysmode_wind)
        {
            KymeraAhm_SetTargetGain((uint16)ff_fine_gain);
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
            if (!kymeraAncCommon_IsModeChangeInProgress())
#endif
            {
                /* AHM ramps to target gain only after setting it's sysmode to fullproc */
                KymeraAhm_SetSysMode(ahm_sysmode_full);
            }
            status = TRUE;
        }
    }

    return status;
}

bool KymeraAncCommon_UpdateAncCompanderMakeupGain(int32 makeup_gain_fixed_point)
{
    return KymeraAdaptiveAmbient_UpdateAncCompanderMakeupGain(makeup_gain_fixed_point);
}

bool KymeraAncCommon_GetAncCompanderMakeupQGain(int32* makeup_gain_fixed_point)
{
    return KymeraAdaptiveAmbient_GetCompanderGain(makeup_gain_fixed_point);
}

static uint16 kymeraAncCommon_GetTargetGain(void)
{
    uint16 target_gain=0;

    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
            if(kymeraAncCommon_IsAdaptiveMode())
            {
                target_gain = KymeraAdaptiveAnc_GetFreezedGain();
            }
            else
            {
                target_gain = KymeraAncCommon_GetAhmAdjustedFfFineGain();
            }
        break;

        case aanc_config_leakthrough:
            if(kymeraAncCommon_IsAdaptiveMode())
            {
                target_gain = KymeraAdaptiveAmbient_GetCompanderAdjustedGain();
            }
            else
            {
                target_gain = AncStateManager_GetAncGain();
            }
        break;
#else
        case aanc_config_adaptive_anc:
            target_gain = KymeraAdaptiveAnc_GetFreezedGain();
        break;

        case aanc_config_adaptive_ambient:
            target_gain = KymeraAdaptiveAmbient_GetCompanderAdjustedGain();
        break;

        case aanc_config_static_anc:
            target_gain = KymeraAncCommon_GetAhmAdjustedFfFineGain();
            break;

        case aanc_config_static_leakthrough:
            target_gain = AncStateManager_GetAncGain();/*The user configured gain*/
        break;
#endif
        default:
            break;
    }

    DEBUG_LOG("kymeraAncCommon_GetTargetGain %d", target_gain);
    return target_gain;
}

bool KymeraAncCommon_IsHowlingDetectionSupported(void)
{
    return TRUE;
}

bool KymeraAncCommon_IsHowlingDetectionEnabled(void)
{
    return KymeraHcgr_IsHowlingDetectionEnabled();
}

void KymeraAncCommon_UpdateHowlingDetectionState(bool enable)
{
    DEBUG_LOG("KymeraAncCommon_UpdateHowlingDetectionState");
    if(enable)
    {
        KymeraHcgr_UpdateUserState(howling_detection_enabled);
        KymeraHcgr_SetHowlingControlSysMode(hc_sysmode_full);
        AncStateManager_HowlingDetectionStateUpdateInd(howling_detection_enabled);
    }
    else
    {
        KymeraHcgr_UpdateUserState(howling_detection_disabled);
        KymeraHcgr_SetHowlingControlSysMode(hc_sysmode_standby);
        AncStateManager_HowlingDetectionStateUpdateInd(howling_detection_disabled);
    }
}

void KymeraAncCommon_UpdateAahState(bool enable)
{
    DEBUG_LOG("KymeraAncCommon_UpdateAahState");
    if(enable)
    {
        KymeraAah_SetSysMode(aah_sysmode_full);
        AncStateManager_AahStateUpdateInd(aah_sysmode_full);
    }
    else
    {
        KymeraAah_SetSysMode(aah_sysmode_standby);
        AncStateManager_AahStateUpdateInd(aah_sysmode_standby);
    }
}

bool KymeraAncCommon_IsAancActive(void)
{
    switch(kymeraAncCommon_GetAancState())
    {
    case aanc_state_idle:
        return KymeraAncCommon_AdaptiveAncIsEnabled();

    case aanc_state_enable_initiated:
    case aanc_state_enabled:
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

void KymeraAncCommon_NoiseIDEnable(void)
{
    if(AncNoiseId_IsFeatureEnabled()
            && AncConfig_IsNoiseIdSupportedForMode(AncStateManager_GetCurrentMode())
            && AncNoiseId_CanProcess())
    {
        DEBUG_LOG("KymeraAncCommon_NoiseIDEnable");
        KymeraNoiseID_Enable();
    }
}

void KymeraAncCommon_NoiseIDEnableOrDisable(void)
{
    if(AncNoiseId_IsFeatureEnabled()
            && AncConfig_IsNoiseIdSupportedForMode(AncStateManager_GetCurrentMode())
            && AncNoiseId_CanProcess())
    {
        KymeraNoiseID_Enable();
    }
    else
    {
        KymeraNoiseID_Disable();
    }
}

void KymeraAncCommon_NoiseIDDisable(void)
{
    DEBUG_LOG("KymeraAncCommon_NoiseIDDisable");
    KymeraNoiseID_Disable();
}

void KymeraAncCommon_SetCategoryBasedOnCurrentMode(void)
{
    KymeraNoiseID_SetCategoryBasedOnCurrentMode();
}

void KymeraAncCommon_SetNoiseID(noise_id_category_t nid)
{
    KymeraNoiseID_SetCurrentCategory(nid);
}

noise_id_category_t KymeraAncCommon_GetNoiseID(void)
{
    noise_id_category_t nid;
    KymeraNoiseID_GetCurrentNoiseCategory(&nid);

    return nid;
}

#if defined(ENABLE_WIND_DETECT)

void KymeraAncCommon_EnableWindDetect(void)
{
    /*Always start with 1-mic mode*/
    KymeraWindDetect_SetSysMode1Mic();
}

void KymeraAncCommon_DisableWindDetect(void)
{
    KymeraWindDetect_SetSysModeStandby();
}

static bool kymeraAncCommon_IsWindConfirmed(void)
{
    return (data.wind_confirmed);
}

static void kymeraAncCommon_SetWindConfirmed(void)
{
    data.wind_confirmed = TRUE;
}

static void kymeraAncCommon_ResetWindConfirmed(void)
{
    data.wind_confirmed = FALSE;
}

static bool kymeraAncCommon_IsWindDetectInStage2(void)
{
    return (data.wind_detect_in_stage2);
}

static void kymeraAncCommon_SetWindDetectInStage2(void)
{
    if (appConfigWindDetect2MicSupported())
    {
        data.wind_detect_in_stage2=TRUE;
    }
}

static void kymeraAncCommon_ResetWindDetectInStage2(void)
{
    kymeraAncCommon_ResetWindConfirmed(); /* AHM sysmode would have been set to full proc if Wind is released in stage-2 */
    data.wind_detect_in_stage2 = FALSE;
}


static void kymeraAncCommon_RampGainsToTarget(kymeraAncCommonCallback func)
{
    DEBUG_LOG("kymeraAncCommon_RampGainsToTarget");
    ahmRampingCompleteCallback = func;
    KymeraAhm_SetSysMode(ahm_sysmode_full);
}

#ifdef ENABLE_WIND_INTENSITY_DETECT
static void kymeraAncCommon_UpdateWindMitigationParameters(wind_detect_intensity_t intensity)
{
    wind_mitigation_parameters_t* wind_params = PanicUnlessMalloc(sizeof(wind_mitigation_parameters_t));
    KymeraWindDetect_GetMitigationParametersForIntensity(intensity, wind_params);
    KymeraAhm_SetWindMitigationParameters(wind_params);
    free(wind_params);
}
#endif

static void kymeraAncCommon_WindConfirmed(wind_detect_intensity_t intensity)
{
    DEBUG_LOG_INFO("kymeraAncCommon_WindConfirmed, intensity: enum:wind_detect_intensity_t:%d", intensity);
    kymeraAncCommon_SetWindConfirmed();
#ifdef ENABLE_WIND_INTENSITY_DETECT
    kymeraAncCommon_UpdateWindMitigationParameters(intensity);
#endif
    KymeraAhm_SetSysMode(ahm_sysmode_wind);
}


static void kymeraAncCommon_StartOnWindRelease(void)
{
    kymeraAncCommon_StartClientConsumer(data.op_consumer_ff, data.op_consumer_fb);

    DEBUG_LOG("kymeraAncCommon_StartOnWindRelease");
    switch(kymeraAncCommon_GetAancCurrentConfig())
    {
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        case aanc_config_anc:
            DEBUG_LOG("kymeraAncCommon_StartOnWindRelease: ANC");
            KymeraAhm_SetTargetGain(kymeraAncCommon_GetTargetGain());
            kymeraAncCommon_RampGainsToTarget(kymeraAncCommon_StartOrFreezeAdaptivity);
            KymeraHcgr_Start();
            kymeraAncCommon_AhmStart(); /* Start ramping to target gains */
            KymeraWindDetect_Start();
            KymeraNoiseID_Start();
            KymeraAncCommon_NoiseIDEnableOrDisable();
            KymeraAdaptiveAnc_StartChain();
            kymeraAncCommon_EchoCancellerStart();
            kymeraAncCommon_StartRefSplitter();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
        break;

        case aanc_config_leakthrough:
            DEBUG_LOG("kymeraAncCommon_StartOnWindRelease: Transparency");
            KymeraAhm_SetTargetGain(kymeraAncCommon_GetTargetGain());
            kymeraAncCommon_RampGainsToTarget(kymeraAncCommon_StartOrFreezeAdaptivity);
            KymeraHcgr_Start();
            kymeraAncCommon_AhmStart(); /* Start ramping to target gains */
            KymeraWindDetect_Start();
            KymeraAdaptiveAmbient_StartChain();
            kymeraAncCommon_EchoCancellerStart();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
        break;
#else
        case aanc_config_adaptive_anc:
            kymeraAncCommon_RampGainsToTarget(kymeraAncCommon_StartAdaptivity);
            KymeraHcgr_Start();
            kymeraAncCommon_AhmStart(); /* Start ramping to target gains */
            KymeraWindDetect_Start();
            KymeraNoiseID_Start();
            KymeraAncCommon_NoiseIDEnableOrDisable();
            KymeraAdaptiveAnc_StartChain();
            kymeraAncCommon_EchoCancellerStart();
            kymeraAncCommon_StartRefSplitter();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
        break;

        case aanc_config_adaptive_ambient:
            KymeraAhm_SetTargetGain(kymeraAncCommon_GetTargetGain());
            kymeraAncCommon_RampGainsToTarget(kymeraAncCommon_StartAdaptivity);
            KymeraHcgr_Start();
            kymeraAncCommon_AhmStart(); /* Start ramping to target gains */
            KymeraWindDetect_Start();
            KymeraAdaptiveAmbient_StartChain();
            kymeraAncCommon_EchoCancellerStart();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
            break;

        case aanc_config_static_anc:
            KymeraAhm_SetTargetGain(kymeraAncCommon_GetTargetGain());
            kymeraAncCommon_RampGainsToTarget(kymeraAncCommon_StartAdaptivity);
            KymeraHcgr_Start();
            kymeraAncCommon_AhmStart(); /* Start ramping to target gains */
            KymeraWindDetect_Start();
            KymeraNoiseID_Start();
            KymeraAncCommon_NoiseIDEnableOrDisable();
            kymeraAncCommon_EchoCancellerStart();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
        break;

        case aanc_config_static_leakthrough:
            KymeraAhm_SetTargetGain(kymeraAncCommon_GetTargetGain());
            kymeraAncCommon_RampGainsToTarget(kymeraAncCommon_StartAdaptivity);
            KymeraHcgr_Start();
            kymeraAncCommon_AhmStart(); /* Start ramping to target gains */
            KymeraWindDetect_Start();
            kymeraAncCommon_EchoCancellerStart();
            kymeraAncCommon_AahStart();
            KymeraSelfSpeechDetect_Start();
            KymeraBasicPassthrough_Start();
            break;
#endif
        default:
            break;
    }
}

/*Disconnect diversity mic*/
static void kymeraAncCommon_MoveToStage1(void)
{
    if (appConfigWindDetect2MicSupported())
    {
        kymeraAncCommon_AncMicDisconnect();

        /*Connect mic with new configuration*/
        kymeraAncCommon_ResetWindDetectInStage2();
        if (!Kymera_MicConnect(mic_user_aanc))
        {
            DEBUG_LOG("MicConnect failure on kymeraAncCommon_MoveToStage1");
        }
        else
        {
            kymeraAncCommon_Connect();
            KymeraWindDetect_SetSysMode1Mic();
            kymeraAncCommon_StartOnWindRelease();
        }
    }
}

/*Connect diversity mic*/
static void kymeraAncCommon_MoveToStage2(void)
{
    DEBUG_LOG("kymeraAncCommon_MoveToStage2");
    if (appConfigWindDetect2MicSupported())
    {
        kymeraAncCommon_AncMicDisconnect();

        /*Connect mic with new configuration*/
        kymeraAncCommon_SetWindDetectInStage2();
        if (!Kymera_MicConnect(mic_user_aanc))
        {
            DEBUG_LOG("MicConnect failure on kymeraAncCommon_MoveToStage2");
        }
        else
        {
            kymeraAncCommon_Connect();
            KymeraWindDetect_SetSysMode2Mic();
            kymeraAncCommon_Start();
        }
    }
}

/*Freeze AANC, disconnect mics, connect new mic and put wind detect in 2-mic mode*/
void KymeraAncCommon_WindDetectAttack(windDetectStatus_t attack_stage, wind_detect_intensity_t wind_intensity)
{
    DEBUG_LOG("KymeraAncCommon_WindDetectAttack attack_stage: enum:windDetectStatus_t:%d, intensity: enum:wind_detect_intensity_t:%d", attack_stage, wind_intensity);

    if (appConfigWindDetect2MicSupported())
    {
        if (attack_stage==stage1_wind_detected)
        {
            /*Move to stage 2: Ignore if already in stage 2*/
            if (!kymeraAncCommon_IsWindDetectInStage2())
            {
                kymeraAncCommon_MoveToStage2();
            }
        }
        else if (attack_stage==stage2_wind_detected)
        {
            /*Move AHM to windy, confirm if in stage 2*/
            if (kymeraAncCommon_IsWindDetectInStage2())
            {
                kymeraAncCommon_WindConfirmed(wind_intensity);
            }
        }
    }
    else
    {
        /*Product supports only stage 1 wind*/
        if (attack_stage==stage1_wind_detected)
        {
            kymeraAncCommon_FreezeAdaptivity();
            kymeraAncCommon_WindConfirmed(wind_intensity);
        }
    }
}

void KymeraAncCommon_WindDetectRelease(windDetectStatus_t release_stage)
{
    DEBUG_LOG("KymeraAncCommon_WindDetectRelease release_stage enum:windDetectStatus_t:%d", release_stage);

    if (appConfigWindDetect2MicSupported())
    {
        if ((release_stage==stage2_wind_released) || (release_stage==stage1_wind_released))
        {
            /*Move to stage 1*/
            if (kymeraAncCommon_IsWindDetectInStage2())
            {
                kymeraAncCommon_MoveToStage1();
            }
        }
    }
    else
    {
        /*Product supports only stage 1 wind*/
        if (release_stage==stage1_wind_released)
        {
            /*Move AHM to Full*/
            KymeraAhm_SetTargetGain(kymeraAncCommon_GetTargetGain());
            kymeraAncCommon_RampGainsToTarget(kymeraAncCommon_StartOrFreezeAdaptivity);

            /*Reset wind detect flag*/
            kymeraAncCommon_ResetWindDetectInStage2();
        }
    }
}
#endif

#if defined(ENABLE_SELF_SPEECH) && defined (ENABLE_AUTO_AMBIENT)

#define SELF_SPEECH_RETRY_MS (2000)

static bool kymeraAncCommon_IsVoiceConcurrencyActive(void)
{
    bool status = FALSE;

    if (((Kymera_OutputGetConnectedUsers() & output_user_sco)==output_user_sco) || Kymera_IsVaActive()
            || (data.connecting_user==output_user_sco) || (data.connecting_user==output_user_le_srec)
            || (data.connecting_user==output_user_le_voice_mono) || (data.connecting_user==output_user_le_voice_stereo))
    {
        status = TRUE;
    }
    return status;
}

static void kymeraAncCommon_SelfSpeechDetect_MicReConnect(void)
{
    if (KymeraSelfSpeechDetect_IsActive() && !kymeraAncCommon_isTransitionInProgress())
    {
        KymeraBasicPassthrough_Create();
        KymeraBasicPassthrough_Configure(&data.mode_settings);
        if (!Kymera_MicConnect(mic_user_aanc))
        {
            DEBUG_LOG_ERROR("KymeraAncCommon_SelfSpeechDetect_MicReConnect: Mic connection was not successful..");
            Panic();
        }
        KymeraSelfSpeechDetect_Connect();
        kymeraAncCommon_BasicPassthroughConnect();
        KymeraSelfSpeechDetect_Start();
        KymeraBasicPassthrough_Start();
    }
}

static void kymeraAncCommon_SelfSpeechDetect_MicDisconnect(void)
{
    if (KymeraSelfSpeechDetect_IsActive() && !kymeraAncCommon_isTransitionInProgress())
        /*Since Mode change scenarios Disconnect and Connect mics, can optimise here for Mode change scenarios*/
    {
        KymeraBasicPassthrough_Stop();
        KymeraSelfSpeechDetect_Stop();
        kymeraAncCommon_BasicPassthroughDisconnect();
        KymeraSelfSpeechDetect_Disconnect();
        Kymera_MicDisconnect(mic_user_aanc);
        KymeraBasicPassthrough_Destroy();
    }
}

static void kymeraAncCommon_AncMicReConnect(void)
{
    if (KymeraAncCommon_AdaptiveAncIsEnabled() && !kymeraAncCommon_isTransitionInProgress())
    {
        if (!Kymera_MicConnect(mic_user_aanc))
        {
            DEBUG_LOG_ERROR("kymeraAncCommon_AncMicReConnect: Mic connection was not successful..");
            Panic();
        }

        kymeraAncCommon_Connect();
        kymeraAncCommon_Start();
    }
}

void KymeraAncCommon_SelfSpeechDetectEnable(void)
{
    DEBUG_LOG("KymeraAncCommon_SelfSpeechDetectEnable");

    if (!KymeraSelfSpeechDetect_IsActive())
    {
        /*Message Cancel earlier Enable/Disable messages*/
        MessageCancelAll(AncStateManager_GetTask(), KYMERA_INTERNAL_SELF_SPEECH_ENABLE_TIMEOUT);
        MessageCancelAll(AncStateManager_GetTask(), KYMERA_INTERNAL_SELF_SPEECH_DISABLE_TIMEOUT);

        if (kymeraAncCommon_isTransitionInProgress() ||
            kymeraAncCommon_IsVoiceConcurrencyActive())
        {
            MessageSendLater(KymeraGetTask(), KYMERA_INTERNAL_SELF_SPEECH_ENABLE_TIMEOUT,
                         NULL, SELF_SPEECH_RETRY_MS);
            return;
        }

        /*If ANC is active, Disconnect Mic*/
        kymeraAncCommon_AncMicDisconnect();

        KymeraSelfSpeechDetect_Create();
        KymeraSelfSpeechDetect_Configure();
        KymeraSelfSpeechDetect_SetSysMode(atr_vad_sysmode_1mic_detection);

        if(Kymera_OutputIsChainInUse())
        {
            kymeraAncCommon_UpdateConcurrencyRequest(TRUE);
        }

        appKymeraConfigureDspPowerMode();

        /*If ANC is active, Connect and Start ANC operators*/
        if (KymeraAncCommon_AdaptiveAncIsEnabled())
        {
            /*If ANC is active, Connect both ANC and Self speech Mics*/
            Kymera_MicConnect(mic_user_aanc);
            kymeraAncCommon_Connect();
            kymeraAncCommon_Start();
        }
        else
        {
            kymeraAncCommon_SelfSpeechDetect_MicReConnect();
        }
    }
    else
    {
        DEBUG_LOG("Self Speech graph Already active");
    }
}

void KymeraAncCommon_SelfSpeechDetectDisable(void)
{
    DEBUG_LOG("KymeraSelfSpeechDetect_Disable");

    if (KymeraSelfSpeechDetect_IsActive())
    {
        /*Message Cancel earlier Enable/Disable messages*/
        MessageCancelAll(AncStateManager_GetTask(), KYMERA_INTERNAL_SELF_SPEECH_ENABLE_TIMEOUT);
        MessageCancelAll(AncStateManager_GetTask(), KYMERA_INTERNAL_SELF_SPEECH_DISABLE_TIMEOUT);

        if (kymeraAncCommon_isTransitionInProgress() ||
            kymeraAncCommon_IsVoiceConcurrencyActive())
        {
            MessageSendLater(KymeraGetTask(), KYMERA_INTERNAL_SELF_SPEECH_DISABLE_TIMEOUT,
                         NULL, SELF_SPEECH_RETRY_MS);
            return;
        }

        if (KymeraAncCommon_AdaptiveAncIsEnabled())
        {
            /*Stop ANC operators if ANC is ON*/
            kymeraAncCommon_AncStopOperators();
            Kymera_MicDisconnect(mic_user_aanc);
        }
        else
        {
            /*Stop and Disconnect Self Speech*/
            kymeraAncCommon_SelfSpeechDetect_MicDisconnect();
        }

        /*Destroy Self Speech*/
        KymeraSelfSpeechDetect_Destroy();

        /*If ANC is active, ReConnect Mic*/
        kymeraAncCommon_AncMicReConnect();
    }
    else
    {
        DEBUG_LOG("No active Self Speech graph");
    }
}

#endif

void KymeraAncCommon_TransitionCompleteAction(void)
{
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
    DEBUG_LOG("KymeraAncCommon_TransitionCompleteAction!");

    /* Cancel any pending AHM Transition timeout messages */
    MessageCancelAll(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_AHM_TRANSITION_TIMEOUT);

#if defined(ENABLE_WIND_DETECT)
    if (!kymeraAncCommon_IsWindConfirmed())
#endif
    {
        if(AncConfig_IsAncModeStaticLeakThrough(AncStateManager_GetCurrentMode()))
        {
            KymeraAhm_SetTargetGain(kymeraAncCommon_GetTargetGain());
        }
        KymeraAhm_SetSysMode(ahm_sysmode_full);
    }
#if defined(ENABLE_WIND_DETECT)
    else
    {
        KymeraAhm_SetSysMode(ahm_sysmode_wind);
    }
#endif /* ENABLE_WIND_DETECT */

    if (kymeraAncCommon_IsModeChangeInProgress())
    {
        /* Allow AHM to finish full processing ramps in 100msec time and inform ANC SM to clear mode change lock */
        MessageSendLater(AncStateManager_GetTask(), KYMERA_ANC_COMMON_CAPABILITY_MODE_CHANGE_TRIGGER, NULL, AHM_FULL_PROC_RAMP_TIME_MS);
    }


    /* When Fast Mode switch is enabled, Mode change gets completed on receiving Transition complete message. */
    kymeraAncCommon_SetModeChangeInProgress(FALSE);

#endif /* ENABLE_ANC_FAST_MODE_SWITCH */
}

void KymeraAncCommon_SetFilterCoefficients(void)
{
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
    DEBUG_LOG("KymeraAncCommon_SetFilterCoefficients!");

    KymeraAncCommon_SetFilterCoefficientsForInstance(AUDIO_ANC_INSTANCE_0);
    if (appKymeraIsParallelAncFilterEnabled())
    {
        KymeraAncCommon_SetFilterCoefficientsForInstance(AUDIO_ANC_INSTANCE_1);
    }
#endif /* ENABLE_ANC_FAST_MODE_SWITCH */
}

#ifndef ENABLE_UNIFIED_ANC_GRAPH
static void KymeraAncCommon_StandaloneToConcurrencyReq(void)
{
    DEBUG_LOG("KymeraAncCommon_StandaloneToConcurrencyReq");
    AncStateManager_StandaloneToConcurrencyReq();
}

static void KymeraAncCommon_ConcurrencyToStandaloneReq(void)
{
    DEBUG_LOG("KymeraAncCommon_ConcurrencyToStandaloneReq");
    AncStateManager_ConcurrencyToStandaloneReq();
}
#endif

void KymeraAncCommon_HandleConcurrencyUpdate(bool is_concurrency_active)
{
    DEBUG_LOG_INFO("KymeraAncCommon_HandleConcurrencyUpdate");
    if(is_concurrency_active)
    {
        if((!kymeraAncCommon_IsConcurrencyChainActive())
           && (kymeraAncCommon_IsConcurrencySwitchRequired(kymeraAncCommon_GetAancCurrentConfig()))
           && (Kymera_OutputIsChainInUse()))
        {
            kymeraAncCommon_UpdateConcurrencyRequest(TRUE);
            kymeraAncCommon_StandaloneToConcurrencyChain();
        }
    }
    else
    {
        if(kymeraAncCommon_IsConcurrencyChainActive())
        {
            kymeraAncCommon_UpdateConcurrencyRequest(FALSE);
            kymeraAncCommon_ConcurrencyToStandaloneChain();
        }
    }
}

uint8 KymeraAncCommon_GetAhmAdjustedFfFineGain(void)
{
    return KymeraAhm_GetAdjustedFfFineGain();
}

bool KymeraAncCommon_IsSelfSpeechDetectActive(void)
{
    return KymeraSelfSpeechDetect_IsActive();
}

bool KymeraAncCommon_IsAahFeatureSupported(void)
{
    return KymeraAah_IsFeatureSupported();
}

bool KymeraAncCommon_GetAahCurrentState(void)
{
    return KymeraAah_GetCurrentState();
}

void KymeraAncCommon_StartEftDelayed(void)
{
    MessageCancelAll(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_START_EFT_DELAYED);
    MessageSendLater(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_START_EFT_DELAYED, NULL, FITTEST_START_DELAYED_MS);
}

void KymeraAncCommon_StopContinuousEft(void)
{
    if (!MessageCancelAll(kymeraAncCommon_GetTask(), KYMERA_ANC_COMMON_INTERNAL_START_EFT_DELAYED)) /* To ensure message is already delivered and the graph is active */
    {
        KymeraFitTest_ContinuousDisableEftMicClient();
    }
}

#ifdef ENABLE_WIND_DETECT

void KymeraAncCommon_SetWindyModeRampDurationParameters(uint32 ff_ramp_duration, uint32 fb_ramp_duration)
{
    KymeraAhm_SetWindyModeRampDurationParameters(ff_ramp_duration, fb_ramp_duration);
}

void KymeraAncCommon_SetWindyModeGainParameters(uint32 ff_fine_gain, uint32 fb_fine_gain)
{
    KymeraAhm_SetWindyModeGainParameters(ff_fine_gain, fb_fine_gain);
}

#endif /* ENABLE_WIND_DETECT */

#endif /* ENABLE_ADAPTIVE_ANC */

