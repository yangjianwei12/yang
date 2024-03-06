/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle LE mic chain

*/

#ifndef KYMERA_LE_MIC_CHAIN_H_
#define KYMERA_LE_MIC_CHAIN_H_

#include "kymera_source_sync.h"
#include "kymera_output_if.h"
#include "kymera_adaptation_audio_protected.h"

#ifdef INCLUDE_LE_AUDIO_UNICAST

/*! \brief Handle request to create LE mic chain.

    \param le_microphone_config_t params to create the mic graph with
    \param is_voice_back_channel is the mic chain for voice back channel in gaming mode
*/
void Kymera_CreateLeMicChain(const le_microphone_config_t *microphone_params, bool is_voice_back_channel);

/*! \brief Start LE mic only chain.
*/
void Kymera_StartLeMicChain(void);

/*! \brief Handle request to stop LE Mic chain.
*/
void Kymera_StopLeMicChain(void);

/*! \brief Initialize the LE Mic chain
*/
void Kymera_LeMicChainInit(void);

/*! \brief Interface to disconnect output chain in LE stereo recording
*/
void Kymera_LeMicDisconnectOutputChain(void);

/*! \brief Returns the LE Audio mic chain where Cvc operator is present
    \return Active LE Audio mic chain chandle
*/
kymera_chain_handle_t Kymera_LeAudioGetCvcChain(void);

/*! \brief Returns whether gaming mode voice back channel is enabled
    \return TRUE if gaming voice back channel is enabled
*/
bool Kymera_IsVoiceBackChannelEnabled(void);

/*! \brief Returns sample rate used for LE-Audio VBC/Stereo-Recording
    \return Current selected sample rate in Hz
*/
unsigned Kymera_LeMicSampleRate(void);

/*! \brief Returns cVc send operator if used for LE-Audio VBC/Stereo-Recording chain
    \return Operator
*/
Operator Kymera_GetLeMicCvcSend(void);

/*! \brief Returns whether codec type used is aptX Lite
    \return TRUE if voice back channel codec type is aptX Lite
*/
bool Kymera_LeMicIsCodecTypeAptxLite(void);

#else /* INCLUDE_LE_AUDIO_UNICAST */

#define Kymera_CreateLeMicChain(microphone_params, is_voice_back_channel)      ((void)(0))
#define Kymera_StartLeMicChain()              ((void)(0))
#define Kymera_StopLeMicChain()               ((void)(0))
#define Kymera_LeMicChainInit()               ((void)(0))
#define Kymera_LeMicDisconnectOutputChain()   ((void)(0))
#define Kymera_IsVoiceBackChannelEnabled()    (FALSE)
#define Kymera_LeMicSampleRate()              (0)
#define Kymera_GetLeMicCvcSend()              (INVALID_OPERATOR)
#define Kymera_LeMicIsCodecTypeAptxLite()     (FALSE)

#endif /* INCLUDE_LE_AUDIO_UNICAST */

#endif /* KYMERA_LE_MIC_CHAIN_H_ */
