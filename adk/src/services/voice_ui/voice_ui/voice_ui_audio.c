/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    voice_ui
    \brief      Implementation of the voice UI audio related interface.
*/

#ifdef INCLUDE_VOICE_UI
#include "voice_ui_audio.h"
#include "voice_ui.h"
#include "voice_ui_va_client_if.h"
#include "voice_audio_manager.h"
#include "voice_ui_container.h"
#include "voice_ui_session.h"
#include "voice_ui_async_req.h"
#include <hfp_profile.h>
#include <mirror_profile_protected.h>
#include <bandwidth_manager.h>
#include <logging.h>
#include <panic.h>
#include <message.h>

#define MAXIMUM_CAPTURE_PERIOD_MS  D_MIN(1)
#define MIC_DATA_TIMEOUT_PERIOD_MS D_SEC(2)

typedef enum
{
    CAPTURE_ENDPOINT_TIMEOUT,
    CAPTURE_MIC_DATA_TIMEOUT
} internal_message_ids_t;

static struct
{
    unsigned detection_suspended:1;
    unsigned capture_active:1;
    unsigned detection_active:1;
    unsigned detection_start_pending:1;
} state = {0};

static void voiceUi_AudioMsgHandler(Task task, MessageId id, Message message);
static const TaskData msg_handler = { voiceUi_AudioMsgHandler };

static va_audio_wuw_detection_params_t detection_config;

static void voiceUi_CaptureStarted(void)
{
    MessageSendLater((Task)&msg_handler, CAPTURE_ENDPOINT_TIMEOUT, NULL, MAXIMUM_CAPTURE_PERIOD_MS);
    MessageSendLater((Task)&msg_handler, CAPTURE_MIC_DATA_TIMEOUT, NULL, MIC_DATA_TIMEOUT_PERIOD_MS);
    BandwidthManager_FeatureStart(BANDWIDTH_MGR_FEATURE_VA);
    state.capture_active = TRUE;
}

static void voiceUi_AdjustBtBandwidthUsage(bool throttle_required)
{
    voice_ui_handle_t *va_handle = VoiceUi_GetActiveVa();
    DEBUG_LOG_DEBUG("voiceUi_AdjustBtBandwidthUsage: throttle_required[%d]", throttle_required);
    if (va_handle->voice_assistant->AdjustBtBandwidthUsage)
        va_handle->voice_assistant->AdjustBtBandwidthUsage(throttle_required);
}

static bool voiceUi_CaptureDataReceived(Source source)
{
    voice_ui_handle_t *va_handle = VoiceUi_GetActiveVa();
    PanicFalse(va_handle->voice_assistant->audio_if.CaptureDataReceived != NULL);
    bool data_processed = va_handle->voice_assistant->audio_if.CaptureDataReceived(source);

    if(data_processed)
    {
        MessageCancelAll((Task) &msg_handler, CAPTURE_MIC_DATA_TIMEOUT);
        MessageSendLater((Task) &msg_handler, CAPTURE_MIC_DATA_TIMEOUT, NULL, MIC_DATA_TIMEOUT_PERIOD_MS);
    }

    return data_processed;
}

static va_audio_wuw_detected_response_t voiceUi_WakeUpWordDetected(const va_audio_wuw_detection_info_t *wuw_info)
{
    va_audio_wuw_detected_response_t response = {0};
    voice_ui_handle_t *va_handle = VoiceUi_GetActiveVa();
    UNUSED(MirrorProfile_PeerLinkPolicyLowLatencyKick());
    PanicFalse(va_handle->voice_assistant->audio_if.WakeUpWordDetected != NULL);
    response.start_capture = va_handle->voice_assistant->audio_if.WakeUpWordDetected(&response.capture_params, wuw_info);
    response.capture_callback = voiceUi_CaptureDataReceived;

    if (response.start_capture)
    {
        voiceUi_CaptureStarted();
    }

    return response;
}

static bool voiceUi_StartCapture(const va_audio_voice_capture_params_t *capture_config)
{
    bool status = VoiceAudioManager_StartCapture(voiceUi_CaptureDataReceived, capture_config);

    if (status)
    {
        voiceUi_CaptureStarted();
    }

    return status;
}

static void voiceUi_StopCapture(void)
{
    if (VoiceAudioManager_StopCapture())
    {
        MessageCancelAll((Task)&msg_handler, CAPTURE_ENDPOINT_TIMEOUT);
        MessageCancelAll((Task)&msg_handler, CAPTURE_MIC_DATA_TIMEOUT);
        BandwidthManager_FeatureStop(BANDWIDTH_MGR_FEATURE_VA);
    }

    state.capture_active = FALSE;
}

