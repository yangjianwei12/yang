/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   le_audio_volume LE Audio Volume
    @{
    \ingroup    bt_domain
    \brief      
*/

#ifndef LE_AUDIO_VOLUME_H_
#define LE_AUDIO_VOLUME_H_


#include "source_param_types.h"


/*! \brief Initialises the LE Volume

    Initialises the LE Volume
 */
bool LeAudioVolume_Init(Task init_task);


/*! \brief Get the routed source that accepts volume input from VCP.

    Get the currently routed generic source that will accept volume commands
    from VCP - the LE Audio Volume Control Profile.

    Usually the source will be from a LEA source - for example unicast audio
    or voice or broadcast audio. But on devices that support both LEA and
    BR/EDR audio sources, e.g. A2DP, the volume commands from the remote device
    for A2DP may also be sent using VCP instead of AVRCP.

    If no compatible source is currently routed this function will return FALSE.

    \param generic_source [out] If a compatible source is routed the details
                                will be written into this struct.

    \return TRUE if a compatible source is currently routed, FALSE otherwise.
*/
bool LeAudioVolume_GetRoutedSource(generic_source_t *generic_source);

#endif /* LE_AUDIO_VOLUME_H_ */

/**! @} */