/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #10 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_gatt_private.h"

#ifdef GATT_DATA_LOGGER
#include "csr_bt_addr.h"
#include "csr_bt_tasks.h"
#include "gatt_data_logger.h"
#include "gatt_mcs_client.h"
#include "gatt_telephone_bearer_client.h"
#include "gatt_data_logger_lib.h"

typedef struct
{
    CsrBtGattHandle     attributeHandle;        /* Relative attribute handle */
    CsrBtUuid16         uuid;                   /* value of the UUID for attribute handle */
}gattUuidHandle;

/**
 * Array size (of first dimension if multi-dimension)
 */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define GATT_INVALID_UUID    0x0000

extern GattMainInst *gattMainInstPtr;
CsrBtGattQueueElement *restoreElement = NULL;
static bool readType = FALSE;

CsrBool CsrBtGattAppInstFindLoggerApps(CsrCmnListElm_t *elem, void *value);

static const gattUuidHandle gattAscsServerUuidHandleTable[] =
{
    /* ASCS specific entry ref: gatt_ascs_server_dh.h & gatt_ascs_server_uuid.h */
    {
        .attributeHandle     = 0x0003, /* HANDLE_ASCS_ASE_CHAR_1 */
        .uuid                = 0x2BC4, /* Sink ASE */
    },
    {
        .attributeHandle     = 0x0006, /* HANDLE_ASCS_ASE_CHAR_2 */
        .uuid                = 0x2BC4, /* Sink ASE */
    },
    {
        .attributeHandle     = 0x0009, /* HANDLE_ASCS_ASE_CHAR_3 */
        .uuid                = 0x2BC4, /* Sink ASE */
    },
    {
        .attributeHandle     = 0x000c, /* HANDLE_ASCS_ASE_CHAR_4 */
        .uuid                = 0x2BC4, /* Sink ASE */
    },
    {
        .attributeHandle     = 0x000f, /* HANDLE_ASCS_ASE_CHAR_5 */
        .uuid                = 0x2BC5, /* Source ASE */
    },
    {
        .attributeHandle     = 0x0012, /* HANDLE_ASCS_ASE_CHAR_6 */
        .uuid                = 0x2BC5, /* Source ASE */
    },
    {
        .attributeHandle     = 0x0015, /* HANDLE_ASCS_ASE_CONTROL_POINT_CHAR */
        .uuid                = 0x2BC6, /* ASE Control Point ASE */
    },
};

const gattUuidHandle gattBassServerUuidHandleTable[] =
{
    /* BASS specific entry gatt_bass_server_dh.h & gatt_bass_server_uuid.h*/
    {
        .attributeHandle     = 0x0003, /* HANDLE_BROADCAST_AUDIO_SCAN_CONTROL_POINT */
        .uuid                = 0x2BC7, /* Broadcast Audio Scan Control Point */
    },
    {
        .attributeHandle     = 0x0005, /* HANDLE_BASS_BROADCAST_RECEIVE_STATE_1 */
        .uuid                = 0x2BC8, /* Broadcast Receive State */
    },
    {
        .attributeHandle     = 0x0008, /* HANDLE_BASS_BROADCAST_RECEIVE_STATE_2 */
        .uuid                = 0x2BC8, /* Broadcast Receive State */
    },
};

