/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup mirror_profile
    @{
    \brief      The LE voice source interface implementation for Mirror Profile
*/

#ifndef MIRROR_PROFILE_LE_VOICE_SOURCE_H_
#define MIRROR_PROFILE_LE_VOICE_SOURCE_H_

#include "voice_sources_audio_interface.h"
#include "voice_sources_telephony_control_interface.h"

#ifdef ENABLE_LEA_CIS_DELEGATION

/*! \brief Gets the mirror profile LE voice interface.

    \return The voice source interface for mirror profile
 */
const voice_source_audio_interface_t * MirrorProfile_GetLeVoiceInterface(void);

/*! \brief Gets the mirror profile LE telephony control interface.

    \return The interface.
 */
const voice_source_telephony_control_interface_t *MirrorProfile_GetLeTelephonyControlInterface(void);

/*! \brief Starts the LE voice call

    This is only expected to be called on the Secondary.
*/
void MirrorProfile_StartLeVoice(void);

/*! \brief Stops the LE Voice call

    This is only expected to be called on the Secondary.
*/
void MirrorProfile_StopLeVoice(void);

/*! \brief Function that evaluates and reconfigures LE Voice graph.
*/
void MirrorProfile_ReconfigureLeVoiceGraph(void);

#endif /* ENABLE_LEA_CIS_DELEGATION */

#endif /* MIRROR_PROFILE_LE_VOICE_SOURCE_H_ */
/*! @} */