/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
    All Rights Reserved.
    Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup gatt_service_discovery_synergy
    \brief   Gatt Service Discovery implementation.
*/

#include <logging.h>
#include <panic.h>

#include "gatt_service_discovery.h"
#include "gatt_service_discovery_context.h"
#include "gatt_connect.h"
#include "synergy.h"
#include "gatt.h"
#include "device.h"
#include "bt_device.h"
#include "device_properties.h"
#include "device_db_serialiser.h"
#include "pairing.h"

gatt_service_discovery_task_data_t gsd_taskdata;

typedef struct
{
    const client_id_t** clients;
    const GattSdSrvcId* service_id;
    uint8               size;
    uint8*              num_service_instances;
}gatt_service_discovery_legacy_client_priority_list_t;

/*! Legacy clients registered with gatt service discovery module */
static gatt_service_discovery_legacy_client_priority_list_t client_priority_list = {NULL, NULL, 0, NULL};

/*! \brief Function to find the handles of registered legacy clients */
static void gattServiceDiscovery_ProcessClientPriorityList(gatt_cid_t cid);

static bool gattServiceDiscovery_DestroyClients(gatt_cid_t cid);
/*! \brief Function to register legacy clients for service discovery */
extern void gattServiceDiscovery_InitLegacyClientsList(const client_id_t* gatt_client_prioritised_id[],
                                                       const GattSdSrvcId service_id[],
                                                       uint8 num_elements);

/*! \brief Handler to receive/process notifications from Gatt Service Discovery library */
static void gattServiceDisovery_HandleMessage(Task task, MessageId id, Message message);

/*! \brief Check if GATT service discovery flag is set or not for the handset device
    \param cid Connection ID

    \return TRUE if gatt service discovery is completed, FALSE otherwise
*/
bool GattServiceDisovery_IsServiceDiscoveryCompleted(gatt_cid_t cid)
{
    uint8 gsd_completed_flag = 0;
    device_t device;

    device = GattConnect_GetBtLeDevice(cid);

    if (device != NULL)
    {
        Device_GetPropertyU8(device, device_property_gatt_service_discovery, &gsd_completed_flag);
    }
    DEBUG_LOG("GattServiceDisovery_IsServiceDiscoveryCompleted device 0x%p, completed %d", device, gsd_completed_flag);

    return (gsd_completed_flag != 0);
}

/*! \brief Set GATT service discovery complete flag for the device.

    \param cid Connection ID

    \param gsd_completed_flag TRUE/FALSE depending on service discovery completed
*/
static void gattServiceDisovery_SetServiceDiscoveryCompleteFlag(gatt_cid_t cid, bool gsd_completed_flag)
{
    device_t device;

    device = GattConnect_GetBtLeDevice(cid);
    if (device != NULL)
    {
        DEBUG_LOG("gattServiceDisovery_SetServiceDiscoveryCompleteFlag device 0x%p, completed %d",
                   device, gsd_completed_flag);
        Device_SetPropertyU8(device, device_property_gatt_service_discovery, gsd_completed_flag);
    }
    else
    {
        DEBUG_LOG("gattServiceDisovery_SetServiceDiscoveryCompleteFlag device not exists");
    }
}

/*! \brief Initiate service discovery on given cid */
static void gattServiceDiscovery_InitiateDiscovery(gatt_cid_t cid)
{
    GattSdSrvcId services_to_discover = GATT_SD_INVALID_SRVC;
#ifdef GATT_SERVICE_DISCOVER_ALWAYS
    bool services_discovery_completed = FALSE;
#else
    bool services_discovery_completed = GattServiceDisovery_IsServiceDiscoveryCompleted(cid);
#endif /* GATT_SERVICE_DISCOVER_ALWAYS */

    /* Check if service discovery already completed for this device */
    if (services_discovery_completed)
    {
        /* Services already discovered, still we need to initiate service discovery for
         * legacy clients if they exists. */
        services_to_discover = gsd_taskdata.services_with_handle_to_discover;
    }
    else
    {
        /* Service discovery is not completed before, discover all the services */
        services_to_discover = gsd_taskdata.services_to_discover;
    }

    if (services_to_discover != GATT_SD_INVALID_SRVC)
    {
        DEBUG_LOG("gattServiceDiscovery_InitiateDiscovery Start discovering services 0x%08x", services_to_discover);
        gsd_taskdata.cid_in_discovery = cid;
        GattServiceDiscoveryRegisterSupportedServices(TrapToOxygenTask((Task)&gsd_taskdata.task_data),
                                                      services_to_discover,
                                                      TRUE);
    }
    else
    {
        /* Send a discovery complete message to all clients as service discovery already
         * completed and no more services to discover */
        if (services_discovery_completed)
        {
            MESSAGE_MAKE(ind, GATT_SERVICE_DISCOVERY_COMPLETE_T);
            ind->cid = cid;
            TaskList_MessageSend(gsd_taskdata.client_tasks, GATT_SERVICE_DISCOVERY_COMPLETE, ind);
        }
    }
}

