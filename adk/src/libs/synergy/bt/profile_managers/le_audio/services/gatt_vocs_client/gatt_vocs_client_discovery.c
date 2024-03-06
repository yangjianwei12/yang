/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_vocs_client_discovery.h"
#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client_uuids.h"
#include "gatt_vocs_client_init.h"
#include "gatt_vocs_client_common.h"

#include "csr_bt_gatt_lib.h"

/****************************************************************************/
void vocsClientHandleDiscoverAllVocsCharacteristicsResp(GVOCS *gatt_vocs_client,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_VOCS_CLIENT_INFO("DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                           cfm->status,
                           cfm->handle,
                           cfm->uuid[0],
                           cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_OFFSET_STATE)
            {
                /* Offset State UUID found so store its handle */
                gatt_vocs_client->handles.offsetStateHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_LOCATION)
            {
                /* Audio Location UUID found so store its handle */
                gatt_vocs_client->handles.audioLocationHandle = cfm->handle;
                gatt_vocs_client->handles.audioLocationProperties = cfm->properties;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_OFFSET_CONTROL_POINT)
            {
                /* Volume Offset Control Point UUID found so store its handle */
                gatt_vocs_client->handles.volumeOffsetControlPointHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_OUTPUT_DESCRIPTION)
            {
                /* Audio Output Description UUID found so store its handle */
                gatt_vocs_client->handles.audioOutputDescriptionHandle = cfm->handle;
                gatt_vocs_client->handles.audioOutputDescProperties = cfm->properties;
            }
        }

        if (!cfm->more_to_come)
        {
            if (!gatt_vocs_client->handles.volumeOffsetControlPointHandle ||
                !gatt_vocs_client->handles.audioLocationHandle            ||
                !gatt_vocs_client->handles.audioOutputDescriptionHandle   ||
                !gatt_vocs_client->handles.offsetStateHandle)
            {
                /* One of the VOCS characteristic is not found, initialisation complete */
                vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_DISCOVERY_ERR);
            }
            else
            {
                /* All VOCS characteristics found, find the descriptors */
                vocsClientdiscoverAllVocsCharacteristicDescriptors(gatt_vocs_client);
            }
        }
      }
    else
    {
        vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_DISCOVERY_ERR);
    }
}

/****************************************************************************/
void vocsClientdiscoverAllVocsCharacteristicDescriptors(GVOCS *gatt_vocs_client)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(gatt_vocs_client->srvcElem->gattId,
                                                 gatt_vocs_client->srvcElem->cid,
                                                 gatt_vocs_client->handles.startHandle + 1,
                                                 gatt_vocs_client->handles.endHandle);
}

/****************************************************************************/
void vocsClientHandleDiscoverAllVocsCharacteristicDescriptorsResp(GVOCS *gatt_vocs_client,
                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_VOCS_CLIENT_INFO("DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                           cfm->status,
                           cfm->handle,
                           cfm->uuid[0],
                           cfm->more_to_come);
    
    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
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
            else if (cfm->uuid[0] == CSR_BT_GATT_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC)
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

    if (!cfm->more_to_come)
    {
        if (!gatt_vocs_client->handles.offsetStateCccHandle             ||
            !gatt_vocs_client->handles.audioLocationCccHandle           ||
            !gatt_vocs_client->handles.audioOutputDescriptionCccHandle)
        {
            vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_DISCOVERY_ERR);
        }
        else
        {
            vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_SUCCESS);
        }
    }
}
