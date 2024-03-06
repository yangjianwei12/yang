/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   media_player Media Player
\ingroup    services
\brief      Private functions and helpers for the media_player service.
*/

#ifndef MEDIA_PLAYER_PRIVATE_H_
#define MEDIA_PLAYER_PRIVATE_H_

#include "media_player_typedef.h"

/*! Delay in milliseconds before pausing a source that doesn't get routed */
#define MEDIA_PLAYER_DELAY_BEFORE_PAUSING_UNROUTED_SOURCE   (1500)

/*! Number of internal message groups. Each group contains a message ID for every Audio Source. */
#define MEDIA_PLAYER_INTERNAL_MESSAGE_GROUPS_COUNT   \
        ((MEDIA_PLAYER_INTERNAL_MESSAGES_MAX_SOURCES - MEDIA_PLAYER_INTERNAL_UNROUTED_PAUSE_BASE) / max_audio_sources)

/*! \brief Convert message group to media_player_internal_message id base
*/
#define mediaPlayerConvertInternalMessageGroupToIdBase(group)               (group * max_audio_sources)

/*! \brief Convert audio source to media_player_internal_message id
*/
#define mediaPlayerConvertAudioSourceToInternalMessageId(source, id_base)   (source + id_base)

/*! \brief Convert media_player_internal_message id to audio source
*/
#define mediaPlayerConvertInternalMessageIdToAudioSource(id, id_base)       (id - id_base)

extern media_player_task_data_t media_player_task_data;

/*! Helper macros */
#define mediaPlayer_MediaTaskData()     (&media_player_task_data)
#define mediaPlayer_MediaTask()         (&media_player_task_data.media_task)
#define mediaPlayer_UiTask()            (&media_player_task_data.ui_task)

#endif /* MEDIA_PLAYER_PRIVATE_H_ */
