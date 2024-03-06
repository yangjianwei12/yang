/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_anc_common.h
\brief      Private header to common anc kymera realted functionality
*/
#ifndef KYMERA_ANC_COMMON_H_
#define KYMERA_ANC_COMMON_H_

#include <app/audio/audio_if.h>
#include <operator.h>
#include "av_seids.h"
#include "kymera.h"
#ifdef ENABLE_WIND_DETECT
#include "wind_detect.h"
#endif

#if (defined(ENABLE_SELF_SPEECH) || defined (ENABLE_AUTO_AMBIENT)) && !defined (ENABLE_ADAPTIVE_ANC)
#error Defining Self speech without adaptive ANC is not allowed.
#endif

/*! Connect parameters for Adaptive ANC tuning  */
typedef struct
{
    uint32 usb_rate;
    Source spkr_src;
    Sink mic_sink;
    uint8 spkr_channels;
    uint8 mic_channels;
    uint8 frame_size;
} adaptive_anc_tuning_connect_parameters_t;

/*! Disconnect parameters for Adaptive ANC tuning  */
typedef struct
{
    Source spkr_src;
    Sink mic_sink;
    void (*kymera_stopped_handler)(Source source);
} adaptive_anc_tuning_disconnect_parameters_t;

/*! \brief The KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_START message content. */
typedef struct
{
    uint32 usb_rate;
    Source spkr_src;
    Sink mic_sink;
    uint8 spkr_channels;
    uint8 mic_channels;
    uint8 frame_size;
} KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_START_T;

/*! \brief The KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_STOP message content. */
typedef struct
{
    Source spkr_src;
    Sink mic_sink;
    void (*kymera_stopped_handler)(Source source);
} KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_STOP_T;

/*********************************** Quiet Mode related Functionality *****************************/
#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_EnableQuietMode(void);
#else
#define KymeraAncCommon_EnableQuietMode() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_DisableQuietMode(void);
#else
#define KymeraAncCommon_DisableQuietMode() ((void)(0))
#endif

/*********************************** Quiet Mode related Functionality Ends ****************************/

/*********************************** ANC common Functionality     ****************************/
#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_Init(void);
#else
#define KymeraAncCommon_Init() ((void)(0))
#endif


#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_GetFFGain(uint8 *gain);
#else
#define KymeraAncCommon_GetFFGain(gain) (UNUSED(gain))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_GetFBGain(uint8 *gain);
#else
#define KymeraAncCommon_GetFBGain(gain) (UNUSED(gain))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_GetFineGain(uint8 *gain_inst0, uint8 *gain_inst1, audio_anc_path_id audio_anc_path);
#else
#define KymeraAncCommon_GetFineGain(gain_inst0, gain_inst1, audio_anc_path) \
    (UNUSED(gain_inst0)); \
    (UNUSED(gain_inst1)); \
    (UNUSED(audio_anc_path));
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_GetCoarseGain(int8 *gain_inst0, int8 *gain_inst1, audio_anc_path_id audio_anc_path);
#else
#define KymeraAncCommon_GetCoarseGain(gain_inst0, gain_inst1, audio_anc_path) \
    (UNUSED(gain_inst0)); \
    (UNUSED(gain_inst1)); \
    (UNUSED(audio_anc_path));
#endif

#ifdef ENABLE_ADAPTIVE_ANC
audio_dsp_clock_type KymeraAncCommon_GetOptimalAudioClockAancScoConcurrency(appKymeraScoMode mode);
#else
#define KymeraAncCommon_GetOptimalAudioClockAancScoConcurrency(x) ((FALSE))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
audio_dsp_clock_type KymeraAncCommon_GetOptimalAudioClockAancMusicConcurrency(int seid);
#else
#define KymeraAncCommon_GetOptimalAudioClockAancMusicConcurrency(x) ((FALSE))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_AdaptiveAncSetGainValues(uint32 mantissa, uint32 exponent);
#else
#define KymeraAncCommon_AdaptiveAncSetGainValues(mantisaa,exponent) ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_UpdateInEarStatus(void);
#else
#define KymeraAncCommon_UpdateInEarStatus() ((void) (0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_UpdateOutOfEarStatus(void);
#else
#define KymeraAncCommon_UpdateOutOfEarStatus() ((void) (0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_ApplyModeChange(anc_mode_t mode, audio_anc_path_id anc_path,adaptive_anc_hw_channel_t anc_hw_channel);
#else
#define KymeraAncCommon_ApplyModeChange(mode,anc_path,anc_hw_channel) ((FALSE))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_AdaptiveAncEnableGentleMute(void);
#else
#define KymeraAncCommon_AdaptiveAncEnableGentleMute() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_EnableAdaptivity(void);
#else
#define KymeraAncCommon_EnableAdaptivity() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_DisableAdaptivity(void);
#else
#define KymeraAncCommon_DisableAdaptivity() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_AdaptiveAncIsNoiseLevelBelowQmThreshold(void);
#else
#define KymeraAncCommon_AdaptiveAncIsNoiseLevelBelowQmThreshold() (FALSE)
#endif

