/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module for configuring the DSP clock/power settings
*/

#include "kymera_dsp_clock.h"
#include "kymera_data.h"
#include "kymera_state.h"
#include "kymera_output_if.h"
#include "kymera_latency_manager.h"
#include "kymera_tones_prompts.h"
#include "kymera_va.h"
#include "kymera_a2dp.h"
#include "kymera_sco_private.h"
#include "latency_config.h"
#include "av_seids.h"
#include "anc_state_manager.h"
#include "fit_test.h"
#include "kymera_anc_common.h"
#include "kymera_anc.h"
#include "kymera_mic_if.h"
#include <audio_power.h>

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST) 
#include "kymera_le_common.h"
#endif

#include <list.h>

#define MHZ_TO_HZ (1000000)

#if defined(__QCC307X__) || defined(__QCC517X__)
#define MAX_DSP_CLOCK AUDIO_DSP_TURBO_PLUS_CLOCK
#else
#define MAX_DSP_CLOCK  AUDIO_DSP_TURBO_CLOCK
#endif

typedef struct
{
    /* The clock speed mapped to the AUDIO_DSP_SLOW_CLOCK type */
    uint8 slow_clock_speed_mhz;
    /* The clock speed mapped to the AUDIO_DSP_VERY_SLOW_CLOCK type */
    uint8 very_slow_clock_speed_mhz;
} kymera_dsp_clock_speed_config_t;

typedef struct
{
      audio_dsp_clock_configuration clocks;
      kymera_dsp_clock_speed_config_t speed;
} kymera_dsp_clock_configs_t;

static kymera_dsp_clock_configs_t current_kymera_dsp_clock_config =
{
    .clocks = {
        .active_mode = AUDIO_DSP_CLOCK_NO_CHANGE,
        .low_power_mode = AUDIO_DSP_CLOCK_NO_CHANGE,
        .trigger_mode = AUDIO_DSP_CLOCK_NO_CHANGE
    },
    .speed = {
        .slow_clock_speed_mhz = DEFAULT_LOW_POWER_CLK_SPEED_MHZ,
        .very_slow_clock_speed_mhz = DEFAULT_VERY_LOW_POWER_CLK_SPEED_MHZ
    }
};

static list_t kymera_dsp_clock_config_user_ifs = NULL;
static void kymera_MapDspPowerModes(kymera_dsp_clock_speed_config_t * new_cpu_speed);

static audio_dsp_clock_type appKymeraGetNbWbScoDspClockType(void)
{
#if defined(KYMERA_SCO_USE_3MIC)
#if defined(INCLUDE_HYBRID_CVC)
    return AUDIO_DSP_TURBO_PLUS_CLOCK;
#else
    return AUDIO_DSP_TURBO_CLOCK;
#endif
#else
    if((Kymera_MicGetActiveUsers() & mic_user_aanc) != 0)
    {
        return AUDIO_DSP_TURBO_CLOCK;
    }
    return AUDIO_DSP_BASE_CLOCK;
#endif
}

static audio_dsp_clock_type appKymeraGetSwbScoDspClockType(void)
{
    return MAX_DSP_CLOCK;
}

static audio_dsp_clock_type appKymeraGetMinClockForTonesPrompts(void)
{
    /* Ideally the minimum clock required for standalone tone/prompts play back is AUDIO_DSP_SLOW_CLOCK (32MHz).
       However, its observed that this might not be enough and under some stress use-cases even for standalone
       prompt/tone playback we need to have AUDIO_DSP_BASE_CLOCK (80MHz), to give the additional sleep time.
       But this automatically doesn't mean that all the other use-case, like 96KHz output rate or faster kick period
       or for single core the clock concurrency scenario, needs to be AUDIO_DSP_TURBO_CLOCK (120MHz).
       Its should work under AUDIO_DSP_BASE_CLOCK clock.
       This is the reason for this interface to return the minimum clock as AUDIO_DSP_BASE_CLOCK.
    */
    return AUDIO_DSP_BASE_CLOCK;
}