const gattUuidHandle gattPacsServerUuidHandleTable[] =
{
    /* PACS specific entry gatt_pacs_server_dh.h & gatt_pacs_server_uuid.h */
    {
        .attributeHandle     = 0x0003, /* HANDLE_SINK_PAC_1 */
        .uuid                = 0x2BC9, /* UUID_SINK_PAC */
    },
    {
        .attributeHandle     = 0x0006, /* HANDLE_SINK_PAC_1 */
        .uuid                = 0x2BC9, /* UUID_SINK_PAC */
    },
    {
        .attributeHandle     = 0x0009, /* HANDLE_SINK_PAC_1 */
        .uuid                = 0x2BC9, /* UUID_SINK_PAC */
    },
    {
        .attributeHandle     = 0x000c, /* HANDLE_SINK_PAC_1 */
        .uuid                = 0x2BC9, /* UUID_SINK_PAC */
    },
    {
        .attributeHandle     = 0x000f, /* HANDLE_SINK_PAC_1 */
        .uuid                = 0x2BCA, /* UUID SINK AUDIO location */
    },
    {
        .attributeHandle     = 0x0012, /* HANDLE_SOURCE_PAC_1 */
        .uuid                = 0x2BCB, /* UUID_SOURCE_PAC */
    },
    {
        .attributeHandle     = 0x0015, /* HANDLE_SOURCE_PAC_2 */
        .uuid                = 0x2BCB, /* UUID_SOURCE_PAC */
    },
    {
        .attributeHandle     = 0x0018, /* HANDLE_SOURCE_PAC_3 */
        .uuid                = 0x2BCB, /* UUID_SOURCE_PAC */
    },
    {
        .attributeHandle     = 0x001b, /* HANDLE_SOURCE_PAC_APTX */
        .uuid                = 0x2BCB, /* UUID_SOURCE_PAC */
    },
    {
        .attributeHandle     = 0x001e, /* HANDLE_SOURCE_AUDIO_LOCATIONS */
        .uuid                = 0x2BCC, /* UUID SOURCE AUDIO location */
    },
    {
        .attributeHandle     = 0x0021, /* HANDLE_AVAILABLE_AUDIO_CONTEXTS */
        .uuid                = 0x2BCD, /* UUID available audio context  */
    },
    {
        .attributeHandle     = 0x0024, /* HANDLE_SUPPORTED_AUDIO_CONTEXTS */
        .uuid                = 0x2BCE, /* UUID Supported audio context */
    },
};

const gattUuidHandle gattVcsServerUuidHandleTable[] =
{
    /* VCS specific entry gatt_vcs_server_dh.h & gatt_vcs_server_uuid.h */
    {
        .attributeHandle     = 0x0003, /* HANDLE_VOLUME_STATE */
        .uuid                = 0x2B7D, /* UUID VOLUME_STATE */
    },
    {
        .attributeHandle     = 0x0006, /* HANDLE_VOLUME_CONTROL_POINT */
        .uuid                = 0x2B7E, /* UUID VOLUME_CONTROL_POINT */
    },
    {
        .attributeHandle     = 0x0008, /* HANDLE_VOLUME_FLAGS */
        .uuid                = 0x2B7F, /* UUID VOLUME_FLAGS */
    },
};

const gattUuidHandle gattMicsServerUuidHandleTable[] =
{
    /* MICS specific entry gatt_mics_server_dh.h & gatt_mics_server_uuid.h */
    {
        .attributeHandle     = 0x0003, /* HANDLE_MUTE */
        .uuid                = 0x2BC3, /* UUID MUTE */
    },
};


/* Client UUID handle table placeholder, Invalid handle to be updated once Client
   discovered the Atttibute handle from remote server */
