/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    handset_service
    \brief      Handset service
*/

#include <bdaddr.h>
#include <vm.h>
#include <logging.h>
#include <task_list.h>

#include <bt_device.h>
#include <pairing.h>
#include <bredr_scan_manager.h>
#include <connection_manager.h>
#include <device_properties.h>
#include <hfp_profile.h>
#include <profile_manager.h>
#include <pairing.h>
#include <timestamp_event.h>
#if defined(INCLUDE_MIRRORING) && !defined(DISABLE_KEY_SYNC)
#include <key_sync.h>
#endif
#include <device_list.h>
#ifdef INCLUDE_TWS
#include <device_sync.h>
#endif
#include <stdlib.h>
#include <ui.h>
#include <ui_user_config.h>
#include <focus_device.h>
#include <handset_bredr_context.h>

#include "context_framework.h"
#include "device_db_serialiser.h"
#include "handset_service.h"
#include "handset_service_sm.h"
#include "handset_service_connectable.h"
#include "handset_service_config.h"

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
#include "handset_service_advertising.h"
#include "lea_advertising_policy.h"
#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
#include "le_unicast_manager.h"
#endif
#else
#include "handset_service_extended_advertising.h"
#include "handset_service_legacy_advertising.h"
#endif
#include "handset_service_pddu.h"
#include "handset_service_protected.h"
#include "qualcomm_connection_manager.h"
#include "sass.h"
#include "user_accounts.h"

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(handset_service_msg_t)
LOGGING_PRESERVE_MESSAGE_TYPE(handset_service_internal_msg_t)
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(HANDSET_SERVICE, HANDSET_SERVICE_MESSAGE_END)

/*! Handset Service module data. */
handset_service_data_t handset_service;

#ifdef INCLUDE_UI_USER_CONFIG
static const ui_user_config_context_id_map_t handset_map[] =
{
    { .context_id=ui_context_handset_connected, .context=context_handset_connected },
    { .context_id=ui_context_handset_disconnected, .context=context_handset_not_connected },
};
#endif

/*
    Helper functions
*/

static void handsetService_DisconnectAllMessageHandler(Task task, MessageId id, Message message);

static const TaskData disconnect_all_task = {handsetService_DisconnectAllMessageHandler};

/*! Stores if the Handset can be paired.

    \param pairing The headset pairing state.
*/
static void HandsetService_SetPairing(bool pairing)
{ 
    HandsetService_Get()->pairing = pairing;
}

/*! Try to find an active handset state machine for a device_t.

    \param device Device to search for.
    \return Pointer to the matching state machine, or NULL if no match.
*/
static handset_service_state_machine_t *handsetService_GetSmForDevice(device_t device)
{
    handset_service_state_machine_t *sm_match = NULL;

    FOR_EACH_HANDSET_SM(sm)
    {
        if (   (sm->state != HANDSET_SERVICE_STATE_NULL)
            && (sm->handset_device == device))
        {
            sm_match = sm;
            break;
        }
    }

    return sm_match;
}

handset_service_state_machine_t *handsetService_GetSmForBredrAddr(const bdaddr *addr)
{
    handset_service_state_machine_t *sm_match = NULL;
    const bdaddr *bredr_bdaddr;

    DEBUG_LOG_VERBOSE("handsetService_GetSmForBredrAddr Searching for addr [%04x,%02x,%06lx]",
                        addr->nap, addr->uap, addr->lap);

    if (!BdaddrIsZero(addr))
    {
        FOR_EACH_HANDSET_SM(sm)
        {
            bredr_bdaddr = &sm->handset_addr;

            DEBUG_LOG_VERBOSE("handsetService_GetSmForBredrAddr Check SM [%p] state [%d] addr [%04x,%02x,%06lx]",
                              sm, sm->state, bredr_bdaddr->nap, bredr_bdaddr->uap, bredr_bdaddr->lap);

            if (   (sm->state != HANDSET_SERVICE_STATE_NULL)
                && BdaddrIsSame(bredr_bdaddr, addr))
            {
                sm_match = sm;
                break;
            }
        }
    }

    return sm_match;
}

handset_service_state_machine_t *handsetService_GetLeSmForTypedBdAddr(const typed_bdaddr *taddr)
{
    handset_service_state_machine_t *sm_match = NULL;

    DEBUG_LOG_VERBOSE("handsetService_GetLeSmForTypedBdAddr searching for type [%d] addr [%04x,%02x,%06lx]",
                       taddr->type, taddr->addr.nap, taddr->addr.uap, taddr->addr.lap);

    if (!BdaddrTypedIsEmpty(taddr))
    {
        FOR_EACH_HANDSET_SM(sm)
        {
            typed_bdaddr le_taddr = HandsetServiceSm_GetLeTpBdaddr(sm).taddr;

            DEBUG_LOG_VERBOSE("handsetService_GetLeSmForTypedBdAddr Check SM [%p] state [%d] type [%d] addr [%04x,%02x,%06lx]",
                          sm, sm->state, le_taddr.type, le_taddr.addr.nap, le_taddr.addr.uap, le_taddr.addr.lap);

            if (   (sm->state != HANDSET_SERVICE_STATE_NULL)
                && BtDevice_BdaddrTypedIsSame(taddr, &le_taddr))
            {
                sm_match = sm;
                break;
            }
        }
    }

    return sm_match;
}

/*! Try to find an active handset state machine for a tp_bdaddr.

    Note: handset_service currently supports only one handset sm.

    \param tp_addr Address to search for.
    \return Pointer to the matching state machine, or NULL if no match.
*/
static handset_service_state_machine_t *handsetService_GetSmForTpBdAddr(const tp_bdaddr *tp_addr)
{
    handset_service_state_machine_t *sm = NULL;

    DEBUG_LOG("handsetService_GetSmForTpBdAddr transport [%d] type [%d] addr [%04x,%02x,%06lx]",
              tp_addr->transport, tp_addr->taddr.type,
              tp_addr->taddr.addr.nap, tp_addr->taddr.addr.uap, tp_addr->taddr.addr.lap);

    switch (tp_addr->transport)
    {
    case TRANSPORT_BREDR_ACL:
        {
            /* First try to match the BR/EDR address */
            sm = handsetService_GetSmForBredrAddr(&tp_addr->taddr.addr);
            if (!sm)
            {
                /* Second try to match the device_t handle */
                device_t dev = BtDevice_GetDeviceForBdAddr(&tp_addr->taddr.addr);
                if (dev)
                {
                    sm = handsetService_GetSmForDevice(dev);
                }
            }

            /* Third try to match to the LE address. */
            if (!sm)
            {
                sm = handsetService_GetLeSmForTypedBdAddr(&tp_addr->taddr);
            }
        }
        break;

    case TRANSPORT_BLE_ACL:
        {
            /* First try to match the LE address to a sm LE addr */
            sm = handsetService_GetLeSmForTypedBdAddr(&tp_addr->taddr);

            /* Second try to match the bdaddr component to a sm BR/EDR address */
            if (!sm)
            {
                tp_bdaddr resolved_tpaddr = {0};
                ConManagerResolveTpaddr(tp_addr, &resolved_tpaddr);
                sm = handsetService_GetSmForBredrAddr(&resolved_tpaddr.taddr.addr);
            }
        }
        break;

    default:
        HS_LOG("handsetService_GetSmForTpBdAddr Unsupported transport type %d", tp_addr->transport);
    }
    
    return sm;
}

handset_service_state_machine_t *HandsetService_GetSmForBdAddr(const bdaddr *addr)
{
    tp_bdaddr tp_addr = {0};

    BdaddrTpFromBredrBdaddr(&tp_addr, addr);
    return handsetService_GetSmForTpBdAddr(&tp_addr);
}

/*! \brief Create a new instance of a handset state machine.

    This will return NULL if a new state machine cannot be created,
    for example if the maximum number of handsets already exists.

    \param device Device to create state machine for.

    \return Pointer to new state machine, or NULL if it couldn't be created.
*/
static handset_service_state_machine_t *handsetService_CreateSm(device_t device)
{
    handset_service_state_machine_t *new_sm = NULL;

    FOR_EACH_HANDSET_SM(sm)
    {
        if (sm->state == HANDSET_SERVICE_STATE_NULL)
        {
            new_sm = sm;
            HandsetServiceSm_Init(new_sm);
            HandsetServiceSm_SetDevice(new_sm, device);
            HandsetServiceSm_SetState(new_sm, HANDSET_SERVICE_STATE_DISCONNECTED);
            /* A device already exists. Pairing may be possible if cross transport
               key derivation is used. */
            break;
        }
    }

    return new_sm;
}