/*! \brief Initiate discovery if there is pending service discovery */
static void gattServiceDiscovery_InitiatePendingDiscoveryIfAny(void)
{
    /* See if there is any pending service discovery */
    if (gsd_taskdata.cid_pending_to_discover != INVALID_CID)
    {
        /* Initiate the pending service discovery */
        gattServiceDiscovery_InitiateDiscovery(gsd_taskdata.cid_pending_to_discover);
        gsd_taskdata.cid_pending_to_discover = INVALID_CID;
    }
}

/*! \brief Callback function to initiate GATT discovery upon receiving a GATT Connect notification */
static void gattServiceDiscovery_AppDiscoveryGattConnect(gatt_cid_t cid)
{
    device_t device;

    DEBUG_LOG("gattServiceDiscovery_AppDiscoveryGattConnect: cid=0x%04X", cid);
    device = GattConnect_GetBtLeDevice(cid);

    if (device == NULL)
    {
        return;
    }

    /* Check if the gatt connection is from a handset or a sink device */
    if (BtDevice_IsDeviceHandsetOrLeHandset(device) || BtDevice_GetDeviceType(device) == DEVICE_TYPE_SINK)
    {
        if (gsd_taskdata.cid_in_discovery == INVALID_CID)
        {
            gattServiceDiscovery_InitiateDiscovery(cid);
        }
        else
        {
            /* Service discovery already in progress */
            if (gsd_taskdata.cid_in_discovery != cid)
            {
                /* Keep the cid for discovering later */
                gsd_taskdata.cid_pending_to_discover = cid;
            }
        }
    }
}

/*! \brief Callback function to stop GATT discovery upon receiving a GATT Disconnect notification */
static void gattServiceDiscovery_AppDiscoveryGattDisconnect(gatt_cid_t cid)
{
    DEBUG_LOG("gattServiceDiscovery_AppDiscoveryGattDisconnect: cid=0x%04X", cid);

    if (gsd_taskdata.cid_in_discovery == cid)
    {
        GattServiceDiscoveryStop(TrapToOxygenTask((Task)&gsd_taskdata.task_data), cid);
    }
    else if(gsd_taskdata.cid_pending_to_discover == cid)
    {
        /* Clear pending discovery cid as disconnected */
        gsd_taskdata.cid_pending_to_discover = INVALID_CID;
    }

    if (gsd_taskdata.services_with_handle_to_discover)
    {
        gattServiceDiscovery_DestroyClients(cid);
    }
}

static const gatt_connect_observer_callback_t gatt_observer_callback =
{
    .OnConnection = gattServiceDiscovery_AppDiscoveryGattConnect,
    .OnDisconnection = gattServiceDiscovery_AppDiscoveryGattDisconnect,
};

/*! \brief Check if any legacy client handles discover is pending. */
static inline bool gattServiceDiscovery_IsLegacyServiceHandlesDiscoveryPending(void)
{
    bool stop_discovery_requested = GattClient_GetDiscoveryStopRequest(gsd_taskdata.legacy_services_discovery_state.current_client_id);
    bool more_clients_to_process = (stop_discovery_requested == FALSE) &&
                                   ((gsd_taskdata.legacy_services_discovery_state.current_service_index + 1) < client_priority_list.size);

    /* Ok to continue if we're NOT in the middle of a multiple service find AND there are more clients to process */
    return more_clients_to_process;
}

static void gattServiceDiscovery_ProcessClientPriorityList(gatt_cid_t cid)
{
    client_id_t client_id;
    GattSdSrvcId service_id;

    client_id = *(client_priority_list.clients[gsd_taskdata.legacy_services_discovery_state.current_service_index]);

    /*! Update the discovery state machine with this client id */
    gsd_taskdata.legacy_services_discovery_state.current_client_id = client_id;

    /*! Retrieve the service that needs to be discovered for this client */
    service_id = client_priority_list.service_id[gsd_taskdata.legacy_services_discovery_state.current_service_index];

    DEBUG_LOG("gattServiceDiscovery_ProcessClientPriorityList: client_id : %x srvcid=%x",client_id, service_id);

    GattServiceDiscoveryFindServiceRange(TrapToOxygenTask((Task)&gsd_taskdata.task_data),
                                         cid, service_id);
}

