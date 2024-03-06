/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera header for LE Audio
*/

#ifndef KYMERA_LE_AUDIO_H_
#define KYMERA_LE_AUDIO_H_

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)

#include <kymera_adaptation_audio_protected.h>
#include <kymera_data.h>

/*! \brief The KYMERA_INTERNAL_LE_AUDIO_START_T message content. */
typedef struct
{
    bool media_present;
    bool microphone_present;
    bool reconfig;
    int16 volume_in_db;
    le_media_config_t media_params;
    le_microphone_config_t microphone_params;
} KYMERA_INTERNAL_LE_AUDIO_START_T;

/*! \brief The KYMERA_INTERNAL_LE_AUDIO_SET_VOLUME message content. */
typedef struct
{
    /*! The volume to set. */
    int16 volume_in_db;
} KYMERA_INTERNAL_LE_AUDIO_SET_VOLUME_T;

typedef struct
{
    rtime_t unmute_time;
} KYMERA_INTERNAL_LE_AUDIO_UNMUTE_T;

/*! \brief Handle request to start LE Audio.

    \param KYMERA_INTERNAL_LE_AUDIO_START_T params to configure the audio graph with
*/
void kymeraLeAudio_Start(const KYMERA_INTERNAL_LE_AUDIO_START_T * start_params);

/*! \brief Handle request to stop LE Audio.
*/
void kymeraLeAudio_Stop(void);

void kymeraLeAudio_SetVolume(int16 volume_in_db);

/*! \brief Checks if audio chain running has aptX adaptive decoder.
    \return TRUE if audio chain running has aptX adaptive decoder, else FALSE
*/
bool KymeraLeAudio_IsAptxAdaptiveStreaming(void);

/*! \brief Fetch QSS lossless data.
    \param lossless_data
    \return TRUE if fetch was was successful, else FALSE
*/
bool KymeraLeAudio_GetAptxAdaptiveLossLessInfo(uint32 *lossless_data);

#ifdef INCLUDE_MIRRORING
void KymeraLeAudio_HandleAudioUnmuteInd(const KYMERA_INTERNAL_LE_AUDIO_UNMUTE_T *unmute_params);
void KymeraLeAudio_ConfigureStartSync(kymeraTaskData *theKymera, bool start_muted);
#endif

void kymeraLeAudio_Init(void);
#endif

#else

#define KymeraLeAudio_IsAptxAdaptiveStreaming() (FALSE)

#define KymeraLeAudio_GetAptxAdaptiveLossLessInfo(lossless_data) UNUSED(lossless_data)

#endif // KYMERA_LE_AUDIO_H_
