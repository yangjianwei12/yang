/******************************************************************************
 Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "gatt_csis_server_private.h"
#include "gatt_csis_server_msg_handler.h"
#include "gatt_csis_server_lock_management.h"
#include "gatt_csis_server_notify.h"
#include "gatt_csis_server_access.h"
#include "gatt_csis_server_debug.h"

#define GATT_CSIS_SERVER_INVALID_CID_INDEX  (0xFF)
extern CsrSchedTid csis_tId;

/*! @brief csis sirk encryption/decryption request */
typedef struct
{
    connection_id_t            cid;           /* the remote client cid for which op is in progress */
    uint8                      operation;     /* read request or notify (local change)
                                               */
} gCsisSirk_t;

gCsisSirk_t csisServerSirk;

static void csisServerSendConfigChangeIndication(GCSISS_T  *const csis_server,
    CsisServerConnectionIdType cid,
    bool configChangeComplete)
{
    /* Indicate the application all CCCD are written by client */
    MAKE_CSIS_SERVER_MESSAGE(GattCsisServerConfigChangeInd);

    message->csisHandle = csis_server->srvc_hndl;
    message->cid = cid;
    message->configChangeComplete = configChangeComplete;

    CsisMessageSend(csis_server->app_task, GATT_CSIS_SERVER_CONFIG_CHANGE_IND, message);
}

