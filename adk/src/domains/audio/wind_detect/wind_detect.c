/*!
\copyright  Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       wind_detect.c
\brief      Wind noise detection and anc gain reduction Implementation.
*/
#include "wind_detect.h"
#include "ui.h"
#include "kymera_anc_common.h"
#include "anc_state_manager.h"
#include "peer_signalling.h"
#include "anc_trigger_manager.h"

#ifdef ENABLE_WIND_DETECT
typedef struct
{
    TaskData task;
    windDetectState_t current_state;
    windDetectStatus_t local_attack_status;
    windDetectStatus_t remote_attack_status;
    wind_detect_intensity_t local_intensity;
}windDetectTaskData;

static windDetectTaskData wind_detect_task_data;

#define windDetectGetTaskData() (&wind_detect_task_data)
#define windDetectGetTask()     (&wind_detect_task_data.Task)
#define MIC_CONFIGURATION_OK    TRUE;
#define MIC_CONFIGURATION_FAIL  FALSE;

/***************************************
 ********* Static Functions ************
 ***************************************/
static bool windDetect_isStageOneWindDetected(void);
static bool windDetect_isWindReleased(void);
static windDetectState_t windDetect_GetCurrentState(void);
static void windDetect_SetState(windDetectState_t state);
static ui_input_t windDetect_GetUiEventFromWindStatus(windDetectStatus_t status);
static bool windDetect_CheckMicConfig(void);
static void windDetect_msgHandler(Task task, MessageId id, Message message);

static bool windDetect_CheckMicConfig(void)
{
    bool mic_config_status =FALSE;
    if(appConfigWindDetect2MicSupported() && appConfigWindDetectANCFFMic()!= MICROPHONE_NONE && appConfigWindDetectDiversityMic() != MICROPHONE_NONE)
    {
        if(appConfigWindDetectANCFFMic() == appConfigWindDetectDiversityMic())
        {
            DEBUG_LOG_ALWAYS("WIND DETECT:- INCORRECT MIC CONFIGURATION");
            Panic();
            mic_config_status=MIC_CONFIGURATION_FAIL;
        }
        else
        {
            mic_config_status=MIC_CONFIGURATION_OK;
        }
    }
    else if(!appConfigWindDetect2MicSupported() && appConfigWindDetectANCFFMic()!=MICROPHONE_NONE)
    {
        mic_config_status=MIC_CONFIGURATION_OK;
    }
    else
    {
        DEBUG_LOG_ALWAYS("WIND DETECT:- INCORRECT MIC CONFIGURATION");
        Panic();
        mic_config_status=MIC_CONFIGURATION_FAIL;
    }
    return mic_config_status;
}

static bool windDetect_isStageOneWindDetected(void)
{
    if(WindDetect_GetLocalAttackStatus() == stage1_wind_detected ||
       WindDetect_GetRemoteAttackStatus() == stage1_wind_detected )
    {
        return TRUE;
    }
    return FALSE;
}

static bool windDetect_isStageTwoWindDetected(void)
{
    if(WindDetect_GetLocalAttackStatus() == stage2_wind_detected ||
        WindDetect_GetRemoteAttackStatus() == stage2_wind_detected )
    {
        return TRUE;
    }
    return FALSE;
}

static bool windDetect_isWindReleased(void)
{
    if(appPeerSigIsConnected() &&
      (WindDetect_GetLocalAttackStatus() == stage1_wind_released   ||
       WindDetect_GetLocalAttackStatus() == stage2_wind_released)  &&
       (WindDetect_GetRemoteAttackStatus() == stage1_wind_released ||
        WindDetect_GetRemoteAttackStatus() == stage2_wind_released))
    {
        return TRUE;
    }
    else if(!appPeerSigIsConnected() &&
           (WindDetect_GetLocalAttackStatus() == stage1_wind_released ||
            WindDetect_GetLocalAttackStatus() == stage2_wind_released ||
            WindDetect_GetRemoteAttackStatus() == stage2_wind_released))
    {
        return TRUE;
    }
    return FALSE;
}

static windDetectState_t windDetect_GetCurrentState(void)
{
    return windDetectGetTaskData()->current_state;
}

static void windDetect_SetState(windDetectState_t state)
{
    DEBUG_LOG_ALWAYS("windDetect_SetState %d",state);
    windDetectGetTaskData()->current_state=state;
}

