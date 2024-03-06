/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_mcs_client.h"
#include "gatt_mcs_client_debug.h"
#include "gatt_mcs_client_private.h"
#include "gatt_mcs_client_read.h"
#include "gatt_mcs_client_common_util.h"

/***************************************************************************/
void mcsClientHandleInternalRead(const GMCSC * mcsClient, MediaPlayerAttribute charac)
{
    uint16 currHandle = GATT_ATTR_HANDLE_INVALID;

    switch (charac)
    {
        case MCS_MEDIA_PLAYER_NAME:
        {
            currHandle = mcsClient->handles.mediaPlayerNameHandle;
            break;
        }
        case MCS_TRACK_TITLE:
        {
            currHandle = mcsClient->handles.trackTitleHandle;
            break;
        }
        case MCS_TRACK_DURATION:
        {
            currHandle = mcsClient->handles.trackDurationHandle;
            break;
        }
        case MCS_TRACK_POSITION:
        {
            currHandle = mcsClient->handles.trackPositionHandle;
            break;
        }
        case MCS_PLAYBACK_SPEED:
        {
            currHandle = mcsClient->handles.playbackSpeedHandle;
            break;
        }
        case MCS_SEEKING_SPEED:
        {
            currHandle = mcsClient->handles.seekingSpeedHandle;
            break;
        }
        case MCS_PLAYING_ORDER:
        {
            currHandle = mcsClient->handles.playingOrderHandle;
            break;
        }
        case MCS_PLAYING_ORDER_SUPP:
        {
            currHandle = mcsClient->handles.playingOrderSuppHandle;
            break;
        }
        case MCS_MEDIA_STATE:
        {
            currHandle = mcsClient->handles.mediaStateHandle;
            break;
        }
        case MCS_MEDIA_CONTROL_POINT_OP_SUPP:
        {
            currHandle = mcsClient->handles.mediaControlPointOpSuppHandle;
            break;
        }
        case MCS_CONTENT_CONTROL_ID:
        {
            currHandle = mcsClient->handles.contentControlIdHandle;
            break;
        }
        default :
        {
            break;
        }
    }

    if (currHandle != GATT_ATTR_HANDLE_INVALID)
    {
        CsrBtGattReadReqSend(mcsClient->srvcElem->gattId,
                             mcsClient->srvcElem->cid,
                             currHandle,
                             0);
    }
    else
    {
        GattMcsClientGetMediaPlayerAttributeCfm *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = mcsClient->srvcElem->service_handle;
        message->status = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
        message->charac = charac;
        message->sizeValue = 0;
        message->value = NULL;

        McsMessageSend(mcsClient->appTask, GATT_MCS_CLIENT_GET_MEDIA_PLAYER_ATTRIBUTE_CFM, message);
    }
}


/****************************************************************************/
void GattMcsClientGetMediaPlayerAttribute(ServiceHandle clnthndl, MediaPlayerAttribute charac)
{
    GMCSC *gattMcsClient = ServiceHandleGetInstanceData(clnthndl);

    if (gattMcsClient)
    {
        McsClientInternalMsgRead *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = gattMcsClient->srvcElem->service_handle;
        message->charac = charac;

        McsMessageSend(gattMcsClient->libTask,
                       MCS_CLIENT_INTERNAL_MSG_READ_REQ,
                       message);
    }
    else
    {
        GATT_MCS_CLIENT_ERROR("Invalid MCS Client instance!\n");
    }
}

/****************************************************************************/
void handleMcsReadValueResp(GMCSC *mcsClient, uint16 handle, status_t resultCode, uint16 valueLength, uint8 *value)
{
    uint8 *readVal;
    MediaPlayerAttribute charac;
    GattMcsClientGetMediaPlayerAttributeCfm *message = CsrPmemAlloc(sizeof(*message));

    charac = getMcsCharacFromHandle(mcsClient, handle);

    message->srvcHndl = mcsClient->srvcElem->service_handle;
    message->status = getMcsClientStatusFromGattStatus(resultCode);
    message->charac = charac;
    message->sizeValue = valueLength;

    if(value)
    {
        readVal = (uint8 *) CsrPmemAlloc(valueLength);
        CsrMemCpy(readVal, value, valueLength);

        message->value = readVal;
    }
    else
    {
        message->value = NULL;
    }

    McsMessageSend(mcsClient->appTask, GATT_MCS_CLIENT_GET_MEDIA_PLAYER_ATTRIBUTE_CFM, message);
}
