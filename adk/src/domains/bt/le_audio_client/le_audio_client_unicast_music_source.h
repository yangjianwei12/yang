/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Internal APIs for LE Audio Client music sources.
*/

#ifndef LE_AUDIO_CLIENT_UNICAST_MUSIC_SOURCE_H_
#define LE_AUDIO_CLIENT_UNICAST_MUSIC_SOURCE_H_

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

void leAudioClient_InitMusicSource(void);

#else /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#define leAudioClient_InitMusicSource()

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#endif /* LE_AUDIO_CLIENT_UNICAST_MUSIC_SOURCE_H_ */

