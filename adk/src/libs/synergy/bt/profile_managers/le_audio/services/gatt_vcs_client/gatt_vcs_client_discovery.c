/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_vcs_client_discovery.h"
#include "gatt_vcs_client_debug.h"
#include "gatt_vcs_client.h"
#include "gatt_vcs_client_uuid.h"
#include "gatt_vcs_client_init.h"
#include "gatt_vcs_client_common.h"

#include "csr_bt_gatt_lib.h"

/****************************************************************************/
void handleDiscoverAllVcsCharacteristicsResp(GVCSC *gatt_vcs_client,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_VCS_CLIENT_INFO("GVCSC: DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                          cfm->status,
                          cfm->handle,
                          cfm->uuid[0],
                          cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_STATE)
            {
                /* Volume State UUID found so store its handle */
                gatt_vcs_client->handles.volumeStateHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_CONTROL_POINT)
            {
                /* Volume Control Point UUID found so store its handle */
                gatt_vcs_client->handles.volumeControlPointHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_FLAGS)
            {
                /* Volume Flag UUID found so store its handle */
                gatt_vcs_client->handles.volumeFlagsHandle = cfm->handle;
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
    }

}

/****************************************************************************/
void discoverAllVcsCharacteristicDescriptors(GVCSC *gatt_vcs_client)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(gatt_vcs_client->srvcElem->gattId,
                                                 gatt_vcs_client->srvcElem->cid,
                                                 gatt_vcs_client->handles.startHandle + 1,
                                                 gatt_vcs_client->handles.endHandle);
}

/****************************************************************************/
void handleDiscoverAllVcsCharacteristicDescriptorsResp(GVCSC *gatt_vcs_client,
                                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_VCS_CLIENT_INFO("GVCSC: DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                          cfm->status,
                          cfm->handle,
                          cfm->uuid[0],
                          cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_STATE)
            {
                gatt_vcs_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_VOLUME_FLAGS)
            {
                gatt_vcs_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == CSR_BT_GATT_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC)
            {
                if (gatt_vcs_client->pending_handle == gatt_vcs_client->handles.volumeStateHandle)
                {
                    gatt_vcs_client->handles.volumeStateCccHandle = cfm->handle;
                    gatt_vcs_client->pending_handle = 0;
                }
                else if (gatt_vcs_client->pending_handle == gatt_vcs_client->handles.volumeFlagsHandle)
                {
                    gatt_vcs_client->handles.volumeFlagsCccHandle =cfm->handle;
                    gatt_vcs_client->pending_handle = 0;
                }
            }
        }
    }

    if (!cfm->more_to_come)
    {
        if (!gatt_vcs_client->handles.volumeStateCccHandle)
        {
            gattVcsClientSendInitCfm(gatt_vcs_client, GATT_VCS_CLIENT_STATUS_DISCOVERY_ERR);
        }
        else
        {
            gattVcsClientSendInitCfm(gatt_vcs_client, GATT_VCS_CLIENT_STATUS_SUCCESS);
        }
    }
}

