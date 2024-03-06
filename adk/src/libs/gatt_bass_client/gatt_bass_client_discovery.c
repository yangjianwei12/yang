/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_bass_client_discovery.h"
#include "gatt_bass_client_debug.h"
#include "gatt_bass_client.h"
#include "gatt_bass_client_uuid.h"
#include "gatt_bass_client_init.h"
#include "gatt_bass_client_common.h"
#include "gatt_bass_client_private.h"

/****************************************************************************/
void bassClientHandleDiscoverAllCharacteristicsResp(GBASSC *gatt_bass_client,
                                                    const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_BASS_CLIENT_DEBUG_INFO(("GBASSC: DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                            cfm->status,
                            cfm->handle,
                            cfm->uuid[0],
                            cfm->more_to_come));
                            
    if (cfm->status == gatt_status_success)
    {
        if (cfm->uuid_type == gatt_uuid16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_BROADCAST_AUDIO_SCAN_CONTROL_POINT &&
                cfm->properties == (uint8) (gatt_char_prop_write | gatt_char_prop_write_no_response))
            {
                /* Broadcast Audio Scan Control Point UUID found so store its handle */
                gatt_bass_client->client_data.broadcast_audio_scan_control_point_handle = cfm->handle;
                GATT_BASS_CLIENT_DEBUG_INFO(("GBASSC: Broadcast Audio Scan Control Point cid=[0x%x] handle=[0x%x]\n",
                                             cfm->cid, cfm->handle));
            }
            else if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_BROADCAST_RECEIVE_STATE &&
                     cfm->properties == (uint8) (gatt_char_prop_read | gatt_char_prop_notify))
            {
                /* Broadcast Receive State UUID found so store its handle */
                GATT_BASS_CLIENT_DEBUG_INFO(("GBASSC: Broadcast Receive State cid=[0x%x] handle=[0x%x]\n",
                                             cfm->cid, cfm->handle));
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
                if(!ServiceHandleFreeInstanceData(gatt_bass_client->clnt_hndl))
                {
                    GATT_BASS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
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
        if(!ServiceHandleFreeInstanceData(gatt_bass_client->clnt_hndl))
        {
            GATT_BASS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }
}

/****************************************************************************/
void bassClientdiscoverAllBassCharacteristicDescriptors(GBASSC *gatt_bass_client)
{
    gatt_manager_client_service_data_t service_data;
    
    if (GattManagerGetClientData(&gatt_bass_client->lib_task, &service_data))
    {
        GattManagerDiscoverAllCharacteristicDescriptors(&gatt_bass_client->lib_task,
                                                        service_data.start_handle,
                                                        service_data.end_handle);
    }
    else
    {
        GATT_BASS_CLIENT_DEBUG_PANIC(("GBASSC: Internal error\n"));
        gattBassClientSendInitCfm(gatt_bass_client, GATT_BASS_CLIENT_STATUS_DISCOVERY_ERR);
        if(!ServiceHandleFreeInstanceData(gatt_bass_client->clnt_hndl))
        {
            GATT_BASS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
        }
    }
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
    GATT_BASS_CLIENT_DEBUG_INFO(("GBASSC: DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                                 cfm->status, cfm->handle, cfm->uuid[0], cfm->more_to_come));
    
    if (cfm->status == gatt_status_success)
    {
        if (cfm->uuid_type == gatt_uuid16)
        {
            if (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_BROADCAST_RECEIVE_STATE)
            {
                gatt_bass_client->pending_handle = cfm->handle;
            }
            else if (cfm->uuid[0] == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID)
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
            if(!ServiceHandleFreeInstanceData(gatt_bass_client->clnt_hndl))
            {
                GATT_BASS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
            }
        }
        else
        { 
            bassClientSendInternalMsgInitRead(gatt_bass_client);
        }
    }
}
