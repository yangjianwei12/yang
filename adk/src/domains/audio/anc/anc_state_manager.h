/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_state_manager.h
\defgroup   anc_state_manager ANC
\ingroup    audio_domain
\brief      State manager for Active Noise Cancellation (ANC).

Responsibilities:
  Handles state transitions between init, enable and disable states.
  The ANC audio domain is used by \ref audio_curation.
*/

#ifndef ANC_STATE_MANAGER_H_
#define ANC_STATE_MANAGER_H_

#include <anc.h>
#include <operators.h>
#include <rtime.h>
#include <marshal_common.h>
#include "domain_message.h"
#include "anc_auto_ambient.h"
#include "anc_noise_id.h"

/* Compiler error checks for macros that are supported under ANC V2 only */
#ifndef INCLUDE_ANC_V2
    #if defined(ENABLE_ANC) || defined(ENABLE_ADAPTIVE_ANC) || defined(ENABLE_ENHANCED_ANC)
        #error ENABLE_ANC/ ENABLE_ADAPTIVE_ANC/ ENABLE_ENHANCED_ANC can be used \
               along with INCLUDE_ANC_V2 only.
    #endif
#endif

#define MAXIMUM_AUDIO_ANC_INSTANCES (2)

#define GET_GAIN_INDEX_FROM_ANC_INSTANCE(anc_instance) ((anc_instance) - AUDIO_ANC_INSTANCE_0)

/*! @{ */

/*! \brief ANC state manager defines the various states handled in ANC. */
typedef enum
{
    anc_state_manager_uninitialised,
    anc_state_manager_power_off,
    anc_state_manager_enabled,
    anc_state_manager_disabled,
    anc_state_manager_tuning_mode_active,
    anc_state_manager_adaptive_anc_tuning_mode_active
} anc_state_manager_t;

typedef struct
{
    uint8 new_mode;
}ANC_STATE_MANAGER_INTERNAL_EVENT_MODE_CHANGE_T;

typedef struct
{
    uint8 mode;
}ANC_UPDATE_MODE_CHANGED_IND_T;

typedef struct
{
    anc_filter_topology_t filter_topology;
}ANC_SET_FILTER_TOPOLOGY_T;

typedef struct
{
    uint16 previous_config;
} ANC_UPDATE_PREV_CONFIG_IND_T;

typedef struct
{
    uint16 previous_mode;
} ANC_UPDATE_PREV_MODE_IND_T;

typedef struct
{
    uint8 anc_gain;
} ANC_UPDATE_GAIN_IND_T;

typedef struct
{
    uint8 aanc_ff_gain; /* FF gain */
} AANC_FF_GAIN_UPDATE_IND_T; /* Used to Update ANC Clients when local device AANC FF Gain is read from capablity*/

/*Currently FF Gain is the only logging information. If any data is added, this double typecasting can be removed */
typedef AANC_FF_GAIN_UPDATE_IND_T AANC_LOGGING_T;

typedef struct
{
    uint8 left_ff_gain;
    uint8 right_ff_gain;
} ANC_FF_GAIN_NOTIFY_T; /* Used to notify ANC Clients with both(local & remote device) FF Gains*/

typedef struct
{
    uint8 aanc_fb_gain; /* FB gain */
} AANC_FB_GAIN_UPDATE_IND_T; /* Used to Update ANC Clients when local device AANC FB Gain is read from capablity*/

/*Currently FB Gain is the only logging information. If any data is added, this double typecasting can be removed */
typedef AANC_FB_GAIN_UPDATE_IND_T AANC_FB_GAIN_LOGGING_T;

typedef struct
{
    uint8 left_fb_gain;
    uint8 right_fb_gain;
} ANC_FB_GAIN_NOTIFY_T; /* Used to notify ANC Clients with both(local & remote device) FB Gains*/

/* Supported ANC toggle configurations */
typedef enum
{
    anc_toggle_config_off = 0,
    anc_toggle_config_mode_1,
    anc_toggle_config_mode_2,
    anc_toggle_config_mode_3,
    anc_toggle_config_mode_4,
    anc_toggle_config_mode_5,
    anc_toggle_config_mode_6,
    anc_toggle_config_mode_7,
    anc_toggle_config_mode_8,
    anc_toggle_config_mode_9,
    anc_toggle_config_mode_10,
    anc_toggle_config_is_same_as_current = 0xFF,
    anc_toggle_config_not_configured = 0xFF
} anc_toggle_config_t;

/*! \brief ANC toggle configuration msg ids. */
typedef enum {
    anc_toggle_way_config_id_1 = 1,
    anc_toggle_way_config_id_2,
    anc_toggle_way_config_id_3,
} anc_toggle_way_config_id_t;

/*! \brief ANC scenario configuration msg ids. */
typedef enum {
    anc_scenario_config_id_standalone = 1,
    anc_scenario_config_id_playback,
    anc_scenario_config_id_sco,
    anc_scenario_config_id_va,
#ifdef INCLUDE_LE_STEREO_RECORDING    
    anc_scenario_config_id_stereo_recording_le
#endif /* INCLUDE_LE_STEREO_RECORDING */
} anc_scenario_config_id_t;

typedef struct
{
    anc_toggle_way_config_id_t anc_toggle_config_id;
    anc_toggle_config_t anc_config;
} ANC_TOGGLE_WAY_CONFIG_UPDATE_IND_T;

