/*******************************************************************************
Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 
*******************************************************************************/

#include "gatt_vcs_server.h"
#include "gatt_vcs_server_private.h"
#include "gatt_vcs_server_msg_handler.h"
#include "gatt_vcs_server_access.h"
#include "gatt_vcs_server_common.h"
#include "gatt_vcs_server_db.h"
#include "gatt_vcs_server_debug.h"

/******************************************************************************/
static void vcsServerHandleChangeVolumeState(GVCS *volume_control_server)
{
    uint8 i;

    vcsServerHandleChangeCounter(volume_control_server);

    for (i=0; i<GATT_VCS_MAX_CONNECTIONS; i++)
    {
        if (volume_control_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (volume_control_server->data.connected_clients[i].client_cfg.volumeStateClientCfg == GATT_VCS_SERVER_CCC_NOTIFY)
            {
                uint8 value[GATT_VCS_SERVER_VOLUME_STATE_SIZE];

                vcsServerComposeVolumeStateValue(value, volume_control_server);

                vcsServerSendCharacteristicChangedNotification(
                        (Task) &volume_control_server->lib_task,
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

/******************************************************************************/
ServiceHandle GattVcsServerInit(Task    theAppTask,
                                   uint16  startHandle,
                                   uint16  endHandle,
                                   GattVcsInitData initData)
{
    GVCS *gatt_vcs_server_inst = NULL;
    gatt_manager_server_registration_params_t reg_params;
    ServiceHandle srvc_hndl = 0;

    if (theAppTask == NULL)
    {
        GATT_VCS_SERVER_PANIC(("Application Task NULL\n"));
    }

    srvc_hndl = ServiceHandleNewInstance((void **) &gatt_vcs_server_inst, sizeof(GVCS));

    if (gatt_vcs_server_inst)
    {
        /* Reset all the service library memory */
        memset(gatt_vcs_server_inst, 0, sizeof(GVCS));

        /* Set up library handler for external messages */
        gatt_vcs_server_inst->lib_task.handler = vcsServerMsgHandler;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        gatt_vcs_server_inst->app_task = theAppTask;

        /* Save the service handle */
        gatt_vcs_server_inst->srvc_hndl = srvc_hndl;

        /* Initiliasation of the Volume Control Charateristics  and parameters*/
        gatt_vcs_server_inst->data.volume_setting = initData.volumeSetting;
        gatt_vcs_server_inst->data.mute = initData.mute;
        gatt_vcs_server_inst->data.change_counter = initData.changeCounter;
        gatt_vcs_server_inst->data.volume_flag = GATT_VCS_SERVER_VOLUME_SETTING_NOT_PERSISTED;
        gatt_vcs_server_inst->data.step_size = initData.stepSize;

        memset(gatt_vcs_server_inst->data.connected_clients, 0, (sizeof(gatt_vcs_client_data) * GATT_VCS_MAX_CONNECTIONS));

        /* Setup data required for Volume Control Service to be registered with the GATT Manager */
        reg_params.start_handle = startHandle;
        reg_params.end_handle = endHandle;
        reg_params.task = &gatt_vcs_server_inst->lib_task;

        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterServer(&reg_params) == gatt_manager_status_success)
        {
            return srvc_hndl;
        }
        else
        {
            GATT_VCS_SERVER_DEBUG_PANIC(("Register with the GATT Manager failed!\n"));
            /* If the registration with GATT Manager fails and we have allocated memory
             * for the new instance successfully (service handle not zero), we have to free
             * the memnory of that instance.
             */
            if (srvc_hndl)
            {
                ServiceHandleFreeInstanceData(srvc_hndl);
            }
            return 0;
        }
    }
    else
    {
        GATT_VCS_SERVER_DEBUG_PANIC(("Memory alllocation of VCS Server instance failed!\n"));
        return 0;
    }
}

/******************************************************************************/
gatt_status_t GattVcsServerAddConfig(ServiceHandle srvcHndl,
                                     connection_id_t cid,
                                     const GattVcsServerConfig* config)
{
    uint8 i;
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);

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
                    GATT_VCS_SERVER_DEBUG_INFO(("Invalid Client Configuration Characteristic!\n"));
                    return gatt_status_value_not_allowed;
                }

                /* Save new ccc for the client */
                volume_control_server->data.connected_clients[i].client_cfg.volumeStateClientCfg = config->volumeStateClientCfg;
                volume_control_server->data.connected_clients[i].client_cfg.volumeFlagClientCfg = config->volumeFlagClientCfg;

                if (config->volumeStateClientCfg == GATT_VCS_SERVER_CCC_NOTIFY)
                {
                    /* If its ccc is NOTIFY, we have to notify the Volume State characteristic value */
                    uint8 value[GATT_VCS_SERVER_VOLUME_STATE_SIZE];

                    vcsServerComposeVolumeStateValue(value, volume_control_server);

                    vcsServerSendCharacteristicChangedNotification(
                                (Task) &(volume_control_server->lib_task),
                                cid,
                                HANDLE_VOLUME_STATE,
                                GATT_VCS_SERVER_VOLUME_STATE_SIZE,
                                value);
                }

                if (config->volumeFlagClientCfg == GATT_VCS_SERVER_CCC_NOTIFY)
                {
                    /* If its ccc is NOTIFY, we have to notify the Volume Flag characteristic value */
                    vcsServerSendCharacteristicChangedNotification(
                                (Task) &(volume_control_server->lib_task),
                                cid,
                                HANDLE_VOLUME_FLAGS,
                                GATT_VCS_SERVER_VOLUME_FLAG_SIZE,
                                &(volume_control_server->data.volume_flag));
                }
            }

            volume_control_server->data.connected_clients[i].cid = cid;

            return gatt_status_success;
        }
    }

    return gatt_status_insufficient_resources;
}