gattUuidHandle gattGmcsClientUuidHandleTable[] =
{
    /* GMCS specific entry */
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE  */
        .uuid                = 0x2B93, /* GATT_GMCS_UUID_MEDIA_PLAYER_NAME */
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B94, /* GATT_GMCS_UUID_MEDIA_PLAYER_ICON_OBJ_ID */
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B95, /* GATT_GMCS_UUID_MEDIA_PLAYER_ICON_URL*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B96, /* GATT_GMCS_UUID_TRACK_CHANGED*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B97, /* GATT_GMCS_UUID_TRACK_TITLE*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B98, /* GATT_GMCS_UUID_TRACK_DURATION*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B99, /* GATT_GMCS_UUID_TRACK_POSITION*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B9A, /* GATT_GMCS_UUID_PLAYBACK_SPEED*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B9B, /* GATT_GMCS_UUID_SEEKING_SPEED*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B9C, /* GATT_GMCS_UUID_CURRENT_TRACK_SEGMENTS_OBJ_ID*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B9D, /* GATT_GMCS_UUID_CURRENT_TRACK_OBJ_ID*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B9E, /* GATT_GMCS_UUID_NEXT_TRACK_OBJ_ID*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BA0, /* GATT_GMCS_UUID_CURRENT_GROUP_OBJ_ID*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2B9F, /* GATT_GMCS_UUID_PARENT_GROUP_OBJ_ID*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BA1, /* GATT_GMCS_UUID_PLAYING_ORDER*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BA2, /* GATT_GMCS_UUID_PLAYING_ORDER_SUPPORTED*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BA3, /* GATT_GMCS_UUID_MEDIA_STATE*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BA4, /* GATT_GMCS_UUID_MEDIA_CONTROL_POINT*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BA5, /* GATT_GMCS_UUID_MEDIA_CONTROL_POINT_OP_SUPP*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BA6, /* GATT_GMCS_UUID_SEARCH_RESULTS_OBJ_ID*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BA7, /* GATT_GMCS_UUID_SEARCH_CONTROL_POINT*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BBA, /* GATT_GMCS_UUID_MEDIA_CONTENT_CONTROL_ID*/
    },
};

/* Client UUID handle table placeholder, Invalid handle to be updated once Client
   discovered the Atttibute handle from remote server */
gattUuidHandle gattGtbsClientUuidHandleTable[] =
{
    /* GTBS entry */
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BB3, /* GATT_TBS_UUID_BEARER_PROVIDER_NAME*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BB4, /* GATT_TBS_UUID_BEARER_UCI*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BB5, /* GATT_TBS_UUID_BEARER_TECHNOLOGY*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BB6, /* GATT_TBS_UUID_BEARER_URI_PREFIX_LIST*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BB7, /* GATT_TBS_UUID_SIGNAL_STRENGTH*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BB8, /* GATT_TBS_UUID_SIGNAL_STRENGTH_REPORTING_INTERVAL*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BB9, /* GATT_TBS_UUID_LIST_CURRENT_CALLS*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BBA, /* GATT_TBS_UUID_CONTENT_CONTROL_ID*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BBB, /* GATT_TBS_UUID_STATUS_FLAGS*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BBC, /* GATT_TBS_UUID_INCOMING_CALL_TARGET_BEARER_URI*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BBD, /* GATT_TBS_UUID_CALL_STATE*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BBE, /* GATT_TBS_UUID_CALL_CONTROL_POINT*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BBF, /* GATT_TBS_UUID_CALL_CONTROL_POINT_OPCODES*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BC0, /* GATT_TBS_UUID_TERMINATION_REASON*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BC1, /* GATT_TBS_UUID_INCOMING_CALL*/
    },
    {
        .attributeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID, /* HANDLE */
        .uuid                = 0x2BC2, /* GATT_TBS_UUID_REMOTE_FRIENDLY_NAME*/
    },
};

CsrBool CsrBtGattAppInstFindLoggerApps(CsrCmnListElm_t *elem, void *value)
{ /* Returns TRUE if appInst "elem" has given gattId "value" */
    CsrBtGattAppElement *element = (CsrBtGattAppElement *)elem;
    CSR_UNUSED(value);

    return ((CSR_MASK_IS_SET(element->eventMask, GATT_EVENT_MASK_SUBSCRIBE_DATA_LOGGER)) ? TRUE : FALSE);
}

#define CSR_BT_GATT_APP_INST_FIND_LOGGER_APP(_appList ) \
    ((CsrBtGattAppElement *)CsrCmnListSearch(&(_appList), \
                                             CsrBtGattAppInstFindLoggerApps, \
                                             NULL))


static uint8 gattReadUint8(uint8 **buf)
{
    return 0xFF & *((*buf)++);
}

