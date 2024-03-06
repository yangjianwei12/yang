/* Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <stdio.h>
#include <stdlib.h>

#include "gatt_csis_server_private.h"

#include "gatt_csis_server_msg_handler.h"
#include "gatt_csis_server_lock_management.h"
#include "gatt_csis_server_notify.h"
#include "gatt_csis_server_access.h"

#include <connection.h>

#define GATT_CSIS_SERVER_INVALID_CID_INDEX  (0xFF)

/*! @brief csis sirk encryption/decryption request */
typedef struct
{
    connection_id_t            cid;           /* the remote client cid for which op is in progress */
    uint8                      operation;     /* read request or notify (local change)
                                               */
} gCsisSirk_t;

gCsisSirk_t csisServerSirk;

/***************************************************************************
NAME
    sendCsisServerAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
static void sendCsisServerAccessRsp(Task task,
                                    uint16 cid,
                                    uint16 handle,
                                    uint16 result,
                                    uint16 size_value,
                                    const uint8 *value)
{
    if (!GattManagerServerAccessResponse(task, cid, handle, result, size_value, value))
    {
        /* The GATT Manager should always know how to send this response */
        GATT_CSIS_SERVER_PANIC(("Couldn't send GATT access response\n"));
    }
}

/***************************************************************************
NAME
    sendCsisServerAccessErrorRsp

DESCRIPTION
    Send an error access response to the GATT Manager library.
*/
static void sendCsisServerAccessErrorRsp(Task task,
                            const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
                            uint16 error)
{
    sendCsisServerAccessRsp(task,
                            access_ind->cid,
                            access_ind->handle,
                            error,
                            0,
                            NULL);
}


/******************************************************************************/
static uint8 csisServerGetCidIndex(const GCSISS_T *csis_server, connection_id_t cid)
{
    uint8 index = GATT_CSIS_SERVER_INVALID_CID_INDEX;
    uint8 i;

    for (i=0; i<GATT_CSIS_MAX_CONNECTIONS; i++)
    {
        if(csis_server->data.connected_clients[i].cid == cid)
        {
            index = i;
            break;
        }
    }

    return index;
}

/***************************************************************************/
static gatt_status_t csisServerSetCCC(const GCSISS_T *csis_server,
                            connection_id_t cid,
                            uint16 handle,
                            uint8 *ccc)
{
    uint8 index_client = csisServerGetCidIndex(csis_server, cid);

    if(index_client != GATT_CSIS_SERVER_INVALID_CID_INDEX)
    {
        if (handle == HANDLE_LOCK_CLIENT_CONFIG)
        {
            csis_server->data.connected_clients[index_client].client_cfg.lockValueClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SIRK_CLIENT_CONFIG)
        {
            csis_server->data.connected_clients[index_client].client_cfg.sirkValueClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SIZE_CLIENT_CONFIG)
        {
            csis_server->data.connected_clients[index_client].client_cfg.sizeValueClientCfg = ccc[0];
        }
        else
        {
            /* Invalid handle */
            GATT_CSIS_SERVER_DEBUG_INFO(("Invalid handle!\n"))
            return gatt_status_invalid_handle;
        }
    }
    else
    {
        /* Invalid cid */
        GATT_CSIS_SERVER_DEBUG_INFO(("Invalid cid!\n"))
        return gatt_status_invalid_cid;
    }

    return gatt_status_success;
}

/***************************************************************************
NAME
    csisServerServiceAccess

DESCRIPTION
    Deals with access of the HANDLE_COORDINATED_SET_IDENTIFICATION_SERVICE handle.
*/
static void csisServerServiceAccess(Task task, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendCsisServerAccessRsp(task,
                                access_ind->cid,
                                HANDLE_COORDINATED_SET_IDENTIFICATION_SERVICE,
                                gatt_status_success,
                                0,
                                NULL);
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        /* Write of CSIS not allowed. */
        sendCsisServerAccessErrorRsp(task, access_ind, gatt_status_write_not_permitted);
    }
    else
    {
        /* Reject access requests that aren't read/write, which shouldn't happen. */
        sendCsisServerAccessErrorRsp(task, access_ind, gatt_status_request_not_supported);
    }
}

