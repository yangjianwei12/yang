/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_vcs_server_msg_handler.h"
#include "gatt_vcs_server_access.h"
#include "gatt_vcs_server_common.h"
#include "gatt_vcs_server_db.h"

/******************************************************************************/
ServiceHandle vcs_service_handle;

static void vcsServerHandleChangeVolumeState(GVCS *volume_control_server)
{
    uint8 i;
    uint8* value = NULL;

    vcsServerHandleChangeCounter(volume_control_server);

    for (i=0; i<GATT_VCS_MAX_CONNECTIONS; i++)
    {
        if (volume_control_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (volume_control_server->data.connected_clients[i].client_cfg.volumeStateClientCfg == GATT_VCS_SERVER_CCC_NOTIFY)
            {
                value = (uint8*) CsrPmemAlloc(GATT_VCS_SERVER_VOLUME_STATE_SIZE);

                vcsServerComposeVolumeStateValue(value, volume_control_server);

                vcsServerSendCharacteristicChangedNotification(
                        volume_control_server->gattId,
                        volume_control_server->data.connected_clients[i].cid,
                        HANDLE_VOLUME_STATE,
                        GATT_VCS_SERVER_VOLUME_STATE_SIZE,
                        value
                        );
            }
        }
    }

    if (!volume_control_server->data.volume_flag)
    {
        /* It's the first time the volume setting is changed */
        vcsServerSetVolumeFlag(volume_control_server);
    }
}

static bool vcsServerConnected(GVCS *volume_control_server)
{
    uint8 i;

    for (i=0; i < GATT_VCS_MAX_CONNECTIONS; i++)
    {
        if (volume_control_server->data.connected_clients[i].cid != 0)
        {
            /* If any cid value is non-zero, it means we have an active connection */
            return TRUE;
        }
    }

    return FALSE;
}

/******************************************************************************/
ServiceHandle GattVcsServerInit(
                     AppTask    theAppTask,
                     uint16  startHandle,
                     uint16  endHandle,
                     GattVcsInitData* initData)
{
    GVCS *gatt_vcs_server_inst = NULL;
    uint8 i;

    if (theAppTask == CSR_SCHED_QID_INVALID)
    {
        GATT_VCS_SERVER_PANIC("Application Task NULL\n");
    }

    vcs_service_handle = ServiceHandleNewInstance((void **) &gatt_vcs_server_inst, sizeof(GVCS));

    if (gatt_vcs_server_inst)
    {
        /* Reset all the service library memory */
        CsrMemSet(gatt_vcs_server_inst, 0, sizeof(GVCS));

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        gatt_vcs_server_inst->app_task = theAppTask;


        /* Initiliasation of the Volume Control Charateristics  and parameters*/
        gatt_vcs_server_inst->data.volume_setting = initData->volumeSetting;
        gatt_vcs_server_inst->data.mute = initData->mute;
        gatt_vcs_server_inst->data.change_counter = initData->changeCounter;
        gatt_vcs_server_inst->data.volume_flag = GATT_VCS_SERVER_VOLUME_SETTING_NOT_PERSISTED;
        gatt_vcs_server_inst->data.step_size = initData->stepSize;

        gatt_vcs_server_inst->srvc_hndl = vcs_service_handle;
        gatt_vcs_server_inst->start_handle = startHandle;
        gatt_vcs_server_inst->end_handle = endHandle;

        /* Reset the client data memory */
        CsrMemSet(gatt_vcs_server_inst->data.connected_clients, 0, (sizeof(gatt_vcs_client_data) * GATT_VCS_MAX_CONNECTIONS));

        /* Reset the client config value to default GATT_VCS_SERVER_INVALID_CLIENT_CONFIG */
        for(i = 0; i < GATT_VCS_MAX_CONNECTIONS; i++)
        {
           gatt_vcs_server_inst->data.connected_clients[i].client_cfg.volumeStateClientCfg = GATT_VCS_SERVER_INVALID_CLIENT_CONFIG;
           gatt_vcs_server_inst->data.connected_clients[i].client_cfg.volumeFlagClientCfg = GATT_VCS_SERVER_INVALID_CLIENT_CONFIG;
        }

        /* Register with the GATT */

        CsrBtGattRegisterReqSend(CSR_BT_VCS_SERVER_IFACEQUEUE,
                                 0);

        return gatt_vcs_server_inst->srvc_hndl;

    }
    else
    {
        GATT_VCS_SERVER_PANIC("Memory alllocation of VCS Server instance failed!\n");
        return 0;
    }
}

