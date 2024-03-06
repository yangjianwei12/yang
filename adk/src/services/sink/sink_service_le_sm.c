/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    sink_service
    \brief      Sink service LE state machine implementation
*/

#ifdef ENABLE_LE_SINK_SERVICE

/* local logging */
#include "sink_service_logging.h"

/* local includes */
#include "sink_service_protected.h"
#include "sink_service_le_sm.h"
#include "sink_service_config.h"
#include "sink_service_util.h"
#include "gatt_connect.h"
#include "gatt_service_discovery.h"
#include "gatt.h"
#include "pairing.h"

/* framework includes */

#include <bt_device.h>
#include <rssi_pairing.h>
#include <connection_manager.h>
#include <device_properties.h>
#include <profile_manager.h>
#include <device_list.h>
#include <task_list.h>
#include <device_db_serialiser.h>
#include <unexpected_message.h>
#include <le_audio_client.h>

/* system includes */
#include <bdaddr.h>
#include <stdlib.h>
#include <panic.h>

#define SINK_SERVICE_CLIENT_TASKS_LIST_INIT_CAPACITY 1

/*! \brief Cast a Task to a sink_service_state_machine_t.
    This depends on task_data being the first member of sink_service_state_machine_t. */
#define sinkServiceLeSm_GetTaskForSm(_sm)       sinkService_GetTaskForSm(_sm)
#define sinkServiceLeSm_GetTaskForSm(_sm)       sinkService_GetTaskForSm(_sm)
#define sinkServiceLeSm_GetStateForSm(_sm)      sinkService_GetStateForSm(_sm)

static device_t sinkServiceLeSm_CreateDevice(bdaddr addr);

static lea_device_info_t * sinkServiceLeSm_GetLeDeviceInfoByCid(sink_service_state_machine_t *sm, gatt_cid_t cid)
{
    int i;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        if (sm->lea_device[i].gatt_cid == cid)
        {
            return &sm->lea_device[i];
        }
    }

    return NULL;
}

static bool sinkServiceLeSm_UpdateGattCid(sink_service_state_machine_t *sm,
                                          tp_bdaddr tpaddr,
                                          gatt_cid_t cid)
{
    int i;
    bool cid_added = FALSE;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        if (BdaddrIsSame(&sm->lea_device[i].tp_acl_hold_addr.taddr.addr, &tpaddr.taddr.addr))
        {
            sm->lea_device[i].gatt_cid = cid;
            cid_added = TRUE;
        }
    }

    return cid_added;
}

static void sinkServiceLeSm_ClearLeDeviceInfo(sink_service_state_machine_t *sm)
{
    int i;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
       memset(&sm->lea_device[i], 0, sizeof(lea_device_info_t));
       sm->lea_device[i].gatt_cid = INVALID_CID;
    }
}

static bool sinkServiceLeSm_IsLeDevicePresent(sink_service_state_machine_t *sm, tp_bdaddr *tpaddr)
{
    int i;
    bool device_present = FALSE;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        if (BdaddrIsSame(&sm->lea_device[i].tp_acl_hold_addr.taddr.addr, &tpaddr->taddr.addr))
        {
            device_present = TRUE;
        }
    }

    return device_present;
}

static void sinkServiceLeSm_EvaluateAndConnectLeProfiles(sink_service_state_machine_t *sm,
                                                         lea_device_info_t *sink_device_info)
{
    if (sink_device_info->gatt_discovery_completed &&
        sink_device_info->link_encrypted)
    {
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        LeAudioClient_ClientRegister(sinkServiceLeSm_GetTaskForSm(sm));
        LeAudioClient_Connect(sink_device_info->gatt_cid);
#else
        UNUSED(sm);
#endif
    }
}

/*! \brief Callback function to handle GATT connection */
static void sinkServiceLeSm_OnGattConnection(gatt_cid_t cid)
{
    tp_bdaddr tpaddr;
    sink_service_state_machine_t *sm = NULL;
    bool gatt_discovery_needed = FALSE;

    DEBUG_LOG("SinkServiceLeSm_HandleGattConnectInd GATT Connected %d", cid);

    if (GattConnect_GetTpaddrFromConnectionId(cid, &tpaddr))
    {
        sm = SinkServiceLeSm_GetSmFromTpaddr(&tpaddr);

        if (sm == NULL)
        {
            /* GATT has connected.SM is still empty.check if CON_MANAGER_TP_CONNECT_IND
             * is still pending to be delivered.
             */
            FOR_EACH_SINK_SM(le_sm)
            {
                if ((le_sm->state == SINK_SERVICE_STATE_PAIRING || le_sm->state == SINK_SERVICE_STATE_CONNECTING_LE_ACL ||
                     le_sm->state == SINK_SERVICE_STATE_CONNECTING_LE_WHITELIST) &&
                    MessagePendingFirst(sinkServiceLeSm_GetTaskForSm(le_sm), CON_MANAGER_TP_CONNECT_IND, NULL))
                {
                    if (BtDevice_GetDeviceForBdAddr(&tpaddr.taddr.addr) == NULL)
                    {
                        /* If device is not yet available, service discovery might have not triggered
                           upon gatt connect indication. So here we create the device and initiate
                           the service discovery manually */
                        gatt_discovery_needed = TRUE;
                    }

                    PanicFalse(sinkServiceLeSm_AddLeDeviceInfo(le_sm, tpaddr));
                    sinkServiceLeSm_UpdateGattCid(le_sm, tpaddr, cid);

                    if (gatt_discovery_needed)
                    {
                        GattServiceDiscovery_StartServiceDiscovery(cid);
                    }
                    break;
                }
            }
        }
        else
        {
            sinkServiceLeSm_UpdateGattCid(sm, tpaddr, cid);
        }
    }
}