static audio_dsp_clock_type kymera_GetDSPClockDuringAptx(void)
{
#ifdef ENABLE_CONTINUOUS_EARBUD_FIT_TEST
    return AUDIO_DSP_TURBO_CLOCK;
#else
    return AncStateManager_IsEnabled() ? AUDIO_DSP_TURBO_CLOCK : AUDIO_DSP_BASE_CLOCK;
#endif
}

static audio_dsp_clock_type kymera_GetDSPClockDuringAptxAdaptive(void)
{
    audio_dsp_clock_type mode; 

#ifdef INCLUDE_DECODERS_ON_P1

    /* For aptX Adaptive decoder running on P1, clock needs to be AUDIO_DSP_TURBO_CLOCK (120MHz).*/ 
    mode = AUDIO_DSP_TURBO_CLOCK; 

#else /* !INCLUDE_DECODERS_ON_P1 */

    /* Not enough MIPs to run aptX adaptive (TWS standard and TWS+) on base clock in a single audio core */
    mode = MAX_DSP_CLOCK; 

#endif /* INCLUDE_DECODERS_ON_P1 */

#if !defined(__QCC307X__) && !defined(__QCC517X__)
    mode = AUDIO_DSP_BASE_CLOCK;
#ifdef INCLUDE_STEREO
    /* Not enough MIPS in 4x to run aptX AD 96KHz on base clock */
    if(KymeraGetTaskData()->output_rate == 96000)
        mode = AUDIO_DSP_TURBO_CLOCK;
#endif /*INCLUDE_STEREO*/
#endif

    return mode;
}

static void kymera_UpdateHighestDspClockUsingRegisteredUsersInput(audio_dsp_clock_configuration * cconfig, audio_power_save_mode * mode)
{
    if (kymera_dsp_clock_config_user_ifs == NULL)
        return;

    LIST_FOREACH(&kymera_dsp_clock_config_user_ifs)
    {
        const kymera_dsp_clock_user_if_t * registered_if = ListGetCurrentReference(&kymera_dsp_clock_config_user_ifs);
        audio_dsp_clock_configuration user_config = *cconfig;
        audio_power_save_mode user_mode = *mode;

        if(registered_if->get_dsp_clock_config(&user_config, &user_mode))
        {
            cconfig->active_mode = MAX(cconfig->active_mode, user_config.active_mode);
            cconfig->low_power_mode = MAX(cconfig->low_power_mode, user_config.low_power_mode);
            cconfig->trigger_mode = MAX(cconfig->trigger_mode, user_config.trigger_mode);
            *mode = MIN(*mode, user_mode);
        }
    }
}

static inline bool kymera_HasClockModeChanged(audio_dsp_clock_type old_mode, audio_dsp_clock_type new_mode)
{
    return (new_mode != AUDIO_DSP_CLOCK_NO_CHANGE) &&
           (new_mode != old_mode);
}

static bool kymera_HasDspClockConfigChanged(audio_dsp_clock_configuration * cconfig, kymera_dsp_clock_speed_config_t * speed_config)
{
    bool config_changed = FALSE;

    if (kymera_HasClockModeChanged(current_kymera_dsp_clock_config.clocks.active_mode, cconfig->active_mode))
    {
        config_changed = TRUE;
        current_kymera_dsp_clock_config.clocks.active_mode = cconfig->active_mode;
    }
    if (kymera_HasClockModeChanged(current_kymera_dsp_clock_config.clocks.low_power_mode, cconfig->low_power_mode))
    {
        config_changed = TRUE;
        current_kymera_dsp_clock_config.clocks.low_power_mode = cconfig->low_power_mode;
    }
    if (kymera_HasClockModeChanged(current_kymera_dsp_clock_config.clocks.trigger_mode, cconfig->trigger_mode))
    {
        config_changed = TRUE;
        current_kymera_dsp_clock_config.clocks.trigger_mode = cconfig->trigger_mode;
    }
    if (current_kymera_dsp_clock_config.speed.slow_clock_speed_mhz != speed_config->slow_clock_speed_mhz)
    {
        config_changed = TRUE;
        current_kymera_dsp_clock_config.speed.slow_clock_speed_mhz = speed_config->slow_clock_speed_mhz;
    }
    if (current_kymera_dsp_clock_config.speed.very_slow_clock_speed_mhz != speed_config->very_slow_clock_speed_mhz)
    {
        config_changed = TRUE;
        current_kymera_dsp_clock_config.speed.very_slow_clock_speed_mhz = speed_config->very_slow_clock_speed_mhz;
    }

    return config_changed;
}

