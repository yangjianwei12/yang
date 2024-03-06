
#include "gatt_mcs_server_private.h"
#include "gatt_mcs_server_access.h"
#include "gatt_mcs_server_debug.h"

ServiceHandle mcsServiceHandle;
void initClientData(GattMcsClientData *clientDataPtr);

void initClientData(GattMcsClientData *clientDataPtr)
{
    if(clientDataPtr == NULL)
    {
        GATT_MCS_SERVER_PANIC("GMCS_T: Client data pointer is a NULL pointer");
    }
    else
    {
        clientDataPtr->cid = 0;
        clientDataPtr->clientCfg.mediaControlPointClientCfg = 0;
        clientDataPtr->clientCfg.mediaPlayerNameClientCfg =0;
        clientDataPtr->clientCfg.mediaStateClientCfg = 0;
        clientDataPtr->clientCfg.playbackSpeedClientCfg = 0;
        clientDataPtr->clientCfg.playingOrderClientCfg = 0;
        clientDataPtr->clientCfg.seekingSpeedClientCfg = 0;
        clientDataPtr->clientCfg.trackChangedClientCfg = 0;
        clientDataPtr->clientCfg.trackDurationClientCfg = 0;
        clientDataPtr->clientCfg.trackPositionClientCfg = 0;
        clientDataPtr->clientCfg.trackTitleClientCfg = 0;
    }
}

/* Notify all the connected clients when the characteristic of 'type'
 * is changed
 * */