/***************************************************************************
NAME
    csisServerServiceAccess

DESCRIPTION
    Deals with access of the Lock CSIS handle.
*/
static void csisServerLockAccess(GCSISS_T *csis_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        /* Identify the current Lock value */
        uint8 value;

        value= (uint8)csisServerGetLockState();

        sendCsisServerAccessRsp(
                (Task)&csis_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                GATT_CSIS_SERVER_LOCK_SIZE,
                &value
                );

    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        /* Evaluate Lock state and which instance & cid is using the Lock
         * before giving it to the remote client. Update Lock value if 
         * successfully taken and then notify application and other
         * clients who have registered for 'Lock' change notification.
         */
        uint8 result;
        uint8 update_timer;
        tp_bdaddr tpaddr = {0};
        memset(&tpaddr, 0, sizeof(tp_bdaddr));

        result = csisServerEvaluateLockValue(
                               access_ind->cid,
                               access_ind->size_value,
                               (GattCsisServerLocktype )access_ind->value[0],
                               &update_timer,
                               &tpaddr);

        /* Send Write response to GATT Manager */
        sendCsisServerAccessRsp(
                         (Task)&csis_server->lib_task,
                          access_ind->cid,
                          access_ind->handle,
                          result,
                          GATT_CSIS_SERVER_LOCK_SIZE,
                          access_ind->value);

        if (result == GATT_CSIS_LOCK_ALREADY_GRANTED && update_timer == LOCK_TIMER_UPDATE_NOT_REQUIRED)
        {
            /* Special case: on re-connection cid may be diff if locked
             * previously by same client and lock expiry timer is still runnnig.
             * In that case we need to just update cid in csis_server
             */
            if (access_ind->cid != csisServerGetClientLockConnId())
            {
                csisServerUpdateLockCid(access_ind->cid);
            }

            return;
        }
        else if (result == gatt_status_success)
        {
            /* If lock is already unlocked and client(s) tries to unlock
             * it again
             */
            if (update_timer == LOCK_TIMER_UPDATE_NOT_REQUIRED)
            {
                /* Another client wrote unlocked value to already unlocked lock
                 * In that case we need to just update cid in csis_server
                 * for sake of which client wrote to lock char
                 */
                if (access_ind->cid != csisServerGetClientLockConnId())
                {
                    csisServerUpdateLockCid(access_ind->cid);
                }

                return;
            }

            /* Update the lock */
            csisServerUpdateLock(
                            (GattCsisServerLocktype )access_ind->value[0],
                            csisServerGetLockTimeout(),
                            csis_server->srvc_hndl ,
                            access_ind->cid,
                            &tpaddr);

            /* Notify all connected clients except this one about Lock state */
            csisServerNotifyLockChangeOtherClients(csis_server, access_ind->cid);

            /* Start the Lock timer if lock is taken */
            if ((GattCsisServerLocktype)access_ind->value[0] == CSIS_SERVER_LOCKED)
            {
                MessageCancelAll(
                           (Task) &csis_server->lib_task,
                            CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER);

                MAKE_CSIS_SERVER_MESSAGE(CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER);
                message->cid = access_ind->cid;

                MessageSendLater(
                          (Task) &csis_server->lib_task,
                           CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER,
                           message,
                           D_SEC(csisServerGetLockTimeout()));

            }
            else if ((GattCsisServerLocktype)access_ind->value[0] == CSIS_SERVER_UNLOCKED)
            {
                MessageCancelAll(
                           (Task) &csis_server->lib_task,
                            CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER);
            }

            /* Inform application about Lock*/
            MAKE_CSIS_SERVER_MESSAGE(GATT_CSIS_LOCK_STATE_IND);
            message->csisHandle = GattCsisServerGetLock();
            message->cid = access_ind->cid;
            message->lockValue = access_ind->value[0];

            MessageSend(csis_server->app_task, GATT_CSIS_LOCK_STATE_IND, message);
        }

    }
    else
    {
        /* Reject access requests that aren't read/write, which shouldn't happen. */
        sendCsisServerAccessErrorRsp((Task) &csis_server->lib_task, access_ind, gatt_status_request_not_supported);
    }
}