static void kymera_UpdateConfigWithHighestValuesFromProposedConfig(audio_dsp_clock_configuration * current_cconfig,
                                                                   audio_dsp_clock_configuration * proposed_cconfig)
{
    current_cconfig->active_mode =    MAX(current_cconfig->active_mode, proposed_cconfig->active_mode);
    current_cconfig->low_power_mode = MAX(current_cconfig->low_power_mode, proposed_cconfig->low_power_mode);
    current_cconfig->trigger_mode =   MAX(current_cconfig->trigger_mode, proposed_cconfig->trigger_mode);
}

static bool appKymeraSetDspClock(audio_dsp_clock_configuration *cconfig, kymera_dsp_clock_speed_config_t * speed_config)
{
    DEBUG_LOG_FN_ENTRY("appKymeraSetDspClock, active_mode enum:audio_dsp_clock_type:%d", cconfig->active_mode);

    bool config_changed = kymera_HasDspClockConfigChanged(cconfig, speed_config);

    if (Kymera_VaSetDspClock(cconfig->active_mode, cconfig->trigger_mode))
    {
        DEBUG_LOG("appKymeraSetDspClock: DSP clock is controlled by VA");
    }
    else if (config_changed)
    {
        PanicFalse(AudioDspClockConfigure(cconfig));
    }
    return config_changed;
}

void appKymeraBoostDspClockToMax(void)
{
    audio_dsp_clock_configuration cconfig =
    {
        .active_mode = MAX_DSP_CLOCK,
        .low_power_mode =  AUDIO_DSP_CLOCK_NO_CHANGE,
        .trigger_mode = MAX_DSP_CLOCK,
    };

    DEBUG_LOG("appKymeraBoostDspClockToMax: enum:audio_dsp_clock_type:%d", MAX_DSP_CLOCK);
    appKymeraProspectiveDspPowerOn(KYMERA_POWER_ACTIVATION_MODE_IMMEDIATE);
    appKymeraSetDspClock(&cconfig, &current_kymera_dsp_clock_config.speed);
}

