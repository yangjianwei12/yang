/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
    All Rights Reserved.
    Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup media_control_client
    \brief   Media control Profile - Client Role Implementation.
*/

#include <logging.h>
#include <panic.h>

#include "gatt_service_discovery.h"
#include "media_control_client.h"
#include "media_control_client_private.h"
#include "mcp.h"
#include "pddu_map.h"
#include "device.h"
#include "gatt_connect.h"
#include "bt_device.h"
#include "device_properties.h"
#include "device_db_serialiser.h"
#include "gatt.h"
#include "gatt_service_discovery_lib.h"
#include "synergy.h"
#include "pairing.h"
#include "device_list.h"

#ifdef INCLUDE_LE_AUDIO_UNICAST

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#endif

/*! \brief Media control client task data. */
media_control_client_task_data_t media_control_taskdata;

/*! \brief Handler that receives notification from Media Control Profile library */
static void MediaControlClient_HandleMessage(Task task, MessageId id, Message message);

/*! \brief Action to take on the Media client instance */
typedef void (*media_control_action)(media_control_client_instance_t *instance, void *action_param);

/*! \brief Callback function to handle GATT Connect notification */
static void mediaControlClient_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

static void mediaControlPoint_SetLeAudioSourceProperty(media_control_client_instance_t *instance)
{
    device_t device = NULL;

    if (instance != NULL)
    {
        device = GattConnect_GetBtDevice(instance->cid);

        if (device != NULL && instance->state == media_client_state_connected)
        {
            DeviceProperties_SetLeAudioSource(device, audio_source_le_audio_unicast_1);
        }
    }
}

/*! \brief Destroy media control profile if any established for this connection */
static void mediaControlClient_OnGattDisconnect(gatt_cid_t cid)
{
    media_control_client_instance_t *instance;

    DEBUG_LOG("mediaControlClient_OnGattDisconnect: cid=0x%04X", cid);
    instance = MediaControlClient_GetInstance(media_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        if (instance->state == media_client_state_discovery)
        {
            /* Initialization is in progress Do not place a destroy request.Just reset the instance */
            MediaControlClient_ResetMediaClientInstance(instance);
        }
        else
        {
            McpDestroyReq(instance->mcp_profile_handle);
        }
    }
}

static const gatt_connect_observer_callback_t gatt_media_client_callback =
{
    .OnConnection = mediaControlClient_OnGattConnect,
    .OnDisconnection = mediaControlClient_OnGattDisconnect
};

/*! \brief Function that checks whether the GMCS handles are already present in NVM */
static bool mediaControlClient_IsHandlesSameAsStoredData(GattMcsClientDeviceData *current_data,
                                                          unsigned gatt_cid)
{
    GattMcsClientDeviceData *stored_data = NULL;
    bool is_same_as_stored_data = FALSE;

    /* Get the GMCS handle information from NVM */
    stored_data = (GattMcsClientDeviceData*) MediaControlClient_RetrieveClientHandles(gatt_cid);

    if (stored_data != NULL &&
        (memcmp(stored_data, current_data, sizeof(GattMcsClientDeviceData)) == 0))
    {
        is_same_as_stored_data = TRUE;
    }

    return is_same_as_stored_data;
}

/*! \brief Function that checks whether the media client instance matches based on the compare type */
static bool mediaControlClient_Compare(media_instance_compare_by_type_t type,
                                               unsigned compare_value,
                                               media_control_client_instance_t *instance)
{
    bool found = FALSE;

    switch (type)
    {
        case media_client_compare_by_cid:
            found = instance->cid == (gatt_cid_t) compare_value;
        break;

        case media_client_compare_by_profile:
            found = instance->mcp_profile_handle == (McpProfileHandle) compare_value;
        break;

        case media_client_compare_by_state:
            found = instance->state == (media_client_state_t) compare_value;
        break;

        case media_client_compare_by_bdaddr:
        {
            bdaddr addr;
            bdaddr *device_addr = (bdaddr *) compare_value;
            found = instance->state == media_client_state_connected &&
                    GattConnect_GetPublicAddrFromConnectionId(instance->cid, &addr) &&
                    BdaddrIsSame(&addr, device_addr);
        }
        break;

        default:
        break;
    }

    return found;
}

