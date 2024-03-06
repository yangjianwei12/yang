/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_audio_volume
    \brief      Definitions private to the le_audio_volume module.
    @{
*/

#ifndef LE_AUDIO_VOLUME_PRIVATE
#define LE_AUDIO_VOLUME_PRIVATE

/* Minimum volume level for LE audio */
#define LE_AUDIO_VOLUME_MIN         0

/* Maximum volume level for LE audio */
#define LE_AUDIO_VOLUME_MAX         255

/* Number of steps between minimum and maximum volume */
#define LE_AUDIO_VOLUME_STEPS       256

/* Config values for a LE audio volume_t */
#define LE_AUDIO_VOLUME_CONFIG      { .range = { .min = LE_AUDIO_VOLUME_MIN, .max = LE_AUDIO_VOLUME_MAX }, .number_of_steps = LE_AUDIO_VOLUME_STEPS }

/* Macro to initialise a volume_t struct to the given raw volume value */
#define LE_AUDIO_VOLUME(step)       { .config = LE_AUDIO_VOLUME_CONFIG, .value = step }

/* Default LE Audio volume */
#define LE_AUDIO_VOLUME_DEFAULT     200

/* Step size of VCP volume changes exposed by the VCS server. */
#define LE_AUDIO_VOLUME_STEP_SIZE   16

/* Check if the audio source is a LEA source that uses VCP to control the volume. */
#define LeAudioVolume_IsValidLeMusicSource(source) \
    (   ((source) == audio_source_le_audio_unicast_1) \
     || ((source) == audio_source_le_audio_broadcast) )


/* Check if the audio source is non-LEA but can use VCP to control the volume */
#define LeAudioVolume_IsValidOtherMusicSource(source) \
    (   ((source) == audio_source_a2dp_1) \
     || ((source) == audio_source_a2dp_2))

/* Check if the voice source is a LEA source that uses VCP to control the volume. */
#define LeAudioVolume_IsValidLeVoiceSource(source)  \
    ((source) == voice_source_le_audio_unicast_1)

#endif /* LE_AUDIO_VOLUME_PRIVATE */
/**! @} */