/*!
    \copyright  Copyright (c) 2021-  2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    handset_service
    \brief      Handset service BLE state machine
*/

#include <bdaddr.h>
#include <device.h>
#include <device_properties.h>
#include <pairing.h>


#include "handset_service_ble_sm.h"
#include "handset_service_sm.h"
#include "handset_service_protected.h"
#include "device_db_serialiser.h"

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
#include "handset_service_advertising.h"
#else
#include "handset_service_legacy_advertising.h"
#include "handset_service_extended_advertising.h"
#endif

#include "context_framework.h"
static bool handsetService_BredrSmInactive(handset_service_ble_state_machine_t* ble_sm)
{
    handset_service_state_machine_t *sm = HandsetService_GetSmFromBleSm(ble_sm);
    return (sm == NULL) || ((sm->state <= HANDSET_SERVICE_STATE_DISCONNECTED) && (TaskList_Size(&sm->connect_list) == 0) && (sm->connect_stop_task == NULL));
}

static void handsetService_BleSmEnterDisconnecting(handset_service_ble_state_machine_t* ble_sm)
{
    handset_service_data_t *hs = HandsetService_Get();

    /* Remove LE ACL */
    if (!BdaddrTpIsEmpty(&ble_sm->le_addr))
    {
        ConManagerReleaseTpAclWithReasonCode(&ble_sm->le_addr, hs->disconnect_reason_code);
    }
    else
    {
        /* We did not have anything to disconnect. 
           May be a disconnect message in flight, or a bug.
           Change state. uses recursion, but one level only */
        HandsetService_BleSmSetState(ble_sm, HANDSET_SERVICE_STATE_BLE_DISCONNECTED);
    }
}

static void handsetService_BleSmDeleteDeviceIfNotPaired(handset_service_ble_state_machine_t *sm)
{
    uint16 flags = DEVICE_FLAGS_NO_FLAGS;

    appDeviceGetFlagsForDevice(BtDevice_GetDeviceFromTpAddr(&sm->le_addr), &flags);

    if (flags & DEVICE_FLAGS_NOT_PAIRED)
    {
        HS_LOG("handsetService_BleSmDeleteDeviceIfNotPaired delete device");
        /* Delete the device as not paired */
        appDeviceDeleteWithTpAddr(&sm->le_addr);
    }
}

static void handsetService_BleSmEnterDisconnected(handset_service_ble_state_machine_t* ble_sm)
{
    handset_service_state_machine_t *sm = HandsetService_GetSmFromBleSm(ble_sm);
    bool all_sm_transports_disconnected = HandsetServiceSm_AllConnectionsDisconnected(sm, BREDR_AND_BLE);
    bool bredr_transport_inactive = handsetService_BredrSmInactive(ble_sm);

    if (all_sm_transports_disconnected)
    {
        HandsetServiceSm_CompleteDisconnectRequests(sm, handset_service_status_success);
    }

    handsetService_BleSmDeleteDeviceIfNotPaired(ble_sm);

    if (HandsetServiceSm_GetHandsetDeviceIfValid(sm))
    {
        DeviceDbSerialiser_SerialiseDevice(sm->handset_device);
    }

#ifdef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    HandsetService_UpdateLegacyAdvertisingData();
    UNUSED(HandsetServiceExtAdv_UpdateAdvertisingData());
#endif
    
    if (all_sm_transports_disconnected)
    {
        if (bredr_transport_inactive)
        {
            if (sm->handset_device != NULL)
            {
                HandsetService_SendAllTransportsDisconnectedIndNotification(&ble_sm->le_addr);
            }

            HS_LOG("handsetService_BleSmEnterDisconnected destroying sm for dev 0x%x", sm->handset_device);
            HandsetServiceSm_DeInit(sm);
        }
        else
        {
            HS_LOG("handsetService_BleSmEnterDisconnected destroying only BLE sm for dev 0x%x", sm->handset_device);
            HandsetService_BleSmReset(&sm->ble_sm);
        }
    }
    else
    {
        BdaddrTpSetEmpty(&ble_sm->le_addr);
    }

    appLinkPolicyUpdatePowerTable(&ble_sm->le_addr.taddr.addr);
}