typedef struct
{
    anc_scenario_config_id_t anc_scenario_config_id;
    anc_toggle_config_t anc_config;
} ANC_SCENARIO_CONFIG_UPDATE_IND_T;

typedef struct
{
    anc_scenario_config_id_t scenario;
} ANC_CONCURRENCY_CONNECT_REQ_T;

typedef ANC_CONCURRENCY_CONNECT_REQ_T ANC_CONCURRENCY_DISCONNECT_REQ_T;

/*! \brief Events sent by Anc_Statemanager to other modules. */
typedef enum
{
    ANC_UPDATE_STATE_DISABLE_IND = ANC_MESSAGE_BASE,
    ANC_UPDATE_STATE_ENABLE_IND,
    ANC_UPDATE_MODE_CHANGED_IND,
    ANC_UPDATE_GAIN_IND,
    ANC_TOGGLE_WAY_CONFIG_UPDATE_IND,
    ANC_SCENARIO_CONFIG_UPDATE_IND,
    ANC_UPDATE_DEMO_MODE_DISABLE_IND,
    ANC_UPDATE_DEMO_MODE_ENABLE_IND,
    ANC_UPDATE_AANC_ADAPTIVITY_PAUSED_IND,
    ANC_UPDATE_AANC_ADAPTIVITY_RESUMED_IND,
    ANC_WORLD_VOLUME_GAIN_DB_UPDATE_IND,
    ANC_WORLD_VOLUME_BALANCE_UPDATE_IND,
    ANC_UPDATE_PREV_CONFIG_IND,
    ANC_UPDATE_PREV_MODE_IND,
    AANC_FB_GAIN_UPDATE_IND,
    ANC_WORLD_VOLUME_CONFIG_UPDATE_IND,
    AANC_FF_GAIN_UPDATE_IND,
    ANC_FF_GAIN_NOTIFY,
    ANC_UPDATE_QUIETMODE_ON_IND,
    ANC_UPDATE_QUIETMODE_OFF_IND,
    AANC_UPDATE_QUIETMODE_IND,
    ANC_UPDATE_WIND_DETECTION_DISABLE_IND,
    ANC_UPDATE_WIND_DETECTION_ENABLE_IND,    
    ANC_UPDATE_WIND_REDUCTION_ENABLE_IND,
    ANC_UPDATE_WIND_REDUCTION_DISABLE_IND,
    ANC_AUTO_TRANSPARENCY_ENABLE_IND,
    ANC_AUTO_TRANSPARENCY_DISABLE_IND,
    ANC_AUTO_TRANSPARENCY_RELEASE_TIME_IND,
    ANC_UPDATE_HOWLING_DETECTION_DISABLE_IND,
    ANC_UPDATE_HOWLING_DETECTION_ENABLE_IND,
    ANC_UPDATE_HOWLING_GAIN_REDUCTION_DISABLE_IND,
    ANC_UPDATE_HOWLING_GAIN_REDUCTION_ENABLE_IND,
    ANC_UPDATE_AAH_DETECTION_DISABLE_IND,
    ANC_UPDATE_AAH_DETECTION_ENABLE_IND,
    ANC_UPDATE_AAH_GAIN_REDUCTION_DISABLE_IND,
    ANC_UPDATE_AAH_GAIN_REDUCTION_ENABLE_IND,
    ANC_NOISE_ID_ENABLE_IND,
    ANC_NOISE_ID_DISABLE_IND,
    ANC_NOISE_ID_CATEGORY_CHANGE_IND,
    ANC_FB_GAIN_NOTIFY,
    /*! This must be the final message */
    ANC_MESSAGE_END
} anc_msg_t;

#define ANC_MAX_TOGGLE_CONFIG (3U)

/*! \brief ANC toggle way configurations. */
typedef struct {
    uint16 anc_toggle_way_config[ANC_MAX_TOGGLE_CONFIG];
} anc_toggle_way_config_t;

/*! \brief ANC toggle configuration during scenarios e.g., standalone, playback, SCO, VA, LE Stereo Recording. */
typedef struct {
    uint16 anc_config;
    uint16 is_same_as_current;
} anc_toggle_config_during_scenario_t;

/*! \brief World Volume Gain configuration of a leakthrough mode. */
typedef struct {
    int8 min_gain_dB;
    int8 max_gain_dB;
}anc_sm_world_volume_gain_mode_config_t;

/*! \brief World Volume Gain balance information.
           When the balance_device_side is left, world volume gain of right earbud will be decreased and vice-versa.
*/
typedef struct {
    bool balance_device_side; /* Left: FALSE, Right: TRUE */
    int8 balance_percentage; /* -100 to 100 % */
}anc_sm_world_volume_gain_balance_info_t;

typedef struct
{
    int8 world_volume_gain_dB;
} ANC_WORLD_VOLUME_GAIN_DB_UPDATE_IND_T;

typedef anc_sm_world_volume_gain_balance_info_t ANC_WORLD_VOLUME_BALANCE_UPDATE_IND_T;

typedef anc_sm_world_volume_gain_mode_config_t ANC_WORLD_VOLUME_CONFIG_UPDATE_IND_T;

/* Register with state proxy after initialization */
#ifdef ENABLE_ANC
void AncStateManager_PostInitSetup(void);
#else
#define AncStateManager_PostInitSetup() ((void)(0))
#endif

