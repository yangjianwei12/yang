/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    tmap_profile
    \brief      Initializes the MCS server for TMAP
*/

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

#include "tmap_profile_mcs_tbs_private.h"
#include "audio_sources.h"

#define TMAP_MCS_LOG     DEBUG_LOG

static tmap_profile_mcs_server_data_t tmapProfile_mcs_data =
{
    .service_handle = TMAP_INVALID_SERVICE_HANDLE,
    .media_state    = MCS_MEDIA_STATE_INACTIVE,
    .track_duration = INVALID_TRACK_DURATION,
    .track_position = TRACK_POSITION_UNAVAILABLE,
};

/*! \brief Set the media control point opcode */
static bool tmapProfileMcsServer_SetMediaControlPoint(GattMcsMediaControlPointType opcode)
{
    return GattMcsServerSetMediaPlayerAttribute(tmapProfile_mcs_data.service_handle, sizeof(opcode),
                                                &opcode, MCS_SRV_MEDIA_CONTROL_POINT);
}

/*! \brief Sets the current track as given */
static bool tmapProfileMcsServer_SetCurrentTrack(char *track_name, uint8 track_name_len, int32 duration)
{
    tmapProfile_mcs_data.track_duration = duration;
    tmapProfile_mcs_data.track_position = 0;

    return GattMcsServerSetCurrentTrack(tmapProfile_mcs_data.service_handle, track_name_len, track_name, duration);

}

/*! \brief Send response to media control point write operation */
static bool tmapProfileMcsServer_MediaControlPointWriteResponse(ConnectionId cid,
                                                                GattMcsMediaControlPointType opcode,
                                                                GattMediaControlPointAccessResultCode result)
{
    return GattMcsServerMediaControlPointWriteResponse(tmapProfile_mcs_data.service_handle, cid, opcode, result);
}

void tmapProfileMcsServer_Init(Task app_task)
{
    GattMcsInitData mcs_init_data = {0};

    TMAP_MCS_LOG("tmapProfile_McsServerInit");

    /* Initialise with default values */
    mcs_init_data.mediaPlayerNameLen = TMAP_PROFILE_MCS_SERVER_MEDIA_PLAYER_NAME_LEN;
    if (mcs_init_data.mediaPlayerNameLen)
    {
        mcs_init_data.mediaPlayerName = (char*)PanicUnlessMalloc(mcs_init_data.mediaPlayerNameLen);
        memcpy(mcs_init_data.mediaPlayerName, TMAP_PROFILE_MCS_SERVER_MEDIA_PLAYER_NAME,
               mcs_init_data.mediaPlayerNameLen);
    }

    mcs_init_data.trackTitleLen = TMAP_PROFILE_MCS_SERVER_TRACK_TITLE_LEN;
    if (mcs_init_data.trackTitleLen)
    {
        mcs_init_data.trackTitle = (char*)PanicUnlessMalloc(mcs_init_data.trackTitleLen);
        memcpy(mcs_init_data.trackTitle, TMAP_PROFILE_MCS_SERVER_TRACK_TITLE,
               mcs_init_data.trackTitleLen);
    }

    mcs_init_data.playingOrderSupported = TMAP_PROFILE_MCS_SERVER_PLAYING_ORDER_SUPPORTED;
    mcs_init_data.playingOrder = MCS_PO_SINGLE_ONCE;
    mcs_init_data.contentControlId = TMAP_PROFILE_MCS_CCID;
    mcs_init_data.supportedOpcode = TMAP_PROFILE_MCS_SERVER_SUPPORTED_OPCODES;
    mcs_init_data.trackPosition = tmapProfile_mcs_data.track_position;
    mcs_init_data.trackDuration = tmapProfile_mcs_data.track_duration;
    mcs_init_data.playbackSpeed = TMAP_PROFILE_MCS_SERVER_PLAYBACK_SPEED;
    mcs_init_data.seekingSpeed = TMAP_PROFILE_MCS_SERVER_SEEKING_SPEED;
    mcs_init_data.mediaState = MCS_MEDIA_STATE_INACTIVE;
    mcs_init_data.mediaControlPoint = TMAP_PROFILE_MCS_OPCODE_INVALID;

    tmapProfile_mcs_data.service_handle = GattMcsServerInit(TrapToOxygenTask(app_task),
                                                            HANDLE_GENERIC_MEDIA_CONTROL_SERVICE,
                                                            HANDLE_GENERIC_MEDIA_CONTROL_SERVICE_END,
                                                            &mcs_init_data);
    PanicFalse(tmapProfile_mcs_data.service_handle != TMAP_INVALID_SERVICE_HANDLE);
}