void mcsServerNotifyConnectedClients(GMCS_T* mcs, MediaPlayerAttributeType type)
{
    uint16 i;

    for (i=0; i< MCS_MAX_CONNECTIONS; i++)
    {
        if (mcs->data.connectedClients[i].cid != 0)
        {
            switch(type)
            {
                case MCS_SRV_MEDIA_PLAYER_NAME:
                {
                     /* If the Client Config is 0x01 (Notify is TRUE), a notification will
                     * be sent to the client */
                     if (mcs->data.connectedClients[i].clientCfg.mediaPlayerNameClientCfg == MCS_SERVER_NOTIFY)
                     {
                         mcsServerSendCharacteristicChangedNotification(
                                        mcs->gattId,
                                        mcs->data.connectedClients[i].cid,
                                        HANDLE_MEDIA_PLAYER_NAME,
                                        mcs->data.mediaPlayerNameLen,
                                        (uint8*)mcs->data.mediaPlayerName
                                        );
                     }
                     break;
                }
                case MCS_SRV_TRACK_TITLE:
                {
                     if (mcs->data.connectedClients[i].clientCfg.trackTitleClientCfg == MCS_SERVER_NOTIFY)
                     {
                         mcsServerSendCharacteristicChangedNotification(
                                                      mcs->gattId,
                                                      mcs->data.connectedClients[i].cid,
                                                      HANDLE_TRACK_TITLE,
                                                      mcs->data.trackTitleLen,
                                                      (uint8*)mcs->data.trackTitle
                                                      );
                     }
                     break;
                }
                case MCS_SRV_TRACK_CHANGED:
                {
                    if (mcs->data.connectedClients[i].clientCfg.trackChangedClientCfg == MCS_SERVER_NOTIFY)
                    {
                         mcsServerSendCharacteristicChangedNotification(
                                                     mcs->gattId,
                                                     mcs->data.connectedClients[i].cid,
                                                     HANDLE_TRACK_CHANGED,
                                                     0,
                                                     NULL
                                                     );
                    }
                    break;
                }
                case MCS_SRV_TRACK_DURATION:
                {
                    if (mcs->data.connectedClients[i].clientCfg.trackDurationClientCfg == MCS_SERVER_NOTIFY)
                    {
                         mcsServerSendCharacteristicChangedNotification(
                                                     mcs->gattId,
                                                     mcs->data.connectedClients[i].cid,
                                                     HANDLE_TRACK_DURATION,
                                                     sizeof(mcs->data.trackDuration),
                                                     (uint8*)(&(mcs->data.trackDuration))
                                                     );
                    }
                    break;
                }
                case MCS_SRV_TRACK_POSITION:
                {
                    if (mcs->data.connectedClients[i].clientCfg.trackPositionClientCfg == MCS_SERVER_NOTIFY)
                    {
                         mcsServerSendCharacteristicChangedNotification(
                                                     mcs->gattId,
                                                     mcs->data.connectedClients[i].cid,
                                                     HANDLE_TRACK_POSITION,
                                                     sizeof(mcs->data.trackPosition),
                                                     (uint8*)(&(mcs->data.trackPosition))
                                                     );
                    }
                    break;
                }
                case MCS_SRV_PLAYBACK_SPEED:
                {
                    if (mcs->data.connectedClients[i].clientCfg.playbackSpeedClientCfg == MCS_SERVER_NOTIFY)
                    {
                         mcsServerSendCharacteristicChangedNotification(
                                                     mcs->gattId,
                                                     mcs->data.connectedClients[i].cid,
                                                     HANDLE_PLAYBACK_SPEED,
                                                     sizeof(mcs->data.playbackSpeed),
                                                     (uint8*)(&(mcs->data.playbackSpeed))
                                                     );
                    }
                    break;
                }
                case MCS_SRV_SEEKING_SPEED:
                {
                    if (mcs->data.connectedClients[i].clientCfg.seekingSpeedClientCfg == MCS_SERVER_NOTIFY)
                    {
                         mcsServerSendCharacteristicChangedNotification(
                                                     mcs->gattId,
                                                     mcs->data.connectedClients[i].cid,
                                                     HANDLE_SEEKING_SPEED,
                                                     sizeof(mcs->data.seekingSpeed),
                                                     (uint8*)(&(mcs->data.seekingSpeed))
                                                     );
                    }
                    break;
                }
                case MCS_SRV_PLAYING_ORDER:
                {
                    if (mcs->data.connectedClients[i].clientCfg.playingOrderClientCfg == MCS_SERVER_NOTIFY)
                    {
                         mcsServerSendCharacteristicChangedNotification(
                                                     mcs->gattId,
                                                     mcs->data.connectedClients[i].cid,
                                                     HANDLE_PLAYING_ORDER,
                                                     sizeof(mcs->data.playingOrder),
                                                     (uint8*)(&(mcs->data.playingOrder))
                                                     );
                    }
                    break;
                }
                case MCS_SRV_MEDIA_STATE:
                {
                    if (mcs->data.connectedClients[i].clientCfg.mediaStateClientCfg == MCS_SERVER_NOTIFY)
                    {
                         mcsServerSendCharacteristicChangedNotification(
                                                     mcs->gattId,
                                                     mcs->data.connectedClients[i].cid,
                                                     HANDLE_MEDIA_STATE,
                                                     sizeof(mcs->data.mediaState),
                                                     (uint8*)(&(mcs->data.mediaState))
                                                     );
                    }
                    break;
                }
                case MCS_SRV_MEDIA_CONTROL_POINT:
                {
                    if (mcs->data.connectedClients[i].clientCfg.mediaControlPointClientCfg == MCS_SERVER_NOTIFY)
                    {
                         mcsServerSendCharacteristicChangedNotification(
                                                     mcs->gattId,
                                                     mcs->data.connectedClients[i].cid,
                                                     HANDLE_MEDIA_CONTROL_POINT,
                                                     sizeof(mcs->data.mediaControlPoint),
                                                     (uint8*)(&(mcs->data.mediaControlPoint))
                                                     );
                    }
                    break;
                }
                case MCS_SRV_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED:
                {
                    if (mcs->data.connectedClients[i].clientCfg.mediaControlOpcodeSupportedCfg == MCS_SERVER_NOTIFY)
                    {
                         mcsServerSendCharacteristicChangedNotification(
                                                     mcs->gattId,
                                                     mcs->data.connectedClients[i].cid,
                                                     HANDLE_MEDIA_CONTROL_POINT_OP_SUPP,
                                                     sizeof(mcs->data.supportedOpcode),
                                                     (uint8*)(&(mcs->data.supportedOpcode))
                                                     );
                    }
                    break;
                }
                default:
                {
                     GATT_MCS_SERVER_ERROR("/nGMCS: Notification not supported for this characteristic/n");
                     break;
                }
            }
        }
    }
}

/* mcsServerUpdateTrackPosition
 *
 * Update server track position
 * */

