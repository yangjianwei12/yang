/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief  Handle the LE Audio Client volume.
*/

#ifndef LE_AUDIO_CLIENT_VOLUME_H_
#define LE_AUDIO_CLIENT_VOLUME_H_

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)

/*! \brief Initialises the LE Audio client Volume

    Initialises the LE Audio client Volume
 */
void LeAudioClientVolume_Init(void);

#else /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) */

#define LeAudioClientVolume_Init()

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) */

#endif /* LE_AUDIO_CLIENT_VOLUME_H_ */

