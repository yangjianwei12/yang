/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Internal APIs for LE Audio Client Broadcast audio sources.
*/

#ifndef LE_AUDIO_CLIENT_BROADCAST_AUDIO_SOURCE_H_
#define LE_AUDIO_CLIENT_BROADCAST_AUDIO_SOURCE_H_

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

void leAudioClient_InitBroadcastAudioSource(void);

#else /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#define leAudioClient_InitBroadcastAudioSource()

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#endif /* LE_AUDIO_CLIENT_BROADCAST_AUDIO_SOURCE_H_ */

