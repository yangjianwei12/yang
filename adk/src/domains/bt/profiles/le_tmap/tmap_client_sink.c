/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    tmap_profile
    \brief      TMAP client sink
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "tmap_client_sink.h"
#include "tmap_client_sink_private.h"
#include "bt_device.h"
#include "pairing.h"
#include "call_control_client.h"
#include "media_control_client.h"

#ifdef INCLUDE_LE_AUDIO_UNICAST

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#endif

#define TMAP_LOG     DEBUG_LOG

#ifndef DISABLE_LE_AUDIO_VOICE
#define tmapClient_InitiateCallControlClientServiceDiscovery(cid)   CallControlClient_DiscoverServerService(cid)
#else
#define tmapClient_InitiateCallControlClientServiceDiscovery(cid)
#endif

#ifndef DISABLE_LE_AUDIO_MEDIA
#define tmapClient_InitiateMediaControlClientServiceDiscovery(cid)  MediaControlClient_DiscoverServerService(cid)
#else
#define tmapClient_InitiateMediaControlClientServiceDiscovery(cid)
#endif

/*! TMAP profile role is not known */
#define TMAP_CLIENT_ROLE_UNKNOWN    0

/*! \brief TMAP client task data. */
tmap_client_task_data_t tmap_taskdata;

/*! \brief Handler that receives notification from TMAP Library */
static void tmapClientSink_HandleMessage(Task task, MessageId id, Message message);

/*! \brief Callback function to handle GATT Connect notification */
static void tmapClientSink_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

/*! \brief Destroy tmap profile if any established for this connection */
static void tmapClientSink_OnGattDisconnect(gatt_cid_t cid)
{
    tmap_client_instance_t *instance = NULL;

    TMAP_LOG("tmapClientSink_OnGattDisconnect: cid=0x%04X", cid);

    instance = TmapClientSink_GetInstance(tmap_client_compare_by_cid, (unsigned)cid);
    if (instance != NULL)
    {
        if (instance->state == tmap_client_state_discovery)
        {
            /* Initialization is in progress Do not place a destroy request.Just reset the instance */
            TmapClientSink_ResetTmapClientInstance(instance);
        }
        else
        {
            TmapClientSink_DestroyInstance(cid);
        }
    }
}

/*! \brief Connect to tmap profile */
static void tmapClientSink_ConnectProfile(gatt_cid_t cid)
{
    if (TmapClientSink_RetrieveClientHandles(cid) != NULL)
    {
        TMAP_LOG("tmapClientSink_ConnectProfile create instance cid 0x%x", cid);
        /* If handles are already there, it means we can directly create the instance */
        PanicFalse(TmapClientSink_CreateInstance(cid));
    }
    else
    {
        TMAP_LOG("tmapClientSink_ConnectProfile start find service range cid 0x%x", cid);
        GattServiceDiscoveryFindServiceRange(TrapToOxygenTask((Task)&tmap_taskdata.task_data),
                                             cid, GATT_SD_TMAS_SRVC);
    }
}

/*! \brief Handle encryption change indication */
static void tmapClientSink_OnEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    TMAP_LOG("tmapClientSink_OnEncryptionChanged: cid=0x%04X, encrypted:%d", cid, encrypted);

    /* If encryption change indication is from a handset and we received service discovery
     * completion message  */
    if (encrypted && BtDevice_IsDeviceHandsetOrLeHandset(GattConnect_GetBtLeDevice(cid)))
    {
        tmap_client_instance_t *instance = NULL;

        instance = TmapClientSink_GetInstance(tmap_client_compare_by_cid, (unsigned)cid);
        if (!instance)
        {
            /* Check if service discovery complete indication is received from gatt service discovery module */
            if (GattServiceDisovery_IsServiceDiscoveryCompleted(cid))
            {
                tmapClientSink_ConnectProfile(cid);
            }
        }
    }
}

static const gatt_connect_observer_callback_t gatt_tmap_client_callback =
{
    .OnConnection = tmapClientSink_OnGattConnect,
    .OnDisconnection = tmapClientSink_OnGattDisconnect,
    .OnEncryptionChanged = tmapClientSink_OnEncryptionChanged
};

static pdd_size_t tmapClientSink_GetDeviceDataLength(device_t device)
{
    void *config = NULL;
    size_t config_size = 0;

    if (!Device_GetProperty(device, device_property_tmap_client, &config, &config_size))
    {
        config_size = 0;
    }
    return config_size;
}