/*! \brief Create a new instance of a handset state machine for a LE connection.

    This will return NULL if a new state machine cannot be created,
    for example if the maximum number of handsets already exists.

    \param addr Address to create state machine for.

    \return Pointer to new state machine, or NULL if it couldn't be created.
*/
static handset_service_state_machine_t *handsetService_CreateLeSm(const tp_bdaddr *addr)
{
    handset_service_state_machine_t *new_sm = NULL;

    FOR_EACH_HANDSET_SM(sm)
    {
        if (sm->state == HANDSET_SERVICE_STATE_NULL)
        {
            new_sm = sm;
            HandsetServiceSm_Init(new_sm);
            new_sm->ble_sm.le_addr = *addr;
            HandsetServiceSm_SetState(new_sm, HANDSET_SERVICE_STATE_DISCONNECTED);

                /* If the address is resolvable then adding pairing can be an issue */
            bool random = (addr->taddr.type != TYPED_BDADDR_PUBLIC);
            bool resolvable = ((addr->taddr.addr.nap & 0xC000) == 0x4000);
            new_sm->pairing_possible = random && resolvable;

            if (new_sm->pairing_possible)
            {
                DEBUG_LOG("handsetService_CreateLeSm for 0x%06x - kicking off timeout", addr->taddr.addr.lap);
                MESSAGE_MAKE(message, HANDSET_SERVICE_INTERNAL_POSSIBLE_PAIRING_TIMEOUT_T);

                message->address = *addr;
                message->sm = new_sm;
                MessageSendLater(HandsetService_GetTask(),
                                 HANDSET_SERVICE_INTERNAL_POSSIBLE_PAIRING_TIMEOUT, message,
                                 handsetService_AdvertisingSuspensionForPairingMs());
            }
            break;
        }
    }

    return new_sm;
}

handset_service_state_machine_t *handsetService_FindOrCreateSm(const tp_bdaddr *tp_addr)
{
    handset_service_state_machine_t *sm = handsetService_GetSmForTpBdAddr(tp_addr);
    
    if(!sm)
    {
        if (tp_addr->transport == TRANSPORT_BLE_ACL)
        {
            sm = handsetService_CreateLeSm(tp_addr);
        }
        else if(tp_addr->transport == TRANSPORT_BREDR_ACL)
        {
            if(!HandsetServiceSm_MaxBredrAclConnectionsReached())
            {
                device_t device = BtDevice_GetDeviceForBdAddr(&tp_addr->taddr.addr);
                sm = handsetService_CreateSm(device);
            }
        }
        else
        {
            Panic();
        }
    }
    
    return sm;
}

/* \brief Check if a new handset is allowed to connect

   This function will check if a new handset should be allowed to connect.

   Currently we do not support more than one handset connected at a time, so
   we must be able to reject or disconnect any other handset that tries to
   connect.

   A handset is considered connected if the BR/EDR ACL is connected.
   For example, if the ACL is connected but the BR/EDR profiles are connecting
   then it is considered to be connected.
*/
bool handsetService_CheckHandsetCanConnect(const bdaddr *addr)
{
    if(HandsetServiceSm_MaxBredrAclConnectionsReached())
    {
        handset_service_state_machine_t *sm = HandsetService_GetSmForBdAddr(addr);
        
        if(!sm || !HandsetServiceSm_IsBredrAclConnected(sm))
        {
            return FALSE;
        }
    }
    
    return TRUE;
}

bool HandsetService_IsBleConnected(void)
{
    unsigned num_connections = HandsetServiceSm_GetLeAclConnectionCount();

    return num_connections != 0;
}

/*! \brief Send a HANDSET_SERVICE_INTERNAL_CONNECT_REQ to a state machine. */
static void handsetService_InternalConnectReq(handset_service_state_machine_t *sm, uint32 profiles)
{
    MESSAGE_MAKE(req, HANDSET_SERVICE_INTERNAL_CONNECT_REQ_T);
    req->device = sm->handset_device;
    req->profiles = profiles;
    MessageSend(&sm->task_data, HANDSET_SERVICE_INTERNAL_CONNECT_REQ, req);
}

/*! \brief Send a HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ to a state machine. */
static void handsetService_InternalDisconnectReq(handset_service_state_machine_t *sm,
    const bdaddr *addr, uint32 exclude)
{
    MESSAGE_MAKE(req, HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ_T);
    req->addr = *addr;
    req->exclude = exclude;
    MessageSend(&sm->task_data, HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ, req);
}

/*! \brief Send a HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ to a state machine. */
static void handsetService_InternalConnectStopReq(handset_service_state_machine_t *sm)
{
    MESSAGE_MAKE(req, HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ_T);
    req->device = sm->handset_device;
    MessageSend(&sm->task_data, HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ, req);
}

/*! \brief Helper function for starting a connect req. */
static void handsetService_ConnectReq(Task task, const bdaddr *addr, uint32 profiles)
{
    handset_service_state_machine_t *sm = HandsetService_GetSmForBdAddr(addr);
    device_t dev = BtDevice_GetDeviceForBdAddr(addr);

    TimestampEvent(TIMESTAMP_EVENT_HANDSET_CONNECTION_START);

    /* If the state machine doesn't exist yet, and we are allowed to connect
       to a new handset, create a new state machine. */
    if (!sm && handsetService_CheckHandsetCanConnect(addr))
    {
        HS_LOG("handsetService_ConnectReq creating new handset sm");
        sm = handsetService_CreateSm(dev);
    }

    if (sm)
    {
        if (!sm->handset_device)
        {
            HandsetServiceSm_SetDevice(sm, dev);
        }
        HandsetServiceSm_CompleteDisconnectRequests(sm, handset_service_status_cancelled);
        handsetService_InternalConnectReq(sm, profiles);
        TaskList_AddTask(&sm->connect_list, task);
    }
    else
    {
        HS_LOG("handsetService_ConnectReq Couldn't create a new handset sm");

        MESSAGE_MAKE(cfm, HANDSET_SERVICE_CONNECT_CFM_T);
        cfm->addr = *addr;
        cfm->status = handset_service_status_failed;
        MessageSend(task, HANDSET_SERVICE_CONNECT_CFM, cfm);
    }
}

/*! \brief Callback to resolve the conflicts happened during device merge ie, BtDevice_Merge() */
static device_merge_action_t handsetService_DeviceMergeResolveCallback(device_t source_device, device_t target_device,
                                                                       device_property_t property_id)
{
    device_merge_action_t resolve_action = DEVICE_MERGE_ACTION_UNRESOLVABLE;
    deviceType *source_object = NULL, *target_object = NULL;
    size_t source_size = 0, target_size = 0;

    PanicFalse(Device_GetProperty(source_device, property_id, (void *)&source_object, &source_size));
    PanicFalse(Device_GetProperty(target_device, property_id, (void *)&target_object, &target_size));
    PanicFalse(source_size == target_size);

    if (property_id == device_property_type)
    {
        PanicFalse(source_size == sizeof(deviceType));

        if (*((deviceType*)source_object) == DEVICE_TYPE_HANDSET_LE && *((deviceType*)target_object) == DEVICE_TYPE_HANDSET)
        {
            /* This is expected to happen when an LE handset connects over random address first and
               then gets paired. Keep the target device type as it is. */
            resolve_action = DEVICE_MERGE_ACTION_ACCEPT_TARGET_LIST;
        }
    }
    else if (property_id == device_property_flags)
    {
        uint16 source_flag, target_flag, source_flag_conflict_bits;

        PanicFalse(source_size == sizeof(uint16));
        source_flag = *((uint16*)source_object);
        target_flag = *((uint16*)target_object);
        source_flag_conflict_bits = (source_flag ^ target_flag) & source_flag;

        /* Check if this conflict happened due to bit DEVICE_FLAGS_NOT_PAIRED in the source device */
        if (source_flag_conflict_bits == DEVICE_FLAGS_NOT_PAIRED)
        {
            /* This is expected to happen when a handset device(BREDR) gets paired first and then LE gets paired */
            resolve_action = DEVICE_MERGE_ACTION_ACCEPT_TARGET_LIST;
        }
    }
    else if (property_id == device_property_handset_account_key_index)
    {
        /* Property 'device_property_handset_account_key_index' is getting set with a default value during device
           creation itself. Check if the conflict is due to source device (LE device) having the default value. */
        if (*((uint8*)source_object) == INVALID_USER_ACCOUNT_KEY_INDEX)
        {
            resolve_action = DEVICE_MERGE_ACTION_ACCEPT_TARGET_LIST;
        }
    }
    else if (property_id == device_property_gatt_service_discovery)
    {
        /* This property can be set to 1 upon service discovery completion for handset LE device. If conflict
           happen due to this we need to use the value in handset LE device as merged value */
        if (*((uint8*)source_object) == 1)
        {
            resolve_action = DEVICE_MERGE_ACTION_ACCEPT_SOURCE_LIST;
        }
    }

    HS_LOG("handsetService_DeviceMergeResolveCallback property %d resolve_action %d", property_id, resolve_action);

    return resolve_action;
}

/*! \brief Merges the BREDR handset state machine and the BLE handset state machine(created before pairing) into one 
           handset service state machine for the same handset Once merging is done, it resets the BLE handset service
           state machine */
static void handsetService_MergeHandsetStateMachinesForBredrAndBleDevices(device_t device, device_t device_le)
{
    tp_bdaddr tpaddr;
    handset_service_state_machine_t *ble_device_sm;
    handset_service_state_machine_t *bredr_device_sm = handsetService_GetSmForDevice(device);

    PanicFalse(BtDevice_GetRandomTpAddrForDevice(device_le, &tpaddr));
    ble_device_sm = handsetService_GetSmForTpBdAddr(&tpaddr);

    if (ble_device_sm != NULL && bredr_device_sm != NULL)
    {
        memcpy(&bredr_device_sm->ble_sm, &ble_device_sm->ble_sm, sizeof(handset_service_ble_state_machine_t));
        HandsetServiceSm_DeInit(ble_device_sm);
    }
}

