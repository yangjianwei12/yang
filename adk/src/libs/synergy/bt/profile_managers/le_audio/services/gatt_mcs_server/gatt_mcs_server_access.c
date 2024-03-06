/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_mcs_server_access.h"
#include "gatt_mcs_server_debug.h"
#include "gatt_mcs_server_private.h"


#define MCS_MEDIA_CONTROL_POINT_RESPONSE_SIZE (2)
/***************************************************************************
NAME
    mcsHandleMediaPlayerNameAccess

DESCRIPTION
    Deals with access of the HANDLE_MEDIA_PLAYER_NAME handle.
*/

static void mcsHandleMediaPlayerNameAccess(
        GMCS_T *const mcs,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd,
        uint16 maxRespValueLen)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        uint16 sizeValue = 0;
        if (((mcs->readLongProcData.attValChanged & GATT_MCS_SERVER_MEDIA_PLAYER_NAME_CHANGED) == GATT_MCS_SERVER_MEDIA_PLAYER_NAME_CHANGED) && (accessInd->offset != 0))
        {
            sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                ATT_RESULT_APP_MASK
                );
            return;
        }
        else if (maxRespValueLen < (mcs->data.mediaPlayerNameLen - accessInd->offset))
        {
            sizeValue = maxRespValueLen;
        }
        else
        {
            sizeValue = (mcs->data.mediaPlayerNameLen - accessInd->offset);
        }

        mcs->readLongProcData.attValChanged &= (~GATT_MCS_SERVER_MEDIA_PLAYER_NAME_CHANGED);

        sendMcsServerAccessRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeValue,
                (uint8*)(mcs->data.mediaPlayerName + accessInd->offset)
                );
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    mcsHandleMediaIconUrlAccess

DESCRIPTION
    Deals with access of the HANDLE_MEDIA_PLAYER_ICON_URL handle.
*/

static void mcsHandleMediaIconUrlAccess(
    GMCS_T* const mcs,
    GATT_MANAGER_SERVER_ACCESS_IND_T const* accessInd,
    uint16 maxRespValueLen)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        uint16 sizeValue = 0;
        if (((mcs->readLongProcData.attValChanged & GATT_MCS_SERVER_MEDIA_PLAYER_ICON_URL_CHANGED) == GATT_MCS_SERVER_MEDIA_PLAYER_ICON_URL_CHANGED) && (accessInd->offset != 0))
        {
            sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                ATT_RESULT_APP_MASK
                );
            return;
        }
        else if (maxRespValueLen < (mcs->data.mediaPlayerIconUrlLen - accessInd->offset))
        {
            sizeValue = maxRespValueLen;
        }
        else
        {
            sizeValue = (mcs->data.mediaPlayerIconUrlLen - accessInd->offset);
        }

        mcs->readLongProcData.attValChanged &= (~GATT_MCS_SERVER_MEDIA_PLAYER_ICON_URL_CHANGED);
        sendMcsServerAccessRsp(
            mcs->gattId,
            accessInd->cid,
            accessInd->handle,
            CSR_BT_GATT_ACCESS_RES_SUCCESS,
            sizeValue,
            (uint8*)(mcs->data.mediaPlayerIconUrl + accessInd->offset)
        );
    }
    else
    {
        sendMcsServerAccessErrorRsp(
            mcs->gattId,
            accessInd->cid,
            accessInd->handle,
            CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
        );
    }
}

/***************************************************************************
NAME
    mcsHandleTrackTitle

DESCRIPTION
    Deals with access of the HANDLE_TRACK_TITLE handle.
*/

static void mcsHandleTrackTitle(
        GMCS_T *const mcs,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd,
        uint16 maxRespValueLen)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        uint16 sizeValue = 0;
        if (((mcs->readLongProcData.attValChanged & GATT_MCS_SERVER_TRACK_TITLE_CHANGED) == GATT_MCS_SERVER_TRACK_TITLE_CHANGED) && (accessInd->offset != 0))
        {
            sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                ATT_RESULT_APP_MASK
                );
            return;
        }
        else if (maxRespValueLen < (mcs->data.trackTitleLen - accessInd->offset))
        {
            sizeValue = maxRespValueLen;
        }
        else
        {
            sizeValue = (mcs->data.trackTitleLen - accessInd->offset);
        }

        mcs->readLongProcData.attValChanged &= (~GATT_MCS_SERVER_TRACK_TITLE_CHANGED);

        sendMcsServerAccessRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeValue,
                (uint8*)(mcs->data.trackTitle + accessInd->offset)
                );
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}
/***************************************************************************
NAME
    mcsHandleTrackDurationAccess

DESCRIPTION
    Deals with access of the HANDLE_TRACK_DURATION handle.
*/