static pdd_size_t mediaControlClient_GetDeviceDataLength(device_t device)
{
    void *config = NULL;
    size_t config_size = 0;

    if (!Device_GetProperty(device, device_property_media_control_client, &config, &config_size))
    {
        config_size = 0;
    }
    return config_size;
}

static void mediaControlClient_SerialiseDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    void *config = NULL;
    size_t config_size = 0;
    UNUSED(offset);

    if (Device_GetProperty(device, device_property_media_control_client, &config, &config_size))
    {
        memcpy(buf, config, config_size);
    }
}

static void mediaControlClient_DeserialiseDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);

    Device_SetProperty(device, device_property_media_control_client, buf, data_length);
}

void MediaControlClient_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_LEA_MEDIA_CLIENT_CONTROL,
        mediaControlClient_GetDeviceDataLength,
        mediaControlClient_SerialiseDeviceData,
        mediaControlClient_DeserialiseDeviceData);
}

/*! \brief Write all the media control service handles to NVM */
static void mediaControlClient_WriteMediaDeviceDataToStore(media_control_client_instance_t *instance)
{
    GattMcsClientDeviceData *dev_data = NULL;

    /* Retrieve all the discovered media control service handles */
    dev_data = McpGetMediaPlayerAttributeHandles(instance->mcp_profile_handle,
                                                 instance->mcs_service_handle);

    /* Try to store the GMCS handle information in NVM */
    if (dev_data != NULL &&
        !mediaControlClient_IsHandlesSameAsStoredData(dev_data, instance->cid))
    {
        DEBUG_LOG("mediaControlClient_WriteMediaDeviceDataToStore");
        MediaControlClient_StoreClientHandles(instance->cid,
                                              (void*)dev_data,
                                              sizeof(GattMcsClientDeviceData));
    }

    /* Free the handle information */
    if (dev_data != NULL)
    {
        pfree(dev_data);
    }
}

/*! \brief Upon a successful initialization of MCP, preserve the information in media client instance */
static void mediaControlClient_HandleMediaProfileInitConfirmation(const McpInitCfm *message)
{
    media_control_client_instance_t *instance;

    instance = MediaControlClient_GetInstance(media_client_compare_by_profile, message->prflHndl);

    DEBUG_LOG("mediaControlClient_HandleMediaProfileInitConfirmation prfl_handle: 0x%x, status: %d, instance: %p",
            message->prflHndl, message->status, instance);

    if (!instance)
    {
        /* Instance not found; find an unused slot */
        instance = MediaControlClient_GetInstance(media_client_compare_by_profile, 0);
        DEBUG_LOG_VERBOSE("mediaControlClient_HandleMediaProfileInitConfirmation: new instance %p", instance);
    }

    DEBUG_LOG("mediaControlClient_HandleMediaProfileInitConfirmation: status: %d", message->status);

    if (instance != NULL &&
        message->status == MCP_STATUS_SUCCESS &&
        instance->mcp_profile_handle == message->prflHndl)
    {
        PanicFalse(message->mcsInstCount == 1);
        instance->mcs_service_handle = *message->mcsSrvcHandle;
        instance->state = media_client_state_connected;

        if (!instance->handover_in_progress)
        {
            DEBUG_LOG("mediaControlClient_HandleMediaProfileInitConfirmation: Registering for Notifications");
            McpRegisterForNotificationReq(instance->mcp_profile_handle,
                                          instance->mcs_service_handle,
                                          MCS_MEDIA_STATE_POS12 | MCS_MEDIA_CONTROL_POINT_POS13,
                                          MCS_MEDIA_STATE_POS12 | MCS_MEDIA_CONTROL_POINT_POS13);

            McpGetMediaPlayerAttribute(instance->mcp_profile_handle,
                                       instance->mcs_service_handle,
                                       MCS_CONTENT_CONTROL_ID);
        }
        else
        {
            instance->handover_in_progress = FALSE;
        }

        pfree(message->mcsSrvcHandle);
        mediaControlClient_WriteMediaDeviceDataToStore(instance);
        mediaControlPoint_SetLeAudioSourceProperty(instance);
    }
    else if (message->status == MCP_STATUS_IN_PROGRESS &&
             instance != NULL &&
             instance->mcp_profile_handle == 0)
    {
        instance->mcp_profile_handle = message->prflHndl;
    }
    else 
    {
        if (message->status == MCP_STATUS_SUCCESS &&
            (instance == NULL || instance->mcp_profile_handle != message->prflHndl))
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
            McpDestroyReq(message->prflHndl);
        }
        else
        {
            /* An error occurred during MCP internal discovery. Reset the Media client instance */
            MediaControlClient_ResetMediaClientInstance(instance);
        }
    }
}