void tmapProfileMcsServer_AddConfig(gatt_cid_t cid)
{
    if (tmapProfile_mcs_data.service_handle != TMAP_INVALID_SERVICE_HANDLE)
    {
        /* All notifications are disabled by default */
        GattMcsClientConfigDataType mcs_config = {0};

        GattMcsServerAddConfig(tmapProfile_mcs_data.service_handle, cid, &mcs_config);
    }
}

void tmapProfileMcsServer_RemoveConfig(gatt_cid_t cid)
{
    if (tmapProfile_mcs_data.service_handle != TMAP_INVALID_SERVICE_HANDLE)
    {
        GattMcsClientConfigDataType *config;

        config = GattMcsServerRemoveConfig(tmapProfile_mcs_data.service_handle, cid);
        pfree(config);

        /* Clear the media state as well */
        tmapProfile_mcs_data.media_state = MCS_MEDIA_STATE_INACTIVE;
    }
}

bool tmapProfileMcsServer_SetMediaState(GattMcsMediaStateType new_state)
{
    tmapProfile_mcs_data.media_state = new_state;

    return GattMcsServerSetMediaPlayerAttribute(tmapProfile_mcs_data.service_handle, sizeof(new_state),
                                                &new_state, MCS_SRV_MEDIA_STATE);
}

GattMcsMediaStateType tmapProfileMcsServer_GetMediaState(void)
{
    TMAP_MCS_LOG("tmapProfileMcsServer_GetMediaState %d", tmapProfile_mcs_data.media_state);

    return tmapProfile_mcs_data.media_state;
}

/*! \brief Handles media control point write operations */
static void tmapProfile_HandleMediaControlPointWriteInd(GattMcsControlPointWriteInd *message)
{
    switch (message->opcode)
    {
        case MCS_OPCODE_PLAY:
            AudioSources_Play(audio_source_usb);
            tmapProfileMcsServer_SetMediaControlPoint(MCS_OPCODE_PLAY);
            tmapProfileMcsServer_SetMediaState(MCS_MEDIA_STATE_PLAYING);
            tmapProfileMcsServer_MediaControlPointWriteResponse(message->cid, MCS_OPCODE_PLAY,
                                                                MCS_CONTROL_POINT_ACCESS_SUCCESS);
        break;

        case MCS_OPCODE_PAUSE:
            AudioSources_Pause(audio_source_usb);
            tmapProfileMcsServer_SetMediaControlPoint(MCS_OPCODE_PAUSE);
            tmapProfileMcsServer_SetMediaState(MCS_MEDIA_STATE_PAUSED);
            tmapProfileMcsServer_MediaControlPointWriteResponse(message->cid, MCS_OPCODE_PAUSE,
                                                                MCS_CONTROL_POINT_ACCESS_SUCCESS);
        break;

        case MCS_OPCODE_PREVIOUS_TRACK:
            AudioSources_Back(audio_source_usb);
            tmapProfileMcsServer_SetMediaControlPoint(MCS_OPCODE_PREVIOUS_TRACK);

            if (tmapServer_IsPtsModeEnabled())
            {
                /* In PTS mode, set the track to default track */
                tmapProfileMcsServer_SetCurrentTrack(TMAP_PROFILE_MCS_SERVER_PTS_TRACK_TITLE,
                                                     TMAP_PROFILE_MCS_SERVER_PTS_TRACK_TITLE_LEN,
                                                     TMAP_PROFILE_MCS_SERVER_PTS_TRACK_DURATION);
            }
            tmapProfileMcsServer_MediaControlPointWriteResponse(message->cid, MCS_OPCODE_PREVIOUS_TRACK,
                                                                MCS_CONTROL_POINT_ACCESS_SUCCESS);
        break;

        case MCS_OPCODE_NEXT_TRACK:
            AudioSources_Forward(audio_source_usb);
            tmapProfileMcsServer_SetMediaControlPoint(MCS_OPCODE_NEXT_TRACK);

            if (tmapServer_IsPtsModeEnabled())
            {
                /* In PTS mode, set the track to default track */
                tmapProfileMcsServer_SetCurrentTrack(TMAP_PROFILE_MCS_SERVER_PTS_TRACK_TITLE,
                                                     TMAP_PROFILE_MCS_SERVER_PTS_TRACK_TITLE_LEN,
                                                     TMAP_PROFILE_MCS_SERVER_PTS_TRACK_DURATION);
            }
            tmapProfileMcsServer_MediaControlPointWriteResponse(message->cid, MCS_OPCODE_NEXT_TRACK,
                                                                MCS_CONTROL_POINT_ACCESS_SUCCESS);
        break;

        default:
            TMAP_MCS_LOG("tmapProfile_HandleMediaControlPointWriteInd unhandled opcode:%d", message->opcode);
        break;
    }
}