static void gattServiceDiscovery_HandleServiceRange(const GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *message)
{
    DEBUG_LOG("gattServiceDiscovery_HandleServiceRange cid: 0x%x, result: %d, Found: %d",
               message->cid, message->result, message->srvcInfoCount);

    if (message->result == GATT_SD_RESULT_SUCCESS && message->srvcInfoCount != 0)
    {
        /*! Retrieve the service that needs to be discovered for this client */
        GattSdSrvcId service_id = client_priority_list.service_id[gsd_taskdata.legacy_services_discovery_state.current_service_index];

        if (message->srvcInfoCount == 1 && (message->srvcInfo->srvcId & service_id))
        {
            DEBUG_LOG_INFO("gattServiceDiscovery_HandleServiceRange GattClient_AttachClient srvcid=%x s:%x e:%x",
                           message->srvcInfo->srvcId, message->srvcInfo->startHandle, message->srvcInfo->endHandle);
            /* Attach client to the discovered service */
            client_priority_list.num_service_instances[gsd_taskdata.legacy_services_discovery_state.current_service_index]++;
            GattClient_AttachClient(gsd_taskdata.legacy_services_discovery_state.current_client_id, message->cid,
                                    message->srvcInfo->startHandle, message->srvcInfo->endHandle);
        }

        if (gattServiceDiscovery_IsLegacyServiceHandlesDiscoveryPending())
        {
            /* Continue to iterate over client list */
            gsd_taskdata.legacy_services_discovery_state.current_service_index++;
            gattServiceDiscovery_ProcessClientPriorityList(message->cid);
        }
        else
        {
            /* All client handles are discovered. */
            gsd_taskdata.gsd_status = gsd_in_idle;

            /* See if there is any pending service discovery */
            gattServiceDiscovery_InitiatePendingDiscoveryIfAny();
        }
        pfree(message->srvcInfo);
    }
}

/*! Function to destroy the legacy GATT clients.

    \param cid Connection in which the Service discovery should start.

    \return TRUE if all the clients destroyed successfully, FALSE otherwise.
*/
static bool gattServiceDiscovery_DestroyClients(gatt_cid_t cid)
{
    PanicNull(client_priority_list.clients);
    uint8 client_index = 0;

    if (gsd_taskdata.gsd_status != gsd_in_progress)
    {
        /*! Iterate through all the client list */
        for (client_index = 0; client_index < client_priority_list.size; client_index++)
        {
            client_id_t client_id = *client_priority_list.clients[client_index];
            uint8 service_instance = 0;

            for (service_instance = 0; service_instance < client_priority_list.num_service_instances[client_index]; service_instance++)
            {
                DEBUG_LOG("gattServiceDiscovery_DestroyClients detach client");
                /* Each service may have multiple service instances */
                GattClient_DetachClient(client_id, cid);
            }
        }

        gsd_taskdata.legacy_services_discovery_state.current_service_index = 0;
    }

    return (client_index == client_priority_list.size);
}

static void gattServiceDisovery_HandleRegisterMessage(const GATT_SERVICE_DISCOVERY_REGISTER_SUPPORTED_SERVICES_CFM_T *message)
{
    gatt_cid_t cid_to_discover = gsd_taskdata.cid_in_discovery;
    DEBUG_LOG("gattServiceDisovery_HandleRegisterMessage Status = %d", message->result);

    if ((message->result == GATT_SD_RESULT_SUCCESS) && (cid_to_discover != INVALID_CID))
    {
        gsd_taskdata.gsd_status = gsd_in_progress;
        GattServiceDiscoveryStart(TrapToOxygenTask((Task)&gsd_taskdata.task_data), cid_to_discover);
    }
}