/*!
    \brief Initialisation of ANC feature, reads microphone configuration  
           and default mode.
    \param init_task Unused
    \return TRUE always.
*/
#ifdef ENABLE_ANC
bool AncStateManager_Init(Task init_task);
#else
#define AncStateManager_Init(x) (FALSE)
#endif

#ifdef ENABLE_ANC
TaskData *AncStateManager_GetTask(void);
#else
#define AncStateManager_GetTask() (NULL)
#endif


#ifdef ENABLE_ANC
bool AncStateManager_CheckIfDspClockBoostUpRequired(void);
#else
#define AncStateManager_CheckIfDspClockBoostUpRequired() (FALSE)
#endif
/*!
    \brief ANC specific handling due to the device Powering On.
*/
#ifdef ENABLE_ANC
void AncStateManager_PowerOn(void);
#else
#define AncStateManager_PowerOn() ((void)(0))
#endif

/*!
    \brief ANC specific handling due to the device Powering Off.
*/  
#ifdef ENABLE_ANC
void AncStateManager_PowerOff(void);
#else
#define AncStateManager_PowerOff() ((void)(0))
#endif

/*!
    \brief Enable ANC functionality.  
*/   
#ifdef ENABLE_ANC
void AncStateManager_Enable(void);
#else
#define AncStateManager_Enable() ((void)(0))
#endif

/*! 
    \brief Disable ANC functionality.
 */   
#ifdef ENABLE_ANC
void AncStateManager_Disable(void);
#else
#define AncStateManager_Disable() ((void)(0))
#endif

/*!
    \brief Is ANC supported in this build ?

    This just checks if ANC may be supported in the build.
    Separate checks are needed to determine if ANC is permitted
    (licenced) or enabled.

    \return TRUE if ANC is enabled in the build, FALSE otherwise.
 */
#ifdef ENABLE_ANC
#define AncStateManager_IsSupported() TRUE
#else
#define AncStateManager_IsSupported() FALSE
#endif


/*!
    \brief Set the operating mode of ANC to configured mode_n. 
    \param mode To be set from existing avalaible modes 0 to 9.
*/
#ifdef ENABLE_ANC
void AncStateManager_SetMode(anc_mode_t mode);
#else
#define AncStateManager_SetMode(x) ((void)(0 * (x)))
#endif

/*!
    \brief Handles the toggle way event from the user to switch to configured ANC mode
     This config comes from the GAIA app
    \param mode None
*/
#ifdef ENABLE_ANC
void AncStateManager_HandleToggleWay(void);
#else
#define AncStateManager_HandleToggleWay() ((void)(0))
#endif


/*!
    \brief Get the AANC params to implicitly enable ANC on a SCO call
    \param KYMERA_INTERNAL_AANC_ENABLE_T parameters to configure AANC capability
*/
#ifdef ENABLE_ANC
void AncStateManager_GetAdaptiveAncEnableParams(bool *in_ear, audio_anc_path_id *control_path, adaptive_anc_hw_channel_t *hw_channel, anc_mode_t *current_mode);
#else
#define AncStateManager_GetAdaptiveAncEnableParams(in_ear, control_path, hw_channel, current_mode) ((void)in_ear, (void)control_path, (void)hw_channel, (void)current_mode)
#endif

/*! 
    \brief Get the Anc mode configured.
    \return mode which is set (from available mode 0 to 9).
 */
#ifdef ENABLE_ANC
anc_mode_t AncStateManager_GetMode(void);
#else
#define AncStateManager_GetMode() (0)
#endif

/*! 
    \brief Checks if ANC is due to be enabled.
    \return TRUE if it is enabled else FALSE.
 */
#ifdef ENABLE_ANC
bool AncStateManager_IsEnabled (void);
#else
#define AncStateManager_IsEnabled() (FALSE)
#endif

/*! 
    \brief Get the Anc mode configured.
    \return mode which is set (from available mode 0 to 9).
 */
#ifdef ENABLE_ANC
anc_mode_t AncStateManager_GetCurrentMode(void);
#else
#define AncStateManager_GetCurrentMode() (0)
#endif

/*! 
    \brief The function returns the number of modes configured.
    \return total modes in anc_modes_t.
 */
#ifdef ENABLE_ANC
uint8 AncStateManager_GetNumberOfModes(void);
#else
#define AncStateManager_GetNumberOfModes() (0)
#endif

/*!
    \brief Checks whether tuning mode is currently active.
    \return TRUE if it is active, else FALSE.
 */
#ifdef ENABLE_ANC
bool AncStateManager_IsTuningModeActive(void);
#else
#define AncStateManager_IsTuningModeActive() (FALSE)
#endif

/*! 
    \brief Cycles through next mode and sets it.
 */
#ifdef ENABLE_ANC
void AncStateManager_SetNextMode(void);
#else
#define AncStateManager_SetNextMode() ((void)(0))
#endif

/*! 
    \brief Enters ANC tuning mode.
 */
#ifdef ENABLE_ANC
void AncStateManager_EnterAncTuningMode(void);
#else
#define AncStateManager_EnterAncTuningMode() ((void)(0))
#endif

/*! 
    \brief Exits the ANC tuning mode.
 */
#ifdef ENABLE_ANC
void AncStateManager_ExitAncTuningMode(void);
#else
#define AncStateManager_ExitAncTuningMode() ((void)(0))
#endif