/*! \brief Callback function to handle GATT Link encryption notification */
static void sinkServiceLeSm_OnGattLinkEncrypted(gatt_cid_t cid, bool encrypted)
{
    tp_bdaddr tpaddr;
    lea_device_info_t *sink_device_info;
    sink_service_state_machine_t *sm = NULL;

    DEBUG_LOG_INFO("sinkServiceLeSm_OnGattLinkEncrypted LE Link encrypted %d", encrypted);
 
    if (encrypted && GattConnect_GetTpaddrFromConnectionId(cid, &tpaddr))
    {
        sm = SinkServiceLeSm_GetSmFromTpaddr(&tpaddr);
        sink_device_info = sinkServiceLeSm_GetLeDeviceInfoByAddr(sm, (bdaddr*)&tpaddr.taddr.addr);
        PanicFalse(sink_device_info != NULL && sm != NULL);

        sink_device_info->link_encrypted = TRUE;
        sinkServiceLeSm_EvaluateAndConnectLeProfiles(sm, sink_device_info);
    }
}

/*! \brief Handle security confirmation */
static void sinkServiceLeSm_HandleSecurityCfm(sink_service_state_machine_t *sm, const PAIRING_SECURITY_CFM_T *cfm)
{
    tp_bdaddr tp_addr;

    DEBUG_LOG_INFO("SinkServiceLeSm_HandleSecurityCfm Error Code %d", cfm->hci_status);

    if (cfm->hci_status == HCI_ERROR_KEY_MISSING)
    {
        tp_addr.transport = TRANSPORT_BLE_ACL;
        memcpy(&tp_addr.taddr, &cfm->typed_addr, sizeof(typed_bdaddr));

        /* The remote has lost the keys. Just disconnect the link */
        sm->local_initiated_disconnect = TRUE;
        ConManagerReleaseTpAcl(&tp_addr);
    }

    Pairing_UnregisterForSecurityEvents(sinkServiceLeSm_GetTaskForSm(sm));
}

/*! \brief Callback function to handle GATT Disconnect notification */
static void sinkServiceLeSm_OnGattDisconnect(gatt_cid_t gatt_cid)
{
    DEBUG_LOG("sinkServiceLeSm_OnGattDisconnect Cid : %d", gatt_cid);
}

/*! \brief Callback function to handle GATT Disconnect notification */
static const gatt_connect_observer_callback_t sink_service_le_sm_gatt_callback =
{
    .OnEncryptionChanged = sinkServiceLeSm_OnGattLinkEncrypted,
    .OnDisconnection = sinkServiceLeSm_OnGattDisconnect,
    .OnConnection = sinkServiceLeSm_OnGattConnection
};

/* After entering the disconnected state, ensure sink service triggers the next set of actions */
static void sinkServiceLeSm_EvaluateNextActionForDisconnectedState(sink_service_state_machine_t *sm, bool acl_close_initiated_locally)
{
    /* Check if we need to enter the DISABLED state */
    if (!SinkService_IsEnabled())
    {
        SinkServiceLeSm_ClearInstance(sm);
        sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISABLED);
        SinkService_GetTaskData()->pairing_request_pending = FALSE;

        return;
    }
    /* Check if there is a pairing request pending to enter the PAIRING state. */
    else if (SinkService_GetTaskData()->pairing_request_pending)
    {
        SinkServiceLeSm_ClearInstance(sm);
        SinkService_PairRequest();
        SinkService_GetTaskData()->pairing_request_pending = FALSE;

        return;
    }
#ifdef ENABLE_SOURCE_ACCEPTOR_LIST
    /* If there are paired sink devices and connection is lost/closed by remote, add the device to whitelist */
    else if (sinkServiceUtil_DetermineSinkDevice() && !acl_close_initiated_locally)
    {
        sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_CONNECTING_LE_WHITELIST);
        return;
    }
#else
    UNUSED(acl_close_initiated_locally);
#endif

    /* Clear the entire state machine */
    SinkServiceLeSm_ClearInstance(sm);
}

#ifdef ENABLE_SOURCE_ACCEPTOR_LIST
static bool sinkServiceLeSm_GetTpAddrFromSinkDevice(device_t sink_device, tp_bdaddr *tpaddr)
{
    bool status = FALSE;
    bdaddr addr;

    if (sink_device != NULL && tpaddr != NULL)
    {
        addr = DeviceProperties_GetBdAddr(sink_device);
        tpaddr->transport = TRANSPORT_BLE_ACL;
        tpaddr->taddr.type = TYPED_BDADDR_PUBLIC;
        memcpy(&tpaddr->taddr.addr, &addr, sizeof(bdaddr));
        status = TRUE;
    }

    return status;
}

static void sinkServiceLeSm_AddDeviceToWhitelist(sink_service_state_machine_t *sm, device_t sink_device)
{
    tp_bdaddr tpaddr;

    /* Add this paired device to white list and issue a connect request to whitelist */
    if (sinkServiceLeSm_GetTpAddrFromSinkDevice(sink_device, &tpaddr))
    {
        DEBUG_LOG("sinkServiceLeSm_AddDeviceToWhitelist lap : %d", tpaddr.taddr.addr.lap);

         /* Request low latency for this re-connection setup.*/
         ConManagerRequestDefaultQos(cm_transport_ble, cm_qos_low_latency);

         /* Todo: Iterate through all the paired devices and add into whitelist */
         /* Add the MRU device to the whitelist for now */
         ConManagerSendAddDeviceToLeWhiteListRequest(&tpaddr.taddr);

         /* Initiate a LE-ACL connection.*/
         ConManagerSendAclOpenUseFilterAcceptListRequest();

         /* Subscribe to synergy CM events. Cancelling the connect request placed
          * at any point, the sink service will not receive any TP Connect failed event
          * from CM(The CM does not forwards ACL open request failures to higher layers).
          * So the sink service has to directly subscribe for ACL events from synergy.
          * This below code will be removed when proper whitelisting API's are available.
          */
          CmSetEventMaskReqSend(sinkServiceLeSm_GetTaskForSm(sm),
                                (CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION |
                                 CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE |
                                 CSR_BT_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE |
                                 CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY),
                                 CSR_BT_CM_EVENT_MASK_COND_ALL);
    }
    else
    {
        /* We should not enter here */
        Panic();
    }
}
static void sinkServiceLeSm_AddAllDevicesToWhitelist(sink_service_state_machine_t *sm)
{

    device_t sink_device;

    /* For now just add the MRU device into whitelist */
    sink_device = sinkServiceUtil_DetermineSinkDevice();

    sinkServiceLeSm_AddDeviceToWhitelist(sm, sink_device);
}