void appKymeraConfigureDspPowerMode(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    bool tone_playing = appKymeraIsPlayingPrompt();

    kymera_dsp_clock_speed_config_t new_clock_speed_config = {0};
    kymera_MapDspPowerModes(&new_clock_speed_config);

    DEBUG_LOG("appKymeraConfigureDspPowerMode, tone %d, enum:appKymeraState:%u, a2dp active %d seid %u", tone_playing, appKymeraGetState(), Kymera_A2dpIsActive(), theKymera->a2dp_seid);

    /* Assume we are switching to the low power slow clock unless one of the
     * special cases below applies */
    audio_dsp_clock_configuration cconfig =
    {
        .active_mode = AUDIO_DSP_SLOW_CLOCK,
#ifdef ENABLE_GRAPH_MANAGER_CLOCK_CONTROL
        .low_power_mode = AUDIO_DSP_VERY_SLOW_CLOCK,
#else
        .low_power_mode = AUDIO_DSP_SLOW_CLOCK,
#endif
        .trigger_mode = AUDIO_DSP_CLOCK_NO_CHANGE
    };

    audio_dsp_clock kclocks;
    audio_power_save_mode mode = AUDIO_POWER_SAVE_MODE_3;
    if (Kymera_A2dpIsActive() || (appKymeraGetState() == KYMERA_STATE_STANDALONE_LEAKTHROUGH))
    {
        if(KymeraAnc_IsDspClockUpdateRequired())
        {
            audio_dsp_clock_configuration anc_cconfig = { 0 };
            audio_power_save_mode anc_mode = mode;

            anc_cconfig.active_mode = KymeraAnc_GetOptimalDspClockForMusicConcurrency(theKymera->a2dp_seid);
            anc_mode = AUDIO_POWER_SAVE_MODE_1;

            kymera_UpdateConfigWithHighestValuesFromProposedConfig(&cconfig, &anc_cconfig);
            mode = MIN(mode, anc_mode);
        }

        if(Kymera_IsVaActive())
        {
            audio_dsp_clock_configuration va_cconfig = { 0 };
            audio_power_save_mode va_mode = mode;

#if !defined(INCLUDE_DECODERS_ON_P1) || defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
            va_cconfig.active_mode = AUDIO_DSP_TURBO_PLUS_CLOCK;
            va_cconfig.trigger_mode = AUDIO_DSP_TURBO_PLUS_CLOCK;
#else
            va_cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
            va_cconfig.trigger_mode = AUDIO_DSP_TURBO_CLOCK;
#endif

            va_mode = AUDIO_POWER_SAVE_MODE_1;

            kymera_UpdateConfigWithHighestValuesFromProposedConfig(&cconfig, &va_cconfig);
            mode = MIN(mode, va_mode);
        }

        audio_dsp_clock_configuration a2dp_cconfig = { 0 };
        audio_power_save_mode a2dp_mode = mode;

        if(tone_playing)
        {
            mode = AUDIO_POWER_SAVE_MODE_1;
            switch(theKymera->a2dp_seid)
            {
                case AV_SEID_APTX_SNK:
                case AV_SEID_APTXHD_SNK:
                    a2dp_cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                    break;

                case AV_SEID_APTX_ADAPTIVE_SNK:
                case AV_SEID_APTX_ADAPTIVE_TWS_SNK:
                    a2dp_cconfig.active_mode = kymera_GetDSPClockDuringAptxAdaptive();
                    break;

                default:
                    /* For most codecs there is not enough MIPs when running on a slow clock to also play a tone */
                    a2dp_cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                    break;
            }
        }
        else
        {
            switch(theKymera->a2dp_seid)
            {
                /* In Music-ANC concurrency usecase the repetitive a2dp music pause and play causes DAC
                 * endpoint buffer overflow, that in-turn causes MCPS shortage. Workaround is to increase
                 * the audio clock to Turbo when concurrency active. */
                case AV_SEID_APTX_SNK:
                case AV_SEID_APTXHD_SNK:
                {
                    a2dp_cconfig.active_mode = kymera_GetDSPClockDuringAptx();
                    a2dp_mode = AUDIO_POWER_SAVE_MODE_1;
                }
                break;

                case AV_SEID_APTX_ADAPTIVE_SNK:
                case AV_SEID_APTX_ADAPTIVE_TWS_SNK:
                    a2dp_cconfig.active_mode = kymera_GetDSPClockDuringAptxAdaptive();
                    a2dp_mode = AUDIO_POWER_SAVE_MODE_1;
                    break;

                case AV_SEID_SBC_SNK:
#if defined(INCLUDE_MIRRORING) && !defined(INCLUDE_DECODERS_ON_P1) && !defined(ENABLE_TWM_SPEAKER)
                {
                    if (Kymera_OutputIsAecAlwaysUsed())
                    {
                        a2dp_cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                        a2dp_mode = AUDIO_POWER_SAVE_MODE_1;
                    }
                }
#elif defined(INCLUDE_STEREO) && defined(__QCC308X__)
                {
                    a2dp_cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                }
#endif
                break;

                case AV_SEID_AAC_SNK:
#if (!defined(INCLUDE_DECODERS_ON_P1) && defined(INCLUDE_MUSIC_PROCESSING)) || defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
                {
                    a2dp_cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                    a2dp_mode = AUDIO_POWER_SAVE_MODE_1;
                }
#endif
                break;

                default:
                break;
            }
        }

        kymera_UpdateConfigWithHighestValuesFromProposedConfig(&cconfig, &a2dp_cconfig);
        mode = MIN(mode, a2dp_mode);
        
        if (Kymera_BoostClockInGamingMode() && Kymera_LatencyManagerIsGamingModeEnabled())
        {
            cconfig.active_mode += 1;
            cconfig.active_mode = MIN(cconfig.active_mode, MAX_DSP_CLOCK);
        }
    }
    // SCO assumed mutually exclusive to A2DP
    else if (Kymera_ScoIsActive())
    {
        audio_dsp_clock_configuration sco_cconfig = { 0 };
        audio_power_save_mode sco_mode = mode;

        DEBUG_LOG("appKymeraConfigureDspPowerMode, sco_mode enum:appKymeraScoMode:%d", theKymera->sco_info->mode);

        if(KymeraAnc_IsDspClockUpdateRequired())
        {
            sco_cconfig.active_mode = KymeraAnc_GetOptimalDspClockForScoConcurrency(theKymera->sco_info->mode);
            sco_mode = AUDIO_POWER_SAVE_MODE_1;
        }
        else
        {
            switch (theKymera->sco_info->mode)
            {
                case SCO_NB:
                case SCO_WB:
                {
                    sco_cconfig.active_mode = appKymeraGetNbWbScoDspClockType();
                    sco_mode = AUDIO_POWER_SAVE_MODE_1;
                }
                break;

                case SCO_SWB:
#ifdef INCLUDE_SWB_LC3
                case SCO_SWB_LC3:
#endif
                case SCO_UWB:
                {
                    sco_cconfig.active_mode = appKymeraGetSwbScoDspClockType();
                    sco_mode = AUDIO_POWER_SAVE_MODE_1;
                }
                break;

                default:
                break;
            }
        }
        
        if (appConfigHybridCvcSupported())
        {
            sco_cconfig.active_mode = MAX(AUDIO_DSP_TURBO_PLUS_CLOCK, cconfig.active_mode);
        }

        kymera_UpdateConfigWithHighestValuesFromProposedConfig(&cconfig, &sco_cconfig);
        mode = MIN(mode, sco_mode);
    }
    else
    {
        switch (appKymeraGetState())
        {
            case KYMERA_STATE_ANC_TUNING:
            {
                audio_dsp_clock_configuration anc_cconfig = { 0 };
                audio_power_save_mode anc_mode = mode;

                anc_cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                anc_mode = AUDIO_POWER_SAVE_MODE_1;

                kymera_UpdateConfigWithHighestValuesFromProposedConfig(&cconfig, &anc_cconfig);
                mode = MIN(mode, anc_mode);
            }
            break;

            case KYMERA_STATE_MIC_LOOPBACK:
            {
                audio_dsp_clock_configuration anc_cconfig = { 0 };
                audio_power_save_mode anc_mode = mode;

                if(AncStateManager_CheckIfDspClockBoostUpRequired())
                {
                    anc_cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                    anc_mode = AUDIO_POWER_SAVE_MODE_1;
                }
                else if ((appKymeraInConcurrency()) || (FitTest_IsRunning()))
                {
                    anc_cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                    anc_mode = AUDIO_POWER_SAVE_MODE_1;
                }

                kymera_UpdateConfigWithHighestValuesFromProposedConfig(&cconfig, &anc_cconfig);
                mode = MIN(mode, anc_mode);

                if(Kymera_IsVaActive())
                {
                    audio_dsp_clock_configuration va_cconfig = { 0 };
                    audio_power_save_mode va_mode = mode;

                    va_cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                    va_cconfig.trigger_mode = AUDIO_DSP_TURBO_CLOCK;
                    va_mode = AUDIO_POWER_SAVE_MODE_1;

                    kymera_UpdateConfigWithHighestValuesFromProposedConfig(&cconfig, &va_cconfig);
                    mode = MIN(mode, va_mode);
                }
            }
            break;

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
            case KYMERA_STATE_LE_AUDIO_ACTIVE:
            case KYMERA_STATE_LE_VOICE_ACTIVE:
            {
                cconfig.active_mode = kymeraLeAudio_IsMaxDspClockRequired() ? MAX_DSP_CLOCK : AUDIO_DSP_TURBO_CLOCK;
                mode = AUDIO_POWER_SAVE_MODE_1;
            }
            break;
#endif

            case KYMERA_STATE_WIRED_AUDIO_PLAYING:
            case KYMERA_STATE_IDLE:
            case KYMERA_STATE_ADAPTIVE_ANC_STARTED:
            {
                audio_dsp_clock_configuration anc_cconfig = { 0 };
                audio_power_save_mode anc_mode = mode;

                audio_dsp_clock_configuration va_cconfig = { 0 };
                audio_power_save_mode va_mode = mode;

                if(AncStateManager_CheckIfDspClockBoostUpRequired())
                {
                    anc_cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                    anc_mode = AUDIO_POWER_SAVE_MODE_1;

                    if(Kymera_IsVaActive())
                    {
                        va_cconfig.trigger_mode = AUDIO_DSP_TURBO_CLOCK;
                    }
                }
                else if(AncConfig_IsAdvancedAnc())
                {
                    if(appKymeraInConcurrency())
                    {
                        anc_cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                        anc_mode = AUDIO_POWER_SAVE_MODE_1;
                    }
                    else if(KymeraAncCommon_IsAancActive())
                    {
                        anc_cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                        anc_mode = AUDIO_POWER_SAVE_MODE_1;
                    }

                    if(Kymera_IsVaActive())
                    {
                        va_cconfig.active_mode = Kymera_VaGetMinDspClock();
                        va_cconfig.trigger_mode = AUDIO_DSP_TURBO_CLOCK;
                        va_mode = AUDIO_POWER_SAVE_MODE_1;
                    }
                }
                else
                {
                    if(Kymera_IsVaActive())
                    {
                        va_cconfig.active_mode = Kymera_VaGetMinDspClock();
                        va_cconfig.trigger_mode = MAX(AUDIO_DSP_SLOW_CLOCK, cconfig.active_mode);
                        va_mode = AUDIO_POWER_SAVE_MODE_1;
                    }

                    if ((appKymeraInConcurrency()) || (FitTest_IsRunning()))
                    {
                        anc_cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                        anc_mode = AUDIO_POWER_SAVE_MODE_1;
                    }
                }
                
                kymera_UpdateConfigWithHighestValuesFromProposedConfig(&cconfig, &anc_cconfig);
                mode = MIN(mode, anc_mode);

                kymera_UpdateConfigWithHighestValuesFromProposedConfig(&cconfig, &va_cconfig);
                mode = MIN(mode, va_mode);
            }
            break;

            case KYMERA_STATE_USB_AUDIO_ACTIVE:
#if defined(INCLUDE_A2DP_USB_SOURCE)
                cconfig.active_mode = MAX_DSP_CLOCK;
#else
                cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
#endif
                mode = AUDIO_POWER_SAVE_MODE_1;
                break;

            case KYMERA_STATE_USB_VOICE_ACTIVE:
            case KYMERA_STATE_USB_SCO_VOICE_ACTIVE:
                cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                mode = AUDIO_POWER_SAVE_MODE_1;
                break;

            case KYMERA_STATE_WIRED_A2DP_STREAMING:
            case KYMERA_STATE_USB_LE_AUDIO_ACTIVE:
            case KYMERA_STATE_WIRED_LE_AUDIO_ACTIVE:
                cconfig.active_mode = MAX_DSP_CLOCK;
                mode = AUDIO_POWER_SAVE_MODE_1;
                break;

            default:
                break;
        }
    }

    if (tone_playing)
    {
        cconfig.active_mode = MAX(cconfig.active_mode, appKymeraGetMinClockForTonesPrompts());
    }

#ifdef AUDIO_IN_SQIF
    /* Make clock faster when running from SQIF */
    cconfig.active_mode += 1;
#endif

#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
    DEBUG_LOG("appKymeraConfigureDspPowerMode, Clock is always configured for AUDIO_DSP_TURBO_PLUS_CLOCK");
    cconfig.active_mode = AUDIO_DSP_TURBO_PLUS_CLOCK;
#endif

    kymera_UpdateHighestDspClockUsingRegisteredUsersInput(&cconfig, &mode);
    PanicFalse(AudioPowerSaveModeSet(mode));

    if(appKymeraSetDspClock(&cconfig, &new_clock_speed_config))
    {
        PanicFalse(AudioDspGetClock(&kclocks));

        DEBUG_LOG_INFO("appKymeraConfigureDspPowerMode, kymera clocks %d %d %d", kclocks.active_mode, kclocks.low_power_mode, kclocks.trigger_mode);
    }
    else
    {
        DEBUG_LOG_INFO("appKymeraConfigureDspPowerMode, kymera clock config change not required");
    }

    mode = AudioPowerSaveModeGet();
    DEBUG_LOG_INFO("appKymeraConfigureDspPowerMode, mode %d", mode);
}