/***************************************************************************
NAME
    csisServerServiceAccess

DESCRIPTION
    Deals with access of the Lock CSIS handle.
*/
static void csisServerSirkAccess(GCSISS_T *csis_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint8 i,j;
        uint8 value[GATT_CSIS_SERVER_SIRK_SIZE_PLUS_TYPE_SIZE];

        if((csis_server->data.sirkOp & GATT_CSIS_SHARE_SIRK_OOB_ONLY) == GATT_CSIS_SHARE_SIRK_OOB_ONLY)
        {
            sendCsisServerAccessErrorRsp(
                    (Task)&csis_server->lib_task,
                    access_ind,
                    GATT_CSIS_OOB_SIRK_ONLY
                    );

            return;
        }
        else if((csis_server->data.sirkOp & GATT_CSIS_SHARE_SIRK_ENCRYPTED) == GATT_CSIS_SHARE_SIRK_ENCRYPTED)
        {
            /* Generate encrypted SIRK */
            if (csisServerSirk.cid == 0)
            {
               csisServerGenerateEncryptedSirk(csis_server, GATT_CSIS_SERVER_READ_ACCESS, 0, access_ind->cid);
            }
            else
            {
                /* Request is currently in progress for a cid - Send Busy ?*/
                sendCsisServerAccessErrorRsp((Task) &csis_server->lib_task,
                                             access_ind,
                                             gatt_status_busy);
            }

            return;
        }
        else
        {
            /* default plain text */
            value[0] = GATT_CSIS_PLAIN_TEXT_SIRK;

            for(i=0, j= GATT_CSIS_SERVER_SIRK_SIZE-1; i< GATT_CSIS_SERVER_SIRK_SIZE && j>=0; i++, j--)
            {
                value[1+i] =csis_server->data.sirk[j];
            }
        }

        sendCsisServerAccessRsp(
                (Task)&csis_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                GATT_CSIS_SERVER_SIRK_SIZE_PLUS_TYPE_SIZE,
                value
                );
    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendCsisServerAccessErrorRsp((Task) &csis_server->lib_task, access_ind, gatt_status_request_not_supported);
    }
}

/***************************************************************************
NAME
    csisServerServiceAccess

DESCRIPTION
    Deals with access of the Lock CSIS handle.
*/
static void csisServerSizeAccess(GCSISS_T *csis_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint8 value;

        value = csis_server->data.cs_size;

        sendCsisServerAccessRsp(
                (Task)&csis_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                GATT_CSIS_SERVER_SIZE_CHARACTERISTCIS_SIZE,
                &value
                );
    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendCsisServerAccessErrorRsp((Task) &csis_server->lib_task, access_ind, gatt_status_request_not_supported);
    }
}


/***************************************************************************
NAME
    csisServerRankAccess

DESCRIPTION
    Deals with access of the Rank handle.
*/
static void csisServerRankAccess(GCSISS_T *csis_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint8 value;

        value = csis_server->data.rank;

        sendCsisServerAccessRsp(
                (Task)&csis_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                GATT_CSIS_SERVER_RANK_CHARACTERISTCIS_SIZE,
                &value
                );
    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendCsisServerAccessErrorRsp((Task) &csis_server->lib_task, access_ind, gatt_status_request_not_supported);
    }
}


/***************************************************************************/
static void csisHandleReadClientConfigAccess(
        Task task,
        uint16 cid,
        uint16 handle,
        const uint16 client_config
        )
{
    uint8 config_data[GATT_CSIS_SERVER_CCC_VALUE_SIZE];

    if (task == NULL)
    {
        GATT_CSIS_SERVER_DEBUG_PANIC((
                    "CSIS: Null instance!\n"
                    ));
    }
    else if (cid == 0)
    {
        GATT_CSIS_SERVER_DEBUG_PANIC((
                    "CSIS: Null instance!\n"
                    ));
    }

    config_data[0] = (uint8)client_config & 0xFF;
    config_data[1] = (uint8)client_config >> 8;

    sendCsisServerAccessRsp(
            task,
            cid,
            handle,
            gatt_status_success,
            GATT_CSIS_SERVER_CCC_VALUE_SIZE,
            config_data
            );
}


