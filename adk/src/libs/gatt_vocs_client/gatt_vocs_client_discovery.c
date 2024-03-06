/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_vocs_client_discovery.h"
#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client_uuids.h"
#include "gatt_vocs_client_init.h"
#include "gatt_vocs_client_common.h"

#include <gatt.h>
#include <gatt_manager.h>

/****************************************************************************/
void vocsClientHandleDiscoverAllVocsCharacteristicsResp(GVOCS *gatt_vocs_client,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_VOCS_CLIENT_DEBUG_INFO(("DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                                cfm->status,
                                cfm->handle,
                                cfm->uuid[0],
                                cfm->more_to_come));

    if (cfm->status == gatt_status_success)
    {
        if (cfm->uuid_type == gatt_uuid16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_OFFSET_STATE)
            {
                GATT_VOCS_CLIENT_DEBUG_INFO(("Offset State cid=[0x%x] handle=[0x%x]\n",
                                              cfm->cid, cfm->handle));

                /* Offset State UUID found so store its handle */
                gatt_vocs_client->handles.offsetStateHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_LOCATION)
            {
                /* Audio Location UUID found so store its handle */
                gatt_vocs_client->handles.audioLocationHandle = cfm->handle;
                GATT_VOCS_CLIENT_DEBUG_INFO(("Audio Location cid=[0x%x] handle=[0x%x]\n",
                                              cfm->cid, cfm->handle));

                gatt_vocs_client->handles.audioLocationProperties = cfm->properties;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_OFFSET_CONTROL_POINT)
            {
                /* Volume Offset Control Point UUID found so store its handle */
                gatt_vocs_client->handles.volumeOffsetControlPointHandle = cfm->handle;
                GATT_VOCS_CLIENT_DEBUG_INFO(("Volume Offset Control Point cid=[0x%x] handle=[0x%x]\n",
                                              cfm->cid, cfm->handle));
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_OUTPUT_DESCRIPTION)
            {
                GATT_VOCS_CLIENT_DEBUG_INFO(("Audio Output Description cid=[0x%x] handle=[0x%x]\n",
                                              cfm->cid, cfm->handle));

                /* Audio Output Description UUID found so store its handle */
                gatt_vocs_client->handles.audioOutputDescriptionHandle = cfm->handle;
                gatt_vocs_client->handles.audioOutputDescProperties = cfm->properties;
            }
        }

        if (!cfm->more_to_come)
        {
            if (!gatt_vocs_client->handles.volumeOffsetControlPointHandle ||
                !gatt_vocs_client->handles.audioLocationHandle              ||
                !gatt_vocs_client->handles.audioOutputDescriptionHandle    ||
                !gatt_vocs_client->handles.offsetStateHandle)
            {
                /* One of the VOCS characteristic is not found, initialisation complete */
                vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_DISCOVERY_ERR);

                if(!ServiceHandleFreeInstanceData(gatt_vocs_client->srvc_hndl))
                {
                    GATT_VOCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
                }
            }
            else
            {
                /* All VCS characteristics found, find the descriptors */
                vocsClientdiscoverAllVocsCharacteristicDescriptors(gatt_vocs_client);
            }
        }
    }
    else
    {
        vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_DISCOVERY_ERR);

        if(!ServiceHandleFreeInstanceData(gatt_vocs_client->srvc_hndl))
        {
            GATT_VOCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }
}

/****************************************************************************/
void vocsClientdiscoverAllVocsCharacteristicDescriptors(GVOCS *gatt_vocs_client)
{
    gatt_manager_client_service_data_t service_data;
    
    if (GattManagerGetClientData(&gatt_vocs_client->lib_task, &service_data))
    {
        GattManagerDiscoverAllCharacteristicDescriptors(&gatt_vocs_client->lib_task,
                                                        service_data.start_handle,
                                                        service_data.end_handle);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Internal error\n"));
        vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_DISCOVERY_ERR);

        if(!ServiceHandleFreeInstanceData(gatt_vocs_client->srvc_hndl))
        {
            GATT_VOCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }
}

/****************************************************************************/
void vocsClientHandleDiscoverAllVocsCharacteristicDescriptorsResp(GVOCS *gatt_vocs_client,
                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_VOCS_CLIENT_DEBUG_INFO(("DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                            cfm->status,
                            cfm->handle,
                            cfm->uuid[0],
                            cfm->more_to_come));
    
    if (cfm->status == gatt_status_success)
    {
        if (cfm->uuid_type == gatt_uuid16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_OFFSET_STATE)
            {
                gatt_vocs_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_LOCATION)
            {
                gatt_vocs_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_OUTPUT_DESCRIPTION)
            {
                gatt_vocs_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID)
            {
                if (gatt_vocs_client->pending_handle == gatt_vocs_client->handles.offsetStateHandle)
                {
                    gatt_vocs_client->handles.offsetStateCccHandle = cfm->handle;
                    gatt_vocs_client->pending_handle = 0;
                }
                else if (gatt_vocs_client->pending_handle == gatt_vocs_client->handles.audioLocationHandle)
                {
                    gatt_vocs_client->handles.audioLocationCccHandle = cfm->handle;
                    gatt_vocs_client->pending_handle = 0;
                }
                else if (gatt_vocs_client->pending_handle == gatt_vocs_client->handles.audioOutputDescriptionHandle)
                {
                    gatt_vocs_client->handles.audioOutputDescriptionCccHandle = cfm->handle;
                    gatt_vocs_client->pending_handle = 0;
                }
            }
        }
    }
    else
    {
        vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_FAILED);

        if(!ServiceHandleFreeInstanceData(gatt_vocs_client->srvc_hndl))
        {
            GATT_VOCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }

    if (!cfm->more_to_come)
    {
        if (!gatt_vocs_client->handles.offsetStateCccHandle             ||
            !gatt_vocs_client->handles.audioLocationCccHandle           ||
            !gatt_vocs_client->handles.audioOutputDescriptionCccHandle)
        {
            vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_DISCOVERY_ERR);

            if(!ServiceHandleFreeInstanceData(gatt_vocs_client->srvc_hndl))
            {
                GATT_VOCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
            }
        }
        else
        {
            vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_SUCCESS);
        }
    }
}