static void tmapClientSink_SerialiseDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    void *config = NULL;
    size_t config_size = 0;
    UNUSED(offset);

    if (Device_GetProperty(device, device_property_tmap_client, &config, &config_size))
    {
        memcpy(buf, config, config_size);
    }
}

static void tmapClientSink_DeserialiseDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);

    Device_SetProperty(device, device_property_tmap_client, buf, data_length);
}

void TmapClientSink_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_LEA_TMAP_CLIENT,
        tmapClientSink_GetDeviceDataLength,
        tmapClientSink_SerialiseDeviceData,
        tmapClientSink_DeserialiseDeviceData);
}

/*! \brief Function that checks whether the TMAP matches based on the compare type */
static bool tmapClientSink_Compare(tmap_instance_compare_by_type_t type,
                                    unsigned compare_value,
                                    tmap_client_instance_t *instance)
{
    bool found = FALSE;

    switch (type)
    {
        case tmap_client_compare_by_cid:
            found = instance->cid == (gatt_cid_t) compare_value;
        break;

        case tmap_client_compare_by_state:
            found = instance->state == (tmap_client_state_t) compare_value;
        break;

        case tmap_client_compare_by_profile_handle:
            found = instance->tmap_profile_handle == (TmapClientProfileHandle) compare_value;
        break;

        case tmap_client_compare_by_bdaddr:
        {
            bdaddr addr;
            bdaddr *device_addr = (bdaddr *) compare_value;
            found = instance->state == tmap_client_state_connected &&
                    GattConnect_GetPublicAddrFromConnectionId(instance->cid, &addr) &&
                    BdaddrIsSame(&addr, device_addr);
        }
        break;

        case tmap_client_compare_by_valid_invalid_cid:
            found = instance->state == tmap_client_state_connected &&
               (instance->cid == (gatt_cid_t) compare_value || compare_value == INVALID_CID);
        break;

        default:
        break;
    }

    return found;
}

/*! \brief Get the TMAP client instance based on the compare type */
tmap_client_instance_t * TmapClientSink_GetInstance(tmap_instance_compare_by_type_t type, unsigned cmp_value)
{
    tmap_client_task_data_t *tmap_ctx = TmapClientSink_GetContext();
    tmap_client_instance_t *instance = NULL;

    ARRAY_FOREACH(instance, tmap_ctx->tmap_client_instance)
    {
        if (tmapClientSink_Compare(type, cmp_value, instance))
        {
            return instance;
        }
    }

    return NULL;
}

/*! \brief Function that checks whether the TMAS handles are already present in NVM */
static bool tmapClientSink_IsHandlesSameAsStoredData(GattTmasClientDeviceData *current_data,
                                                     unsigned gatt_cid)
{
    GattTmasClientDeviceData *stored_data = NULL;
    bool is_same_as_stored_data = FALSE;

    /* Get the TMAS handle information from NVM */
    stored_data = (GattTmasClientDeviceData*)TmapClientSink_RetrieveClientHandles(gatt_cid);

    if (stored_data != NULL &&
        (memcmp(stored_data, current_data, sizeof(GattTmasClientDeviceData)) == 0))
    {
        is_same_as_stored_data = TRUE;
    }

    return is_same_as_stored_data;
}

/*! \brief Write all the TMAP service handles to NVM */
static void tmapClientSink_WriteDeviceDataToStore(tmap_client_instance_t *instance)
{
    GattTmasClientDeviceData *dev_data = NULL;

    /* Retrieve all the discovered TMAS service handles */
    dev_data = TmapClientGetDevicedata(instance->tmap_profile_handle);

    /* Try to store the TMAS handle information in NVM */
    if (dev_data != NULL &&
        !tmapClientSink_IsHandlesSameAsStoredData(dev_data, instance->cid))
    {
        DEBUG_LOG("tmapClientSink_WriteDeviceDataToStore: Storing Handles in NVM");
        TmapClientSink_StoreClientHandles(instance->cid,
                                          (void*)dev_data,
                                          sizeof(GattTmasClientDeviceData));
    }

    /* Free the handle information */
    if (dev_data != NULL)
    {
        pfree(dev_data);
    }
}

