/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_noise_id.c
\brief     ANC Noise Id: Seamless filter switch to the reported noise category
*/


#ifdef ENABLE_ADAPTIVE_ANC

#include "anc_noise_id.h"
#include "kymera_anc_common.h"
#include "anc_state_manager.h"
#include "phy_state.h"
#include "anc_tws.h"
#include "anc_trigger_manager.h"

typedef struct
{
    bool enabled;
    anc_mode_t new_mode;
    bool mode_change_requested;
}ancNoiseIdData;

static ancNoiseIdData anc_noiseId_data;

#define ancNoiseId() (&anc_noiseId_data)

bool AncNoiseId_CanProcess(void)
{    
    if (AncNoiseId_IsFeatureSupported() && 
        (appPhyStateGetState()==PHY_STATE_IN_EAR) && 
        AncTws_IsPrimary())
    {
        return (TRUE);
    }
    else
    {
        return FALSE;
    }
}

bool AncNoiseId_IsFeatureSupported(void)
{
    return appConfigAncNoiseIdFeatureSupported();
}

bool AncNoiseId_IsFeatureEnabled(void)
{
    return (AncNoiseId_IsFeatureSupported() && ancNoiseId()->enabled);
}

void AncNoiseId_Init(void)
{
    /*Check for valid configurations:
      Atleast two modes should be configured for noise Id for same mode type*/
    if (AncNoiseId_IsFeatureSupported() && !AncConfig_ValidNoiseIdConfiguration())
    {
        DEBUG_LOG("AncNoiseId_Init: Invalid configuration for Noise ID");
        Panic();
    }

    /*Init globals*/
    ancNoiseId()->mode_change_requested = FALSE;
    ancNoiseId()->new_mode = ANC_INVALID_MODE;
}

void AncNoiseId_Enable(bool enable)
{
    if (enable)
    {
        if (AncNoiseId_CanProcess())
        {
            DEBUG_LOG("AncNoiseId_Enable: 1");
            KymeraAncCommon_NoiseIDEnable();
        }
    }
    else
    {        
        DEBUG_LOG("AncNoiseId_Enable: 0");
        KymeraAncCommon_NoiseIDDisable();
    }
    
    /* Send notification to GAIA */
    AncStateManager_NoiseIdStateInd(enable);
}

bool AncNoiseId_IsValidCategory(anc_noise_id_category_t requested_noise_category)
{
    if ((requested_noise_category==ANC_NOISE_ID_CATEGORY_A) ||
        (requested_noise_category==ANC_NOISE_ID_CATEGORY_B))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void AncNoiseId_HandleCategoryChange(anc_noise_id_category_t requested_noise_category)
{
    if (AncNoiseId_IsValidCategory(requested_noise_category))
    {
        anc_mode_t active_anc_mode = AncStateManager_GetCurrentMode();
        bool is_current_mode_adaptive = AncConfig_IsAncModeAdaptive(active_anc_mode);

        /*Check if the new category is defined and supported for the mode type*/
        uint8 new_anc_mode = AncConfig_GetAncModeForNoiseCategoryType(is_current_mode_adaptive, requested_noise_category);
            DEBUG_LOG("AncNoiseId_HandleCategoryChange new mode %d, current_anc_mode enum:anc_mode_t:%d ", new_anc_mode, active_anc_mode);

        if ((new_anc_mode!=active_anc_mode) && (new_anc_mode!=ANC_INVALID_MODE))
        {
            ancNoiseId()->new_mode = new_anc_mode;
            ancNoiseId()->mode_change_requested = TRUE;
            AncTriggerManager_Invoke(ANC_TRIGGER_TYPE_NOISE_ID_CAT_CHANGE);
        }
        else
        {
            ancNoiseId()->mode_change_requested = FALSE;
        }
    }
    else
    {
        DEBUG_LOG("AncNoiseId_HandleCategoryChange Invalid category");
    }

    /* Send Notification to GAIA */
    AncStateManager_NoiseIdCategoryInd(requested_noise_category);
}

void AncNoiseId_TriggerModeChange(void)
{
    ancNoiseId()->mode_change_requested = FALSE;

    anc_mode_t new_mode = ancNoiseId()->new_mode;
    AncStateManager_NoiseIdModeChange(new_mode);
}

bool AncNoiseId_IsModeChangeRequested(void)
{
    DEBUG_LOG("AncNoiseId_IsModeChangeRequested  %d ", ancNoiseId()->mode_change_requested);
    return (ancNoiseId()->mode_change_requested);
}

void AncNoiseId_UpdateTaskData(bool noise_id_enabled)
{
    /*By default feature is not enabled, enable using GAIA or Pydbg commands, info is persisted*/
    ancNoiseId()->enabled = noise_id_enabled;
}


void AncNoiseId_UpdateSessionData(bool *noise_id_enabled)
{
    *noise_id_enabled = ancNoiseId()->enabled;    
    DEBUG_LOG("AncNoiseId_UpdateSessionData Enabled: %d", ancNoiseId()->enabled);
}


#endif /*ENABLE_ADAPTIVE_ANC*/