static void mcsHandleTrackDurationAccess(
                 GMCS_T *const mcs,
                 GATT_MANAGER_SERVER_ACCESS_IND_T const *access_ind
                 )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {

        sendMcsServerAccessRsp(
                mcs->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(int32),
                (uint8*)(&(mcs->data.trackDuration)));
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    mcsHandleTrackPositionAccess

DESCRIPTION
    Deals with access of the HANDLE_TRACK_POSITION handle.
*/

static void mcsHandleTrackPositionAccess(
                 GMCS_T *const mcs,
                 GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
                 )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {

        sendMcsServerAccessRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(int32),
                (uint8*)(&(mcs->data.trackPosition)));
    }
    else if(accessInd->flags & ATT_ACCESS_WRITE)
    {
        int32 newPosition = 0;
        newPosition = (int32)((0x000000FF & accessInd->value[0]) | (0x0000FF00 & (accessInd->value[1]<<8))
                                           | (0x00FF0000 & (accessInd->value[2]<<16)) | (0xFF000000 & (accessInd->value[3]<<24)));

        gattMcsServerWriteGenericResponse(
                    mcs->gattId,
                    accessInd->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    accessInd->handle
                    );

        if(mcs->data.mediaState == MCS_MEDIA_PLAYER_INACTIVE)
        {
            mcs->data.trackPosition = 0xFFFFFFFF;
        }
        else
        {
            GattMcsTrackPositionWriteInd *message = (GattMcsTrackPositionWriteInd*)
                                                        CsrPmemZalloc(sizeof(GattMcsTrackPositionWriteInd));

            message->srvcHndl = mcs->srvcHandle;
            message->newTrackPosition = newPosition;
            message->gattId = mcs->gattId;
            message->cid = accessInd->cid;
            message->currentPosition = mcs->data.trackPosition;

            McsMessageSend(mcs->appTask, GATT_MCS_TRACK_POSITION_WRITE_IND, message);
        }
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    mcsHandleContentControlIdAccess

DESCRIPTION
    Deals with access of the HANDLE_MEDIA_CONTENT_CONTROL_ID handle.
*/

static void mcsHandleContentControlIdAccess(
        GMCS_T *const mcs,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
            )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendMcsServerAccessRsp(
                    mcs->gattId,
                    accessInd->cid,
                    accessInd->handle,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    sizeof(uint8),
                    &(mcs->data.contentControlId));
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                    mcs->gattId,
                    accessInd->cid,
                    accessInd->handle,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                    );
    }
}

/***************************************************************************
NAME
    mcsHandlePlaybackSpeed

DESCRIPTION
    Deals with access of the HANDLE_PLAYBACK_SPEED handle.
*/

