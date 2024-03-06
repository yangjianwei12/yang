/*!
\copyright  Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_config.c
\brief      Configuration for microphones and stubs for Active Noise Cancellation (ANC).
            Static conifguration for ANC FF, FB, HY modes.
*/


#ifdef ENABLE_ANC
#include <logging.h>
#include "anc_config.h"
#include "kymera_config.h"
#include "anc_state_manager.h"

/*! \brief Max ANC Modes that can be configured */
#define ANC_CONFIG_MAX_MODE 10

/*! \brief ANC Configuration data */
typedef struct
{
    bool is_adaptive;        /*If the ANC mode is configured as adaptive ANC, Else Static*/
    bool is_leakthrough;     /*If the ANC mode is configured as LKT/Transparent mode, Else Noise Cancellation*/
    bool is_noise_id_supported;                 /*If noise ID feature is supported for this mode*/
    anc_noise_id_category_t noise_id_category;  /*Whether the mode tunings belong to Category A or B */
} anc_config_data_t;

/*! \brief ANC Config Data 
    This table gives example configuration for use of different ANC modes. 
    The actual mode configuration for a product needs to be defined at the customer end.

    IMPORTANT: IT IS MANDATORY TO UPDATE THIS TABLE IN SYNC WITH THE TUNING OF ANC
    RECOMMENDATION: It is recommended that the modes are configured in sequence.
    appConfigNumOfAncModes() to be updated to the configured number of modes.
*/
#ifdef ENABLE_ADAPTIVE_ANC

#ifdef HAVE_RDP_UI
/*Change the config according to the tuning file for RDP*/
const anc_config_data_t anc_config_data[ANC_CONFIG_MAX_MODE] = 
{
    {ANC_CONFIG_MODE_ADAPTIVE, ANC_CONFIG_MODE_NOISE_CANCELLATION, TRUE, ANC_NOISE_ID_CATEGORY_A},   /*anc_mode_1*/ /* Adaptive EANC */
    {ANC_CONFIG_MODE_ADAPTIVE, ANC_CONFIG_MODE_NOISE_CANCELLATION, TRUE, ANC_NOISE_ID_CATEGORY_B},   /*anc_mode_2*/ /* Adaptive EANC */
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},   /*anc_mode_3*/ /* Static EANC */
#if (defined(HAVE_RDP_HW_18689) || defined(HAVE_RDP_HW_MOTION))
    {ANC_CONFIG_MODE_ADAPTIVE, ANC_CONFIG_MODE_LEAKTHROUGH, FALSE, ANC_NOISE_ID_CATEGORY_NA},      /*anc_mode_4*/ /* Adaptive Transparency EANC */
#else /* HAVE_RDP_HW_YE134 */
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_4*/ /* Static EANC */
#endif
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_LEAKTHROUGH, FALSE, ANC_NOISE_ID_CATEGORY_NA},        /*anc_mode_5*/ /* Static Transparency EANC */
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_6*/ /* Not configured */
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_7*/ /* Custom Preset A */
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_8*/ /* Custom Preset B */
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_9*/ /* Custom Preset C */
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_10*//* Custom Preset D */
};
#elif defined(CORVUS_YD300)
/*Change the config according to the tuning file for Corvus*/
const anc_config_data_t anc_config_data[ANC_CONFIG_MAX_MODE] = 
{
    {ANC_CONFIG_MODE_ADAPTIVE, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_1*/ 
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_LEAKTHROUGH, FALSE, ANC_NOISE_ID_CATEGORY_NA},          /*anc_mode_2*/ 
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},   /*anc_mode_3*/ 
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},   /*anc_mode_4*/ 
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},   /*anc_mode_5*/ 
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},   /*anc_mode_6*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},   /*anc_mode_7*/ 
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},   /*anc_mode_8*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},   /*anc_mode_9*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},   /*anc_mode_10*/
};
#else
/*Change the config according to the tuning file for the selected platform*/
const anc_config_data_t anc_config_data[ANC_CONFIG_MAX_MODE] = 
{
    {ANC_CONFIG_MODE_ADAPTIVE, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},/*anc_mode_1*/  /*Adaptive ANC*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA},  /*anc_mode_2*/  /*Static hybrid mode*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_3*/  /*Static hybrid mode*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_4*/  /*Static hybrid mode*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_5*/  /*Static hybrid mode*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_6*/  /*Static hybrid mode*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_7*/  /*Static hybrid mode*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_8*/  /*Static hybrid mode*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_9*/  /*Static hybrid mode*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_10*/ /*Static hybrid mode*/
};
#endif

