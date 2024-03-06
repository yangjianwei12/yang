/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_unicast_manager
    \brief      Implementation of the voice source interface for LE Voice.
    @{
*/

#ifndef LE_UNICAST_VOICE_SOURCE_H
#define LE_UNICAST_VOICE_SOURCE_H

#include "voice_sources_audio_interface.h"
#include "le_unicast_manager_instance.h"

/*! \brief Get the audio voice source interfaces for LE unicast voice

    \return voice_source_audio_interface_t Pointer to the voice audio source interfaces 
     for unicast voice source.
*/
const voice_source_audio_interface_t * LeUnicastVoiceSource_GetAudioInterface(void);

/*! \brief Initializes the LE Unicast voice source Module.
           Setting up the needed registerations with the voice sources and 
           the telephony service for voice call controls
*/
void LeUnicastVoiceSource_Init(void);

/*! \brief Reconfigures the voice with speaker only/mic only paths.

    \param inst  The unicast manager instance to reconfigure the voice chain for.
*/
void LeUnicastVoiceSource_Reconfig(le_um_instance_t *inst);

#endif /* LE_UNICAST_VOICE_SOURCE_H */
/*! @} */