static bool mcsServerUpdateTrackPosition(GMCS_T* mcsServer, uint16 size, uint8* value)
{
    int32 trackPositionOffset = 0, newTrackPosition = 0;

    if (size != 4)
    {
        GATT_MCS_SERVER_PANIC("/nGMCS: Invalid Track Position/n");
        return FALSE;
    }

    /* convert byte stream into int32 data*/

    trackPositionOffset = (int32)((value[0] & 0x000000FF) | ((value[1] << 8) & 0x0000FF00) |
                                ((value[2] << 16) & 0x00FF0000) | ((value[3] << 24) & 0xFF000000));

    /* Get new offset according to the signedness of value*/

    if (mcsServer->data.mediaState == MCS_MEDIA_STATE_INACTIVE)
    {
        mcsServer->data.trackPosition = 0xFFFFFFFF;
        return TRUE;
    }

    if (trackPositionOffset < 0)
    {
        newTrackPosition = mcsServer->data.trackDuration + trackPositionOffset;
    }
    else
    {
        newTrackPosition =  trackPositionOffset;
    }

    /* Update new track position */
    if (newTrackPosition > mcsServer->data.trackDuration)
    {
        mcsServer->data.trackPosition = mcsServer->data.trackDuration;
    }
    else if (newTrackPosition < 0)
    {
        mcsServer->data.trackPosition = 0;
    }
    else
    {
        mcsServer->data.trackPosition = newTrackPosition;
    }

    if (mcsServer->data.mediaState != MCS_MEDIA_STATE_PLAYING)
        mcsServerNotifyConnectedClients(mcsServer, MCS_SRV_TRACK_POSITION);
    return TRUE;
}

/******************************************************************************/
/* mcsServerUpdateMediaControlPoint
 *
 * Update media control point and perform corresponding functions
 * */
static bool mcsServerUpdateMediaControlPoint(GMCS_T *mcsServer, uint16 mcpLen, uint8* value)
{
    GattMcsMediaControlPointType mcp;
    mcp = (GattMcsMediaStateType)(value[0]);

    /* Check if the opcode is supported by server */

    if(!checkValidOpcode(mcsServer->data.supportedOpcode, mcp))
    {
        GATT_MCS_SERVER_ERROR("/nGMCS: Invalid Media Control Point value /n");
        return FALSE;
    }

    mcsServer->data.mediaControlPoint = mcp;

    switch(mcp)
    {
        case MCS_OPCODE_GOTO_SEGMENT:
        {
            GATT_MCS_SERVER_DEBUG("\n(GMCS): Requires Segment length \n");
            break;
        }
        case MCS_OPCODE_GOTO_TRACK:
        {
            GATT_MCS_SERVER_DEBUG("\n(GMCS): Needs to maintain Track List using OTS \n");
            break;
        }
        case MCS_OPCODE_GOTO_GROUP:
        {
            GATT_MCS_SERVER_DEBUG("\n(GMCS): Needs to maintain Group List using OTS \n");
            break;
        }
        case MCS_OPCODE_MOVE_RELATIVE:
        {
            if(mcpLen > 1 && mcpLen <= 5)
            {
                mcsServerUpdateTrackPosition(mcsServer, (mcpLen - 1), &value[1]);
                break;
            }
            else
            {
                GATT_MCS_SERVER_ERROR("/nGMCS: Invalid Media Control Point Length/n");
                return FALSE;
            }
        }
    }
    return TRUE;
}


/*  mcsServerCharacteristicStringUpdate
 *
 * Update string like properties of Media Player
 * */

static bool mcsServerCharacteristicStringUpdate(uint16* len,
                                                char** propertyString,
                                                char*  newString,
                                                uint16 newLen,
                                                uint16 maxLen)
{
    /* Check if length of new string is zero,if zero then free the pointer
       if not, free the memory allocated for old property string and reallocate
       memory required for new property string and then copy the new property
       */

    if (newLen == 0)
    {
        if (*propertyString)
        {
            free(*propertyString);
        }
        *propertyString = NULL;
        *len = 0;
        return TRUE;
    }
    else
    {
        /* Copy old length*/
        uint16 oldLen = *len;

        *propertyString =
            (char*)McsMemRealloc(*propertyString, len, newLen, maxLen);

        /* check if MemRealloc failed to allocate for new string*/
        if (*len == oldLen && oldLen != newLen)
        {
            return FALSE;
        }
        else
        {
            memcpy(*propertyString, newString, *len);
            return TRUE;
        }
    }
}


/* mcsServerUpdateCharacteristicString
 *
 * Update string like properties of Media Player
 * */

