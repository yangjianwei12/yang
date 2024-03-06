/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_aics_client_discovery.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client_uuids.h"
#include "gatt_aics_client_init.h"
#include "gatt_aics_client_common.h"
#include "csr_bt_gatt_client_util_lib.h"

#include "csr_bt_gatt_lib.h"

/****************************************************************************/
void aicsClientHandleDiscoverAllAicsCharacteristicsResp(GAICS *gatt_aics_client,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_AICS_CLIENT_INFO("DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                           cfm->status,
                           cfm->handle,
                           cfm->uuid[0],
                           cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_INPUT_STATE)
            {
                /* Input State UUID found so store its handle */
                gatt_aics_client->handles.inputStateHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_GAIN_SETTING_PROPERTIES)
            {
                /* Gain Setting Properties UUID found so store its handle */
                gatt_aics_client->handles.gainSettingPropertiesHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_INPUT_TYPE)
            {
                /* Input Type UUID found so store its handle */
                gatt_aics_client->handles.inputTypeHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_INPUT_STATUS)
            {
                /* Input Status UUID found so store its handle */
                gatt_aics_client->handles.inputStatusHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_INPUT_CONTROL_POINT)
            {
                /* Audio Input Control Point UUID found so store its handle */
                gatt_aics_client->handles.audioInputControlPointHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_AUDIO_INPUT_DESCRIPTION)
            {
                /* Audio Input Description UUID found so store its handle */
                gatt_aics_client->handles.audioInputDescriptionHandle = cfm->handle;

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
    }
}

/****************************************************************************/
void aicsClientDiscoverAllCharacteristicDescriptors(GAICS *gatt_aics_client)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(gatt_aics_client->srvcElem->gattId,
                                                 gatt_aics_client->srvcElem->cid,
                                                 gatt_aics_client->handles.startHandle + 1,
                                                 gatt_aics_client->handles.endHandle);
}

/****************************************************************************/
void aicsClientHandleDiscoverAllCharacteristicDescriptorsResp(GAICS *gatt_aics_client,
                                     const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_AICS_CLIENT_INFO("DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                           cfm->status,
                           cfm->handle,
                           cfm->uuid[0],
                           cfm->more_to_come);
    
    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
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
            else if (cfm->uuid[0] == CSR_BT_GATT_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC)
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

    if (!cfm->more_to_come)
    {
        if (!gatt_aics_client->handles.inputStateCccHandle         ||
            !gatt_aics_client->handles.audioInputDescriptionCccHandle ||
            !gatt_aics_client->handles.inputStatusCccHandle)
        {
            gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_DISCOVERY_ERR);
        }
        else
        {
            gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_SUCCESS);
        }
    }
}