static void mcsHandlePlaybackSpeed(
                 GMCS_T *const mcs,
                 GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
                 )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendMcsServerAccessRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(int8),
                (uint8*)(&(mcs->data.playbackSpeed)));
    }
    else if(accessInd->flags & ATT_ACCESS_WRITE)
    {

        gattMcsServerWriteGenericResponse(
                    mcs->gattId,
                    accessInd->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    accessInd->handle
                    );

        if(mcs->data.mediaState == MCS_MEDIA_PLAYER_INACTIVE)
        {
            mcs->data.trackPosition = 0xFFFFFFFF;
        }
        else
        {
            GattMcsPlaybackSpeedWriteInd *message = (GattMcsPlaybackSpeedWriteInd*)
                                                        CsrPmemZalloc(sizeof(GattMcsPlaybackSpeedWriteInd));

            message->srvcHndl = mcs->srvcHandle;
            message->gattId = mcs->gattId;
            message->cid = accessInd->cid;
            message->currentPlaybackSpeed = mcs->data.playbackSpeed;
            message->newPlaybackSpeed = (int8)(accessInd->value[0]);

            McsMessageSend(mcs->appTask, GATT_MCS_PLAYBACK_SPEED_WRITE_IND, message);
        }
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    mcsHandleSeekingSpeed

DESCRIPTION
    Deals with access of the HANDLE_PLAYBACK_SPEED handle.
*/

static void mcsHandleSeekingSpeed(
                 GMCS_T *const mcs,
                 GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
                 )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendMcsServerAccessRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(int8),
                (uint8*)(&(mcs->data.seekingSpeed)));
    }
    else if(accessInd->flags & ATT_ACCESS_WRITE)
    {
        GattMcsSeekingSpeedWriteInd *message = (GattMcsSeekingSpeedWriteInd*)
                                       CsrPmemZalloc(sizeof(GattMcsSeekingSpeedWriteInd));

        gattMcsServerWriteGenericResponse(
                    mcs->gattId,
                    accessInd->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    accessInd->handle
                    );

        message->srvcHndl = mcs->srvcHandle;
        message->gattId = mcs->gattId;
        message->cid = accessInd->cid;
        message->currentPlaybackSpeed = mcs->data.playbackSpeed;
        message->seekSpeed = (int8)(accessInd->value[0]);

        McsMessageSend(mcs->appTask, GATT_MCS_SEEKING_SPEED_WRITE_IND, message);
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    mcsHandlePlayingOrderAccess

DESCRIPTION
    Deals with access of the HANDLE_PLAYBACK_SPEED handle.
*/

static void mcsHandlePlayingOrderAccess(
                 GMCS_T *const mcs,
                 GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
                 )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendMcsServerAccessRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(GattMcsMediaPlayingOrderType),
                (uint8*)(&(mcs->data.playingOrder)));
    }
    else if(accessInd->flags & ATT_ACCESS_WRITE)
    {
        GattMcsMediaPlayingOrderType order = (GattMcsMediaPlayingOrderType)(accessInd->value[0]);

        gattMcsServerWriteGenericResponse(
            mcs->gattId,
            accessInd->cid,
            CSR_BT_GATT_ACCESS_RES_SUCCESS,
            accessInd->handle
        );

        /* Check if the playing order is greater than allowed value*/
        if (order > MCS_PO_SHUFFLE_REPEAT)
        {
            GATT_MCS_SERVER_ERROR("\n Invalid Playing order value \n");
            return;
        }


        if (validatePlayingOrder(mcs->data.playingOrderSupported, order))
        {
            GattMcsPlayingOrderWriteInd* message = (GattMcsPlayingOrderWriteInd*)
                                                 CsrPmemZalloc(sizeof(GattMcsPlayingOrderWriteInd));

            message->srvcHndl = mcs->srvcHandle;
            message->gattId = mcs->gattId;
            message->cid = accessInd->cid;
            message->currentOrder = mcs->data.playingOrder;
            message->newOrder = order;
            message->supportedOrder = mcs->data.playingOrderSupported;

            McsMessageSend(mcs->appTask, GATT_MCS_PLAYING_ORDER_WRITE_IND, message);
        }
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    mcsHandlePlayingOrderSupportedAccess

DESCRIPTION
    Deals with access of the HANDLE_PLAYING_ORDER_SUPPORTED handle.
*/

static void mcsHandlePlayingOrderSupportedAccess(
                 GMCS_T *const mcs,
                 GATT_MANAGER_SERVER_ACCESS_IND_T const *access_ind
                 )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendMcsServerAccessRsp(
                mcs->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(GattMcsMediaSupportedPlayingOrderType),
                (uint8*)(&(mcs->data.playingOrderSupported)));
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    mcsHandleMediaStateAccess

DESCRIPTION
    Deals with access of the HANDLE_MEDIA_STATE handle.
*/

static void mcsHandleMediaStateAccess(
                 GMCS_T *const mcs,
                 GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
                 )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendMcsServerAccessRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(GattMcsMediaStateType),
                (uint8*)(&(mcs->data.mediaState)));
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    sendMcsMediaControlPointAccessResponse

DESCRIPTION
    Sends the media control point access response to remote client.
*/