/*!
    \brief To check if Left ANC channel is enabled.
    \return TRUE if enabled or FALSE if disabled.
 */
#ifdef ENABLE_ANC
bool AncStateManager_IsLeftChannelPathEnabled(void);
#else
#define AncStateManager_IsLeftChannelPathEnabled() (FALSE)
#endif

/*!
    \brief To check if Right ANC channel is enabled.
    \return TRUE if enabled or FALSE if disabled.
 */
#ifdef ENABLE_ANC
bool AncStateManager_IsRightChannelPathEnabled(void);
#else
#define AncStateManager_IsRightChannelPathEnabled() (FALSE)
#endif

/*!
    \brief To check if both Left and Right ANC channels are enabled.
    \return TRUE if enabled or FALSE if disabled.
 */
#ifdef ENABLE_ANC
bool AncStateManager_IsBothAncChannelsPathEnabled(void);
#else
#define AncStateManager_IsBothAncChannelsPathEnabled() (FALSE)
#endif

/*! 
    \brief Enters Adaptive ANC tuning mode.
 */
#if defined(HOSTED_TEST_ENVIRONMENT) || (defined(ENABLE_ANC) && defined(ENABLE_ADAPTIVE_ANC))
void AncStateManager_EnterAdaptiveAncTuningMode(void);
#else
#define AncStateManager_EnterAdaptiveAncTuningMode() ((void)(0))
#endif

/*! 
    \brief Exits Adaptive ANC tuning mode.
 */
#if defined(HOSTED_TEST_ENVIRONMENT) || (defined(ENABLE_ANC) && defined(ENABLE_ADAPTIVE_ANC))
void AncStateManager_ExitAdaptiveAncTuningMode(void);
#else
#define AncStateManager_ExitAdaptiveAncTuningMode() ((void)(0))
#endif

/*!
    \brief Checks whether Adaptive ANC tuning mode is currently active.
    \return TRUE if it is active, else FALSE.
 */
#if defined(HOSTED_TEST_ENVIRONMENT) || (defined(ENABLE_ANC) && defined(ENABLE_ADAPTIVE_ANC))
bool AncStateManager_IsAdaptiveAncTuningModeActive(void);
#else
#define AncStateManager_IsAdaptiveAncTuningModeActive() (FALSE)
#endif

/*! 
    \brief Updates ANC feedforward fine gain from ANC Data structure to ANC H/W. This is not applicable when in 'Mode 1'.
		   AncStateManager_StoreAncLeakthroughGain(uint8 leakthrough_gain) has to be called BEFORE calling AncStateManager_UpdateAncLeakthroughGain()
		   
		   This function shall be called for "World Volume Leakthrough".
		   
 */
#ifdef ENABLE_ANC
void AncStateManager_UpdateAncLeakthroughGain(void);
#else
#define AncStateManager_UpdateAncLeakthroughGain() ((void)(0))
#endif

/*! \brief Register a Task to receive notifications from Anc_StateManager.

    Once registered, #client_task will receive #shadow_profile_msg_t messages.

    \param client_task Task to register to receive shadow_profile notifications.
*/
#ifdef ENABLE_ANC
void AncStateManager_ClientRegister(Task client_task);
#else
#define AncStateManager_ClientRegister(x) ((void)(0))
#endif

/*! \brief Un-register a Task that is receiving notifications from Anc_StateManager.

    If the task is not currently registered then nothing will be changed.

    \param client_task Task to un-register from shadow_profile notifications.
*/
#ifdef ENABLE_ANC
void AncStateManager_ClientUnregister(Task client_task);
#else
#define AncStateManager_ClientUnregister(x) ((void)(0))
#endif

/*! \brief To obtain gain for current mode stored in ANC data structure

    \returns gain of ANC H/w
*/
#ifdef ENABLE_ANC
uint8 AncStateManager_GetAncGain(void);
#else
#define AncStateManager_GetAncGain() (0)
#endif

/*! \brief To obtain gain for current mode stored in ANC data structure

    \returns gain of ANC H/w
*/
#ifdef ENABLE_ANC
uint8 AncStateManager_GetAancFBGain(void);
#else
#define AncStateManager_GetAancFBGain() (0)
#endif

/*! \brief To obtain FF gain for Adaptive modes stored in ANC data structure

    \returns gain of ANC H/w
*/
#ifdef ENABLE_ANC
uint8 AncStateManager_GetAancFFGain(void);
#else
#define AncStateManager_GetAancFFGain() (0)
#endif

/*! \brief To store Leakthrough gain in ANC data structure

    \param leakthrough_gain Leakthrough gain to be stored
*/
#ifdef ENABLE_ANC
void AncStateManager_StoreAncLeakthroughGain(uint8 leakthrough_gain);
#else
#define AncStateManager_StoreAncLeakthroughGain(x) ((void)(0))
#endif

/*!
    \brief Interface to get ANC toggle configuration
 */
#ifdef ENABLE_ANC
anc_toggle_config_t AncStateManager_GetAncToggleConfiguration(anc_toggle_way_config_id_t config_id);
#else
#define AncStateManager_GetAncToggleConfiguration(x) ((0 * (x)))
#endif

/*!
    \brief Interface to set ANC toggle configuration
 */
