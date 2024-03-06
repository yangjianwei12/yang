/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    tmap_profile
    \brief      TMAP client source
*/
#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "tmap_client_source.h"
#include "tmap_client_source_private.h"
#include "tmap_profile_mcs_tbs_private.h"
#include "bt_device.h"


#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#define TMAP_LOG     DEBUG_LOG

/*! TMAP profile role is not known */
#define TMAP_CLIENT_ROLE_UNKNOWN    0

/*! \brief TMAP client source task data. */
tmap_client_source_task_data_t tmap_client_source_taskdata;

/*! \brief When TRUE, the TMAP client operates in PTS mode. */
static bool tmap_profile_client_in_pts_mode = FALSE;

/*! Sends TMAP_CLIENT_MSG_ID_INIT_COMPLETE to registered client */
static void tmapClientSource_SendInitCfm(ServiceHandle group_handle, uint16 role, tmap_client_msg_status_t status)
{
    if (tmap_client_source_taskdata.callback_handler != NULL)
    {
        tmap_client_msg_t msg;

        msg.id = TMAP_CLIENT_MSG_ID_INIT_COMPLETE;
        msg.body.init_complete.group_handle = group_handle;
        msg.body.init_complete.status = status;
        msg.body.init_complete.role = role;

        tmap_client_source_taskdata.callback_handler(&msg);
    }
}

/*! Sends TMAP_CLIENT_MSG_ID_PROFILE_DEVICE_REMOVED to registered client */
static void tmapClient_SendDeviceRemovedInd(gatt_cid_t cid, bool device_removed, bool more_devices_present)
{
    if (tmap_client_source_taskdata.callback_handler != NULL)
    {
        tmap_client_msg_t msg;

        msg.id = TMAP_CLIENT_MSG_ID_PROFILE_DEVICE_REMOVED;
        msg.body.device_removed.cid = cid;
        msg.body.device_removed.more_devices_present = more_devices_present;
        msg.body.device_removed.status = device_removed ? TMAP_CLIENT_MSG_STATUS_SUCCESS : TMAP_CLIENT_MSG_STATUS_FAILED;

        tmap_client_source_taskdata.callback_handler(&msg);
    }
}

static pdd_size_t tmapClientSource_GetDeviceDataLength(device_t device)
{
    void *config = NULL;
    size_t config_size = 0;

    if (!Device_GetProperty(device, device_property_tmap_client, &config, &config_size))
    {
        config_size = 0;
    }
    return config_size;
}

static void tmapClientSource_SerialiseDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    void *config = NULL;
    size_t config_size = 0;
    UNUSED(offset);

    if (Device_GetProperty(device, device_property_tmap_client, &config, &config_size))
    {
        memcpy(buf, config, config_size);
    }
}

static void tmapClientSource_DeserialiseDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);

    Device_SetProperty(device, device_property_tmap_client, buf, data_length);
}

void TmapClientSource_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_LEA_TMAP_CLIENT,
        tmapClientSource_GetDeviceDataLength,
        tmapClientSource_SerialiseDeviceData,
        tmapClientSource_DeserialiseDeviceData);
}

/*! \brief Function that checks whether the TMAP matches based on the compare type */
static bool tmapClientSource_Compare(tmap_source_instance_compare_by_type_t type,
                               unsigned compare_value,
                               tmap_client_source_instance_t *instance)
{
    bool found = FALSE;

    switch (type)
    {
        case tmap_client_source_compare_by_cid:
            found = instance->cid == (gatt_cid_t) compare_value;
        break;

        case tmap_client_source_compare_by_state:
            found = instance->state == (tmap_client_source_state_t) compare_value;
        break;

        case tmap_client_source_compare_by_valid_invalid_cid:
            found = instance->state == tmap_client_source_state_connected &&
               (instance->cid == (gatt_cid_t) compare_value || compare_value == INVALID_CID);
        break;

        default:
        break;
    }

    return found;
}