/*! \brief Upon receiving a destroy confirmation, preserve the handles in NVM */
static void mediaControlClient_HandleMediaProfileDestroyConfirmation(const McpDestroyCfm *message)
{
    media_control_client_instance_t *media_client;

    /* Find the matching media client instance based on the profile handle */
    media_client = MediaControlClient_GetInstance(media_client_compare_by_profile, (unsigned)message->prflHndl);

    DEBUG_LOG("mediaControlClient_HandleMediaProfileDestroyConfirmation prfl_handle: 0x%x, status: %d, instance: %p",
            message->prflHndl, message->status, media_client);

    if (media_client != NULL && message->status != MCP_STATUS_IN_PROGRESS)
    {
        /* Reset the media client instance */
        MediaControlClient_ResetMediaClientInstance(media_client);
    }
}

/*! \brief Handle control point write confirmation */
static void mediaControlClient_HandleControlPointWriteConfirmation(const McpSetMediaControlPointCfm* message)
{
    DEBUG_LOG("mediaControlClient_HandleControlPointWriteConfirmation Opcode %d Status:%d", message->op, message->status);
}

void mediaControlClient_RegisterForMediaStateChangeIndications(media_control_client_callback_if * callback_if)
{
    media_control_mcp_state_client_t * new_client = PanicUnlessMalloc(sizeof(media_control_mcp_state_client_t));
    memset(new_client,0,sizeof(media_control_mcp_state_client_t));
    new_client->callback_if.media_state_change_callback = callback_if->media_state_change_callback;

    new_client->next = media_control_taskdata.clients;

    media_control_taskdata.clients = new_client;
}

static void mediaControlClient_SendIndicationToMediaStateClients(media_control_client_instance_t * media_client)
{
    media_control_mcp_state_client_t * curr = media_control_taskdata.clients;

    while(curr)
    {
        if(curr->callback_if.media_state_change_callback)
        {
            curr->callback_if.media_state_change_callback(media_client->server_state, media_client->cid);
        }

        curr = curr->next;
    }

}

/*! \brief Handle remote media state change notification */
static void mediaControlClient_HandleMediaStateNotification(const McpMediaStateInd *message)
{
    media_control_client_instance_t *media_client;

    media_client = MediaControlClient_GetInstance(media_client_compare_by_profile, (unsigned)message->prflHndl);
    if (media_client == NULL)
    {
        Panic();
        return;
    }

    DEBUG_LOG("mediaControlClient_HandleMediaStateNotification Media State %d For Cid 0x%x",
              message->mediaState, media_client->cid);

    media_client->server_state = message->mediaState;

    if(media_client->server_state == media_server_state_playing)
    {
        tp_bdaddr tp_addr = {0};
        device_t device = GattConnect_GetBtLeDevice(media_client->cid);

        if(BtDevice_GetTpBdaddrForDevice(device, &tp_addr))
        {
            DEBUG_LOG("mediaControlClient_HandleMediaStateNotification update MRU device %p", device);

            appDeviceUpdateMruDevice(&tp_addr.taddr.addr);

            mediaControlClient_SendIndicationToMediaStateClients(media_client);

        }
    }
}

static void mediaControlClient_HandleGetContentControlIdCfm(const McpGetContentControlIdCfm *message)
{
    media_control_client_instance_t *media_client;

    media_client = MediaControlClient_GetInstance(media_client_compare_by_profile, (unsigned)message->prflHndl);
    PanicNull(media_client);

    if (message->status == MCP_STATUS_SUCCESS)
    {
        media_client->content_control_id = message->ccid;
    }
}

static void mediaControlClient_HandleNotificationConfirmation(const McpNtfCfm *message)
{
    DEBUG_LOG("mediaControlClient_HandleNotificationConfirmation Result 0x%x", message->status);
}