/*! \brief Upon a successful initialization of TMAP, preserve the information in tmap client instance */
static void  tmapClientSink_HandleTmapProfileInitConfirmation(const TmapClientInitCfm *message)
{
    tmap_client_instance_t *instance = NULL;

    /* Try to find an existing instance with the same handle.
       If not found then get the next unused instance (based on handle is not set). */
    instance = TmapClientSink_GetInstance(tmap_client_compare_by_profile_handle, (unsigned)message->prflHndl);
    if (!instance)
    {
        instance = TmapClientSink_GetInstance(tmap_client_compare_by_profile_handle, (unsigned)0);
    }

    TMAP_LOG("tmapClientSink_HandleTmapProfileInitConfirmation prfl_handle: 0x%x, status: %d, instance: %p",
            message->prflHndl, message->status, instance);

    if (instance != NULL &&
        message->status == TMAP_CLIENT_STATUS_SUCCESS && 
        instance->tmap_profile_handle == message->prflHndl)
    {
        instance->tmap_profile_handle = message->prflHndl;
        instance->state = tmap_client_state_connected;

        if (!instance->handover_in_progress)
        {
            TmapClientReadRoleReq(instance->tmap_profile_handle);
        }
        else
        {
            instance->handover_in_progress = FALSE;
        }

        /* Write TMAS handle information to Store */
        tmapClientSink_WriteDeviceDataToStore(instance);
    }
    else if (instance != NULL &&
             message->status == TMAP_CLIENT_STATUS_IN_PROGRESS &&
             instance->tmap_profile_handle == 0)
    {
        TMAP_LOG("tmapClientSink_HandleTmapProfileInitConfirmation setting profile handle 0x%x", message->prflHndl);
        instance->tmap_profile_handle = message->prflHndl;
    }
    else
    {
        if (message->status == TMAP_CLIENT_STATUS_SUCCESS &&
           (instance == NULL || instance->tmap_profile_handle != message->prflHndl))
        {
            /* Profile initialization is successful, but there could be below conditions:
             * 1. Instance is NULL, which indicates the GATT connection has already gone.
             *                             (OR)
             * 2. Instance is not NULL, but profile handles mismatch.This could happen
             *    if the init cfm has arrived very late, while we are already processing
             *    a new init request.Though a remote possibility, it can happen
             *
             *    Under both conditions place a destroy request immediately.
             */
            TMAP_LOG("tmapClientSink_HandleTmapProfileInitConfirmation destroying profile handle 0x%x", message->prflHndl);
            TmapClientDestroyReq(message->prflHndl);
        }
        else
        {
            if (instance != NULL)
            {
                /* An error occurred during TMAP internal discovery. Reset the TMAP client instance */
                TMAP_LOG("tmapClientSink_HandleTmapProfileInitConfirmation resetting instance %p", instance);
                TmapClientSink_ResetTmapClientInstance(instance);
            }
        }
    }
}

/*! \brief Upon receiving a destroy confirmation, preserve the handles in NVM */
static void tmapClientSink_HandleTmapProfileDestroyConfirmation(const TmapClientDestroyCfm *message)
{
    tmap_client_instance_t *instance = NULL;

    TMAP_LOG("tmapClientSink_HandleTmapProfileDestroyConfirmation prfl_handle: 0x%x, status: %d",
            message->prflHndl, message->status);

    /* Find the matching call client instance based on the profile handle */
    instance = TmapClientSink_GetInstance(tmap_client_compare_by_profile_handle, (unsigned)message->prflHndl);

    if (instance != NULL &&
        message->status != TMAP_CLIENT_STATUS_IN_PROGRESS)
    {
        /* Reset the TMAP client instance */
        TmapClientSink_ResetTmapClientInstance(instance);
    }
}

/* Handle TMAP role confirmation msg */
static void tmapClientSink_HandleTmapRoleConfirmation(const TmapClientRoleCfm* message)
{
    tmap_client_instance_t *instance = NULL;

    instance = TmapClientSink_GetInstance(tmap_client_compare_by_profile_handle, (unsigned)message->prflHndl);
    PanicNull(instance);

    if (message->status == TMAP_CLIENT_STATUS_SUCCESS)
    {
        TMAP_LOG("tmapClientSink_HandleTmapRoleConfirmation prfl_handle: 0x%x, instance: %p", message->prflHndl, instance);

        if (message->role & TMAP_ROLE_CALL_GATEWAY)
        {
            /* Initialise CCP */
            CallControlClient_ConnectProfile(instance->cid);
        }

        if (message->role & TMAP_ROLE_UNICAST_MEDIA_SENDER)
        {
            /* Initialise MCP */
            MediaControlClient_ConnectProfile(instance->cid);
        }
    }
}

