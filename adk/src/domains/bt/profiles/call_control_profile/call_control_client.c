/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    call_control_client
    \brief      Call control Profile - Client Role Implementation.
*/

#include "call_control_client.h"
#include "call_control_client_private.h"
#include "gatt_service_discovery.h"
#include "bt_device.h"
#include "pairing.h"
#include "device_list.h"

#ifdef INCLUDE_LE_AUDIO_UNICAST

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#endif

#define CCP_LOG     DEBUG_LOG

/*! \brief Call control client task data. */
call_control_client_task_data_t call_control_taskdata;

/*! \brief Handler that receives notification from Call Control Profile library */
static void CallControlClient_HandleMessage(Task task, MessageId id, Message message);

/*! \brief Action to take on the Call client instance */
typedef void (*call_control_action)(call_control_client_instance_t *instance, void *action_param);

/*! \brief Callback function to handle GATT Connect notification */
static void callControlClient_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

static void callControlClient_SetLeVoiceSourceProperty(call_control_client_instance_t *instance)
{
    device_t device = NULL;

    if (instance != NULL)
    {
        device = GattConnect_GetBtDevice(instance->cid);
        if (device != NULL && instance->state == call_client_state_connected)
        {
            DeviceProperties_SetLeVoiceSource(device, voice_source_le_audio_unicast_1);
        }
    }
}

/*! \brief Callback function to handle GATT Encryption changed notification */
static void callControlClient_OnGattEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    call_control_client_instance_t *instance;
    UNUSED(encrypted);

    instance = CallControlClient_GetInstance(call_client_compare_by_cid, (unsigned)cid);
    callControlClient_SetLeVoiceSourceProperty(instance);
}

/*! \brief Destroy call control profile if any established for this connection */
static void callControlClient_OnGattDisconnect(gatt_cid_t cid)
{
    call_control_client_instance_t *instance;

    CCP_LOG("callControlClient_OnGattDisconnect: cid=0x%04X", cid);
    instance = CallControlClient_GetInstance(call_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        MessageCancelFirst(CallControlClient_GetTask(), CCP_INTERNAL_OUT_OF_BAND_RINGTONE_REQ);
        if (instance->state == call_client_state_discovery)
        {
            /* Initialization is in progress Do not place a destroy request. Just reset the instance */
            CallControlClient_ResetCallClientInstance(instance);
        }
        else
        {
            CcpDestroyReq(instance->ccp_profile_handle);
        }
    }
}

static const gatt_connect_observer_callback_t gatt_call_client_callback =
{
    .OnConnection = callControlClient_OnGattConnect,
    .OnDisconnection = callControlClient_OnGattDisconnect,
    .OnEncryptionChanged = callControlClient_OnGattEncryptionChanged,
};

/*! \brief Function that checks whether the GTBS handles are already present in NVM */
static bool callControlClent_IsHandlesSameAsStoredData(GattTelephoneBearerClientDeviceData *current_data,
                                                       unsigned gatt_cid)
{
    GattTelephoneBearerClientDeviceData *stored_data = NULL;
    bool is_same_as_stored_data = FALSE;

    /* Get the GTBS handle information from NVM */
    stored_data = (GattTelephoneBearerClientDeviceData*) CallControlClient_RetrieveClientHandles(gatt_cid);

    if (stored_data != NULL &&
        (memcmp(stored_data, current_data, sizeof(GattTelephoneBearerClientDeviceData)) == 0))
    {
        is_same_as_stored_data = TRUE;
    }

    return is_same_as_stored_data;
}

/*! \brief Function that checks whether the call client instance matches based on the compare type */
static bool callControlClient_Compare(call_instance_compare_by_type_t type,
                                      unsigned compare_value,
                                      call_control_client_instance_t *instance)
{
    bool found = FALSE;

    switch (type)
    {
        case call_client_compare_by_cid:
            found = instance->cid == (gatt_cid_t) compare_value;
        break;

        case call_client_compare_by_profile_handle:
            found = instance->ccp_profile_handle == (CcpProfileHandle) compare_value;
        break;

        case call_client_compare_by_state:
            found = instance->state == (call_client_state_t) compare_value;
        break;

        case call_client_compare_by_bdaddr:
        {
            bdaddr addr;
            bdaddr *device_addr = (bdaddr *) compare_value;
            found = instance->state == call_client_state_connected &&
                    GattConnect_GetPublicAddrFromConnectionId(instance->cid, &addr) &&
                    BdaddrIsSame(&addr, device_addr);
        }
        break;

        case call_client_compare_by_voice_source:
            found = instance->state == call_client_state_connected &&
                    voice_source_le_audio_unicast_1 == (voice_source_t) compare_value;
        break;

        case call_client_compare_by_valid_invalid_cid :
            found = instance->state == call_client_state_connected &&
                   (instance->cid == (gatt_cid_t) compare_value || compare_value == INVALID_CID);

        default:
        break;
    }

    return found;
}

static pdd_size_t callControlClient_GetDeviceDataLength(device_t device)
{
    void *config = NULL;
    size_t config_size = 0;

    if (!Device_GetProperty(device, device_property_call_control_client, &config, &config_size))
    {
        config_size = 0;
    }
    return config_size;
}

static void callControlClient_SerialiseDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    void *config = NULL;
    size_t config_size = 0;
    UNUSED(offset);

    if (Device_GetProperty(device, device_property_call_control_client, &config, &config_size))
    {
        memcpy(buf, config, config_size);
    }
}

static void callControlClient_DeserialiseDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);

    Device_SetProperty(device, device_property_call_control_client, buf, data_length);
}

void CallControlClient_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_LEA_CALL_CLIENT_CONTROL,
        callControlClient_GetDeviceDataLength,
        callControlClient_SerialiseDeviceData,
        callControlClient_DeserialiseDeviceData);
}

/*! \brief Write Call control client device data to NVM */
static void callControlClient_WriteCallControlDeviceDataToStore(call_control_client_instance_t *instance)
{
    GattTelephoneBearerClientDeviceData *dev_data = NULL;

    /* Retrieve all the discovered GTBS service handles */
    dev_data = CcpGetTelephoneBearerAttributeHandles(instance->ccp_profile_handle, 0);

    /* Try to store the GTBS handle information in NVM */
    if (dev_data != NULL &&
        !callControlClent_IsHandlesSameAsStoredData(dev_data, instance->cid))
    {
        CCP_LOG("callControlClient_WriteCallControlDeviceDataToStore");
        CallControlClient_StoreClientHandles(instance->cid,
                                             (void*)dev_data,
                                             sizeof(GattTelephoneBearerClientDeviceData));
    }

    /* Free the GTBS handle information */
    if (dev_data != NULL)
    {
        pfree(dev_data);
    }
}