/*! \brief Process notifications received from MCP library */
static void mediaControlClient_HandleMcpMessage(Message message)
{
    McpMessageId mcpId = *(McpMessageId*)message;
    void *free_it = NULL;

    switch (mcpId)
    {
        case MCP_INIT_CFM:
            mediaControlClient_HandleMediaProfileInitConfirmation((const McpInitCfm*)message);
        break;

        case MCP_DESTROY_CFM:
            mediaControlClient_HandleMediaProfileDestroyConfirmation((const McpDestroyCfm*)message);
        break;

        case MCP_SET_MEDIA_CONTROL_POINT_CFM:
            mediaControlClient_HandleControlPointWriteConfirmation((const McpSetMediaControlPointCfm*)message);
        break;

        case MCP_MEDIA_STATE_IND:
            mediaControlClient_HandleMediaStateNotification((const McpMediaStateInd*)message);
        break;

        case MCP_TRACK_CHANGED_IND:
            DEBUG_LOG("mediaControlClient_HandleMcpMessage : Media Track Changed");
        break;

        case MCP_NTF_CFM:
            mediaControlClient_HandleNotificationConfirmation((const McpNtfCfm*)message);
        break;

        case MCP_GET_MEDIA_PLAYER_NAME_CFM:
            free_it = ((const McpGetMediaPlayerNameCfm *) message)->name;
        break;

        case MCP_GET_MEDIA_PLAYER_ICON_URL_CFM:
            free_it = ((const McpGetMediaPlayerIconUrlCfm *) message)->iconUrl;
        break;

        case MCP_GET_TRACK_TITLE_CFM:
            free_it = ((const McpGetTrackTitleCfm *) message)->trackTitle;
        break;

        case MCP_GET_CONTENT_CONTROL_ID_CFM:
            mediaControlClient_HandleGetContentControlIdCfm((const McpGetContentControlIdCfm*)message);
        break;

        case MCP_MEDIA_PLAYER_NAME_IND:
            free_it = ((const McpMediaPlayerNameInd *) message)->name;
        break;

        case MCP_TRACK_TITLE_IND:
            free_it = ((const McpTrackTitleInd *) message)->trackTitle;
        break;

        default:
            DEBUG_LOG("mediaControlClient_HandleMcpMessage Media Message Received ID : 0x%x", mcpId);
        break;
    }

    if (free_it != NULL)
    {
        pfree(free_it);
    }
}

/*! \brief Create the Media Control Client Instance */
static void mediaControlClient_CreateInstance(gatt_cid_t cid)
{
    McpInitData client_init_params;
    media_control_client_instance_t *instance = NULL;
    McpHandles mcp_handle_data;

    memset(&mcp_handle_data, 0, sizeof(McpHandles));
    client_init_params.cid = cid;

    instance = MediaControlClient_GetInstance(media_client_compare_by_state, media_client_state_idle);

    if (instance != NULL)
    {
        instance->cid = cid;
        instance->state = media_client_state_discovery;
        /* mcsInstCount should always be 1 as we are interested only with GMCS Service */
        mcp_handle_data.mcsInstCount = 1;
        mcp_handle_data.mcsHandle = (GattMcsClientDeviceData*)MediaControlClient_RetrieveClientHandles(cid);
        McpInitReq(TrapToOxygenTask((Task)&media_control_taskdata.task_data),
                   &client_init_params,
                   mcp_handle_data.mcsHandle == NULL ? NULL : &mcp_handle_data);
    }
}

/*! \brief If MCS Service is discovered successfully, initialize the Media control profile */
static void mediaControlClient_HandleServiceRange(const GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *message)
{
    DEBUG_LOG("mediaControlClient_HandleServiceRange Number of LEA Services Found: %d", message->srvcInfoCount);

    /* Initialize MCP only when there is only one instance GMCS service in the remote server */
    if (message->result == GATT_SD_RESULT_SUCCESS && message->srvcInfoCount != 0)
    {
        if (message->srvcInfoCount == 1 && (message->srvcInfo->srvcId & GATT_SD_GMCS_SRVC))
        {
            DEBUG_LOG("mediaControlClient_HandleServiceRange MCS Service found in Remote CID: %d", message->cid);
            mediaControlClient_CreateInstance(message->cid);
        }
        else
        {
            DEBUG_LOG("mediaControlClient_HandleServiceRange unique GMCS Service not found in Remote Server CID: %d", message->cid);
        }

        pfree(message->srvcInfo);
    }
}

