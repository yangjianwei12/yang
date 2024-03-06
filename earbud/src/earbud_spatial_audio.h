/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       earbud_spatial_audio.h
\brief      Provides Spatial (3D) audio support
*/

#ifndef EARBUD_SPATIAL_AUDIO_H
#define EARBUD_SPATIAL_AUDIO_H

#ifdef INCLUDE_SPATIAL_AUDIO

#include "spatial_data.h"

/*! Spatial audio synchronised data commands - for synchronised activities in earbuds during spatial audio.*/
typedef enum
{
    /*! Spatial audio synchronised reset of attitude filter */
    spatial_audio_sync_reset_attitude_filter_command
} earbud_spatial_audio_sync_data_command_t;

/*! \brief Initialise spatial audio functionality

    NOTE: currently this interface is only for demonstrating head tracking as we don't have the complete spatial audio application!

    \param task The init task

    \return TRUE if feature initialisation was successful, otherwise FALSE.
*/
bool EarbudSpatialAudio_Init(Task task);

/*! \brief Enable spatial audio processing

    Only the primary earbud will enable spatial audio. 

    Once the feature is enabled in the primary, it will enable the feature in secondary by sending
    control message over the BT sensor profile in the buddy link.

    NOTE: currently this interface is only for demonstrating head tracking as we don't have the complete spatial audio application!

    \param sensor_sample_rate_hz - Sensor sample rate in Hz
    \param data_report - Requested data report

    \return TRUE if enable is initiated successfully, else returns FALSE.
*/
bool EarbudSpatialAudio_Enable(uint16 sensor_sample_rate_hz, spatial_data_report_id_t data_report);

/*! \brief Get the latest report generated

    Used by external applications to fetch the latest report.

    \return SPATIAL_DATA_REPORT_DATA_IND_T.
*/
SPATIAL_DATA_REPORT_DATA_IND_T *EarbudSpatialAudio_GetLatestReport(void);

/*! \brief Disable spatial audio processing

    Only the primary earbud shall disable spatial audio. 

    Once the feature is disabled in the primary, it shall disable the feature in secondary by sending
    a control message over the BT sensor profile in the buddy link.

    NOTE: currently this interface is only for demonstrating head tracking as we don't have the complete spatial audio application!

    \return TRUE if enable is initiated successfully, else returns FALSE.
*/
bool EarbudSpatialAudio_Disable(void);

/*! \brief Send synchronised data during spatial audio

    This is required to perform synchronised activities in both the earbuds during spatial audio.

    The synchrnoised activity can be triggered in both primary and secondary earbuds.
    
    The activity to perform is passed in the input command parameter. The data to be used while performing
    the synchronised activity is passed in the input data parameter.

    The input data will be send along with the orientation data combined into one ACL packet (fitting in one BT slot) 
    in the next BT sniff subrate instance in the buddy link.

    NOTE: currently this interface is only for demonstrating head tracking as we don't have the complete spatial audio application!

    \param command   Synchronised data command
    \param data_len  Data lenth
    \param data Pointer to the data

    \return Return TRUE if the data is queued for sending, else returns FALSE.
*/
bool EarbudSpatialAudio_SendSynchronisedData(earbud_spatial_audio_sync_data_command_t command, uint16 data_len, const uint8 *data);

#endif /* INCLUDE_SPATIAL_AUDIO */

#endif /* EARBUD_SPATIAL_AUDIO_H */