static void kymera_MapDspPowerModes(kymera_dsp_clock_speed_config_t * new_cpu_speed)
{
    new_cpu_speed->slow_clock_speed_mhz = DEFAULT_LOW_POWER_CLK_SPEED_MHZ;
    new_cpu_speed->very_slow_clock_speed_mhz = DEFAULT_VERY_LOW_POWER_CLK_SPEED_MHZ;

    if (Kymera_IsVaActive())
    {
        Kymera_VaGetMinLpClockSpeedMhz(&new_cpu_speed->slow_clock_speed_mhz, &new_cpu_speed->very_slow_clock_speed_mhz);
    }
    PanicFalse(AudioMapCpuSpeed(AUDIO_DSP_SLOW_CLOCK, new_cpu_speed->slow_clock_speed_mhz * MHZ_TO_HZ));
    PanicFalse(AudioMapCpuSpeed(AUDIO_DSP_VERY_SLOW_CLOCK, new_cpu_speed->very_slow_clock_speed_mhz * MHZ_TO_HZ));
}

static bool kymera_IsInterfaceAlreadyRegistered(const kymera_dsp_clock_user_if_t * const interface)
{
    bool is_already_in_list = FALSE;

    if (kymera_dsp_clock_config_user_ifs)
    {
        LIST_FOREACH(&kymera_dsp_clock_config_user_ifs)
        {
            const kymera_dsp_clock_user_if_t * registered_if = ListGetCurrentReference(&kymera_dsp_clock_config_user_ifs);
            is_already_in_list = (registered_if == interface);
        }
    }

    return is_already_in_list;
}

void appKymeraRegisterDspClockUser(const kymera_dsp_clock_user_if_t * const user_if)
{
    if(!kymera_dsp_clock_config_user_ifs)
    {
        kymera_dsp_clock_config_user_ifs = ListCreate(NULL);
    }

    if(!kymera_IsInterfaceAlreadyRegistered(user_if))
    {
        ListAppend(&kymera_dsp_clock_config_user_ifs, user_if);
    }
}

void appKymeraResetCurrentDspClockConfig(void)
{
    current_kymera_dsp_clock_config.clocks.active_mode = AUDIO_DSP_CLOCK_NO_CHANGE;
    current_kymera_dsp_clock_config.clocks.low_power_mode = AUDIO_DSP_CLOCK_NO_CHANGE;
    current_kymera_dsp_clock_config.clocks.trigger_mode = AUDIO_DSP_CLOCK_NO_CHANGE;
}