/*! \brief Handler to handle GATT Service Discovery related primitives */
static void mediaControlClient_HandleGattPrim(Message message)
{
    switch (*(CsrBtCmPrim *)message)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
            mediaControlClient_HandleServiceRange((const GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T*)message);
        break;

        default:
        break;
    }
}

/*! \brief Check if the Media control service is available in remote server and is discovered */
void MediaControlClient_ConnectProfile(gatt_cid_t cid)
{
    DEBUG_LOG("MediaControlClient_ConnectProfile cid: 0x%x", cid);

    if (MediaControlClient_RetrieveClientHandles(cid) != NULL)
    {
        DEBUG_LOG("MediaControlClient_ConnectProfile create instance");
        /* If handles are already there, it means we can directly create the instance */
        mediaControlClient_CreateInstance(cid);
    }
    else
    {
        /* Client handles not exists, find the service range first before creating instance */
        DEBUG_LOG("MediaControlClient_ConnectProfile start find service range");
        GattServiceDiscoveryFindServiceRange(TrapToOxygenTask((Task)&media_control_taskdata.task_data),
                                             cid,
                                             GATT_SD_GMCS_SRVC);
    }
}

/*! \brief When pairing completes with handset, register for media state notifications */
static void mediaControlClient_HandlePairingActivity(const PAIRING_ACTIVITY_T *message)
{
    media_control_client_instance_t *instance = NULL;

    if (message->status != pairingActivitySuccess)
    {
        return;
    }

    instance = MediaControlClient_GetInstance(media_client_compare_by_bdaddr, (unsigned)&message->device_addr);

    if (instance != NULL)
    {
        DEBUG_LOG("mediaControlClient_HandlePairingActivity Registering for CID 0x%x", instance->cid);
        McpRegisterForNotificationReq(instance->mcp_profile_handle,
                                      instance->mcs_service_handle,
                                      MCS_MEDIA_STATE_POS12 | MCS_MEDIA_CONTROL_POINT_POS13,
                                      MCS_MEDIA_STATE_POS12 | MCS_MEDIA_CONTROL_POINT_POS13);

        McpGetMediaPlayerAttribute(instance->mcp_profile_handle,
                                   instance->mcs_service_handle,
                                   MCS_CONTENT_CONTROL_ID);

        mediaControlClient_WriteMediaDeviceDataToStore(instance);
    }
}

/*! \brief Process notifications received for Media control client task */
static void MediaControlClient_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    DEBUG_LOG("MediaControlClient_HandleMessage Received Message Id : 0x%x", id);

    switch (id)
    {
        case MCP_PROFILE_PRIM:
            mediaControlClient_HandleMcpMessage(message);
        break;

        case GATT_SD_PRIM:
            mediaControlClient_HandleGattPrim(message);
        break;

        case PAIRING_ACTIVITY:
            mediaControlClient_HandlePairingActivity((const PAIRING_ACTIVITY_T*)message);
        break;

        default:
        break;
    }
}

/*! \brief Action function that sends the opcode to the remote MCS Server */
static void mediaControlPoint_SetOpcode(media_control_client_instance_t *instance, void *action_param)
{
    media_control_set_t *set_value = (media_control_set_t *) action_param;

    DEBUG_LOG("mediaControlPoint_SetOpcode Opcode:0x%x cid: 0x%x", set_value->op, instance->cid);
    McpSetMediaControlPoint(instance->mcp_profile_handle, instance->mcs_service_handle, set_value->op, set_value->val);
}

/*! \brief Action function that toggles the media state on the remote MCS Server */
static void mediaControlPoint_ToggleMediaState(media_control_client_instance_t *instance, void *action_param)
{
    GattMcsOpcode opcode = (instance->server_state == media_server_state_playing ||
                            instance->server_state == media_server_state_seeked) ? GATT_MCS_CLIENT_PAUSE : GATT_MCS_CLIENT_PLAY;

    UNUSED(action_param);

    DEBUG_LOG("mediaControlPoint_ToggleMediaState Opcode:0x%x, cid:0x%x", opcode, instance->cid);
    McpSetMediaControlPoint(instance->mcp_profile_handle, instance->mcs_service_handle, opcode, 0);
}