/******************************************************************************/
status_t GattVcsServerAddConfig(
                  ServiceHandle srvcHndl,
                  connection_id_t cid,
                  GattVcsServerConfig *const config)
{
    uint8 i;
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);

    if(volume_control_server)
    {
        for(i=0; i<GATT_VCS_MAX_CONNECTIONS; i++)
        {
            if(volume_control_server->data.connected_clients[i].cid == 0)
            {
                /* Check config parameter:
                 * If config is NULL, it indicates a default config should be used for the
                 * peer device identified by the CID.
                 * The default config is already set when the instance has been initialised.
                 */
                if (config)
                {
                    if (config->volumeStateClientCfg == GATT_VCS_SERVER_CCC_INDICATE ||
                            config->volumeFlagClientCfg == GATT_VCS_SERVER_CCC_INDICATE)
                    {
                        /* Volume State and Volume Flag characteristics can be only notified */
                        GATT_VCS_SERVER_ERROR("Invalid Client Configuration Characteristic!\n");
                        return CSR_BT_GATT_RESULT_INVALID_ATTRIBUTE_VALUE_RECEIVED;
                    }

                    /* Save new ccc for the client */
                    volume_control_server->data.connected_clients[i].client_cfg.volumeStateClientCfg = config->volumeStateClientCfg;
                    volume_control_server->data.connected_clients[i].client_cfg.volumeFlagClientCfg = config->volumeFlagClientCfg;

                    if (config->volumeStateClientCfg == GATT_VCS_SERVER_CCC_NOTIFY)
                    {
                        /* If its ccc is NOTIFY, we have to notify the Volume State characteristic value */
                        uint8* value = (uint8*) CsrPmemAlloc(GATT_VCS_SERVER_VOLUME_STATE_SIZE);

                        vcsServerComposeVolumeStateValue(value, volume_control_server);

                        vcsServerSendCharacteristicChangedNotification(
                                    volume_control_server->gattId,
                                    cid,
                                    HANDLE_VOLUME_STATE,
                                    GATT_VCS_SERVER_VOLUME_STATE_SIZE,
                                    value);
                    }

                    if (config->volumeFlagClientCfg == GATT_VCS_SERVER_CCC_NOTIFY)
                    {
                        /* If its ccc is NOTIFY, we have to notify the Volume State characteristic value */
                        uint8* value = (uint8*) CsrPmemAlloc(sizeof(uint8)*GATT_VCS_SERVER_VOLUME_FLAG_SIZE);
                        *value = volume_control_server->data.volume_flag;
                        /* If its ccc is NOTIFY, we have to notify the Volume Flag characteristic value */
                        vcsServerSendCharacteristicChangedNotification(
                                    volume_control_server->gattId,
                                    cid,
                                    HANDLE_VOLUME_FLAGS,
                                    GATT_VCS_SERVER_VOLUME_FLAG_SIZE,
                                    value);
                    }
                }

                volume_control_server->data.connected_clients[i].cid = cid;

                return CSR_BT_GATT_RESULT_SUCCESS;
            }
        }
    }
    else
    {
        return CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
    }

    return CSR_BT_GATT_RESULT_INSUFFICIENT_NUM_OF_HANDLES;
}

/******************************************************************************/
GattVcsServerConfig *GattVcsServerRemoveConfig(
                              ServiceHandle srvcHndl,
                              connection_id_t  cid)
{
    uint8 i;
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);
    GattVcsServerConfig *config = NULL;

    if(volume_control_server)
    {
        for(i=0; i<GATT_VCS_MAX_CONNECTIONS; i++)
        {
            /* Check the saved CID to find the peeer device */
            if (volume_control_server->data.connected_clients[i].cid == cid)
            {
                /* Found peer device:
                 * - save last client configurations
                 * - remove the peer device
                 * - free the server instance
                 * - return last client configuration
                 */

                config = (GattVcsServerConfig *)CsrPmemAlloc(sizeof(GattVcsServerConfig));
                CsrMemCpy(config, &(volume_control_server->data.connected_clients[i].client_cfg), sizeof(GattVcsServerConfig));

                if ((i == (GATT_VCS_MAX_CONNECTIONS-1)) ||
                    (i == 0 && volume_control_server->data.connected_clients[i+1].cid == 0))
                {
                    /* The peer device is the only or the last element of the array */
                    CsrMemSet(&(volume_control_server->data.connected_clients[i]), 0, sizeof(gatt_vcs_client_data));
                }
                else
                {
                    /* The peer device is in the middle of the array */
                    uint8 j;

                    for (j=i; j<(GATT_VCS_MAX_CONNECTIONS-1) && volume_control_server->data.connected_clients[j+1].cid != 0; j++)
                    {
                        /* Shift all the elements of the array of one position behind */
                        CsrMemMove(&(volume_control_server->data.connected_clients[j]),
                                   &(volume_control_server->data.connected_clients[j+1]),
                                   sizeof(gatt_vcs_client_data));
                    }

                    /* Remove the last element of the array, already shifted behind */
                    CsrMemSet(&(volume_control_server->data.connected_clients[j]), 0, sizeof(gatt_vcs_client_data));
                }
                break;
            }
        }
    }
    return config;
}