#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_AdaptiveAncIsEnabled(void);
#else
#define KymeraAncCommon_AdaptiveAncIsEnabled() (FALSE)
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_AdaptiveAncDisable(void);
#else
#define KymeraAncCommon_AdaptiveAncDisable() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_AncEnable(const KYMERA_INTERNAL_AANC_ENABLE_T * msg);
#else
#define KymeraAncCommon_AncEnable(x) (UNUSED(x))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_PreAncDisable(void);
#else
#define KymeraAncCommon_PreAncDisable() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_PreAncModeChange(void);
#else
#define KymeraAncCommon_PreAncModeChange() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_AdaptiveAncIsConcurrencyActive(void);
#else
#define KymeraAncCommon_AdaptiveAncIsConcurrencyActive() (FALSE)
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_AdaptiveAncSetUcid(anc_mode_t mode);
#else
#define KymeraAncCommon_AdaptiveAncSetUcid(x) (UNUSED(x))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_RampCompleteAction(void);
#else
#define KymeraAncCommon_RampCompleteAction() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_AhmRampExpiryAction(void);
#else
#define KymeraAncCommon_AhmRampExpiryAction() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_ExitAdaptiveAncTuning(const adaptive_anc_tuning_disconnect_parameters_t *param);
#else
#define KymeraAncCommon_ExitAdaptiveAncTuning(x) (UNUSED(x))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_EnterAdaptiveAncTuning(const adaptive_anc_tuning_connect_parameters_t *param);
#else
#define KymeraAncCommon_EnterAdaptiveAncTuning(x) (UNUSED(x))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_CreateAdaptiveAncTuningChain(const KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_START_T *msg);
#else
#define KymeraAncCommon_CreateAdaptiveAncTuningChain(x) (UNUSED(x))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_DestroyAdaptiveAncTuningChain(const KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_STOP_T *msg);
#else
#define KymeraAncCommon_DestroyAdaptiveAncTuningChain(x) (UNUSED(x))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_AncCompanderMakeupGainVolumeDown(void);
#else
#define KymeraAncCommon_AncCompanderMakeupGainVolumeDown() ((void) (0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_AncCompanderMakeupGainVolumeUp(void);
#else
#define KymeraAncCommon_AncCompanderMakeupGainVolumeUp() ((void)(0))
#endif

