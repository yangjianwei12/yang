/*******************************************************************************
Copyright (c) 2018-2022 Qualcomm Technologies International, Ltd.
 
*******************************************************************************/

#include <logging.h>

#include "gatt_fast_pair_server_private.h"
#include "gatt_fast_pair_server_msg_handler.h"

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(gatt_fps_server_message_id_t)

/***************************************************************************
NAME
    sendFastPairServerInitRsp

DESCRIPTION
    Sends response to the application
*/

static void sendFastPairServerInitRsp(const GFPS    *fast_pair_server, 
                                      gatt_fast_pair_server_init_status_t status_code)
{
    MAKE_FPS_MESSAGE(
            GATT_FAST_PAIR_SERVER_INIT_IND
            );

    message->fast_pair_server = fast_pair_server;
    message->status = status_code;

    MessageSend(fast_pair_server->app_task, GATT_FAST_PAIR_SERVER_INIT_IND, message);

}


/******************************************************************************/
bool GattFastPairServerInit(
        GFPS    *fast_pair_server,
        Task    app_task,
        uint16  start_handle,
        uint16  end_handle
        )
{
    if ((app_task == NULL) || (fast_pair_server == NULL ))
    {
        GATT_FAST_PAIR_SERVER_PANIC((
                    "GFPS: Invalid Initialization parameters"
                    ));
    }

    /* Reset all the service library memory */
    memset(fast_pair_server, 0, sizeof(GFPS));

    fast_pair_server->lib_task.handler = fpsServerMsgHandler;
    fast_pair_server->app_task = app_task;

    fast_pair_server->start_handle = start_handle;
    fast_pair_server->end_handle  = end_handle;
    
    GattRegisterReqSend(&(fast_pair_server->lib_task), 1234);
    
    return TRUE;
}

/******************************************************************************/
void fpsHandleRegisterCfm(GFPS    *fast_pair_server,
                          const CsrBtGattRegisterCfm *cfm)
{
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        fast_pair_server->gattId = cfm->gattId;
        GattFlatDbRegisterHandleRangeReqSend(fast_pair_server->gattId, fast_pair_server->start_handle, fast_pair_server->end_handle);
    }
    else
    {
        sendFastPairServerInitRsp(fast_pair_server, gatt_fast_pair_server_init_failed);
    }
}

/******************************************************************************/
void fpsHandleRegisterHandleRangeCfm(GFPS *fast_pair_server,
                                     const CsrBtGattFlatDbRegisterHandleRangeCfm *cfm)
{
    gatt_fast_pair_server_init_status_t status = gatt_fast_pair_server_init_failed;
    
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        status = gatt_fast_pair_server_init_success;
    }

    sendFastPairServerInitRsp(fast_pair_server, status);
}

/******************************************************************************/
static bool gattFastPairServerWriteGenericResponse(
        const GFPS  *fast_pair_server, 
        uint32      cid, 
        uint16      result,
        uint16      handle
        )
{
    if (fast_pair_server == NULL)
    {
        GATT_FAST_PAIR_SERVER_DEBUG_PANIC((
                    "GFPS: Null instance!\n"
                    ));
        return FALSE;
    }
    else if (cid == 0)
    {
        return FALSE;
    }

    GattDbWriteAccessResSend(fast_pair_server->gattId,
                             cid,
                             handle,
                             result);

    return TRUE;
}


/******************************************************************************/
bool GattFastPairServerWriteKeybasedPairingResponse(
        const GFPS  *fast_pair_server, 
        uint32      cid, 
        uint16      result
        )
{
    return gattFastPairServerWriteGenericResponse(
            fast_pair_server, 
            cid, 
            result,
            HANDLE_KEYBASED_PAIRING
            );
}


/******************************************************************************/
static bool gattFastPairServerGenericReadConfigResponse(
        const GFPS  *fast_pair_server, 
        uint32      cid, 
        uint16      client_config,
        uint16      handle
        )
{
    uint8 *config_data = NULL;

    if (fast_pair_server == NULL)
    {
        GATT_FAST_PAIR_SERVER_DEBUG_PANIC((
                    "GFPS: Null instance!\n"
                    ));
        return FALSE;
    }
    else if (cid == 0)
    {
        return FALSE;
    }

    config_data = (uint8*) CsrPmemAlloc(CLIENT_CONFIG_VALUE_SIZE);
    config_data[0] = (uint8)client_config & 0xFF;
    config_data[1] = (uint8)client_config >> 8;

    GattDbReadAccessResSend(fast_pair_server->gattId, 
                            cid, 
                            handle,
                            CSR_BT_GATT_ACCESS_RES_SUCCESS, 
                            CLIENT_CONFIG_VALUE_SIZE,
                            config_data);
                            
    return TRUE;
}

