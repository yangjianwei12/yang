/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_audio_volume
    \brief      Synchronise the LE audio volume from the Primary to the Secondary earbud.
    @{
*/


#ifndef LE_AUDIO_VOLUME_SYNC_H
#define LE_AUDIO_VOLUME_SYNC_H

#if (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)) && defined(INCLUDE_MIRRORING)

/*! \brief The audio volume sync data */
typedef struct
{
    /*! The component's task data */
    TaskData task;

    /*! The current Primary / Secondary role. */
    bool is_primary;

} le_audio_volume_sync_data_t;

extern le_audio_volume_sync_data_t le_audio_volume_sync_data;

#define leAudioVolumeSync_Get() (&le_audio_volume_sync_data)

#define leAudioVolumeSync_GetTask() (&leAudioVolumeSync_Get()->task)


/*! \brief Initialise the LE audio volume synchronisation module.
*/
void leAudioVolumeSync_Init(void);

/*! \brief Set the current role of the LE audio synchronisation

    Only the Primary shall synchronise the volume to the Secondary. This
    function tells the module what role the local device is currently in.

    \param primary TRUE if primary role, False otherwise
*/
void LeAudioVolumeSync_SetRole(bool primary);

#else

#define leAudioVolumeSync_Init() /* Nothing to do */

#define LeAudioVolumeSync_SetRole(primary) UNUSED(primary)

#endif

#endif /* LE_AUDIO_VOLUME_SYNC_H */
/**! @} */