#else /*Static ANC build*/
/*Change the config according to the tuning file for static ANC*/
const anc_config_data_t anc_config_data[ANC_CONFIG_MAX_MODE] = 
{
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_1*/  /*Static hybrid mode, passthrough configuration*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_2*/  /*Static hybrid mode, passthrough configuration*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_3*/  /*Static hybrid mode, passthrough configuration*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_4*/  /*Static hybrid mode, passthrough configuration*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_5*/  /*Static hybrid mode, passthrough configuration*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_6*/  /*Static hybrid mode, passthrough configuration*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_7*/  /*Static hybrid mode, passthrough configuration*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_8*/  /*Static hybrid mode, passthrough configuration*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_9*/  /*Static hybrid mode, passthrough configuration*/
    {ANC_CONFIG_MODE_STATIC, ANC_CONFIG_MODE_NOISE_CANCELLATION, FALSE, ANC_NOISE_ID_CATEGORY_NA}, /*anc_mode_10*/ /*Static hybrid mode, passthrough configuration*/
};
#endif


/*
 * There is no config manager setup yet, so hard-code the default value as Feed Forward Mode on Analog Mic from
 * Kymera_config for reference.
 */
#ifdef INCLUDE_STEREO
#define getFeedForwardLeftMicConfig()   (appConfigAncFeedForwardLeftMic())
#define getFeedForwardRightMicConfig()  (appConfigAncFeedForwardRightMic())
#define getFeedBackLeftMicConfig()      (appConfigAncFeedBackLeftMic())
#define getFeedBackRightMicConfig()     (appConfigAncFeedBackRightMic())
#else
#define getFeedForwardLeftMicConfig()   (appConfigAncFeedForwardMic())
#define getFeedForwardRightMicConfig()  (MICROPHONE_NONE)
#define getFeedBackLeftMicConfig()      (appConfigAncFeedBackMic())
#define getFeedBackRightMicConfig()     (MICROPHONE_NONE)
#endif

anc_readonly_config_def_t anc_readonly_config =
{
    .anc_mic_params_r_config = {
        .feed_forward_left_mic = getFeedForwardLeftMicConfig(),
        .feed_forward_right_mic = getFeedForwardRightMicConfig(),
        .feed_back_left_mic = getFeedBackLeftMicConfig(),
        .feed_back_right_mic = getFeedBackRightMicConfig(),
     },
     .num_anc_modes = appConfigNumOfAncModes(),
};

/* Write to persistance is not enabled for now and set to defaults */
static anc_writeable_config_def_t anc_writeable_config = {

    .persist_initial_mode = appConfigAncMode(),
    .persist_initial_state = anc_state_manager_uninitialised,
    .initial_anc_state = anc_state_manager_uninitialised,
    .initial_anc_mode = appConfigAncMode(),
};


uint16 ancConfigManagerGetReadOnlyConfig(uint16 config_id, const void **data)
{
    UNUSED(config_id);
    *data = &anc_readonly_config;
    DEBUG_LOG("ancConfigManagerGetReadOnlyConfig\n");
    return (uint16) sizeof(anc_readonly_config);
}

void ancConfigManagerReleaseConfig(uint16 config_id)
{
    UNUSED(config_id);
    DEBUG_LOG("ancConfigManagerReleaseConfig\n");
}

uint16 ancConfigManagerGetWriteableConfig(uint16 config_id, void **data, uint16 size)
{
    UNUSED(config_id);
    UNUSED(size);
    *data = &anc_writeable_config;
    DEBUG_LOG("ancConfigManagerGetWriteableConfig\n");
    return (uint16) sizeof(anc_writeable_config);
}

void ancConfigManagerUpdateWriteableConfig(uint16 config_id)
{
    UNUSED(config_id);
    DEBUG_LOG("ancConfigManagerUpdateWriteableConfig\n");
}

