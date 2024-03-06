/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Internal APIs for LE Voice Client sources.
*/

#ifndef LE_AUDIO_CLIENT_UNICAST_VOICE_SOURCE_H_
#define LE_AUDIO_CLIENT_UNICAST_VOICE_SOURCE_H_

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

void leAudioClient_InitVoiceSource(void);

#else

#define leAudioClient_InitVoiceSource()

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#endif /* LE_AUDIO_CLIENT_UNICAST_VOICE_SOURCE_H_ */