/******************************************************************************/
GattVcsServerConfig* GattVcsServerGetConfig(
                        ServiceHandle srvcHndl,
                        connection_id_t  cid)
{
    uint8 i;
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);
    GattVcsServerConfig *config;

    if(volume_control_server)
    {
        for(i=0; i<GATT_VCS_MAX_CONNECTIONS; i++)
        {
            /* Check the saved CID to find the peeer device */
            if (volume_control_server->data.connected_clients[i].cid == cid)
            {
                /* Found peer device:
                 * - save last client configurations
                 * - return last client configuration
                 */

                config = (GattVcsServerConfig *)CsrPmemAlloc(sizeof(GattVcsServerConfig));
                CsrMemCpy(config, &(volume_control_server->data.connected_clients[i].client_cfg), sizeof(GattVcsServerConfig));
                return config;
            }
        }
    }
    return NULL;

}

/******************************************************************************/
bool GattVcsServerSetVolume(ServiceHandle srvcHndl, uint8 volume)
{
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);

    if (volume_control_server && (volume != volume_control_server->data.volume_setting))
    {
        /* The Volume value to set is different from the actual one:
           - save the new value
           - increment the change counter
           - notify the clients have been registered
           - handle the Volume Flag value
        */
        volume_control_server->data.volume_setting = volume;

        vcsServerHandleChangeVolumeState(volume_control_server);

        return TRUE;
    }

    return FALSE;
}

/******************************************************************************/
bool GattVcsServerSetMute(ServiceHandle srvcHndl, uint8 mute)
{
    bool res = FALSE;
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);

    if (volume_control_server &&
        (mute == GATT_VCS_SERVER_MUTE_VALUE || mute == GATT_VCS_SERVER_UNMUTE_VALUE) &&
        (mute != volume_control_server->data.mute))
    {
        /* The Mute value to set is valid and different from the actual one:
           - save the new value
           - increment the change counter
           - notify the clients have been registered
           - handle the Volume Flag value
        */
        volume_control_server->data.mute = mute;

        vcsServerHandleChangeVolumeState(volume_control_server);

        res = TRUE;
    }
    return res;
}

/******************************************************************************/
uint8 GattVcsServerGetVolume(ServiceHandle srvcHndl)
{
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);
    uint8 volume_setting = volume_control_server ? volume_control_server->data.volume_setting : 0;
    return volume_setting;
}

/******************************************************************************/
uint8 GattVcsServerGetMute(ServiceHandle srvcHndl)
{
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);
    uint8 mute = volume_control_server ? volume_control_server->data.mute : GATT_VCS_SERVER_INVALID_MUTE_VALUE;
    return mute;
}

/******************************************************************************/
uint8 GattVcsServerGetVolumeFlag(ServiceHandle srvcHndl)
{
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);
    uint8 volume_flag = volume_control_server ? volume_control_server->data.volume_flag : GATT_VCS_SERVER_INVALID_VOLUME_FLAG_VALUE;
    return volume_flag;
}

/******************************************************************************/
bool GattVcsServerSetVolumeFlag(ServiceHandle srvcHndl, uint8 volumeFlag)
{
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);

    if (volume_control_server && (!vcsServerConnected(volume_control_server)) &&
           (volumeFlag == GATT_VCS_SERVER_VOLUME_SETTING_NOT_PERSISTED ||
            volumeFlag == GATT_VCS_SERVER_VOLUME_SETTING_PERSISTED))
    {
        volume_control_server->data.volume_flag = volumeFlag;
        return TRUE;
    }

    return FALSE;
}

/******************************************************************************/
bool GattVcsServerSetVolumeMuteCc(ServiceHandle srvcHndl, uint8 volume, uint8 mute, uint8 change_counter)
{
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);

    if (volume_control_server)
    {
        bool changed = FALSE;

        if (!(vcsServerConnected(volume_control_server)))
        {
            volume_control_server->data.volume_setting = volume;
            volume_control_server->data.mute = mute;
            volume_control_server->data.change_counter = change_counter;

            return TRUE;
        }

        if (volume_control_server->data.volume_setting != volume)
        {
            volume_control_server->data.volume_setting = volume;
            changed = TRUE;
        }

        if ((mute == GATT_VCS_SERVER_MUTE_VALUE || mute == GATT_VCS_SERVER_UNMUTE_VALUE) &&
                (mute != volume_control_server->data.mute))
        {
            volume_control_server->data.mute = mute;
            changed = TRUE;
        }

        if (changed)
        {
            vcsServerHandleChangeVolumeState(volume_control_server);
        }

        return TRUE;
    }

    return FALSE;
}

/******************************************************************************/
/*Synergy Task Init*/

void gatt_vcs_server_init(void** gash)
{
    *gash = &vcs_service_handle;
    GATT_VCS_SERVER_INFO("VCS: gatt_vcs_client_init\n\n");

}

/*Synergy Task deinit*/
#ifdef ENABLE_SHUTDOWN
void gatt_vcs_server_deinit(void** gash)
{
    ServiceHandle srvc_hndl = *((ServiceHandle*)*gash);
    if(ServiceHandleFreeInstanceData(srvc_hndl))
    {
        GATT_VCS_SERVER_INFO("GVCS: gatt_vcs_client_deinit\n\n");
    }
    else
    {
        GATT_VCS_SERVER_PANIC("Unable to free the VCS server instance\n");
    }
}
#endif