#ifdef ENABLE_ANC
void AncStateManager_SetAncToggleConfiguration(anc_toggle_way_config_id_t config_id, anc_toggle_config_t config);
#else
#define AncStateManager_SetAncToggleConfiguration(x, y) ((void)((0 * x) * (0 * y)))
#endif

/*!
    \brief Interface to get ANC scenario configuration
 */
#ifdef ENABLE_ANC
anc_toggle_config_t AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_t config_id);
#else
#define AncStateManager_GetAncScenarioConfiguration(x) ((0 * (x)))
#endif

/*!
    \brief Interface to set ANC scenario configuration
 */
#ifdef ENABLE_ANC
void AncStateManager_SetAncScenarioConfiguration(anc_scenario_config_id_t config_id, anc_toggle_config_t config);
#else
#define AncStateManager_SetAncScenarioConfiguration(x, y) ((void)((0 * x) * (0 * y)))
#endif

/*!
    \brief Interface to enable Adaptive ANC Adaptivity
 */
#ifdef ENABLE_ANC
void AncStateManager_EnableAdaptiveAncAdaptivity(void);
#else
#define AncStateManager_EnableAdaptiveAncAdaptivity() ((void)(0))
#endif

/*!
    \brief Interface to disable Adaptive ANC Adaptivity
 */
#ifdef ENABLE_ANC
void AncStateManager_DisableAdaptiveAncAdaptivity(void);
#else
#define AncStateManager_DisableAdaptiveAncAdaptivity() ((void)(0))
#endif

/*!
    \brief Interface to get Adaptive ANC Adaptivity
 */
#ifdef ENABLE_ANC
bool AncStateManager_GetAdaptiveAncAdaptivity(void);
#else
#define AncStateManager_GetAdaptiveAncAdaptivity() (FALSE)
#endif

/*!
    \brief Return if Device supports Demo mode
    \param None
    \returns TRUE if supported, FALSE otherwise
*/
#ifdef ENABLE_ANC
bool AncStateManager_IsDemoSupported(void);
#else
#define AncStateManager_IsDemoSupported() FALSE
#endif

/*!
    \brief Return if Device is in Demo State
    \param None
    \returns TRUE if Active, FALSE otherwise
*/
#ifdef ENABLE_ANC
bool AncStateManager_IsDemoStateActive(void);
#else
#define AncStateManager_IsDemoStateActive() FALSE
#endif

/*!
    \brief Set the Demo State
    \param To put in Demo State or exit
*/
#ifdef ENABLE_ANC
void AncStateManager_SetDemoState(bool demo_active);
#else
#define AncStateManager_SetDemoState(x) ((void)(0 * (x)))
#endif

/*!
    \brief Check if a particular ANC mode is Adaptive or not
    \param mode Existing ANC modes anc_mode_1 to anc_mode_10
*/
#ifdef ENABLE_ANC
bool AncConfig_IsAncModeAdaptive(anc_mode_t anc_mode);
#else
#define AncConfig_IsAncModeAdaptive(x) (FALSE)
#endif

/*!
    \brief Check if a particular ANC mode is Leakthrough or not
    \param mode Existing ANC modes anc_mode_1 to anc_mode_10
*/
#ifdef ENABLE_ANC
bool AncConfig_IsAncModeLeakThrough(anc_mode_t anc_mode);
#else
#define AncConfig_IsAncModeLeakThrough(x) (FALSE)
#endif

/*!
    \brief Check if a particular ANC mode is Static or not
    \param mode Existing ANC modes anc_mode_1 to anc_mode_10
*/
#ifdef ENABLE_ANC
bool AncConfig_IsAncModeStatic(anc_mode_t anc_mode);
#else
#define AncConfig_IsAncModeStatic(x) (FALSE)
#endif

/*!
    \brief Check if a particular ANC mode is Adaptive Leakthrough(PSAP) or not
    \param mode Existing ANC modes anc_mode_1 to anc_mode_10
*/
#ifdef ENABLE_ANC
bool AncConfig_IsAncModeAdaptiveLeakThrough(anc_mode_t anc_mode);
#else
#define AncConfig_IsAncModeAdaptiveLeakThrough(x) (FALSE)
#endif

/*!
    \brief Check if a particular ANC mode is Static Leakthrough or not
    \param mode Existing ANC modes anc_mode_1 to anc_mode_10
*/
#ifdef ENABLE_ANC
bool AncConfig_IsAncModeStaticLeakThrough(anc_mode_t anc_mode);
#else
#define AncConfig_IsAncModeStaticLeakThrough(x) (FALSE)
#endif

/*!
    \brief Check if a particular ANC mode is Static Leakthrough or not
    \param mode Existing ANC modes anc_mode_1 to anc_mode_10
*/
#ifdef ENABLE_ANC
bool AncStateManager_IsFBGainTimerUpdateRequire(anc_mode_t anc_mode);
#else
#define AncStateManager_IsFBGainTimerUpdateRequire(x) (FALSE)
#endif

/*!
    \brief Check if Adaptive leakthrough (PSAP) mode configured in anc_config_data table
*/
#ifdef ENABLE_ANC
bool AncConfig_IsAncModeAdaptiveLeakThroughConfigured(void);
#else
#define AncConfig_IsAncModeAdaptiveLeakThroughConfigured() (FALSE)
#endif