/*! \brief Upon a successful initialization of CCP, preserve the information in call client instance */
static void  callControlClient_HandleCallProfileInitConfirmation(const CcpInitCfm *message)
{
    call_control_client_instance_t *instance;

    instance = CallControlClient_GetInstance(call_client_compare_by_profile_handle, message->prflHndl);

    CCP_LOG("callControlClient_HandleCallProfileInitConfirmation prfl_handle: 0x%x, status: %d, instance: %p",
            message->prflHndl, message->status, instance);

    if (!instance)
    {
        /* Instance not found; find an unused slot */
        instance = CallControlClient_GetInstance(call_client_compare_by_profile_handle, 0);
        CCP_LOG("callControlClient_HandleCallProfileInitConfirmation: new instance %p", instance);
    }

    if (instance != NULL &&
        message->status == CCP_STATUS_SUCCESS &&
        instance->ccp_profile_handle == message->prflHndl)
    {
        instance->state = call_client_state_connected;

        if (!instance->handover_in_progress)
        {
            CCP_LOG("callControlClient_HandleCallProfileInitConfirmation Configuring for notifications");
            CcpSetCallStateNotificationRequest(instance->ccp_profile_handle, TRUE);
            CcpSetTerminationReasonNotificationRequest(instance->ccp_profile_handle, TRUE);
            CcpSetCallControlPointNotificationRequest(instance->ccp_profile_handle, TRUE);
            CcpSetFlagsNotificationRequest(instance->ccp_profile_handle, TRUE);
            CcpReadContentControlIdRequest(instance->ccp_profile_handle);
            CcpReadStatusAndFeatureFlagsRequest(instance->ccp_profile_handle);
            CcpReadCallStateRequest(instance->ccp_profile_handle);
            CcpReadIncomingCallRequest(instance->ccp_profile_handle);
            CcpReadCallFriendlyNameRequest(instance->ccp_profile_handle);
            CcpReadCallControlPointOptionalOpcodesRequest(instance->ccp_profile_handle);
        }
        else
        {
            instance->handover_in_progress = FALSE;
        }

        callControlClient_SetLeVoiceSourceProperty(instance);
        callControlClient_WriteCallControlDeviceDataToStore(instance);
    }
    else if (message->status == CCP_STATUS_IN_PROGRESS &&
             instance != NULL &&
             instance->ccp_profile_handle == 0)
    {
        instance->ccp_profile_handle = message->prflHndl;
    }
    else
    {
        if (message->status == CCP_STATUS_SUCCESS &&
            (instance == NULL || instance->ccp_profile_handle != message->prflHndl))
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
            CcpDestroyReq(message->prflHndl);
        }
        else
        {
            /* An error occurred during CCP internal discovery. Reset the Call client instance */
            CallControlClient_ResetCallClientInstance(instance);
        }
    }
}

/*! \brief Upon receiving a destroy confirmation, preserve the handles in NVM */
static void callControlClient_HandleCallProfileDestroyConfirmation(const CcpDestroyCfm *message)
{
    call_control_client_instance_t *call_client;

    /* Find the matching call client instance based on the profile handle */
    call_client = CallControlClient_GetInstance(call_client_compare_by_profile_handle, (unsigned)message->prflHndl);

    CCP_LOG("callControlClient_HandleCallProfileDestroyConfirmation prfl_handle: 0x%x, status: %d, instance: %p",
            message->prflHndl, message->status, call_client);

    if (call_client != NULL && message->status != CCP_STATUS_IN_PROGRESS)
    {
        /* Reset the media client instance */
        CallControlClient_ResetCallClientInstance(call_client);
        callControlClient_SetLeVoiceSourceProperty(call_client);
    }
}

/*! \brief Handle control point write confirmation */
static void callControlClient_HandleCallControlPointWriteConfirmation(const CcpSetCfm *message)
{
    CCP_LOG("callControlClient_HandleCallControlPointWriteConfirmation prfl_handle: 0x%x, srvc_handle: 0x%x, status: %d",
            message->prflHndl, message->srvcHndl, message->status);
}

static ccp_call_info_t* callControlClient_FindCallStateForTbsCallId(call_control_client_instance_t *instance, uint8 call_id)
{
    ccp_call_info_t *call_info = NULL;

    for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
    {
        if (instance->call_state[idx].tbs_call_id == call_id)
        {
            call_info = &instance->call_state[idx];
        }
    }

    return call_info;
}

static ccp_call_info_t * callControlClient_AddCallState(call_control_client_instance_t *instance, TbsCallState *tbs_call_state)
{
    ccp_call_info_t *call_info = NULL;

    for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
    {
        if (instance->call_state[idx].state == CCP_CALL_STATE_IDLE)
        {
           /*A free slot in call state array in*/
           call_info = &instance->call_state[idx];
           CcpSm_SetCallState(instance->cid, call_info, tbs_call_state);
           return call_info;
        }
    }

    CCP_LOG("callControlClient_AddCallState : No free slot available, Rejecting!");
    return call_info;
}

static void callControlClient_PlayRingtone(void)
{
    voice_source_t source = voice_source_le_audio_unicast_1;
    call_control_client_instance_t *instance;

    instance = CallControlClient_GetInstance(call_client_compare_by_voice_source, (unsigned)source);

    /* If the instance is NULL, it indicates CCP profile is disconnected already. Just return */
    if (instance == NULL)
    {
        return;
    }

    /* Play ring tone if AG doesn't support in band ringing */
    if (!instance->tbs_status_info.in_band_ring)
    {
        for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
        {
            ccp_call_state_t call_state = CcpSm_GetCallState(instance, idx);

            if (call_state == CCP_CALL_STATE_INCOMING)
            {
                Telephony_NotifyCallIncomingOutOfBandRingtone(source);
                MessageCancelFirst(CallControlClient_GetTask(), CCP_INTERNAL_OUT_OF_BAND_RINGTONE_REQ);
                MessageSendLater(CallControlClient_GetTask(), CCP_INTERNAL_OUT_OF_BAND_RINGTONE_REQ, NULL, D_SEC(5));
                return;
            }
        }
    }
    else
    {
        /* @Todo Revisit to play in band ringtone*/
    }
}

voice_source_t CallControlClient_GetVoiceSourceForCid(gatt_cid_t cid)
{
    voice_source_t source = voice_source_none;
    bdaddr bd_addr;

    GattConnect_GetPublicAddrFromConnectionId(cid, &bd_addr);
    device_t device = BtDevice_GetDeviceForBdAddr(&bd_addr);
    if (device)
    {
        /* @Todo Get the source from device properties once multipoint is available */
        source = voice_source_le_audio_unicast_1;
    }

    return source;
}

