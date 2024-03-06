/* Copyright (c) 2018-22 Qualcomm Technologies International, Ltd. */
/*  */

#include <vm.h>

#include "gatt_fast_pair_server_private.h"
#include "gatt_fast_pair_server_msg_handler.h"

#define KEYBASED_PAIRING_MIN_SIZE  (16)
#define KEYBASED_PAIRING_MAX_SIZE  (80)

static void sendFpsServerWriteAccessErrorRsp(const GFPS *fast_pair_server, 
                                             const CsrBtGattDbAccessWriteInd *access_ind, 
                                             uint16 error);
/******************************************************************************/
static bool attBearerIsBLE(uint32 connId)
{
    tp_bdaddr tpaddr;

    if (VmGetBdAddrtFromCid(GattClientUtilGetCidByConnId(connId), &tpaddr) &&
        tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
static void fpsHandleReadClientConfigAccess(
        const GFPS *fast_pair_server,
        const CsrBtGattDbAccessReadInd *access_ind,
        MessageId id
        )
{
    /* Same structure for GATT_FAST_PAIR_SERVER_READ_PASSKEY_CONFIG_IND */
    MAKE_FPS_MESSAGE(
            GATT_FAST_PAIR_SERVER_READ_KEYBASED_PAIRING_CONFIG_IND
            );

    message->fast_pair_server = fast_pair_server;
    message->cid = access_ind->btConnId;

    MessageSend(
            fast_pair_server->app_task,
            id,
            message);
}

/******************************************************************************/
static void fpsHandleReadModelIdAccess(
        const GFPS *fast_pair_server,
        const CsrBtGattDbAccessReadInd *access_ind,
        MessageId id
        )
{
    MAKE_FPS_MESSAGE(
            GATT_FAST_PAIR_SERVER_READ_MODEL_ID_IND
            );

    message->fast_pair_server = fast_pair_server;
    message->cid = access_ind->btConnId;

    MessageSend(
            fast_pair_server->app_task,
            id,
            message);
}

/******************************************************************************/
static void fpsHandleWriteClientConfigAccess(
        const GFPS *fast_pair_server,
        const CsrBtGattDbAccessWriteInd *access_ind,
        MessageId id
        )
{
    if (access_ind->writeUnit[0].valueLength != CLIENT_CONFIG_VALUE_SIZE)
    {
        sendFpsServerWriteAccessErrorRsp(
                fast_pair_server, 
                access_ind,
                CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH
                );
    }
    else
    {
        /* Same structure for 
         * GATT_FAST_PAIR_SERVER_WRITE_PASSKEY_CONFIG_IND 
         */
        MAKE_FPS_MESSAGE(
                GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_CONFIG_IND
                );

        message->fast_pair_server = fast_pair_server;
        message->cid = access_ind->btConnId;
        message->config_value = 
            access_ind->writeUnit[0].value[0] | access_ind->writeUnit[0].value[1] << 8;

        MessageSend(
                fast_pair_server->app_task,
                id,
                message
                );
    }
}


/******************************************************************************
 * Both have the same structure and value size
 ******************************************************************************/
static void fpsHandlePasskeyOrAccountKeyWriteAccess(
        const GFPS *fast_pair_server,
        const CsrBtGattDbAccessWriteInd *access_ind,
        MessageId id
        )
{
    /* Passkey size is always 16-octets. */
    if (access_ind->writeUnit[0].valueLength != FAST_PAIR_VALUE_SIZE)
    {
        sendFpsServerWriteAccessErrorRsp(
                fast_pair_server, 
                access_ind,
                CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH
                );
    }
    else
    {
        MAKE_FPS_MESSAGE(
                GATT_FAST_PAIR_SERVER_WRITE_PASSKEY_IND
                );

        message->fast_pair_server = fast_pair_server;
        message->cid = access_ind->btConnId;
        memmove(
                message->value, 
                access_ind->writeUnit[0].value, 
                sizeof(uint8) * FAST_PAIR_VALUE_SIZE
               );

        MessageSend(
                fast_pair_server->app_task,
                id,
                message
                );
    }
}

/******************************************************************************/
static void fpsHandleDataAccess(
        const GFPS *fast_pair_server,
        const CsrBtGattDbAccessWriteInd *access_ind
        )
{
    /* Value size must be minimum of 16 octets. */
    if (access_ind->writeUnit[0].valueLength < FAST_PAIR_DATA_PKT_SIZE_MIN)
    {
        sendFpsServerWriteAccessErrorRsp(
                fast_pair_server,
                access_ind,
                CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH
                );
    }
    else
    {
        MAKE_FPS_MESSAGE_WITH_LEN(
                GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_IND,
                access_ind->writeUnit[0].valueLength
                );

        message->fast_pair_server = fast_pair_server;
        message->cid = access_ind->btConnId;
        message->size_value = access_ind->writeUnit[0].valueLength;
        memmove(
                message->value,
                access_ind->writeUnit[0].value,
                sizeof(uint8) * access_ind->writeUnit[0].valueLength
               );

        MessageSend(
                fast_pair_server->app_task,
                GATT_FAST_PAIR_SERVER_WRITE_DATA_IND,
                message
                );
    }
}

/******************************************************************************/
static void fpsSendNotificationCfm(
        const GFPS *fast_pair_server, 
        const CsrBtGattEventSendCfm *notification_cfm,
        MessageId id
        )
{
    /* Both GATT_FAST_PAIR_SERVER_KEYBASED_PAIRING_NOTIFICATION_CFM and
     * GATT_FAST_PAIR_SERVER_PASSKEY_NOTIFICATION_CFM have the same structure.
     */
    MAKE_FPS_MESSAGE(
            GATT_FAST_PAIR_SERVER_KEYBASED_PAIRING_NOTIFICATION_CFM
            );

    message->fast_pair_server = fast_pair_server;
    message->cid = notification_cfm->btConnId;
    message->status = notification_cfm->resultCode;

    MessageSend(
            fast_pair_server->app_task,
            id,
            message
            );
}

/******************************************************************************/
static void fpsHandleNotificationcfm(GFPS *fast_pair_server,
                                     const CsrBtGattEventSendCfm *notification_cfm)
{
    switch (fast_pair_server->current_notify_handle)
    {
        case HANDLE_KEYBASED_PAIRING:
            fpsSendNotificationCfm(
                    fast_pair_server,
                    notification_cfm,
                    GATT_FAST_PAIR_SERVER_KEYBASED_PAIRING_NOTIFICATION_CFM
                    );
            fast_pair_server->current_notify_handle = 0xFFFF;
            break;

        case HANDLE_PASSKEY:
            fpsSendNotificationCfm(
                    fast_pair_server,
                    notification_cfm,
                    GATT_FAST_PAIR_SERVER_PASSKEY_NOTIFICATION_CFM
                    );
             fast_pair_server->current_notify_handle = 0xFFFF;
             break;

        case HANDLE_DATA:
            break;

        default:
            GATT_FAST_PAIR_SERVER_DEBUG_PANIC((
                    "Unrecognised handle 0x%04x in Notification Cfm\n",
                    fast_pair_server->current_notify_handle
                    ));
    }
}

/***************************************************************************/
static void sendFpsServerReadAccessErrorRsp(const GFPS *fast_pair_server, 
                                            const CsrBtGattDbAccessReadInd *access_ind, 
                                            uint16 error)
{
    GattDbReadAccessResSend(fast_pair_server->gattId,
                            access_ind->btConnId,
                            access_ind->attrHandle,
                            error,
                            0,
                            NULL);
}

/***************************************************************************/

static void fpsHandleBeaconActionReadAccess(
        const GFPS *fast_pair_server,
        const CsrBtGattDbAccessReadInd *access_ind,
        MessageId id)
{
    MAKE_FPS_MESSAGE(
            GATT_FAST_PAIR_SERVER_READ_BEACON_ACTION_IND
            );

    message->fast_pair_server = fast_pair_server;
    message->btConnId = access_ind->btConnId;

    MessageSend(
            fast_pair_server->app_task,
            id,
            message);
}

/***************************************************************************/
static void fpsHandleReadAccess(const GFPS *fast_pair_server, 
                         const CsrBtGattDbAccessReadInd *access_ind)
{
    GATT_FAST_PAIR_SERVER_DEBUG_INFO(("fpsHandleReadAccess: handle %d",access_ind->attrHandle));

    /* Service characteristics and descriptors cannot be accessed over BR/EDR */
    if ( !attBearerIsBLE(access_ind->btConnId))
    {
        sendFpsServerReadAccessErrorRsp(fast_pair_server,
                                        access_ind,
                                        ATT_RESULT_APP_MASK);
        return;
    }

    uint16 handle = access_ind->attrHandle;
    
    switch (handle)
    {
         case HANDLE_MODEL_ID:
         {
            fpsHandleReadModelIdAccess(
                fast_pair_server,
                access_ind,
                GATT_FAST_PAIR_SERVER_READ_MODEL_ID_IND
                );
         }
         break;

        case HANDLE_KEYBASED_PAIRING_CLIENT_CONFIG:
        {
            fpsHandleReadClientConfigAccess(fast_pair_server, 
                                            access_ind,
                                            GATT_FAST_PAIR_SERVER_READ_KEYBASED_PAIRING_CONFIG_IND);
        }
        break;

        case HANDLE_PASSKEY_CLIENT_CONFIG:
        {
            fpsHandleReadClientConfigAccess(fast_pair_server, 
                                            access_ind,
                                            GATT_FAST_PAIR_SERVER_READ_PASSKEY_CONFIG_IND);
         }
         break;

        case HANDLE_DATA:
        {
            sendFpsServerReadAccessErrorRsp(fast_pair_server, access_ind, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
        }
        break;

        case HANDLE_DATA_CLIENT_CONFIG:
        {
            sendFpsServerReadAccessErrorRsp(fast_pair_server, access_ind, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
        }
        break;

        case HANDLE_BEACON_ACTIONS:
        {
            fpsHandleBeaconActionReadAccess(fast_pair_server,
                                            access_ind,
                                            GATT_FAST_PAIR_SERVER_READ_BEACON_ACTION_IND);
        }
        break;

        default:
            sendFpsServerReadAccessErrorRsp(fast_pair_server, access_ind, CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        break;
    }
}

/***************************************************************************/
static void sendFpsServerWriteAccessErrorRsp(const GFPS *fast_pair_server, 
                                             const CsrBtGattDbAccessWriteInd *access_ind, 
                                             uint16 error)
{
    GattDbWriteAccessResSend(fast_pair_server->gattId,
                            access_ind->btConnId,
                            access_ind->attrHandle,
                            error);
}

/***************************************************************************/
static void fpsHandleWriteAccess(GFPS *fast_pair_server,
                                 const CsrBtGattDbAccessWriteInd *access_ind)
{
   GATT_FAST_PAIR_SERVER_DEBUG_INFO(("fpsHandleWriteAccess: handle %d",access_ind->attrHandle));
   uint16 handle = access_ind->attrHandle;

    /* Service characteristics and descriptors cannot be accessed over BR/EDR */
    if ( !attBearerIsBLE(access_ind->btConnId))
    {
        sendFpsServerWriteAccessErrorRsp(fast_pair_server,
                                         access_ind,
                                         ATT_RESULT_APP_MASK);
        return;
    }

    if((access_ind->check & CSR_BT_GATT_ACCESS_CHECK_AUTHORISATION) && 
       (!access_ind->writeUnitCount))
    {
         GattDbWriteAccessResSend(fast_pair_server->gattId,
                             access_ind->btConnId,
                             access_ind->attrHandle,
                             CSR_BT_GATT_ACCESS_RES_SUCCESS);
 
         return;
    }
   
    switch (handle)
    {
        case HANDLE_KEYBASED_PAIRING:
        {
            /* Value size must be 16 or 80-octets. */
            if (access_ind->writeUnit[0].valueLength != KEYBASED_PAIRING_MIN_SIZE &&
                access_ind->writeUnit[0].valueLength != KEYBASED_PAIRING_MAX_SIZE)
            {
                sendFpsServerWriteAccessErrorRsp(fast_pair_server, 
                                                 access_ind,
                                                 CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH);
            }
            else
            {
                MAKE_FPS_MESSAGE_WITH_LEN(
                        GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_IND,
                        access_ind->writeUnit[0].valueLength
                        );

                message->fast_pair_server = fast_pair_server;
                message->cid = access_ind->btConnId;
                message->size_value = access_ind->writeUnit[0].valueLength;
                memmove(
                        message->value, 
                        access_ind->writeUnit[0].value, 
                        sizeof(uint8) * access_ind->writeUnit[0].valueLength
                       );

                MessageSend(
                        fast_pair_server->app_task,
                        GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_IND,
                        message
                        );
            }
        }
        break;

        case HANDLE_MODEL_ID:
        {
            sendFpsServerWriteAccessErrorRsp(fast_pair_server,
                                             access_ind,
                                             CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
        }
        break;

        case HANDLE_KEYBASED_PAIRING_CLIENT_CONFIG:
        {
            fpsHandleWriteClientConfigAccess(
                        fast_pair_server, 
                        access_ind,
                        GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_CONFIG_IND
                        );
        }
        break;

        case HANDLE_PASSKEY:
        {
            fpsHandlePasskeyOrAccountKeyWriteAccess(
                        fast_pair_server, 
                        access_ind,
                        GATT_FAST_PAIR_SERVER_WRITE_PASSKEY_IND
                        );
        }
        break;

        case HANDLE_PASSKEY_CLIENT_CONFIG:
        {
            fpsHandleWriteClientConfigAccess(
                            fast_pair_server, 
                            access_ind,
                            GATT_FAST_PAIR_SERVER_WRITE_PASSKEY_CONFIG_IND
                            );
        }
        break;

        case HANDLE_ACCOUNT_KEY:
        {
            fpsHandlePasskeyOrAccountKeyWriteAccess(
                        fast_pair_server, 
                        access_ind,
                        GATT_FAST_PAIR_SERVER_WRITE_ACCOUNT_KEY_IND
                        );
        }
        break;

        case HANDLE_DATA:
        {
            fpsHandleDataAccess(fast_pair_server,access_ind);
        }
        break;

        case HANDLE_DATA_CLIENT_CONFIG:
        {
            fpsHandleWriteClientConfigAccess(
                    fast_pair_server,
                    access_ind,
                    GATT_FAST_PAIR_SERVER_WRITE_DATA_CONFIG_IND
                    );
        }
        break;

       case HANDLE_BEACON_ACTIONS:
       {
           MAKE_FPS_MESSAGE_WITH_LEN(
                  GATT_FAST_PAIR_SERVER_WRITE_BEACON_ACTION_IND,
                  access_ind->writeUnit[0].valueLength
                  );

           message->fast_pair_server = fast_pair_server;
           message->btConnId = access_ind->btConnId;
           message->size_value = access_ind->writeUnit[0].valueLength;
           memmove(
                   message->value,
                   access_ind->writeUnit[0].value,
                   sizeof(uint8) * access_ind->writeUnit[0].valueLength
                  );

           MessageSend(
                   fast_pair_server->app_task,
                   GATT_FAST_PAIR_SERVER_WRITE_BEACON_ACTION_IND,
                   message
                   );
       }
       break;

       case HANDLE_BEACON_ACTIONS_CLIENT_CONFIG:
       {
         GattDbWriteAccessResSend(fast_pair_server->gattId,
                                  access_ind->btConnId,
                                  access_ind->attrHandle,
                                  CSR_BT_GATT_ACCESS_RES_SUCCESS);
       }
       break;

        default:
            sendFpsServerWriteAccessErrorRsp(fast_pair_server,
                                             access_ind,
                                             CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        break;
    }
}

/******************************************************************************/
void fpsServerMsgHandler(Task task, MessageId id, Message msg)
{
    GFPS *fast_pair_server = (GFPS *)task;

    GATT_FAST_PAIR_SERVER_DEBUG_INFO(("fpsServerMsgHandler: %d (x%x)",id,id));

    if (id != GATT_PRIM)
    {
        return;
    }

    CsrBtGattPrim *primType = (CsrBtGattPrim *)msg;

    switch (*primType)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            fpsHandleRegisterCfm(fast_pair_server, (CsrBtGattRegisterCfm*)msg);
            break;
        
        case CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM:
            fpsHandleRegisterHandleRangeCfm(fast_pair_server, (CsrBtGattFlatDbRegisterHandleRangeCfm*)msg);
            break;

        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
            /* Write access to characteristic */
            fpsHandleWriteAccess(fast_pair_server, (CsrBtGattDbAccessWriteInd *)msg);
            break;
            
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
            /* Read access to characteristic */
            fpsHandleReadAccess(fast_pair_server, (CsrBtGattDbAccessReadInd *)msg);
            break;

        case CSR_BT_GATT_EVENT_SEND_CFM:
            fpsHandleNotificationcfm(fast_pair_server, (const CsrBtGattEventSendCfm *)msg);
            break;

        default:
        {
            GATT_FAST_PAIR_SERVER_DEBUG_PANIC((
                        "GFPS: GATT Manager message 0x%04x not handled\n",
                        (*primType)
                        ));
            break;
        }
    } /* switch */
    GattFreeUpstreamMessageContents((void *)msg);
}