/*!
    \brief Check if ANC supports Advanced features like Adaptive Ambient, 
     ANC Hardware Manager, Howling control etc
    \param TRUE if ANC supports advanced features, else FALSE
*/
#ifdef ENABLE_ANC
bool AncConfig_IsAdvancedAnc(void);
#else
#define AncConfig_IsAdvancedAnc() (FALSE)
#endif

/*!
    \brief Check if ANC LeakThrough gain previously updated
    \param mode ANC mode to be checked from anc_mode_1 to anc_mode_10
*/
#ifdef ENABLE_ANC
bool AncStateManager_IsAncLeakthroughGainPreserved(anc_mode_t mode);
#else
#define AncStateManager_IsAncLeakthroughGainPreserved(x) (FALSE)
#endif

/*!
    \brief Get ANC LeakThrough latest / previous updated gain
    \param leakthrough_mode ANC LeakThrough mode for which gain was updated
    \Note: This should not be called without AncStateManager_IsAncLeakthroughGainPreserved()
*/
#ifdef ENABLE_ANC
void ancStateManager_GetAncLeakthroughPreservedGain(anc_mode_t leakthrough_mode, uint8* leakthrough_gain);
#else
#define ancStateManager_GetAncLeakthroughPreservedGain(leakthrough_mode, leakthrough_gain) \
    (UNUSED(leakthrough_mode)); \
    (UNUSED(leakthrough_gain));
#endif

/*! 
    \brief Test hook for unit tests to reset the ANC state.
    \param state  Reset the particular state
 */
#ifdef ANC_TEST_BUILD

#ifdef ENABLE_ANC
void AncStateManager_ResetStateMachine(anc_state_manager_t state);
#else
#define AncStateManager_ResetStateMachine(x) ((void)(0))
#endif

#endif /* ANC_TEST_BUILD*/

/*!
    \brief Increase world volume in adaptive transparency mode.
    \param None
*/
#ifdef ENABLE_ANC
void AncStateManager_SetWorldVolumeUp(void);
#else
#define AncStateManager_SetWorldVolumeUp() ((void)(0))
#endif


/*!
\brief Decreases world volume in adaptive transparency mode.
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_SetWorldVolumeDown(void);
#else
#define AncStateManager_SetWorldVolumeDown() ((void)(0))
#endif

/*! \brief To store requested world volume gain in ANC data structure.
           AncStateManager_UpdateWorldVolumeGain() API has to be called to update stored world volume gain.
    \param world_volume_gain_dB World volume gain to be stored.
*/
#ifdef ENABLE_ANC
bool AncStateManager_StoreWorldVolumeGain(int8 world_volume_gain_dB);
#else
#define AncStateManager_StoreWorldVolumeGain(x) ((FALSE))
#endif

/*!
    \brief Updates world volume gain stored in ANC data structure. This is only applicable in leakthrough modes.
           AncStateManager_StoreWorldVolumeGain(world_volume_gain_dB) has to be called BEFORE calling this API.
 */
#ifdef ENABLE_ANC
void AncStateManager_UpdateWorldVolumeGain(void);
#else
#define AncStateManager_UpdateWorldVolumeGain() ((void)(0))
#endif

/*!
    \brief Gets the requested world volume gain which is yet to updated.
           This API would return requested world volume gain without considering balance percentage.
    \return Requested world volume gain.
 */
#ifdef ENABLE_ANC
int8 AncStateManager_GetRequestedWorldVolumeGain(void);
#else
#define AncStateManager_GetRequestedWorldVolumeGain() ((0))
#endif

/*!
    \brief Get the current world volume gain configured.
           This API would return current world volume gain without considering balance percentage.
    \param cur_world_volume_gain_dB Pointer to hold current world volume gain.
    \return TRUE if current mode is leakthrough mode, else FALSE.
 */
#ifdef ENABLE_ANC
bool AncStateManager_GetCurrentWorldVolumeGain(int8* cur_world_volume_gain_dB);
#else
#define AncStateManager_GetCurrentWorldVolumeGain(x) ((FALSE))
#endif

/*!
    \brief Get the current world volume dB slider step size.
    \return Step size.
 */
#ifdef ENABLE_ANC
uint8 AncStateManager_GetWorldVolumedBSliderStepSize(void);
#else
#define AncStateManager_GetWorldVolumedBSliderStepSize() ((0))
#endif

/*!
    \brief Get the current world volume configuration.
    \param world_volume_config Pointer to hold current world volume gain config.
 */
#ifdef ENABLE_ANC
void AncStateManager_GetCurrentWorldVolumeConfig(anc_sm_world_volume_gain_mode_config_t* world_volume_config);
#else
#define AncStateManager_GetCurrentWorldVolumeConfig(x) ((void)(0))
#endif

/*! \brief To store requested world volume balance info in ANC data structure.
           AncStateManager_UpdateWorldVolumeBalance() API has to be called to update stored world volume balance.
    \param balance_info World volume balance info to be stored
*/
#ifdef ENABLE_ANC
void AncStateManager_StoreWorldVolumeBalanceInfo(anc_sm_world_volume_gain_balance_info_t balance_info);
#else
#define AncStateManager_StoreWorldVolumeBalanceInfo(x) ((UNUSED(x)))
#endif

/*!
    \brief Updates world volume balance stored in ANC data structure.
           AncStateManager_StoreWorldVolumeBalanceInfo(balance_info) has to be called BEFORE calling this API.
 */