/*! \brief Obtain Current Adaptive ANC mode from AANC operator
    \param aanc_mode - pointer to get the value
    \return TRUE if current mode is stored in aanc_mode, else FALSE
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_GetApdativeAncCurrentMode(adaptive_anc_mode_t *aanc_mode);
#else
#define KymeraAncCommon_GetApdativeAncCurrentMode(aanc_mode) (FALSE)
#endif


/*! \brief Obtain Current Adaptive ANC V2 mode from AANC operator
    \param aanc_mode - pointer to get the value
    \return TRUE if current mode is stored in aanc_mode, else FALSE
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_GetApdativeAncV2CurrentMode(adaptive_ancv2_sysmode_t *aancv2_mode);
#else
#define KymeraAncCommon_GetApdativeAncV2CurrentMode(aancv2_mode) (FALSE)
#endif

/*! \brief Obtain Current AHM mode from AHM operator
    \param ahm_mode - pointer to get the value
    \return TRUE if current mode is stored in ahm_mode, else FALSE
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_GetAhmMode(ahm_sysmode_t *ahm_mode);
#else
#define KymeraAncCommon_GetAhmMode(ahm_mode) (FALSE)
#endif

/*! \brief Update FF fine target gain to AHM operator.
    \param ff_fine_gain FF fine target gain to be updated.
    \return TRUE if FF fine target gain is updated to AHM operator, else FALSE.
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_UpdateAhmFfPathFineTargetGain(uint8 ff_fine_gain);
#else
#define KymeraAncCommon_UpdateAhmFfPathFineTargetGain(x) ((FALSE))
#endif

/*! \brief Update makeup gain to ANC compander operator.
    \param makeup_gain_fixed_point Makeup gain in fixed point format(2.N) to be updated.
    \return TRUE if makeup gain is updated to ANC compander operator, else FALSE.
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_UpdateAncCompanderMakeupGain(int32 makeup_gain_fixed_point);
#else
#define KymeraAncCommon_UpdateAncCompanderMakeupGain(x) ((FALSE))
#endif

/*! \brief Obtain Current makeup gain in fixed point format(2.N) from ANC compander operator.
    \param makeup_gain_fixed_point pointer to get the makeup gain value in fixed point.
    \return TRUE if current makeup gain is stored in makeup_gain_fixed_point, else FALSE.
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_GetAncCompanderMakeupQGain(int32* makeup_gain_fixed_point);
#else
#define KymeraAncCommon_GetAncCompanderMakeupQGain(x) ((FALSE))
#endif

/*! \brief Kymera action on Wind detect attack depending on the stage
    \param attack_stage - whether wind detect attack is detected for stage 1 or stage 2
    \return void
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_WIND_DETECT)
void KymeraAncCommon_WindDetectAttack(windDetectStatus_t attack_stage, wind_detect_intensity_t wind_intensity);
#else
#define KymeraAncCommon_WindDetectAttack(x, y) \
    UNUSED(x); \
    UNUSED(y)
#endif

/*! \brief Kymera action on Wind detect release depending on the stage
    \param release_stage - whether wind detect release is detected for stage 1 or stage 2
    \return void
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_WIND_DETECT)
void KymeraAncCommon_WindDetectRelease(windDetectStatus_t release_stage);
#else
#define KymeraAncCommon_WindDetectRelease(release_stage) (UNUSED(release_stage))
#endif

/*! \brief Kymera action on request to enable wind detect.
    Move Wind Detect out of Standby mode
    \param void
    \return void
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_WIND_DETECT)
void KymeraAncCommon_EnableWindDetect(void);
#else
#define KymeraAncCommon_EnableWindDetect() ((void)(0))
#endif

/*! \brief Kymera action on request to disable wind detect.
    Move Wind Detect in Standby mode so as not to process Wind
    \param void
    \return void
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_WIND_DETECT)
void KymeraAncCommon_DisableWindDetect(void);
#else
#define KymeraAncCommon_DisableWindDetect() ((void)(0))
#endif

/*!
    \brief API identifying Howling Detection support
    \param None
    \return TRUE for feature supported otherwise FALSE
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_IsHowlingDetectionSupported(void);
#else
#define KymeraAncCommon_IsHowlingDetectionSupported() (FALSE)
#endif

/*!
    \brief API identifying Howling Detection state
    \param None
    \return TRUE for state is Enabled otherwise FALSE
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_IsHowlingDetectionEnabled(void);
#else
#define KymeraAncCommon_IsHowlingDetectionEnabled() (FALSE)
#endif

/*! \brief Kymera action on request to change hcgr system mode.
    \param mode: whether HCGR running on Standby mode or fullProc mode
    \return void
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraAncCommon_UpdateHowlingDetectionState(bool enable);
#else
#define KymeraAncCommon_UpdateHowlingDetectionState(enable) (UNUSED(enable))
#endif

/*! \brief Self Speech Enable and ANC mic framework actions
    \param void
    \return void
*/
#if defined(ENABLE_SELF_SPEECH) && defined (ENABLE_AUTO_AMBIENT)
void KymeraAncCommon_SelfSpeechDetectEnable(void);
#else
#define KymeraAncCommon_SelfSpeechDetectEnable() ((void)(0))
#endif

/*! \brief Self Speech Disable and ANC mic framework actions
    \param void
    \return void
*/
#if defined(ENABLE_SELF_SPEECH) && defined (ENABLE_AUTO_AMBIENT)
void KymeraAncCommon_SelfSpeechDetectDisable(void);
#else
#define KymeraAncCommon_SelfSpeechDetectDisable() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_TransitionCompleteAction(void);
#else
#define KymeraAncCommon_TransitionCompleteAction() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_SetFilterCoefficients(void);
#else
#define KymeraAncCommon_SetFilterCoefficients() ((void)(0))
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_HandleConcurrencyUpdate(bool is_concurrency_active);
#else
#define KymeraAncCommon_HandleConcurrencyUpdate(is_concurrency_active) (UNUSED(is_concurrency_active))
#endif

/*!
    \brief API identifying Current whether AANC state Active or not
    \param None
    \return TRUE for state is Enabled or Enable_Initiated otherwise FALSE
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool KymeraAncCommon_IsAancActive(void);
#else
#define KymeraAncCommon_IsAancActive() (FALSE)
#endif

#ifdef ENABLE_ADAPTIVE_ANC
uint8 KymeraAncCommon_GetAhmAdjustedFfFineGain(void);
#else
#define KymeraAncCommon_GetAhmAdjustedFfFineGain() ((0))
#endif

/*!
    \brief API to enable Noise ID capability
    \param None
    \return None
*/
#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_NoiseIDEnable(void);
#else
#define KymeraAncCommon_NoiseIDEnable() ((void)(0))
#endif

