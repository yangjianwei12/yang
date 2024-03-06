/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       earbud_spatial_audio.h
\brief      Provides Spatial (3D) audio support
*/

#ifndef HEADSET_SPATIAL_AUDIO_H
#define HEADSET_SPATIAL_AUDIO_H

#ifdef INCLUDE_SPATIAL_AUDIO

#include "spatial_data.h"

/*! \brief Initialise spatial audio functionality

    \param task The init task

    \return TRUE if feature initialisation was successful, otherwise FALSE.
*/
bool HeadsetSpatialAudio_Init(Task task);

/*! \brief Enable spatial audio processing

    \param sensor_sample_rate_hz - Sensor sample rate in Hz
    \param data_report - Requested data report

    \return TRUE if enable is initiated successfully, else returns FALSE.
*/
bool HeadsetSpatialAudio_Enable(uint16 sensor_sample_rate_hz, spatial_data_report_id_t data_report);

/*! \brief Get the latest report generated

    Used by external applications to fetch the latest report.

    \return SPATIAL_DATA_REPORT_DATA_IND_T.
*/
SPATIAL_DATA_REPORT_DATA_IND_T *HeadsetSpatialAudio_GetLatestReport(void);

/*! \brief Disable spatial audio processing

    \return TRUE if enable is initiated successfully, else returns FALSE.
*/
bool HeadsetSpatialAudio_Disable(void);

#endif /* INCLUDE_SPATIAL_AUDIO */

#endif /* HEADSET_SPATIAL_AUDIO_H */