/*! \brief Get the TMAP client instance based on the compare type */
tmap_client_source_instance_t * TmapClientSource_GetInstance(tmap_source_instance_compare_by_type_t type, unsigned cmp_value)
{
    uint8 i;
    tmap_client_source_instance_t *instance = NULL;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    for (i = 0; i < MAX_TMAP_DEVICES_SUPPORTED; i++)
    {
        if (tmapClientSource_Compare(type, cmp_value, &group_instance->tmap_client_instance[i]))
        {
            instance = &group_instance->tmap_client_instance[i];
            break;
        }
    }

    return instance;
}

/*! \brief Function that checks whether the TMAS handles are already present in NVM */
static bool tmapClientSource_IsHandlesSameAsStoredData(GattTmasClientDeviceData *current_data,
                                                          unsigned gatt_cid)
{
    GattTmasClientDeviceData *stored_data = NULL;
    bool is_same_as_stored_data = FALSE;

    /* Get the TMAS handle information from NVM */
    stored_data = (GattTmasClientDeviceData*)TmapClientSource_RetrieveClientHandles(gatt_cid);

    if (stored_data != NULL &&
        (memcmp(stored_data, current_data, sizeof(GattTmasClientDeviceData)) == 0))
    {
        is_same_as_stored_data = TRUE;
    }

    return is_same_as_stored_data;
}

