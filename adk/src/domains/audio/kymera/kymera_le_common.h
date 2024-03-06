/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle functions common to LE Audio
*/

#ifndef KYMERA_LE_COMMON_H_
#define KYMERA_LE_COMMON_H_

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "le_audio_messages.h"

/* CVC algorithmic delay compensation for 16/32kHz (WB/SWB) is 7.5ms 
   For 24kHz (UWB), CVC algorithmic delay is 5ms*/
#define LE_AUDIO_WB_CVC_ALGORITHMIC_DELAY_US 7500ul
#define LE_AUDIO_UWB_CVC_ALGORITHMIC_DELAY_US 5000ul

/*! \brief Indicates when DSP clock boost is required for certain LEA use-cases
    \return TRUE if DSP clock has to be boosted
*/
bool kymeraLeAudio_IsMaxDspClockRequired(void);

/*! \brief Set the Mic Mute state.
*/
void KymeraLeAudioVoice_SetMicMuteState(bool mute);

#else

#define kymeraLeAudio_IsMaxDspClockRequired()     (FALSE)
#define KymeraLeAudioVoice_SetMicMuteState()      (FALSE)
#endif

#endif /* KYMERA_LE_COMMON_H_ */