/*!
    \brief API to enable or disable Noise ID capability
    \param None
    \return None
*/
#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_NoiseIDEnableOrDisable(void);
#else
#define KymeraAncCommon_NoiseIDEnableOrDisable() ((void)(0))
#endif

/*!
    \brief API to disable Noise ID capability
    \param None
    \return None
*/
#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_NoiseIDDisable(void);
#else
#define KymeraAncCommon_NoiseIDDisable() ((void)(0))
#endif

/*!
    \brief Set the Noise category in capability based on current ANC mode
    \param None
    \return None
*/
#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_SetCategoryBasedOnCurrentMode(void);
#else
#define KymeraAncCommon_SetCategoryBasedOnCurrentMode() ((void)(0));
#endif

/*!
    \brief Set the Noise category in capability
    \param None
    \return None
*/
#ifdef ENABLE_ADAPTIVE_ANC
void KymeraAncCommon_SetNoiseID(noise_id_category_t nid);
#else
#define KymeraAncCommon_SetNoiseID(nid) (UNUSED(nid));
#endif

/*!
    \brief Get the Noise category from capability
    \param None
    \return Noise category
*/
#ifdef ENABLE_ADAPTIVE_ANC
noise_id_category_t KymeraAncCommon_GetNoiseID(void);
#else
#define KymeraAncCommon_GetNoiseID() ((0))
#endif

/*! \brief Set ANC filter topology between parallel and dual mode
    \param mode: filter topology
    \return void
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraAncCommon_SetAncFilterTopology(anc_filter_topology_t filter_topology);
#else
#define KymeraAncCommon_SetAncFilterTopology(x) (UNUSED(x))
#endif

/*! \brief Set ANC filter topology between parallel and dual mode
    \param mode: filter topology
    \return void
*/
#if defined(ENABLE_ADAPTIVE_ANC)
bool KymeraAncCommon_IsSelfSpeechDetectActive(void);
#else
#define KymeraAncCommon_IsSelfSpeechDetectActive() (FALSE)
#endif

/*!
    \brief API to identify if AAH feature is supported or not
    \param None
    \return TRUE for feature supported otherwise FALSE
*/
#if defined(ENABLE_ADAPTIVE_ANC)
bool KymeraAncCommon_IsAahFeatureSupported(void);
#else
#define KymeraAncCommon_IsAahFeatureSupported() (FALSE)
#endif

/*! \brief Get current state of AAH
    \param None
    \return TRUE=enabled, FALSE=disabled
*/
#if defined(ENABLE_ADAPTIVE_ANC)
bool KymeraAncCommon_GetAahCurrentState(void);
#else
#define KymeraAncCommon_GetAahCurrentState(void) (FALSE)
#endif

/*! \brief Kymera action on request to change AAH system mode.
    \param mode: whether AAH running on Standby mode or fullProc mode
    \return void
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraAncCommon_UpdateAahState(bool enable);
#else
#define KymeraAncCommon_UpdateAahState(enable) (UNUSED(enable))
#endif

/*! \brief Start EFT graph with a delay during A2DP start
    \param None
    \return void
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraAncCommon_StartEftDelayed(void);
#else
#define KymeraAncCommon_StartEftDelayed() ((void)(0))
#endif

/*! \brief Stop continuous EFT.
    \param None
    \return void
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraAncCommon_StopContinuousEft(void);
#else
#define KymeraAncCommon_StopContinuousEft() ((void)(0))
#endif

/*!
    \brief API to set Windy mode FF/FB ramp duration parameters
    \param ff_ramp_duration Ramp duration in FF path
    \param fb_ramp_duration Ramp duration in FB path
    \return None
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_WIND_DETECT)
void KymeraAncCommon_SetWindyModeRampDurationParameters(uint32 ff_ramp_duration, uint32 fb_ramp_duration);
#else
#define KymeraAncCommon_SetWindyModeRampDurationParameters(x, y) \
    UNUSED(x); \
    UNUSED(y)
#endif

/*!
    \brief API to set Windy mode FF/FB gain parameters
    \param ff_fine_gain FF path gain
    \param fb_fine_gain FB path gain
    \return None
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_WIND_DETECT)
void KymeraAncCommon_SetWindyModeGainParameters(uint32 ff_fine_gain, uint32 fb_fine_gain);
#else
#define KymeraAncCommon_SetWindyModeGainParameters(x, y) \
    UNUSED(x); \
    UNUSED(y)
#endif

#endif //KYMERA_ANC_COMMON_H_
