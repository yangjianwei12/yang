/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_vcs_server_common.h"
#include "gatt_vcs_server_debug.h"
#include "gatt_vcs_server_db.h"

#define GATT_VCS_SERVER_INVALID_CID_INDEX  (0xFF)

static gatt_status_t vcsServerSetCCC(GVCS *volume_control_server,
                            connection_id_t cid,
                            uint16 handle,
                            uint8 *ccc)
{
    uint8 index_client = vcsServerGetCidIndex(volume_control_server, cid);

    if(index_client != GATT_VCS_SERVER_INVALID_CID_INDEX)
    {
        if (handle == HANDLE_VOLUME_STATE_CLIENT_CONFIG)
        {
            volume_control_server->data.connected_clients[index_client].client_cfg.volumeStateClientCfg = ccc[0];
        }
        else if (handle == HANDLE_VOLUME_FLAGS_CLIENT_CONFIG)
        {
            volume_control_server->data.connected_clients[index_client].client_cfg.volumeFlagClientCfg = ccc[0];
        }
        else
        {
            /* Invalid handle */
            GATT_VCS_SERVER_DEBUG_INFO(("Invalid handle!\n"))
            return gatt_status_invalid_handle;
        }
    }
    else
    {
        /* Invalid cid */
        GATT_VCS_SERVER_DEBUG_INFO(("Invalid cid!\n"))
        return gatt_status_invalid_cid;
    }

    return gatt_status_success;
}

/******************************************************************************/
void vcsServerSendAccessRsp(
        Task task,
        uint16 cid,
        uint16 handle,
        uint16 result,
        uint16 size_value,
        const uint8 *value
        )
{
    if (!GattManagerServerAccessResponse(
                task,
                cid,
                handle,
                result,
                size_value,
                value
                )
       )
    {
        GATT_VCS_SERVER_DEBUG_PANIC((
                    "Couldn't send GattManagerServerAccessRsp\n"
                    ));

    }
}

/******************************************************************************/
void vcsServerWriteGenericResponse(
        Task        task,
        uint16      cid,
        uint16      result,
        uint16      handle
        )
{
    if (task == NULL)
    {
        GATT_VCS_SERVER_DEBUG_PANIC((
                    "GVCS: Null instance!\n"
                    ));
    }
    else if (cid == 0)
    {
        GATT_VCS_SERVER_DEBUG_PANIC((
                    "GVCS: No Cid!\n"
                    ));
    }
    else
    {
        vcsServerSendAccessRsp(
             task,
             cid,
             handle,
             result,
             0,
             NULL
             );
    }
}

/***************************************************************************/
void vcsServerSendCharacteristicChangedNotification(
        Task task,
        connection_id_t cid,
        uint16 handle,
        uint16 size_value,
        const uint8 *value
        )
{
    if (task == NULL)
    {
        GATT_VCS_SERVER_DEBUG_PANIC((
                    "GVCS: Null instance!\n"
                    ));
    }
    else if ( cid == 0 )
    {
        GATT_VCS_SERVER_DEBUG_PANIC((
                    "GVCS: No Cid!\n"
                    ));
    }
    else
    {
        GattManagerRemoteClientNotify(
                                      task,
                                      (uint16) cid,
                                      handle,
                                      size_value,
                                      value);
    }
}

/***************************************************************************/
void vcsServerHandleReadClientConfigAccess(
        Task task,
        uint16 cid,
        uint16 handle,
        const uint16 client_config
        )
{
    uint8 config_data[GATT_VCS_SERVER_CCC_VALUE_SIZE];

    if (task == NULL)
    {
        GATT_VCS_SERVER_DEBUG_PANIC((
                    "GVCS: Null instance!\n"
                    ));
    }
    else if (cid == 0)
    {
        GATT_VCS_SERVER_DEBUG_PANIC((
                    "GVCS: Null instance!\n"
                    ));
    }

    config_data[0] = (uint8)client_config & 0xFF;
    config_data[1] = (uint8)client_config >> 8;

    vcsServerSendAccessRsp(
            task,
            cid,
            handle,
            gatt_status_success,
            GATT_VCS_SERVER_CCC_VALUE_SIZE,
            config_data
            );
}

/***************************************************************************/
void vcsServerHandleWriteClientConfigAccess(
        GVCS *volume_control_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->size_value != GATT_VCS_SERVER_CCC_VALUE_SIZE)
    {
        vcsServerSendAccessErrorRsp(
                (Task) &volume_control_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_invalid_length
                );
    }
    /* Validate the input parameters - ONLY Notify*/
    else if (access_ind->value[0] == GATT_VCS_SERVER_CCC_INDICATE)
    {
        vcsServerSendAccessErrorRsp(
                (Task) &volume_control_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_cccd_improper_config
                );
    }
    else if (access_ind->value[0] == GATT_VCS_SERVER_CCC_NOTIFY || access_ind->value[0] == 0)
    {
        /* Valid value of CCC */

        /* Save the new ccc in the library */
        gatt_status_t status = vcsServerSetCCC(
                                   volume_control_server,
                                   (connection_id_t) access_ind->cid,
                                   access_ind->handle,
                                   access_ind->value);

        /* Send response to the client */
        vcsServerWriteGenericResponse(
                    (Task) &volume_control_server->lib_task,
                    access_ind->cid,
                    status,
                    access_ind->handle
                    );
    }
    else
    {
        /* Send response to the client but the value is ignored*/
        vcsServerWriteGenericResponse(
                    (Task) &volume_control_server->lib_task,
                    access_ind->cid,
                    gatt_status_success,
                    access_ind->handle
                    );
    }
}

/***************************************************************************/

void vcsServerComposeVolumeStateValue(
        uint8 *value,
        const GVCS *volume_control_server
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