static uint16 gattReadUint16(uint8 **buf)
{
    uint16 valLow = gattReadUint8(buf);
    uint16 valHigh = gattReadUint8(buf);

    return valLow | (valHigh << 8);
}

static uint16 gattFindServerUuidForAttributeHandle(CsrSchedQid intfaceQueue, CsrBtGattHandle attributeHandle)
{
    uint8 i = 0;
    uint8 maxIndex = 0;
    const gattUuidHandle *handleUuidTable = NULL;

    if (intfaceQueue == CSR_BT_ASCS_SERVER_IFACEQUEUE)
    {
        handleUuidTable = gattAscsServerUuidHandleTable;
        maxIndex = ARRAY_SIZE(gattAscsServerUuidHandleTable);
    }
    else if (intfaceQueue == CSR_BT_BASS_SERVER_IFACEQUEUE)
    {
        handleUuidTable = gattBassServerUuidHandleTable;
        maxIndex = ARRAY_SIZE(gattBassServerUuidHandleTable);
    }
    else if (intfaceQueue == CSR_BT_PACS_SERVER_IFACEQUEUE)
    {
        handleUuidTable = gattPacsServerUuidHandleTable;
        maxIndex = ARRAY_SIZE(gattPacsServerUuidHandleTable);
    }
    else if (intfaceQueue == CSR_BT_VCS_SERVER_IFACEQUEUE)
    {
        handleUuidTable = gattVcsServerUuidHandleTable;
        maxIndex = ARRAY_SIZE(gattVcsServerUuidHandleTable);
    }    
    else if (intfaceQueue == CSR_BT_MICS_SERVER_IFACEQUEUE)
    {
        handleUuidTable = gattMicsServerUuidHandleTable;
        maxIndex = ARRAY_SIZE(gattMicsServerUuidHandleTable);
    }
    else if (intfaceQueue == CSR_BT_MCS_CLIENT_IFACEQUEUE)
    {
        handleUuidTable = gattGmcsClientUuidHandleTable;
        maxIndex = ARRAY_SIZE(gattGmcsClientUuidHandleTable);
    }
    else if (intfaceQueue == CSR_BT_TBS_CLIENT_IFACEQUEUE)
    {
        handleUuidTable = gattGtbsClientUuidHandleTable;
        maxIndex = ARRAY_SIZE(gattGtbsClientUuidHandleTable);
    }
    else
    {
        return GATT_INVALID_UUID;
    }

    /* Find the UUID value for the attribute handle */
    for( i = 0; i< maxIndex; i++)
    {
        if(attributeHandle ==  handleUuidTable[i].attributeHandle) 
        {
            CSR_LOG_TEXT_DEBUG((CsrBtGattLto, 0, "gattFindServerUuidForAttributeHandle uuid=0x%x AttHandle = 0x%x, InteQueue=0x%x\n", handleUuidTable[i].uuid, attributeHandle, intfaceQueue));
            return handleUuidTable[i].uuid;
        }
    }
    return GATT_INVALID_UUID;
}


static void gattDataLoggerIndSend(CsrSchedQid qHandle,
                                CsrBtConnId btConnId,
                                CsrBtGattHandle handle,
                                uint8 OperationType,
                                bool locallyOriginated,
                                uint8 dataLength,
                                uint8 *data)
{
    GattMainInst *inst = gattMainInstPtr;
    CsrBtUuid16 uuid16 = GATT_INVALID_UUID;

    /* Find the appElmnt registered for logger  */
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_LOGGER_APP(inst->appInst);

    if(appElement == NULL)
    {
        return;
    }

    /* Traverse the Handle UUID table to see if there is any matching UUID for attribute handle */
    uuid16 = gattFindServerUuidForAttributeHandle( qHandle, handle);

    if(uuid16 != GATT_INVALID_UUID)
    {
        GattUuidDataInd *msg = CsrPmemZalloc(sizeof(GattUuidDataInd));

        if(msg)
        {

            msg->type = GATT_UUID_DATA_IND;
            msg->btConnId = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
            msg->operationType = OperationType;
            msg->locallyOriginated = locallyOriginated;

            /* conversion of 16 bit UUID to CsrBtUuid */
            CsrMemSet(msg->uuid.uuid, 0, CSR_BT_UUID128_SIZE);
            msg->uuid.length = (CsrUint16) (sizeof(CsrBtUuid16));
            CSR_COPY_UINT16_TO_LITTLE_ENDIAN(uuid16, msg->uuid.uuid);

            msg->dataLength = dataLength;
            if(dataLength && data)
            {
                msg->data = CsrPmemAlloc(dataLength * sizeof(uint8));
                if(msg->data)
                {
                    SynMemCpyS(msg->data, (CsrSize)msg->dataLength, data, (CsrSize)dataLength);
                }
            }

            /* Send to registered Application for data logger */
            CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(appElement->gattId), msg);
        }
    }
}