bool AncConfig_IsAncModeAdaptive(anc_mode_t anc_mode)
{
    if (anc_mode >= appConfigNumOfAncModes())
    {
        return FALSE;
    }
    return anc_config_data[anc_mode].is_adaptive;
}

bool AncConfig_IsAncModeLeakThrough(anc_mode_t anc_mode)
{
    if (anc_mode >= appConfigNumOfAncModes())
    {
        return FALSE;
    }
    return anc_config_data[anc_mode].is_leakthrough;
}

bool AncConfig_IsAncModeStatic(anc_mode_t anc_mode)
{
    if (anc_mode >= appConfigNumOfAncModes())
    {
        return FALSE;
    }
    return (!anc_config_data[anc_mode].is_adaptive);
}

bool AncConfig_IsAdvancedAnc(void)
{
    return (ancConfigAncAdvanced());
}

bool AncConfig_IsAncModeAdaptiveLeakThrough(anc_mode_t anc_mode)
{
    return (AncConfig_IsAncModeAdaptive(anc_mode) && AncConfig_IsAncModeLeakThrough(anc_mode));
}

bool AncConfig_IsAncModeStaticLeakThrough(anc_mode_t anc_mode)
{
    return (AncConfig_IsAncModeStatic(anc_mode) && AncConfig_IsAncModeLeakThrough(anc_mode));
}

bool AncConfig_IsAncModeAdaptiveLeakThroughConfigured(void)
{
    for(uint8 anc_mode = anc_mode_1; anc_mode < appConfigNumOfAncModes(); anc_mode++)
    {
        if((anc_config_data[anc_mode].is_adaptive) && (anc_config_data[anc_mode].is_leakthrough))
        {
            return TRUE;
        }
    }
    return FALSE;
}

bool AncConfig_IsNoiseIdSupportedForMode(anc_mode_t anc_mode)
{
    if (anc_mode >= appConfigNumOfAncModes())
    {
        return FALSE;
    }
    
    return anc_config_data[anc_mode].is_noise_id_supported;
}

anc_noise_id_category_t AncConfig_GetNoiseIdCategoryForMode(anc_mode_t anc_mode)
{
    if (anc_mode >= appConfigNumOfAncModes())
    {
        return ANC_NOISE_ID_CATEGORY_NA;
    }
    
    return anc_config_data[anc_mode].noise_id_category;
}

uint8 AncConfig_GetAncModeForNoiseCategoryType(bool is_adaptive, anc_noise_id_category_t noise_category)
{
    for(uint8 anc_mode = anc_mode_1; anc_mode < appConfigNumOfAncModes(); anc_mode++)
    {
        if((anc_config_data[anc_mode].is_adaptive == is_adaptive) && 
          (anc_config_data[anc_mode].is_noise_id_supported) &&
          (anc_config_data[anc_mode].noise_id_category == noise_category))
        {
            return anc_mode;
        }
    }
    return ANC_INVALID_MODE;
}

static bool AncConfig_ValidNoiseIdConfig(bool is_adaptive)
{
    bool valid_config = FALSE;
    uint8 count_cat_a = 0;
    uint8 count_cat_b = 0;
    
    for(uint8 anc_mode = anc_mode_1; anc_mode < appConfigNumOfAncModes(); anc_mode++)
    {
        if((anc_config_data[anc_mode].is_adaptive == is_adaptive) &&
          (anc_config_data[anc_mode].is_noise_id_supported))
        {
            if (anc_config_data[anc_mode].noise_id_category == ANC_NOISE_ID_CATEGORY_A)
            {
                count_cat_a++;
            }
            else if (anc_config_data[anc_mode].noise_id_category == ANC_NOISE_ID_CATEGORY_B)               
            {
                count_cat_b++;
            }
        }
    }
    
    if ((count_cat_a >= 1) && (count_cat_b >=1)) /*atleast one modes with each categories*/
    {
        valid_config = TRUE;
    }

    return valid_config;
}

bool AncConfig_ValidNoiseIdConfiguration(void)
{   
    /*Check for valid noise ID configs for Adaptive and static modes*/
    return (AncConfig_ValidNoiseIdConfig(TRUE) != AncConfig_ValidNoiseIdConfig(FALSE));
}



#endif /* ENABLE_ANC */