/***************************************************************************/
static bool csisHandleWriteClientConfigAccess(
        GCSISS_T *csis_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    bool res = FALSE;

    if (access_ind->size_value != GATT_CSIS_SERVER_CCC_VALUE_SIZE)
    {
        sendCsisServerAccessErrorRsp(
                (Task) &csis_server->lib_task,
                access_ind,
                gatt_status_invalid_length
                );
    }
    /* Validate the input parameters - ONLY Notify*/
    else if ( access_ind->value[0] == GATT_CSIS_SERVER_CCC_INDICATE )
    {
        sendCsisServerAccessErrorRsp(
                (Task) &csis_server->lib_task,
                access_ind,
                gatt_status_cccd_improper_config
                );
    }
    else if ( access_ind->value[0] == GATT_CSIS_SERVER_CCC_NOTIFY || access_ind->value[0] == 0 )
    {
        /* Valid value of CCC */

        /* Save the new ccc in the library */
        gatt_status_t status = csisServerSetCCC(
                                   csis_server,
                                   (connection_id_t) access_ind->cid,
                                   access_ind->handle,
                                   access_ind->value);

        /* Send response to the client */
        sendCsisServerAccessRsp(
             (Task) &csis_server->lib_task,
             access_ind->cid,
             access_ind->handle,
             status,
             0,
             NULL
             );
        res = TRUE;
    }
    else
    {
        /* Send response to the client but the value is ignored*/
        sendCsisServerAccessRsp(
             (Task) &csis_server->lib_task,
             access_ind->cid,
             access_ind->handle,
             gatt_status_success,
             0,
             NULL
             );
    }

    return res;
}

static void csisHandleClientConfigAccess(GCSISS_T *const csis_server,
              const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
              uint16 client_config)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        csisHandleReadClientConfigAccess(
                    (Task) &csis_server->lib_task,
                    access_ind->cid,
                    access_ind->handle,
                    client_config
                    );
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        csisHandleWriteClientConfigAccess(
                    csis_server,
                    access_ind
                    );
    }
    else
    {
        sendCsisServerAccessErrorRsp(
                    (Task) &csis_server->lib_task,
                    access_ind,
                    gatt_status_request_not_supported
                    );
    }

}


/***************************************************************************
NAME
    handleCsisServerAccessInd

DESCRIPTION
    Handles the GATT_MANAGER_ACCESS_IND message that was sent to the library.
*/
void handleCsisServerAccessInd(GCSISS_T *csis_server,
              const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    uint8 index_client = csisServerGetCidIndex(csis_server, access_ind->cid);

    switch (access_ind->handle)
    {
        case HANDLE_COORDINATED_SET_IDENTIFICATION_SERVICE:
        {
           /* Service Handle read request */
           csisServerServiceAccess((Task) &csis_server->lib_task, access_ind);
        }
        break;

        case HANDLE_LOCK:
        {
            csisServerLockAccess(csis_server, access_ind);
        }
        break;

        /* Client configuration of the Lock Characteristic is 
         * being read.
         */
        case HANDLE_LOCK_CLIENT_CONFIG:
        {
            uint16 client_config =
                GET_CSIS_CLIENT_CONFIG(csis_server->data.connected_clients[index_client].client_cfg.lockValueClientCfg);

            csisHandleClientConfigAccess(csis_server,
                                access_ind,
                                client_config);
        }
        break;

        case HANDLE_SIZE:
        {
            csisServerSizeAccess(csis_server, access_ind);
        }
        break;

        case HANDLE_RANK:
        {
            csisServerRankAccess(csis_server, access_ind);
        }
        break;

        /* Client configuration of the Size Characteristic is 
         * being read.
         */
        case HANDLE_SIZE_CLIENT_CONFIG:
        {
            uint16 client_config =
                GET_CSIS_CLIENT_CONFIG(csis_server->data.connected_clients[index_client].client_cfg.sizeValueClientCfg);

            csisHandleClientConfigAccess(csis_server,
                                access_ind,
                                client_config);
        }
        break;

        case HANDLE_SIRK:
        {
            csisServerSirkAccess(csis_server, access_ind);
        }
        break;

        /* Client configuration of the SIRK Characteristic is 
         * being read.
         */
        case HANDLE_SIRK_CLIENT_CONFIG:
        {
            uint16 client_config =
                GET_CSIS_CLIENT_CONFIG(csis_server->data.connected_clients[index_client].client_cfg.sirkValueClientCfg);

            csisHandleClientConfigAccess(csis_server,
                                access_ind,
                                client_config);
        }
        break;

        default:
        {
            /* Respond to invalid handles */
            sendCsisServerAccessErrorRsp((Task) &csis_server->lib_task, access_ind,gatt_status_invalid_handle);
        }
        break;
    }
}