/*! \brief Process notifications received from TMAP library */
static void tmapClientSink_HandleTmapMessage(Message message)
{
    CsrBtCmPrim tmap_id = *(CsrBtCmPrim *)message;

    switch (tmap_id)
    {
        case TMAP_CLIENT_INIT_CFM:
            tmapClientSink_HandleTmapProfileInitConfirmation((const TmapClientInitCfm*)message);
        break;

        case TMAP_CLIENT_DESTROY_CFM:
            tmapClientSink_HandleTmapProfileDestroyConfirmation((const TmapClientDestroyCfm*)message);
        break;

        case TMAP_CLIENT_ROLE_CFM:
            tmapClientSink_HandleTmapRoleConfirmation((const TmapClientRoleCfm*)message);
        break;

        default:
            TMAP_LOG("tmapClientSink_HandleTmapMessage Unhandled message id: 0x%x", (*(CsrBtCmPrim *)message));
        break;
    }
}

/*! \brief Create the TMAP Client Instance */
bool TmapClientSink_CreateInstance(gatt_cid_t cid)
{
    tmap_client_instance_t *instance = NULL;

    instance = TmapClientSink_GetInstance(tmap_client_compare_by_state, tmap_client_state_idle);
    if (instance != NULL)
    {
        instance->cid = cid;
        instance->state = tmap_client_state_discovery;

#ifdef ENABLE_TMAP_PROFILE
        TmapClientHandles tmap_handle_data = {0};

        tmap_handle_data.tmasClientHandle = (GattTmasClientDeviceData*)TmapClientSink_RetrieveClientHandles(cid);

        TmapClientInitData client_init_params;
        client_init_params.cid = cid;
        TmapClientInitReq(TrapToOxygenTask((Task)&tmap_taskdata.task_data),
                          &client_init_params,
                          tmap_handle_data.tmasClientHandle == NULL ? NULL : &tmap_handle_data);
#endif
        return TRUE;
    }

    return FALSE;
}

/*! \brief Handle the pairing activity */
static void tmapClientSink_HandlePairingActivity(const PAIRING_ACTIVITY_T *message)
{
    tmap_client_instance_t *instance = NULL;

    if (message->status != pairingActivitySuccess)
    {
        return;
    }

    instance = TmapClientSink_GetInstance(tmap_client_compare_by_bdaddr, (unsigned)&message->device_addr);

    if (instance != NULL)
    {
        TmapClientReadRoleReq(instance->tmap_profile_handle);
        TMAP_LOG("tmapClientSink_HandlePairingActivity Registering for CID 0x%x", instance->cid);
    }
}

/*! \brief If TMAS Service is discovered successfully, initialize the Call control profile */
static void tmapClientSink_HandleServiceRange(const GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *msg)
{
    TMAP_LOG("tmapClientSink_HandleServiceRange Number of LEA Services cid: 0x%x, result: %d, Found: %d",
            msg->cid, msg->result, msg->srvcInfoCount);

    if (msg->result == GATT_SD_RESULT_SUCCESS &&
        msg->srvcInfoCount !=0 &&
        msg->srvcInfo->srvcId & GATT_SD_TMAS_SRVC)
    {
        PanicFalse(TmapClientSink_CreateInstance(msg->cid));
        pfree(msg->srvcInfo);
    }
    else
    {
        TMAP_LOG("tmapClientSink_HandleServiceRange unique TMAS Service not found in Remote Server cid: 0x%x", msg->cid);

        CallControlClient_ConnectProfile(msg->cid);
        MediaControlClient_ConnectProfile(msg->cid);
    }
}

/*! \brief Handler to handle GATT Service Discovery related primitives */
static void tmapClientSink_HandleGattPrim(Message message)
{
    switch (*(CsrBtCmPrim *)message)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
            tmapClientSink_HandleServiceRange((const GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T*)message);
        break;

        default:
        break;
    }
}

/*! \brief Check if the TMAS service is available in remote server and is discovered */
static void tmapClientSink_HandleGattDiscoveryComplete(const GATT_SERVICE_DISCOVERY_COMPLETE_T *ind)
{
    tmap_client_instance_t *instance = NULL;

    TMAP_LOG("tmapClientSink_HandleGattDiscoveryComplete cid: 0x%x", ind->cid);

    /* Need a check to see if TMAP is supported?
       No, that is done later. GATT_SERVICE_DISCOVERY_COMPLETE is only
       telling us that the GATT service discovery process is complete. */

    instance = TmapClientSink_GetInstance(tmap_client_compare_by_cid, (unsigned)ind->cid);

    TMAP_LOG("tmapClientSink_HandleGattDiscoveryComplete instance 0x%p", instance);

    if (!instance)
    {
        if (GattConnect_IsEncrypted(ind->cid))
        {
            /* If link is encrypted already, we can connect to tmap profiles immediately */
            tmapClientSink_ConnectProfile(ind->cid);
        }
    }
}