/*! \brief Write all the TMAP service handles to NVM */
static void tmapClientSource_WriteDeviceDataToStore(tmap_client_source_instance_t *instance)
{
    GattTmasClientDeviceData *dev_data = NULL;

    /* Retrieve all the discovered TMAS service handles */
    dev_data = TmapClientGetDevicedata(TmapClientSource_GetGroupInstance()->tmap_profile_handle);

    /* Try to store the TMAS handle information in NVM */
    if (dev_data != NULL &&
        !tmapClientSource_IsHandlesSameAsStoredData(dev_data, instance->cid))
    {
        DEBUG_LOG("tmapClientSource_WriteDeviceDataToStore: Storing Handles in NVM");
        TmapClientSource_StoreClientHandles(instance->cid,
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
static void  tmapClientSource_HandleTmapProfileInitConfirmation(const TmapClientInitCfm *message)
{
    tmap_client_source_instance_t *instance;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    instance = TmapClientSource_GetInstance(tmap_client_source_compare_by_state, tmap_client_source_state_discovery);

    TMAP_LOG("tmapClientSource_HandleTmapProfileInitConfirmation prfl_handle: 0x%x, status: %d, instance: %p",
            message->prflHndl, message->status, instance);

    if (instance != NULL &&
        message->status == TMAP_CLIENT_STATUS_SUCCESS &&
        group_instance->tmap_profile_handle == message->prflHndl)
    {
        instance->state = tmap_client_source_state_connected;

        tmapClientSource_SetSpeakerIsoHandle(instance, 0xFFFF, 0xFFFF);
        tmapClientSource_SetMicIsoHandle(instance, 0xFFFF, 0xFFFF);
        TmapClientReadRoleReq(group_instance->tmap_profile_handle);

        /* Write TMAS handle information to Store */
        tmapClientSource_WriteDeviceDataToStore(instance);
    }
    else if(instance != NULL &&
            message->status == TMAP_CLIENT_STATUS_SUCCESS_TMAS_SRVC_NOT_FOUND)
    {
        /* TMAS service is not present, but treat it as connected as TMAP lib still allows
           operation (TMAP lib will internally route all operation through CAP) */
        TMAP_LOG("tmapClientSource_HandleTmapProfileInitConfirmation TMAS service not present");

        group_instance->tmap_profile_handle = message->prflHndl;
        instance->state = tmap_client_source_state_connected;

        tmapClientSource_SendInitCfm(group_instance->cap_group_handle, TMAP_CLIENT_ROLE_UNKNOWN, TMAP_CLIENT_MSG_STATUS_SUCCESS_TMAS_SRVC_NOT_FOUND);
    }
    else if (instance != NULL &&
             message->status == TMAP_CLIENT_STATUS_IN_PROGRESS &&
             group_instance->tmap_profile_handle == 0)
    {
        group_instance->tmap_profile_handle = message->prflHndl;
    }
    else
    {
        if (message->status == TMAP_CLIENT_STATUS_SUCCESS &&
           (instance == NULL || group_instance->tmap_profile_handle != message->prflHndl))
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
            TmapClientDestroyReq(message->prflHndl);
        }
        else
        {
            if (group_instance != NULL)
            {
                /* An error occurred during TMAP internal discovery. Reset the TMAP client instance */
                tmapClientSource_SendInitCfm(group_instance->cap_group_handle, TMAP_CLIENT_ROLE_UNKNOWN, TMAP_CLIENT_MSG_STATUS_FAILED);
                TmapClient_ResetTmapGroupInstance(group_instance);
            }
        }
    }
}

/*! \brief Upon receiving a destroy confirmation, preserve the handles in NVM */
static void tmapClientSource_HandleTmapProfileDestroyConfirmation(const TmapClientDestroyCfm *message)
{
    bool status;
    tmap_client_source_instance_t *instance;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TMAP_LOG("tmapClientSource_HandleTmapProfileDestroyConfirmation prfl_handle: 0x%x, status: %d",
            message->prflHndl, message->status);

    /* Find the matching call client instance based on the profile handle */
    PanicFalse(group_instance->tmap_profile_handle == message->prflHndl);

    instance = TmapClientSource_GetInstance(tmap_client_source_compare_by_state, tmap_client_source_state_disconnecting);
    status = (message->status == TMAP_CLIENT_STATUS_SUCCESS || message->status == TMAP_CLIENT_STATUS_ACTIVE_CONN_PRESENT);

    if (message->status != TMAP_CLIENT_STATUS_IN_PROGRESS)
    {
        tmapClient_SendDeviceRemovedInd(instance != NULL ? instance->cid : 0,
                                        status, message->status == TMAP_CLIENT_STATUS_ACTIVE_CONN_PRESENT);

        if (message->status == TMAP_CLIENT_STATUS_ACTIVE_CONN_PRESENT)
        {
            /* Reaching here indicates that device destroy is initiated when not all devices in the group are
               removed. */
            TmapClientSource_ResetTmapClientInstance(instance);
        }
        else
        {
            /* Reaching here indicates that all instances got removed */
            if (tmap_client_source_taskdata.callback_handler != NULL)
            {
                tmap_client_msg_t msg;

                /* The received message is for a device destroy operation */
                msg.id = TMAP_CLIENT_MSG_ID_PROFILE_DISCONNECT;
                msg.body.disconnect_complete.group_handle = group_instance->cap_group_handle;
                msg.body.disconnect_complete.status = message->status == TMAP_CLIENT_STATUS_SUCCESS ?
                                                      TMAP_CLIENT_MSG_STATUS_SUCCESS : TMAP_CLIENT_MSG_STATUS_FAILED ;

                tmap_client_source_taskdata.callback_handler(&msg);
            }

            TmapClient_ResetTmapGroupInstance(group_instance);
        }
    }
}

/* Handle TMAP role confirmation msg */
static void tmapClientSource_HandleTmapRoleConfirmation(const TmapClientRoleCfm* message)
{
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    PanicFalse(group_instance->tmap_profile_handle == message->prflHndl);

    if (message->status == TMAP_CLIENT_STATUS_SUCCESS)
    {
        TMAP_LOG("tmapClientSource_HandleTmapRoleConfirmation prfl_handle: 0x%x, instance: %p", message->prflHndl, group_instance);

        tmapClientSource_SendInitCfm(group_instance->cap_group_handle, message->role, TMAP_CLIENT_MSG_STATUS_SUCCESS);
    }
    else
    {
        tmapClientSource_SendInitCfm(group_instance->cap_group_handle, TMAP_CLIENT_ROLE_UNKNOWN, TMAP_CLIENT_MSG_STATUS_FAILED);
    }
}

/*! \brief Handle TMAP register CAP confirmation message */
static void tmapClientSource_HandleTmapRegisterCapConfirmation(const TmapClientRegisterTaskCfm* message)
{
    tmap_client_msg_t msg;

    TMAP_LOG("tmapClientSource_HandleTmapRegisterCapConfirmation status %d", message->result);

    PanicFalse(TmapClientSource_GetGroupInstance()->cap_group_handle == message->groupId);

    msg.id = TMAP_CLIENT_MSG_ID_REGISTER_CAP_CFM;
    msg.body.cap_register_cfm.group_id = message->groupId;
    msg.body.cap_register_cfm.status = message->result == CAP_CLIENT_RESULT_SUCCESS ?
                                       TMAP_CLIENT_MSG_STATUS_SUCCESS : TMAP_CLIENT_MSG_STATUS_FAILED ;
    tmap_client_source_taskdata.callback_handler(&msg);
}

/*! \brief Handle TMAP unicast volume state indication */
static void tmapClientSource_HandleUnicastVolumeStateInd(const TmapClientVolumeStateInd *volumestate_ind)
{
    tmap_client_msg_t msg;

    TMAP_LOG("tmapClientSource_HandleUnicastVolumeStateInd: volume state 0x%x", volumestate_ind->volumeState);

    PanicFalse(TmapClientSource_GetGroupInstance()->cap_group_handle == volumestate_ind->groupId);

    msg.id = TMAP_CLIENT_MSG_ID_VOLUME_STATE_IND;
    msg.body.volume_state_ind.group_handle = TmapClientSource_GetGroupInstance()->cap_group_handle;
    msg.body.volume_state_ind.volumeState = volumestate_ind->volumeState;
    msg.body.volume_state_ind.mute = volumestate_ind->mute;

    tmap_client_source_taskdata.callback_handler(&msg);
}

/*! \brief Handler that process the profile related messages alone */
void tmapClientSource_HandleTmapProfileMessage(Message message)
{
    CsrBtCmPrim tmap_id = *(CsrBtCmPrim *)message;

    switch (tmap_id)
    {
        case TMAP_CLIENT_INIT_CFM:
            tmapClientSource_HandleTmapProfileInitConfirmation((const TmapClientInitCfm*)message);
        break;

        case TMAP_CLIENT_DESTROY_CFM:
            tmapClientSource_HandleTmapProfileDestroyConfirmation((const TmapClientDestroyCfm*)message);
        break;

        case TMAP_CLIENT_ROLE_CFM:
            tmapClientSource_HandleTmapRoleConfirmation((const TmapClientRoleCfm*)message);
        break;

        case TMAP_CLIENT_REGISTER_CAP_CFM:
            tmapClientSource_HandleTmapRegisterCapConfirmation((const TmapClientRegisterTaskCfm*)message);
        break;

        case TMAP_CLIENT_VOLUME_STATE_IND:
            tmapClientSource_HandleUnicastVolumeStateInd((const TmapClientVolumeStateInd *) message);
        break;

        default:
            TMAP_LOG("tmapClientSource_HandleTmapProfileMessage Unhandled message id: 0x%x", tmap_id);
        break;
    }
}

/*! \brief Create the TMAP Client Instance */
bool TmapClientSource_CreateInstance(gatt_cid_t cid)
{
    tmap_client_source_instance_t *instance = NULL;
    TmapClientHandles tmap_handle_data;

    memset(&tmap_handle_data, 0, sizeof(TmapClientHandles));
    instance = TmapClientSource_GetInstance(tmap_client_source_compare_by_state, tmap_client_source_state_idle);

    if (instance != NULL)
    {
        instance->cid = cid;
        instance->state = tmap_client_source_state_discovery;

        tmap_handle_data.tmasClientHandle = (GattTmasClientDeviceData*)TmapClientSource_RetrieveClientHandles(cid);

#ifdef ENABLE_TMAP_PROFILE
        TmapClientInitData client_init_params;
        client_init_params.cid = cid;
        TmapClientInitReq(TrapToOxygenTask((Task)&tmap_client_source_taskdata.task_data),
                          &client_init_params,
                          tmap_handle_data.tmasClientHandle == NULL ? NULL : &tmap_handle_data);
#endif
        return TRUE;
    }
    return FALSE;
}

bool TmapClientSource_AddDeviceToGroup(gatt_cid_t *cid_array, uint8 count)
{
    int i;
    tmap_client_source_instance_t *instance = NULL;
    bool status = FALSE;

    PanicFalse(count <= MAX_TMAP_DEVICES_SUPPORTED);

    for (i = 0; i < count; i++)
    {
        instance = TmapClientSource_GetInstance(tmap_client_source_compare_by_cid, cid_array[i]);

        if (instance == NULL)
        {
            instance = TmapClientSource_GetInstance(tmap_client_source_compare_by_state, tmap_client_source_state_idle);
            PanicFalse(instance != NULL);
            instance->cid = cid_array[i];
            instance->state = tmap_client_source_state_connected;

            tmapClientSource_SetSpeakerIsoHandle(instance, 0xFFFF, 0xFFFF);
            tmapClientSource_SetMicIsoHandle(instance, 0xFFFF, 0xFFFF);

            status = TRUE;
            TMAP_LOG("TmapClientSource_AddDeviceToGroup cid 0x%x", cid_array[i]);
        }
    }

    return status;
}

/*! \brief Register with GATT LEA Service discovery */
bool TmapClientSource_Init(Task init_task)
{
    int dev_count;
    tmap_client_source_instance_t *instance;
    UNUSED(init_task);
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TMAP_LOG("TmapClientSource_Init");
    memset(&tmap_client_source_taskdata, 0, sizeof(tmap_client_source_taskdata));
    tmap_client_source_taskdata.task_data.handler = tmapClientSourceMessageHandler_HandleMessage;
    tmap_client_source_taskdata.callback_handler = NULL;

    for (dev_count = 0; dev_count < MAX_TMAP_DEVICES_SUPPORTED; dev_count++)
    {
        group_instance->tmap_client_instance[dev_count].cid = INVALID_CID;
        instance = &group_instance->tmap_client_instance[dev_count];

        tmapClientSource_SetSpeakerIsoHandle(instance, 0xFFFF, 0xFFFF);
        tmapClientSource_SetMicIsoHandle(instance, 0xFFFF, 0xFFFF);
    }

    return TRUE;
}

void TmapClientSource_RegisterCallback(tmap_client_source_callback_handler_t handler)
{
    tmap_client_source_taskdata.callback_handler = handler;
}

/*! \brief Method used to retrieve discovered TMAS handles data from NVM */
void * TmapClientSource_RetrieveClientHandles(gatt_cid_t cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    void *server_handle_info = NULL;

    if (device != NULL)
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
bool TmapClientSource_StoreClientHandles(gatt_cid_t cid, void *config, uint8 size)
{
    bool handles_written = FALSE;
    device_t device = GattConnect_GetBtDevice(cid);

    if (device != NULL)
    {
        Device_SetProperty(device, device_property_tmap_client, config, size);
        DeviceDbSerialiser_SerialiseDevice(device);
        handles_written = TRUE;
    }

    return handles_written;
}

/*! \brief Reset the provided TMAP client instance */
void TmapClientSource_ResetTmapClientInstance(tmap_client_source_instance_t *tmap_client)
{
    if (tmap_client != NULL)
    {
        memset(tmap_client, 0, sizeof(tmap_client_source_instance_t));
        tmap_client->cid = INVALID_CID;
    }
}

/*! \brief Reset the provided TMAP group instance */
void TmapClient_ResetTmapGroupInstance(tmap_client_source_group_instance_t *tmap_group_instance)
{
    uint8 i;

    if (tmap_group_instance != NULL)
    {
        memset(tmap_group_instance, 0, sizeof(tmap_client_source_group_instance_t));
        tmap_group_instance->cap_group_handle = TMAP_CLIENT_SOURCE_INVALID_GROUP_ID;

        for (i = 0; i < MAX_TMAP_DEVICES_SUPPORTED ; i++)
        {
            tmap_group_instance->tmap_client_instance[i].cid = INVALID_CID;
        }
    }
}

/*! \brief Read the TMAP role characteristics */
void TmapClientSource_ReadTmapRole(gatt_cid_t cid)
{
    tmap_client_source_instance_t *instance = NULL;

    instance = TmapClientSource_GetInstance(tmap_client_source_compare_by_valid_invalid_cid, (unsigned)cid);
    if (instance != NULL)
    {
        TmapClientReadRoleReq(TmapClientSource_GetGroupInstance()->tmap_profile_handle);
    }
}

/*! \brief Check If TMAP is connected or not */
bool TmapClientSource_IsTmapConnected(void)
{
    return TmapClientSource_GetInstance(tmap_client_source_compare_by_valid_invalid_cid, (unsigned)INVALID_CID) != NULL;
}

bool TmapClientSource_IsTmapConnectedForCid(gatt_cid_t cid)
{
    return TmapClientSource_GetInstance(tmap_client_source_compare_by_valid_invalid_cid, (unsigned)(cid == 0 ? INVALID_CID : cid)) != NULL;
}

/*! \brief This function remove the TMAP device from the group. It also sends indication to the registered clients that
           device is removed.
 */
static bool tmapClientSource_RemoveDeviceFromGroup(tmap_client_source_instance_t *instance, TmapClientProfileHandle profileHandle, gatt_cid_t cid)
{
    bool status;

    /* Before destroying, needs to call TmapClientRemoveDevice() to remove any active devices */
    status = TmapClientRemoveDevice(profileHandle, cid);

    if (status)
    {
        TmapClientSource_ResetTmapClientInstance(instance);
    }

    return status;
}

bool TmapClientSource_DestroyInstance(ServiceHandle group_handle, gatt_cid_t cid)
{
    uint8 i;
    tmap_client_source_instance_t *instance;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    TMAP_LOG("TmapClientSource_DestroyInstance: cid=0x%04X", cid);

    instance = TmapClientSource_GetInstance(tmap_client_source_compare_by_cid, cid);
    PanicFalse(group_instance->cap_group_handle == group_handle);

    if (instance != NULL || cid == 0)
    {
         /* Before destroying, needs to call TmapClientRemoveDevice() to remove any active devices */
        if (cid != 0)
        {
            instance->state = tmap_client_source_state_disconnecting;
            tmapClientSource_RemoveDeviceFromGroup(instance, group_instance->tmap_profile_handle, cid);
        }
        else
        {
            /* Group destroy operation. Remove all devices in the group before destroying the group */
            for (i = 0; i < MAX_TMAP_DEVICES_SUPPORTED; i++)
            {
                if (group_instance->tmap_client_instance[i].cid != INVALID_CID)
                {
                    tmapClientSource_RemoveDeviceFromGroup(&group_instance->tmap_client_instance[i],
                                                            group_instance->tmap_profile_handle,
                                                            group_instance->tmap_client_instance[i].cid);
                }
            }
        }

        TmapClientDestroyReq(group_instance->tmap_profile_handle);

        return TRUE;
    }

    return FALSE;
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
bool TmapClientSource_RegisterTaskWithCap(ServiceHandle group_handle)
{
    bool status = FALSE;
    tmap_client_source_group_instance_t *group_instance = TmapClientSource_GetGroupInstance();

    if (group_instance->cap_group_handle == group_handle)
    {
        /* Register profile with CAP */
        TmapClientRegisterTaskReq(group_handle, group_instance->tmap_profile_handle);
        status = TRUE;
    }

    return status;
}
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

bool TmapClientSource_SetGroupId(gatt_cid_t cid, ServiceHandle group_handle)
{
    bool status = FALSE;
    tmap_client_source_instance_t *instance;

    instance = TmapClientSource_GetInstance(tmap_client_source_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        /* Update group ID */
        TmapClientSource_GetGroupInstance()->cap_group_handle = group_handle;
        status = TRUE;
    }

    return status;
}

void TmapClientSource_SetPtsMode(bool pts_mode)
{
    tmap_profile_client_in_pts_mode = pts_mode;
}

bool TmapClientSource_IsInPtsMode(void)
{
    return tmap_profile_client_in_pts_mode;
}

TmapClientProfileHandle TmapClientSource_GetProfileHandle(void)
{
    return TmapClientSource_GetGroupInstance()->tmap_profile_handle;
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */
