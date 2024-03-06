/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_aics_client_discovery.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client_uuids.h"
#include "gatt_aics_client_init.h"
#include "gatt_aics_client_common.h"

#include <gatt.h>
#include <gatt_manager.h>

/****************************************************************************/
void aicsClientHandleDiscoverAllAicsCharacteristicsResp(GAICS *gatt_aics_client,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_AICS_CLIENT_DEBUG_INFO(("DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                                  cfm->status,
                                  cfm->handle,
                                  cfm->uuid[0],
                                  cfm->more_to_come));

    if (cfm->status == gatt_status_success)
    {
        if (cfm->uuid_type == gatt_uuid16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_INPUT_STATE)
            {
                /* Input State UUID found so store its handle */
                gatt_aics_client->handles.inputStateHandle = cfm->handle;
                GATT_AICS_CLIENT_DEBUG_INFO(("Input State cid=[0x%x] handle=[0x%x]\n",
                                              cfm->cid, cfm->handle));
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_GAIN_SETTING_PROPERTIES)
            {
                /* Gain Setting Properties UUID found so store its handle */
                gatt_aics_client->handles.gainSettingPropertiesHandle = cfm->handle;
                GATT_AICS_CLIENT_DEBUG_INFO(("Gain Setting Properties cid=[0x%x] handle=[0x%x]\n",
                                              cfm->cid, cfm->handle));
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_INPUT_TYPE)
            {
                /* Input Type UUID found so store its handle */
                gatt_aics_client->handles.inputTypeHandle = cfm->handle;
                GATT_AICS_CLIENT_DEBUG_INFO(("Input Type cid=[0x%x] handle=[0x%x]\n",
                                              cfm->cid, cfm->handle));
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_INPUT_STATUS)
            {
                /* Input Status UUID found so store its handle */
                gatt_aics_client->handles.inputStatusHandle = cfm->handle;
                GATT_AICS_CLIENT_DEBUG_INFO(("Input Status cid=[0x%x] handle=[0x%x]\n",
                                              cfm->cid, cfm->handle));
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_INPUT_CONTROL_POINT)
            {
                /* Audio Input Control Point UUID found so store its handle */
                gatt_aics_client->handles.audioInputControlPointHandle = cfm->handle;
                GATT_AICS_CLIENT_DEBUG_INFO(("Audio Input Control Point cid=[0x%x] handle=[0x%x]\n",
                                              cfm->cid, cfm->handle));
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_INPUT_DESCRIPTION)
            {
                /* Audio Input Description UUID found so store its handle */
                gatt_aics_client->handles.audioInputDescriptionHandle = cfm->handle;
                GATT_AICS_CLIENT_DEBUG_INFO(("Audio Input Control Point cid=[0x%x] handle=[0x%x]\n",
                                              cfm->cid, cfm->handle));

                gatt_aics_client->handles.audioInputDescProperties = cfm->properties;
            }
        }

        if (!cfm->more_to_come)
        {
            if (!gatt_aics_client->handles.audioInputControlPointHandle ||
                !gatt_aics_client->handles.audioInputDescriptionHandle  ||
                !gatt_aics_client->handles.gainSettingPropertiesHandle  ||
                !gatt_aics_client->handles.inputStateHandle             ||
                !gatt_aics_client->handles.inputStatusHandle            ||
                !gatt_aics_client->handles.inputTypeHandle)
            {
                /* One of the AICS characteristic is not found, initialisation complete */
                gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_DISCOVERY_ERR);

                if(!ServiceHandleFreeInstanceData(gatt_aics_client->srvc_hndl))
                {
                    GATT_AICS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
                }
            }
            else
            {
                /* All AICS characteristics found, find the descriptors */
                aicsClientDiscoverAllCharacteristicDescriptors(gatt_aics_client);
            }
        }
    }
    else
    {
        gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_DISCOVERY_ERR);

        if(!ServiceHandleFreeInstanceData(gatt_aics_client->srvc_hndl))
        {
            GATT_AICS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }
}

/****************************************************************************/
void aicsClientDiscoverAllCharacteristicDescriptors(GAICS *gatt_aics_client)
{
    gatt_manager_client_service_data_t service_data;

    if (GattManagerGetClientData(&gatt_aics_client->lib_task, &service_data))
    {
        GattManagerDiscoverAllCharacteristicDescriptors(&gatt_aics_client->lib_task,
                                                        service_data.start_handle,
                                                        service_data.end_handle);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Internal error\n"));
        gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_DISCOVERY_ERR);

        if(!ServiceHandleFreeInstanceData(gatt_aics_client->srvc_hndl))
        {
            GATT_AICS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }
}

/****************************************************************************/
void aicsClientHandleDiscoverAllCharacteristicDescriptorsResp(GAICS *gatt_aics_client,
                                     const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_AICS_CLIENT_DEBUG_INFO(("DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                                  cfm->status,
                                  cfm->handle,
                                  cfm->uuid[0],
                                  cfm->more_to_come));
    
    if (cfm->status == gatt_status_success)
    {
        if (cfm->uuid_type == gatt_uuid16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_INPUT_STATE)
            {
                gatt_aics_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_INPUT_STATUS)
            {
                gatt_aics_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_INPUT_DESCRIPTION)
            {
                gatt_aics_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID)
            {
                if (gatt_aics_client->pending_handle == gatt_aics_client->handles.inputStateHandle)
                {
                    gatt_aics_client->handles.inputStateCccHandle = cfm->handle;
                    gatt_aics_client->pending_handle = 0;
                }
                else if (gatt_aics_client->pending_handle == gatt_aics_client->handles.inputStatusHandle)
                {
                    gatt_aics_client->handles.inputStatusCccHandle = cfm->handle;
                    gatt_aics_client->pending_handle = 0;
                }
                else if (gatt_aics_client->pending_handle == gatt_aics_client->handles.audioInputDescriptionHandle)
                {
                    gatt_aics_client->handles.audioInputDescriptionCccHandle = cfm->handle;
                    gatt_aics_client->pending_handle = 0;
                }
            }
        }
    }
    else
    {
        gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_DISCOVERY_ERR);

        if(!ServiceHandleFreeInstanceData(gatt_aics_client->srvc_hndl))
        {
            GATT_AICS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }

    if (!cfm->more_to_come)
    {
        if (!gatt_aics_client->handles.inputStateCccHandle         ||
            !gatt_aics_client->handles.audioInputDescriptionCccHandle ||
            !gatt_aics_client->handles.inputStatusCccHandle)
        {
            gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_DISCOVERY_ERR);

            if(!ServiceHandleFreeInstanceData(gatt_aics_client->srvc_hndl))
            {
                GATT_AICS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
            }
        }
        else
        {
            gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_SUCCESS);
        }
    }
}