/******************************************************************************/
static bool gattFastPairServerReadModelIdResponse(
        const GFPS  *fast_pair_server,
        uint32      cid,
        uint32 model_id,
        uint16 handle
        )
{
    uint8 *model_id_value = NULL;

    if (fast_pair_server == NULL)
    {
        GATT_FAST_PAIR_SERVER_DEBUG_PANIC((
                    "GFPS: Null instance!\n"
                    ));
        return FALSE;
    }
    else if (cid == 0)
    {
        return FALSE;
    }

    model_id_value = (uint8*) CsrPmemAlloc(MODEL_ID_VALUE_SIZE);
    model_id_value[0] = (uint8)(model_id>>16) & 0xFF;
    model_id_value[1] = (uint8)(model_id>>8) & 0xFF;
    model_id_value[2] = (uint8)model_id & 0xFF;

    GattDbReadAccessResSend(fast_pair_server->gattId, 
                            cid, 
                            handle,
                            CSR_BT_GATT_ACCESS_RES_SUCCESS, 
                            MODEL_ID_VALUE_SIZE,
                            model_id_value);

    return TRUE;
}


/******************************************************************************/
bool GattFastPairServerReadModelIdResponse(
        const GFPS  *fast_pair_server,
        uint32      cid,
        uint32 model_id
        )
{
    return gattFastPairServerReadModelIdResponse(
            fast_pair_server,
            cid,
            model_id,
            HANDLE_MODEL_ID
            );
}

/******************************************************************************/
bool GattFastPairServerReadKeybasedPairingConfigResponse(
        const GFPS  *fast_pair_server,
        uint32      cid,
        uint16      client_config
        )
{
    return gattFastPairServerGenericReadConfigResponse(
            fast_pair_server,
            cid,
            client_config,
            HANDLE_KEYBASED_PAIRING_CLIENT_CONFIG
            );
}


/******************************************************************************/
bool GattFastPairServerWriteKeybasedPairingConfigResponse(
        const GFPS  *fast_pair_server, 
        uint32      cid, 
        uint16      result
        )
{
    return gattFastPairServerWriteGenericResponse(
            fast_pair_server, 
            cid, 
            result,
            HANDLE_KEYBASED_PAIRING_CLIENT_CONFIG
            );
}


/******************************************************************************/
static bool gattFastPairServerGenericNotification(
        GFPS       *fast_pair_server,
        uint32      cid, 
        uint8       value[FAST_PAIR_VALUE_SIZE],
        uint16      handle
        )
{
    if (fast_pair_server == NULL)
    {
        GATT_FAST_PAIR_SERVER_DEBUG_PANIC((
                    "GFPS: Null instance!\n"
                    ));
        return FALSE;
    }
    else if (cid == 0)
    {
        return FALSE;
    }

    uint8 *notification = (uint8*)CsrPmemAlloc(FAST_PAIR_VALUE_SIZE);
    memcpy(notification, value, FAST_PAIR_VALUE_SIZE);

    fast_pair_server->current_notify_handle = handle;

    GattNotificationEventReqSend(fast_pair_server->gattId, 
                                 cid, 
                                 handle,
                                 FAST_PAIR_VALUE_SIZE, 
                                 notification);
                                 
    return TRUE;
}


/******************************************************************************/
bool GattFastPairServerKeybasedPairingNotification(
        GFPS  *fast_pair_server,
        uint32      cid, 
        uint8       value[FAST_PAIR_VALUE_SIZE]
        )
{
    return gattFastPairServerGenericNotification(
            fast_pair_server, 
            cid, 
            value ,
            HANDLE_KEYBASED_PAIRING
            );
}
 

/******************************************************************************/
bool GattFastPairServerWritePasskeyResponse(
        const GFPS  *fast_pair_server,
        uint32      cid, 
        uint16      result
        )
{
    return gattFastPairServerWriteGenericResponse(
            fast_pair_server, 
            cid, 
            result,
            HANDLE_PASSKEY
            );
}


/******************************************************************************/
bool GattFastPairServerReadPasskeyConfigResponse(
        const GFPS  *fast_pair_server, 
        uint32      cid, 
        uint16      client_config
        )
{
    return gattFastPairServerGenericReadConfigResponse(
            fast_pair_server, 
            cid, 
            client_config,
            HANDLE_PASSKEY_CLIENT_CONFIG
            );
}


/******************************************************************************/
bool GattFastPairServerWritePasskeyConfigResponse(
        const GFPS  *fast_pair_server, 
        uint32      cid, 
        uint16      result
        )
{
    return gattFastPairServerWriteGenericResponse(
            fast_pair_server, 
            cid, 
            result,
            HANDLE_PASSKEY_CLIENT_CONFIG
            );
}