/*! \brief Merge handset LE device to handset device if required */
static device_t handsetService_MergeLeDeviceAndSmIfRequired(device_t device, device_t device_le, const bdaddr* bd_addr)
{
    if (device != NULL)
    {
       /*  Handset devices exists. Merge LE handset device into the handset device and destroy the LE Handset device.
        *  Also merge the LE handset state machine into the handset state machine and reset the LE handset state
        *  machine.
        */
        DEBUG_LOG("handsetService_MergeLeDeviceIfRequired merge handset devices source device %p target device %p",
                   device_le, device);
        handsetService_MergeHandsetStateMachinesForBredrAndBleDevices(device, device_le);
        BtDevice_Merge(device_le, device, handsetService_DeviceMergeResolveCallback, TRUE);
    }
    else
    {
        DEBUG_LOG("handsetService_MergeLeDeviceIfRequired update LE device %p", device_le);
        /* Set below as LE device won't be having these properties */
        appDeviceSetLinkModeForDevice(device_le, DEVICE_LINK_MODE_UNKNOWN);
        BtDevice_SetSupportedProfilesForDevice(device_le, 0x0);
        PanicFalse(BtDevice_SetDefaultProperties(device_le));

        /* Update LE handset device to a handset device of type 'DEVICE_TYPE_HANDSET' */
        BtDevice_SetDevicePublicAddress(device_le, bd_addr);
        BtDevice_SetDeviceType(device_le, DEVICE_TYPE_HANDSET);

        /* Return the updated LE device */
        device = device_le;
    }

    Device_RemoveProperty(device, device_property_random_tp_bdaddr);

    return device;
}

/*
    Message handler functions
*/

static void handsetService_HandlePairingActivity(const PAIRING_ACTIVITY_T* pair_activity)
{
    DEBUG_LOG("handsetService_HandlePairingActivity");

    bool can_send_first_transport_ind = FALSE;

    switch (pair_activity->status)
    {
        case pairingActivityInProgress:
            if (!HandsetService_IsPairing())
            {
                HS_LOG("handsetService_HandlePairingActivity. Pairing Active");
                HandsetService_SetPairing(TRUE);
                handsetService_ObserveConnections();
                ConManagerHandsetPairingMode(TRUE);
            }
            break;

        case pairingActivityNotInProgress:
            if (HandsetService_IsPairing())
            {
                HS_LOG("handsetService_HandlePairingActivity. Pairing Inactive");
                HandsetService_SetPairing(FALSE);
                ConManagerHandsetPairingMode(FALSE);
            }
            break;

        case pairingActivitySuccess:
            {
                DEBUG_LOG("handsetService_HandlePairingActivity pairingSuccess");

                handset_service_state_machine_t *sm = HandsetService_GetSmForBdAddr(&pair_activity->device_addr);
                if(sm)
                {
                    sm->pairing_possible = FALSE;

                    device_t dev = BtDevice_GetDeviceForBdAddr(&pair_activity->device_addr);

                    /* Get the handset device of type 'DEVICE_TYPE_HANDSET_LE' with random address matching
                       with the received public address */
                    device_t dev_le = BtDevice_GetDeviceWithRraThatResolvesToPublicAddr(&pair_activity->device_addr);

                    /* Device not exists, this isn't expected */
                    PanicFalse(dev != NULL || dev_le != NULL);

                    /* If we have an LE device with matching random address, then it needs to be merged/updated to handset device */
                    if(dev_le != NULL && pair_activity->permanent)
                    {
                        dev = handsetService_MergeLeDeviceAndSmIfRequired(dev, dev_le, &pair_activity->device_addr);
                        can_send_first_transport_ind = TRUE;
                    }

                    if(dev && (BtDevice_GetDeviceType(dev) == DEVICE_TYPE_HANDSET))
                    {
                        /* If this is an LE only handset this will not yet have been populated */
                        if(!sm->handset_device)
                        {
                            HandsetServiceSm_SetDevice(sm, dev);
                        }

                        DEBUG_LOG("handsetService_HandlePairingActivity Synchronise Link Keys");
                        PanicFalse(BtDevice_SetFlags(dev, DEVICE_FLAGS_NOT_PAIRED, 0));
                        PanicFalse(BtDevice_SetFlags(dev, DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD, DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD));

                        /* Now that we have successfully paired, we can set the link behavior within bluestack to disable connection retires */
                        BtDevice_SetLinkBehavior(&pair_activity->device_addr);

#if defined(INCLUDE_MIRRORING) && !defined(DISABLE_KEY_SYNC)
                        /* Sync the newly paired device with the peer */
                        KeySync_Sync();
#endif
                    }
                    /* Update the PDL with the device in the persistent device data. This is in order to ensure
                       we don't lose device information in case of device disconnecting right after pairing 
                       without a profile connection */
                    if (BtDevice_isKnownBdAddr(&pair_activity->device_addr))
                    {
                        can_send_first_transport_ind = TRUE;
                        DEBUG_LOG("handsetService_HandlePairingActivity Known Device, update DB for only that dev");
                        DeviceDbSerialiser_SerialiseDevice(dev);
                    }

                    if ((!HandsetServiceSm_IsBredrAclConnected(sm) || !sm->first_bredr_profile_connected) &&
                         can_send_first_transport_ind && HandsetServiceSm_IsLeAclConnected(sm))
                    {
                        DEBUG_LOG("handsetService_HandlePairingActivity Send First Transport Connected Indication");
                        HandsetService_SendFirstTransportConnectedIndNotification(&sm->ble_sm.le_addr);
                    }
                }
            }
            break;

        default:
            break;
    }
#ifdef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    HandsetService_UpdateLegacyAdvertisingData();
    UNUSED(HandsetServiceExtAdv_UpdateAdvertisingData());
#endif
}

/*! \brief Make message for LE connectable indication and send it to task list */
void HandsetService_SendLeConnectableIndication(bool connectable)
{
    MESSAGE_MAKE(le_connectable_ind, HANDSET_SERVICE_LE_CONNECTABLE_IND_T);
    le_connectable_ind->status = handset_service_status_success;
    le_connectable_ind->le_connectable = connectable;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), HANDSET_SERVICE_LE_CONNECTABLE_IND, le_connectable_ind);
}

/* \brief Handle a CON_MANAGER_TP_CONNECT_IND for BR/EDR and BLE connections */
static void handsetService_HandleConManagerTpConnectInd(const CON_MANAGER_TP_CONNECT_IND_T *ind)
{
    TRANSPORT_T transport = ind->tpaddr.transport;
    const typed_bdaddr* taddr = &ind->tpaddr.taddr;
    
    HS_LOG("handsetService_HandleConManagerTpConnectInd type[%d] addr [%04x,%02x,%06lx] incoming [%d]",
           taddr->type, taddr->addr.nap, taddr->addr.uap, taddr->addr.lap, ind->incoming);

    if (ind->incoming)
    {
        if(!BtDevice_LeDeviceIsPeer(&ind->tpaddr))
        {
            handset_service_state_machine_t *sm;

            if(transport == TRANSPORT_BLE_ACL)
            {
                tp_bdaddr resolved_tpbdaddr = {0};
                if (ConManagerResolveTpaddr(&ind->tpaddr, &resolved_tpbdaddr))
                {
                    HS_LOG("handsetService_HandleConManagerTpConnectInd resolved addr to [%04x,%02x,%06lx]",
                           resolved_tpbdaddr.taddr.addr.nap,
                           resolved_tpbdaddr.taddr.addr.uap,
                           resolved_tpbdaddr.taddr.addr.lap);
                }

                device_t device = BtDevice_GetDeviceForBdAddr(&resolved_tpbdaddr.taddr.addr);

                if(device == NULL)
                {
                    /* Since device not exists, create a handset device based on the address type. For a random
                       address, device type 'DEVICE_TYPE_HANDSET_LE' will gets created. If it gets paired later,
                       then the device will be merged to 'DEVICE_TYPE_HANDSET' type. */
                    deviceType dev_type = (resolved_tpbdaddr.taddr.type == TYPED_BDADDR_RANDOM) ? DEVICE_TYPE_HANDSET_LE : DEVICE_TYPE_HANDSET;
                    device = PanicNull(BtDevice_GetDeviceCreateIfNewWithTpAddr(&resolved_tpbdaddr, dev_type));

                    HS_LOG("handsetService_HandleConManagerTpConnectInd Created new handset device %p type %d", device, dev_type);
                    BtDevice_SetFlags(device, DEVICE_FLAGS_NOT_PAIRED, DEVICE_FLAGS_NOT_PAIRED);

                    /* Set default properties only for handset of type 'DEVICE_TYPE_HANDSET'. For 'DEVICE_TYPE_HANDSET_LE', this
                       will be set after it gets merged to 'DEVICE_TYPE_HANDSET' type. ie, after pairing. */
                    if (dev_type == DEVICE_TYPE_HANDSET)
                    {
                        PanicFalse(BtDevice_SetDefaultProperties(device));
                    }
                }

                sm = PanicNull(handsetService_FindOrCreateSm(&resolved_tpbdaddr));
                HS_LOG("handsetService_HandleConManagerTpConnectInd received for LE handset %p", sm);

#ifdef INCLUDE_BLE_PAIR_HANDSET_ON_CONNECT
                Pairing_PairLeAddress(HandsetService_GetTask(), taddr);
#endif

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
                /* Cancel Directed Advertising timeout message, if any pending in queue and set the ble context as connected */
                MessageCancelFirst(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_DIRECTED_ADVERT_TIMEOUT);

                if (device != NULL && BtDevice_GetDeviceType(device) == DEVICE_TYPE_HANDSET)
                {
                    DeviceProperties_SetHandsetBleContext(device, handset_ble_context_connected);
                    LeaAdvertisingPolicy_SetAdvertisingMode(lea_adv_policy_mode_undirected,
                                                            lea_adv_policy_announcement_type_none,
                                                            lea_adv_policy_announcement_type_general,
                                                            NULL);
                }
#endif
                HandsetServiceSm_HandleConManagerBleTpConnectInd(sm, ind);
            }
            else if(transport == TRANSPORT_BREDR_ACL)
            {
                device_t device = BtDevice_GetDeviceForBdAddr(&taddr->addr);
                
                if(!device)
                {
                    device = PanicNull(BtDevice_GetDeviceCreateIfNew(&taddr->addr, DEVICE_TYPE_HANDSET));
                    HS_LOG("handsetService_HandleConManagerTpConnectInd Create new handset device %p", device);
                    PanicFalse(BtDevice_SetDefaultProperties(device));
                    BtDevice_SetFlags(device, DEVICE_FLAGS_NOT_PAIRED, DEVICE_FLAGS_NOT_PAIRED);
                }

                sm = handsetService_FindOrCreateSm(&ind->tpaddr);
                HS_LOG("handsetService_HandleConManagerTpConnectInd received for BR/EDR handset %p", sm);

                if(sm)
                {
                    if(!HandsetServiceSm_GetHandsetDeviceIfValid(sm))
                    {
                        HandsetServiceSm_SetDevice(sm, device);
                    }

                    /* As handset just connected it cannot have profile connections, so clear flags */
                    BtDevice_SetConnectedProfiles(sm->handset_device, 0);

                    /* Forward the connection to the state machine. */
                    HandsetServiceSm_HandleConManagerBredrTpConnectInd(sm, ind);
                }
            }
#ifdef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
            HandsetService_UpdateLegacyAdvertisingData();
            UNUSED(HandsetServiceExtAdv_UpdateAdvertisingData());
#endif
        }
    }
}