#ifdef ENABLE_ANC
void AncStateManager_UpdateWorldVolumeBalance(void);
#else
#define AncStateManager_UpdateWorldVolumeBalance() ((void)(0))
#endif

/*! \brief To store requested world volume balance info in ANC data structure.
           AncStateManager_UpdateWorldVolumeBalance() API has to be called to update stored world volume balance.
           This API is provided to be called by peer_ui to send balance infomation to peer.
    \param balance_info Encoded world volume balance info to be stored
                        balance_device_side and balance_percentage will be stored in balance_info as uint8 format
                        balance_device_side(TRUE/FALSE) is stored in bit7 and balance_percentage(0-100) will be stored from bit6 to bit0
                        balance_info = ((balance_device_side << 7) | balance_percentage))
*/
#ifdef ENABLE_ANC
void AncStateManager_StoreEncodedWorldVolumeBalanceInfo(uint8 balance_info);
#else
#define AncStateManager_StoreEncodedWorldVolumeBalanceInfo(x) ((void)(0*x))
#endif

/*!
    \brief Gets the requested world volume balance which is yet to updated.
           This API is provided to be called by peer_ui to send balance infomation to peer.
    \return Requested world volume balance info.
 */
#ifdef ENABLE_ANC
uint8 AncStateManager_GetEncodedWorldVolumeBalanceInfo(void);
#else
#define AncStateManager_GetEncodedWorldVolumeBalanceInfo() ((0))
#endif

/*!
    \brief Get the current world volume balance info.
    \param cur_world_volume_gain_dB Pointer to hold current world volume balance info.
    \return TRUE.
 */
#ifdef ENABLE_ANC
bool AncStateManager_GetCurrentBalanceInfo(anc_sm_world_volume_gain_balance_info_t* balance_info);
#else
#define AncStateManager_GetCurrentBalanceInfo(x) ((FALSE))
#endif

/*!
    \brief Get the current world volume balance percentage configured on THIS devie.
    \return balance percentage which is set (-100 to 100).
 */
#ifdef ENABLE_ANC
int8 AncStateManager_GetBalancePercentage(void);
#else
#define AncStateManager_GetBalancePercentage() ((0))
#endif


/*!
\brief API for handling wind detect message from Wind detect capability.
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_HandleWindDetect(void);
#else
#define AncStateManager_HandleWindDetect() ((void)(0))
#endif

/*!
\brief API for handling wind release message from Wind detect capability.
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_HandleWindRelease(void);
#else
#define AncStateManager_HandleWindRelease() ((void)(0))
#endif

/*!
\brief API for handling wind detect enable
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_HandleWindEnable(void);
#else
#define AncStateManager_HandleWindEnable() ((void)(0))
#endif

/*!
\brief API for handling wind detect disable
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_HandleWindDisable(void);
#else
#define AncStateManager_HandleWindDisable() ((void)(0))
#endif

/*!
\brief API for handling Anti howling enable
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_HandleAntiHowlingEnable(void);
#else
#define AncStateManager_HandleAntiHowlingEnable() ((void)(0))
#endif

/*!
\brief API for handling Anti howling disable
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_HandleAntiHowlingDisable(void);
#else
#define AncStateManager_HandleAntiHowlingDisable() ((void)(0))
#endif

/*!
    \brief API exposed to handle the Howling detection notifications to GAIA for Wind Detect State (Enable/Disable)
    \param None
*/
#ifdef ENABLE_ANC
void AncStateManager_HowlingDetectionStateUpdateInd(bool enable);
#else
#define AncStateManager_HowlingDetectionStateUpdateInd(enable) (UNUSED(enable))
#endif

/*!
\brief API exposed to handle the AAH state notification to GAIA (Enable/Disable)
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_AahStateUpdateInd(bool enable);
#else
#define AncStateManager_AahStateUpdateInd(enable) (UNUSED(enable))
#endif

/*!
\brief API exposed to handle the wind detect notifications to GAIA for Wind Detect State (Enable/Disable)
\param enable
*/
#ifdef ENABLE_ANC
void AncStateManager_WindDetectionStateUpdateInd(bool enable);
#else
#define AncStateManager_WindDetectionStateUpdateInd(enable) ((UNUSED(enable)))
#endif

/*!
\brief API exposed to handle the wind detect notifications to GAIA for ongoing Wind Reduction (Windy or not)
\param enable
*/
#ifdef ENABLE_ANC
void AncStateManager_WindReductionUpdateInd(bool enable);
#else
#define AncStateManager_WindReductionUpdateInd(enable) ((UNUSED(enable)))
#endif

/*!
\brief API to handle the trigger from VAD to move to ANC ambient mode
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_AutoAmbientTrigger(void);
#else
#define AncStateManager_AutoAmbientTrigger() ((void)(0))
#endif

/*!
\brief API to handle the release from VAD to move to previous ANC mode from ANC ambient mode
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_AutoAmbientRelease(void);
#else
#define AncStateManager_AutoAmbientRelease() ((void)(0))
#endif


/*!
\brief API exposed to handle the auto ambient enable notifications to GAIA for Auto Ambient State (Enable/Disable)
\param enable
*/
#ifdef ENABLE_ANC
void AncStateManager_AutoAmbientStateInd(bool enable);
#else
#define AncStateManager_AutoAmbientStateInd(enable) ((UNUSED(enable)))
#endif