void sendMcsMediaControlPointAccessResponse(
                                   GMCS_T *mcs,
                                   connection_id_t cid,
                                   GattMcsMediaControlPointType opcode,
                                   GattMediaControlPointAccessResultCode resultCode)
{
	GattMediaControlPointAcessResponseNotifyType* response ;
    response = (GattMediaControlPointAcessResponseNotifyType*)
                             CsrPmemZalloc(MCS_MEDIA_CONTROL_POINT_RESPONSE_SIZE);

    if(response == NULL)
    {
        GATT_MCS_SERVER_ERROR("\n GMCS: memory allocation failed in sendMcsMediaControlPointAccessResponse!! \n");
        return;
    }

    response->opcode = opcode;
    response->resulCode = resultCode;


    mcsServerSendCharacteristicChangedNotification(mcs->gattId,
                                                   cid,
                                                   HANDLE_MEDIA_CONTROL_POINT,
                                                   MCS_MEDIA_CONTROL_POINT_RESPONSE_SIZE,
                                                   (uint8*)response
                                                   );

    CsrPmemFree(response);
}

/***************************************************************************
NAME
    checkValidOpcode

DESCRIPTION
    Checks if opCode received during media control point operation is valid or not.
*/

bool checkValidOpcode(GattMcsOpcodeTypeSupported supportedOc, GattMcsMediaControlPointType oc)
{
    bool isValidOc = FALSE;
    switch(oc)
    {
        case MCS_OPCODE_PLAY:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_PLAY)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_PAUSE:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_PAUSE)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_FAST_REWIND:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_FAST_REWIND)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_FAST_FORWARD:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_FAST_FORWARD)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_STOP:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_STOP)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_MOVE_RELATIVE:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_MOVE_RELATIVE)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_PREVIOUS_SEGMENT:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_PREVIOUS_SEGMENT)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_NEXT_SEGMENT:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_NEXT_SEGMENT)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_FIRST_SEGMENT:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_FIRST_SEGMENT)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_LAST_SEGMENT:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_LAST_SEGMENT)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_GOTO_SEGMENT:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_GOTO_SEGMENT)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_PREVIOUS_TRACK:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_PREVIOUS_TRACK)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_NEXT_TRACK:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_NEXT_TRACK)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_FIRST_TRACK:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_FIRST_TRACK)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_LAST_TRACK:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_LAST_TRACK)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_GOTO_TRACK:
        {
            if(supportedOc & MCS_SUPPORTED_OPCODE_GOTO_TRACK)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_FIRST_GROUP:
        {
            if (supportedOc & MCS_SUPPORTED_OPCODE_FIRST_GROUP)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_LAST_GROUP:
        {
            if (supportedOc & MCS_SUPPORTED_OPCODE_LAST_GROUP)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_NEXT_GROUP:
        {
            if (supportedOc & MCS_SUPPORTED_OPCODE_NEXT_GROUP)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_PREVIOUS_GROUP:
        {
            if (supportedOc & MCS_SUPPORTED_OPCODE_PREVIOUS_GROUP)
                isValidOc = TRUE;
        }
        break;

        case MCS_OPCODE_GOTO_GROUP:
        {
            if (supportedOc & MCS_SUPPORTED_OPCODE_GOTO_GROUP)
                isValidOc = TRUE;
        }
        break;

        default:
            GATT_MCS_SERVER_ERROR("\n GMCS: Opcode not supported\n");
        break;
    }
    return isValidOc;
}

/***************************************************************************
NAME
    validatePlayingOrder

DESCRIPTION
    Checks if Playing Order received during media control point operation is valid or not.
*/

bool validatePlayingOrder(GattMcsMediaSupportedPlayingOrderType pos, GattMcsMediaPlayingOrderType po)
{

    switch (po)
    {
        case MCS_PO_SINGLE_ONCE:
       {
            if (pos & MCS_POS_SINGLE_ONCE)
                return TRUE;
       }
       case MCS_PO_SINGLE_REPEAT:
       {
            if (pos & MCS_POS_SINGLE_REPEAT)
                return TRUE;
       }
       case MCS_PO_SHUFFLE_ONCE:
       {
            if (pos & MCS_POS_SHUFFLE_ONCE)
                return TRUE;
       }
       case MCS_PO_SHUFFLE_REPEAT:
       {
            if (pos & MCS_POS_SHUFFLE_REPEAT)
                return TRUE;
       }
       case MCS_PO_OLDEST_ONCE:
       {
            if (pos & MCS_POS_OLDEST_ONCE)
                return TRUE;
       }
       case MCS_PO_OLDEST_REPEAT:
       {
            if (pos & MCS_POS_OLDEST_REPEAT)
                return TRUE;
       }
       case MCS_PO_NEWEST_ONCE:
       {
            if (pos & MCS_POS_NEWEST_ONCE)
                return TRUE;
       }
       case MCS_PO_NEWEST_REPEAT:
       {
            if (pos & MCS_POS_NEWEST_REPEAT)
                return TRUE;
       }
       case MCS_PO_IN_ORDER_ONCE:
       {
            if (pos & MCS_POS_IN_ORDER_ONCE)
                return TRUE;
       }
       case MCS_PO_IN_ORDER_REPEAT:
       {
            if (pos & MCS_POS_IN_ORDER_REPEAT)
                return TRUE;
       }
    default:
        GATT_MCS_SERVER_ERROR("\n GMCS: Playing Order not supported\n");
        break;
    }
    return FALSE;
}

/***************************************************************************
NAME
    validateStateTransition

DESCRIPTION
    Checks if state transition caused by media control point 
    operation is valid or not.
*/

static GattMediaControlPointAccessResultCode validateStateTransition(GattMcsMediaStateType state, 
                                                             GattMcsMediaControlPointType opcode,
                                                             GattMcsOpcodeTypeSupported supported)
{
    /* Check if server supports the operation */

    if (!checkValidOpcode(supported, opcode))
    {
        return MCS_OPCODE_NOT_SUPPORTED;
    }

    switch (state)
    {
        case MCS_MEDIA_STATE_PLAYING:
        {
            if (opcode == MCS_OPCODE_PLAY)
                return MCS_CONTROL_POINT_COMMAND_CANNOT_BE_COMPLETED;
            else
                return MCS_CONTROL_POINT_ACCESS_SUCCESS;
        }
        case MCS_MEDIA_STATE_PAUSED:
        {
            if (opcode == MCS_OPCODE_PAUSE)
                return MCS_CONTROL_POINT_COMMAND_CANNOT_BE_COMPLETED;
            else
                return MCS_CONTROL_POINT_ACCESS_SUCCESS;
        }
        case MCS_MEDIA_STATE_SEEKING:
        {
            return MCS_CONTROL_POINT_ACCESS_SUCCESS;
        }
        case MCS_MEDIA_STATE_INACTIVE:
        {
            return MCS_MEDIA_PLAYER_INACTIVE;
        }
    default:
        break;
    }
    return MCS_CONTROL_POINT_COMMAND_CANNOT_BE_COMPLETED;
}

/***************************************************************************
NAME
    mcsHandleCallControlPointAccess

DESCRIPTION
    Deals with access of the HANDLE_CALL_CONTROL_POINT handle.
*/

static void mcsHandleMediaControlPointAccess(
                                  GMCS_T *mcs,
                                  GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd)
{
    if (accessInd->flags & ATT_ACCESS_WRITE)
    {
        /* This may contain multiple control point opcodes */
        uint16 mcpSize = accessInd->size_value - 1;
        uint8 opcode = accessInd->value[0];
        GattMediaControlPointAccessResultCode result;

        gattMcsServerWriteGenericResponse(
                    mcs->gattId,
                    accessInd->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    accessInd->handle
                    );

        /* Validate the opcode and State transition and Send Indication to the application, 
         notifications are sent upon response to this message */

        result = validateStateTransition(mcs->data.mediaState, 
                                        opcode,
                                        mcs->data.supportedOpcode);

        if (MCS_CONTROL_POINT_ACCESS_SUCCESS == result)
        {
            GattMcsControlPointWriteInd* message = (GattMcsControlPointWriteInd*)
                                             CsrPmemZalloc(sizeof(GattMcsControlPointWriteInd));

            message->param = 0;
            message->srvcHndl = mcs->srvcHandle;
            message->cid = accessInd->cid;
            message->currentState = mcs->data.mediaState;
            message->opcode = opcode;

            if (mcpSize)
            {
                message->param = (int32)((accessInd->value[1] & 0x000000FF) |
                                      ((accessInd->value[2] << 8) & 0x0000FF00) |
                                      ((accessInd->value[3] << 16) & 0x00FF0000) |
                                      ((accessInd->value[4]) << 24 & 0xFF000000));
            }

            McsMessageSend(mcs->appTask,
                         GATT_MCS_MEDIA_CONTROL_POINT_WRITE_IND,
                         message
                         );
        }
        else
        {
            sendMcsMediaControlPointAccessResponse(mcs,
                                        accessInd->cid,
                                        opcode,
                                        result);
        }

    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    mcsHandleMcsSupportedOpcodeAccess

DESCRIPTION
    Deals with access of the HANDLE_MEDIA_STATE handle.
*/

static void mcsHandleMcsSupportedOpcodeAccess(
                 GMCS_T *const mcs,
                 GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
                 )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendMcsServerAccessRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(GattMcsOpcodeTypeSupported),
                (uint8*)(&(mcs->data.supportedOpcode)));
    }
    else
    {
        sendMcsServerAccessErrorRsp(
                mcs->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************/
bool mcsFindCid(const GMCS_T *mcs, connection_id_t cid, uint8 *index)
{
    uint8 i;
    bool res = FALSE;

    for(i=0; i<MCS_MAX_CONNECTIONS; i++)
    {
        if(mcs->data.connectedClients[i].cid == cid)
        {
            (*index) = i;
            res = TRUE;
        }
    }
    return res;
}

/***************************************************************************/
void mcsHandleAccessIndication(
        GMCS_T *mcs,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd,
        uint16 maxRespValueLen)
{
    uint8 i;

    if(mcsFindCid(mcs, accessInd->cid, &i))
    {
        switch (accessInd->handle)
        {

            case HANDLE_MEDIA_PLAYER_NAME:
            {
                mcsHandleMediaPlayerNameAccess(
                            mcs,
                            accessInd,
                            maxRespValueLen);
                break;
            }            
            
            case HANDLE_MEDIA_PLAYER_NAME_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.mediaPlayerNameClientCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(
                                                mcs->gattId,
                                                accessInd,
                                                &clientConfig
                                                );
                    mcs->data.connectedClients[i].clientCfg.mediaPlayerNameClientCfg = MCS_CCC_MASK(clientConfig);
                }
                else
                {
                    sendMcsServerAccessErrorRsp(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }

            case HANDLE_MEDIA_PLAYER_ICON_URL:
            {
                mcsHandleMediaIconUrlAccess(mcs,
                                            accessInd,
                                            maxRespValueLen);
                break;
            }

            case HANDLE_TRACK_CHANGED:

                sendMcsServerAccessErrorRsp(
                            mcs->gattId,
                            accessInd->cid,
                            accessInd->handle,
                            CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                            );
                break;
                
            case HANDLE_TRACK_CHANGED_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.trackChangedClientCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(
                                                mcs->gattId,
                                                accessInd,
                                                &clientConfig
                                                );
                    mcs->data.connectedClients[i].clientCfg.trackChangedClientCfg = MCS_CCC_MASK(clientConfig);
                }                
                else
                {
                    sendMcsServerAccessErrorRsp(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
                }
                break;                
            }          

            case HANDLE_TRACK_TITLE:
            {
                mcsHandleTrackTitle(mcs,
                                    accessInd,
                                    maxRespValueLen);
                break;
            }

            case HANDLE_TRACK_TITLE_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.trackTitleClientCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(
                                                mcs->gattId,
                                                accessInd,
                                                &clientConfig
                                                );
                    mcs->data.connectedClients[i].clientCfg.trackTitleClientCfg = MCS_CCC_MASK(clientConfig);
                }                
                else
                {
                    sendMcsServerAccessErrorRsp(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }
            case HANDLE_TRACK_DURATION:
                mcsHandleTrackDurationAccess(
                            mcs,
                            accessInd);
                break;
            case HANDLE_TRACK_DURATION_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.trackDurationClientCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(
                                                mcs->gattId,
                                                accessInd,
                                                &clientConfig
                                                );
                    mcs->data.connectedClients[i].clientCfg.trackDurationClientCfg = MCS_CCC_MASK(clientConfig);
                }
                else
                {
                    sendMcsServerAccessErrorRsp(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }
            case HANDLE_TRACK_POSITION:
                mcsHandleTrackPositionAccess(
                                 mcs,
                                 accessInd);
                break;
            case HANDLE_TRACK_POSITION_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.trackPositionClientCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(
                                                mcs->gattId,
                                                accessInd,
                                                &clientConfig
                                                );
                    mcs->data.connectedClients[i].clientCfg.trackPositionClientCfg = MCS_CCC_MASK(clientConfig);
                }
                else
                {
                    sendMcsServerAccessErrorRsp(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;                
            }                
            
            case HANDLE_MEDIA_CONTENT_CONTROL_ID:
                mcsHandleContentControlIdAccess(
                            mcs,
                            accessInd
                            );
                break;  
                     
            case HANDLE_PLAYBACK_SPEED:
                mcsHandlePlaybackSpeed(
                            mcs,
                            accessInd
                            );
                break;
            case HANDLE_PLAYBACK_SPEED_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.playbackSpeedClientCfg);
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(mcs->gattId,
                                                   accessInd->cid,
                                                   accessInd->handle,
                                                   clientConfig
                                                   );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(mcs->gattId,
                                                    accessInd,
                                                    &clientConfig
                                                    );
                    mcs->data.connectedClients[i].clientCfg.playbackSpeedClientCfg = MCS_CCC_MASK(clientConfig);
                }
                else
                {
                    sendMcsServerAccessErrorRsp(mcs->gattId,
                                               accessInd->cid,
                                               accessInd->handle,
                                               CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                               );
                }
                break;
            }
            case HANDLE_SEEKING_SPEED:
                mcsHandleSeekingSpeed(
                            mcs,
                            accessInd
                            );
                break;
                
            case HANDLE_SEEKING_SPEED_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.seekingSpeedClientCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(mcs->gattId,
                                                    accessInd,
                                                    &clientConfig
                                                    );
                    mcs->data.connectedClients[i].clientCfg.seekingSpeedClientCfg = MCS_CCC_MASK(clientConfig);
                }                
                else
                {
                    sendMcsServerAccessErrorRsp(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }             
            case HANDLE_PLAYING_ORDER:
                mcsHandlePlayingOrderAccess(
                            mcs,
                            accessInd
                            );
                break;              
            case HANDLE_PLAYING_ORDER_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.playingOrderClientCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(mcs->gattId,
                                                    accessInd,
                                                    &clientConfig
                                                    );
                    mcs->data.connectedClients[i].clientCfg.playingOrderClientCfg = MCS_CCC_MASK(clientConfig);
                }                
                else
                {
                    sendMcsServerAccessErrorRsp(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;                
            }      
            
            case HANDLE_PLAYING_ORDER_SUPPORTED:
                mcsHandlePlayingOrderSupportedAccess(
                            mcs,
                            accessInd
                            );
                break;        
            case HANDLE_MEDIA_STATE:
                mcsHandleMediaStateAccess(
                                        mcs,
                                        accessInd
                                        );
                            break;
            case HANDLE_MEDIA_STATE_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.mediaStateClientCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(mcs->gattId,
                                                    accessInd,
                                                    &clientConfig
                                                    );
                    mcs->data.connectedClients[i].clientCfg.mediaStateClientCfg = MCS_CCC_MASK(clientConfig);
                }                
                else
                {
                    sendMcsServerAccessErrorRsp(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;                
            }               
                
            case HANDLE_MEDIA_CONTROL_POINT:
            {
                mcsHandleMediaControlPointAccess(
                            mcs,
                            accessInd
                            );
            }
            break;
            case HANDLE_MEDIA_CONTROL_POINT_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.mediaControlPointClientCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(mcs->gattId,
                                                    accessInd,
                                                    &clientConfig
                                                    );
                    mcs->data.connectedClients[i].clientCfg.mediaControlPointClientCfg = MCS_CCC_MASK(clientConfig);
                }
                else
                {
                    sendMcsServerAccessErrorRsp(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }

            case HANDLE_MEDIA_CONTROL_POINT_OP_SUPP:
            {
                mcsHandleMcsSupportedOpcodeAccess(
                            mcs,
                            accessInd
                            );
            }
            break;
            case HANDLE_MEDIA_CONTROL_POINT_OP_SUPP_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MCS_CLIENT_CONFIG(mcs->data.connectedClients[i].clientCfg.mediaControlOpcodeSupportedCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    mcsHandleReadClientConfigAccess(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    mcsHandleWriteClientConfigAccess(mcs->gattId,
                                                    accessInd,
                                                    &clientConfig
                                                    );
                    mcs->data.connectedClients[i].clientCfg.mediaControlOpcodeSupportedCfg = MCS_CCC_MASK(clientConfig);
                }
                else
                {
                    sendMcsServerAccessErrorRsp(
                                mcs->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }

            default:
            {
                sendMcsServerAccessErrorRsp(
                            mcs->gattId,
                            accessInd->cid,
                            accessInd->handle,
                            CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE
                            );
                break;
            }
        } /* switch */
    }
    else
    {
        GATT_MCS_SERVER_ERROR(
                    "GMCS: There is no instance associated with the provided cid!\n"
                    );
    }
}