/*! \brief Sends discovery complete notification to clients after a successful GATT service discovery */
static void gattServiceDiscovery_HandleDiscoveryStartMessage(const GATT_SERVICE_DISCOVERY_START_CFM_T *message)
{
    bool service_discovery_completed = FALSE;
    if (message->result == GATT_SD_RESULT_INPROGRESS)
    {
        DEBUG_LOG("gattServiceDiscovery_HandleDiscoveryStartMessage GATT Service Discovery In Progress");
    }
    else if (message->result == GATT_SD_RESULT_SUCCESS)
    {
        DEBUG_LOG("gattServiceDiscovery_HandleDiscoveryStartMessage GATT Service Discovery Completed");

        if (gsd_taskdata.cid_in_discovery == message->cid)
        {
            gsd_taskdata.cid_in_discovery = INVALID_CID;

            /* Send a discovery complete message to all the registered clients */
            MESSAGE_MAKE(ind, GATT_SERVICE_DISCOVERY_COMPLETE_T);
            ind->cid = message->cid;
            TaskList_MessageSend(gsd_taskdata.client_tasks, GATT_SERVICE_DISCOVERY_COMPLETE, ind);

            /* Set the gatt discovery flag to TRUE to indicate that services
             * for the device is discovered.
             */
            gattServiceDisovery_SetServiceDiscoveryCompleteFlag(message->cid, TRUE);

            /* If any legacy client services with handles to discover exists */
            if (gsd_taskdata.services_with_handle_to_discover)
            {
                if (gsd_taskdata.gsd_status == gsd_in_progress)
                {
                    /* Start processing the priority clients one by one */
                    gattServiceDiscovery_ProcessClientPriorityList(message->cid);
                }
            }
            else
            {
                /* No service handles needs to be discovered */
                service_discovery_completed = TRUE;
                gsd_taskdata.gsd_status = gsd_in_idle;
            }
        }
    }
    else
    {
        DEBUG_LOG("gattServiceDiscovery_HandleDiscoveryStartMessage Error Code cid=0x%04X", message->result);
        gsd_taskdata.cid_in_discovery = INVALID_CID;
        service_discovery_completed = TRUE;
    }

    /* If current service discovery is completed and there is discovery pending */
    if (service_discovery_completed)
    {
        /* See if there is any pending service discovery */
        gattServiceDiscovery_InitiatePendingDiscoveryIfAny();
    }
}

/*! \brief GATT Service discovery stopped.Cleanup if the discovery is in progress for the connection */
static void gattServiceDiscovery_HandleDiscoveryStopMessage(const GATT_SERVICE_DISCOVERY_STOP_CFM_T *message)
{
    if (message->result == GATT_SD_RESULT_SUCCESS)
    {
        DEBUG_LOG("gattServiceDiscovery_HandleDiscoveryStopMessage GATT Service Discovery Stopped");

        if (gsd_taskdata.cid_in_discovery == message->cid)
        {
            gsd_taskdata.cid_in_discovery = INVALID_CID;

            /* See if there is any pending service discovery */
            gattServiceDiscovery_InitiatePendingDiscoveryIfAny();
        }
    }
}

/*! \brief Handler to handle GATT Service Discovery related primitives */
static void gattServiceDisovery_HandleGattPrim(Message message)
{
    switch (*(CsrBtCmPrim *)message)
    {
        case GATT_SERVICE_DISCOVERY_REGISTER_SUPPORTED_SERVICES_CFM:
            gattServiceDisovery_HandleRegisterMessage((const GATT_SERVICE_DISCOVERY_REGISTER_SUPPORTED_SERVICES_CFM_T*)message);
        break;

        case GATT_SERVICE_DISCOVERY_START_CFM:
            gattServiceDiscovery_HandleDiscoveryStartMessage((const GATT_SERVICE_DISCOVERY_START_CFM_T*)message);
        break;

        case GATT_SERVICE_DISCOVERY_STOP_CFM:
            gattServiceDiscovery_HandleDiscoveryStopMessage((const GATT_SERVICE_DISCOVERY_STOP_CFM_T*)message);
        break;

        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
            gattServiceDiscovery_HandleServiceRange((const GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T*)message);
        break;

        default:
        break;
    }
}

/*! \brief When pairing completes with handset, check if service discovery complete flag
           need to be written.
 */
static void gattServiceDisovery_HandlePairingActivity(const PAIRING_ACTIVITY_T *message)
{
    if (message->status != pairingActivitySuccess)
    {
        return;
    }

    if (appDeviceIsHandset(&message->device_addr) || appDeviceTypeIsSink(&message->device_addr))
    {
        device_t device;
        device = BtDevice_GetDeviceForBdAddr(&message->device_addr);
        DEBUG_LOG("gattServiceDisovery_HandlePairingActivity Serialise device %p", device);
        DeviceDbSerialiser_SerialiseDevice(device);
    }
}