#endif /* ENABLE_SOURCE_ACCEPTOR_LIST */

static void sinkServiceLeSm_ReleaseAllLeAcls(sink_service_state_machine_t *sm)
{
    int i;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        /* Release the LE-ACL if there is an ACL */
        if (!BdaddrIsZero(&sm->lea_device[i].tp_acl_hold_addr.taddr.addr) &&
            ConManagerIsTpConnectedOrConnecting(&sm->lea_device[i].tp_acl_hold_addr))
        {
            /* Set the local initiated disconnect flag to TRUE, so we don't add the device to 
             * whitelist later 
             */
            sm->local_initiated_disconnect = TRUE;

            DEBUG_LOG_DEBUG("sinkServiceLeSm_ReleaseAllLeAcls: LAP: %06lx", sm->lea_device[i].tp_acl_hold_addr.taddr.addr.lap);
            ConManagerReleaseTpAcl(&sm->lea_device[i].tp_acl_hold_addr);
        }
    }
}

static device_t sinkServiceLeSm_CreateDevice(bdaddr addr)
{
    device_t sink_device;

    /* Create a device with type SINK */
    sink_device = BtDevice_GetDeviceCreateIfNew(&addr, DEVICE_TYPE_SINK);
    PanicFalse(BtDevice_SetFlags(sink_device, DEVICE_FLAGS_NOT_PAIRED, DEVICE_FLAGS_NOT_PAIRED));
    BtDevice_SetDefaultProperties(sink_device);

    return sink_device;
}

static void sinkServiceLeSm_EnterConnectingLeAcl(sink_service_state_machine_t *sm)
{
    bdaddr addr = DeviceProperties_GetBdAddr(sinkServiceUtil_DetermineSinkDevice());
    MESSAGE_MAKE(cfm, SINK_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE_T);

    cfm->tpaddr.transport = TRANSPORT_BLE_ACL;
    cfm->tpaddr.taddr.type = TYPED_BDADDR_PUBLIC;
    memcpy(&cfm->tpaddr.taddr.addr, &addr, sizeof(bdaddr));
    PanicFalse(sinkServiceLeSm_AddLeDeviceInfo(sm, cfm->tpaddr));

    /* Request low latency during initial connection setup.*/
    ConManagerRequestDefaultQos(cm_transport_ble, cm_qos_low_latency);
    MessageSendConditionally(sinkServiceLeSm_GetTaskForSm(sm), SINK_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE,
                             cfm, ConManagerCreateTpAcl(&cfm->tpaddr));
}

