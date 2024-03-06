/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include "gatt_vcs_server_common.h"
#include "gatt_vcs_server_debug.h"
#include "gatt_vcs_server_db.h"

static void vcsServerSendConfigChangeIndication(GVCS *volume_control_server,
    connection_id_t cid,
    bool configChangeComplete)
{
    /* Indicate the application all CCCD are written by client */
    MAKE_VCS_MESSAGE(GattVcsServerConfigChangeInd);

    message->vcsServiceHandle = volume_control_server->srvc_hndl;
    message->cid = cid;
    message->id = GATT_VCS_SERVER_CONFIG_CHANGE_IND;
    message->configChangeComplete = configChangeComplete;

    VcsServerMessageSend(volume_control_server->app_task, message);
}

static bool vcsServerAllClientConfigWritten(GVCS *volume_control_server,
    connection_id_t cid)
{
    uint8 i;

    for(i = 0; i < GATT_VCS_MAX_CONNECTIONS; i++)
    {
        if(volume_control_server->data.connected_clients[i].cid == cid &&
            volume_control_server->data.connected_clients[i].client_cfg.volumeStateClientCfg != GATT_VCS_SERVER_INVALID_CLIENT_CONFIG &&
            volume_control_server->data.connected_clients[i].client_cfg.volumeFlagClientCfg != GATT_VCS_SERVER_INVALID_CLIENT_CONFIG)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void vcsServerSetClientConfigWrite(GVCS *volume_control_server,
                            connection_id_t cid,
                            bool clientConfigChanged)
{
    bool configChangeComplete = vcsServerAllClientConfigWritten(volume_control_server, cid);

    if (clientConfigChanged)
    {
        /* Inform above layer when all CCCD handles are written by client*/
        vcsServerSendConfigChangeIndication(volume_control_server,
                                         cid,
                                         configChangeComplete);
    }
}

static bool vcsServerClientConfigChanged(uint16 clientCfg, uint8 newClientCfg)
{
    /* Check if the client config has changed, to notify above layer */
    if(((uint8)clientCfg) != newClientCfg)
        return TRUE;

    return FALSE;
}

static status_t vcsServerSetCCC(GVCS *volume_control_server,
                            connection_id_t cid,
                            uint16 handle,
                            uint8 *ccc)
{
    uint8 index_client = vcsServerGetCidIndex(volume_control_server, cid);
    bool clientConfigChanged = FALSE;

    if(index_client != GATT_VCS_SERVER_INVALID_CID_INDEX)
    {
        if (handle == HANDLE_VOLUME_STATE_CLIENT_CONFIG)
        {
            /* Check if the client config has changed, to notify above layer */
            clientConfigChanged = vcsServerClientConfigChanged(
                  volume_control_server->data.connected_clients[index_client].client_cfg.volumeStateClientCfg,
                                  ccc[0]);

            volume_control_server->data.connected_clients[index_client].client_cfg.volumeStateClientCfg = ccc[0];
        }
        else if (handle == HANDLE_VOLUME_FLAGS_CLIENT_CONFIG)
        {
            /* Check if the client config has changed, to notify above layer */
            clientConfigChanged = vcsServerClientConfigChanged(
                  volume_control_server->data.connected_clients[index_client].client_cfg.volumeFlagClientCfg,
                                  ccc[0]);

            volume_control_server->data.connected_clients[index_client].client_cfg.volumeFlagClientCfg = ccc[0];
        }
        else
        {
            /* Invalid handle */
            GATT_VCS_SERVER_ERROR("Invalid handle!\n");
            return CSR_BT_GATT_RESULT_INVALID_HANDLE_RANGE;
        }

        /* Inform application for client write operation */
        vcsServerSetClientConfigWrite(volume_control_server, cid, clientConfigChanged);
    }
    else
    {
        /* Invalid cid */
        GATT_VCS_SERVER_ERROR("Invalid cid!\n");
        return CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID;
    }
    return CSR_BT_GATT_RESULT_SUCCESS;
}

/******************************************************************************/
void vcsServerSendAccessRsp(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 size_value,
        uint8 *const value
        )
{
    CsrBtGattDbReadAccessResSend(task,
                                 cid,
                                 handle,
                                 result,
                                 size_value,
                                 value);
}

/******************************************************************************/
void vcsServerWriteGenericResponse(
        CsrBtGattId task,
        connection_id_t  cid,
        uint16      result,
        uint16      handle
        )
{
    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_VCS_SERVER_PANIC(
                    "GVCS: Invalid GattId!\n"
                    );
    }
    else if (cid == 0)
    {
        GATT_VCS_SERVER_PANIC(
                    "GVCS: No Cid!\n"
                    );
    }
    else
    {
        CsrBtGattDbWriteAccessResSend(task,
                                      cid,
                                      handle,
                                      (CsrBtGattDbAccessRspCode)result);
    }
}

/***************************************************************************/
void vcsServerSendCharacteristicChangedNotification(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 size_value,
        uint8 *const value
        )
{
    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_VCS_SERVER_PANIC(
                    "GVCS: Invalid Gatt Id!\n"
                    );
    }
    else if ( cid == 0 )
    {
        GATT_VCS_SERVER_PANIC(
                    "GVCS: No Cid!\n"
                    );
    }
    else
    {
        CsrBtGattNotificationEventReqSend(task,
                                          cid,
                                          handle,
                                          size_value,
                                          value);
    }
}