/*! \brief Handler to receive/process notifications from Gatt Service Discovery library */
static void gattServiceDisovery_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    DEBUG_LOG("gattServiceDisovery_HandleMessage message id, MESSAGE:0x%x", id);

    switch (id)
    {
        case GATT_SD_PRIM:
            gattServiceDisovery_HandleGattPrim(message);
        break;

        case PAIRING_ACTIVITY:
            gattServiceDisovery_HandlePairingActivity((const PAIRING_ACTIVITY_T*)message);
        break;

        default:
        break;
    }
}

/*! \brief Register GATT services to discover.
           GATT service discovery module will only discover the services registered with it.
    \param service_id Service ID of the service to discover
    \param discover_handles Indicates gatt service module itself needs to discover the internal
           service range (ie, start and end handles).
           If TRUE, gatt service module will try to discover handles of the service after service
           discovery is completed. The handles will be passed to those clients registered using
           gattServiceDiscovery_InitLegacyClientsList() function.
*/
static void gattServiceDiscovery_RegisterServiceForDiscoveryWithHandle(GattSdSrvcId service_id)
{
    /* Register the service for discovery */
    GattServiceDiscovery_RegisterServiceForDiscovery(service_id);
    /* We need to find the handles of this service too. */
    gsd_taskdata.services_with_handle_to_discover |= service_id;
}

/*! \brief Registers the GATT services that needs to be discovered on a remote server */
bool GattServiceDiscovery_Init(Task init_task)
{
    UNUSED(init_task);
    gsd_taskdata.client_tasks = TaskList_Create();
    gsd_taskdata.task_data.handler = gattServiceDisovery_HandleMessage;
    gsd_taskdata.services_to_discover = GATT_SD_INVALID_SRVC;
    gsd_taskdata.services_with_handle_to_discover = GATT_SD_INVALID_SRVC;
    gsd_taskdata.cid_in_discovery = INVALID_CID;
    gsd_taskdata.cid_pending_to_discover = INVALID_CID;
    gsd_taskdata.gsd_status = gsd_in_idle;

    GattConnect_RegisterObserver(&gatt_observer_callback);
    Pairing_ActivityClientRegister(&gsd_taskdata.task_data);

#ifdef INCLUDE_GATT_SERVICE_DISCOVERY
    /* Initialise Legacy GATT clients */
    gattServiceDiscovery_InitLegacyClients();
#endif

    return TRUE;
}

/*! \brief Initialise GATT clients for service discovery.
    \note  This function should be used for legacy clients service discovery.
           The gatt discovery module will be responsible to discover the handles of these clients.
           Also the services will be rediscovered even if services are already discovered before.
           This is because the legacy clients does not supports persisting of handles.
 */
void gattServiceDiscovery_InitLegacyClientsList(const client_id_t* gatt_client_prioritised_id[],
                                                const GattSdSrvcId service_id[],
                                                uint8 num_elements)
{
    uint8 client_index = 0;
    /* Init may be called once only */
    PanicNotZero(client_priority_list.clients);

    client_priority_list.clients = gatt_client_prioritised_id;
    client_priority_list.size = num_elements;
    client_priority_list.num_service_instances = PanicUnlessMalloc(sizeof(*client_priority_list.num_service_instances)*num_elements);
    memset(client_priority_list.num_service_instances, 0, sizeof(*client_priority_list.num_service_instances)*num_elements);
    client_priority_list.service_id = service_id;

    /* Register all the services specified for service discovery */
    for (client_index = 0; client_index < num_elements; client_index++)
    {
        /* Register the service for discovery and handles */
        gattServiceDiscovery_RegisterServiceForDiscoveryWithHandle(service_id[client_index]);
    }
}

void GattServiceDiscovery_ClientRegister(Task client_task)
{
    PanicNull((void *)client_task);
    TaskList_AddTask(gsd_taskdata.client_tasks, client_task);
}

void GattServiceDiscovery_ClientUnregister(Task client_task)
{
    PanicNull((void *)client_task);
    TaskList_RemoveTask(gsd_taskdata.client_tasks, client_task);
}

void GattServiceDiscovery_RegisterServiceForDiscovery(GattSdSrvcId service_id)
{
    DEBUG_LOG("GattServiceDiscovery_RegisterServiceForDiscovery service_id %x", service_id);
    gsd_taskdata.services_to_discover |= service_id;
}

service_discovery_status GattServiceDiscovery_GetStatus(void)
{
    return gsd_taskdata.gsd_status;
}

void GattServiceDiscovery_StartServiceDiscovery(gatt_cid_t cid)
{
    gattServiceDiscovery_InitiateDiscovery(cid);
}