static bool voiceUi_StartDetection(const va_audio_wuw_detection_params_t *wuw_config)
{
    voice_ui_handle_t *va_handle = NULL;

    bool status = VoiceAudioManager_StartDetection(voiceUi_WakeUpWordDetected, wuw_config);

    if (status)
    {
        state.detection_active = TRUE;

        va_handle = VoiceUi_GetActiveVa();

        if (va_handle && va_handle->voice_assistant->GetBtAddress && va_handle->voice_assistant->GetBtAddress())
        {
            VoiceUi_SendLinkPolicyUpdateReq(va_handle->voice_assistant->GetBtAddress());
        }  
    }

    DEBUG_LOG_DEBUG("voiceUi_StartDetection: %u", status);
    return status;
}

static void voiceUi_StopDetection(void)
{
    voice_ui_handle_t *va_handle = VoiceUi_GetActiveVa();

    VoiceAudioManager_StopDetection();

    state.detection_start_pending = FALSE;
    state.detection_active = FALSE;

    if(va_handle && va_handle->voice_assistant->GetBtAddress && va_handle->voice_assistant->GetBtAddress())
    {
        VoiceUi_SendLinkPolicyUpdateReq(va_handle->voice_assistant->GetBtAddress());
    }
}

static bool voiceUi_IsCaptureActive(void)
{
    return state.capture_active;
}

static bool voiceUi_IsDetectionActive(void)
{
    return state.detection_active;
}

static bool voiceUi_IsDetectionStartPending(void)
{
    return state.detection_start_pending;
}

static void voiceUi_SendSessionCancelledInd(void)
{
    voice_ui_handle_t *va_handle = VoiceUi_GetActiveVa();
    if (va_handle && va_handle->voice_assistant->SessionCancelled)
    {
        DEBUG_LOG("voiceUi_SendSessionCancelledInd");
        va_handle->voice_assistant->SessionCancelled(voiceUi_IsCaptureActive());
    }
}

static void voiceUi_CancelVaSession(void)
{
    if (voiceUi_IsCaptureActive() || VoiceUi_IsSessionInProgress())
    {
        voiceUi_SendSessionCancelledInd();
    }
    if (voiceUi_IsCaptureActive())
    {
        voiceUi_StopCapture();
    }
    VoiceUi_VaSessionReset();
}

static void voiceUi_UpdateFeatureStoppedState(void)
{
    if(!VoiceUi_IsVaActive())
    {
        FeatureManager_StopFeatureIndication(VoiceUi_GetFeatureManagerHandle());
    }
}

static void voiceUi_AudioMsgHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);
    
    if (id == CAPTURE_MIC_DATA_TIMEOUT || id == CAPTURE_ENDPOINT_TIMEOUT)
    {
        DEBUG_LOG_WARN("voiceUi_AudioMsgHandler enum:internal_message_ids_t:%d", id);
        voiceUi_CancelVaSession();
    }
}

void VoiceUi_SuspendAudio(void)
{
    DEBUG_LOG_FN_ENTRY("VoiceUi_SuspendAudio");

    voiceUi_CancelVaSession();

    if (voiceUi_IsDetectionActive())
    {
        voiceUi_StopDetection();
        state.detection_suspended = TRUE;
    }

    if(voiceUi_IsDetectionStartPending())
    {
        state.detection_start_pending = FALSE;
        state.detection_suspended = TRUE;
    }
}

void VoiceUi_ResumeAudio(void)
{
    DEBUG_LOG_FN_ENTRY("VoiceUi_ResumeAudio");
    PanicFalse(voiceUi_StartDetection(&detection_config));
    state.detection_suspended = FALSE;
}

void VoiceUi_UnrouteAudio(void)
{
    voiceUi_StopCapture();
    voiceUi_StopDetection();
    memset(&state, FALSE, sizeof(state));
    voiceUi_UpdateFeatureStoppedState();
}

bool VoiceUi_IsActiveAssistant(voice_ui_handle_t *va_handle)
{
    return (va_handle != NULL) && (va_handle == VoiceUi_GetActiveVa());
}

bool VoiceUi_IsAudioSuspended(voice_ui_handle_t* va_handle)
{
    if (VoiceUi_IsActiveAssistant(va_handle) == FALSE)
        return TRUE;

    return !FeatureManager_CanFeatureStart(VoiceUi_GetFeatureManagerHandle());
}

bool VoiceUi_IsDetectionSuspended(voice_ui_handle_t* va_handle)
{
    if ((VoiceUi_IsActiveAssistant(va_handle) == FALSE) && (va_handle != NULL))
        return TRUE;

    return state.detection_suspended;
}

