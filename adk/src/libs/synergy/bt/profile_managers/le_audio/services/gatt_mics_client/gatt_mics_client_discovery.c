/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#include "gatt_mics_client_discovery.h"
#include "gatt_mics_client_debug.h"
#include "gatt_mics_client.h"
#include "gatt_mics_client_uuid.h"
#include "gatt_mics_client_init.h"
#include "gatt_mics_client_common.h"

#include "csr_bt_gatt_lib.h"

/****************************************************************************/
void handleDiscoverAllMicsCharacteristicsResp(GMICSC *gatt_mics_client,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_MICS_CLIENT_INFO("GMICSC: DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                          cfm->status,
                          cfm->handle,
                          cfm->uuid[0],
                          cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_MUTE)
            {
                /* Mute UUID found so store its handle */
                gatt_mics_client->handles.muteHandle = cfm->handle;
            }
        }

        if (!cfm->more_to_come)
        {
            if (!gatt_mics_client->handles.muteHandle)
            {
                /* Mandatory MICS characteristic is not found, initialisation complete */
                gattMicsClientSendInitCfm(gatt_mics_client, GATT_MICS_CLIENT_STATUS_DISCOVERY_ERR);
            }
            else
            {
                discoverAllMicsCharacteristicDescriptors(gatt_mics_client);
            }
        }
    }
    else
    {
        gattMicsClientSendInitCfm(gatt_mics_client, GATT_MICS_CLIENT_STATUS_DISCOVERY_ERR);
    }

}

/****************************************************************************/
void discoverAllMicsCharacteristicDescriptors(GMICSC *gatt_mics_client)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(gatt_mics_client->srvcElem->gattId,
                                                 gatt_mics_client->srvcElem->cid,
                                                 gatt_mics_client->handles.startHandle + 1,
                                                 gatt_mics_client->handles.endHandle);
}

/****************************************************************************/
void handleDiscoverAllMicsCharacteristicDescriptorsResp(GMICSC *gatt_mics_client,
                                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_MICS_CLIENT_INFO("GMICSC: DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                          cfm->status,
                          cfm->handle,
                          cfm->uuid[0],
                          cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_MUTE)
            {
                gatt_mics_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == CSR_BT_GATT_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC)
            {
                if (gatt_mics_client->pending_handle == gatt_mics_client->handles.muteHandle)
                {
                    gatt_mics_client->handles.muteCccHandle = cfm->handle;
                    gatt_mics_client->pending_handle = 0;
                }
            }
        }
    }

    if (!cfm->more_to_come)
    {
        if (!gatt_mics_client->handles.muteCccHandle)
        {
            gattMicsClientSendInitCfm(gatt_mics_client, GATT_MICS_CLIENT_STATUS_DISCOVERY_ERR);
        }
        else
        {
            gattMicsClientSendInitCfm(gatt_mics_client, GATT_MICS_CLIENT_STATUS_SUCCESS);
        }
    }
}