void HandsetService_BleSmReset(handset_service_ble_state_machine_t* ble_sm)
{
    if (!BdaddrIsZero(&ble_sm->le_addr.taddr.addr))
    {
        /* Delete the unpaired device matching with the address if it exists*/
        handsetService_BleSmDeleteDeviceIfNotPaired(ble_sm);
    }
    BdaddrTpSetEmpty(&ble_sm->le_addr);
    ble_sm->le_state = HANDSET_SERVICE_STATE_BLE_DISCONNECTED;
}

void HandsetService_BleSmSetState(handset_service_ble_state_machine_t* ble_sm, handset_service_ble_state_t state)
{
    handset_service_ble_state_t old_state = ble_sm->le_state;

    HS_LOG("HandsetService_BleSmSetState enum:handset_service_ble_state_t:%d -> enum:handset_service_ble_state_t:%d", old_state, state);

    ble_sm->le_state = state;

    if(old_state != HANDSET_SERVICE_STATE_BLE_DISCONNECTING && state == HANDSET_SERVICE_STATE_BLE_DISCONNECTING)
    {
        handsetService_BleSmEnterDisconnecting(ble_sm);
    }

    if(old_state != HANDSET_SERVICE_STATE_BLE_DISCONNECTED && state == HANDSET_SERVICE_STATE_BLE_DISCONNECTED)
    {
        handsetService_BleSmEnterDisconnected(ble_sm);
    }
#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    if(old_state != state && (state == HANDSET_SERVICE_STATE_BLE_CONNECTED || state == HANDSET_SERVICE_STATE_BLE_DISCONNECTED))
    {
        HandsetService_UpdateAdvertising();
    }
#endif
    ContextFramework_NotifyContextUpdate(context_connected_handsets_info);
}

bool HandsetService_BleIsConnected(handset_service_ble_state_machine_t* ble_sm)
{
    PanicNull(ble_sm);
    
    switch(ble_sm->le_state)
    {
        case HANDSET_SERVICE_STATE_BLE_CONNECTED:
        case HANDSET_SERVICE_STATE_BLE_DISCONNECTING:
        {
            if (!BdaddrTpIsEmpty(&ble_sm->le_addr) && ConManagerIsTpConnected(&ble_sm->le_addr))
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        
        default:
        {
            return FALSE;
        }
    }
}

void HandsetService_BleDisconnectIfConnected(handset_service_ble_state_machine_t* ble_sm)
{
    if (HandsetService_BleIsConnected(ble_sm))
    {
        HandsetService_BleSmSetState(ble_sm, HANDSET_SERVICE_STATE_BLE_DISCONNECTING);
    }
}

void HandsetService_BleHandleConnected(handset_service_ble_state_machine_t* ble_sm, tp_bdaddr* tpaddr)
{
    device_t device;
    handset_service_state_machine_t* sm = HandsetService_GetSmFromBleSm(ble_sm);

    if (BdaddrTpIsEmpty(&ble_sm->le_addr))
    {
        ble_sm->le_addr = *tpaddr;
    }

    HandsetService_BleSmSetState(ble_sm, HANDSET_SERVICE_STATE_BLE_CONNECTED);

    device = BtDevice_GetDeviceForTpbdaddr(tpaddr);

    /* We might get BLE connection before BR/EDR connection, but it could so happen that
       we are in middle of pairing, so we end up triggering First Transport connected indication.
       Impact is that "connection" prompt will be followed by "pairing successful" or worst
       "pairing failed". This gives bad user behavior. So, we need to avoid sending the notification
       if pairing is still active */
    if (device != NULL
        && BtDevice_GetDeviceType(device) == DEVICE_TYPE_HANDSET
        && (!HandsetServiceSm_IsBredrAclConnected(sm) || !sm->first_bredr_profile_connected)
        && PairingIsIdle())
    {
        /* Send the first tranport connected notification to play the connected prompt */
        HandsetService_SendFirstTransportConnectedIndNotification(tpaddr);
    }

    appLinkPolicyUpdatePowerTable(&ble_sm->le_addr.taddr.addr);
}