static void callControlClient_UpdateCallState(CcpProfileHandle ccp_profile_handle, TbsCallState *call_state_list, uint8 call_state_list_size)
{
    call_control_client_instance_t *call_client;
    TbsCallState *tbs_call_state = NULL;
    TbsCallState tbs_call_state_clear;
    ccp_call_info_t *call_info = NULL;

    call_client = CallControlClient_GetInstance(call_client_compare_by_profile_handle, (unsigned)ccp_profile_handle);
    PanicNull(call_client);

    CCP_LOG("callControlClient_UpdateCallState prfl_handle: 0x%x, callStateListSize: %d, instance: %p",
            ccp_profile_handle, call_state_list_size, call_client);

    /* Empty call list means there is no call at the remote, clear the call states */
    if (call_state_list_size == 0)
    {
        CCP_LOG("callControlClient_UpdateCallState Empty call list, clearing the call states");
        for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
        {
            call_info = &call_client->call_state[idx];
            tbs_call_state_clear.callId = call_info->tbs_call_id;
            tbs_call_state_clear.callState = TBS_CALL_STATE_INVALID;
            /* Clear the call state*/
            CcpSm_SetCallState(call_client->cid, call_info, &tbs_call_state_clear);
        }
        return;
    }

    for (uint8 idx = 0; idx < call_state_list_size; idx++)
    {
        tbs_call_state = &call_state_list[idx];

        CCP_LOG("callControlClient_UpdateCallState callId: 0x%02x, callState: 0x%02x, callFlag: 0x%02x,",
                    tbs_call_state->callId, tbs_call_state->callState, tbs_call_state->callFlags);
        if (tbs_call_state->callId == 0 || tbs_call_state->callState > TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)
        {
            continue;
        }

        call_info = callControlClient_FindCallStateForTbsCallId(call_client, tbs_call_state->callId);

        if (call_info != NULL)
        {
            /* Handle the call state from the call state list */
            CcpSm_SetCallState(call_client->cid, call_info, tbs_call_state);
        }
        else
        {
            CCP_LOG("callControlClient_UpdateCallState New call. Adding to the call state instance");
            call_info = callControlClient_AddCallState(call_client, tbs_call_state);
        }

        if (call_info != NULL && call_info->state == CCP_CALL_STATE_INCOMING)
        {
            /* Req to play an out-of-band ring once to notify user */
            callControlClient_PlayRingtone();
        }
     }
}

static void callControlClient_HandleFlagsInd(const CcpFlagsInd *message)
{
    call_control_client_instance_t *call_client;
    call_client = CallControlClient_GetInstance(call_client_compare_by_profile_handle, (unsigned)message->prflHndl);
    PanicNull(call_client);

    call_client->tbs_status_info.in_band_ring = IsInbandRingingEnabled(message->flags);
    call_client->tbs_status_info.silent_mode = IsServerInSilentMode(message->flags);

    CCP_LOG("callControlClient_HandleFlagsInd in_band_ring :%d, silent_mode :%d", call_client->tbs_status_info.in_band_ring,
             call_client->tbs_status_info.silent_mode);
}

static void callControlClient_HandleCallTerminationReasonNotification(const CcpTerminationReasonInd *message)
{
    call_control_client_instance_t *call_client;
    TbsCallState tbs_call_state;
    ccp_call_info_t *call_info = NULL;
    uint8 reason_code;

    call_client = CallControlClient_GetInstance(call_client_compare_by_profile_handle, (unsigned)message->prflHndl);
    PanicNull(call_client);

    reason_code = message->reasonCode;

    for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
    {
        if (message->callId == call_client->call_state[idx].tbs_call_id)
        {
            call_info = &call_client->call_state[idx];
            tbs_call_state.callId = call_info->tbs_call_id;
            tbs_call_state.callState = TBS_CALL_STATE_INVALID;

            /* For detailed call termination reason check Enumeration of call termination reasons GattTbsCallTerminationReason */
            CCP_LOG("callControlClient_HandleCallTerminationReasonNotification callId = %d, active_call_index = %d, reasonCode=0x%x",
                    message->callId, call_client->call_state[idx].tbs_call_id, reason_code);
            /* Clear the call state*/
            CcpSm_SetCallState(call_client->cid, call_info, &tbs_call_state);
            break;
        }
    }
}

static void callControlClient_HandleReadContentControlIdCfm(const CcpReadContentControlIdCfm *message)
{
    call_control_client_instance_t *call_client;

    call_client = CallControlClient_GetInstance(call_client_compare_by_profile_handle, (unsigned)message->prflHndl);
    PanicNull(call_client);

    if (message->status == CCP_STATUS_SUCCESS)
    {
        call_client->content_control_id = message->contentControlId;
    }
}

static void callControlClient_HandleReadFeatureAndStatusFlagsCfm(const CcpReadFlagsCfm *message)
{
    call_control_client_instance_t *call_client;

    call_client = CallControlClient_GetInstance(call_client_compare_by_profile_handle, (unsigned)message->prflHndl);
    PanicNull(call_client);
    call_client->tbs_status_info.in_band_ring = IsInbandRingingEnabled(message->flags);
    call_client->tbs_status_info.silent_mode = IsServerInSilentMode(message->flags);
}

/*! \brief Process notifications received from CCP library */
static void callControlClient_HandleCcpMessage(Message message)
{
    CsrBtCmPrim ccp_id = *(CsrBtCmPrim *)message;
    const CcpReadCallStateCfm *msg_cfm = NULL;
    const CcpCallStateInd *msg_ind = NULL;

    switch (ccp_id)
    {
        case CCP_INIT_CFM:
            callControlClient_HandleCallProfileInitConfirmation((const CcpInitCfm*)message);
        break;

        case CCP_DESTROY_CFM:
            callControlClient_HandleCallProfileDestroyConfirmation((const CcpDestroyCfm*)message);
        break;

        case CCP_READ_CONTENT_CONTROL_ID_CFM:
            callControlClient_HandleReadContentControlIdCfm((const CcpReadContentControlIdCfm*)message);
        break;

        case CCP_READ_FEATURE_AND_STATUS_FLAGS_CFM:
            callControlClient_HandleReadFeatureAndStatusFlagsCfm((const CcpReadFlagsCfm*)message);
        break;

        case CCP_WRITE_CALL_CONTROL_POINT_CFM:
            callControlClient_HandleCallControlPointWriteConfirmation((const CcpSetCfm*)message);
        break;

        case CCP_READ_CALL_STATE_CFM:
            msg_cfm = (const CcpReadCallStateCfm*)message;
            if (msg_cfm->status == GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS)
            {
                callControlClient_UpdateCallState(msg_cfm->prflHndl, msg_cfm->callStateList, msg_cfm->callStateListSize);
            }
        break;

        case CCP_CALL_STATE_IND:
            msg_ind = (const CcpCallStateInd*)message;
            callControlClient_UpdateCallState(msg_ind->prflHndl, msg_ind->callStateList, msg_ind->callStateListSize);
        break;

        case CCP_TERMINATION_REASON_IND:
            callControlClient_HandleCallTerminationReasonNotification((const CcpTerminationReasonInd*)message);
        break;

        case CCP_FLAGS_IND:
            callControlClient_HandleFlagsInd((const CcpFlagsInd*)message);
        break;

        default:
            CCP_LOG("callControlClient_HandleCcpMessage Unhandled message id: 0x%x", (*(CsrBtCmPrim *)message));
        break;
    }
}