/******************************************************************************/
GattVcsServerConfig *GattVcsServerRemoveConfig(ServiceHandle srvcHndl,
                                               connection_id_t cid)
{
    uint8 i;
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);
    GattVcsServerConfig *config = NULL;

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

            config = PanicUnlessMalloc(sizeof(GattVcsServerConfig));
            memcpy(config, &(volume_control_server->data.connected_clients[i].client_cfg), sizeof(GattVcsServerConfig));

            if ((i == (GATT_VCS_MAX_CONNECTIONS-1)) || (i == 0 && volume_control_server->data.connected_clients[i+1].cid == 0))
            {
                /* The peer device is the only or the last element of the array */
                memset(&(volume_control_server->data.connected_clients[i]), 0, sizeof(gatt_vcs_client_data));
            }
            else
            {
                /* The peer device is in the middle of the array */
                uint8 j;

                for (j=i; j<(GATT_VCS_MAX_CONNECTIONS - 1) && volume_control_server->data.connected_clients[j+1].cid != 0; j++)
                {
                    /* Shift all the elements of the array of one position behind */
                    memmove(&(volume_control_server->data.connected_clients[j]),
                           &(volume_control_server->data.connected_clients[j+1]),
                           sizeof(gatt_vcs_client_data));
                }

                /* Remove the last element of the array, already shifted behind */
                memset(&(volume_control_server->data.connected_clients[j]), 0, sizeof(gatt_vcs_client_data));
            }
        }
    }

    return config;
}

/******************************************************************************/
bool GattVcsServerSetVolume(ServiceHandle srvcHndl, uint8 volume)
{
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);

    if (volume != volume_control_server->data.volume_setting)
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

    if ((mute == GATT_VCS_SERVER_MUTE_VALUE || mute == GATT_VCS_SERVER_UNMUTE_VALUE) &&
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
    return volume_control_server->data.volume_setting;
}

/******************************************************************************/
uint8 GattVcsServerGetMute(ServiceHandle srvcHndl)
{
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);
    return volume_control_server->data.mute;
}

/******************************************************************************/
uint8 GattVcsServerGetVolumeFlag(ServiceHandle srvcHndl)
{
    GVCS *volume_control_server = (GVCS *) ServiceHandleGetInstanceData(srvcHndl);
    return volume_control_server->data.volume_flag;
}