/* \brief Handle a CON_MANAGER_AUTHORISE_IND for handset connections */
static void handsetService_HandleConManagerAuthoriseInd(const CON_MANAGER_AUTHORISE_IND_T *ind)
{
    bool auth = FALSE;
    HS_LOG("handsetService_HandleConManagerAuthReqInd addr [%04x,%02x,%06lx]",
           ind->bd_addr.addr.nap,
           ind->bd_addr.addr.uap,
           ind->bd_addr.addr.lap);

    if(HandsetService_GetSmForBdAddr(&ind->bd_addr.addr))
    {
        HS_LOG("handsetService_HandleConManagerAuthReqInd Accept");
        auth = TRUE;
    }

    ConManagerSendAuthoriseResponse(auth, &ind->bd_addr, ind->protocol_id, ind->channel, ind->incoming);
}

/* \brief Handle a Tp Disconnect ind for BLE transport */
static void handsetServiceSm_HandleConManagerBleTpDisconnectInd(handset_service_state_machine_t *sm, const CON_MANAGER_TP_DISCONNECT_IND_T *ind)
{

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
    handset_service_data_t *hs = HandsetService_Get();
    device_t device = HandsetServiceSm_GetHandsetDeviceIfValid(sm);
    tp_bdaddr handset_bdaddr;
    handset_service_lea_connect_params_type_t lea_param = {0};
    handset_service_reconnect_type_t reconnect_type = handset_service_reconnect_type_linkloss_no_streaming;
    handset_ble_context_t ble_context;
#else
    UNUSED(ind);
#endif

    HandsetService_BleSmSetState(&sm->ble_sm, HANDSET_SERVICE_STATE_BLE_DISCONNECTED);

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT

    /* Check whether device is valid or not before accessing as device can get deleted when moving to
       HANDSET_SERVICE_STATE_BLE_DISCONNECTED (if unpaired) */
    if (device != NULL && BtDevice_DeviceIsValid(device) &&
        BtDevice_GetDeviceType(device) == DEVICE_TYPE_HANDSET &&
        BtDevice_GetTpBdaddrForDevice(device, &handset_bdaddr))
    {
        ble_context = handset_ble_context_disconnected;

        if (ind->reason == hci_error_conn_timeout || ind->reason == hci_error_lmp_response_timeout)
        {
            ble_context = handset_ble_context_link_loss;

            lea_param.dir_announcement = lea_adv_policy_announcement_type_targeted;
            lea_param.undir_announcement = lea_adv_policy_announcement_type_none;

            lea_param.dir_sink_audio_context = audio_context_type_unknown;
            lea_param.dir_source_audio_context = audio_context_type_unknown;

            lea_param.undir_sink_audio_context = audio_context_type_unknown;
            lea_param.undir_source_audio_context = audio_context_type_unknown;

            if (LeUnicastManager_IsLinklossWhileStreaming(&handset_bdaddr) ||
                LeUnicastManager_IsStreamingActive() ||
                DeviceProperties_GetHandsetBleContext(device) == handset_ble_context_link_loss_streaming)
            {
                reconnect_type = handset_service_reconnect_type_linkloss_streaming;
                ble_context = handset_ble_context_link_loss_streaming;
            }

            if (hs->connect_callback != NULL &&
                hs->connect_callback->GetDeviceAndLeaParamsToConnect != NULL)
            {
                hs->connect_callback->GetDeviceAndLeaParamsToConnect(reconnect_type,
                                                                     &handset_bdaddr,
                                                                     &lea_param);
            }

            HandsetService_ConnectLeaDevice(&handset_bdaddr.taddr.addr, &lea_param);
        }

        DeviceProperties_SetHandsetBleContext(device, ble_context);
    }
#endif
}

/* \brief Handle a CON_MANAGER_TP_DISCONNECT_IND for BR/EDR and BLE disconnections */
static void handsetService_HandleConManagerTpDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T *ind)
{
    handset_service_state_machine_t *sm = handsetService_GetSmForTpBdAddr(&ind->tpaddr);

    HS_LOG("handsetService_HandleConManagerTpDisconnectInd sm [%p] type[%d] addr [%04x,%02x,%06lx], enum:hci_status:%u",
           sm, 
           ind->tpaddr.taddr.type, 
           ind->tpaddr.taddr.addr.nap,
           ind->tpaddr.taddr.addr.uap,
           ind->tpaddr.taddr.addr.lap,
           ind->reason);

    if(sm)
    {
        if (ind->tpaddr.transport == TRANSPORT_BLE_ACL)
        {
            handsetServiceSm_HandleConManagerBleTpDisconnectInd(sm, ind);
        }
        else if(ind->tpaddr.transport == TRANSPORT_BREDR_ACL)
        {
            HandsetServiceSm_HandleConManagerBredrTpDisconnectInd(sm, ind);
        }

        HandsetService_Get()->pairing_request_pending_lock = FALSE;
    }
}

static void handsetService_HandleProfileManagerConnectedInd(const CONNECTED_PROFILE_IND_T *ind)
{
    bdaddr addr = DeviceProperties_GetBdAddr(ind->device);
    bool is_handset = appDeviceIsHandset(&addr);

    HS_LOG("handsetService_HandleProfileManagerConnectedInd device 0x%x profile 0x%x handset %d [%04x,%02x,%06lx]",
           ind->device,
           ind->profile,
           is_handset,
           addr.nap,
           addr.uap,
           addr.lap);

    if (is_handset)
    {
        handset_service_state_machine_t *sm = HandsetService_GetSmForBdAddr(&addr);

        /* If state machine doesn't exist yet, need to create a new one */
        if (!sm)
        {
            HS_LOG("handsetService_HandleProfileManagerConnectedInd creating new handset sm");
            sm = handsetService_CreateSm(ind->device);
        }

        /* TBD: handset_service currently only supports one active handset, 
                so what should happen if a new device can't be created? */
        assert(sm);
        
        if (!sm->handset_device)
        {
            HandsetServiceSm_SetDevice(sm, ind->device);
        }

        /* Forward the connect ind to the state machine */
        HandsetServiceSm_HandleProfileManagerConnectedInd(sm, ind);
    }
}

static void handsetService_HandleProfileManagerDisconnectedInd(const DISCONNECTED_PROFILE_IND_T *ind)
{
    if (ind != NULL)
    {
        /* ACL disconnection may cause the Device handle in the indication to be invalid, if the device no longer exists, ignore the indication. */
        if(DeviceList_IsDeviceOnList(ind->device))
        {
            bdaddr addr = DeviceProperties_GetBdAddr(ind->device);
            bool is_handset = appDeviceIsHandset(&addr);

            HS_LOG("handsetService_HandleProfileManagerDisconnectedInd device 0x%x profile 0x%x handset %d [%04x,%02x,%06lx]",
                   ind->device,
                   ind->profile,
                   is_handset,
                   addr.nap,
                   addr.uap,
                   addr.lap);

            if (is_handset)
            {
                handset_service_state_machine_t *sm = handsetService_GetSmForDevice(ind->device);

                if (sm)
                {
                    /* Forward the disconnect ind to the state machine */
                    HandsetServiceSm_HandleProfileManagerDisconnectedInd(sm, ind);
                }
            }
        }
        else
        {
            HS_LOG("handsetService_HandleProfileManagerDisconnectedInd, device doesn't exists anymore in the database");    
        }
    }
    else
    {
        HS_LOG("handsetService_HandleProfileManagerDisconnectedInd, shouldn't expect NULL indication");
    }
}

