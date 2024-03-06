/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera header for LE Voice
*/

#ifndef KYMERA_LE_VOICE_H_
#define KYMERA_LE_VOICE_H_

#ifdef INCLUDE_LE_AUDIO_UNICAST

#include <kymera_adaptation_voice_protected.h>
#include <chain.h>

/*! \brief The KYMERA_INTERNAL_LE_VOICE_START message content. */
typedef struct
{
    bool microphone_present;
    bool speaker_present;
    /*! The starting volume. */
    int16 volume_in_db;
    bool reconfig;
    /*! LE voice connect parameters */
    le_speaker_config_t speaker_params;
    le_microphone_config_t microphone_params;
} KYMERA_INTERNAL_LE_VOICE_START_T;

/*! \brief The KYMERA_INTERNAL_LE_VOICE_SET_VOLUME message content. */
typedef struct
{
    /*! The volume to set. */
    int16 volume_in_db;
} KYMERA_INTERNAL_LE_VOICE_SET_VOLUME_T;

/*! \brief Handle request to start LE Voice.

    \param start_params LE Voice configuration parameters
*/
void KymeraLeVoice_HandleInternalStart(const KYMERA_INTERNAL_LE_VOICE_START_T * start_params);

/*! \brief Handle request to stop LE Voice.
*/
void KymeraLeVoice_HandleInternalStop(void);

void KymeraLeVoice_HandleInternalSetVolume(int16 volume_in_db);

void kymeraLeVoice_HandleInternalMicMute(bool mute);

void KymeraLeVoice_Init(void);

/*! \brief Returns the LE Voice chain where Cvc operator is present
    \return Active LE Voice chain chandle
*/
kymera_chain_handle_t Kymera_LeVoiceGetCvcChain(void);

/*! \brief Check if LE voice chain is using split stereo configuration
    \return TRUE if voice chain is split stereo
            FALSE otherwise
*/
bool Kymera_IsLeVoiceSplitStereoChain(void);

#else /* INCLUDE_LE_AUDIO_UNICAST */

#define Kymera_IsLeVoiceSplitStereoChain()       (FALSE)

#endif /* INCLUDE_LE_AUDIO_UNICAST */

#endif // KYMERA_LE_VOICE_H_