static void sinkServiceLeSm_StateConnectingLeAclHandler(sink_service_state_machine_t * sm, MessageId id, Message message)
{
    switch(id)
    {
        case LE_AUDIO_CLIENT_DISCONNECT_IND:
        {
            sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
        }
        break;

        case CON_MANAGER_TP_CONNECT_IND:
        {
            const CON_MANAGER_TP_CONNECT_IND_T *msg = (CON_MANAGER_TP_CONNECT_IND_T *) message;
            CsrBtTypedAddr addr;

            if (msg->tpaddr.transport != TRANSPORT_BLE_ACL)
            {
                /* Ignore if not LE ACL */
                return;
            }

            DEBUG_LOG_DEBUG("sinkServiceLeSm_StateConnectingLeAclHandler: CON_MANAGER_TP_CONNECT_IND LAP: 0x%06lx",
                             msg->tpaddr.taddr.addr.lap);

            if (!appDeviceTypeIsSink(&msg->tpaddr.taddr.addr) || !ConManagerIsTpAclLocal(&msg->tpaddr))
            {
                DEBUG_LOG_DEBUG("sinkServiceLeSm_StateConnectingLeAclHandler: Ignore connections from non-sink devices");

                /* Ignore connections from non-sink devices (outside of pairing). These
                   are tracked separately by other components (e.g. handset service). */
                Panic();
                return;
            }

            Pairing_RegisterForSecurityEvents(sinkServiceLeSm_GetTaskForSm(sm));
            BdaddrConvertTypedVmToBluestack(&addr, &msg->tpaddr.taddr);

            /* On a reconnection, encrypt the link as a master */
            CsrBtCmSmLeSecurityReqSend(addr,
                                       L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_MASTER_DIRECTED),
                                       0,
                                       DM_SM_SECURITY_ENCRYPTION);

            PanicFalse(sinkServiceLeSm_AddLeDeviceInfo(sm, msg->tpaddr));
        }
        break;

        case SINK_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE:
        {
            DEBUG_LOG_INFO("sinkServiceLeSm_StateConnectingLeAclHandler SINK_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE");

            const SINK_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE_T *msg = (SINK_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE_T *) message;

            if (!ConManagerIsTpConnected(&msg->tpaddr))
            {
                DEBUG_LOG_INFO("sinkServiceLeSm_StateConnectingLeAclHandler: Failed to connect to Lap : 0x%06lx",
                                msg->tpaddr.taddr.addr.lap);
                sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
            }
        }
        break;

        case GATT_SERVICE_DISCOVERY_COMPLETE:
        {
            const GATT_SERVICE_DISCOVERY_COMPLETE_T *msg = (GATT_SERVICE_DISCOVERY_COMPLETE_T *) message;

            sinkServiceLeSm_HandleServiceDiscoveryComplete(msg->cid, sm);
        }
        break;

        case CON_MANAGER_TP_DISCONNECT_IND:
        {
            const CON_MANAGER_TP_DISCONNECT_IND_T *msg = message;

            if (msg->tpaddr.transport != TRANSPORT_BLE_ACL)
            {
                /* Ignore if not LE ACL */
                return;
            }

            DEBUG_LOG_DEBUG("sinkServiceLeSm_StateConnectingLeAclHandler: CON_MANAGER_TP_DISCONNECT_IND for LAP : 0x%06lx",
                            msg->tpaddr.taddr.addr.lap);
            sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
        }
        break;

        case LE_AUDIO_CLIENT_CONNECT_IND:
        {
            const LE_AUDIO_CLIENT_CONNECT_IND_T *msg = message;

            DEBUG_LOG_DEBUG("sinkServiceLeSm_StateConnectingLeAclHandler: LE_AUDIO_CLIENT_CONNECTED status %d total_devices:%d connected_devices:%d",
                             msg->status, msg->total_devices, msg->connected_devices);

            if (msg->status == LE_AUDIO_CLIENT_STATUS_SUCCESS)
            {
                sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_CONNECTED);
            }
            else
            {
                sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
            }
        }
        break;

        case LE_AUDIO_CLIENT_DEVICE_ADDED_IND:
        {
            const LE_AUDIO_CLIENT_DEVICE_ADDED_IND_T *msg = message;

            DEBUG_LOG_DEBUG("sinkServiceLeSm_StateConnectingLeAclHandler: LE_AUDIO_CLIENT_DEVICE_ADDED_IND_T status %d", msg->status);

            if (msg->more_devices_needed)
            {
                /* There are more device to connect to. Try to connect with the second device */
                sinkServiceLeSm_EnterConnectingLeAcl(sm);
            }
        }
        break;

        case PAIRING_SECURITY_CFM:
        {
            sinkServiceLeSm_HandleSecurityCfm(sm, message);
        }
        break;

        default:
        {
            DEBUG_LOG_DEBUG("sinkServiceLeSm_StateConnectingLeAclHandler: Unhandled 0x%d", id);
        }
        break;
    }
}

static void sinkServiceLeSm_ExitConnectingLeAcl(sink_service_state_machine_t *sm)
{
    UNUSED(sm);

    DEBUG_LOG_INFO("sinkServiceLeSm_ExitConnectingLeAcl");
}

static void sinkServiceLeSm_StateConnectedHandler(sink_service_state_machine_t *sm, MessageId id, Message message)
{
    UNUSED(message);

    DEBUG_LOG("sinkServiceLeSm_StateConnectedHandler");

    switch(id)
    {
        case LE_AUDIO_CLIENT_DISCONNECT_IND:
        {
            const LE_AUDIO_CLIENT_DISCONNECT_IND_T *msg = message;

            if (msg->status == LE_AUDIO_CLIENT_STATUS_SUCCESS)
            {
                /* All device(s) are removed. Move to disconnected state */
                sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
                sinkServiceLeSm_ClearLeDeviceInfo(sm);
            }
        }
        break;

        case LE_AUDIO_CLIENT_DEVICE_REMOVED_IND:
        {
            const LE_AUDIO_CLIENT_DEVICE_REMOVED_IND_T *msg = message;
            lea_device_info_t *lea_device;

            /* If only one device got disconnected, move to connected and scanning.
               Note : If more_devices_present is zero, then we will soon be getting
               message LE_AUDIO_CLIENT_DISCONNECT_IND */
            if (msg->status == LE_AUDIO_CLIENT_STATUS_SUCCESS && msg->more_devices_present)
            {
                /* Add this device to whitelist and start scanning */
                lea_device = sinkServiceLeSm_GetLeDeviceInfoByCid(sm, msg->cid);
                PanicNull(lea_device);
                sinkServiceLeSm_AddDeviceToWhitelist(sm, lea_device->sink_device);

                /* Remove this device from sink service device list.*/
                /* sinkServiceLeSm_ClearLeDeviceInfoByCid(sm, msg->cid); */

                /* Move to connected and scanning state */
                sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_CONNECTED_AND_SCANNING);
            }
        }
        break;

        case CON_MANAGER_TP_DISCONNECT_IND:
        {
            lea_device_info_t *sink_device_info;
            const CON_MANAGER_TP_DISCONNECT_IND_T *msg = message;

            if (msg->tpaddr.transport != TRANSPORT_BLE_ACL)
            {
                /* Ignore if not LE ACL */
                return;
            }

            sink_device_info = sinkServiceLeSm_GetLeDeviceInfoByAddr(sm, (bdaddr*)&msg->tpaddr.taddr.addr);

            DEBUG_LOG_INFO("sinkServiceLeSm_StateConnectedHandler: Disconnected from Lap : 0x%06lx", msg->tpaddr.taddr.addr.lap);

            PanicFalse(sink_device_info != NULL);
            (void) LeAudioClient_Disconnect(sink_device_info->gatt_cid, ConManagerIsTpConnected(&msg->tpaddr));
        }
        break;

        default:
        break;
    }
}