static void handsetService_HandlePairingPairCfm(PAIRING_PAIR_CFM_T* cfm)
{
    Task pairing_task = HandsetService_Get()->pairing_task;
    
    HS_LOG("handsetService_HandlePairingPairCfm enum:pairingStatus:%d", cfm->status);
    
    if(pairing_task)
    {
        MESSAGE_MAKE(msg, HANDSET_SERVICE_PAIR_HANDSET_CFM_T);
        msg->status = (cfm->status == pairingSuccess) ? handset_service_status_success : handset_service_status_failed;
        msg->bd_addr = cfm->device_bd_addr;
        MessageSend(pairing_task, HANDSET_SERVICE_PAIR_HANDSET_CFM, msg);
    }
}

static void handsetService_HandlePairingStopCfm(PAIRING_STOP_CFM_T* cfm)
{
    Task pairing_task = HandsetService_Get()->pairing_task;
    
    HS_LOG("handsetService_HandlePairingStopCfm enum:pairingStatus:%d", cfm->status);
    
    if(pairing_task)
    {
        MESSAGE_MAKE(msg, HANDSET_SERVICE_CANCEL_PAIR_HANDSET_CFM_T);
        msg->status = (cfm->status == pairingSuccess) ? handset_service_status_success : handset_service_status_failed;
        MessageSend(pairing_task, HANDSET_SERVICE_CANCEL_PAIR_HANDSET_CFM, msg);
    }
}

static void handsetService_HandlePairingTimeout(const HANDSET_SERVICE_INTERNAL_POSSIBLE_PAIRING_TIMEOUT_T *message)
{
    handset_service_state_machine_t *sm;

    HS_LOG("handsetService_HandlePairingTimeout 0x%06lx", message->address.taddr.addr.lap);

    sm = handsetService_GetSmForTpBdAddr(&message->address);
    /* Check that the SM still matches */
    if (sm && sm == message->sm && sm->state != HANDSET_SERVICE_STATE_NULL)
    {
        if (sm->pairing_possible)
        {
            sm->pairing_possible = FALSE;
#ifdef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
            HandsetService_UpdateLegacyAdvertisingData();
            UNUSED(HandsetServiceExtAdv_UpdateAdvertisingData());
#endif
        }
    }
}

static bool handsetService_IsPairedHandset(bdaddr* bd_addr)
{
    if(appDeviceIsHandset(bd_addr))
    {
        uint16 flags = DEVICE_FLAGS_NO_FLAGS;
        appDeviceGetFlags(bd_addr, &flags);
        if(!(flags & DEVICE_FLAGS_NOT_PAIRED))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void handsetService_HandleBargeInInd(const QCOM_CON_MANAGER_BARGE_IN_IND_T* msg)
{
    DEBUG_LOG("handsetService_HandleBargeInInd");

    if (HandsetService_IsPairing() || handsetService_IsPairedHandset((bdaddr*)&msg->bd_addr))
    {
        device_t device = BtDevice_GetDeviceForBdAddr(&msg->bd_addr);
        if(device != NULL)
        {
            DeviceProperties_SetHandsetBredrContext(device, handset_bredr_context_barge_in);
        }
        HandsetService_DisableTruncatedPageScan();
        HandsetService_DisconnectLruHandsetRequest(HandsetService_GetTask());
    }
}

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
static void handsetService_HandleInternalDirectedAdvertTimeout(const HANDSET_SERVICE_INTERNAL_DIRECTED_ADVERT_TIMEOUT_T *message)
{
    handset_ble_context_t ble_context;
    DEBUG_LOG("handsetService_HandleInternalDirectedAdvertTimeout");

    if (HandsetServiceSm_GetLeAclConnectionCount() < handsetService_LeAclMaxConnections())
    {
        /* Set Advert type as undirected and announcement type as general */
        LeaAdvertisingPolicy_SetAdvertisingMode(lea_adv_policy_mode_undirected,
                                                lea_adv_policy_announcement_type_none,
                                                lea_adv_policy_announcement_type_general,
                                                NULL);
    }

    ble_context = DeviceProperties_GetHandsetBleContext(message->device);

    if (ble_context == handset_ble_context_link_loss || ble_context == handset_ble_context_link_loss_streaming)
    {
        DeviceProperties_SetHandsetBleContext(message->device, handset_ble_context_disconnected);
    }
}
#endif

static void handsetService_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
    /* Connection Manager messages */
    case CON_MANAGER_TP_CONNECT_IND:
        handsetService_HandleConManagerTpConnectInd((const CON_MANAGER_TP_CONNECT_IND_T *)message);
        break;

    case CON_MANAGER_TP_DISCONNECT_IND:
        handsetService_HandleConManagerTpDisconnectInd((const CON_MANAGER_TP_DISCONNECT_IND_T *)message);
        break;

    case CON_MANAGER_AUTHORISE_IND:
        handsetService_HandleConManagerAuthoriseInd((const CON_MANAGER_AUTHORISE_IND_T*)message);
        break;

    /* Profile Manager messages */
    case CONNECTED_PROFILE_IND:
        handsetService_HandleProfileManagerConnectedInd((const CONNECTED_PROFILE_IND_T *)message);
        /* Some times Handset may initiate SASS switch when its not required, 
           so user has an option in the phone screen to revert back such switches, 
           in such cases earbud would have to accept the connection from phone fairly quickly, 
           so current restriction of not allowing barge-in for next 30 secs is not acceptable in this case. */
        if(Sass_IsConnectedDeviceForSassSwitch(((CONNECTED_PROFILE_IND_T *)message)->device) && 
           HandsetService_IsConnectionBargeInEnabled() && 
           HandsetServiceSm_MaxBredrAclConnectionsReached())
        {
            MessageCancelAll(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_TRUNC_PAGE_SCAN_ENABLE);
            HandsetService_EnableTruncatedPageScan();
        }
        break;

    case DISCONNECTED_PROFILE_IND:
        handsetService_HandleProfileManagerDisconnectedInd((const DISCONNECTED_PROFILE_IND_T *)message);
        break;

    /* BREDR Scan Manager messages */
    case BREDR_SCAN_MANAGER_PAGE_SCAN_PAUSED_IND:
    case BREDR_SCAN_MANAGER_PAGE_SCAN_RESUMED_IND:
        /* These are informational so no need to act on them. */
        break;

    /* Pairing messages */
    case PAIRING_ACTIVITY:
        HS_LOG("handsetService_MessageHandler MESSAGE:0x%x", id);
        handsetService_HandlePairingActivity((PAIRING_ACTIVITY_T*)message);
        break;
        
    case PAIRING_PAIR_CFM:
        handsetService_HandlePairingPairCfm((PAIRING_PAIR_CFM_T*)message);
        break;
        
    case PAIRING_STOP_CFM:
        handsetService_HandlePairingStopCfm((PAIRING_STOP_CFM_T*)message);
        break;

    case HANDSET_SERVICE_INTERNAL_POSSIBLE_PAIRING_TIMEOUT:
        handsetService_HandlePairingTimeout((const HANDSET_SERVICE_INTERNAL_POSSIBLE_PAIRING_TIMEOUT_T *)message);
        break;

    case HANDSET_SERVICE_INTERNAL_TRUNC_PAGE_SCAN_ENABLE:
        HandsetService_HandleInternalTruncatedPageScanEnable();
        break;
        
    case HANDSET_SERVICE_INTERNAL_FAST_PAGE_SCAN_TIMEOUT_IND:
        handsetService_HandleFastPageScanTimeout();
        break;

    /* Device property related messages */
    case BT_DEVICE_SELF_CREATED_IND:
        HS_LOG("handsetService_MessageHandler BT_DEVICE_SELF_CREATED_IND");
        handsetService_HandleSelfCreated();
        break;

    case BT_DEVICE_EARBUD_CREATED_IND:
        HS_LOG("handsetService_MessageHandler BT_DEVICE_EARBUD_CREATED_IND");
        HandsetService_HandleEarbudCreated();
        break;

#ifdef INCLUDE_TWS
    case DEVICE_SYNC_PROPERTY_UPDATE_IND:
        HS_LOG("handsetService_MessageHandler DEVICE_SYNC_PROPERTY_UPDATE_IND");
        handsetService_HandleConfigUpdate();
        break;
#endif

    case QCOM_CON_MANAGER_BARGE_IN_IND:
        HS_LOG("handsetService_MessageHandler QCOM_CON_MANAGER_BARGE_IN_IND");
        handsetService_HandleBargeInInd((const QCOM_CON_MANAGER_BARGE_IN_IND_T*) message);
        break;

    case LE_ADV_MGR_ENABLE_CONNECTABLE_CFM:
        HS_LOG("handsetService_MessageHandler LE_ADV_MGR_ENABLE_CONNECTABLE_CFM");
        break;
 
    case HANDSET_SERVICE_INTERNAL_PAIR_REQ:
        HS_LOG("handsetService_MessageHandler HANDSET_SERVICE_INTERNAL_PAIR_REQ");
        Pairing_PairWithParams(HandsetService_GetTask(), (pairing_params_t*) message);
        break;

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
    case HANDSET_SERVICE_INTERNAL_DIRECTED_ADVERT_TIMEOUT:
        handsetService_HandleInternalDirectedAdvertTimeout((const HANDSET_SERVICE_INTERNAL_DIRECTED_ADVERT_TIMEOUT_T *)message);
        break;
#endif

    default:
        HS_LOG("handsetService_MessageHandler unhandled id MESSAGE:0x%x", id);
        break;
    }
}