static bool mcsServerUpdateCharacteristicString(GMCS_T* mcs,
                                               uint16 len,
                                               char* string,
                                               uint16 maxLen,
                                               MediaPlayerAttributeType type)
{
    bool result;

    if (mcs == NULL)
    {
        return FALSE;
    }

    switch (type)
    {
        case MCS_SRV_MEDIA_PLAYER_NAME:
        {
            result = mcsServerCharacteristicStringUpdate(&(mcs->data.mediaPlayerNameLen),
                                                              &(mcs->data.mediaPlayerName),
                                                              string,
                                                              len,
                                                              maxLen);

            /* Notify Clients about the change*/
            if (result)
            {
                GATT_MCS_SERVER_WARNING("GMCS: mcsServerUpdateCharacteristicString MCS_SRV_MEDIA_PLAYER_NAME value changed\n");
                mcs->readLongProcData.attValChanged |= GATT_MCS_SERVER_MEDIA_PLAYER_NAME_CHANGED;

                mcsServerNotifyConnectedClients(mcs, type);
            }
            return result;
        }
        case MCS_SRV_TRACK_TITLE:
        {
            result = mcsServerCharacteristicStringUpdate(&(mcs->data.trackTitleLen),
                                                              &(mcs->data.trackTitle),
                                                              string,
                                                              len,
                                                              maxLen);

             /* Notify Clients about the change*/
            if (result)
            {
                GATT_MCS_SERVER_WARNING("GMCS: mcsServerUpdateCharacteristicString MCS_SRV_TRACK_TITLE value changed\n");
                mcs->readLongProcData.attValChanged |= GATT_MCS_SERVER_TRACK_TITLE_CHANGED;

                mcsServerNotifyConnectedClients(mcs, type);
            }
            return result;
        }
        case MCS_SRV_MEDIA_PLAYER_ICON_URL:
        {
            result = mcsServerCharacteristicStringUpdate(&(mcs->data.mediaPlayerIconUrlLen),
                                                              &(mcs->data.mediaPlayerIconUrl),
                                                              string,
                                                              len,
                                                              maxLen);

            /* Notify Clients about the change*/
            if (result)
            {
                GATT_MCS_SERVER_WARNING("GMCS: mcsServerUpdateCharacteristicString MCS_SRV_MEDIA_PLAYER_ICON_URL value changed\n");
                mcs->readLongProcData.attValChanged |= GATT_MCS_SERVER_MEDIA_PLAYER_ICON_URL_CHANGED;

                mcsServerNotifyConnectedClients(mcs, type);
            }
            return result;
        }
        default:
            break;
    }
    return FALSE;
}


/******************************************************************************/
ServiceHandle GattMcsServerInit(AppTask  appTask,
                                  uint16  startHandle,
                                  uint16  endHandle,
                                  GattMcsInitData* initData)
{
    GMCS_T  *mcsServer = NULL;
    CsrBtGattId gattId;
    int clientIndex;

    if(appTask == CSR_SCHED_QID_INVALID)
    {
        GATT_MCS_SERVER_PANIC("GattMcsServerInit fail invalid params");
        return 0;
    }

    mcsServiceHandle = ServiceHandleNewInstance((void **) &mcsServer, sizeof(GMCS_T));

    if (mcsServiceHandle !=0)
    {
        /* Reset all the service library memory */
        memset(mcsServer, 0, sizeof(GMCS_T));

        mcsServer->srvcHandle = mcsServiceHandle;

        /* Set up library handler for external messages */
        mcsServer->libTask = CSR_BT_MCS_SERVER_IFACEQUEUE;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        mcsServer->appTask = appTask;

        if(initData)
        {
            memcpy(&(mcsServer->data), initData, sizeof(GattMcsInitData));
        }
        else{
            memset(&(mcsServer->data), 0, sizeof(GattMcsInitData));
        }

        for (clientIndex=0; clientIndex< MCS_MAX_CONNECTIONS; clientIndex++)
        {
            initClientData(&(mcsServer->data.connectedClients[clientIndex]));
        }

        mcsServer->readLongProcData.attValChanged = 0x00;

        /* Register with the GATT  */
        gattId = CsrBtGattRegister(mcsServer->libTask);
        /* verify the result */
        if (gattId == CSR_BT_GATT_INVALID_GATT_ID)
        {
            ServiceHandleFreeInstanceData(mcsServiceHandle);
            mcsServiceHandle = 0;
        }
        else
        {
            mcsServer->gattId = gattId;
            CsrBtGattFlatDbRegisterHandleRangeReqSend(gattId, startHandle,endHandle);
        }
    }
    return mcsServiceHandle;
}