/*! \brief Process notifications received for TMAP client task */
static void tmapClientSink_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    TMAP_LOG("tmapClientSink_HandleMessage Received Message Id : 0x%x", id);

    switch (id)
    {
        case GATT_SERVICE_DISCOVERY_COMPLETE:
            tmapClientSink_HandleGattDiscoveryComplete((const GATT_SERVICE_DISCOVERY_COMPLETE_T*)message);
        break;

        case GATT_SD_PRIM:
            tmapClientSink_HandleGattPrim(message);
        break;

        case TMAP_CLIENT_PROFILE_PRIM:
            tmapClientSink_HandleTmapMessage(message);
        break;

        case PAIRING_ACTIVITY:
            tmapClientSink_HandlePairingActivity((const PAIRING_ACTIVITY_T*)message);
        break;

        default:
        break;
    }
}

/*! \brief Register with GATT LEA Service discovery */
bool TmapClientSink_Init(Task init_task)
{
    tmap_client_instance_t *instance = NULL;

    UNUSED(init_task);

    TMAP_LOG("TmapClientSink_Init");

    memset(&tmap_taskdata, 0, sizeof(tmap_taskdata));
    tmap_taskdata.task_data.handler = tmapClientSink_HandleMessage;

    ARRAY_FOREACH(instance, tmap_taskdata.tmap_client_instance)
    {
        TmapClientSink_ResetTmapClientInstance(instance);
    }

    GattServiceDiscovery_ClientRegister(&tmap_taskdata.task_data);
    GattConnect_RegisterObserver(&gatt_tmap_client_callback);
    Pairing_ActivityClientRegister(&tmap_taskdata.task_data);

    return TRUE;
}

/*! \brief Method used to retrieve discovered TMAS handles data from NVM */
void * TmapClientSink_RetrieveClientHandles(gatt_cid_t cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    void *server_handle_info = NULL;

    if (device)
    {
        size_t size;

        if (!Device_GetProperty(device, device_property_tmap_client, &server_handle_info, &size))
        {
            server_handle_info = NULL;
        }
    }

    return server_handle_info;
}

/*! \brief Method used to store discovered TMAS handles data to NVM */
bool TmapClientSink_StoreClientHandles(gatt_cid_t cid, void *config, uint8 size)
{
    bool handles_written = FALSE;
    device_t device = GattConnect_GetBtDevice(cid);

    if (device)
    {
        Device_SetProperty(device, device_property_tmap_client, config, size);
        DeviceDbSerialiser_SerialiseDevice(device);
        handles_written = TRUE;
    }

    return handles_written;
}

/*! \brief Reset the provided TMAP client instance */
void TmapClientSink_ResetTmapClientInstance(tmap_client_instance_t *tmap_client)
{
    if (tmap_client)
    {
        memset(tmap_client, 0, sizeof(tmap_client_instance_t));

        tmap_client->cid = INVALID_CID;
        tmap_client->state = tmap_client_state_idle;
        tmap_client->tmap_profile_handle = 0;
    }
}

/*! \brief Read the TMAP role characteristics */
void TmapClientSink_ReadTmapRole(gatt_cid_t cid)
{
    tmap_client_instance_t *instance = NULL;

    instance = TmapClientSink_GetInstance(tmap_client_compare_by_valid_invalid_cid, (unsigned)cid);
    if (instance)
    {
        TmapClientReadRoleReq(instance->tmap_profile_handle);
    }
}

/*! \brief Check If TMAP is connected or not */
bool TmapClientSink_IsTmapConnected(void)
{
    return TmapClientSink_GetInstance(tmap_client_compare_by_valid_invalid_cid, (unsigned)INVALID_CID) != NULL;
}

bool TmapClientSink_DestroyInstance(gatt_cid_t cid)
{
    tmap_client_instance_t *instance = NULL;

    TMAP_LOG("TmapClientSink_DestroyInstance: cid=0x%04X", cid);

    instance = TmapClientSink_GetInstance(tmap_client_compare_by_cid, (unsigned)cid);
    if (instance)
    {
        /* Before destroying, needs to call TmapClientRemoveDevice() to remove any active devices */
        TmapClientRemoveDevice(instance->tmap_profile_handle, cid);
        TmapClientDestroyReq(instance->tmap_profile_handle);

        return TRUE;
    }

    return FALSE;
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST) */