voice_ui_audio_status_t VoiceUi_StartAudioCapture(voice_ui_handle_t *va_handle, const va_audio_voice_capture_params_t *audio_config)
{
    if (VoiceUi_IsActiveAssistant(va_handle) == FALSE)
        return voice_ui_audio_not_active;

    if (FeatureManager_StartFeatureRequest(VoiceUi_GetFeatureManagerHandle()))
    {
        if(voiceUi_StartCapture(audio_config))
        {
            return voice_ui_audio_success;
        }
        else
        {
            voiceUi_UpdateFeatureStoppedState();
            return voice_ui_audio_failed;
        }
    }
    else
    {
        return voice_ui_audio_suspended;
    }
}

void VoiceUi_StopAudioCapture(voice_ui_handle_t *va_handle)
{
    if (VoiceUi_IsActiveAssistant(va_handle))
    {
        voiceUi_StopCapture();
        if (voiceUi_IsDetectionStartPending())
        {
            VoiceUi_StartWakeUpWordDetection(va_handle, &detection_config);
            state.detection_start_pending = FALSE;
        }
    }

    voiceUi_UpdateFeatureStoppedState();
}

voice_ui_audio_status_t VoiceUi_StartWakeUpWordDetection(voice_ui_handle_t *va_handle, const va_audio_wuw_detection_params_t *audio_config)
{
    voice_ui_audio_status_t status = voice_ui_audio_success;

    if (VoiceUi_IsActiveAssistant(va_handle) == FALSE)
    {
        status = voice_ui_audio_not_active;
    }
    else if (voiceUi_IsDetectionActive())
    {
        if (VA_AUDIO_DETECTION_PARAMS_EQUAL(&detection_config, audio_config))
        {
            status = voice_ui_audio_already_started;
        }
        else
        {
            DEBUG_LOG_VERBOSE("VoiceUi_StartWakeUpWordDetection: stopping to apply new paramenters");
            voiceUi_StopDetection();
        }
    }

    if (status == voice_ui_audio_success)
    {
        if(FeatureManager_StartFeatureRequest(VoiceUi_GetFeatureManagerHandle()))
        {
            if (voiceUi_StartDetection(audio_config))
            {
                detection_config = *audio_config;
            }
            else if(voiceUi_IsCaptureActive())
            {
                detection_config = *audio_config;
                state.detection_start_pending = TRUE;
            }
            else
            {
                voiceUi_UpdateFeatureStoppedState();
                status = voice_ui_audio_failed;
            }
        }
        else
        {
            detection_config = *audio_config;
            state.detection_suspended = TRUE;
            status = voice_ui_audio_suspended;
        }
    }

    DEBUG_LOG_DEBUG("VoiceUi_StartWakeUpWordDetection: enum:voice_ui_audio_status_t:%d", status);

    return status;
}

void VoiceUi_StopWakeUpWordDetection(voice_ui_handle_t *va_handle)
{
    if (VoiceUi_IsActiveAssistant(va_handle))
        voiceUi_StopDetection();

    voiceUi_UpdateFeatureStoppedState();
    state.detection_suspended = FALSE;
}

bool VoiceUi_IsHfpIsActive(void)
{
    /* HFP could be active or, with iPhone, Siri might have SCO active to use the mic. */
    bool mic_in_use = HfpProfile_IsScoActive();
    DEBUG_LOG("VoiceUi_IsHfpIsActive sco_active %d", mic_in_use);

    if (!mic_in_use)
    {
        mic_in_use = VoiceSources_IsAnyVoiceSourceRouted();
        DEBUG_LOG("VoiceUi_IsHfpIsActive voice_routed %d", mic_in_use);
    }

    return mic_in_use;
}

bool VoiceUi_IsVaActive(void)
{
    return voiceUi_IsDetectionActive() || voiceUi_IsCaptureActive();
}

void VoiceUi_AudioInit(void)
{
    PanicFalse(BandwidthManager_RegisterFeature(BANDWIDTH_MGR_FEATURE_VA, medium_bandwidth_manager_priority, voiceUi_AdjustBtBandwidthUsage));
}

#ifdef HOSTED_TEST_ENVIRONMENT
bool VoiceUi_CaptureDataReceived(Source source)
{
    return voiceUi_CaptureDataReceived(source);
}
void VoiceUi_TestResetAudio(void)
{
    memset(&state, FALSE, sizeof(state));
    memset(&detection_config, 0, sizeof(detection_config));
}
va_audio_wuw_detected_response_t VoiceUi_WakeUpWordDetected(const va_audio_wuw_detection_info_t *wuw_info)
{
    return voiceUi_WakeUpWordDetected(wuw_info);
}
void VoiceUi_AdjustBtBandwidthUsage(uint8 level)
{
    voiceUi_AdjustBtBandwidthUsage(level);
}
#endif

#endif /* INCLUDE_VOICE_UI */