/******************************************************************************/
status_t GattMcsServerAddConfig(ServiceHandle srvcHndl,
                                ConnectionId  cid,
                                GattMcsClientConfigDataType *const config)
{
    uint8 i;
    GMCS_T* mcs = (GMCS_T*)ServiceHandleGetInstanceData(srvcHndl);

    if (mcs == NULL)
    {
        GATT_MCS_SERVER_ERROR("/nGMCS: is NULL/n");
        return CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE;
    }

    for (i = 0; i < MCS_MAX_CONNECTIONS; i++)
    {
        if (mcs->data.connectedClients[i].cid == 0)
        {
            /* Add client btConnId to the server data */
            mcs->data.connectedClients[i].cid = cid;

            /* Check config parameter:
             * If config is NULL, it indicates a default config should be used for the
             * peer device identified by the CID.
             * The default config is already set when the instance has been initialised.
             */

            if (config)
            {
                uint32 index;

                /* Save new ccc for the client */
                CsrMemCpy(&(mcs->data.connectedClients[i].clientCfg), config, sizeof(GattMcsClientConfigDataType));

                /* Notify all characteristics which are enabled for notification */
                for(index = 1; index <= MCS_SRV_MEDIA_PLAYER_ICON_URL; index = index << 1)
                {
                    mcsServerNotifyConnectedClients(mcs, index);
                }
            }

            return CSR_BT_GATT_ACCESS_RES_SUCCESS;
        }
    }

    return CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
}

/******************************************************************************/
GattMcsClientConfigDataType* GattMcsServerRemoveConfig(ServiceHandle srvcHndl,
                                                       ConnectionId  cid)
{
    uint8 i;
    GattMcsClientConfigDataType* config = NULL;
    GMCS_T* mcs = (GMCS_T*)ServiceHandleGetInstanceData(srvcHndl);

    if (mcs == NULL)
    {
        GATT_MCS_SERVER_ERROR("\n GMCS: NULL instance \n");
        return NULL;
    }

    for (i = 0; i < MCS_MAX_CONNECTIONS; i++)
    {
        /* Check the saved CID to find the peeer device */
        if (mcs->data.connectedClients[i].cid == cid)
        {
            /* Found peer device:
             * - save last client configurations
             * - remove the peer device
             * - free the server instance
             * - return last client configuration
             */

            config = CsrPmemAlloc(sizeof(GattMcsClientConfigDataType));
            memcpy(config, &(mcs->data.connectedClients[i].clientCfg), sizeof(GattMcsClientConfigDataType));

            if ((i == (MCS_MAX_CONNECTIONS - 1)) || (i == 0 && mcs->data.connectedClients[i + 1].cid == 0))
            {
                /* The peer device is the only or the last element of the array */
                memset(&(mcs->data.connectedClients[i]), 0, sizeof(GattMcsClientData));
            }
            else
            {
                /* The peer device is in the middle of the array */
                uint8 j;

                for (j = i; j < (MCS_MAX_CONNECTIONS - 1) && mcs->data.connectedClients[j + 1].cid != 0; j++)
                {
                    /* Shift all the elements of the array of one position behind */
                    memmove(&(mcs->data.connectedClients[j]),
                        &(mcs->data.connectedClients[j + 1]),
                             sizeof(GattMcsClientData));
                }

                /* Remove the last element of the array, already shifted behind */
                memset(&(mcs->data.connectedClients[j]), 0, sizeof(GattMcsClientData));
            }
        }
    }
    return config;
}