/*! \brief Handles track position write operation */
static void tmapProfile_HandleTrackPositionWriteInd(GattMcsTrackPositionWriteInd *message)
{
    /* As per spec, if the position value is less than zero, then the current playing position shall be set to the
       offset from the end of the track */
    tmapProfile_mcs_data.track_position = message->newTrackPosition < 0 ? tmapProfile_mcs_data.track_duration + message->newTrackPosition :
                                                                          message->newTrackPosition;

    TMAP_MCS_LOG("tmapProfile_HandleTrackPositionWriteInd new_pos : %d", tmapProfile_mcs_data.track_position);

    GattMcsServerSetAbsoluteTrackPosition(tmapProfile_mcs_data.service_handle, tmapProfile_mcs_data.track_position);
}

/*! \brief Handles messages received from MCS server */
static void tmapProfile_HandleMcsServerMessage(Message message)
{
    GattMcsMessageId mcs_msg_id = *(GattMcsMessageId*)message;

    switch (mcs_msg_id)
    {
        case GATT_MCS_MEDIA_CONTROL_POINT_WRITE_IND:
            tmapProfile_HandleMediaControlPointWriteInd((GattMcsControlPointWriteInd*)message);
        break;

        case GATT_MCS_TRACK_POSITION_WRITE_IND:
            tmapProfile_HandleTrackPositionWriteInd((GattMcsTrackPositionWriteInd*)message);
        break;

        default:
            TMAP_MCS_LOG("tmapProfile_HandleMcsServerMessage unhandled message:%d", mcs_msg_id);
        break;
    }
}

void tmapProfileMcsServer_ProcessMcsServerMessage(Message message)
{
    tmapProfile_HandleMcsServerMessage(message);
}

void tmapProfileMcsServer_EnablePtsMode(bool enable)
{
    if (enable)
    {
        /* Set default track and media state as paused in PTS mode */
        tmapProfileMcsServer_SetMediaState(MCS_MEDIA_STATE_PAUSED);
        tmapProfileMcsServer_SetCurrentTrack(TMAP_PROFILE_MCS_SERVER_PTS_TRACK_TITLE,
                                             TMAP_PROFILE_MCS_SERVER_PTS_TRACK_TITLE_LEN,
                                             TMAP_PROFILE_MCS_SERVER_PTS_TRACK_DURATION);
    }
}
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