void csisServerGenerateEncryptedSirk(GCSISS_T *csis_server,
    uint8 operation,
    uint8 index,
    connection_id_t cid)
{
    tp_bdaddr clientTpAddr;

    if (cid != 0)
        csisServerSirk.cid = cid;
    else
    {
        csisServerSirk.cid = csis_server->data.connected_clients[index].cid;
    }

    csisServerSirk.operation = operation;
    clientTpAddr = csisServerGetTpAddrFromCid(cid);

    ConnectionSmSirkOperationReq(
              (Task)&csis_server->lib_task,
              &clientTpAddr,
              0,
              &csis_server->data.sirk[0]
              );
}


void csisServerhandleSirkOperation(GCSISS_T *csis_server,
    const CL_SM_SIRK_OPERATION_CFM_T * cfm)
{
    tp_bdaddr clientTpAddr;
    uint8 i,j;
    uint8 value[GATT_CSIS_SERVER_SIRK_SIZE_PLUS_TYPE_SIZE];

    if (cfm->status != 0) 
    {
        csisServerSirk.cid = 0;
        csisServerSirk.operation = GATT_CSIS_SERVER_NONE;
        return;
    }

    if (csisServerSirk.cid != 0)
    {
        clientTpAddr = csisServerGetTpAddrFromCid(csisServerSirk.cid);

        if (BdaddrTpIsSame(&(cfm->tpaddr), &clientTpAddr))
        {
            value[0] = GATT_CSIS_ENCRYPTED_SIRK;

            for(i=0, j= CL_SM_SIRK_KEY_LEN-1; i< CL_SM_SIRK_KEY_LEN && j>=0; i++, j--)
            {
                value[1+i] = cfm->sirk_key[j];
            }

            if (csisServerSirk.operation == GATT_CSIS_SERVER_READ_ACCESS)
            {
                sendCsisServerAccessRsp(
                        (Task)&csis_server->lib_task,
                        csisServerSirk.cid,
                        HANDLE_SIRK,
                        gatt_status_success,
                        GATT_CSIS_SERVER_SIRK_SIZE_PLUS_TYPE_SIZE,
                        value
                        );

                csisServerSirk.cid = 0;
                csisServerSirk.operation = GATT_CSIS_SERVER_NONE;
            }
            else
            {
                for (i=0; i< GATT_CSIS_MAX_CONNECTIONS; i++)
                {
                    if (csis_server->data.connected_clients[i].cid != 0 &&
                        (csisServerSirk.cid == csis_server->data.connected_clients[i].cid))
                    {
                        uint16 client_config;
                        client_config = 
                            GET_CSIS_CLIENT_CONFIG(csis_server->data.connected_clients[i].client_cfg.sirkValueClientCfg);

                        /* If the Client Config is 0x01 (Notify is TRUE), a notification will
                         * be sent to the client */
                        if (client_config == GATT_CSIS_SERVER_CCC_NOTIFY)
                        {
                            csisServerSendCharacteristicChangedNotification(
                                    (Task) &(csis_server->lib_task),
                                    csis_server->data.connected_clients[i].cid,
                                    HANDLE_SIRK,
                                    GATT_CSIS_SERVER_SIRK_SIZE_PLUS_TYPE_SIZE,
                                    value
                                    );
                        }
                        else
                        {
                            /* handle not configured for Notification - Nothing to do */
                        }

                        /* generate for other connected client(s)*/
                        if ( i < GATT_CSIS_MAX_CONNECTIONS - 1)
                            csisServerGenerateEncryptedSirk(csis_server, GATT_CSIS_SERVER_NOTIFY, i + 1 , 0);

                        break;
                    }
                }
            }
        }
    }
}