/*! \brief Executes the assigned action on the connected media profile */
static void mediaControlClient_ActionOnConnectedProfile(gatt_cid_t cid, media_control_action action_fn, void *action_param)
{
    media_control_client_instance_t *instance = NULL;

    for (instance = &media_control_taskdata.media_client_instance[0];
         instance < &media_control_taskdata.media_client_instance[MAX_MEDIA_SERVER_SUPPORTED];
         instance++)
    {
        if (instance->state == media_client_state_connected &&
            (cid == INVALID_CID || cid == instance->cid))
        {
            action_fn(instance, action_param);

            if (instance->cid == cid)
            {
                break;
            }
        }
    }
}

/*! \brief Action function that gets the media player attribute from remote server */
static void mediaControlPoint_GetMediaPlayerAttribute(media_control_client_instance_t *instance, void *action_param)
{
    media_control_attrib_t *get_value = (media_control_attrib_t *) action_param;

    DEBUG_LOG("mediaControlPoint_GetMediaPlayerAttribute Attribute:0x%x cid: 0x%x", get_value->attrib, instance->cid);
    McpGetMediaPlayerAttribute(instance->mcp_profile_handle, instance->mcs_service_handle, get_value->attrib);
}

/*! \brief Action function that sets the media player attribute in remote server */
static void mediaControlPoint_SetMediaPlayerAttribute(media_control_client_instance_t *instance, void *action_param)
{
    media_control_attrib_t *set_value = (media_control_attrib_t *) action_param;

    DEBUG_LOG("mediaControlPoint_SetMediaPlayerAttribute Opcode:0x%x cid: 0x%x", set_value->attrib, instance->cid);
    McpSetMediaPlayerAttribute(instance->mcp_profile_handle,
                               instance->mcs_service_handle,
                               set_value->attrib,
                               set_value->len,
                               set_value->val);
}

/*! \brief Action function that sends the notifications registration request */
static void mediaControlPoint_SendNotificationRegisterRequest(media_control_client_instance_t *instance, void *action_param)
{
    media_control_notification_req_t *register_req = (media_control_notification_req_t *) action_param;

    DEBUG_LOG("mediaControlPoint_SendNotificationRegisterRequest cid: 0x%x", instance->cid);
    McpRegisterForNotificationReq(instance->mcp_profile_handle,
                                  instance->mcs_service_handle,
                                  register_req->attrib,
                                  register_req->notif_value);
}

/*! \brief Register with GATT LEA Service discovery */
void MediaControlClient_Init(void)
{
    DEBUG_LOG("MediaControlClient_Init");
    memset(&media_control_taskdata, 0, sizeof(media_control_taskdata));
    media_control_taskdata.task_data.handler = MediaControlClient_HandleMessage;

    GattConnect_RegisterObserver(&gatt_media_client_callback);
    Pairing_ActivityClientRegister(&media_control_taskdata.task_data);
    GattServiceDiscovery_RegisterServiceForDiscovery(GATT_SD_GMCS_SRVC);
}

/*! \brief Send the media control opcode to the remote server */
void MediaControlClient_SendMediaControlOpcode(gatt_cid_t cid, GattMcsOpcode op, int32 val)
{
    media_control_set_t set_value;

    set_value.op = op;
    set_value.val = val;
    mediaControlClient_ActionOnConnectedProfile(cid, mediaControlPoint_SetOpcode, &set_value);
}

/*! \brief Function that toggles and sends Play/Pause opcode to media server */
void MediaControlClient_TogglePlayPause(gatt_cid_t cid)
{
    mediaControlClient_ActionOnConnectedProfile(cid, mediaControlPoint_ToggleMediaState, NULL);
}

/*! \brief Function that finds a matching audio context based on the present media state */
audio_source_provider_context_t MediaClientControl_GetAudioSourceContext(gatt_cid_t cid)
{
    audio_source_provider_context_t audio_context = context_audio_disconnected;
    media_control_client_instance_t *media_client = MediaControlClient_GetInstance(media_client_compare_by_cid, (unsigned) cid);

    if (media_client != NULL)
    {
        if (media_client->server_state == media_server_state_playing ||
            media_client->server_state == media_server_state_seeked)
        {
            audio_context = context_audio_is_playing;
        }
        else if (media_client->server_state == media_server_state_paused)
        {
            audio_context = context_audio_is_paused;
        }
        else
        {
            audio_context = context_audio_connected;
        }
    }

    if (audio_context == context_audio_disconnected &&
        MediaControlClient_GetInstance(media_client_compare_by_state, (unsigned)media_client_state_connected) != NULL)
    {
        audio_context = context_audio_connected;
    }

    return audio_context;
}