static void handsetService_DisconnectAllComplete(handset_service_status_t status)
{
    task_list_t *disconnect_all_tasklist = TaskList_GetFlexibleBaseTaskList(HandsetService_GetDisconnectAllClientList());
    handset_service_data_t *hs = HandsetService_Get();

    DEBUG_LOG_FN_ENTRY("handsetService_DisconnectAllComplete");

    hs->disconnect_all_in_progress = FALSE;

    /* Restore the OETC user error code back */
    hs->disconnect_reason_code = HCI_ERROR_OETC_USER;

    if (TaskList_Size(disconnect_all_tasklist))
    {
        MESSAGE_MAKE(cfm, HANDSET_SERVICE_MP_DISCONNECT_ALL_CFM_T);
        cfm->status = status;

        TaskList_MessageSend(disconnect_all_tasklist, HANDSET_SERVICE_MP_DISCONNECT_ALL_CFM, cfm);
        TaskList_RemoveAllTasks(disconnect_all_tasklist);
    }
}


static bool handsetService_GetConnectedBredrHandsetAddress(bdaddr *addr)
{
    bool bredr_handset = FALSE;
    PanicNull(addr);

    FOR_EACH_HANDSET_SM(sm)
    {
        if (sm->state != HANDSET_SERVICE_STATE_NULL)
        {
            if(HandsetServiceSm_IsBredrAclConnected(sm))
            {
                *addr = sm->handset_addr;
                bredr_handset = TRUE;
                break;
            }
        }
    }
    return bredr_handset;
}

static unsigned handsetService_GetCurrentContxt(void)
{
    handset_provider_context_t current_ctxt;

    if (appHfpIsConnected() || appDeviceIsHandsetA2dpConnected())
    {
        current_ctxt = context_handset_connected;
    }
    else
    {
        current_ctxt = context_handset_not_connected;
    }

    return (unsigned)current_ctxt;
}

static bool handsetService_DisconnectOneConnectedHandset(void)
{
    bool disconnecting = FALSE;
    bdaddr bredr_address;

    DEBUG_LOG_FN_ENTRY("handsetService_DisconnectOneConnectedHandset");

    if(handsetService_GetConnectedBredrHandsetAddress(&bredr_address))
    {
        DEBUG_LOG_VERBOSE("handsetService_DisconnectOneConnectedHandset disconnecting 0x%04x", bredr_address.lap);
        HandsetService_DisconnectRequest((Task)&disconnect_all_task, &bredr_address, 0);
        disconnecting = TRUE;
    }

    if(!disconnecting)
    {
        tp_bdaddr le_handset_tpaddr;
        if(HandsetService_GetConnectedLeHandsetTpAddress(&le_handset_tpaddr))
        {
            DEBUG_LOG_VERBOSE("handsetService_DisconnectOneConnectedHandset: LE Handset connected. Disconnecting 0x%04x", le_handset_tpaddr.taddr.addr.lap);
            HandsetService_DisconnectTpAddrRequest((Task)&disconnect_all_task, &le_handset_tpaddr, 0);
            disconnecting = TRUE;
        }
    }
    return disconnecting;
}

static void handsetService_DisconnectAllMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG_FN_ENTRY("handsetService_DisconnectAllMessageHandler message = 0x%04x", id);

    if(id == HANDSET_SERVICE_DISCONNECT_CFM)
    {
        HANDSET_SERVICE_DISCONNECT_CFM_T* cfm = (HANDSET_SERVICE_DISCONNECT_CFM_T*)message;
        DEBUG_LOG_VERBOSE("handsetService_DisconnectAllMessageHandler disconnected 0x%04x", cfm->addr.lap);

        if(!((cfm->status == handset_service_status_success) && handsetService_DisconnectOneConnectedHandset()))
        {
            /* nothing left to disconnect, message the clients */
            handsetService_DisconnectAllComplete(cfm->status);
        }
    }
}

static void handsetService_StartBargeInEnableTimerIfMaxConnectionsActive(uint32 delay)
{
    bool max_connections_reached;

    HS_LOG("handsetService_StartBargeInEnableTimerIfMaxConnectionsActive. Delay added: %d ms", delay);
    max_connections_reached = HandsetServiceSm_MaxBredrAclConnectionsReached();

    if( max_connections_reached && HandsetService_IsConnectionBargeInEnabled() )
    {
        HandsetService_RestartBargeInEnableTimer(delay);
    }
}

static bool handsetService_GetConnectedHandsetsInfo(unsigned * context_data, uint8 context_data_size)
{
    PanicZero(context_data_size >= sizeof(context_connected_handsets_info_t));
    memset(context_data, 0, sizeof(context_connected_handsets_info_t));
    uint8 connected_handsets_found = 0;
    FOR_EACH_HANDSET_SM(sm)
    {
        if (sm->state != HANDSET_SERVICE_STATE_NULL)
        {
            if(HandsetService_Connected(sm->handset_device))
            {
                ((context_connected_handsets_info_t *)context_data)->connected_handsets[connected_handsets_found].handset
                            = sm->handset_device;
                ((context_connected_handsets_info_t *)context_data)->connected_handsets[connected_handsets_found].is_bredr_connected
                            = HandsetServiceSm_IsBredrAclConnected(sm);
                ((context_connected_handsets_info_t *)context_data)->connected_handsets[connected_handsets_found].is_le_connected
                            = HandsetServiceSm_IsLeAclConnected(sm);

                connected_handsets_found++;
                if(connected_handsets_found >= CONTEXT_INFO_MAX_HANDSETS)
                {
                    break;
                }
            }
        }
    }
    ((context_connected_handsets_info_t *)context_data)->number_of_connected_handsets = connected_handsets_found;
    return TRUE;
}

/*
    Public functions
*/

bool HandsetService_Init(Task task)
{
    UNUSED(task);

    handset_service_data_t *hs = HandsetService_Get();
    memset(hs, 0, sizeof(*hs));
    hs->task_data.handler = handsetService_MessageHandler;
    hs->disconnect_reason_code = HCI_ERROR_OETC_USER;

    HandsetServiceConfig_Init();

    ProfileManager_ClientRegister(&hs->task_data);

    Pairing_ActivityClientRegister(&hs->task_data);

    BtDevice_RegisterListener(&hs->task_data);

    Ui_RegisterUiProvider(ui_provider_handset, handsetService_GetCurrentContxt);

#ifdef INCLUDE_UI_USER_CONFIG
    UiUserConfig_RegisterContextIdMap(ui_provider_handset, handset_map, ARRAY_DIM(handset_map));
#endif

#ifdef INCLUDE_TWS
    DeviceSync_RegisterForNotification(&hs->task_data);
#endif

    HandsetServiceSm_Init(hs->state_machine);
    HandsetServiceMultipointSm_Init();
    handsetService_ConnectableInit();
#ifdef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    HandsetService_LegacyAdvertisingInit();
    HandsetServiceExtAdv_Init();
#endif

    TaskList_InitialiseWithCapacity(HandsetService_GetClientList(), HANDSET_SERVICE_CLIENT_LIST_INIT_CAPACITY);
    TaskList_InitialiseWithCapacity(HandsetService_GetDisconnectAllClientList(), HANDSET_SERVICE_DISCONNECT_ALL_CLIENT_LIST_INIT_CAPACITY);

    QcomConManagerRegisterBargeInClient(HandsetService_GetTask());
    ContextFramework_RegisterContextProvider(context_connected_handsets_info, handsetService_GetConnectedHandsetsInfo);

    return TRUE;
}

void HandsetService_ClientRegister(Task client_task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), client_task);
}

void HandsetService_ClientUnregister(Task client_task)
{
    TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), client_task);
}

static void HandsetService_RegisterMessageGroup(Task client_task, message_group_t group)
{
    PanicFalse(group == HANDSET_SERVICE_MESSAGE_GROUP);
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), client_task);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(HANDSET_SERVICE, HandsetService_RegisterMessageGroup, NULL);

void HandsetService_ConnectAddressRequest(Task task, const bdaddr *addr, uint32 profiles)
{
    HS_LOG("HandsetService_ConnectAddressRequest addr [%04x,%02x,%06lx] profiles 0x%x",
            addr->nap, addr->uap, addr->lap, profiles);

    handsetService_ConnectReq(task, addr, profiles);
}

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT

bool HandsetService_ConnectLeaDevice(const bdaddr *addr,
                                     handset_service_lea_connect_params_type_t *lea_param)
{
    lea_adv_policy_mode_t adv_mode;
    device_t device = BtDevice_GetDeviceForBdAddr(addr);
    bool is_le_connectable = HandsetService_IsBleConnectable();

    HS_LOG("HandsetService_ConnectLeaDevice addr [%04x,%02x,%06lx]",
            addr->nap, addr->uap, addr->lap);

    if (device != NULL &&
       (!HandsetServiceSm_MaxLeAclConnectionsReached()))
    {
        typed_bdaddr peer_bd_addr;

        /* Cancel Directed Advertising timeout message, if any pending in queue */
        MessageCancelFirst(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_DIRECTED_ADVERT_TIMEOUT);

        peer_bd_addr.type = TYPED_BDADDR_PUBLIC;
        peer_bd_addr.addr.nap = addr->nap;
        peer_bd_addr.addr.uap = addr->uap;
        peer_bd_addr.addr.lap = addr->lap;

        adv_mode = (lea_param->undir_announcement == lea_adv_policy_announcement_type_none) ?
                   lea_adv_policy_mode_directed : lea_adv_policy_mode_directed_and_undirected;

        LeaAdvertisingPolicy_SetAudioContext(lea_adv_policy_mode_directed, lea_param->dir_sink_audio_context, lea_param->dir_source_audio_context);
        LeaAdvertisingPolicy_SetAudioContext(lea_adv_policy_mode_undirected, lea_param->undir_sink_audio_context, lea_param->undir_source_audio_context);

        LeaAdvertisingPolicy_SetAdvertisingMode(adv_mode,
                                                lea_param->dir_announcement,
                                                lea_param->undir_announcement,
                                                &peer_bd_addr);

        if (!is_le_connectable)
        {
            LeAdvertisingManager_EnableConnectableAdvertising(NULL, TRUE);
        }

        MESSAGE_MAKE(message, HANDSET_SERVICE_INTERNAL_DIRECTED_ADVERT_TIMEOUT_T);
        message->device = device;
        MessageSendLater(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_DIRECTED_ADVERT_TIMEOUT,
                         message, HANDSET_SERVICE_DIRECTED_ADVERTS_TIMEOUT_MS);

        HS_LOG("HandsetService_ConnectLeaDevice adv_mode : enum:lea_adv_policy_mode_t:%d, is_le_connectable:%d",
                adv_mode, is_le_connectable);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

void HandsetService_RegisterConnectDeviceCallback(const handset_service_connect_device_callback_t *const callback)
{
    handset_service_data_t *hs = HandsetService_Get();

    HS_LOG("HandsetService_RegisterConnectDeviceCallback %p", callback);
    hs->connect_callback = callback;
}

#endif

void HandsetService_DisconnectAll(Task task, uint8 reason_code)
{
    DEBUG_LOG_FN_ENTRY("HandsetService_DisconnectAll");

    handset_service_data_t *hs = HandsetService_Get();

    if(task)
    {
        TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(HandsetService_GetDisconnectAllClientList()), task);
    }

    if(!hs->disconnect_all_in_progress)
    {
        hs->disconnect_all_in_progress = TRUE;
        hs->disconnect_reason_code = reason_code;

        if(!handsetService_DisconnectOneConnectedHandset())
        {
            handsetService_DisconnectAllComplete(handset_service_status_success);
        }
    }
}

void HandsetService_DisconnectTpAddrRequest(Task task, const tp_bdaddr *tp_addr, uint32 exclude)
{
    HS_LOG("HandsetService_DisconnectTpAddrRequest transport [%d] type [%d] addr [%04x,%02x,%06lx], exclude 0x%08x",
              tp_addr->transport, tp_addr->taddr.type,
              tp_addr->taddr.addr.nap, tp_addr->taddr.addr.uap, tp_addr->taddr.addr.lap,
              exclude);

    handset_service_state_machine_t *sm = handsetService_GetSmForTpBdAddr(tp_addr);

    if (sm)
    {
        HandsetServiceSm_CompleteConnectRequests(sm, handset_service_status_cancelled);
        handsetService_InternalDisconnectReq(sm, &tp_addr->taddr.addr, exclude);
        TaskList_AddTask(&sm->disconnect_list, task);
    }
    else
    {
        HS_LOG("HandsetService_DisconnectTpAddrRequest sm not found");

        MESSAGE_MAKE(cfm, HANDSET_SERVICE_DISCONNECT_CFM_T);
        cfm->addr = tp_addr->taddr.addr;
        cfm->status = handset_service_status_success;
        MessageSend(task, HANDSET_SERVICE_DISCONNECT_CFM, cfm);
    }
}

void HandsetService_DisconnectRequest(Task task, const bdaddr *addr, uint32 exclude)
{
    HS_LOG("HandsetService_DisconnectRequest addr [%04x,%02x,%06lx], exclude 0x%08x",
            addr->nap, addr->uap, addr->lap, exclude);

    tp_bdaddr tp_addr = {0};

    BdaddrTpFromBredrBdaddr(&tp_addr, addr);
    HandsetService_DisconnectTpAddrRequest(task, &tp_addr, exclude);
}

void HandsetService_StopConnect(Task task, const bdaddr *addr)
{
    handset_service_state_machine_t *sm = HandsetService_GetSmForBdAddr(addr);

    HS_LOG("HandsetService_StopConnect task[0x%p] addr [%04x,%02x,%06lx]",
           task, 
           addr->nap, 
           addr->uap, 
           addr->lap);

    if (sm)
    {
        if(sm->connect_stop_task)
        {
            if(sm->connect_stop_task != task)
            {
                DEBUG_LOG_ERROR("HandsetService_StopConnect Called by two Tasks: task %p connect_stop_task %p", task, sm->connect_stop_task);
                Panic();
            }
            else
            {
                DEBUG_LOG_WARN("HandsetService_StopConnect called twice; task %p", task);
            }
        }
        handsetService_InternalConnectStopReq(sm);
        sm->connect_stop_task = task;

        /* Flush any queued internal connect requests */
        HandsetServiceSm_CancelInternalConnectRequests(sm);
    }
    else
    {
        HS_LOG("HandsetService_StopConnect no handset connection to stop");

        MESSAGE_MAKE(cfm, HANDSET_SERVICE_CONNECT_STOP_CFM_T);
        cfm->addr = *addr;
        cfm->status = handset_service_status_disconnected;
        MessageSend(task, HANDSET_SERVICE_CONNECT_STOP_CFM, cfm);
    }
}

void HandsetService_StopReconnect(Task task)
{
    HS_LOG("HandsetService_StopReconnect task[%p]", task);

    HandsetServiceMultipointSm_StopReconnect(task);
}

void HandsetService_ConnectableRequest(Task task)
{
    HS_LOG("HandsetService_ConnectableRequest");
    UNUSED(task);

    HandsetService_SetBleConnectable(TRUE);
    handsetService_ConnectableAllowBredr(TRUE);
    HandsetServiceSm_EnableConnectableIfMaxConnectionsNotActive();
    handsetService_StartBargeInEnableTimerIfMaxConnectionsActive(D_IMMEDIATE);
}

void HandsetService_CancelConnectableRequest(Task task)
{
    HS_LOG("HandsetService_CancelConnectableRequest");
    UNUSED(task);

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    /* Disable LE advertising globally with the new LE advertising manager.
       Individual modules can override this at any time, e.g. peer find role
       when the EB is taken out of the case. */
    HandsetService_SetBleConnectable(FALSE);
#else
    /* HandsetService_SetBleConnectable(FALSE) is not called here because
       the handset service continues BLE advertising even if BREDR is set to be
       not connectable. */
#endif
    handsetService_ConnectableEnableBredr(handset_service_connectable_disable);
    handsetService_ConnectableAllowBredr(FALSE);
    if(HandsetService_IsConnectionBargeInEnabled())
    {
        HandsetService_DisableTruncatedPageScan();
    }
}

void HandsetService_SendConnectedIndNotification(device_t device,
    uint32 profiles_connected)
{
    PanicNull(device);
    MESSAGE_MAKE(ind, HANDSET_SERVICE_CONNECTED_IND_T);
    ind->addr = DeviceProperties_GetBdAddr(device);
    ind->profiles_connected = profiles_connected;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), HANDSET_SERVICE_CONNECTED_IND, ind);
    ContextFramework_NotifyContextUpdate(context_connected_handsets_info);
}

void HandsetService_SendDisconnectedIndNotification(const bdaddr *addr,
    handset_service_status_t status)
{
    MESSAGE_MAKE(ind, HANDSET_SERVICE_DISCONNECTED_IND_T);
    ind->addr = *addr;
    ind->status = status;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), HANDSET_SERVICE_DISCONNECTED_IND, ind);
    ContextFramework_NotifyContextUpdate(context_connected_handsets_info);
}

void HandsetService_SendFirstProfileConnectedIndNotification(device_t device)
{
    PanicNull(device);
    MESSAGE_MAKE(ind, HANDSET_SERVICE_FIRST_PROFILE_CONNECTED_IND_T);
    ind->addr = DeviceProperties_GetBdAddr(device);
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), HANDSET_SERVICE_FIRST_PROFILE_CONNECTED_IND, ind);
}

void HandsetService_SendFirstTransportConnectedIndNotification(tp_bdaddr *tp_addr)
{
    PanicNull(tp_addr);
    MESSAGE_MAKE(ind, HANDSET_SERVICE_FIRST_TRANSPORT_CONNECTED_IND_T);
    memcpy(&ind->tp_addr, tp_addr, sizeof(tp_bdaddr));
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), HANDSET_SERVICE_FIRST_TRANSPORT_CONNECTED_IND, ind);
}