void gattDataLoggerUpstreamMsgIndSend( CsrSchedQid phandle, void *msg )
{
    CsrBtGattPrim id = *(CsrBtGattPrim*)msg;
    CsrBtConnId btConnId = CSR_BT_CONN_ID_INVALID;
    CsrBtGattHandle handle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
    GattOpType operationType = GATT_READ_REQUEST;
    bool  locallyOriginated = FALSE;
    uint8 dataLength = 0;
    uint8 *data = NULL;
    CsrBtGattAppElement *appElement = NULL;

    appElement = CSR_BT_GATT_APP_INST_FIND_LOGGER_APP(gattMainInstPtr->appInst);

    if(appElement == NULL)
    {
        CSR_LOG_TEXT_DEBUG((CsrBtGattLto, 0, "gattDataLoggerUpstreamMsgIndSend No logger App \n"));
        return;
    }

    switch(id)
    {
        /* Server Upstream prim */
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            CsrBtGattDbAccessReadInd *message = (CsrBtGattDbAccessReadInd*)msg;
            btConnId = message->btConnId;
            handle = message->attrHandle;
            operationType = GATT_READ_REQUEST;
            readType = TRUE;
        }
        break;
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            CsrBtGattDbAccessWriteInd *message = (CsrBtGattDbAccessWriteInd*)msg;
            btConnId = message->btConnId;
            handle = message->attrHandle;
            operationType = GATT_WRITE_REQUEST;
            readType = FALSE;

            if(message->writeUnitCount == 1)
            {
                dataLength = (uint8)message->writeUnit->valueLength;
                data =  message->writeUnit->value;
            }
            else if(message->writeUnitCount > 1)
            {
                uint8 i = 0;
                operationType = GATT_WRITE_REQUEST_MORE;

                /* Write long handling */
                for(i = 0; i< message->writeUnitCount; i++)
                {
                    if((i+1) == message->writeUnitCount)
                        operationType = GATT_WRITE_REQUEST;
                    dataLength = (uint8)message->writeUnit[i].valueLength;
                    data =  message->writeUnit[i].value;

                    gattDataLoggerIndSend( phandle, btConnId, handle, operationType, locallyOriginated,
                                 dataLength, data);
                }
                return;
            }
        }
        break;
        /* Client upstream prim */
        case CSR_BT_GATT_READ_CFM:
        {
            CsrBtGattReadCfm *message = (CsrBtGattReadCfm*)msg;
            btConnId = message->btConnId;
            handle = message->handle;
            operationType = GATT_READ_RESPONSE;
            locallyOriginated = TRUE;
            dataLength = (uint8)message->valueLength;
            data =  message->value;
        }
        break;
        case CSR_BT_GATT_WRITE_CFM:
        {
            CsrBtGattWriteCfm *message = (CsrBtGattWriteCfm*)msg;
            btConnId = message->btConnId;
            handle = message->handle;
            operationType = GATT_WRITE_RESPONSE;
            locallyOriginated = TRUE;
        }
        break;
        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            CsrBtGattClientNotificationInd *message = (CsrBtGattClientNotificationInd*)msg;
            btConnId = message->btConnId;
            handle = message->valueHandle;
            operationType = GATT_NOTIFICATION;
            locallyOriginated = TRUE;
            dataLength = (uint8)message->valueLength;
            data =  message->value;
        }
        break;

        case CSR_BT_GATT_DISCOVER_SERVICES_IND:
        {
            CsrBtGattDiscoverServicesInd *message = (CsrBtGattDiscoverServicesInd*)msg;

            /*  Current requirement is only for 16 bit UUID */
            if(message->uuid.length == CSR_BT_UUID16_SIZE)
            {
                GattUuidDataInd *msgs = CsrPmemZalloc(sizeof(GattUuidDataInd));

                if(msgs)
                {
                    msgs->type = GATT_UUID_DATA_IND;
                    msgs->btConnId = (message->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
                    msgs->operationType = GATT_DISCOVER_SERVICES;
                    msgs->locallyOriginated = FALSE;

                    /* conversion of 16 bit UUID to CsrBtUuid */
                    CsrMemSet(msgs->uuid.uuid, 0, CSR_BT_UUID128_SIZE);
                    msgs->uuid.length = (CsrUint16) (sizeof(CsrBtUuid16));
                    SynMemCpyS(msgs->uuid.uuid, (CsrSize)msgs->uuid.length,
                            message->uuid.uuid, (CsrSize)message->uuid.length);

                    /* Send to registered Application for data logger */
                    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(appElement->gattId), msgs);
                }
            }
            return;
        }
        default:
        {
            return;
        }
    }

    gattDataLoggerIndSend( phandle, btConnId, handle, operationType, locallyOriginated,
                                 dataLength, data);
}

