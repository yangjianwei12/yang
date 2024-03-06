/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the feature manager priority list for the application.
*/

#include "app_feature_manager_priority_list.h"


/*! \brief  Priority list for SCO and VA functionality.

    The intention of this list is that when SCO is active VA is disabled.
*/
static const feature_id_t sco_va_priority_order[] =
{
  feature_id_sco,
#ifdef ENABLE_EARBUD_FIT_TEST
  feature_id_fit_test,
#endif
  feature_id_va
};

static const feature_manager_priority_list_t sco_va_priority_list = { sco_va_priority_order, ARRAY_DIM(sco_va_priority_order) };

#ifdef ENABLE_WIRED_AUDIO_FEATURE_PRIORITY
/* We want to add analog audio v/s VA priority because both use the same ADC as source */
static const feature_id_t analog_audio_va_priority_order[] = { feature_id_wired_analog_audio, feature_id_va };
static const feature_manager_priority_list_t analog_audio_va_priority_list = { analog_audio_va_priority_order, ARRAY_DIM(analog_audio_va_priority_order) };
#endif /* ENABLE_WIRED_AUDIO_FEATURE_PRIORITY */

#ifndef INCLUDE_STEREO
#if defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_GAA)
static const feature_id_t gaa_ota_le_audio_priority_order[] =
{
    feature_id_le_broadcast,
    feature_id_gaa_ota
};

static const feature_manager_priority_list_t gaa_ota_le_audio_priority_list = { gaa_ota_le_audio_priority_order, ARRAY_DIM(gaa_ota_le_audio_priority_order) };
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_GAA) */
#endif /* INCLUDE_STEREO */

/* List of lists */
static const feature_manager_priority_list_t * priority_lists[] = {
    &sco_va_priority_list,
#ifdef ENABLE_WIRED_AUDIO_FEATURE_PRIORITY
    &analog_audio_va_priority_list,
#endif /* ENABLE_WIRED_AUDIO_FEATURE_PRIORITY */
#ifndef INCLUDE_STEREO
#if defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_GAA)
    &gaa_ota_le_audio_priority_list,
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_GAA) */
#endif /* INCLUDE_STEREO */
};

static const feature_manager_priority_lists_t feature_manager_priority_lists = { priority_lists, ARRAY_DIM(priority_lists) };

const feature_manager_priority_lists_t * App_GetFeatureManagerPriorityLists(void)
{
    return &feature_manager_priority_lists;
}