/******************************************************************************/
bool GattMcsServerSetMediaPlayerAttribute(ServiceHandle srvcHndl,
                                         uint16 size,
                                         uint8* value,
                                         MediaPlayerAttributeType type)
{
    GMCS_T *mcs = (GMCS_T *) ServiceHandleGetInstanceData(srvcHndl);

    if(mcs == NULL)
    {
        GATT_MCS_SERVER_ERROR("/nGMCS: NULL instance encountered/n");
        return FALSE;
    }

    switch(type)
    {
        case MCS_SRV_MEDIA_PLAYER_NAME:
        {
           return mcsServerUpdateCharacteristicString(mcs,
                                                size,
                                                (char*)value,
                                                MEDIA_PLAYER_NAME_SIZE,
                                                type);
        }

        case MCS_SRV_MEDIA_PLAYER_ICON_URL:
        {
            return mcsServerUpdateCharacteristicString(mcs,
                                                     size,
                                                     (char*)value,
                                                     MEDIA_PLAYER_URL_SIZE,
                                                     type);
        }

        case MCS_SRV_TRACK_CHANGED:
        {
            GATT_MCS_SERVER_ERROR("/nGMCS: Track Changed characteristic is not writable/n");
            mcsServerNotifyConnectedClients(mcs, type);
            return TRUE;
        }

        case MCS_SRV_TRACK_TITLE:
        {
            return mcsServerUpdateCharacteristicString(mcs,
                                                 size,
                                                 (char*)value,
                                                 TRACK_TITLE_SIZE,
                                                 type);
        }

        case MCS_SRV_TRACK_DURATION:
        {
           int32 trackDuration = 0;

           if (size != 4 )
           {
               GATT_MCS_SERVER_ERROR("/nGMCS: Invalid Data/n");
               return FALSE;
           }
           /* convert byte stream into int32 data*/
           trackDuration = (int32)((value[0] & 0x000000FF)|(value[1] & 0x0000FF00)|
                                          (value[2] & 0x00FF0000) | (value[3] & 0xFF000000));
           mcs->data.trackDuration = trackDuration;

           /* Notify all connected Clients */
           mcsServerNotifyConnectedClients(mcs, type);
           return TRUE;
        }

        case MCS_SRV_TRACK_POSITION:
        {
            return mcsServerUpdateTrackPosition(mcs, size, value);
        }

        case MCS_SRV_PLAYBACK_SPEED:
        {
            int8 playbackSpeed = 0;

            if (size != 1 )
            {
                GATT_MCS_SERVER_ERROR("/nGMCS: Invalid Playback Speed/n");
                return FALSE;
            }

            /* convert byte into int8 data*/
            playbackSpeed = (int8)value[0];

            /* If new value is same as old value, do nothing else update*/
            if (playbackSpeed == mcs->data.playbackSpeed)
            {
                return TRUE;
            }

            mcs->data.playbackSpeed = playbackSpeed;

            /* Notify all connected Clients */
            mcsServerNotifyConnectedClients(mcs, type);
            return TRUE;
        }

        case MCS_SRV_SEEKING_SPEED:
        {
            int8 seekingSpeed = 0;

            if (size != 1 )
            {
                GATT_MCS_SERVER_ERROR("/nGMCS: Invalid Playback Speed/n");
                return FALSE;
            }
            /* convert byte stream into int32 data*/
            seekingSpeed = (int8)value[0];
            mcs->data.seekingSpeed = seekingSpeed;

            /* Notify all connected Clients */
            mcsServerNotifyConnectedClients(mcs, type);
            return TRUE;
        }

        case MCS_SRV_PLAYING_ORDER:
        {
            GattMcsMediaPlayingOrderType playingOrder = 0;

            if (size != 1 )
            {
                GATT_MCS_SERVER_ERROR("/nGMCS: Invalid data/n");
                return FALSE;
            }

            /* convert byte stream into int8 data*/
            playingOrder = (GattMcsMediaPlayingOrderType)value[0];

            /* If new value is same as old value, do nothing*/
            if (playingOrder == mcs->data.playingOrder)
            {
                return TRUE;
            }

            /* If playing order is supported by server return TRUE*/
            if (validatePlayingOrder(mcs->data.playingOrderSupported, playingOrder))
            {
                mcs->data.playingOrder = playingOrder;

                /* Notify all connected Clients */
                mcsServerNotifyConnectedClients(mcs, type);
                return TRUE;
            }

            GATT_MCS_SERVER_ERROR("/nGMCS: Invalid playing order/n");
            return FALSE;
        }

        case MCS_SRV_PLAYING_ORDER_SUPPORTED:
        {
            GattMcsMediaSupportedPlayingOrderType playingOrderSupported = 0;

            if (size != 2 )
            {
                GATT_MCS_SERVER_ERROR("/nGMCS: Invalid playing_order_supported /n");
                return FALSE;
            }
            /* convert byte stream into uint16 data*/
            playingOrderSupported = (GattMcsMediaSupportedPlayingOrderType)((value[0] & 0x00FF) | (value[1] << 8 & 0xFF00)) ;

            /* If new value is same as old value, do nothing*/
            if (playingOrderSupported == mcs->data.playingOrderSupported)
            {
                return TRUE;
            }

            /* Reject if the playing order supported value is invalid */
            if (playingOrderSupported & INVALID_PLAYING_ORDER_SUPPORTED_RANGE)
            {
                GATT_MCS_SERVER_ERROR("/nGMCS: Invalid playing_order_supported /n");
                return FALSE;
            }

            mcs->data.playingOrderSupported = playingOrderSupported;
            return TRUE;
        }

        case MCS_SRV_MEDIA_STATE:
        {
            GattMcsMediaStateType state = 0;

            if (size != 1 )
            {
                GATT_MCS_SERVER_ERROR("/nGMCS: Invalid media state /n");
                return FALSE;
            }
            /* convert byte stream into uint8 data*/
            state = (GattMcsMediaStateType)(value[0]) ;

            /* if the state is set to INACTIVE
             *  Make track title invalid, set track duration and position to 0xFFFFFFFF
             * */

            if(state == MCS_MEDIA_STATE_INACTIVE)
            {
                mcs->data.mediaState = state;
                mcsServerNotifyConnectedClients(mcs, type);

                mcs->data.trackTitleLen = 0;
                mcs->data.trackTitle = NULL;
                mcsServerNotifyConnectedClients(mcs, MCS_SRV_TRACK_TITLE);

                mcs->data.trackDuration = 0xFFFFFFFF;
                mcsServerNotifyConnectedClients(mcs, MCS_SRV_TRACK_DURATION);

                mcs->data.trackPosition = 0xFFFFFFFF;
                mcsServerNotifyConnectedClients(mcs, MCS_SRV_TRACK_POSITION);

                return TRUE;
            }

            /* Notify all connected Clients */
            mcs->data.mediaState = state;
            mcsServerNotifyConnectedClients(mcs, type);
            return TRUE;
        }

        case MCS_SRV_MEDIA_CONTROL_POINT:
        {
            return mcsServerUpdateMediaControlPoint(mcs, size, value);
        }

        case MCS_SRV_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED:
        {
            GattMcsOpcodeTypeSupported supported = 0;

            if (size != 4 )
            {
                GATT_MCS_SERVER_ERROR("/nGMCS: Invalid playing_order_supported /n");
                return FALSE;
            }
            /* convert byte stream into uint32 data*/
            supported = (GattMcsOpcodeTypeSupported)((value[0] & 0x000000FF) | (value[1]<<8 & 0x0000FF00))
                                 | (GattMcsOpcodeTypeSupported)((value[2] << 16 & 0x00FF0000) | (value[3] << 24 & 0xFF000000)) ;

            /* If new value is same as old value, do nothing*/
            if (supported == mcs->data.supportedOpcode)
            {
                return TRUE;
            }

            /* reject if the value is invalid*/
            if (supported & INVALID_CONTROL_POINT_OPCODE_SUPPORTED_RANGE)
            {
                GATT_MCS_SERVER_ERROR("/nGMCS: Invalid control point opcode /n");
                return FALSE;
            }

            mcs->data.supportedOpcode = supported;

            /* Notify all connected Clients */
            mcsServerNotifyConnectedClients(mcs, type);
            return TRUE;
        }

        case MCS_SRV_CONTENT_CONTROL_ID:
        {
            uint8 contentControlId;

            if (size != 1 )
            {
                GATT_MCS_SERVER_ERROR("/nGMCS: Invalid content control Id /n");
                return FALSE;
            }
            /* convert byte stream into uint8 data*/
            contentControlId = (uint8)value[0];

            /* If new value is same as old value, do nothing*/
            if (contentControlId == mcs->data.contentControlId)
            {
                return TRUE;
            }

            mcs->data.contentControlId = contentControlId;

            return TRUE;
        }

        default:
        {
             GATT_MCS_SERVER_ERROR("\n GMCS: Unsupported characteristic written on Server \n");
        }
    }
    return FALSE;
}

