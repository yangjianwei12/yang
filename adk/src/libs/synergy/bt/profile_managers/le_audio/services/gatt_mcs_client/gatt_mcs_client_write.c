/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_mcs_client_private.h"
#include "gatt_mcs_client_write.h"
#include "gatt_mcs_client_debug.h"
#include "gatt_mcs_client.h"
#include "gatt_mcs_client_common_util.h"


/***************************************************************************/
void mcsClientHandleInternalWrite(GMCSC *const mcsClient,
                                  MediaPlayerAttributeMask charac,
                                  uint16 sizeValue,
                                  uint8 * value)
{
    uint16 currHandle = GATT_ATTR_HANDLE_INVALID;

    switch(charac)
    {
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
        case MCS_PLAYING_ORDER:
        {
            currHandle = mcsClient->handles.playingOrderHandle;
            break;
        }
        case MCS_MEDIA_CONTROL_POINT:
        {
            currHandle = mcsClient->handles.mediaControlPointHandle;
            break;
        }
        default :
        {
            break;
        }
    }

    if (currHandle != GATT_ATTR_HANDLE_INVALID)
    {
        CsrBtGattWriteReqSend(mcsClient->srvcElem->gattId,
                              mcsClient->srvcElem->cid,
                              currHandle,
                              0,
                              sizeValue,
                              value);
    }
    else
    {
        GattMcsClientSetMediaPlayerAttributeCfm *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = mcsClient->srvcElem->service_handle;
        message->status = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
        message->charac = charac;

        McsMessageSend(mcsClient->appTask, GATT_MCS_CLIENT_SET_MEDIA_PLAYER_ATTRIBUTE_CFM, message);
    }

}

void mcsClientHandleInternalSetMediaControlPoint(GMCSC *const mcsClient,
                                                 GattMcsOpcode op,
                                                 int32 val)
{
    if (mcsClient->handles.mediaControlPointHandle)
    {
        uint8 opSize = MCS_CLIENT_OPCODE_SIZE;
        uint8 *value;

        switch (op)
        {
            case GATT_MCS_CLIENT_MOVE_RELATIVE:
            case GATT_MCS_CLIENT_GOTO_SEGMENT:
            case GATT_MCS_CLIENT_GOTO_TRACK:
            case GATT_MCS_CLIENT_GOTO_GROUP:
            {
                opSize = MCS_CLIENT_OPCODE_SIZE_PARAM;
                break;
            }
            default:
            {
                break;
            }
        }

        value = (uint8*) (CsrPmemAlloc(opSize));
        value[0] = op;

        if (opSize != MCS_CLIENT_OPCODE_SIZE)
            memcpy(&value[1], &val, sizeof(int32));

        CsrBtGattWriteReqSend(mcsClient->srvcElem->gattId,
                              mcsClient->srvcElem->cid,
                              mcsClient->handles.mediaControlPointHandle,
                              0,
                              opSize,
                              value);
    }
    else
    {
        GattMcsClientSetMediaControlPointCfm *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = mcsClient->srvcElem->service_handle;
        message->status = GATT_MCS_OP_RESULT_CHARAC_NOT_SUPPORTED;
        message->op = op;

        McsMessageSend(mcsClient->appTask, GATT_MCS_CLIENT_SET_MEDIA_CONTROL_POINT_CFM, message);
    }

}


void GattMcsClientSetMediaPlayerAttribute(ServiceHandle clntHndl, MediaPlayerAttribute charac, uint16 len, uint8 *val)
{
    GMCSC *gattMcsClient = ServiceHandleGetInstanceData(clntHndl);

    if (gattMcsClient)
    {
        if (gattMcsClient->pendingCmd == MCS_CLIENT_PENDING_OP_WRITE_CCCD)
        {
            GattMcsClientSetMediaPlayerAttributeCfm *message = CsrPmemAlloc(sizeof(*message));

            message->srvcHndl = clntHndl;
            message->status = GATT_MCS_CLIENT_STATUS_BUSY;
            message->charac = charac;

            McsMessageSend(gattMcsClient->appTask,
                           GATT_MCS_CLIENT_SET_MEDIA_PLAYER_ATTRIBUTE_CFM,
                           message);
        }
        else
        {
            McsClientInternalMsgWrite *message = CsrPmemAlloc(sizeof(*message));

            message->srvcHndl = gattMcsClient->srvcElem->service_handle;
            message->charac = charac;
            message->sizeValue = len;

            message->value = (uint8*) (CsrPmemAlloc(len));
            memcpy(message->value, val, len);

            McsMessageSend(gattMcsClient->libTask,
                           MCS_CLIENT_INTERNAL_MSG_WRITE_REQ,
                           message);
        }
    }
    else
    {
        GATT_MCS_CLIENT_ERROR("Invalid MCS Client instance!\n");
    }
}

void GattMcsClientSetMediaControlPoint(ServiceHandle clntHndl, GattMcsOpcode op, int32 val)
{
    GMCSC *gattMcsClient = ServiceHandleGetInstanceData(clntHndl);

    if (gattMcsClient)
    {
        if (gattMcsClient->pendingCmd == MCS_CLIENT_PENDING_OP_WRITE_CCCD)
        {
            GattMcsClientSetMediaControlPointCfm *message = CsrPmemAlloc(sizeof(*message));

            message->srvcHndl = clntHndl;
            message->status = GATT_MCS_CLIENT_STATUS_BUSY;
            message->op = op;

            McsMessageSend(gattMcsClient->appTask,
                           GATT_MCS_CLIENT_SET_MEDIA_CONTROL_POINT_CFM,
                           message);
        }
        else
        {
            McsClientInternalMsgSetMediaControlPoint *message = CsrPmemAlloc(sizeof(*message));

            message->srvcHndl = gattMcsClient->srvcElem->service_handle;
            message->op = op;
            message->val = val;

            McsMessageSend(gattMcsClient->libTask,
                           MCS_CLIENT_INTERNAL_MSG_SET_MEDIA_CONTROL_POINT_REQ,
                           message);
        }
    }
    else
    {
        GATT_MCS_CLIENT_ERROR("Invalid MCS Client instance!\n");
    }
}

/****************************************************************************/
void handleMcsWriteValueResp(GMCSC *mcsClient, uint16 handle, status_t resultCode)
{
    MediaPlayerAttribute charac;

    charac = getMcsCharacFromHandle(mcsClient, handle);

    if (charac!= MCS_MEDIA_CONTROL_POINT)
    {
        GattMcsClientSetMediaPlayerAttributeCfm *message = CsrPmemAlloc(sizeof(*message));
        message->srvcHndl = mcsClient->srvcElem->service_handle;
        message->status = getMcsClientStatusFromGattStatus(resultCode);
        message->charac = charac;

        McsMessageSend(mcsClient->appTask,
                       GATT_MCS_CLIENT_SET_MEDIA_PLAYER_ATTRIBUTE_CFM,
                       message);
    }
}