/*! \brief Create the Call Control Client Instance */
static void callControlClient_CreateInstance(gatt_cid_t cid)
{
    call_control_client_instance_t *instance = NULL;
    CcpHandles ccp_handle_data;
    CcpInitData client_init_params;
    GattTelephoneBearerClientDeviceData *tbs_client_handle = NULL;

    memset(&ccp_handle_data, 0, sizeof(CcpHandles));
    instance = CallControlClient_GetInstance(call_client_compare_by_state, call_client_state_idle);

    if (instance != NULL)
    {
        instance->cid = cid;
        instance->state = call_client_state_discovery;

        tbs_client_handle = (GattTelephoneBearerClientDeviceData*)CallControlClient_RetrieveClientHandles(cid);
        if (tbs_client_handle)
        {
            memcpy(&ccp_handle_data.tbsHandle, (void*)tbs_client_handle, sizeof(GattTelephoneBearerClientDeviceData));
        }

        client_init_params.cid = cid;
        CcpInitReq(TrapToOxygenTask((Task)&call_control_taskdata.task_data),
                    &client_init_params,
                    tbs_client_handle == NULL ? NULL : &ccp_handle_data,
                    FALSE);
    }
}

/*! \brief If GTBS Service is discovered successfully, initialize the Call control profile */
static void callControlClient_HandleServiceRange(const GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *message)
{
    CCP_LOG("callControlClient_HandleServiceRange Number of LEA Services cid: 0x%x, result: %d, Found: %d",
            message->cid, message->result, message->srvcInfoCount);

    if (message->result == GATT_SD_RESULT_SUCCESS && message->srvcInfoCount != 0)
    {
        if (message->srvcInfoCount == 1 && (message->srvcInfo->srvcId & GATT_SD_TBS_SRVC))
        {
            callControlClient_CreateInstance(message->cid);
        }
        else
        {
            CCP_LOG("callControlClient_HandleServiceRange unique GTBS Service not found in Remote Server CID: %d", message->cid);
        }

        pfree(message->srvcInfo);
    }
}

/*! \brief Handler to handle GATT Service Discovery related primitives */
static void callControlClient_HandleGattPrim(Message message)
{
    switch (*(CsrBtCmPrim *)message)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
            callControlClient_HandleServiceRange((const GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T*)message);
        break;

        default:
        break;
    }
}

/*! \brief Check if the Call control service is available in remote server and is discovered */
void CallControlClient_ConnectProfile(gatt_cid_t cid)
{
    CCP_LOG("CallControlClient_ConnectProfile cid: 0x%x", cid);

    if (CallControlClient_RetrieveClientHandles(cid) != NULL)
    {
        CCP_LOG("CallControlClient_ConnectProfile create instance");
        /* If handles are already there, it means we can directly create the instance */
        callControlClient_CreateInstance(cid);
    }
    else
    {
        /* Client handles not exists, find the service range first before creating instance */
        CCP_LOG("CallControlClient_ConnectProfile start find service range");
        GattServiceDiscoveryFindServiceRange(TrapToOxygenTask((Task)&call_control_taskdata.task_data),
                                             cid,
                                             GATT_SD_TBS_SRVC);
    }
}