/*! \brief Function that reads the Media player attribute */
void MediaControlClient_GetMediaPlayerAttribute(gatt_cid_t cid, MediaPlayerAttribute charac)
{
    media_control_attrib_t get_value;

    get_value.attrib = charac;
    get_value.val = NULL;
    get_value.len = 0;
    mediaControlClient_ActionOnConnectedProfile(cid, mediaControlPoint_GetMediaPlayerAttribute, &get_value);
}

/*! \brief Function that write into the Media player attribute */
void MediaControlClient_SetMediaPlayerAttribute(gatt_cid_t cid, 
                                                MediaPlayerAttribute charac,
                                                uint16 len,
                                                uint8 *val)
{
    media_control_attrib_t set_value;

    set_value.attrib = charac;
    set_value.val = val;
    set_value.len = len;
    mediaControlClient_ActionOnConnectedProfile(cid, mediaControlPoint_SetMediaPlayerAttribute, &set_value);
}

/*! \brief Register for Media Server notifications */
void MediaControlClient_RegisterForNotifications(gatt_cid_t cid, 
                                                 MediaPlayerAttributeMask characType,
                                                 uint32 notif_value)
{
    media_control_notification_req_t notif_req;

    notif_req.attrib = characType;
    notif_req.notif_value = notif_value;
    mediaControlClient_ActionOnConnectedProfile(cid, mediaControlPoint_SendNotificationRegisterRequest, &notif_req);
}

/*! \brief Get the Media client instance based on the compare type */
media_control_client_instance_t * MediaControlClient_GetInstance(media_instance_compare_by_type_t type, unsigned cmp_value)
{
    media_control_client_instance_t *instance = NULL;

    for (instance = &media_control_taskdata.media_client_instance[0];
         instance < &media_control_taskdata.media_client_instance[MAX_MEDIA_SERVER_SUPPORTED];
         instance++)
    {
        if (mediaControlClient_Compare(type, cmp_value, instance))
        {
            return instance;
        }
    }

    return NULL;
}

/*! \brief Retrieve GMCS handle data from NVM for the provided connection identifier */
void * MediaControlClient_RetrieveClientHandles(gatt_cid_t cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    void *server_handle_info = NULL;

    if (device)
    {
        size_t size;

        if (!Device_GetProperty(device, device_property_media_control_client, &server_handle_info, &size))
        {
            server_handle_info = NULL;
        }
    }

    return server_handle_info;
}

/*! \brief Store GMCS handle data to NVM for the provided connection identifier */
bool MediaControlClient_StoreClientHandles(gatt_cid_t cid, void *config, uint8 size)
{
    bool handles_stored = FALSE;
    device_t device = GattConnect_GetBtDevice(cid);

    if (device)
    {
        Device_SetProperty(device, device_property_media_control_client, config, size);
        DeviceDbSerialiser_SerialiseDevice(device);
        handles_stored = TRUE;
    }

    return handles_stored;
}

/* Get the content control id for media client */
uint8 MediaClientControl_GetContentControlId(gatt_cid_t cid)
{
    media_control_client_instance_t *instance;
    uint16 content_control_id = 0;

    instance = MediaControlClient_GetInstance(media_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        content_control_id = instance ->content_control_id;
    }

    return content_control_id;
}

/*! \brief Reset the provided media client instance */
void MediaControlClient_ResetMediaClientInstance(media_control_client_instance_t *media_client)
{
    if (media_client != NULL)
    {
        memset(media_client, 0, sizeof(media_control_client_instance_t));
        media_client->cid = INVALID_CID;
    }
}

device_t MediaClientControl_GetDeviceForAudioSource(audio_source_t source)
{
    return DeviceList_GetFirstDeviceWithPropertyValue(device_property_le_audio_source, &source, sizeof(audio_source_t));
}