static void sinkServiceLeSm_StateDisabledHandler(sink_service_state_machine_t *sm, MessageId id, Message message)
{
    UNUSED(message);
    UNUSED(id);
    UNUSED(sm);
}

static void sinkServiceLeSm_StateDisconnectedHandler(sink_service_state_machine_t *sm, MessageId id, Message message)
{
    UNUSED(message);
    UNUSED(id);
    UNUSED(sm);
}

#ifdef ENABLE_SOURCE_ACCEPTOR_LIST
static void sinkServiceLeSm_StateConnectingWhitelistHandler(sink_service_state_machine_t *sm, MessageId id, Message message)
{
    sink_service_state_machine_t *le_sm;
    CsrBtTypedAddr addr;

    switch (id)
    {
        case CON_MANAGER_TP_CONNECT_IND:
        {
            /* A device got connected when sink service was in connecting whitelist state */
            const CON_MANAGER_TP_CONNECT_IND_T *ind = (const CON_MANAGER_TP_CONNECT_IND_T *) message;

            if (ind->tpaddr.transport != TRANSPORT_BLE_ACL)
            {
                /* Ignore if not LE ACL */
                return;
            }

            DEBUG_LOG_ALWAYS("sinkServiceLeSm_StateConnectingWhitelistHandler enum:TRANSPORT_T:%d type %d [%04x,%02x,%06lx] ",
                   ind->tpaddr.transport,
                   ind->tpaddr.taddr.type,
                   ind->tpaddr.taddr.addr.nap,
                   ind->tpaddr.taddr.addr.uap,
                   ind->tpaddr.taddr.addr.lap);

            if (!appDeviceTypeIsSink(&ind->tpaddr.taddr.addr))
            {
                return;
            }

            /* Use the first instance in the list for this request */
            le_sm = SinkService_GetTaskData()->state_machines;

            PanicFalse(sinkServiceLeSm_AddLeDeviceInfo(le_sm, ind->tpaddr));

            /* On a reconnection, encrypt the link as a master */
            BdaddrConvertTypedVmToBluestack(&addr, &ind->tpaddr.taddr);

            Pairing_RegisterForSecurityEvents(sinkServiceLeSm_GetTaskForSm(sm));

            /* If all good, trigger a request to encrypt the link */
            CsrBtCmSmLeSecurityReqSend(addr,
                                       L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_MASTER_DIRECTED),
                                       0,
                                       DM_SM_SECURITY_ENCRYPTION);

            /* Note:
             * Sink service should stay in the current state itself.When GATT service discovery completes
             * it will get a GATT_SERVICE_DISCOVERY_COMPLETE notification and moves to connecting LE Profiles.
             */
        }
        break;

        case GATT_SERVICE_DISCOVERY_COMPLETE:
        {
            const GATT_SERVICE_DISCOVERY_COMPLETE_T *msg = (GATT_SERVICE_DISCOVERY_COMPLETE_T *) message;

            /* GATT Service discovery completed. Try to connect LE Profiles now */
            sinkServiceLeSm_HandleServiceDiscoveryComplete(msg->cid, sm);
        }
        break;

        case CON_MANAGER_TP_DISCONNECT_IND:
        {
            const CON_MANAGER_TP_DISCONNECT_IND_T *msg = message;

            if (msg->tpaddr.transport != TRANSPORT_BLE_ACL)
            {
                /* Ignore if not LE ACL */
                return;
            }

            DEBUG_LOG_DEBUG("sinkServiceLeSm_StateConnectingWhitelistHandler: CON_MANAGER_TP_DISCONNECT_IND for LAP : 0x%06lx",
                            msg->tpaddr.taddr.addr.lap);

            /* The LE-ACL link got disconnected. Ensure we move to the disconnecting state to re-evaluate what sink service 
             * should do
             */
            sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
        }
        break;

        case LE_AUDIO_CLIENT_CONNECT_IND:
        {
            const LE_AUDIO_CLIENT_CONNECT_IND_T *msg = message;

            DEBUG_LOG_DEBUG("sinkServiceLeSm_StateConnectingWhitelistHandler: LE_AUDIO_CLIENT_CONNECTED status %d total_devices:%d connected_devices:%d",
                            msg->status, msg->total_devices, msg->connected_devices);

            if (msg->status == LE_AUDIO_CLIENT_STATUS_SUCCESS)
            {
                sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_CONNECTED);
            }
            else
            {
                sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
            }
        }
        break;

        /* The ACL connect request which sink service placed to connect with the whitelisted remote device got expired. */
        case CM_PRIM:
        {
            CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

            if (*prim == CSR_BT_CM_LE_EVENT_CONNECTION_IND)
            {
                /* Only handle Connection failures here. */
                const CsrBtCmLeEventConnectionInd *ind = (CsrBtCmLeEventConnectionInd *) message;

                if (ind->connectStatus == CSR_BT_LE_EVENT_CONNECT_FAIL)
                {
                    /* If user initiated fresh pairing, then move to disconnected state and starts the pairing from there */
                    if (SinkService_GetTaskData()->pairing_request_pending)
                    {
                        /* Unsubcribe to synergy CM events and move to disconnected state */
                        CmSetEventMaskReqSend(sinkServiceLeSm_GetTaskForSm(sm),
                                              CSR_BT_CM_EVENT_MASK_SUBSCRIBE_NONE,
                                              CSR_BT_CM_EVENT_MASK_COND_NA);

                        /* Set the locally initiated disconnect flag to TRUE, so we don't re-enter
                         * into whitelisting again.
                         */
                        sm->local_initiated_disconnect = TRUE;
                        sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
                    }
                    else
                    {
                        /* Keep on trying to connect with whitelisted devices */
                        ConManagerSendAclOpenUseFilterAcceptListRequest();
                    }
                }
            }
        }
        break;

        case PAIRING_SECURITY_CFM:
        {
            sinkServiceLeSm_HandleSecurityCfm(sm, message);
        }
        break;

        default:
        break;
    }
}
#endif