void HandsetService_SendAllTransportsDisconnectedIndNotification(tp_bdaddr *tp_addr)
{
    PanicNull(tp_addr);
    MESSAGE_MAKE(ind, HANDSET_SERVICE_ALL_TRANSPORTS_DISCONNECTED_IND_T);
    memcpy(&ind->tp_addr, tp_addr, sizeof(tp_bdaddr));
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), HANDSET_SERVICE_ALL_TRANSPORTS_DISCONNECTED_IND, ind);
}

bool HandsetService_Connected(device_t device)
{
    handset_service_state_machine_t *sm = handsetService_GetSmForDevice(device);

    if (sm)
    {
        return (HANDSET_SERVICE_STATE_CONNECTED_BREDR == sm->state) || HandsetService_BleIsConnected(&sm->ble_sm);
    }

    return FALSE;
}

bool HandsetService_IsBredrConnected(const bdaddr *addr)
{
    handset_service_state_machine_t *sm = handsetService_GetSmForBredrAddr(addr);
    bool bredr_connected = FALSE;

    if (sm)
    {
        bredr_connected = HandsetServiceSm_IsBredrAclConnected(sm);
    }

    HS_LOG("HandsetService_IsBredrConnected bredr_connected %u", bredr_connected);

    return bredr_connected;
}

bool HandsetService_IsAnyBredrConnected(void)
{
    return (HandsetServiceSm_GetBredrAclConnectionCount() != 0);
}

bool HandsetService_IsAnyBredrHandsetFullyConnected(void)
{
    return (HandsetServiceSm_GetFullyConnectedBredrHandsetCount() != 0);
}

bool HandsetService_IsDisconnecting(void)
{
    handset_service_data_t *hs = HandsetService_Get();
    return hs->disconnect_all_in_progress || HandsetServiceSm_IsDisconnecting();
}

bool HandsetService_IsConnecting(void)
{
    return HandsetServiceMultipointSm_IsReconnectionInProgress() || HandsetServiceSm_IsConnecting();
}

bool HandsetService_IsAnyLeConnected(void)
{
    return (HandsetServiceSm_GetLeAclConnectionCount() != 0);
}

bool HandsetService_IsAnyDeviceConnected(void)
{
    return (HandsetService_IsAnyBredrConnected() || HandsetService_IsAnyLeConnected());
}

bool HandsetService_GetConnectedBredrHandsetTpAddress(tp_bdaddr * tp_addr)
{
    bool bredr_handset = FALSE;
    PanicNull(tp_addr);
    bdaddr addr;
    bredr_handset = handsetService_GetConnectedBredrHandsetAddress(&addr);
    if(bredr_handset)
    {
        BdaddrTpFromBredrBdaddr(tp_addr, &addr);
    }
    return bredr_handset;
}

bool HandsetService_GetConnectedLeHandsetTpAddress(tp_bdaddr *tp_addr)
{
    unsigned le_handset_count = 0;

    PanicNull(tp_addr);
    FOR_EACH_HANDSET_SM(sm)
    {
        if (sm->state != HANDSET_SERVICE_STATE_NULL)
        {
            if (HandsetServiceSm_IsLeAclConnected(sm))
            {
                if (le_handset_count == 0)
                {
                    *tp_addr = HandsetServiceSm_GetLeTpBdaddr(sm);
                }
                le_handset_count++;
            }
        }
    }

    if (le_handset_count > 1)
    {
        /* Additional API needed to allow for multiple LE handsets */
        DEBUG_LOG_WARN("HandsetService_GetConnectedLeHandsetTpAddress More than one LE exists (%d total)",
                        le_handset_count);
    }

    return le_handset_count != 0;
}

void HandsetService_SetBleConnectable(bool connectable)
{
    handset_service_data_t *hs = HandsetService_Get();

    DEBUG_LOG("HandsetService_SetBleConnectable connectable %d le_connectable %d adv_hdl %p",
              connectable, hs->ble_connectable, hs->legacy_advert.handle);

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    hs->ble_connectable = connectable;
    HandsetService_UpdateAdvertising();
    HandsetService_SendLeConnectableIndication(connectable);
#else
    if (connectable == hs->ble_connectable)
    {
        /* We still need to send a HANDSET_SERVICE_LE_CONNECTABLE_IND when the
           requested value is the same as the current value */
        HandsetService_SendLeConnectableIndication(connectable);
    }
    else
    {
        /* Connectable flag has changed so may need to update the advertising
           data to match the new value. */
        hs->ble_connectable = connectable;
        if(   !HandsetService_UpdateLegacyAdvertisingData()
           || !HandsetServiceExtAdv_UpdateAdvertisingData())
        {
            /* Advertisement data was not updated. Possibly device is already connected
               over LE and there is no active advertisement set.
               Send the indication to inform the client. */
            HandsetService_SendLeConnectableIndication(connectable);
        }
    }
#endif
}

unsigned HandsetService_GetNumberOfConnectedBredrHandsets(void)
{
    unsigned num_handsets = HandsetServiceSm_GetBredrAclConnectionCount();
    DEBUG_LOG_FN_ENTRY("HandsetService_GetNumberOfConnectedBredrHandsets handsets=%d", num_handsets);
    return num_handsets;
}

void HandsetService_ReconnectRequest(Task task)
{
    DEBUG_LOG_FN_ENTRY("HandsetService_ReconnectRequest");
    HandsetServiceMultipointSm_ReconnectRequest(task, FALSE);
}

void HandsetService_ReconnectLinkLossRequest(Task task)
{
    DEBUG_LOG_FN_ENTRY("HandsetService_ReconnectLinkLossRequest");
    HandsetServiceMultipointSm_ReconnectRequest(task, TRUE);
}

void HandsetService_DisconnectLruHandsetRequest(Task task)
{
    device_t handset_device = NULL;
    DEBUG_LOG("HandsetService_DisconnectLruHandsetRequest");

    if(Focus_GetDeviceForUiInput(ui_input_disconnect_lru_handset, &handset_device))
    {
        bdaddr handset_addr = DeviceProperties_GetBdAddr(handset_device);

        HandsetService_DisconnectRequest(task, &handset_addr, 0);
    }
}

bool HandsetService_IsBrEdrMultipointEnabled(void)
{
    return (handsetService_BredrAclMaxConnections() == HANDSET_SERVICE_MAX_PERMITTED_BREDR_CONNECTIONS);
}

unsigned HandsetService_GetMaxNumberOfConnectedBredrHandsets(void)
{
    return handsetService_BredrAclMaxConnections();
}

bool HandsetService_IsConnectionBargeInEnabled(void)
{
    return handsetService_IsConnectionBargeInEnabled();
}

void HandsetService_RestartBargeInEnableTimer(uint32 delay)
{
    HS_LOG("HandsetService_RestartBargeInEnableTimer");
    MessageCancelFirst(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_TRUNC_PAGE_SCAN_ENABLE);
    MessageSendLater(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_TRUNC_PAGE_SCAN_ENABLE, NULL, delay);
}

bool HandsetService_IsHandsetInBredrContextPresent(handset_bredr_context_t context)
{
    bool handset_present = FALSE;

    device_t device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_handset_bredr_context, &context, sizeof(handset_bredr_context_t));
    if (device != NULL)
    {
        handset_present = TRUE;
    }

    DEBUG_LOG_VERBOSE("HandsetService_IsHandsetInBredrContextPresent enum:handset_bredr_context_t:%d present=%d", context, handset_present);

    return handset_present;
}

void HandsetService_RegisterPddu(void)
{
    HandsetServicePddu_RegisterPdduInternal();
}

void HandsetService_PairHandset(Task task, bool is_user_initiated)
{
    pairing_params_t *params = PanicUnlessNew(pairing_params_t);

    HandsetService_Get()->pairing_task = task;

    memset(params, 0, sizeof(pairing_params_t));
    params->is_user_initiated = is_user_initiated;
    params->control_page_scan = FALSE;

    if (HandsetServiceSm_MaxLeAclConnectionsReached())
    {
        HandsetService_Get()->pairing_request_pending_lock = TRUE;

        /* If the max number of LE connections has been reached we must
           disconnect the LRU handset because we don't support LE barge-in. */
        HandsetService_DisconnectLruHandsetRequest(HandsetService_GetTask());
    }
    else if (HandsetServiceSm_MaxBredrAclConnectionsReached())
    {
        if(HandsetService_IsConnectionBargeInEnabled())
        {
            HandsetService_EnableTruncatedPageScan();
        }
        else
        {
            HandsetService_Get()->pairing_request_pending_lock = TRUE;
            HandsetService_DisconnectLruHandsetRequest(HandsetService_GetTask());
        }
    }

    /* pend the pairing request if a LRU Handset disconnect request is in progress */
    MessageSendConditionally(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_PAIR_REQ,
                             params, &HandsetService_Get()->pairing_request_pending_lock);
}

void HandsetService_CancelPairHandset(Task task)
{
    HandsetService_Get()->pairing_task = task;

    if (HandsetService_Get()->pairing_request_pending_lock)
    {
        MessageCancelFirst(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_PAIR_REQ);
        HandsetService_Get()->pairing_request_pending_lock = FALSE;
    }
    else
    {
        Pairing_PairStop(HandsetService_GetTask());
    }
}