/*!
\brief API exposed to handle the auto ambient enable notifications to GAIA for Auto Ambient Release time config
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_AutoAmbientReleaseTimeInd(void);
#else
#define AncStateManager_AutoAmbientReleaseTimeInd()  ((void)(0))
#endif

/*!
\brief Get the previous config - this includes ANC mode and state
\return previous config
 */
#ifdef ENABLE_ANC
uint16 AncStateManager_GetPreviousConfig(void);
#else
#define AncStateManager_GetPreviousConfig() ((0))
#endif

/*!
\brief Get the previous mode while ANC was OFF
\return previous mode
 */
#ifdef ENABLE_ANC
uint16 AncStateManager_GetPreviousMode(void);
#else
#define AncStateManager_GetPreviousMode() ((0))
#endif


/*!
\brief API exposed to handle standalone to concurrency request
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_StandaloneToConcurrencyReq(void);
#else
#define AncStateManager_StandaloneToConcurrencyReq(void)  ((void)(0))
#endif

/*!
\brief API exposed to handle concurrency to standalone request
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_ConcurrencyToStandaloneReq(void);
#else
#define AncStateManager_ConcurrencyToStandaloneReq(void)  ((void)(0))
#endif

/*!
\brief API to return if Noise ID feature is supported by this ANC mode or not
\param None
*/
#ifdef ENABLE_ANC
bool AncConfig_IsNoiseIdSupportedForMode(anc_mode_t anc_mode);
#else
#define AncConfig_IsNoiseIdSupportedForMode(anc_mode) ((FALSE))
#endif

/*!
    \brief Interface to set ANC scenario configuration
 */
#ifdef ENABLE_ANC
uint8 AncConfig_GetAncModeForNoiseCategoryType(bool is_adaptive, anc_noise_id_category_t noise_category);
#else
#define AncConfig_GetAncModeForNoiseCategoryType(x, y) ((void)((0 * x) * (0 * y)))
#endif


/*!
\brief API to return if Noise ID feature is supported by this ANC mode or not
\param None
*/
#ifdef ENABLE_ANC
anc_noise_id_category_t AncConfig_GetNoiseIdCategoryForMode(anc_mode_t anc_mode);
#else
#define AncConfig_GetNoiseIdCategoryForMode(anc_mode) ((0 * (anc_mode)))
#endif

/*!
\brief API to return if Noise ID configurations for static and adaptive modes are valid
       Each mode type must support atleast two noise categories for Noise ID feature.
\param None
*/
#ifdef ENABLE_ANC
bool AncConfig_ValidNoiseIdConfiguration(void);
#else
#define AncConfig_ValidNoiseIdConfiguration() ((FALSE))
#endif

/*!
\brief API to handle the mode changes for noise ID category changes
\param enable
*/
#ifdef ENABLE_ANC
void AncStateManager_NoiseIdModeChange(anc_mode_t mode);
#else
#define AncStateManager_NoiseIdModeChange(anc_mode) ((0 * (anc_mode)))
#endif

/*!
\brief API to handle the noise ID state notifications to GAIA (Enable/Disable)
\param enable
*/
#ifdef ENABLE_ANC
void AncStateManager_NoiseIdStateInd(bool enable);
#else
#define AncStateManager_NoiseIdStateInd(enable) ((UNUSED(enable)))
#endif

/*!
\brief API to handle the noise ID category change notifications to GAIA
\param ANC Mode
*/
#ifdef ENABLE_ANC
void AncStateManager_NoiseIdCategoryInd(anc_noise_id_category_t nid);
#else
#define AncStateManager_NoiseIdCategoryInd(nid) ((UNUSED(nid)))
#endif

/*!
\brief API to set parallel ANC filter topology
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_SetFilterTopology(anc_filter_topology_t filter_topology);
#else
#define AncStateManager_SetFilterTopology(x) ( (UNUSED(x)) )
#endif

/*!
\brief API exposed to handle the Howling reduction notification to GAIA (Enable/Disable)
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_HowlingDetectionUpdateInd(bool enable);
#else
#define AncStateManager_HowlingDetectionUpdateInd(enable) (UNUSED(enable))
#endif

/*!
\brief API exposed to handle the AAH reduction notification to GAIA (Enable/Disable)
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_AahDetectionUpdateInd(bool enable);
#else
#define AncStateManager_AahDetectionUpdateInd(enable) (UNUSED(enable))
#endif

/*!
\brief API for handling AAH enable
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_HandleAahEnable(void);
#else
#define AncStateManager_HandleAahEnable() ((void)(0))
#endif

/*!
\brief API for handling AAH disable
\param None
*/
#ifdef ENABLE_ANC
void AncStateManager_HandleAahDisable(void);
#else
#define AncStateManager_HandleAahDisable() ((void)(0))
#endif

/*!
\brief API to return if quiet mode is allowed
\param None
\return TRUE = allowed, FALSE = not allowed
*/
#ifdef ENABLE_ANC
bool AncStateManager_IsQuietModeAllowed(void);
#else
#define AncStateManager_IsQuietModeAllowed() ((FALSE))
#endif

/*!
    \brief To check if ANC state manager is initialized
    \param None
    \return TRUE if state manager is initialized.
*/
#ifdef ENABLE_ANC
bool AncStateManager_IsInitialized(void);
#else
#define AncStateManager_IsInitialized() (FALSE)
#endif

#endif /* ANC_STATE_MANAGER_H_ */
/*! @} */