/* State exit functions */
static void sinkServiceLeSm_ExitDisconnected(sink_service_state_machine_t *sm)
{
    DEBUG_LOG_DEBUG("sinkServiceLeSm_ExitDisconnected");

    ConManagerRegisterTpConnectionsObserver(cm_transport_ble, sinkServiceLeSm_GetTaskForSm(sm));
    GattServiceDiscovery_ClientRegister(sinkServiceLeSm_GetTaskForSm(sm));
}

static void sinkServiceLeSm_ExitConnected(sink_service_state_machine_t *sm)
{
    UNUSED(sm);

    DEBUG_LOG_DEBUG("sinkServiceLeSm_ExitConnected");
}

static void sinkServiceLeSm_ExitDisabled(sink_service_state_machine_t *sm)
{
    UNUSED(sm);

    DEBUG_LOG_DEBUG("sinkServiceLeSm_ExitDisableds");
}

/* State enter functions */
static void sinkServiceLeSm_EnterDisconnected(sink_service_state_machine_t *sm)
{
    DEBUG_LOG_DEBUG("sinkServiceLeSm_EnterDisconnected");

    /* Release all the LE-ACL's */
    sinkServiceLeSm_ReleaseAllLeAcls(sm);

    if (sm->lea_device[0].sink_device != NULL)
    {
        SinkService_SendDisconnectedCfm(sm->lea_device[0].sink_device);

        if (Device_IsPropertySet(sm->lea_device[0].sink_device, device_property_audio_source))
        {
            /* Remove the audio and voice sources */
            DeviceProperties_RemoveAudioSource(sm->lea_device[0].sink_device);
            DeviceProperties_RemoveVoiceSource(sm->lea_device[0].sink_device);
        }
        sinkServiceLeSm_DeleteDeviceIfNotPaired(sm);
    }

    /* Sink service is in disconnected state. Evaluate the following conditions:
     * 1. If user has initiated fresh pairing, ensure sink service kicks in RSSI pairing. (OR)
     * 2. If Link is lost because of OETC, ensure sink service adds the MRU device to whitelist. (OR)
     * 3. If Sink service is disabled, enter disabled state.
     * 4. If none of the above are true, stay in disconnected state.
     */
     sinkServiceLeSm_EvaluateNextActionForDisconnectedState(sm, sm->local_initiated_disconnect);
}

static void sinkServiceLeSm_EnterConnected(sink_service_state_machine_t *sm)
{
    int i;

    DEBUG_LOG_INFO("sinkServiceLeSm_EnterConnected");

    /* ToDo: sm is connected, release the ACL(why we need to release the LE-ACL?) */
    /* sinkServiceLeSm_ReleaseAcl(sm); */
    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        /* For now send the connect confirmaton for the first device(for Split-Cig). For CSIP
         * we need to determine which device needs to be propagated to USB or a change in logic
         * is needed
         */
        SinkService_SendConnectedCfm(sm->lea_device[i].sink_device, SINK_SERVICE_TRANSPORT_LE, sink_service_status_success);

        /* Set both voice and audio source as unicast */
        DeviceProperties_SetAudioSource(sm->lea_device[i].sink_device, audio_source_le_audio_unicast_sender);
        DeviceProperties_SetVoiceSource(sm->lea_device[i].sink_device, voice_source_le_audio_unicast_1);
        break;
    }
}

static void sinkServiceLeSm_EnterDisabled(sink_service_state_machine_t *sm)
{
    UNUSED(sm);

    DEBUG_LOG_INFO("sinkServiceLeSm_EnterDisabled");
}

#ifdef ENABLE_SOURCE_ACCEPTOR_LIST
static void sinkServiceLeSm_ExitConnectingWhitelist(sink_service_state_machine_t *sm)
{
    UNUSED(sm);

    DEBUG_LOG_INFO("sinkServiceLeSm_ExitConnectingWhitelist");

    /* Remove all the devices from LE acceptor list */
    SinkServiceLeSm_RemoveAllDevicesFromWhitelist();
}

static void sinkServiceLeSm_EnterConnectingWhitelist(sink_service_state_machine_t *sm)
{
    /* Add paired devices to LE Acceptor list */
    sinkServiceLeSm_AddAllDevicesToWhitelist(sm);
    SinkServiceLeSm_ClearInstance(sm);
    sm->state = SINK_SERVICE_STATE_CONNECTING_LE_WHITELIST;
}

void SinkServiceLeSm_RemoveAllDevicesFromWhitelist(void)
{
    /* Clear the entire whitelist devices */
    ConManagerSendClearAllDevicesFromLeWhiteListRequest();
}

#endif /* ENABLE_SOURCE_ACCEPTOR_LIST */

