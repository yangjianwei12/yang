/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_bass_client_discovery.h"
#include "gatt_bass_client_debug.h"
#include "gatt_bass_client.h"
#include "gatt_bass_client_uuid.h"
#include "gatt_bass_client_init.h"
#include "gatt_bass_client_common.h"
#include "gatt_bass_client_private.h"

#include "csr_bt_gatt_lib.h"
#include "csr_bt_uuids.h"
/****************************************************************************/
void bassClientHandleDiscoverAllCharacteristicsResp(GBASSC *gatt_bass_client,
                                                    const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_BASS_CLIENT_INFO("GBASSC: DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                           cfm->status,
                           cfm->handle,
                           cfm->uuid[0],
                           cfm->more_to_come);
                            
    if (cfm->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_BROADCAST_AUDIO_SCAN_CONTROL_POINT &&
                cfm->properties == (uint8) (ATT_PERM_WRITE_REQ | ATT_PERM_WRITE_CMD))
            {
                /* Broadcast Audio Scan Control Point UUID found so store its handle */
                gatt_bass_client->client_data.broadcast_audio_scan_control_point_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_BROADCAST_RECEIVE_STATE &&
                     cfm->properties == (uint8) (ATT_PERM_READ | ATT_PERM_NOTIFY))
            {
                /* Broadcast Receive State UUID found so store its handle */
                bassClientAddHandle(gatt_bass_client, 0, cfm->handle, 0);
                gatt_bass_client->client_data.broadcast_source_num += 1;
            }
        }

        if (!cfm->more_to_come)
        {
            if (!gatt_bass_client->client_data.broadcast_audio_scan_control_point_handle ||
                !gatt_bass_client->client_data.broadcast_source_num)
            {
                /* One of the BASS characteristic is not found, initialization complete */
                gattBassClientSendInitCfm(gatt_bass_client, GATT_BASS_CLIENT_STATUS_DISCOVERY_ERR);
                if(!ServiceHandleFreeInstanceData(gatt_bass_client->srvcElem->service_handle))
                {
                    GATT_BASS_CLIENT_PANIC("Freeing of memory instance failed\n");
                }
            }
            else
            {
                /* All BASS characteristics found, find the descriptors */
                bassClientdiscoverAllBassCharacteristicDescriptors(gatt_bass_client);
            }
        }
    }
    else
    {
        gattBassClientSendInitCfm(gatt_bass_client, GATT_BASS_CLIENT_STATUS_DISCOVERY_ERR);
        if(!ServiceHandleFreeInstanceData(gatt_bass_client->srvcElem->service_handle))
        {
            GATT_BASS_CLIENT_PANIC("Freeing of memory instance failed\n");
        }
    }
}

/****************************************************************************/
void bassClientdiscoverAllBassCharacteristicDescriptors(GBASSC *gatt_bass_client)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(gatt_bass_client->srvcElem->gattId,
                                                 gatt_bass_client->srvcElem->cid,
                                                 gatt_bass_client->client_data.start_handle + 1,
                                                 gatt_bass_client->client_data.end_handle);
}

/****************************************************************************/
static void bassServerSetHandleCcc(GBASSC *gatt_bass_client,
                                   uint16 handle,
                                   uint16 handle_ccc)
{
    bass_client_handle_t *list_item = gatt_bass_client->client_data.broadcast_receive_state_handles_first;

    while(list_item)
    {
        if (list_item->handle == handle)
        {
            list_item->handle_ccc = handle_ccc;
            return;
        }

        list_item = list_item->next;
    }
}

/****************************************************************************/
static bool bassServerCheckHandleCcc(GBASSC *gatt_bass_client)
{
    bass_client_handle_t *list_item = gatt_bass_client->client_data.broadcast_receive_state_handles_first;

    while(list_item)
    {
        if (!list_item->handle_ccc)
        {
            return FALSE;
        }

        list_item = list_item->next;
    }

    return TRUE;
}

/****************************************************************************/
void bassClientHandleDiscoverAllCharacteristicDescriptorsResp(GBASSC *gatt_bass_client,
                                                              const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_BASS_CLIENT_DEBUG("GBASSC: DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                           cfm->status, cfm->handle, cfm->uuid[0], cfm->more_to_come);
    
    if (cfm->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_BROADCAST_RECEIVE_STATE)
            {
                gatt_bass_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == CSR_BT_GATT_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC)
            {
                if (gatt_bass_client->pending_handle)
                {
                    bassServerSetHandleCcc(gatt_bass_client,
                                           gatt_bass_client->pending_handle,
                                           cfm->handle);
                    gatt_bass_client->pending_handle = 0;
                }
            }
        }
    }

    if (!cfm->more_to_come)
    {
        if (!bassServerCheckHandleCcc(gatt_bass_client))
        {
            gattBassClientSendInitCfm(gatt_bass_client, GATT_BASS_CLIENT_STATUS_DISCOVERY_ERR);
            if(!ServiceHandleFreeInstanceData(gatt_bass_client->srvcElem->service_handle))
            {
                GATT_BASS_CLIENT_PANIC("Freeing of memory instance failed\n");
            }
        }
        else
        {
            bassClientSendInternalMsgInitRead(gatt_bass_client);
        }
    }
}

