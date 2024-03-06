/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      Media Control implementation for the LE broadcast audio source.
*/

#include "le_broadcast_media_control.h"
#include "le_broadcast_manager_source.h"
#include "le_broadcast_manager.h"

#include <audio_sources.h>

#include <logging.h>


#if defined(INCLUDE_LE_AUDIO_BROADCAST)

static unsigned leBroadcastManager_MediaControlGetContext(audio_source_t source);
static void leBroadcastManager_MediaControlStop(audio_source_t source);
static void leBroadcastManager_MediaControlPlayPause(audio_source_t source);
static void leBroadcastManager_MediaControlPlay(audio_source_t source);
static void leBroadcastManager_MediaControlPause(audio_source_t source);
static void leBroadcastManager_MediaControlForward(audio_source_t source);
static void leBroadcastManager_MediaControlBack(audio_source_t source);

static const media_control_interface_t media_control_interface = {
    .Play = leBroadcastManager_MediaControlPlay,
    .Pause = leBroadcastManager_MediaControlPause,
    .PlayPause = leBroadcastManager_MediaControlPlayPause,
    .Stop = leBroadcastManager_MediaControlStop,
    .Forward = leBroadcastManager_MediaControlForward,
    .Back = leBroadcastManager_MediaControlBack,
    .FastForward = NULL,
    .FastRewind = NULL,
    .NextGroup = NULL,
    .PreviousGroup = NULL,
    .Shuffle = NULL,
    .Repeat = NULL,
    .Context = leBroadcastManager_MediaControlGetContext,
    .Device = NULL,
};

static unsigned leBroadcastManager_MediaControlGetContext(audio_source_t source)
{
    audio_source_provider_context_t context = context_audio_disconnected;

    if (source == audio_source_le_audio_broadcast)
    {
        if(LeBroadcastManager_IsPaused())
        {
            context = context_audio_is_paused; 
        }
        else
        {
            if (LeBroadcastManager_SourceIsBisSync())
            {
                broadcast_source_state_t * active_bis = LeBroadcastManager_GetSourceOfActiveBis();
                if(active_bis)
                {
                    switch(active_bis->streaming_audio_context)
                    {
                        case audio_context_type_instructional:
                        case audio_context_type_voice_assistant:
                        case audio_context_type_notifications:
                        case audio_context_type_alerts:
                        case audio_context_type_emergency_alarm:
#if defined(ENABLE_GBSS_RDP_DEMO)
                        case audio_context_type_media:
#endif
                            context = context_audio_is_high_priority;
                            break;

                        default:
                            context = context_audio_is_broadcast;
                            break;
                    }
                }
            }
            else if (LeBroadcastManager_IsAnySourceSyncedToPa())
            {
                context = context_audio_connected;
            }
            else if (leBroadcastManager_GetTargetBisSourceId() != SCAN_DELEGATOR_SOURCE_ID_INVALID)
            {
                context = context_audio_connected;
            }
        }
    }

    DEBUG_LOG("leBroadcastManager_MediaControlGetContext context = enum:audio_source_provider_context_t:%d", context);

    return (unsigned)context;
}

static void leBroadcastManager_MediaControlStop(audio_source_t source)
{
    DEBUG_LOG("leBroadcastManager_MediaControlStop");

    if(source == audio_source_le_audio_broadcast)
    {
        LeBroadcastManager_Unsync();
    }
}

static void leBroadcastManager_MediaControlPlayPause(audio_source_t source)
{
    DEBUG_LOG("leBroadcastManager_MediaControlPlayPause");

    if(source == audio_source_le_audio_broadcast)
    {
        if(LeBroadcastManager_IsPaused())
        {
            LeBroadcastManager_Resume(LeBroadcastManager_SourceGetTask());
        }
        else
        {
            LeBroadcastManager_Pause(LeBroadcastManager_SourceGetTask());
        }
    }
}

static void leBroadcastManager_MediaControlPlay(audio_source_t source)
{
    DEBUG_LOG("leBroadcastManager_MediaControlPlay");

    if(source == audio_source_le_audio_broadcast)
    {
        LeBroadcastManager_Resume(LeBroadcastManager_SourceGetTask());
    }
}