/******************************************************************************/
bool GattMcsServerMediaControlPointWriteResponse(ServiceHandle srvcHndl,
                                                 ConnectionId cid,
                                                 GattMcsMediaControlPointType opcode,
                                                 GattMediaControlPointAccessResultCode result)
{
    GMCS_T* mcs = (GMCS_T*)ServiceHandleGetInstanceData(srvcHndl);

    if(mcs == NULL)
    {
        GATT_MCS_SERVER_ERROR("\n GMCS: NULL instance \n");
        return FALSE;
    }

    sendMcsMediaControlPointAccessResponse(mcs,
                                           cid,
                                           opcode,
                                           result);
    return TRUE;
}

/******************************************************************************/
bool GattMcsServerMediaAttributeResponse(ServiceHandle srvcHndl,
                                         ConnectionId cid,
                                         GattMcsMediaControlPointType opcode,
                                         GattMediaControlPointAccessResultCode result)
{
    bool status = TRUE;
    status = GattMcsServerMediaControlPointWriteResponse(srvcHndl,
                                                         cid,
                                                         opcode,
                                                         result);

    status &= GattMcsServerSetMediaPlayerAttribute(srvcHndl, sizeof(opcode), &opcode, MCS_SRV_MEDIA_CONTROL_POINT);

    return status;
}

/******************************************************************************/
bool GattMcsServerSetCurrentTrack(ServiceHandle srvcHndl,
                                  uint16 titleLen,
                                  char* title,
                                  int32 duration)
{

    GMCS_T* mcs = (GMCS_T*)ServiceHandleGetInstanceData(srvcHndl);

    if (mcs == NULL)
    {
        GATT_MCS_SERVER_ERROR("\n GMCS: NULL instance \n");
        return FALSE;
    }

    /* Notify the track change */
    mcsServerNotifyConnectedClients(mcs, MCS_SRV_TRACK_CHANGED);

    /* Set the new track title */
    if(!GattMcsServerSetMediaPlayerAttribute(srvcHndl, titleLen, (uint8*)title, MCS_SRV_TRACK_TITLE))
    {
        GATT_MCS_SERVER_ERROR("\n GMCS: Track title updation failed \n");
        return FALSE;
    }

    /* Set new track duration  and notify*/
    mcs->data.trackDuration = duration;
    mcsServerNotifyConnectedClients(mcs, MCS_SRV_TRACK_DURATION);

    /* set new track position and notify*/
    mcs->data.trackPosition = 0;
    mcsServerNotifyConnectedClients(mcs, MCS_SRV_TRACK_POSITION);

    /* Set new media state */
    mcs->data.mediaState = MCS_MEDIA_STATE_PAUSED;
    mcsServerNotifyConnectedClients(mcs, MCS_SRV_MEDIA_STATE);

    return TRUE;
}

/******************************************************************************/
bool GattMcsServerSetAbsoluteTrackPosition(ServiceHandle srvcHndl, int32 newPos)
{
    GMCS_T* mcs = (GMCS_T*)ServiceHandleGetInstanceData(srvcHndl);

    if (mcs == NULL)
    {
        GATT_MCS_SERVER_ERROR("/nGMCS: NULL instance encountered/n");
        return FALSE;
    }

    if (mcs->data.mediaState == MCS_MEDIA_STATE_INACTIVE)
    {
        mcs->data.trackPosition = 0xFFFFFFFF;
        return TRUE;
    }

    /* Set New track Position */
    if (newPos < 0)
    {
        mcs->data.trackPosition = 0;
    }
    else if (newPos > mcs->data.trackDuration)
    {
        mcs->data.trackPosition = mcs->data.trackDuration;
    }
    else
    {
        mcs->data.trackPosition = newPos;
    }

    /* Notify Clients*/
    mcsServerNotifyConnectedClients(mcs, MCS_SRV_TRACK_POSITION);

    return TRUE;
}


/******************************************************************************/
/* MCS server init Synergy Scheduler Task */

void gattMcsServerInit(void** gash)
{
    *gash = &mcsServiceHandle;
    GATT_MCS_SERVER_INFO("GMCS: gattMcsServerInit\n\n");
}

/******************************************************************************/
/* MCS server De-init Synergy Scheduler Task */

#ifdef ENABLE_SHUTDOWN

void gatt_mcs_server_deinit(void** gash)
{
    ServiceHandle srvcHndl = *((ServiceHandle*)*gash);

    if (srvcHndl)
    {
        if (ServiceHandleFreeInstanceData(srvcHndl))
        {
            GATT_MCS_SERVER_INFO("GMCS: gatt_mcs_server_deinit\n\n");
        }
        else
        {
            GATT_MCS_SERVER_PANIC("GMCS ERROR: Unable to free the GMCS server instance\n");
        }
    }
    else
    {
        GATT_MCS_SERVER_DEBUG("GMCS: Invalid Service Handle\n\n");
    }
}
#endif