void sinkServiceLeSm_SetState(sink_service_state_machine_t *sm, sink_service_state_t state)
{
    sink_service_state_t old_state = sm->state;

    /* It is not valid to re-enter the same state */
    if (old_state == state)
    {
        return;
    }

    DEBUG_LOG_INFO("SinkService: enum:sink_service_state_t:%d -> enum:sink_service_state_t:%d", old_state, state);

    /* Handle state exit functions */
    switch (old_state)
    {
        case SINK_SERVICE_STATE_DISCONNECTED:
            sinkServiceLeSm_ExitDisconnected(sm);
        break;

        case SINK_SERVICE_STATE_CONNECTING_LE_ACL:
            sinkServiceLeSm_ExitConnectingLeAcl(sm);
        break;

        case SINK_SERVICE_STATE_CONNECTED:
            sinkServiceLeSm_ExitConnected(sm);
        break;

        case SINK_SERVICE_STATE_DISABLED:
            sinkServiceLeSm_ExitDisabled(sm);
        break;

#ifdef ENABLE_SOURCE_ACCEPTOR_LIST
        case SINK_SERVICE_STATE_CONNECTING_LE_WHITELIST:
            sinkServiceLeSm_ExitConnectingWhitelist(sm);
        break;
#endif

        default:
        break;
    }

    /* Set new state */
    sm->state = state;

    /* Handle state entry functions */
    switch (sm->state)
    {
        case SINK_SERVICE_STATE_DISCONNECTED:
            sinkServiceLeSm_EnterDisconnected(sm);
        break;

        case SINK_SERVICE_STATE_CONNECTING_LE_ACL:
            sinkServiceLeSm_EnterConnectingLeAcl(sm);
        break;

        case SINK_SERVICE_STATE_CONNECTED:
            sinkServiceLeSm_EnterConnected(sm);
        break;

        case SINK_SERVICE_STATE_DISABLED:
            sinkServiceLeSm_EnterDisabled(sm);
        break;

#ifdef ENABLE_SOURCE_ACCEPTOR_LIST
        case SINK_SERVICE_STATE_CONNECTING_LE_WHITELIST:
            sinkServiceLeSm_EnterConnectingWhitelist(sm);
        break;
#endif

        default:
        break;
    }

    SinkService_UpdateUi();
}

void SinkServiceLeSm_HandleMessage(sink_service_state_machine_t *sm, MessageId id, Message message)
{
    switch (sm->state)
    {
        case SINK_SERVICE_STATE_CONNECTING_LE_ACL:
            sinkServiceLeSm_StateConnectingLeAclHandler(sm, id, message);
        break;

        case SINK_SERVICE_STATE_CONNECTED:
        case SINK_SERVICE_STATE_CONNECTED_AND_SCANNING:
            sinkServiceLeSm_StateConnectedHandler(sm, id, message);
        break;

        case SINK_SERVICE_STATE_DISABLED:
            sinkServiceLeSm_StateDisabledHandler(sm, id, message);
        break;

        case SINK_SERVICE_STATE_DISCONNECTED:
            sinkServiceLeSm_StateDisconnectedHandler(sm, id, message);
        break;

#ifdef ENABLE_SOURCE_ACCEPTOR_LIST
        case SINK_SERVICE_STATE_CONNECTING_LE_WHITELIST:
            sinkServiceLeSm_StateConnectingWhitelistHandler(sm, id, message);
        break;
#endif
        default:
            UnexpectedMessage_HandleMessage(id);
        break;
    }
}

/* Public functions */
/* Handle GATT Service discovery complete message */
void sinkServiceLeSm_HandleServiceDiscoveryComplete(gatt_cid_t cid, sink_service_state_machine_t *sm)
{
    int i;

    DEBUG_LOG_DEBUG("sinkServiceLeSm_HandleServiceDiscoveryComplete %d", cid);

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        if (sm->lea_device[i].gatt_cid == cid)
        {
            sm->lea_device[i].gatt_discovery_completed = TRUE;
            sinkServiceLeSm_EvaluateAndConnectLeProfiles(sm, &sm->lea_device[i]);
            return;
        }
    }
}

lea_device_info_t * sinkServiceLeSm_GetLeDeviceInfoByAddr(sink_service_state_machine_t *sm, bdaddr *addr)
{
    int i;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        if (BdaddrIsSame(&sm->lea_device[i].tp_acl_hold_addr.taddr.addr, addr))
        {
            return &sm->lea_device[i];
        }
    }

    return NULL;
}

bool sinkServiceLeSm_IsAnyLeDeviceConnected(sink_service_state_machine_t *sm)
{
    int i;
    bool device_connected = FALSE;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        if (sm->lea_device[i].gatt_cid != INVALID_CID &&
            sm->lea_device[i].link_encrypted &&
            sm->lea_device[i].gatt_discovery_completed)
        {
            device_connected = TRUE;
            break;
        }
    }

    return device_connected;
}

bool sinkServiceLeSm_AddLeDeviceInfo(sink_service_state_machine_t *sm, tp_bdaddr tp_addr)
{
    int i;
    bool device_added = FALSE;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        if (BdaddrIsSame(&sm->lea_device[i].tp_acl_hold_addr.taddr.addr, &tp_addr.taddr.addr))
        {
            device_added = TRUE;
            break;
        }

        if (BdaddrIsZero(&sm->lea_device[i].tp_acl_hold_addr.taddr.addr))
        {
            sm->lea_device[i].tp_acl_hold_addr = tp_addr;
            sm->lea_device[i].sink_device = BtDevice_GetDeviceForBdAddr(&tp_addr.taddr.addr);

            /* Create the device only if device doesn't exists, otherwise we will end up on overwriting it. */
            if (sm->lea_device[i].sink_device == NULL)
            {
                sm->lea_device[i].sink_device = sinkServiceLeSm_CreateDevice(tp_addr.taddr.addr);
            }

            device_added = TRUE;
            break;
        }
    }

    return device_added;
}

void sinkServiceLeSm_ClearLeDeviceInfoByAddr(sink_service_state_machine_t *sm, bdaddr *addr)
{
    int i;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        if (BdaddrIsSame(&sm->lea_device[i].tp_acl_hold_addr.taddr.addr, addr))
        {
            memset(&sm->lea_device[i], 0, sizeof(lea_device_info_t));
            return;
        }
    }
}

bool sinkServiceLeSm_DeviceIsPaired(device_t device)
{
    uint16 flags = DEVICE_FLAGS_NO_FLAGS;
    bdaddr device_addr;

    PanicNull(device);
    device_addr = DeviceProperties_GetBdAddr(device);
    appDeviceGetFlags(&device_addr, &flags);

    return (flags & DEVICE_FLAGS_NOT_PAIRED) ? FALSE : TRUE;
}