static void leBroadcastManager_MediaControlPause(audio_source_t source)
{
    DEBUG_LOG("leBroadcastManager_MediaControlPause");

    if(source == audio_source_le_audio_broadcast)
    {
        LeBroadcastManager_Pause(LeBroadcastManager_SourceGetTask());
    }
}

/*! \brief Get the next broadcast source in the BASS server given a current source.

    \param[in] current_source The current source to start from.

    \return The next valid source. If there is no other valid source it will be set to NULL.
*/
static broadcast_source_state_t *leBroadcastManager_SourceGetNextSource(broadcast_source_state_t *current_source)
{
    broadcast_source_state_t *next_source = NULL;
    broadcast_source_state_t *bss = NULL;

    ARRAY_FOREACH(bss, le_broadcast_manager.broadcast_source_receive_state)
    {
        if (   (bss != current_source)
            && leBroadcastManager_IsSourceValid(bss))
        {
            /* This logic assumes that there are only two possible
               broadcast sources. There can only be one other possible
               source, so if that is valid select that one. */
            next_source = bss;
            break;
        }
    }

    DEBUG_LOG("leBroadcastManager_SourceGetNextSource current source_id 0x%x next source_id 0x%x",
              current_source->source_id,
              next_source ? next_source->source_id : SCAN_DELEGATOR_SOURCE_ID_INVALID);

    return next_source;
}

/*! \brief Get the previous broadcast source in the BASS server given a current source.

    \param[in] current_source The current source to start from.

    \return The previous valid source. If there is no other valid source it will be set to NULL.
*/
static broadcast_source_state_t *leBroadcastManager_SourceGetPreviousSource(broadcast_source_state_t *current_source)
{
    broadcast_source_state_t *next_source = NULL;
    broadcast_source_state_t *bss = NULL;

    ARRAY_FOREACH(bss, le_broadcast_manager.broadcast_source_receive_state)
    {
        if (   (bss != current_source)
            && leBroadcastManager_IsSourceValid(bss))
        {
            /* This logic assumes that there are only two possible
               broadcast sources. There can only be one other possible
               source, so if that is valid select that one. */
            next_source = bss;
            break;
        }
    }

    return next_source;
}

static void leBroadcastManager_MediaControlForward(audio_source_t source)
{
    if (source == audio_source_le_audio_broadcast)
    {
        /* Try to sync to the next source if there is an active broadcast source. */
        broadcast_source_state_t *active_bss = LeBroadcastManager_GetSourceOfActiveBis();

        if (!active_bss)
        {
            active_bss = LeBroadcastManager_GetSourceById(leBroadcastManager_GetTargetBisSourceId());
        }

        DEBUG_LOG("leBroadcastManager_MediaControlForward active_bss source_id 0x%x",
                  active_bss ? active_bss->source_id : SCAN_DELEGATOR_SOURCE_ID_INVALID);

        if (active_bss)
        {
            broadcast_source_state_t *next_bss = leBroadcastManager_SourceGetNextSource(active_bss);
            if (next_bss)
            {
                LeBroadcastManager_SwitchToBroadcastSource(next_bss);
            }
        }
    }
}

static void leBroadcastManager_MediaControlBack(audio_source_t source)
{
    if (source == audio_source_le_audio_broadcast)
    {
        /* Try to sync to the next source if there is an active broadcast source. */
        broadcast_source_state_t *active_bss = LeBroadcastManager_GetSourceOfActiveBis();

        if (!active_bss)
        {
            active_bss = LeBroadcastManager_GetSourceById(leBroadcastManager_GetTargetBisSourceId());
        }

        DEBUG_LOG("leBroadcastManager_MediaControlBack active_bss source_id 0x%x",
                  active_bss ? active_bss->source_id : SCAN_DELEGATOR_SOURCE_ID_INVALID);

        if (active_bss)
        {
            broadcast_source_state_t *prev_bss = leBroadcastManager_SourceGetPreviousSource(active_bss);
            if (prev_bss)
            {
                LeBroadcastManager_SwitchToBroadcastSource(prev_bss);

            }
        }
    }
}

const media_control_interface_t * leBroadcastManager_MediaControlGetInterface(void)
{
    return &media_control_interface;
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) */