/******************************************************************************/
bool GattFastPairServerPasskeyNotification(
        GFPS        *fast_pair_server, 
        uint32      cid, 
        uint8       value[FAST_PAIR_VALUE_SIZE]
        )
{
    return gattFastPairServerGenericNotification(
            fast_pair_server, 
            cid, 
            value ,
            HANDLE_PASSKEY
            );
}


/******************************************************************************/
bool GattFastPairServerWriteAccountKeyResponse(
        const GFPS  *fast_pair_server, 
        uint32      cid, 
        uint16      result
        )
{
    return gattFastPairServerWriteGenericResponse(
            fast_pair_server, 
            cid, 
            result,
            HANDLE_ACCOUNT_KEY
            );
}

/******************************************************************************/
bool GattFastPairServerWriteDataConfigResponse(
        const GFPS  *fast_pair_server,
        uint32      cid,
        uint16      result
        )
{
       return gattFastPairServerWriteGenericResponse(
            fast_pair_server,
            cid,
            result,
            HANDLE_DATA_CLIENT_CONFIG
            );
}

/******************************************************************************/
bool GattFastPairServerWriteDataResponse(
        const GFPS  *fast_pair_server,
        uint32      cid,
        uint16      result
        )
{
     return gattFastPairServerWriteGenericResponse(
            fast_pair_server,
            cid,
            result,
            HANDLE_DATA
            );
}

/******************************************************************************/
bool GattFastPairServerAdditionalDataNotification(
        GFPS        *fast_pair_server,
        uint32      cid,
        uint8       *value,
        uint8       len
        )
{
    if (fast_pair_server == NULL)
    {
        GATT_FAST_PAIR_SERVER_DEBUG_PANIC((
                    "GFPS: Null instance!\n"
                    ));
        return FALSE;
    }
    else if (cid == 0)
    {
        return FALSE;
    }

    uint8 *notification = NULL;
    if(len)
    {
        notification = (uint8*)CsrPmemAlloc(len);
        memcpy(notification, value, len);
    }

    fast_pair_server->current_notify_handle = HANDLE_DATA;
    
    GattNotificationEventReqSend(fast_pair_server->gattId,
                                 cid,
                                 HANDLE_DATA,
                                 len,
                                 notification);

    return TRUE;
}

/******************************************************************************/
bool GattFastPairServerBeaconActionReadResponse(
        const GFPS  *fast_pair_server,
        uint32      btConnId,
        uint8*      resp_data,
        uint16      resp_len)
{
    if (fast_pair_server == NULL)
    {
        GATT_FAST_PAIR_SERVER_DEBUG_PANIC((
                    "GFPS: Null instance!\n"
                    ));
        return FALSE;
    }
    else if (btConnId == 0)
    {
        return FALSE;
    }

    GattDbReadAccessResSend(fast_pair_server->gattId,
                            btConnId,
                            HANDLE_BEACON_ACTIONS,
                            CSR_BT_GATT_ACCESS_RES_SUCCESS,
                            resp_len,
                            resp_data);

    return TRUE;
}

/******************************************************************************/
bool GattFastPairServerBeaconActionWriteResponse(
       const GFPS  *fast_pair_server,
       uint32      btConnId,
       uint16      result)
{
    if (fast_pair_server == NULL)
    {
        GATT_FAST_PAIR_SERVER_DEBUG_PANIC((
                    "GFPS: Null instance!\n"
                    ));
        return FALSE;
    }
    else if (btConnId == 0)
    {
        return FALSE;
    }

    GattDbWriteAccessResSend(fast_pair_server->gattId,
                             btConnId,
                             HANDLE_BEACON_ACTIONS,
                             result);

    return TRUE;
}

/******************************************************************************/
bool GattFastPairServerBeaconServiceResponse(
        GFPS        *fast_pair_server,
        uint32      btConnId,
        uint8       *value,
        uint8       value_len)
{
    uint8 *notification = NULL;

    if (fast_pair_server == NULL)
    {
        GATT_FAST_PAIR_SERVER_DEBUG_PANIC((
                    "GFPS: Null instance!\n"
                    ));
        return FALSE;
    }
    else if (btConnId == 0)
    {
        return FALSE;
    }

    if(value_len != 0)
    {
        notification = (uint8*)CsrPmemAlloc(value_len);
        memcpy(notification, value, value_len);

        fast_pair_server->current_notify_handle = HANDLE_BEACON_ACTIONS;

        GattNotificationEventReqSend(fast_pair_server->gattId,
                                     btConnId,
                                     HANDLE_BEACON_ACTIONS,
                                     value_len,
                                     notification);
        return TRUE;
    }

    return FALSE;
}