/* Iterate through all the LE devices and delete the device if not paired */
void sinkServiceLeSm_DeleteDeviceIfNotPaired(sink_service_state_machine_t *sm)
{
    int i;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        if (!sm->lea_device[i].sink_device)
        {
            continue;
        }

        if (!sinkServiceLeSm_DeviceIsPaired(sm->lea_device[i].sink_device))
        {
            bdaddr device_addr = DeviceProperties_GetBdAddr(sm->lea_device[i].sink_device);
            appDeviceDelete(&device_addr);
            sm->lea_device[i].sink_device = NULL;
        }
    }
}

sink_service_state_machine_t *SinkServiceLeSm_CreateSm(device_t device)
{
    sink_service_state_machine_t *new_sm = NULL;
    UNUSED(device);

    FOR_EACH_SINK_SM(sm)
    {
        if (sm->state == SINK_SERVICE_STATE_DISCONNECTED)
        {
            new_sm = sm;
            SinkServiceLeSm_ClearInstance(new_sm);
            break;
        }
    }

    return new_sm;
}

sink_service_state_machine_t* SinkServiceLeSm_GetSmForDevice(device_t device)
{
    int i;

    FOR_EACH_SINK_SM(sm)
    {
        if (sm->state > SINK_SERVICE_STATE_DISCONNECTED)
        {
            for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
            {
                if (sm->lea_device[i].sink_device == device)
                {
                    return sm;
                }
            }
        }
    }

    return NULL;
}

/* Get the sink service state machine from TP Address */
sink_service_state_machine_t* SinkServiceLeSm_GetSmFromTpaddr(tp_bdaddr *tpaddr)
{
    sink_service_state_machine_t *sm_match = NULL;

    DEBUG_LOG_VERBOSE("sinkServiceLeSm_GetSmFromTpaddr Searching for addr [%04x,%02x,%06lx]",
                       tpaddr->taddr.addr.nap,
                       tpaddr->taddr.addr.uap,
                       tpaddr->taddr.addr.lap);

    FOR_EACH_SINK_SM(sm)
    {
        if (sm->state > SINK_SERVICE_STATE_DISCONNECTED &&
            sinkServiceLeSm_IsLeDevicePresent(sm, tpaddr))
        {
            sm_match = sm;
            break;
        }
    }

    return sm_match;
}

void SinkServiceLeSm_ClearInstance(sink_service_state_machine_t *sm)
{
    int i;
    PanicNull(sm);

    memset(sm, 0, sizeof(*sm));
    sm->state = SINK_SERVICE_STATE_DISCONNECTED;
    sm->task_data.handler = sinkService_MainMessageHandler;

    for (i=0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        sm->lea_device[i].gatt_cid = INVALID_CID;
    }
}

bool SinkServiceLeSm_ConnectRequest(sink_service_state_machine_t *sm)
{
    DEBUG_LOG_INFO("SinkServiceLeSm_ConnectRequest Connecting to existing device...");

    sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_CONNECTING_LE_ACL);

    return TRUE;
}

bool SinkServiceLeSm_DisconnectRequest(sink_service_state_machine_t *sm)
{
    bool status = FALSE;

    DEBUG_LOG_INFO("SinkServiceLeSm_DisconnectRequest");

    if (sinkServiceLeSm_GetStateForSm(sm) > SINK_SERVICE_STATE_DISCONNECTED)
    {
        /* cid is given as zero to disconnect all the devices in the group */
        if (!LeAudioClient_Disconnect(0, TRUE))
        {
            sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
        }
    }

    return status;
}

/*! \brief Count the number of active LE sink state machines */
unsigned SinkServiceLeSm_GetLeAclConnectionCount(void)
{
    unsigned active_sm_count = 0;
    bdaddr device_address;
    int i;

    FOR_EACH_SINK_SM(sm)
    {
        if (sm->state > SINK_SERVICE_STATE_DISCONNECTED)
        {
            for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
            {
                if (sm->lea_device[i].sink_device != NULL)
                {
                    device_address = DeviceProperties_GetBdAddr(sm->lea_device[i].sink_device);
                }

                if (ConManagerIsConnected(&device_address))
                {
                    active_sm_count++;
                }
            }
        }
    }

    DEBUG_LOG_VERBOSE("SinkServiceLeSm_GetLeAclConnectionCount %u", active_sm_count);

    return active_sm_count;
}

bool SinkServiceLeSm_MaxLeAclConnectionsReached(void)
{
    unsigned num_le_connections = SinkServiceLeSm_GetLeAclConnectionCount();
    unsigned max_le_connections = sinkService_LeAclMaxConnections();

    DEBUG_LOG_DEBUG("SinkServiceLeSm_MaxLeAclConnectionsReached  %u of %u LE connections", num_le_connections, max_le_connections);

    return num_le_connections >= max_le_connections;
}

void SinkServiceLeSm_DisableAll(void)
{
    FOR_EACH_SINK_SM(sm)
    {
        if (sm->state == SINK_SERVICE_STATE_DISCONNECTED)
        {
            sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISABLED);
        }
    }
}

void SinkServiceLeSm_EnableAll(void)
{
    FOR_EACH_SINK_SM(sm)
    {
        if (sm->state == SINK_SERVICE_STATE_DISABLED)
        {
            sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
        }
    }
}

void SinkServiceLeSm_RegisterForGattNotifications(void)
{
    GattConnect_RegisterObserver(&sink_service_le_sm_gatt_callback);
}

#endif /* ENABLE_LE_SINK_SERVICE */

