/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup mirror_profile
    @{
    \brief      Mirror profile LE audio source control.
*/

#ifndef MIRROR_PROFILE_LE_AUDIO_SOURCE_H_
#define MIRROR_PROFILE_LE_AUDIO_SOURCE_H_

#include "audio_sources.h"
#include "audio_sources_audio_interface.h"
#include "source_param_types.h"

#ifdef ENABLE_LEA_CIS_DELEGATION
/*! \brief Gets the mirror LE audio interface.

    \return The audio source audio interface for a mirror LE audio source
 */
const audio_source_audio_interface_t *MirrorProfile_GetLeAudioInterface(void);

/*! \brief Gets the mirror media control interface.

    \return The interface.
 */
const media_control_interface_t *MirrorProfile_GetLeMediaControlInterface(void);

/*! \brief Start audio for LE Audio unicast source. */
void MirrorProfile_StartLeAudio(void);

/*! \brief Stop audio for LE Audio Unicast source  */
void MirrorProfile_StopLeAudio(void);

/*! \brief Function that evaluates and reconfigures LE Audio graph.
*/
void MirrorProfile_ReconfigureLeaAudioGraph(void);

#endif /* ENABLE_LEA_CIS_DELEGATION */

#endif /* MIRROR_PROFILE_LE_AUDIO_SOURCE_H_ */
/*! @} */