void gattDataLoggerUpstreamAttDebugIndSend( void *msg )
{
    ATT_DEBUG_IND_T    *prim     = (ATT_DEBUG_IND_T *) msg;
    CsrBtGattAppElement *appElement = NULL;

    appElement = CSR_BT_GATT_APP_INST_FIND_LOGGER_APP(gattMainInstPtr->appInst);
    if(appElement == NULL)
    {
        CSR_LOG_TEXT_DEBUG((CsrBtGattLto, 0, "gattDataLoggerUpstreamAttDebugIndSend No logger App \n"));
        return;
    }
    else if(prim->debug && prim->size_debug)
    {
        uint8 *ptr = (uint8 *)prim->debug;
        GattAttDebugInfo debugInfo;
        GattUuidDataInd *msgs = CsrPmemZalloc(sizeof(GattUuidDataInd));

        debugInfo.cid = (uint16)gattReadUint16(&ptr);
        debugInfo.connectionHandle = (uint16)gattReadUint16(&ptr);
        debugInfo.locallyOriginated= (uint8)gattReadUint8(&ptr);
        debugInfo.valueLength = (uint16)gattReadUint16(&ptr);

        msgs->type = GATT_UUID_DATA_IND;
        msgs->btConnId = (debugInfo.connectionHandle & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
        msgs->operationType = GATT_ATT_ERROR_RSP;
        msgs->locallyOriginated = debugInfo.locallyOriginated ? TRUE : FALSE;
        CsrMemSet(msgs->uuid.uuid, 0, CSR_BT_UUID128_SIZE);
        msgs->uuid.length = 0;
        msgs->dataLength = (uint8)debugInfo.valueLength;
        if(debugInfo.valueLength)
        {
            msgs->data = CsrPmemAlloc(msgs->dataLength * sizeof(uint8));
            SynMemCpyS(msgs->data, (CsrSize)msgs->dataLength,
                        ptr, (CsrSize)debugInfo.valueLength);
        }

        /* Send to registered Application for data logger */
        CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(appElement->gattId), msgs);
    }
}

static void gattDataLoggerDownstreamMsgIndSend(void)