static ui_input_t windDetect_GetUiEventFromWindStatus(windDetectStatus_t status)
{
    ui_input_t ui_input=ui_input_anc_wind_released;

    switch(status)
    {
        case stage1_wind_released:
        case stage2_wind_released:
            ui_input= ui_input_anc_wind_released;
        break;
        case stage1_wind_detected:
        case stage2_wind_detected:
            ui_input = ui_input_anc_wind_detected;
        break;
    }

    return ui_input;
}

static void WindDetect_SetLocalIntensity(wind_detect_intensity_t intensity)
{
    windDetectGetTaskData()->local_intensity = intensity;
}

static wind_detect_intensity_t WindDetect_GetLocalIntensity(void)
{
    return windDetectGetTaskData()->local_intensity;
}

static void windDetect_HandleOperatorMessage(windDetectStatus_t status, wind_detect_intensity_t intensity)
{
    DEBUG_LOG_ALWAYS("windDetect_HandleOperatorMessage, status: enum:windDetectStatus_t:%d, intensity: enum:wind_detect_intensity_t:%d ", status, intensity);
    WindDetect_SetLocalAttackStatus(status);
    WindDetect_SetLocalIntensity(intensity);
    Ui_InjectRedirectableUiInput(windDetect_GetUiEventFromWindStatus(status),FALSE);
}

static void windDetect_msgHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    wind_detect_intensity_t wind_intensity = wind_detect_intensity_none;
    kymera_aanc_event_msg_t* msg = (kymera_aanc_event_msg_t*)message;
    windDetectStatus_t wind_status = stage1_wind_detected;
    bool update = TRUE;

    switch(id)
    {
        case KYMERA_WIND_STAGE1_DETECTED:
            wind_status = stage1_wind_detected;
            wind_intensity = (wind_detect_intensity_t)(msg->info);
        break;

        case KYMERA_WIND_STAGE2_DETECTED:
            wind_status = stage2_wind_detected;
            wind_intensity = (wind_detect_intensity_t)(msg->info);
        break;

        case KYMERA_WIND_STAGE1_RELEASED:
            wind_status = stage1_wind_released;
        break;

        case KYMERA_WIND_STAGE2_RELEASED:
            wind_status = stage2_wind_released;
        break;

        default:
            update = FALSE;
        break;
    }

    if (update)
    {
        windDetect_HandleOperatorMessage(wind_status, wind_intensity);
    }
}

windDetectStatus_t WindDetect_GetLocalAttackStatus(void)
{
    return windDetectGetTaskData()->local_attack_status;
}

void WindDetect_SetLocalAttackStatus(windDetectStatus_t status)
{
    windDetectGetTaskData()->local_attack_status=status;
}

windDetectStatus_t WindDetect_GetRemoteAttackStatus(void)
{
    return windDetectGetTaskData()->remote_attack_status;
}

void WindDetect_SetRemoteAttackStatus(windDetectStatus_t status)
{
    windDetectGetTaskData()->remote_attack_status=status;
}

void WindDetect_HandleWindDetect(void)
{
    wind_detect_intensity_t wind_intensity = WindDetect_GetLocalIntensity();

    /* If the wind is not detected on one of the buds, consider low intensity on that bud */
    wind_intensity = ((wind_intensity == wind_detect_intensity_none) ? wind_detect_intensity_low : wind_intensity);

    /*2 MIC wind detect is supported*/
    if(appConfigWindDetect2MicSupported())
    {
        if(windDetect_isStageOneWindDetected() &&
           windDetect_GetCurrentState() == wind_detect_state_idle)
        {
            KymeraAncCommon_WindDetectAttack(stage1_wind_detected, wind_intensity);
            windDetect_SetState(wind_detect_state_analysing);
        }
        else if(windDetect_isStageTwoWindDetected())
        {
            if (windDetect_GetCurrentState() == wind_detect_state_analysing || windDetect_GetCurrentState() == wind_detect_state_windy)
            {
                KymeraAncCommon_WindDetectAttack(stage2_wind_detected, wind_intensity);
                windDetect_SetState(wind_detect_state_windy);
            }
        }
    }
    else
    {
        /*2 MIC wind detect is not supported*/
        if(windDetect_isStageOneWindDetected() &&
           windDetect_GetCurrentState() == wind_detect_state_idle)
        {
            KymeraAncCommon_WindDetectAttack(stage1_wind_detected, wind_intensity);
            windDetect_SetState(wind_detect_state_windy);
        }
    }
    
    if (windDetect_GetCurrentState()==wind_detect_state_windy)
    {
        AncStateManager_WindReductionUpdateInd(TRUE);
    }
}

