/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_vcs_client_discovery.h"
#include "gatt_vcs_client_debug.h"
#include "gatt_vcs_client.h"
#include "gatt_vcs_client_uuid.h"
#include "gatt_vcs_client_init.h"
#include "gatt_vcs_client_common.h"

#include <gatt.h>
#include <gatt_manager.h>

/****************************************************************************/
void handleDiscoverAllVcsCharacteristicsResp(GVCSC *gatt_vcs_client,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_VCS_CLIENT_DEBUG_INFO(("GVCSC: DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                            cfm->status,
                            cfm->handle,
                            cfm->uuid[0],
                            cfm->more_to_come));
                            
    if (cfm->status == gatt_status_success)
    {
        if (cfm->uuid_type == gatt_uuid16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_STATE)
            {
                /* Volume State UUID found so store its handle */
                gatt_vcs_client->handles.volumeStateHandle = cfm->handle;
                GATT_VCS_CLIENT_DEBUG_INFO(("GVCSC: Volume State cid=[0x%x] handle=[0x%x]\n",
                                            cfm->cid, cfm->handle));
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_CONTROL_POINT)
            {
                /* Volume Control Point UUID found so store its handle */
                gatt_vcs_client->handles.volumeControlPointHandle = cfm->handle;
                GATT_VCS_CLIENT_DEBUG_INFO(("GVCSC: Volume Control Point cid=[0x%x] handle=[0x%x]\n",
                                            cfm->cid, cfm->handle));
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_FLAGS)
            {
                /* Volume Flag UUID found so store its handle */
                gatt_vcs_client->handles.volumeFlagsHandle = cfm->handle;
                GATT_VCS_CLIENT_DEBUG_INFO(("GVCSC: Volume Flag cid=[0x%x] handle=[0x%x]\n",
                                            cfm->cid, cfm->handle));
            }
        }

        if (!cfm->more_to_come)
        {
            if (!gatt_vcs_client->handles.volumeControlPointHandle ||
                !gatt_vcs_client->handles.volumeFlagsHandle ||
                !gatt_vcs_client->handles.volumeStateHandle)
            {
                /* One of the VCS characteristic is not found, initialisation complete */
                gattVcsClientSendInitCfm(gatt_vcs_client, GATT_VCS_CLIENT_STATUS_DISCOVERY_ERR);

                if(!ServiceHandleFreeInstanceData(gatt_vcs_client->srvc_hndl))
                {
                    GATT_VCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
                }
            }
            else
            {
                /* All VCS characteristics found, find the descriptors */
                discoverAllVcsCharacteristicDescriptors(gatt_vcs_client);
            }
        }
    }
    else
    {
        gattVcsClientSendInitCfm(gatt_vcs_client, GATT_VCS_CLIENT_STATUS_DISCOVERY_ERR);

        if(!ServiceHandleFreeInstanceData(gatt_vcs_client->srvc_hndl))
        {
            GATT_VCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }
}

/****************************************************************************/
void discoverAllVcsCharacteristicDescriptors(GVCSC *gatt_vcs_client)
{
    gatt_manager_client_service_data_t service_data;
    
    if (GattManagerGetClientData(&gatt_vcs_client->lib_task, &service_data))
    {
        GattManagerDiscoverAllCharacteristicDescriptors(&gatt_vcs_client->lib_task,
                                                        service_data.start_handle,
                                                        service_data.end_handle);
    }
    else
    {
        GATT_VCS_CLIENT_DEBUG_PANIC(("GVCSC: Internal error\n"));
        gattVcsClientSendInitCfm(gatt_vcs_client, GATT_VCS_CLIENT_STATUS_DISCOVERY_ERR);

        if(!ServiceHandleFreeInstanceData(gatt_vcs_client->srvc_hndl))
        {
            GATT_VCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }
}


/****************************************************************************/
void handleDiscoverAllVcsCharacteristicDescriptorsResp(GVCSC *gatt_vcs_client,
                                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_VCS_CLIENT_DEBUG_INFO(("GVCSC: DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                            cfm->status,
                            cfm->handle,
                            cfm->uuid[0],
                            cfm->more_to_come));
    
    if (cfm->status == gatt_status_success)
    {
        if (cfm->uuid_type == gatt_uuid16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_STATE)
            {
                gatt_vcs_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_FLAGS)
            {
                gatt_vcs_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID)
            {
                if (gatt_vcs_client->pending_handle == gatt_vcs_client->handles.volumeStateHandle)
                {
                    gatt_vcs_client->handles.volumeStateCccHandle = cfm->handle;
                    gatt_vcs_client->pending_handle = 0;
                }
                else if (gatt_vcs_client->pending_handle == gatt_vcs_client->handles.volumeFlagsHandle)
                {
                    gatt_vcs_client->handles.volumeFlagsCccHandle = cfm->handle;
                    gatt_vcs_client->pending_handle = 0;
                }
            }
        }
    }
    else
    {
        gattVcsClientSendInitCfm(gatt_vcs_client, GATT_VCS_CLIENT_STATUS_DISCOVERY_ERR);

        if(!ServiceHandleFreeInstanceData(gatt_vcs_client->srvc_hndl))
        {
            GATT_VCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }

    if (!cfm->more_to_come)
    {
        if (!gatt_vcs_client->handles.volumeStateCccHandle)
        {
            gattVcsClientSendInitCfm(gatt_vcs_client, GATT_VCS_CLIENT_STATUS_DISCOVERY_ERR);

            if(!ServiceHandleFreeInstanceData(gatt_vcs_client->srvc_hndl))
            {
                GATT_VCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
            }
        }
        else
        {
            gattVcsClientSendInitCfm(gatt_vcs_client, GATT_VCS_CLIENT_STATUS_SUCCESS);
        }
    }
}