{
    GattMainInst *inst = gattMainInstPtr;
    CsrBtGattPrim id = 0;
    CsrBtGattId    gattId = CSR_BT_GATT_INVALID_GATT_ID;
    CsrBtConnId btConnId = CSR_BT_CONN_ID_INVALID;
    CsrBtGattHandle handle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
    GattOpType operationType = GATT_READ_REQUEST;
    bool  locallyOriginated = FALSE;
    uint8 dataLength = 0;
    uint8 *data = NULL;
    CsrBtGattAppElement *appElement = NULL;
    void  *gattMsg = inst->msg;

    appElement = CSR_BT_GATT_APP_INST_FIND_LOGGER_APP(inst->appInst);
    if(appElement == NULL)
    {
        CSR_LOG_TEXT_DEBUG((CsrBtGattLto, 0, "gattDataLoggerDownstreamMsgIndSend No logger App \n"));
        return;
    }

    if((inst->msg == NULL) && (restoreElement == NULL))
    {
        return;
    }
    
    if(restoreElement && restoreElement->gattMsg)
    {
        gattMsg = restoreElement->gattMsg;
        restoreElement = NULL;
    }

    if(gattMsg == NULL)
    {
        return;
    }

    id = *(CsrBtGattPrim*)gattMsg;

    switch(id)
    {
        /* Server Downstream prim */
        case CSR_BT_GATT_DB_ACCESS_RES:
        {
            CsrBtGattDbAccessRes *message = (CsrBtGattDbAccessRes *)gattMsg;
            gattId = message->gattId;
            btConnId = message->btConnId;
            handle = message->attrHandle;
            operationType = readType ? GATT_READ_RESPONSE : GATT_WRITE_RESPONSE;
            dataLength = (uint8)message->valueLength;
            data =  message->value;
        }
        break;
        case CSR_BT_GATT_EVENT_SEND_REQ:
        {
            CsrBtGattEventSendReq *message = (CsrBtGattEventSendReq *)gattMsg;
            gattId = message->gattId;
            btConnId = message->btConnId;
            handle = message->attrHandle;
            operationType = GATT_NOTIFICATION;
            dataLength = (uint8)message->valueLength;
            data =  message->value;
        }
        break;

        default:
            return;
            
    }

    gattDataLoggerIndSend( CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), btConnId, handle, operationType,
                                 locallyOriginated, dataLength, data);
}

void GattDataLoggerSendPrim(void)
{
    gattDataLoggerDownstreamMsgIndSend();
}