/***************************************************************************/
void vcsServerHandleReadClientConfigAccess(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        const uint16 client_config
        )
{
    uint8* config_data = (uint8*) CsrPmemAlloc(GATT_VCS_SERVER_CCC_VALUE_SIZE);

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_VCS_SERVER_PANIC(
                    "GVCS: Invalid GattID!\n"
                    );
    }
    else if (cid == 0)
    {
        GATT_VCS_SERVER_PANIC(
                    "GVCS: Null instance!\n"
                    );
    }

     /* Default value of clientConfig is set as 0xFFFF. If client has not written
        any CCCD then we need to replace 0xFFFF with 0x0 (Disable) while
        responding. Default value is changed from 0 to 0xFFFF because of
        CCCD values getting lost if the device panics without these values are
        passed to application.
      */
    if(client_config != GATT_VCS_SERVER_INVALID_CLIENT_CONFIG)
    {
        config_data[0] = (uint8)client_config & 0xFF;
        config_data[1] = (uint8)(client_config >> 8);
    }
    else
    {
        config_data[0] = 0;
        config_data[1] = 0;
    }

    vcsServerSendAccessRsp(
            task,
            cid,
            handle,
            CSR_BT_GATT_ACCESS_RES_SUCCESS,
            GATT_VCS_SERVER_CCC_VALUE_SIZE,
            config_data
            );
}

/***************************************************************************/
void vcsServerHandleWriteClientConfigAccess(
        GVCS *volume_control_server,
        GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind)
{
    if (access_ind->size_value != GATT_VCS_SERVER_CCC_VALUE_SIZE)
    {
        vcsServerSendAccessErrorRsp(
                volume_control_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH
                );
    }
    /* Validate the input parameters - ONLY Notify*/
    else if (access_ind->value[0] == GATT_VCS_SERVER_CCC_INDICATE)
    {
        vcsServerSendAccessErrorRsp(
                volume_control_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF
                );
    }
    else if (access_ind->value[0] == GATT_VCS_SERVER_CCC_NOTIFY || access_ind->value[0] == 0)
    {
        /* Valid value of CCC */

        /* Save the new ccc in the library */
        status_t status = vcsServerSetCCC(
                                   volume_control_server,
                                   (connection_id_t) access_ind->cid,
                                   access_ind->handle,
                                   access_ind->value);

        /* Send response to the client */
        vcsServerWriteGenericResponse(
                    volume_control_server->gattId,
                    access_ind->cid,
                    status,
                    access_ind->handle
                    );
    }
    else
    {
        /* Send response to the client but the value is ignored*/
        vcsServerWriteGenericResponse(
                    volume_control_server->gattId,
                    access_ind->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    access_ind->handle
                    );
    }
}

/***************************************************************************/

void vcsServerComposeVolumeStateValue(
        uint8 *value,
        GVCS *const volume_control_server
        )
{
    value[0] = volume_control_server->data.volume_setting;
    value[1] = volume_control_server->data.mute;
    value[2] = volume_control_server->data.change_counter;
}

/******************************************************************************/
void vcsServerHandleChangeCounter(GVCS *volume_control_server)
{
    if (volume_control_server->data.change_counter == GATT_VCS_SERVER_CHANGE_COUNTER_VALUE_MAX)
    {
        /* If the change counter has reached its maximum value, it has to be reset to 0 */
        volume_control_server->data.change_counter = 0;
    }
    else
    {
        /* If the change counter has NOT reached its maximum value, it has to be incremented */
        volume_control_server->data.change_counter += 1;
    }
}

/******************************************************************************/
uint8 vcsServerGetCidIndex(GVCS *volume_control_server, connection_id_t cid)
{
    uint8 index = GATT_VCS_SERVER_INVALID_CID_INDEX;
    uint8 i;

    for (i=0; i<GATT_VCS_MAX_CONNECTIONS; i++)
    {
        if(volume_control_server->data.connected_clients[i].cid == cid)
        {
            index = i;
            break;
        }
    }
    return index;
}

