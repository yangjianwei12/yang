/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_mcs_client.h"
#include "gatt_mcs_client_debug.h"
#include "gatt_mcs_client_private.h"
#include "gatt_mcs_client_notification.h"
#include "gatt_mcs_client_common_util.h"


/*******************************************************************************/
void mcsClientNotifCfm(GMCSC *const mcsClient,
                       GattMcsClientStatus status,
                       GattMcsClientMessageId id)
{
    GattMcsClientNtfCfm *message = CsrPmemAlloc(sizeof(*message));

    /* Fill in client reference */
    message->srvcHndl = mcsClient->srvcElem->service_handle;

    /* Send the confirmation message to app task  */
    McsMessageSend(mcsClient->appTask, id, message);

    CSR_UNUSED(status);
}

/*******************************************************************************/
void mcsClientNotifInd(GMCSC *const mcsClient,
                       MediaPlayerAttribute charac,
                       status_t status,
                       GattMcsClientMessageId id)
{
    GattMcsClientNtfInd *message = CsrPmemAlloc(sizeof(*message));

    /* Fill in client reference */
    message->srvcHndl = mcsClient->srvcElem->service_handle;

    message->charac = charac;

    /* Fill in the status */
    message->status = getMcsClientStatusFromGattStatus(status);

    /* Send the confirmation message to app task  */
    McsMessageSend(mcsClient->appTask, id, message);
}

/***************************************************************************/
void GattMcsClientRegisterForNotificationReq(ServiceHandle clntHndl, MediaPlayerAttributeMask characType, uint32 notifValue)
{
    GMCSC *gattMcsClient = ServiceHandleGetInstanceData(clntHndl);

    if (gattMcsClient)
    {
        McsClientInternalMsgNotificationReq *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = gattMcsClient->srvcElem->service_handle;
        message->characType = characType;
        message->notifValue = notifValue;

        gattMcsClient->pendingCmd = MCS_CLIENT_PENDING_OP_WRITE_CCCD;

        McsMessageSend(gattMcsClient->libTask,
                       MCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                       message);
    }
    else
    {
        GATT_MCS_CLIENT_ERROR("Invalid MCS Client instance!\n");
    }
}


/****************************************************************************/
void mcsClientHandleInternalRegisterForNotification(GMCSC *gatt_mcs_client, MediaPlayerAttributeMask characList, uint32 notif_value)
{
    if (gatt_mcs_client)
    {
        uint16 currHandle;
        uint32 mask = 0x00000001;
        uint32 val = 0;

        while (characList)
        {
            currHandle = GATT_ATTR_HANDLE_INVALID;

            switch (characList & mask)
            { /* Retrieve the characteristic for which notification request was sent from flag */
                case MCS_MEDIA_PLAYER_NAME_POS0:
                {
                    currHandle = gatt_mcs_client->handles.mediaPlayerNameCccHandle;
                    val = notif_value & MCS_MEDIA_PLAYER_NAME_POS0;
                    break;
                }
                case MCS_TRACK_CHANGED_POS1:
                {
                    currHandle = gatt_mcs_client->handles.trackChangedCccHandle;
                    val = notif_value & MCS_TRACK_CHANGED_POS1;
                    break;
                }
                case MCS_TRACK_TITLE_POS2:
                {
                    currHandle = gatt_mcs_client->handles.trackTitleCccHandle;
                    val = notif_value & MCS_TRACK_TITLE_POS2;
                    break;
                }
                case MCS_TRACK_DURATION_POS3:
                {
                    currHandle = gatt_mcs_client->handles.trackDurationCccHandle;
                    val = notif_value & MCS_TRACK_DURATION_POS3;
                    break;
                }
                case MCS_TRACK_POSITION_POS4:
                {
                    currHandle = gatt_mcs_client->handles.trackPositionCccHandle;
                    val = notif_value & MCS_TRACK_POSITION_POS4;
                    break;
                }
                case MCS_PLAYBACK_SPEED_POS5:
                {
                    currHandle = gatt_mcs_client->handles.playbackSpeedCccHandle;
                    val = notif_value & MCS_PLAYBACK_SPEED_POS5;
                    break;
                }
                case MCS_SEEKING_SPEED_POS6:
                {
                    currHandle = gatt_mcs_client->handles.seekingSpeedCccHandle;
                    val = notif_value & MCS_SEEKING_SPEED_POS6;
                    break;
                }
                case MCS_PLAYING_ORDER_POS11:
                {
                    currHandle = gatt_mcs_client->handles.playingOrderCccHandle;
                    val = notif_value & MCS_PLAYING_ORDER_POS11;
                    break;
                }
                case MCS_MEDIA_STATE_POS12:
                {
                    currHandle = gatt_mcs_client->handles.mediaStateCccHandle;
                    val = notif_value & MCS_MEDIA_STATE_POS12;
                    break;
                }
                case MCS_MEDIA_CONTROL_POINT_POS13:
                {
                    currHandle = gatt_mcs_client->handles.mediaControlPointCccHandle;
                    val = notif_value & MCS_MEDIA_CONTROL_POINT_POS13;
                    break;
                }
                case MCS_MEDIA_CONTROL_POINT_OP_SUPP_POS14:
                {
                    currHandle = gatt_mcs_client->handles.mediaControlPointOpSuppCccHandle;
                    val = notif_value & MCS_MEDIA_CONTROL_POINT_OP_SUPP_POS14;
                    break;
                }

                default:
                {
                    break;
                }
            }

            characList &= ~mask;
            mask <<= 1;

            if (currHandle != GATT_ATTR_HANDLE_INVALID)
            {
                uint8* value = (uint8*)(CsrPmemAlloc(GATT_MCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE));

                value[0] = val ? MCS_NOTIFICATION_VALUE : 0;
                value[1] = 0;

                /* Increment readCount to track total write requests made to GATT */
                gatt_mcs_client->writeCccCount++;

                CsrBtGattWriteReqSend(gatt_mcs_client->srvcElem->gattId,
                                      gatt_mcs_client->srvcElem->cid,
                                      currHandle,
                                      0,
                                      GATT_MCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                                      value);
            }
        }
    }
    else
    {
        GATT_MCS_CLIENT_ERROR("Invalid MCS Client instance!\n");
    }
}

/****************************************************************************/
void handleMcsClientNotification(GMCSC *mcsClient, uint16 handle, uint16 valueLength, uint8 *value)
{
    MediaPlayerAttribute charac;

    charac = getMcsCharacFromHandle(mcsClient, handle);

    if(charac == 0)
    {
        /* Unlikely */
    }
    else if (charac == MCS_MEDIA_CONTROL_POINT)
    {
        GattMcsClientSetMediaControlPointCfm *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = mcsClient->srvcElem->service_handle;
        message->op = value[0];
        message->status = value[1];

        McsMessageSend(mcsClient->appTask,
                       GATT_MCS_CLIENT_SET_MEDIA_CONTROL_POINT_CFM,
                       message);
    }
    else
    {
        GattMcsClientMediaPlayerAttributeInd *message = CsrPmemAlloc(sizeof(*message));
        uint8 *indVal;

        message->srvcHndl = mcsClient->srvcElem->service_handle;
        message->charac = charac;
        message->sizeValue = valueLength;

        if (value)
        {
            indVal = (uint8 *) CsrPmemAlloc(valueLength);
            CsrMemCpy(indVal, value, valueLength);

            message->value = indVal;
        }
        else
        {
            message->value = NULL;
        }

        McsMessageSend(mcsClient->appTask,
                       GATT_MCS_CLIENT_MEDIA_PLAYER_ATTRIBUTE_IND,
                       message);
    }
}