/*! \brief Executes the assigned action on the connected call profile */
static void callControlClient_ActionOnConnectedProfile(gatt_cid_t cid, call_control_action action_fn, void *action_param)
{
    call_control_client_instance_t *instance = NULL;

    for (instance = &call_control_taskdata.call_client_instance[0];
         instance < &call_control_taskdata.call_client_instance[MAX_CALL_SERVER_SUPPORTED];
         instance++)
    {
        if (instance->state == call_client_state_connected &&
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

/*! \brief When pairing completes with handset, register for call state notifications */
static void callControlClient_HandlePairingActivity(const PAIRING_ACTIVITY_T *message)
{
    call_control_client_instance_t *instance = NULL;

    if (message->status != pairingActivitySuccess)
    {
        return;
    }

    instance = CallControlClient_GetInstance(call_client_compare_by_bdaddr, (unsigned)&message->device_addr);

    if (instance != NULL)
    {
        CCP_LOG("callControlClient_HandlePairingActivity Registering for CID 0x%x", instance->cid);
        CcpSetCallStateNotificationRequest(instance->ccp_profile_handle, TRUE);
        CcpSetTerminationReasonNotificationRequest(instance->ccp_profile_handle, TRUE);
        CcpSetCallControlPointNotificationRequest(instance->ccp_profile_handle, TRUE);
        CcpSetFlagsNotificationRequest(instance->ccp_profile_handle, TRUE);
        CcpReadContentControlIdRequest(instance->ccp_profile_handle);
        CcpReadStatusAndFeatureFlagsRequest(instance->ccp_profile_handle);
        CcpReadCallStateRequest(instance->ccp_profile_handle);
        CcpReadIncomingCallRequest(instance->ccp_profile_handle);
        CcpReadCallFriendlyNameRequest(instance->ccp_profile_handle);
        CcpReadCallControlPointOptionalOpcodesRequest(instance->ccp_profile_handle);

        callControlClient_WriteCallControlDeviceDataToStore(instance);
        callControlClient_SetLeVoiceSourceProperty(instance);
    }
}

/*! \brief Process notifications received for Call control client task */
static void CallControlClient_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    CCP_LOG("CallControlClient_HandleMessage Received Message Id : 0x%x", id);

    switch (id)
    {
        case CCP_PROFILE_PRIM:
            callControlClient_HandleCcpMessage(message);
        break;

        case GATT_SD_PRIM:
            callControlClient_HandleGattPrim(message);
        break;

        case PAIRING_ACTIVITY:
            callControlClient_HandlePairingActivity((const PAIRING_ACTIVITY_T*)message);
        break;

        case CCP_INTERNAL_OUT_OF_BAND_RINGTONE_REQ:
            callControlClient_PlayRingtone();
        break;

        default:
        break;
    }
}

static void callControlPoint_AcceptIncomingCall(call_control_client_instance_t *instance, call_control_set_t *set_value)
{
    for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
    {
        if (CcpSm_GetCallState(instance, idx) == CCP_CALL_STATE_INCOMING)
        {
            CCP_LOG("callControlPoint_AcceptIncomingCall callId: %d", instance->call_state[idx].tbs_call_id);
            CcpWriteCallControlPointSimpleRequest(instance->ccp_profile_handle, set_value->op,
                instance->call_state[idx].tbs_call_id);
            break;
        }
    }
}

static void callControlPoint_TerminateCall(call_control_client_instance_t *instance, call_control_set_t *set_value)
{
    CCP_LOG("callControlPoint_TerminateCall callId: 0x%x ", set_value->val);
    if (set_value->val != 0)
    {
        CcpWriteCallControlPointSimpleRequest(instance->ccp_profile_handle,
                                              set_value->op,
                                              set_value->val);
    }
}

static void callControlPoint_LocalHold(call_control_client_instance_t *instance, call_control_set_t *set_value)
{
    ccp_call_state_t call_state = CcpSm_GetCallStateForMatchingCallId(instance, set_value->val);

    CCP_LOG("callControlPoint_LocalHold callId: 0x%x callState:  enum:ccp_call_state_t:%d ", set_value->val, call_state);
    if (call_state == CCP_CALL_STATE_INCOMING ||
        call_state == CCP_CALL_STATE_ACTIVE ||
        call_state == CCP_CALL_STATE_REMOTELY_HELD)
    {
        CcpWriteCallControlPointSimpleRequest(instance->ccp_profile_handle, set_value->op, set_value->val);
    }
}

static void callControlPoint_LocalRetrieve(call_control_client_instance_t *instance, call_control_set_t *set_value)
{
    ccp_call_state_t call_state = CcpSm_GetCallStateForMatchingCallId(instance, set_value->val);

    CCP_LOG("callControlPoint_LocalRetrieve callId: 0x%x callState: enum:ccp_call_state_t:%d ", set_value->val, call_state);
    if (call_state == CCP_CALL_STATE_LOCALLY_HELD ||
        call_state == CCP_CALL_STATE_LOCALLY_REMOTELY_HELD)
    {
        CcpWriteCallControlPointSimpleRequest(instance->ccp_profile_handle, set_value->op, set_value->val);
    }
}

static void callControlPoint_OriginateCall(call_control_client_instance_t *instance, call_control_set_t *set_value)
{
    UNUSED(instance);
    UNUSED(set_value);
}

static void callControlPoint_JoinCall(call_control_client_instance_t *instance, call_control_set_t *set_value)
{
    uint8 tbs_call_index[MAX_ACTIVE_CALLS_SUPPORTED] = {0};
    uint8 count = 0;

    for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
    {
        ccp_call_state_t call_state = CcpSm_GetCallState(instance, idx);

        if (call_state == CCP_CALL_STATE_ACTIVE ||
            call_state == CCP_CALL_STATE_LOCALLY_HELD ||
            call_state == CCP_CALL_STATE_REMOTELY_HELD ||
            call_state == CCP_CALL_STATE_LOCALLY_REMOTELY_HELD)
        {
            tbs_call_index[count++] = instance->call_state[idx].tbs_call_id;
        }
    }

    if (count > 1)
    {
        uint8 size = count;
        CcpWriteCallControlPointRequest(instance->ccp_profile_handle, set_value->op, size, tbs_call_index);
    }
}

/*! \brief Action function that sends the opcode to the remote Server */
static void callControlPoint_SetOpcode(call_control_client_instance_t *instance, void *action_param)
{
    call_control_set_t *set_value = (call_control_set_t *) action_param;

    CCP_LOG("callControlPoint_SetOpcode Opcode:0x%x cid: 0x%x", set_value->op, instance->cid);
    /* Set control point interface.*/
    switch (set_value->op)
    {
        case TBS_CCP_ACCEPT:
            callControlPoint_AcceptIncomingCall(instance, set_value);
        break;

        case TBS_CCP_TERMINATE :
            callControlPoint_TerminateCall(instance, set_value);
        break;

        case TBS_CCP_LOCAL_HOLD :
            callControlPoint_LocalHold(instance, set_value);
        break;

        case TBS_CCP_LOCAL_RETRIEVE :
            callControlPoint_LocalRetrieve(instance, set_value);
        break;

        case TBS_CCP_ORIGINATE :
            callControlPoint_OriginateCall(instance, set_value);
        break;

        case TBS_CCP_JOIN :
            callControlPoint_JoinCall(instance, set_value);
        break;

        default :
            break;
    }
}

/*! \brief Register with GATT LEA Service discovery */
void CallControlClient_Init(void)
{
    CCP_LOG("CallControlClient_Init");
    memset(&call_control_taskdata, 0, sizeof(call_control_taskdata));
    call_control_taskdata.task_data.handler = CallControlClient_HandleMessage;

    GattConnect_RegisterObserver(&gatt_call_client_callback);
    Pairing_ActivityClientRegister(&call_control_taskdata.task_data);
    GattServiceDiscovery_RegisterServiceForDiscovery(GATT_SD_TBS_SRVC);
}

/*! \brief Send the call control opcode to the remote server */
void CallControlClient_SendCallControlOpcode(gatt_cid_t cid, GattTbsOpcode op, int32 val)
{
    call_control_set_t set_value;

    set_value.op = op;
    set_value.val = val;

    CCP_LOG("CallControlClient_SendCallControlOpcode OPCODE:0x%x, callId:%d", op, val);
    callControlClient_ActionOnConnectedProfile(cid, callControlPoint_SetOpcode, &set_value);
}

void CallControlClient_TerminateCalls(voice_source_t source, call_terminate_type_t terminate_type)
{
    call_control_set_t set_value;
    call_control_client_instance_t *instance = NULL;

    instance = CallControlClient_GetInstance(call_client_compare_by_voice_source, (unsigned) source);

    if (instance == NULL)
    {
        return;
    }

    set_value.op = TBS_CCP_TERMINATE;

    for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
    {
        ccp_call_state_t call_state = CcpSm_GetCallState(instance, idx);

        if (terminate_type == CCP_TERMINATE_ALL_CALLS && call_state != CCP_CALL_STATE_IDLE)
        {
            set_value.val = instance->call_state[idx].tbs_call_id;
        }
        else if (terminate_type == CCP_HANGUP_ONGOING_CALLS &&
                 CallControlClient_IsCallOngoing(call_state))
        {
            set_value.val = instance->call_state[idx].tbs_call_id;
        }
        else if (terminate_type == CCP_REJECT_INCOMING_CALL &&
                 call_state == CCP_CALL_STATE_INCOMING)
        {
            set_value.val = instance->call_state[idx].tbs_call_id;
        }
        else if (terminate_type == CCP_TERMINATE_HELD_CALLS &&
                 CallControlClient_IsCallHeld(call_state))
        {
            set_value.val = instance->call_state[idx].tbs_call_id;
        }
        else
        {
            continue;
        }

        callControlPoint_TerminateCall(instance, &set_value);
    }
}

static voice_source_provider_context_t callControlClient_DeriveVoiceContext(voice_source_provider_context_t voice_context, ccp_call_state_t call_state)
{
    switch (voice_context)
    {
        case context_voice_connected:
            switch (call_state)
            {
                case CCP_CALL_STATE_IDLE:
                    return context_voice_connected;

                case CCP_CALL_STATE_INCOMING:
                    return context_voice_ringing_incoming;

                case CCP_CALL_STATE_OUTGOING_DIALING:
                case CCP_CALL_STATE_OUTGOING_ALERTING:
                    return context_voice_ringing_outgoing;

                case CCP_CALL_STATE_ACTIVE:
                    return context_voice_in_call;

                case CCP_CALL_STATE_LOCALLY_HELD:
                case CCP_CALL_STATE_REMOTELY_HELD:
                case CCP_CALL_STATE_LOCALLY_REMOTELY_HELD:
                    return context_voice_call_held;

                default:
                    break;
            }
            break;

        case context_voice_ringing_incoming:
            switch (call_state)
            {
                case CCP_CALL_STATE_IDLE:
                case CCP_CALL_STATE_INCOMING:
                case CCP_CALL_STATE_OUTGOING_DIALING:
                case CCP_CALL_STATE_OUTGOING_ALERTING:
                case CCP_CALL_STATE_LOCALLY_HELD:
                case CCP_CALL_STATE_REMOTELY_HELD:
                case CCP_CALL_STATE_LOCALLY_REMOTELY_HELD:
                    return context_voice_ringing_incoming;

                case CCP_CALL_STATE_ACTIVE:
                    return context_voice_in_call_with_incoming;

                default:
                    break;
            }
        break;

        case context_voice_ringing_outgoing:
            switch (call_state)
            {
                case CCP_CALL_STATE_IDLE:
                case CCP_CALL_STATE_OUTGOING_DIALING:
                case CCP_CALL_STATE_OUTGOING_ALERTING:
                case CCP_CALL_STATE_LOCALLY_HELD:
                case CCP_CALL_STATE_REMOTELY_HELD:
                case CCP_CALL_STATE_LOCALLY_REMOTELY_HELD:
                    return context_voice_ringing_outgoing;

                case CCP_CALL_STATE_INCOMING:
                    return context_voice_ringing_incoming;

                case CCP_CALL_STATE_ACTIVE:
                    return context_voice_in_call_with_outgoing;

                default:
                    break;
            }
        break;

        case context_voice_in_call:
            switch (call_state)
            {
                case CCP_CALL_STATE_IDLE:
                    return context_voice_in_call;

                case CCP_CALL_STATE_INCOMING:
                    return context_voice_in_call_with_incoming;

                case CCP_CALL_STATE_ACTIVE:
                    return context_voice_in_multiparty_call;

                case CCP_CALL_STATE_OUTGOING_DIALING:
                case CCP_CALL_STATE_OUTGOING_ALERTING:
                    return context_voice_in_call_with_outgoing;

                case CCP_CALL_STATE_LOCALLY_HELD:
                case CCP_CALL_STATE_REMOTELY_HELD:
                case CCP_CALL_STATE_LOCALLY_REMOTELY_HELD:
                    return context_voice_in_call_with_held;

                default:
                    break;
            }
        break;

        case context_voice_call_held:
            switch (call_state)
            {
                case CCP_CALL_STATE_IDLE:
                case CCP_CALL_STATE_LOCALLY_HELD:
                case CCP_CALL_STATE_REMOTELY_HELD:
                case CCP_CALL_STATE_LOCALLY_REMOTELY_HELD:
                    return context_voice_call_held;

                case CCP_CALL_STATE_INCOMING:
                    return context_voice_ringing_incoming;

                case CCP_CALL_STATE_ACTIVE:
                    return context_voice_in_call_with_held;

                case CCP_CALL_STATE_OUTGOING_DIALING:
                case CCP_CALL_STATE_OUTGOING_ALERTING:
                    return context_voice_ringing_outgoing;

                default:
                    break;
            }
        break;

        default:
            break;
    }
    return voice_context;
}

unsigned CallClientControl_GetContext(voice_source_t source)
{
    call_control_client_instance_t *instance;
    voice_source_provider_context_t le_voice_context = context_voice_disconnected;

    instance = CallControlClient_GetInstance(call_client_compare_by_voice_source, (unsigned) source);

    if (instance != NULL)
    {
        le_voice_context = context_voice_connected;
        for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
        {
            ccp_call_state_t cur_call_state = CcpSm_GetCallState(instance, idx);
            le_voice_context = callControlClient_DeriveVoiceContext(le_voice_context, cur_call_state);
        }
    }

    CCP_LOG("CallClientControl_GetContext source: enum:voice_source_t:%d, instance:%p, le_voice_context: enum:voice_source_provider_context_t:%d",
            source, instance, le_voice_context);

    return le_voice_context;
}

bool CallClientControl_IsCallContextIdle(voice_source_t source)
{
    voice_source_provider_context_t le_voice_context = CallClientControl_GetContext(source);

    return (le_voice_context == context_voice_disconnected || le_voice_context == context_voice_connected);
}

/*! \brief Set the call characteristics notifications */
void CallControlClient_SetCallCharacteristicsNotification(gatt_cid_t cid, ccp_notification_id_t type, bool notification_enable)
{
    call_control_client_instance_t *instance;

    instance = CallControlClient_GetInstance(call_client_compare_by_valid_invalid_cid, (unsigned)cid);

    if (instance != NULL)
    {

        CCP_LOG("CallControlClient_SetCallCharacteristicsNotification cid: 0x%x type enum:ccp_notification_id_t:%d", cid, type);

        switch(type)
        {
            case CCP_NOTIFICATION_ID_PROVIDER_NAME:
                CcpSetProviderNameNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            case CCP_NOTIFICATION_ID_TECHNOLOGY:
                CcpSetTechnologyNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            case CCP_NOTIFICATION_ID_SIGNAL_STRENGTH:
                CcpSetSignalStrengthNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            case CCP_NOTIFICATION_ID_LIST_CURRENT_CALL:
                CcpSetListCurrentCallsNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            case CCP_NOTIFICATION_ID_FLAGS:
                CcpSetFlagsNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            case CCP_NOTIFICATION_ID_INCOMING_CALL_TARGET_BEARER_URI:
                CcpSetIncomingCallTargetBearerUriNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            case CCP_NOTIFICATION_ID_CALL_STATE:
                CcpSetCallStateNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            case CCP_NOTIFICATION_ID_CALL_CONTROL_POINT:
                CcpSetCallControlPointNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            case CCP_NOTIFICATION_ID_TERMINATION_REASON:
                CcpSetTerminationReasonNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            case CCP_NOTIFICATION_ID_INCOMING_CALL:
                CcpSetIncomingCallNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            case CCP_NOTIFICATION_ID_CALL_FRIENDLY_NAME:
                CcpSetCallFriendlyNameNotificationRequest(instance->ccp_profile_handle, notification_enable);
            break;

            default : break;
        }
    }
}

/*! \brief Read the call characteristics */
void CallControlClient_ReadCallCharacteristics(gatt_cid_t cid, ccp_read_characteristics_id_t id)
{
    call_control_client_instance_t *instance;

    instance = CallControlClient_GetInstance(call_client_compare_by_valid_invalid_cid, (unsigned)cid);

    if (instance != NULL)
    {
        CCP_LOG("CallControlClient_ReadCallCharacteristics cid: 0x%x type enum:ccp_read_characteristics_id_t:%d", cid, id);

        switch(id)
        {
            case CCP_READ_PROVIDER_NAME:
                CcpReadProviderNameRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_BEARER_UCI:
                CcpReadBearerUciRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_BEARER_TECHNOLOGY:
                CcpReadBearerTechnologyRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST:
                CcpReadBearerUriRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_SIGNAL_STRENGTH:
                CcpReadSignalStrengthRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_SIGNAL_STRENGTH_INTERVAL:
                CcpReadSignalStrengthIntervalRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_CURRENT_CALLS_LIST:
                CcpReadCurrentCallsRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_CONTENT_CONTROL_ID:
                CcpReadContentControlIdRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_FEATURE_AND_STATUS_FLAGS:
                CcpReadStatusAndFeatureFlagsRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_INCOMING_CALL_TARGET_BEARER_URI:
                CcpReadIncomingTargetBearerUriRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_CALL_STATE:
                CcpReadCallStateRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_INCOMING_CALL:
                CcpReadIncomingCallRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_CALL_FRIENDLY_NAME:
                CcpReadCallFriendlyNameRequest(instance->ccp_profile_handle);
            break;

            case CCP_READ_CCP_OPTIONAL_OPCODES:
                CcpReadCallControlPointOptionalOpcodesRequest(instance->ccp_profile_handle);
            break;

            default : break;
        }
    }
}

/*! \brief Send call control point opcode */
void CallControlClient_SendCallControlPointOpcode(gatt_cid_t cid, uint8 Opcode, uint8 callid1, uint8 callid2)
{
    call_control_client_instance_t *instance;
    uint8 tbs_call_index[MAX_ACTIVE_CALLS_SUPPORTED] = {0};

    instance = CallControlClient_GetInstance(call_client_compare_by_valid_invalid_cid, (unsigned)cid);

    if (instance != NULL)
    {
        CCP_LOG("CallControlClient_SendCallControlPointOpcode Opcode:0x%x cid: 0x%x", Opcode, instance->cid);

        switch (Opcode)
        {
            case TBS_CCP_ACCEPT:
            case TBS_CCP_TERMINATE :
            case TBS_CCP_LOCAL_HOLD :
            case TBS_CCP_LOCAL_RETRIEVE :
                if (callid1 != 0)
                {
                    CcpWriteCallControlPointSimpleRequest(instance->ccp_profile_handle, Opcode, callid1);
                }
            break;

            case TBS_CCP_JOIN :
                if (callid1 != 0 && callid2 != 0)
                {
                    tbs_call_index[0] = callid1;
                    tbs_call_index[1] = callid2;
                    CcpWriteCallControlPointRequest(instance->ccp_profile_handle, Opcode, 2, tbs_call_index);
                }
            break;

            default : break;
        }
    }
}

/*! \brief Prints all calls info */
void CallControlClient_PrintAllCallsInfo(void)
{
    call_control_client_instance_t *call_client = NULL;
    bdaddr addr;

    for (call_client = &call_control_taskdata.call_client_instance[0];
         call_client < &call_control_taskdata.call_client_instance[MAX_CALL_SERVER_SUPPORTED];
         call_client++)
    {
        if (call_client->state == call_client_state_connected)
        {
            GattConnect_GetPublicAddrFromConnectionId(call_client->cid, &addr);
            CCP_LOG("CallControlClient_PrintAllCallsInfo cid: 0x%x, bdaddr: %04x::%02x::%08x",
                    call_client->cid, addr.uap, addr.nap, addr.lap);
            for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
            {
                CCP_LOG("CallControlClient_PrintAllCallsInfo call_id: %d, call_state enum:ccp_call_state_t:%d",
                        call_client->call_state[idx].tbs_call_id, call_client->call_state[idx].state);
            }
        }
    }
}

/*! \brief Release any call which is held/waiting */
void CallControlClient_TwcReleaseHeldRejectWaiting(voice_source_t source)
{
    call_control_client_instance_t *instance;

    instance = CallControlClient_GetInstance(call_client_compare_by_voice_source, (unsigned) source);

    if (instance != NULL)
    {
        for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
        {
            ccp_call_state_t call_state = CcpSm_GetCallState(instance, idx);
            if (call_state == CCP_CALL_STATE_LOCALLY_HELD ||
                call_state == CCP_CALL_STATE_REMOTELY_HELD ||
                call_state == CCP_CALL_STATE_LOCALLY_REMOTELY_HELD ||
                call_state == CCP_CALL_STATE_INCOMING)
            {
                /* Release the held / reject the waiting call*/
                CcpWriteCallControlPointSimpleRequest(instance->ccp_profile_handle, TBS_CCP_TERMINATE,
                    instance->call_state[idx].tbs_call_id);
                CCP_LOG("CallControlClient_TwcReleaseHeldRejectWaiting call_id: %d",
                    instance->call_state[idx].tbs_call_id);
            }
        }
    }
}

static void CallControlClient_TwcAcceptOther(voice_source_t source, GattTbsOpcode terminate_or_hold)
{
    call_control_client_instance_t *instance;

    instance = CallControlClient_GetInstance(call_client_compare_by_voice_source, (unsigned) source);

    if (instance == NULL)
    {
        return;
    }

    for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
    {
        ccp_call_state_t call_state = CcpSm_GetCallState(instance, idx);

        if (call_state == CCP_CALL_STATE_ACTIVE)
        {
            /* Terminate/Hold the call in active state */
            CcpWriteCallControlPointSimpleRequest(instance->ccp_profile_handle, terminate_or_hold,
                instance->call_state[idx].tbs_call_id);

            CCP_LOG("CallControlClient_TwcAcceptOther call_id: %d opcode: %d", instance->call_state[idx].tbs_call_id, terminate_or_hold);
        }
        else if (call_state == CCP_CALL_STATE_INCOMING)
        {
            /* Accept the incoming call */
            CCP_LOG("CallControlClient_TwcAcceptOther accept call: %d", instance->call_state[idx].tbs_call_id);
            CcpWriteCallControlPointSimpleRequest(instance->ccp_profile_handle, TBS_CCP_ACCEPT, instance->call_state[idx].tbs_call_id);
        }
        else if (call_state == CCP_CALL_STATE_LOCALLY_HELD ||
                 call_state == CCP_CALL_STATE_LOCALLY_REMOTELY_HELD)
        {
            /* Retrieve the held call */
            CCP_LOG("CallControlClient_TwcAcceptOther local retrieve call: %d", instance->call_state[idx].tbs_call_id);
            CcpWriteCallControlPointSimpleRequest(instance->ccp_profile_handle, TBS_CCP_LOCAL_RETRIEVE, instance->call_state[idx].tbs_call_id);
        }
    }
}

/*! \brief Release the active call and accept incoming */
void CallControlClient_TwcReleaseActiveAcceptOther(voice_source_t source)
{
    CallControlClient_TwcAcceptOther(source, TBS_CCP_TERMINATE);
}

/*! \brief Hold the active call and accept incoming */
void CallControlClient_TwcHoldActiveAcceptOther(voice_source_t source)
{
    CallControlClient_TwcAcceptOther(source, TBS_CCP_LOCAL_HOLD);
}

/*! \brief Joins all the calls in call control clients instance */
void CallControlClient_TwcJoinCalls(voice_source_t source)
{
    uint8 tbs_call_index[MAX_ACTIVE_CALLS_SUPPORTED] = {0};
    uint8 count = 0;
    call_control_client_instance_t *instance;

    instance = CallControlClient_GetInstance(call_client_compare_by_voice_source, (unsigned) source);
    if (instance == NULL)
    {
        return;
    }

    for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
    {
        ccp_call_state_t call_state = CcpSm_GetCallState(instance, idx);

        if (call_state == CCP_CALL_STATE_ACTIVE ||
            call_state == CCP_CALL_STATE_LOCALLY_HELD ||
            call_state == CCP_CALL_STATE_REMOTELY_HELD ||
            call_state == CCP_CALL_STATE_LOCALLY_REMOTELY_HELD)
        {
            tbs_call_index[count++] = instance->call_state[idx].tbs_call_id;
        }
    }

    if (count > 1)
    {
        CCP_LOG("CallControlClient_TwcJoinCalls join %d calls", count);
        CcpWriteCallControlPointRequest(instance->ccp_profile_handle, TBS_CCP_JOIN, count, tbs_call_index);
    }
}

/*! \brief Get the Call client instance based on the compare type */
call_control_client_instance_t * CallControlClient_GetInstance(call_instance_compare_by_type_t type, unsigned cmp_value)
{
    call_control_client_instance_t *instance = NULL;

    for (instance = &call_control_taskdata.call_client_instance[0];
         instance < &call_control_taskdata.call_client_instance[MAX_CALL_SERVER_SUPPORTED];
         instance++)
    {
        if (callControlClient_Compare(type, cmp_value, instance))
        {
            return instance;
        }
    }

    return NULL;
}

void * CallControlClient_RetrieveClientHandles(gatt_cid_t cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    void *server_handle_info = NULL;

    if (device)
    {
        size_t size;

        if (!Device_GetProperty(device, device_property_call_control_client, &server_handle_info, &size))
        {
            server_handle_info = NULL;
        }
    }

    return server_handle_info;
}

bool CallControlClient_StoreClientHandles(gatt_cid_t cid, void *config, uint8 size)
{
    bool handles_written = FALSE;
    device_t device = GattConnect_GetBtDevice(cid);

    if (device)
    {
        Device_SetProperty(device, device_property_call_control_client, config, size);
        DeviceDbSerialiser_SerialiseDevice(device);
        handles_written = TRUE;
    }

    return handles_written;
}

/*! \brief Reset the provided call client instance */
void CallControlClient_ResetCallClientInstance(call_control_client_instance_t *call_client)
{
    if (call_client != NULL)
    {
        memset(call_client, 0, sizeof(call_control_client_instance_t));
        call_client->cid = INVALID_CID;
    }
}

/* Get the content control id for call control client */
uint16 CallClientControl_GetContentControlId(gatt_cid_t cid)
{
    call_control_client_instance_t *instance;
    uint16 content_control_id = 0;

    instance = CallControlClient_GetInstance(call_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL)
    {
        content_control_id = instance ->content_control_id;
    }

    return content_control_id;
}

/*! \brief Check If CCP is connected or not */
bool CallClientControl_IsCcpConnected(void)
{
    call_control_client_instance_t *instance;

    instance = CallControlClient_GetInstance(call_client_compare_by_valid_invalid_cid, (unsigned)INVALID_CID);

    if (instance != NULL)
    {
        return TRUE;
    }

    return FALSE;
}

/*! \brief Refresh the call state for ccp */
void CallControlClient_RefreshCallState(voice_source_t source)
{
    call_control_client_instance_t *instance;
    tp_bdaddr tp_bd_addr = {0};
    gatt_cid_t cid;

    device_t device = CallControlClient_FindDeviceFromVoiceSource(source);
    CCP_LOG("CallControlClient_RefreshCallState device=%p ", device);

    if (device != NULL)
    {
        BtDevice_GetTpBdaddrForDevice(device, &tp_bd_addr);
        cid = GattConnect_GetConnectionIdFromTpaddr(&tp_bd_addr);
        instance = CallControlClient_GetInstance(call_client_compare_by_cid, (unsigned)cid);

        if (instance != NULL)
        {
            CcpReadCallStateRequest(instance->ccp_profile_handle);
        }
    }
}

/*! \brief Find Device from voice source */
device_t CallControlClient_FindDeviceFromVoiceSource(voice_source_t source)
{
    return DeviceList_GetFirstDeviceWithPropertyValue(device_property_le_voice_source, &source, sizeof(voice_source_t));
}

/*! \brief Initiate a call using number */
void CallControlClient_InitiateCallUsingNumber(voice_source_t source, uint8 *digits, unsigned number_of_digits)
{
    call_control_client_instance_t *instance;
    const char *prefix = "tel:";
    const char *suffix = ",";
    char *buffer = NULL;
    char *dial_command = NULL;
    unsigned size_dial_command;

    instance = CallControlClient_GetInstance(call_client_compare_by_voice_source, (unsigned) source);
    if (instance == NULL)
    {
        return;
    }

    if (digits == NULL || number_of_digits == 0)
    {
        /* Use fixed number to originate the call */
        digits = CCP_FIXED_NUMBER;
        number_of_digits = CCP_SIZE_OF_FIXED_NUMBER;
    }

    size_dial_command = CCP_SIZE_OF_PREFIX + number_of_digits + CCP_SIZE_OF_SUFFIX;

    /* Create the Originate call command */
    dial_command = (char *) PanicUnlessMalloc(size_dial_command);

    buffer = dial_command;
    memmove(buffer, prefix, CCP_SIZE_OF_PREFIX);
    buffer += CCP_SIZE_OF_PREFIX;
    memmove(buffer, digits, number_of_digits);
    buffer += number_of_digits;
    memmove(buffer, suffix, CCP_SIZE_OF_SUFFIX);

    CCP_LOG("CallControlClient_InitiateCallUsingNumber dial_command size :%d, dial_command : %s", size_dial_command, dial_command);

    /* Send the call control point request*/
    CcpWriteCallControlPointRequest(instance->ccp_profile_handle, TBS_CCP_ORIGINATE, size_dial_command, (uint8*)dial_command);

    /* Free up the allocated memory since the cmd has been copied into the TBSC buffer */
    free(dial_command);
}