bool GattDataLoggerRegisterClientHandles(CsrSchedQid phandle, CsrBtConnId btConnId, void *clientHandles)
{
    bool status = FALSE;

    /* btConnId to used in future for multipoint support */
    CSR_UNUSED(btConnId);

    if(phandle == CSR_BT_MCS_CLIENT_IFACEQUEUE)
    {
        GattMcsClientDeviceData *mcsClientHandles = (GattMcsClientDeviceData *)clientHandles;

        gattGmcsClientUuidHandleTable[0].attributeHandle = mcsClientHandles->mediaPlayerNameHandle;
        gattGmcsClientUuidHandleTable[1].attributeHandle = mcsClientHandles->mediaPlayerIconObjIdHandle;
        gattGmcsClientUuidHandleTable[2].attributeHandle = mcsClientHandles->mediaPlayerIconUrlHandle;
        gattGmcsClientUuidHandleTable[3].attributeHandle = mcsClientHandles->trackChangedHandle;
        gattGmcsClientUuidHandleTable[4].attributeHandle = mcsClientHandles->trackTitleHandle;
        gattGmcsClientUuidHandleTable[5].attributeHandle = mcsClientHandles->trackDurationHandle;
        gattGmcsClientUuidHandleTable[6].attributeHandle = mcsClientHandles->trackPositionHandle;
        gattGmcsClientUuidHandleTable[7].attributeHandle = mcsClientHandles->playbackSpeedHandle;
        gattGmcsClientUuidHandleTable[8].attributeHandle = mcsClientHandles->seekingSpeedHandle;
        gattGmcsClientUuidHandleTable[9].attributeHandle = mcsClientHandles->currentTrackSegmentsObjIdHandle;
        gattGmcsClientUuidHandleTable[10].attributeHandle = mcsClientHandles->currentTrackObjIdHandle; 
        gattGmcsClientUuidHandleTable[11].attributeHandle = mcsClientHandles->nextTrackObjIdHandle;
        gattGmcsClientUuidHandleTable[12].attributeHandle = mcsClientHandles->currentGroupObjIdHandle;
        gattGmcsClientUuidHandleTable[13].attributeHandle = mcsClientHandles->parentGroupObjIdHandle;
        gattGmcsClientUuidHandleTable[14].attributeHandle = mcsClientHandles->playingOrderHandle;
        gattGmcsClientUuidHandleTable[15].attributeHandle = mcsClientHandles->playingOrderSuppHandle;
        gattGmcsClientUuidHandleTable[16].attributeHandle = mcsClientHandles->mediaStateHandle;
        gattGmcsClientUuidHandleTable[17].attributeHandle = mcsClientHandles->mediaControlPointHandle;
        gattGmcsClientUuidHandleTable[18].attributeHandle = mcsClientHandles->mediaControlPointOpSuppHandle;
        gattGmcsClientUuidHandleTable[19].attributeHandle = mcsClientHandles->searchResultsObjIdHandle;
        gattGmcsClientUuidHandleTable[20].attributeHandle = mcsClientHandles->searchControlPointHandle;
        gattGmcsClientUuidHandleTable[21].attributeHandle = mcsClientHandles->contentControlIdHandle;

        status = TRUE;
    }
    else if(phandle == CSR_BT_TBS_CLIENT_IFACEQUEUE)
    {
        GattTelephoneBearerClientDeviceData *gtbsClientHandles = (GattTelephoneBearerClientDeviceData *)clientHandles;

        gattGtbsClientUuidHandleTable[0].attributeHandle = gtbsClientHandles->bearerNameHandle;
        gattGtbsClientUuidHandleTable[1].attributeHandle = gtbsClientHandles->bearerUciHandle;
        gattGtbsClientUuidHandleTable[2].attributeHandle = gtbsClientHandles->bearerTechHandle;
        gattGtbsClientUuidHandleTable[3].attributeHandle = gtbsClientHandles->bearerUriPrefixListHandle;
        gattGtbsClientUuidHandleTable[4].attributeHandle = gtbsClientHandles->signalStrengthHandle;
        gattGtbsClientUuidHandleTable[5].attributeHandle = gtbsClientHandles->signalStrengthIntervalHandle;
        gattGtbsClientUuidHandleTable[6].attributeHandle = gtbsClientHandles->listCurrentCallsHandle;
        gattGtbsClientUuidHandleTable[7].attributeHandle = gtbsClientHandles->contentControlIdHandle;
        gattGtbsClientUuidHandleTable[8].attributeHandle = gtbsClientHandles->statusFlagsHandle;
        gattGtbsClientUuidHandleTable[9].attributeHandle = gtbsClientHandles->incomingTargetBearerUriHandle;
        gattGtbsClientUuidHandleTable[10].attributeHandle = gtbsClientHandles->callStateHandle;
        gattGtbsClientUuidHandleTable[11].attributeHandle = gtbsClientHandles->callControlPointHandle;
        gattGtbsClientUuidHandleTable[12].attributeHandle = gtbsClientHandles->callControlPointOptionalOpcodesHandle;
        gattGtbsClientUuidHandleTable[13].attributeHandle = gtbsClientHandles->terminationReasonHandle;
        gattGtbsClientUuidHandleTable[14].attributeHandle = gtbsClientHandles->incomingCallHandle;
        gattGtbsClientUuidHandleTable[15].attributeHandle = gtbsClientHandles->remoteFriendlyNameHandle;

        status = TRUE;
    }

    return status;
}
#endif