static bool csisServerAllClientConfigWritten(GCSISS_T  *const csis_server,
    CsisServerConnectionIdType cid)
{
    uint8 i;

    for(i = 0; i < GATT_CSIS_MAX_CONNECTIONS; i++)
    {
        if(csis_server->data.connected_clients[i].cid == cid &&
            csis_server->data.connected_clients[i].client_cfg.lockValueClientCfg != GATT_CSIS_SERVER_INVALID_CLIENT_CONFIG &&
            csis_server->data.connected_clients[i].client_cfg.sirkValueClientCfg != GATT_CSIS_SERVER_INVALID_CLIENT_CONFIG &&
            csis_server->data.connected_clients[i].client_cfg.sizeValueClientCfg != GATT_CSIS_SERVER_INVALID_CLIENT_CONFIG )
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void csisServerSetClientConfigWrite(GCSISS_T  *const csis_server,
    CsisServerConnectionIdType cid,
    uint16 handle,
    bool clientConfigChanged)
{
    bool configChangeComplete = csisServerAllClientConfigWritten(csis_server, cid);

    /* Except Lock in CSIS other handles does not change values currently.
       This needs to be revisted if others change their values too.
       We don't want to flood unncessary application for remote CCCD write on
       char which does not change their values at all.
     */
    if (clientConfigChanged &&
        (handle == HANDLE_LOCK_CLIENT_CONFIG))
    {
        /* Inform above layer about CCCD change*/
        csisServerSendConfigChangeIndication(csis_server,
                                         cid,
                                         configChangeComplete);
    }
}


static bool csisServerClientConfigChanged(uint16 clientCfg, uint8 newClientCfg)
{
    /* Check if the client config has changed, to notify above layer */
    if(((uint8)clientCfg) != newClientCfg)
        return TRUE;

    return FALSE;
}

/***************************************************************************
NAME
    sendCsisServerAccessWriteRsp

DESCRIPTION
    Send an write access response to the GATT  library.
*/
static void sendCsisServerAccessWriteRsp(CsrBtGattId task,
                                        connection_id_t cid,
                                        uint16 handle,
                                        uint16 result,
                                        uint16 size_value,
                                        uint8 const *value)
{
    CsrBtGattDbWriteAccessResSend(task,
                                  cid,
                                  handle,
                                  result);
    CSR_UNUSED(size_value);
    CSR_UNUSED(value);
}

/***************************************************************************
NAME
    sendCsisServerAccessRsp

DESCRIPTION
    Send an read access response to the GATT  library.
*/
static void sendCsisServerAccessRsp(CsrBtGattId task,
                                    connection_id_t cid,
                                    uint16 handle,
                                    uint16 result,
                                    uint16 size_value,
                                    uint8  *const value)
{
    uint8* data;

    data = (uint8*)CsrPmemZalloc(size_value);
    CsrMemCpy(data, value, size_value);

    CsrBtGattDbReadAccessResSend(task,
                                 cid,
                                 handle,
                                 result,
                                 size_value,
                                 data);

}

/***************************************************************************
NAME
    sendCsisServerAccessWriteErrorRsp

DESCRIPTION
    Send an error access response to the GATT  library.
*/
static void sendCsisServerAccessWriteErrorRsp(CsrBtGattId task,
                            const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
                            uint16 error)
{
    sendCsisServerAccessWriteRsp(task,
                            access_ind->cid,
                            access_ind->handle,
                            error,
                            0,
                            NULL);
}


/***************************************************************************
NAME
    sendCsisServerAccessErrorRsp

DESCRIPTION
    Send an error access response to the GATT  library.
*/
static void sendCsisServerAccessErrorRsp(CsrBtGattId task,
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
static status_t csisServerSetCCC(GCSISS_T *const csis_server,
                            connection_id_t cid,
                            uint16 handle,
                            uint8 *ccc)
{
    uint8 index_client = csisServerGetCidIndex(csis_server, cid);
    bool clientConfigChanged = FALSE;

    if(index_client != GATT_CSIS_SERVER_INVALID_CID_INDEX)
    {
        if (handle == HANDLE_LOCK_CLIENT_CONFIG)
        {
            /* Check if the client config has changed, to notify above layer */
            clientConfigChanged = csisServerClientConfigChanged(
                  csis_server->data.connected_clients[index_client].client_cfg.lockValueClientCfg,
                                  ccc[0]);

            csis_server->data.connected_clients[index_client].client_cfg.lockValueClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SIRK_CLIENT_CONFIG)
        {
            /* Check if the client config has changed, to notify above layer */
            clientConfigChanged = csisServerClientConfigChanged(
                  csis_server->data.connected_clients[index_client].client_cfg.sirkValueClientCfg,
                                  ccc[0]);

            csis_server->data.connected_clients[index_client].client_cfg.sirkValueClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SIZE_CLIENT_CONFIG)
        {
            /* Check if the client config has changed, to notify above layer */
            clientConfigChanged = csisServerClientConfigChanged(
                  csis_server->data.connected_clients[index_client].client_cfg.sizeValueClientCfg,
                                  ccc[0]);

            csis_server->data.connected_clients[index_client].client_cfg.sizeValueClientCfg = ccc[0];
        }
        else
        {
            /* Invalid handle */
            GATT_CSIS_SERVER_ERROR("Invalid handle!\n");
            return CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE;
        }

        /* Inform application for client write operation */
        csisServerSetClientConfigWrite(csis_server, cid, handle, clientConfigChanged);
    }
    else
    {
        /* Invalid cid */
        GATT_CSIS_SERVER_ERROR("Invalid cid!\n");
        return CSR_BT_GATT_ACCESS_RES_INVALID_TRANSPORT;
    }

    return CSR_BT_GATT_ACCESS_RES_SUCCESS;
}

/***************************************************************************
NAME
    csisServerServiceAccess

DESCRIPTION
    Deals with access of the HANDLE_COORDINATED_SET_IDENTIFICATION_SERVICE handle.
*/
static void csisServerServiceAccess(CsrBtGattId task, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendCsisServerAccessRsp(task,
                                access_ind->cid,
                                HANDLE_COORDINATED_SET_IDENTIFICATION_SERVICE,
                                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                0,
                                NULL);
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        /* Write of CSIS not allowed. */
    	sendCsisServerAccessWriteErrorRsp(task, access_ind, CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED);
    }
    else
    {
        /* Reject access requests that aren't read/write, which shouldn't happen. */
        sendCsisServerAccessErrorRsp(task, access_ind, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
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
                csis_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
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
        CsrBtTpdAddrT tpaddr = { 0 };
        memset(&tpaddr, 0, sizeof(CsrBtTpdAddrT));

        result = csisServerEvaluateLockValue(
                               access_ind->cid,
                               access_ind->size_value,
                               (GattCsisServerLocktype )access_ind->value[0],
                               &update_timer,
                               &tpaddr);

        /* Send Write response to GATT  */
        sendCsisServerAccessWriteRsp(
                          csis_server->gattId,
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
        else if (result == CSR_BT_GATT_ACCESS_RES_SUCCESS)
        {
            /* if lock is already unlocked and client(s) tries to unlock
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

                csis_tId = CsrSchedTimerSet((uint32)(CSIS_LOCK_TIMEOUT_1_SEC *csisServerGetLockTimeout()),
                                            csisServerHandleLockTimerExpiry,
                                            0x0,
                                            csis_server);

            }
            else if ((GattCsisServerLocktype)access_ind->value[0] == CSIS_SERVER_UNLOCKED)
            {
                CsrSchedTimerCancel(csis_tId, 0, NULL);
            }

            {
                /* Inform application about Lock*/
                MAKE_CSIS_SERVER_MESSAGE(GattCsisLockStateInd);
                message->csisHandle = GattCsisServerGetLock();
                message->cid = access_ind->cid;
                message->lockValue = access_ind->value[0];

                CsisMessageSend(csis_server->app_task, GATT_CSIS_LOCK_STATE_IND, message);
            }
        }

    }
    else
    {
        /* Reject access requests that aren't read/write, which shouldn't happen. */
        sendCsisServerAccessErrorRsp(csis_server->gattId, access_ind, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}


/***************************************************************************
NAME
    csisServerSirkAccess

DESCRIPTION
    Deals with access of the Lock CSIS handle.
*/
static void csisServerSirkAccess(GCSISS_T *csis_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint8 i, j;
        uint8 *value;

        if((csis_server->data.sirkOp & GATT_CSIS_SHARE_SIRK_OOB_ONLY) == GATT_CSIS_SHARE_SIRK_OOB_ONLY)
        {
            sendCsisServerAccessErrorRsp(
                    csis_server->gattId,
                    access_ind,
                    GATT_CSIS_OOB_SIRK_ONLY
                    );

            return;
        }
        if((csis_server->data.sirkOp & GATT_CSIS_SHARE_SIRK_ENCRYPTED) == GATT_CSIS_SHARE_SIRK_ENCRYPTED)
        {
            /* Generate encrypted SIRK */
            if (csisServerSirk.cid == 0)
            {
               csisServerGenerateEncryptedSirk(csis_server, GATT_CSIS_SERVER_READ_ACCESS, 0, access_ind->cid);
            }
            else
            {
                /* Request is currently in progress for a cid - Send Busy ?*/
                sendCsisServerAccessErrorRsp(csis_server->gattId,
                                             access_ind,
                                             CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES);
            }

            return;
        }

        value = (uint8*)CsrPmemAlloc(sizeof(uint8)*GATT_CSIS_SERVER_SIRK_SIZE_PLUS_TYPE_SIZE);

        /* default plain text */
        value[0] = GATT_CSIS_PLAIN_TEXT_SIRK;

        for(i=0, j= GATT_CSIS_SERVER_SIRK_SIZE-1; i< GATT_CSIS_SERVER_SIRK_SIZE && j>=0; i++, j--)
        {
             value[1+i] =csis_server->data.sirk[j];
        }

        sendCsisServerAccessRsp(
                csis_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                GATT_CSIS_SERVER_SIRK_SIZE_PLUS_TYPE_SIZE,
                value);

        CsrPmemFree(value);
    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendCsisServerAccessErrorRsp(csis_server->gattId, access_ind, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
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
                csis_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                GATT_CSIS_SERVER_SIZE_CHARACTERISTCIS_SIZE,
                &value);
    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendCsisServerAccessErrorRsp(csis_server->gattId, access_ind, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
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
                csis_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                GATT_CSIS_SERVER_RANK_CHARACTERISTCIS_SIZE,
                &value
                );
    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendCsisServerAccessErrorRsp(csis_server->gattId, access_ind, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}


/***************************************************************************/
static void csisHandleReadClientConfigAccess(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        const uint16 client_config
        )
{
    uint8 *config_data;
    config_data = (uint8*)CsrPmemAlloc(sizeof(uint8)*GATT_CSIS_SERVER_CCC_VALUE_SIZE);

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_CSIS_SERVER_PANIC(
                    "CSIS: Null instance!\n"
                    );
    }
    else if (cid == 0)
    {
        GATT_CSIS_SERVER_PANIC(
                    "CSIS: Null instance!\n"
                    );
    }

    /* Default value of clientConfig is set as 0xFFFF. If client has not written
       any CCCD then we need to replace 0xFFFF with 0x0 (Disable) while
       responding. Default value is changed from 0 to 0xFFFF because of
       CCCD values getting lost if the device panics without these values are
       passed to application.
    */
    if(client_config != GATT_CSIS_SERVER_INVALID_CLIENT_CONFIG)
    {
        config_data[0] = (uint8)(client_config & 0xFF);
        config_data[1] = (uint8)(client_config >> 8);
    }
    else
    {
        config_data[0] = 0;
        config_data[1] = 0;
    }

    sendCsisServerAccessRsp(
            task,
            cid,
            handle,
            CSR_BT_GATT_ACCESS_RES_SUCCESS,
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
        sendCsisServerAccessWriteErrorRsp(
                csis_server->gattId,
                access_ind,
                CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH
                );
    }
    /* Validate the input parameters - ONLY Notify*/
    else if ( access_ind->value[0] == GATT_CSIS_SERVER_CCC_INDICATE )
    {
        sendCsisServerAccessWriteErrorRsp(
                csis_server->gattId,
                access_ind,
                CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF
                );
    }
    else if ( access_ind->value[0] == GATT_CSIS_SERVER_CCC_NOTIFY || access_ind->value[0] == 0 )
    {
        /* Valid value of CCC */

        /* Save the new ccc in the library */
        status_t status = csisServerSetCCC(
                                   csis_server,
                                   (connection_id_t) access_ind->cid,
                                   access_ind->handle,
                                   access_ind->value);

        /* Send response to the client */
        sendCsisServerAccessWriteRsp(
             csis_server->gattId,
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
        sendCsisServerAccessWriteRsp(
             csis_server->gattId,
             access_ind->cid,
             access_ind->handle,
             CSR_BT_GATT_ACCESS_RES_SUCCESS,
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
                    csis_server->gattId,
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
                    csis_server->gattId,
                    access_ind,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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

    if (index_client == GATT_CSIS_SERVER_INVALID_CID_INDEX)
    {
        GATT_CSIS_SERVER_ERROR("Invalid cid!\n");
        return;
    }

    switch (access_ind->handle)
    {
        case HANDLE_COORDINATED_SET_IDENTIFICATION_SERVICE:
        {
           /* Service Handle read request */
           csisServerServiceAccess(csis_server->gattId, access_ind);
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
            sendCsisServerAccessErrorRsp(csis_server->gattId, access_ind,CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        }
        break;
    }
}

void csisServerGenerateEncryptedSirk(GCSISS_T *csis_server,
                                    uint8 operation,
                                    uint8 index,
                                    connection_id_t cid)
{
    CsrBtTpdAddrT clientTpAddr;

    if (cid != 0)
        csisServerSirk.cid = cid;
    else
    {
        csisServerSirk.cid = csis_server->data.connected_clients[index].cid;
    }

    csisServerSirk.operation = operation;
    clientTpAddr = csisServerGetTpAddrFromCid(cid);

    CsrBtCmLeSirkOperationReqSend(
              csis_server->lib_task,
              clientTpAddr,
              0,
              &csis_server->data.sirk[0]
              );
}


void csisServerhandleSirkOperation(GCSISS_T *csis_server,
                 const CsrBtCmLeSirkOperationCfm * cfm)
{
    CsrBtTpdAddrT clientTpAddr;
    uint8 i,j;
    uint8 value[GATT_CSIS_SERVER_SIRK_SIZE_PLUS_TYPE_SIZE];

    if (cfm->resultCode != 0)
    {
        csisServerSirk.operation = GATT_CSIS_SERVER_NONE;

        value[0] = GATT_CSIS_ENCRYPTED_SIRK;

        for(i=0, j= GATT_CSIS_SERVER_SIRK_SIZE-1; i< GATT_CSIS_SERVER_SIRK_SIZE && j>=0; i++, j--)
        {
            value[1+i] = cfm->sirkKey[j];
        }

        sendCsisServerAccessRsp(
                        csis_server->gattId,
                        csisServerSirk.cid,
                        HANDLE_SIRK,
                        CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR,
                        GATT_CSIS_SERVER_SIRK_SIZE_PLUS_TYPE_SIZE,
                        value
                        );
        return;
    }

    if (csisServerSirk.cid != 0)
    {
        clientTpAddr = csisServerGetTpAddrFromCid(csisServerSirk.cid);

        if (CsrBtAddrEq(&(cfm->tpAddrt.addrt), &(clientTpAddr.addrt)))
        {
            value[0] = GATT_CSIS_ENCRYPTED_SIRK;

            for(i=0, j= GATT_CSIS_SERVER_SIRK_SIZE-1; i< GATT_CSIS_SERVER_SIRK_SIZE && j>=0; i++, j--)
            {
                value[1+i] = cfm->sirkKey[j];
            }

            if (csisServerSirk.operation == GATT_CSIS_SERVER_READ_ACCESS)
            {
                sendCsisServerAccessRsp(
                        csis_server->gattId,
                        csisServerSirk.cid,
                        HANDLE_SIRK,
                        CSR_BT_GATT_ACCESS_RES_SUCCESS,
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
                                    (csis_server->gattId),
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