void WindDetect_HandleWindRelease(void)
{
    /*2 MIC wind detect is supported*/
    if(appConfigWindDetect2MicSupported())
    {
        if(windDetect_GetCurrentState()== wind_detect_state_analysing &&
           windDetect_isWindReleased())
        {
            KymeraAncCommon_WindDetectRelease(stage1_wind_released);
            windDetect_SetState(wind_detect_state_idle);
            WindDetect_SetLocalIntensity(wind_detect_intensity_none);
            AncTriggerManager_ActionPostRelease();
        }
        else if(windDetect_GetCurrentState()== wind_detect_state_windy &&
                windDetect_isWindReleased())
        {
            KymeraAncCommon_WindDetectRelease(stage2_wind_released);
            windDetect_SetState(wind_detect_state_idle);
            WindDetect_SetLocalIntensity(wind_detect_intensity_none);
            AncTriggerManager_ActionPostRelease();
        }
    }
    else
    {
        /*2 MIC wind detect is not supported*/
        if(windDetect_GetCurrentState()== wind_detect_state_windy &&
           windDetect_isWindReleased())
        {
            KymeraAncCommon_WindDetectRelease(stage1_wind_released);
            windDetect_SetState(wind_detect_state_idle);
            WindDetect_SetLocalIntensity(wind_detect_intensity_none);
            AncTriggerManager_ActionPostRelease();
        }
    }

    if (windDetect_GetCurrentState()==wind_detect_state_idle)
    {
        AncStateManager_WindReductionUpdateInd(FALSE);
    }
}

static void WindDetect_HandleWindReleaseOnDisable(void)
{
    if(appConfigWindDetect2MicSupported())
    {
        if(windDetect_GetCurrentState()== wind_detect_state_analysing)          
        {
            KymeraAncCommon_WindDetectRelease(stage1_wind_released);
            windDetect_SetState(wind_detect_state_idle);
        }
        else if(windDetect_GetCurrentState()== wind_detect_state_windy)
        {
            KymeraAncCommon_WindDetectRelease(stage2_wind_released);
            windDetect_SetState(wind_detect_state_idle);
        }
    }
    else
    {
        if(windDetect_GetCurrentState()== wind_detect_state_windy)
        {
            KymeraAncCommon_WindDetectRelease(stage1_wind_released);
            windDetect_SetState(wind_detect_state_idle);
        }
    }
    
    if (windDetect_GetCurrentState()==wind_detect_state_idle)
    {
        AncStateManager_WindReductionUpdateInd(FALSE);
    }
}



bool WindDetect_Init(Task init_task)
{
    UNUSED(init_task);
    windDetectTaskData *windDetectTask = windDetectGetTaskData();
    windDetectGetTaskData()->task.handler=windDetect_msgHandler;
    Kymera_ClientRegister(&windDetectTask->task);

    WindDetect_Reset();
    windDetect_CheckMicConfig();
    return TRUE;
}

void WindDetect_Reset(void)
{
    windDetect_SetState(wind_detect_state_idle);
    WindDetect_SetLocalAttackStatus(stage1_wind_released);
    WindDetect_SetRemoteAttackStatus(stage1_wind_released);
    WindDetect_SetLocalIntensity(wind_detect_intensity_none);
}

bool WindDetect_IsWindNoiseReductionEnabled(void)
{
    bool status = TRUE;
    if (windDetect_GetCurrentState()==wind_detect_state_disable)
    {
        status=FALSE;
    }
    
    DEBUG_LOG_ALWAYS("WindDetect_IsWindNoiseReductionEnabled status %d", status);
    return status;
}

void WindDetect_Enable(bool enable)
{
    if (enable)
    {
        windDetect_SetState(wind_detect_state_idle);
        KymeraAncCommon_EnableWindDetect();
    }
    else
    {
        WindDetect_HandleWindReleaseOnDisable();
        WindDetect_Reset();
        windDetect_SetState(wind_detect_state_disable);
        KymeraAncCommon_DisableWindDetect();
    }
    
     /* Send notification to GAIA */
    AncStateManager_WindDetectionStateUpdateInd(enable);
}

bool WindDetect_IsEnvWindy(void)
{
    DEBUG_LOG_ALWAYS("WindDetect_IsEnvWindy Wind Current state %d", windDetect_GetCurrentState());
    return (windDetect_GetCurrentState()==wind_detect_state_windy);
}


#endif /*ENABLE_WIND_DETECT*/
