/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_cap
    \brief      CAP - Client Role Implementation.
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "cap_profile_client_instance.h"
#include "pddu_map.h"
#include "device.h"
#include "bt_device.h"
#include "device_properties.h"
#include "device_db_serialiser.h"
#include "gatt_connect.h"
#include "bt_types.h"
#include "cap_client_prim.h"
#include "cap_client_lib.h"
#include "local_addr.h"

#include <logging.h>
#include <panic.h>

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif


#define CAP_PROFILE_CLIENT_INVALID_GROUP_HANDLE    0
#define CAP_QHS_CODEC_CONFIG_LENGTH                2
#define CAP_QHS_CODEC_TX_CONFIG_TYPE               3
#define CAP_USECASE_CODEC_CONFIG_LENGTH            2
#define CAP_USECASE_CODEC_CONFIG_TYPE              2
#define CAP_USECASE_IDENTIFIER_GAMING              1
#define CAP_USECASE_IDENTIFIER_GAMING_VBC          2
#define CAP_QHS_CODEC_RX_CONFIG_TYPE               4

#define CAP_CLIENT_BROADCAST_LC3_BLOCKS_PER_SDU    1
#define CAP_CLIENT_NUM_SUBGROUPS_SUPPORTED         1
#define CAP_CLIENT_PRESENTATION_DELAY              0x4E20
#define CAP_CLIENT__PA_INTERVAL_DEFAULT            360    /* 450ms */
#define CAP_CLIENT_NUMBER_OF_SINK_INFO_SUPPORTED   1
#define CAP_CLIENT_SCAN_PARAM_DEFAULT              0
#define CAP_CLIENT_CODEC_PARAM_DEFAULT             0
#define CAP_CLIENT_FRAME_DURATION_DEFAULT          10000
#define CAP_CLIENT_FRAME_LEN_DEFAULT               0x64
#define CAP_CLIENT_SAMPLE_RATE_DEFAULT             48000

/*! Default broadcast advertising interval in range 20ms to 10.24 sec( Interval = N * 0.625ms) */
#define CAP_CLIENT_SOURCE_ADV_INTERVAL_MIN         0x0080  /* 80 ms */
#define CAP_CLIENT_SOURCE_ADV_INTERVAL_MAX         0x00C0  /* 120 ms */

/*! \brief Periodic advertisement interval max and min to use (PA interval = N * 1.25ms) */
#define CAP_CLIENT_SOURCE_PA_INTERVAL_DELTA        16     /* 20ms */
#define CAP_CLIENT_SOURCE_PA_INTERVAL_MAX          (CAP_CLIENT__PA_INTERVAL_DEFAULT + CAP_CLIENT_SOURCE_PA_INTERVAL_DELTA)
#define CAP_CLIENT_SOURCE_PA_INTERVAL_MIN          (CAP_CLIENT__PA_INTERVAL_DEFAULT - CAP_CLIENT_SOURCE_PA_INTERVAL_DELTA)

/*! \brief Default advertising TX power value 0x7F(Host has no preference) */
#define  CAP_CLIENT_SOURCE_PA_DEFAULT_ADVERTISING_TX_POWER     0x7f

/*! \brief QHS rate to use for transmission */
typedef enum
{
    cap_qhs_type_qhs2,
    cap_qhs_type_qhs3,
    cap_qhs_type_qhs4,
    cap_qhs_type_qhs5,
    cap_qhs_type_qhs6
} cap_qhs_type_t;

#define CAP_QHS_TYPE_FOR_APTX_LITE      cap_qhs_type_qhs5

#define capProfileClient_IsCapabilitySupported(capability_data, requested_capability) \
            (((capability_data) & (requested_capability)) == requested_capability)

/*! \brief CAP client task data. */
cap_profile_client_task_data_t cap_client_taskdata;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
static const uint8 aptx_lite_gaming_qhs_config_type[] = {CAP_QHS_CODEC_CONFIG_LENGTH, CAP_QHS_CODEC_TX_CONFIG_TYPE, CAP_QHS_TYPE_FOR_APTX_LITE,
                                                         CAP_QHS_CODEC_CONFIG_LENGTH, CAP_USECASE_CODEC_CONFIG_TYPE, CAP_USECASE_IDENTIFIER_GAMING};

static const uint8 aptx_lite_gaming_with_vbc_qhs_config_type[] = {CAP_QHS_CODEC_CONFIG_LENGTH, CAP_QHS_CODEC_TX_CONFIG_TYPE, CAP_QHS_TYPE_FOR_APTX_LITE,
                                                                  CAP_USECASE_CODEC_CONFIG_LENGTH, CAP_USECASE_CODEC_CONFIG_TYPE, CAP_USECASE_IDENTIFIER_GAMING_VBC,
                                                                  CAP_QHS_CODEC_CONFIG_LENGTH, CAP_QHS_CODEC_RX_CONFIG_TYPE, CAP_QHS_TYPE_FOR_APTX_LITE};
#endif

/*! \brief Handler that receives notification from CAP Profile library */
static void capProfileClient_HandleMessage(Task task, MessageId id, Message message);
cap_profile_group_instance_t* CapProfileClient_GetGroupInstance(ServiceHandle group_handle);

/*! Sends CAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT to registered clients */
static void capProfileClient_SendDisconnectCfm(ServiceHandle group_handle, bool disconnected)
{
    cap_profile_client_msg_t msg;

    msg.id = CAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT;
    msg.body.disconnect_complete.group_handle = group_handle;
    msg.body.disconnect_complete.status = disconnected ? CAP_PROFILE_CLIENT_STATUS_SUCCESS : CAP_PROFILE_CLIENT_STATUS_FAILED;

    cap_client_taskdata.callback_handler(group_handle, &msg);
}

/*! Sends CAP_PROFILE_CLIENT_INIT_CFM to registered clients */
static void capProfileClient_SendInitCfm(ServiceHandle group_handle, cap_profile_client_status_t status, uint8 set_size, uint8 connected_devices)
{
    cap_profile_client_msg_t msg;

    msg.id = CAP_PROFILE_CLIENT_MSG_ID_INIT_COMPLETE;
    msg.body.init_complete.group_handle = group_handle;
    msg.body.init_complete.total_devices = set_size;
    msg.body.init_complete.connected_devices = connected_devices;
    msg.body.init_complete.status = status;

    cap_client_taskdata.callback_handler(group_handle, &msg);
}

/*! Sends CAP_PROFILE_CLIENT_MSG_ID_DEVICE_ADDED to registered clients */
static void capProfileClient_SendDeviceAddedInd(ServiceHandle group_handle, gatt_cid_t cid, bool device_added, bool more_devices_needed)
{
    cap_profile_client_msg_t msg;

    msg.id = CAP_PROFILE_CLIENT_MSG_ID_DEVICE_ADDED;
    msg.body.device_added.cid = cid;
    msg.body.device_added.group_handle = group_handle;
    msg.body.device_added.status = device_added ? CAP_PROFILE_CLIENT_STATUS_SUCCESS : CAP_PROFILE_CLIENT_STATUS_FAILED;
    msg.body.device_added.more_devices_needed = more_devices_needed;

    cap_client_taskdata.callback_handler(group_handle, &msg);
}

/*! Sends CAP_PROFILE_CLIENT_MSG_ID_DEVICE_REMOVED to registered clients */
static void capProfileClient_SendDeviceRemovedInd(ServiceHandle group_handle, gatt_cid_t cid, bool removed, bool more_devices_present)
{
    cap_profile_client_msg_t msg;

    msg.id = CAP_PROFILE_CLIENT_MSG_ID_DEVICE_REMOVED;
    msg.body.device_removed.cid = cid;
    msg.body.device_removed.status = removed ? CAP_PROFILE_CLIENT_STATUS_SUCCESS : CAP_PROFILE_CLIENT_STATUS_FAILED;
    msg.body.device_removed.more_devices_present = more_devices_present;

    cap_client_taskdata.callback_handler(group_handle, &msg);
}

/*! Sends Close request to destroy all the CAP profile instances in the given group */
static void capProfileClient_CloseGroupInstance(ServiceHandle group_handle)
{
    CapProfileClient_DestroyInstance(group_handle, 0);
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
static bool capProfileClient_IsSampleRateSupported(cap_profile_group_instance_t *group_instance,
                                                   uint8 direction, uint32 sample_rate)
{
    int i;
    CapClientStreamCapability *cap_info = group_instance->cap_info.capability;

    DEBUG_LOG("capProfileClient_IsSampleRateSupported: direction 0x%x, capability 0x%x", direction, sample_rate);

    for (i = 0; i < group_instance->cap_info.stream_cap_count; i++)
    {
        if (cap_info->direction == direction && cap_info->capability & sample_rate)
        {
            return TRUE;
        }
        cap_info++;
    }

    return FALSE;
}

/*! \brief Check if the given audio context contains multiple contexts */
static bool CapProfileClient_IsGivenContextsHaveMultipleContexts(CapClientContext audio_context)
{
    uint8 set_bit_count = 0, bit_count;

    /* If audio context contains more than one bit set, then return TRUE */
    for (bit_count = 0; bit_count < 15; bit_count++)
    {
        if (audio_context & 0x01)
        {
            set_bit_count++;
        }

        audio_context = audio_context >> 1;
    }

    return set_bit_count > 1;
}

/*! \brief Get the first stream capability supported for the given type (source/sink) */
static CapClientSreamCapability capProfileClient_GetFirstStreamCapability(ServiceHandle group_handle, CapClientAseType ase_type)
{
    int i;
    cap_profile_group_instance_t *group_instance;
    CapClientStreamCapability *stream_capability_info;
    CapClientSreamCapability stream_capability = CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN;

    group_instance = CapProfileClient_GetGroupInstance(group_handle);
    PanicNull(group_instance);
    stream_capability_info = group_instance->cap_info.capability;

    for (i = 0; i < group_instance->cap_info.stream_cap_count; i++)
    {
        if (stream_capability_info->direction == ase_type &&
            stream_capability_info->capability != CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)
        {
            /* If multiple capabilities are supported, return the first capability */
            stream_capability = stream_capability_info->capability;
            break;
        }
        stream_capability_info++;
    }

    DEBUG_LOG("capProfileClient_GetFirstStreamCapability ase_type:%d stream_capability 0x%x",
               ase_type, stream_capability);

    return stream_capability;
}

/*! \brief Get the number of source ASEs */
static uint8 capProfileClient_GetSourceAseCount(ServiceHandle group_handle)
{
    uint8 source_count = 0, i;
    cap_profile_group_instance_t *group_instance = NULL;
    CapClientDeviceInfo *device_info;

    group_instance = CapProfileClient_GetGroupInstance(group_handle);
    PanicNull(group_instance);
    device_info = group_instance->cap_info.device_list;

    for (i = 0; i < group_instance->cap_info.device_count; i++)
    {
        if (device_info->direction == CAP_CLIENT_ASE_SOURCE)
        {
            /* Todo: Better way to counts the number of sources based on the number of bits
               set in device_info->audioLocation */
            if (device_info->audioLocation & CAP_CLIENT_AUDIO_LOCATION_FL)
            {
                source_count++;
            }
            if (device_info->audioLocation & CAP_CLIENT_AUDIO_LOCATION_FR)
            {
                source_count++;
            }
        }
        device_info++;
    }

    DEBUG_LOG("capProfileClient_GetSourceAseCount %d", source_count);

    return source_count;
}

static CapClientAudioLocation capProfileClient_GetAudioLocationForAseType(ServiceHandle group_handle,
                                                                          CapClientAseType ase_type)
{
    int i;
    cap_profile_group_instance_t *group_instance;
    CapClientDeviceInfo *device_info;
    CapClientAudioLocation audio_locations = CAP_CLIENT_AUDIO_LOCATION_MONO;

    group_instance = CapProfileClient_GetGroupInstance(group_handle);
    PanicNull(group_instance);
    device_info = group_instance->cap_info.device_list;

    for (i = 0; i < group_instance->cap_info.device_count; i++)
    {
        if (device_info->direction == ase_type)
        {
            audio_locations |= device_info->audioLocation;
        }
        device_info++;
    }

    DEBUG_LOG("capProfileClient_GetAudioLocationForAseType ase_type:%d audio location 0x%x",
               ase_type, audio_locations);

    return audio_locations;
}
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

static CapClientAudioLocation capProfileClient_GetSourceAudioLocation(ServiceHandle group_handle)
{
    int i;
    cap_profile_group_instance_t *group_instance;
    CapClientDeviceInfo *device_info;

    group_instance = CapProfileClient_GetGroupInstance(group_handle);
    PanicNull(group_instance);
    device_info = group_instance->cap_info.device_list;

    for (i=0; i < group_instance->cap_info.device_count; i++)
    {
        if (device_info->direction == CAP_CLIENT_ASE_SOURCE)
        {
            DEBUG_LOG("capProfileClient_GetSourceAudioLocation() audio location %x", device_info->audioLocation);
            return device_info->audioLocation;
        }
        device_info++;
    }
    /* return default audio location as MONO */
    return CAP_CLIENT_AUDIO_LOCATION_MONO;
}

/*! \brief Function that checks whether the CAP client instance matches based on the compare type */
static bool capProfileClient_Compare(cap_profile_client_instance_compare_by_type_t type,
                                     unsigned compare_value,
                                     cap_profile_client_device_info_t *instance)
{
    bool found = FALSE;

    switch (type)
    {
        case cap_profile_client_compare_by_cid:
            found = instance->cid == (gatt_cid_t) compare_value;
        break;

        case cap_profile_client_compare_by_state:
            found = instance->state == (cap_profile_client_state_t) compare_value;
        break;

        case cap_profile_client_compare_by_bdaddr:
        {
            bdaddr addr;
            bdaddr *device_addr = (bdaddr *) compare_value;
            found = instance->state == cap_profile_client_state_connected &&
                    GattConnect_GetPublicAddrFromConnectionId(instance->cid, &addr) &&
                    BdaddrIsSame(&addr, device_addr);
        }
        break;

        case cap_profile_client_compare_by_valid_invalid_cid:
            found = instance->state == cap_profile_client_state_connected &&
                    (instance->cid == (gatt_cid_t) compare_value || compare_value == INVALID_CID);
        break;

        default:
        break;
    }

    return found;
}


/*! \brief Get the CAP group instance based on the group handle */
cap_profile_group_instance_t* CapProfileClient_GetGroupInstance(ServiceHandle group_handle)
{
    int i;

    for (i = 0; i < MAX_CAP_GROUP_SUPPORTED; i++)
    {
        if (cap_client_taskdata.cap_group_instance[i].cap_group_handle == group_handle)
        {
            return &cap_client_taskdata.cap_group_instance[i];
        }
    }

    return NULL;
}

/*! \brief Get the CAP Profile CIg ID based on the group_handle */
uint8 CapProfileClient_GetCigId(ServiceHandle group_handle)
{
    cap_profile_group_instance_t *group_instance = CapProfileClient_GetGroupInstance(group_handle);

    PanicNull(group_instance);

    return group_instance->cig_id;
}

/*! \brief Get the CAP Profile instance based on the compare type */
cap_profile_client_device_info_t * CapProfileClient_GetDeviceInstance(cap_profile_group_instance_t *group_instance,
                                                                      cap_profile_client_instance_compare_by_type_t type, unsigned cmp_value)
{
    cap_profile_client_device_info_t *instance = NULL;

    PanicNull(group_instance);

    for (instance = &group_instance->device_info[0];
         instance < &group_instance->device_info[MAX_CAP_DEVICES_SUPPORTED];
         instance++)
    {
        if (capProfileClient_Compare(type, cmp_value, instance))
        {
            return instance;
        }
    }

    return NULL;
}

static void capProfileClient_RegisterAsPersistentDeviceDataUser(void)
{
    /* ToDo */
}

/*! \brief Write all the CAS Service handles to NVM */
static void capProfileClient_WriteDeviceDataToStore(cap_profile_client_device_info_t *instance, CapClientHandleList* handles)
{
    UNUSED(instance);
    UNUSED(handles);

    /* ToDo */
}

/*! \brief Reset all the CAP client instances in the given group */
static void CapProfileClient_ResetAllCapClientInstancesInGroup(cap_profile_group_instance_t *group_instance)
{
    uint8 i;

    if (group_instance->cap_info.capability != NULL)
    {
        for (i = 0; i < group_instance->cap_info.stream_cap_count; i++)
        {
            pfree(group_instance->cap_info.capability[i].metadata);
        }

        pfree(group_instance->cap_info.capability);
        DEBUG_LOG("CapProfileClient_ResetAllCapClientInstancesInGroup");
    }

    pfree(group_instance->cap_info.device_list);
    memset(group_instance, 0, sizeof(cap_profile_group_instance_t));

    for (i = 0; i < MAX_CAP_DEVICES_SUPPORTED; i++)
    {
        group_instance->device_info[i].cid = INVALID_CONNID;
    }
}

static void capProfileClient_HandleCapInitCfm(CapClientInitCfm *cfm)
{
    bool status;
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_device_info_t *instance;

    group_instance = CapProfileClient_GetGroupInstance(CAP_PROFILE_CLIENT_INVALID_GROUP_HANDLE);
    PanicNull(group_instance);
    instance = CapProfileClient_GetDeviceInstance(group_instance, cap_profile_client_compare_by_state, cap_profile_client_state_discovery);

    /* Initialise group instance with the values received */
    group_instance->cap_group_handle = cfm->groupId;
    group_instance->device_count = cfm->deviceCount;
    group_instance->coordinated_set_size = (cfm->setAttrib != NULL) ? cfm->setAttrib->setSize : 0;

    status = (cfm->result == CAP_CLIENT_RESULT_SUCCESS || cfm->result == CAP_CLIENT_RESULT_SUCCESS_DISCOVERY_ERR);

    DEBUG_LOG("capProfileClient_HandleProfileInitConfirmation: status: 0x%x set size:%d",
               cfm->result, group_instance->coordinated_set_size);

    /* Indicate registered clients on the devie add status */
    capProfileClient_SendDeviceAddedInd(group_instance->cap_group_handle, cfm->cid, status,
                                        group_instance->device_count < group_instance->coordinated_set_size);

    if (status && instance != NULL)
    {
        instance->cid = cfm->cid;

        if (cfm->result == CAP_CLIENT_RESULT_SUCCESS_DISCOVERY_ERR)
        {
            /* Clear the group handles as without CAS there is no group */
            group_instance->cap_group_handle = 0;
            capProfileClient_SendInitCfm(0, CAP_PROFILE_CLIENT_STATUS_FAILED_AS_CAS_NOT_PRESENT, group_instance->coordinated_set_size, 0);
        }
        else
        {
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
            CapClientRegisterTaskReq(TrapToOxygenTask((Task) &cap_client_taskdata.task_data), group_instance->cap_group_handle);
#endif
            if (group_instance->coordinated_set_size > 1)
            {
                memcpy(group_instance->sirk, cfm->setAttrib->sirk, sizeof(group_instance->sirk));

                /* Indicate that initialisation succeeded but needs to discover other members in the coordinated set*/
                instance->state = cap_profile_client_state_connected;
            }
            else
            {
                /* There is no coordinated set or only one member. So we can proceed further without waiting to discover
                   other coordinated members */
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
                CapClientInitStreamControlReq(group_instance->cap_group_handle);
#endif
            }
        }
    }
    else
    {
        /* An error occurred during CAP initialization. Reset the CAP client instance */
        capProfileClient_SendInitCfm(cfm->groupId, CAP_PROFILE_CLIENT_STATUS_FAILED, group_instance->coordinated_set_size, 0);
        CapProfileClient_ResetAllCapClientInstancesInGroup(group_instance);
    }

    pfree(cfm->setAttrib);
}

static void capProfileClient_HandleAddNewDeviceCfm(CapClientAddNewDevCfm *cfm)
{
    cap_profile_client_device_info_t *instance;
    cap_profile_group_instance_t *group_instance;

    group_instance = CapProfileClient_GetGroupInstance(cfm->groupId);
    PanicNull(group_instance);
    instance = CapProfileClient_GetDeviceInstance(group_instance, cap_profile_client_compare_by_state, cap_profile_client_state_discovery);
    PanicNull(instance);

    DEBUG_LOG("capProfileClient_HandleAddNewDeviceCfm: device_count:%d status: 0x%x", cfm->deviceCount, cfm->result);

    /* Send indication to registered clients on the device added */
    capProfileClient_SendDeviceAddedInd(cfm->groupId,
                                        instance->cid,
                                        cfm->result == CAP_CLIENT_RESULT_SUCCESS,
                                        cfm->deviceCount < group_instance->coordinated_set_size);

    if (cfm->result == CAP_CLIENT_RESULT_SUCCESS)
    {
        group_instance->device_count = cfm->deviceCount;

        if (group_instance->device_count == group_instance->coordinated_set_size)
        {
            /* Proceed with further initialisation as all set members got added */
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
            CapClientInitStreamControlReq(group_instance->cap_group_handle);
#endif
        }
    }
    else
    {
        /* An error occurred during CAP initialization. Reset the CAP client instance */
        capProfileClient_SendInitCfm(group_instance->cap_group_handle, CAP_PROFILE_CLIENT_STATUS_FAILED,
                                     group_instance->coordinated_set_size, 0);
        capProfileClient_CloseGroupInstance(group_instance->cap_group_handle);
    }
}

static void capProfileClient_HandleInitStreamControlCfm(CapClientInitStreamControlCfm *message)
{
    cap_profile_group_instance_t *group_instance;

    DEBUG_LOG("capProfileClient_HandleInitStreamControlCfm: status: 0x%x", message->result);

    group_instance = CapProfileClient_GetGroupInstance(message->groupId);
    PanicNull(group_instance);

    if (message->result == CAP_CLIENT_RESULT_SUCCESS)
    {
        group_instance->role = message->role;
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        CapClientDiscoverStreamCapabilitiesReq(message->groupId, CAP_CLIENT_PUBLISHED_CAPABILITY_ALL);
#endif
    }
    else
    {
        capProfileClient_SendInitCfm(message->groupId, CAP_PROFILE_CLIENT_STATUS_FAILED,
                                     group_instance->coordinated_set_size, 0);
        capProfileClient_CloseGroupInstance(message->groupId);
    }

    if (message->deviceStatusLen != 0)
    {
        pfree(message->deviceStatus);
    }
}

static void capProfileClient_HandleDiscoverStreamCapabilitiesCfm(CapClientDiscoverStreamCapabilitiesCfm *message)
{
    cap_profile_group_instance_t *group_instance;

    DEBUG_LOG("capProfileClient_HandleDiscoverStreamCapabilitiesCfm: status: 0x%x deviceInfoCount:%d", message->result, message->deviceInfoCount);

    group_instance = CapProfileClient_GetGroupInstance(message->groupId);
    PanicNull(group_instance);

    if (message->result == CAP_CLIENT_RESULT_SUCCESS)
    {
        if (message->deviceInfoCount != 0)
        {
            group_instance->cap_info.device_count = message->deviceInfoCount;
            group_instance->cap_info.device_list = message->deviceInfo;
        }

        if (message->streamCapCount != 0)
        {
            int i;
            group_instance->cap_info.stream_cap_count = message->streamCapCount;
            group_instance->cap_info.capability = message->capability;

            DEBUG_LOG("capProfileClient_HandleDiscoverStreamCapabilitiesCfm: stream_cap_count %d", group_instance->cap_info.stream_cap_count);

            for (i=0; i < group_instance->cap_info.stream_cap_count; i++)
            {
                DEBUG_LOG("Record %d Direction  :%s : Capability : 0x%04x : Context : 0x%04x ",
                         i + 1, message->capability[i].direction == CAP_CLIENT_ASE_SINK? "Sink": "Source",
                         message->capability[i].capability, message->capability[i].context);
            }
        }

        group_instance->cap_info.supported_context = message->supportedContext;
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        CapClientDiscoverAvailableAudioContextReq(message->groupId);
#endif
    }
    else
    {
        capProfileClient_SendInitCfm(message->groupId, CAP_PROFILE_CLIENT_STATUS_FAILED, group_instance->coordinated_set_size, 0);
        capProfileClient_CloseGroupInstance(group_instance->cap_group_handle);
    }
}

static void capProfileClient_HandleAvailableAudioContextCfm(CapClientDiscoverAvailableAudioContextCfm *message)
{
    int i;
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_device_info_t *dev_instance;


    DEBUG_LOG("capProfileClient_HandleAvailableAudioContextCfm: status: 0x%x", message->status);

    group_instance = CapProfileClient_GetGroupInstance(message->groupId);
    PanicNull(group_instance);

    if (message->status == CAP_CLIENT_RESULT_SUCCESS && message->deviceContextLen != 0)
    {
        CapClientAvailableAudioContextInfo *deviceContext = message->deviceContext;

        for (i = 0; i < message->deviceContextLen; i++)
        {
            dev_instance = CapProfileClient_GetDeviceInstance(group_instance, cap_profile_client_compare_by_cid,
                                                              deviceContext->cid);

            if (dev_instance != NULL)
            {
                /* Here we assume all devices in the group have the same audio contexts
                   and update to group instance */
                group_instance->available_audio_context = deviceContext->context;
                dev_instance->state = cap_profile_client_state_connected;
            }
            deviceContext++;
        }

        capProfileClient_SendInitCfm(message->groupId, CAP_PROFILE_CLIENT_STATUS_SUCCESS, group_instance->coordinated_set_size,
                                     group_instance->device_count);
    }
    else
    {
        capProfileClient_SendInitCfm(message->groupId, CAP_PROFILE_CLIENT_STATUS_FAILED, group_instance->coordinated_set_size, 0);
        capProfileClient_CloseGroupInstance(message->groupId);
    }

    if (message->deviceContextLen != 0)
    {
        pfree(message->deviceContext);
    }
}

static void capProfileClient_HandleCapTerminateCfm(CapClientRemoveDeviceCfm *message)
{
    cap_profile_client_device_info_t *instance;
    cap_profile_group_instance_t *group_instance;

    DEBUG_LOG("capProfileClient_HandleCapTerminateCfm: status: 0x%x", message->result);

    group_instance = CapProfileClient_GetGroupInstance(message->groupId);
    PanicNull(group_instance);
    instance = CapProfileClient_GetDeviceInstance(group_instance, cap_profile_client_compare_by_state,
                                                  cap_profile_client_state_disconnecting);
    PanicFalse(instance != NULL || group_instance->device_count == 0);

    capProfileClient_WriteDeviceDataToStore(instance, NULL);

    /* The received message is for a device destroy operation */
    group_instance->device_count--;
    capProfileClient_SendDeviceRemovedInd(group_instance->cap_group_handle,
                                          instance != NULL ? instance->cid : 0,
                                          message->result == CAP_CLIENT_RESULT_SUCCESS,
                                          group_instance->device_count > 0);
    CapProfileClient_ResetCapClientInstance(instance);


    if (group_instance->device_count <= 0)
    {
        /* All devices are now destroyed. Indicate to registered clients that profile is disconnected */
        capProfileClient_SendDisconnectCfm(group_instance->cap_group_handle, message->result == CAP_CLIENT_RESULT_SUCCESS);

        /* Reset the all instances in the group */
        CapProfileClient_ResetAllCapClientInstancesInGroup(group_instance);
    }
}

static void capProfileClient_UpdateAvailableAudioContext(CapClientAvailableAudioContextInd *message)
{
    cap_profile_group_instance_t *group_instance = CapProfileClient_GetGroupInstance(message->groupId);

    DEBUG_LOG("capProfileClient_UpdateAvailableAudioContext: Available Context : 0x%x", message->context);

    if (group_instance != NULL)
    {
       group_instance->available_audio_context = message->context;
    }
}

static void capProfileClient_HandleUnicastConnectCfm(CapClientUnicastConnectCfm *connect_cfm)
{
    cap_profile_group_instance_t *instance;
    cap_profile_client_msg_t msg;

    DEBUG_LOG("capProfileClient_HandleUnicastConnectCfm: 0x%x", connect_cfm->result);

    instance = CapProfileClient_GetGroupInstance(connect_cfm->groupId);
    PanicNull(instance);
    instance->cig_id = connect_cfm->cigId;

    msg.id = CAP_PROFILE_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE;
    msg.body.unicast_config_complete.group_handle = connect_cfm->groupId;
    msg.body.unicast_config_complete.status = (connect_cfm->result == CAP_CLIENT_RESULT_SUCCESS) ? CAP_PROFILE_CLIENT_STATUS_SUCCESS : CAP_PROFILE_CLIENT_STATUS_FAILED;
    msg.body.unicast_config_complete.audio_context = connect_cfm->context;

    cap_client_taskdata.callback_handler(instance->cap_group_handle, &msg);

    if (connect_cfm->deviceStatusLen != 0)
    {
        pfree(connect_cfm->deviceStatus);
    }
}

static void capProfileClient_HandleUnicastStartStreamCfm(CapClientUnicastStartStreamCfm *start_stream_cfm)
{
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_msg_t msg;

    DEBUG_LOG("capProfileClient_HandleUnicastStartStreamCfm: 0x%x", start_stream_cfm->result);

    group_instance = CapProfileClient_GetGroupInstance(start_stream_cfm->groupId);
    PanicNull(group_instance);

    msg.id = CAP_PROFILE_CLIENT_MSG_ID_UNICAST_STREAM_START;
    msg.body.unicast_stream_start.group_handle = group_instance->cap_group_handle;
    msg.body.unicast_stream_start.status = (start_stream_cfm->result == CAP_CLIENT_RESULT_SUCCESS) ? CAP_PROFILE_CLIENT_STATUS_SUCCESS : CAP_PROFILE_CLIENT_STATUS_FAILED;

    cap_client_taskdata.callback_handler(group_instance->cap_group_handle, &msg);
}

static void capProfileClient_HandleCisConnectInd(CapClientUnicastStartStreamInd *start_stream_ind)
{
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_msg_t msg;

    DEBUG_LOG("capProfileClient_HandleCisConnectInd: 0x%x", start_stream_ind->result);

    group_instance = CapProfileClient_GetGroupInstance(start_stream_ind->groupId);
    PanicNull(group_instance);

    msg.id = CAP_PROFILE_CLIENT_MSG_ID_UNICAST_CIS_CONNECT;
    msg.body.unicast_cis_connect.cid = start_stream_ind->cid;
    msg.body.unicast_cis_connect.cis_handles = (void *) start_stream_ind->cishandles;
    msg.body.unicast_cis_connect.codec_qos_config = (void *) start_stream_ind->audioConfig;
    msg.body.unicast_cis_connect.cis_count = start_stream_ind->cisCount;
    msg.body.unicast_cis_connect.status = (start_stream_ind->result == CAP_CLIENT_RESULT_SUCCESS) ? CAP_PROFILE_CLIENT_STATUS_SUCCESS : CAP_PROFILE_CLIENT_STATUS_FAILED;

    cap_client_taskdata.callback_handler(group_instance->cap_group_handle, &msg);
    pfree(start_stream_ind->cishandles);
    pfree(start_stream_ind->audioConfig);

    if (start_stream_ind->vsMetadata != NULL)
    {
        pfree(start_stream_ind->vsMetadata->srcVsMetadata);
        pfree(start_stream_ind->vsMetadata->sinkVsMetadata);
        pfree(start_stream_ind->vsMetadata);
    }
}

static void capProfileClient_HandleUnicastStopStreamCfm(CapClientUnicastStopStreamCfm *stop_stream_cfm)
{
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_msg_t msg;

    DEBUG_LOG("capProfileClient_HandleUnicastStopStreamCfm: 0x%x", stop_stream_cfm->result);

    group_instance = CapProfileClient_GetGroupInstance(stop_stream_cfm->groupId);
    PanicNull(group_instance);

    msg.id = CAP_PROFILE_CLIENT_MSG_ID_UNICAST_STREAM_STOP;
    msg.body.unicast_stream_stop.group_handle = group_instance->cap_group_handle;
    msg.body.unicast_stream_stop.status = (stop_stream_cfm->result == CAP_CLIENT_RESULT_SUCCESS) ? CAP_PROFILE_CLIENT_STATUS_SUCCESS : CAP_PROFILE_CLIENT_STATUS_FAILED;

    cap_client_taskdata.callback_handler(group_instance->cap_group_handle, &msg);

    if (stop_stream_cfm->deviceStatusLen != 0)
    {
        pfree(stop_stream_cfm->deviceStatus);
    }
}

static void capProfileClient_HandleUnicastCisLinkLossInd(CapClientUnicastLinkLossInd *message)
{
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_msg_t msg;

    DEBUG_LOG("capProfileClient_HandleUnicastCisLinkLossInd");

    group_instance = CapProfileClient_GetGroupInstance(message->activeGroupId);
    PanicNull(group_instance);

    msg.id = CAP_PROFILE_CLIENT_MSG_ID_CIS_LINK_LOSS;
    /* Todo: Needs to find out how to know in which connection link loss happened */
    msg.body.cis_link_loss.group_handle = group_instance->cap_group_handle;

    cap_client_taskdata.callback_handler(group_instance->cap_group_handle, &msg);
}

static void capProfileClient_HandleChangeVolumeCfm(CapClientChangeVolumeCfm *message)
{
    DEBUG_LOG("capProfileClient_HandleChangeVolumeCfm status = %d", message->result);

    if (message->deviceStatusLen != 0)
    {
        pfree(message->deviceStatus);
    }
}

static void capProfileClient_HandleVolumeStateInd(CapClientVolumeStateInd *message)
{
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_msg_t msg;

    DEBUG_LOG("capProfileClient_HandleVolumeStateInd volumeState = %d", message->volumeState);

    group_instance = CapProfileClient_GetGroupInstance(message->groupId);
    PanicNull(group_instance);

    msg.id = CAP_PROFILE_CLIENT_MSD_ID_VOLUME_STATE;
    msg.body.volume_state.group_handle = group_instance->cap_group_handle;
    msg.body.volume_state.volumeState = message->volumeState;
    msg.body.volume_state.mute = message->mute;

    cap_client_taskdata.callback_handler(group_instance->cap_group_handle, &msg);
}

static void capProfileClient_HandleMuteCfm(CapClientMuteCfm *message)
{
    DEBUG_LOG("capProfileClient_HandleMuteCfm status = %d", message->result);

    if (message->deviceStatusLen != 0)
    {
        pfree(message->deviceStatus);
    }
}

static void capProfileClient_HandleUnlockCoOrdinatedSetCfm(CapClientUnlockCoordinatedSetCfm *message)
{
    DEBUG_LOG("capProfileClient_HandleUnlockCoOrdinatedSetCfm status = %d", message->result);

    if (message->deviceStatusLen != 0)
    {
        pfree(message->deviceStatus);
    }
}

static bool capProfileClient_FindVsLtvOffset(uint8 * ltvData, uint8 ltvDataLength, uint8 type, uint8 * offset)
{
    uint8 ltvIndex = 0;

    *offset = 0;

    if (ltvData == NULL || ltvDataLength == 0)
    {
        return FALSE;
    }

    while(ltvIndex < ltvDataLength && ltvData[ltvIndex + CAP_LTV_LENGTH_OFFSET])
    {
        uint8 length = ltvData[ltvIndex + CAP_LTV_LENGTH_OFFSET];

        if(ltvData[ltvIndex + CAP_LTV_TYPE_OFFSET] == type)
        {
            *offset = ltvIndex;
            return TRUE;
        }
        else
        {
            ltvIndex += (1 + length);
        }
    }

    return FALSE;
}

static bool capProfileClient_FindLtvValue(uint8 * ltvData, uint8 ltvDataLength, uint8 type, uint8 * value, uint8 valueLength)
{
    bool ltvFound = FALSE;
    if(ltvData && ltvDataLength && value)
    {
        int ltvIndex = 0;
        while(ltvIndex < ltvDataLength && ltvFound == FALSE && ltvData[ltvIndex + CAP_LTV_LENGTH_OFFSET])
        {
            uint8 length = ltvData[ltvIndex + CAP_LTV_LENGTH_OFFSET];

            DEBUG_LOG("capProfileClient_FindLtvValue: index=%d length=%d type=0x%x\n", ltvIndex, ltvData[ltvIndex + CAP_LTV_LENGTH_OFFSET], ltvData[ltvIndex+CAP_LTV_TYPE_OFFSET]);

            if(ltvData[ltvIndex + CAP_LTV_TYPE_OFFSET] == type)
            {
                if(ltvData[ltvIndex + CAP_LTV_LENGTH_OFFSET] == (valueLength + 1))
                {
                    uint8 i;
                    for(i = 0; i < valueLength; i++)
                    {
                        value[i] = ltvData[ltvIndex + CAP_LTV_VALUE_OFFSET + i];
                    }
                    ltvFound = TRUE;
                }
                else
                {
                    DEBUG_LOG("capProfileClient_FindLtvValue: Unexpected length\n");
                    break;
                }
            }
            else
            {
                ltvIndex += (1 + length);
            }
        }
    }
    else
    {
        DEBUG_LOG("capProfileClient_FindLtvValue: Invalid LTV data\n");
    }
    DEBUG_LOG("capProfileClient_FindLtvValue: ltv_found=%d\n", ltvFound);
    return ltvFound;
}

static bool capProfileClient_ParseVsMetadataForFlushTimeoutReq(uint8 * metadata, uint8 metadataLength, cap_profile_ft_info_t * ft_info)
{
    uint8 vs_ltv_value[CAP_VS_METADATA_FT_REQUEST_VALUE_LENGTH] = {0};
    uint8 vs_ltv_offset = 0;

    DEBUG_LOG("capProfileClient_ParseVsMetadataForFlushTimeoutReq()");

    if (capProfileClient_FindVsLtvOffset(metadata, metadataLength, CAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC, &vs_ltv_offset))
    {
        /* VS Metadata LTV present, check if FT request VS LTV type is available in it */
        if (capProfileClient_FindLtvValue(&metadata[vs_ltv_offset + CAP_LTV_VALUE_OFFSET + CAP_VS_METADATA_COMPANY_ID_QUALCOMM_SIZE],
                                          metadata[vs_ltv_offset + CAP_LTV_LENGTH_OFFSET] - (CAP_LTV_TYPE_OFFSET + CAP_VS_METADATA_COMPANY_ID_QUALCOMM_SIZE),
                                          CAP_VS_METADATA_TYPE_FT_REQUESTED_SETTING, vs_ltv_value, CAP_VS_METADATA_FT_REQUEST_VALUE_LENGTH))
        {
            ft_info->min_flush_timeout = vs_ltv_value[0];
            ft_info->max_flush_timeout = vs_ltv_value[1];
            ft_info->max_bit_rate = vs_ltv_value[2];
            ft_info->err_resilience = vs_ltv_value[3];
            ft_info->latency_mode =  vs_ltv_value[4];

            return TRUE;
        }
    }

    return FALSE;
}

static void capProfileClient_HandleUpdateMetadataInd(CapClientUpdateMetadataInd *message)
{
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_msg_t msg;

    DEBUG_LOG("capProfileClient_HandleUpdateMetadataInd status = %d", message->result);

    group_instance = CapProfileClient_GetGroupInstance(message->groupId);
    PanicNull(group_instance);

    if (message->metadataLen != 0)
    {
        cap_profile_ft_info_t flush_timeout_info;

        if (capProfileClient_ParseVsMetadataForFlushTimeoutReq(message->metadata, message->metadataLen, &flush_timeout_info))
        {
            DEBUG_LOG("capProfileClient_HandleUpdateMetadataInd VS FT Info Present");

            msg.id = CAP_PROFILE_CLIENT_MSD_ID_FLUSH_TIMEOUT_INFO;
            msg.body.flush_timeout_info.group_handle = message->groupId;
            msg.body.flush_timeout_info.ft_info = flush_timeout_info;

            cap_client_taskdata.callback_handler(group_instance->cap_group_handle, &msg);
        }
        pfree(message->metadata);
    }
}

static void capProfileClient_HandleSetVsConfigDataCfm(CapClientUnicastSetVsConfigDataCfm *message)
{
    DEBUG_LOG("capProfileClient_HandleSetVsConfigDataCfm status = %d", message->result);
}

static void capProfileClient_HandleUnicastDisconnectCfm(CapClientUnicastDisConnectCfm *message)
{
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_msg_t msg;

    DEBUG_LOG("capProfileClient_HandleUnicastDisconnectCfm status = %d", message->result);

    group_instance = CapProfileClient_GetGroupInstance(message->groupId);
    PanicNull(group_instance);

    msg.id = CAP_PROFILE_CLIENT_MSD_ID_UNICAST_CONFIG_REMOVED;
    msg.body.unicast_config_removed.group_handle = message->groupId;
    msg.body.unicast_config_removed.status = message->result == CAP_CLIENT_RESULT_SUCCESS ? CAP_PROFILE_CLIENT_STATUS_SUCCESS :
                                                                                            CAP_PROFILE_CLIENT_STATUS_FAILED;

    cap_client_taskdata.callback_handler(group_instance->cap_group_handle, &msg);
}

/*! \brief  Sets the periodic advertisement parameters for the PBP Source */
static void capProfileClient_SetDefaultAdvParam(void)
{
    CapClientBcastSrcAdvParams src_adv_param;

    src_adv_param.advEventProperties = 0;
    src_adv_param.advIntervalMin = CAP_CLIENT_SOURCE_ADV_INTERVAL_MIN;
    src_adv_param.advIntervalMax = CAP_CLIENT_SOURCE_ADV_INTERVAL_MAX;
    src_adv_param.primaryAdvPhy = BAP_LE_1M_PHY;
    src_adv_param.primaryAdvChannelMap = HCI_ULP_ADVERT_CHANNEL_DEFAULT;
    src_adv_param.secondaryAdvMaxSkip = 0;
    src_adv_param.secondaryAdvPhy = BAP_LE_1M_PHY;
    src_adv_param.advSid = CM_EXT_ADV_SID_ASSIGNED_BY_STACK;
    src_adv_param.periodicAdvIntervalMin = CAP_CLIENT_SOURCE_PA_INTERVAL_MIN;
    src_adv_param.periodicAdvIntervalMax = CAP_CLIENT_SOURCE_PA_INTERVAL_MAX;
    src_adv_param.advertisingTransmitPower = CAP_CLIENT_SOURCE_PA_DEFAULT_ADVERTISING_TX_POWER;

    CapClientBcastSrcSetAdvParamsReq(cap_client_taskdata.cap_broadcast_data.bcast_handle, &src_adv_param);
}

static void capProfileClient_BroadcastCommonCfm(const CapClientBcastAsstCommonMsg *cfm)
{
    pfree(cfm->status);
}

static void capProfileClient_BroadcastSrctInitCfm(const CapClientBcastSrcInitCfm *cfm)
{
    DEBUG_LOG("capProfileClient_BroadcastSrctInitCfm status %d", cfm->result);

    PanicFalse(cfm->bcastSrcProfileHandle != 0);

    if (cfm->result == CAP_CLIENT_RESULT_SUCCESS)
    {
        cap_client_taskdata.cap_broadcast_data.bcast_handle = cfm->bcastSrcProfileHandle;

        /* Set the default adv parameters for broadcast */
        capProfileClient_SetDefaultAdvParam();
    }
}

static void capProfileClient_BroadcastSrcStreamingStartCfm(const CapClientBcastSrcStartStreamCfm *cfm)
{
    uint8 index;

    DEBUG_LOG("capProfileClient_BroadcastSrcStreamingStartCfm status %d", cfm->result);

    PanicFalse(cfm->bcastSrcProfileHandle != 0);

    if (cfm->result == CAP_CLIENT_RESULT_SUCCESS)
    {
        for (index = 0; index < cfm->subGroupInfo->numBis; index++)
        {
            cap_client_taskdata.cap_broadcast_data.bis_handles[index] = cfm->subGroupInfo->bisHandles[index];
        }
    }

    pfree(cfm->bigParameters);

    if (cfm->subGroupInfo != NULL)
    {
        pfree(cfm->subGroupInfo->audioConfig);
        pfree(cfm->subGroupInfo->bisHandles);
        pfree(cfm->subGroupInfo->metadata);
        pfree(cfm->subGroupInfo);
    }
}

static void capProfileClient_BroadcastAsstScanStartCfm(const CapClientBcastAsstStartSrcScanCfm *cfm)
{
    cap_profile_group_instance_t *group_instance;

    DEBUG_LOG("capProfileClient_BroadcastAsstScanStartCfm status %d", cfm->result);

    group_instance = CapProfileClient_GetGroupInstance(cfm->groupId);
    PanicNull(group_instance);

    if (cfm->result == CAP_CLIENT_RESULT_SUCCESS)
    {
        cap_client_taskdata.cap_broadcast_data.scan_handle = cfm->scanHandle;
    }

    pfree(cfm->status);
}

static void capProfileClient_BroadcastAsstSourceScanReport(const CapClientBcastAsstSrcReportInd *report)
{
    cap_profile_client_broadcast_data_t *broadcast_data = &cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("capProfileClient_BroadcastAsstSourceScanReport");

    if (!broadcast_data->source_found)
    {
        broadcast_data->sync_handle = report->advHandle;
        broadcast_data->broadcast_id = report->broadcastId;
        broadcast_data->adv_sid = report->advSid;

        if (broadcast_data->scan_for_colllocated && report->collocated)
        {
            broadcast_data->source_found = TRUE;
        }
        else if (report->sourceAddrt.addr.lap ==  broadcast_data->source_addr.addr.lap &&
                 report->sourceAddrt.addr.nap ==  broadcast_data->source_addr.addr.nap &&
                 report->sourceAddrt.addr.uap ==  broadcast_data->source_addr.addr.uap)
        {
             broadcast_data->source_found = TRUE;
        }
    }

    if (report->subgroupInfo != NULL)
    {
        pfree(report->subgroupInfo->bisInfo);
        pfree(report->subgroupInfo);
    }

    pfree(report->bigName);
    pfree(report->serviceData);
}

static void capProfileClient_BroadcastAsstReadtReceiveStateInd(const CapClientBcastAsstReadReceiveStateInd *ind)
{
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_broadcast_data_t *broadcast_data = &cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("capProfileClient_BroadcastAsstReadtReceiveStateInd status %d", ind->result);

    group_instance = CapProfileClient_GetGroupInstance(ind->groupId);
    PanicNull(group_instance);

    if (ind->result == CAP_CLIENT_RESULT_SUCCESS)
    {
        broadcast_data->adv_sid = ind->advSid;
        broadcast_data->broadcast_id = ind->broadcastId;
        broadcast_data->source_id = ind->sourceId;
        broadcast_data->pa_sync_state = ind->paSyncState;
        broadcast_data->big_encryption = ind->bigEncryption;
        broadcast_data->source_addr.addr = ind->sourceAddress;
        broadcast_data->source_addr.type = ind->advertiseAddType;
    }

    pfree(ind->badCode);

    if (ind->subGroupInfo != NULL)
    {
        pfree(ind->subGroupInfo->metadataValue);
        pfree(ind->subGroupInfo);
    }
}

static void capProfileClient_BroadcastAsstBrsInd(const CapClientBcastAsstBrsInd *ind)
{
    cap_profile_group_instance_t *group_instance;

    cap_profile_client_broadcast_data_t *broadcast_data = &cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("capProfileClient_BroadcastAsstBrsInd");

    group_instance = CapProfileClient_GetGroupInstance(ind->groupId);
    PanicNull(group_instance);

    broadcast_data->adv_sid = ind->advSid;
    broadcast_data->broadcast_id = ind->broadcastId;
    broadcast_data->source_id = ind->sourceId;
    broadcast_data->pa_sync_state = ind->paSyncState;
    broadcast_data->big_encryption = ind->bigEncryption;

    broadcast_data->source_addr.addr = ind->sourceAddress;
    broadcast_data->source_addr.type = ind->advertiseAddType;

    pfree(ind->badCode);

    if (ind->subGroupInfo != NULL)
    {
        pfree(ind->subGroupInfo->metadataValue);
        pfree(ind->subGroupInfo);
    }
}

static void capProfileClient_BroadcastAsstSyncToSrcCfm(const CapClientBcastAsstSyncToSrcStartCfm *cfm)
{
    cap_profile_group_instance_t *group_instance;

    cap_profile_client_broadcast_data_t *broadcast_data = &cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("capProfileClient_BroadcastAsstSyncToSrcCfm status %d", cfm->result);

    group_instance = CapProfileClient_GetGroupInstance(cfm->groupId);
    PanicNull(group_instance);

    broadcast_data->sync_handle = cfm->syncHandle;
    broadcast_data->adv_sid = cfm->advSid;
}

static void capProfileClient_HandleCapMessage(Message message)
{
    CapClientPrim capId = *(CapClientPrim*)message;

    switch(capId)
    {
        case CAP_CLIENT_INIT_CFM:
            capProfileClient_HandleCapInitCfm((CapClientInitCfm *) message);
        break;

        case CAP_CLIENT_ADD_NEW_DEV_CFM:
            capProfileClient_HandleAddNewDeviceCfm((CapClientAddNewDevCfm *) message);
        break;

        case CAP_CLIENT_INIT_STREAM_CONTROL_CFM:
            capProfileClient_HandleInitStreamControlCfm((CapClientInitStreamControlCfm *) message);
        break;

        case CAP_CLIENT_DISCOVER_STREAM_CAPABILITIES_CFM:
            capProfileClient_HandleDiscoverStreamCapabilitiesCfm((CapClientDiscoverStreamCapabilitiesCfm *) message);
        break;

        case CAP_CLIENT_DISCOVER_AVAILABLE_AUDIO_CONTEXT_CFM:
            capProfileClient_HandleAvailableAudioContextCfm((CapClientDiscoverAvailableAudioContextCfm *) message);
        break;

        case CAP_CLIENT_REMOVE_DEV_CFM:
            capProfileClient_HandleCapTerminateCfm((CapClientRemoveDeviceCfm *) message);
        break;

        case CAP_CLIENT_AVAILABLE_AUDIO_CONTEXT_IND:
            capProfileClient_UpdateAvailableAudioContext((CapClientAvailableAudioContextInd *) message);
        break;

        case CAP_CLIENT_UNICAST_CONNECT_CFM:
            capProfileClient_HandleUnicastConnectCfm((CapClientUnicastConnectCfm *) message);
        break;

        case CAP_CLIENT_UNICAST_START_STREAM_IND:
            capProfileClient_HandleCisConnectInd((CapClientUnicastStartStreamInd *) message);
        break;

        case CAP_CLIENT_UNICAST_START_STREAM_CFM:
            capProfileClient_HandleUnicastStartStreamCfm((CapClientUnicastStartStreamCfm *) message);
        break;

        case CAP_CLIENT_UNICAST_STOP_STREAM_CFM:
            capProfileClient_HandleUnicastStopStreamCfm((CapClientUnicastStopStreamCfm *) message);
        break;

        case CAP_CLIENT_UNICAST_LINK_LOSS_IND:
            capProfileClient_HandleUnicastCisLinkLossInd((CapClientUnicastLinkLossInd *) message);
        break;

        case CAP_CLIENT_VOLUME_STATE_IND:
            capProfileClient_HandleVolumeStateInd((CapClientVolumeStateInd *) message);
        break;

        case CAP_CLIENT_CHANGE_VOLUME_CFM:
            capProfileClient_HandleChangeVolumeCfm((CapClientChangeVolumeCfm *) message);
        break;

        case CAP_CLIENT_MUTE_CFM:
            capProfileClient_HandleMuteCfm((CapClientMuteCfm *) message);
        break;

        case CAP_CLIENT_UNICAST_DISCONNECT_CFM:
            capProfileClient_HandleUnicastDisconnectCfm((CapClientUnicastDisConnectCfm *) message);
        break;

        case CAP_CLIENT_UNLOCK_COORDINATED_SET_CFM:
            capProfileClient_HandleUnlockCoOrdinatedSetCfm((CapClientUnlockCoordinatedSetCfm *) message);
        break;

        case CAP_CLIENT_UPDATE_METADATA_IND:
            capProfileClient_HandleUpdateMetadataInd((CapClientUpdateMetadataInd *) message);
        break;

        case CAP_CLIENT_UNICAST_SET_VS_CONFIG_DATA_CFM:
            capProfileClient_HandleSetVsConfigDataCfm((CapClientUnicastSetVsConfigDataCfm *) message);
        break;

        case CAP_CLIENT_BCAST_SRC_INIT_CFM:
             capProfileClient_BroadcastSrctInitCfm((const CapClientBcastSrcInitCfm *) message);
        break;

        case CAP_CLIENT_BCAST_SRC_START_STREAM_CFM:
             capProfileClient_BroadcastSrcStreamingStartCfm((const CapClientBcastSrcStartStreamCfm *) message);
        break;

        case CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM:
            capProfileClient_BroadcastAsstScanStartCfm((const CapClientBcastAsstStartSrcScanCfm *) message);
        break;

        case CAP_CLIENT_BCAST_ASST_SRC_REPORT_IND:
            capProfileClient_BroadcastAsstSourceScanReport((const CapClientBcastAsstSrcReportInd *) message);
        break;

        case CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM:
        case CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM:
        case CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM:
        case CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM:
        case CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM:
            capProfileClient_BroadcastCommonCfm((const CapClientBcastAsstCommonMsg *) message);
        break;

        case CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_IND:
            capProfileClient_BroadcastAsstReadtReceiveStateInd((const CapClientBcastAsstReadReceiveStateInd *) message);
        break;

        case CAP_CLIENT_BCAST_ASST_BRS_IND:
            capProfileClient_BroadcastAsstBrsInd((const CapClientBcastAsstBrsInd *) message);
        break;

        case CAP_CLIENT_BCAST_ASST_START_SYNC_TO_SRC_CFM:
            capProfileClient_BroadcastAsstSyncToSrcCfm((const CapClientBcastAsstSyncToSrcStartCfm *) message);
        break;

        default:
            DEBUG_LOG("capProfileClient_HandleCapMessage Unhandled Message Id : 0x%x", capId);
        break;
    }
}

/*! \brief Handler that receives notification from CAP Profile library */
static void capProfileClient_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    DEBUG_LOG("capProfileClient_HandleMessage Received Message Id : 0x%x", id);

    switch (id)
    {
        case CAP_CLIENT_PROFILE_PRIM:
            capProfileClient_HandleCapMessage(message);
        break;

        default:
        break;
    }
}

/*! \brief Initialise the CAP client component */
bool CapProfileClient_Init(cap_profile_client_callback_handler_t handler)
{
    DEBUG_LOG("CapProfileClient_Init");

    memset(&cap_client_taskdata, 0, sizeof(cap_profile_client_task_data_t));
    cap_client_taskdata.task_data.handler = capProfileClient_HandleMessage;
    cap_client_taskdata.callback_handler = handler;

    capProfileClient_RegisterAsPersistentDeviceDataUser();

    return TRUE;
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! \brief Create the CAP client instance */
bool CapProfileClient_CreateInstance(gatt_cid_t cid)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_device_info_t *instance = NULL;

    /* Get a group instance that is not set before */
    group_instance = CapProfileClient_GetGroupInstance(CAP_PROFILE_CLIENT_INVALID_GROUP_HANDLE);
    if (group_instance != NULL)
    {
        instance = CapProfileClient_GetDeviceInstance(group_instance, cap_profile_client_compare_by_state, cap_profile_client_state_idle);
    }

    DEBUG_LOG("CapProfileClient_CreateInstance Idle Instance 0x%04X", instance);

    if (instance != NULL)
    {
        CapClientInitData cap_init_data;

        cap_init_data.cid = cid;
        cap_init_data.handles = NULL;

        instance->cid = cid;
        instance->state = cap_profile_client_state_discovery;

        /* cap_init_data->handles = ((CapClientHandleList*) CapProfileClient_RetrieveClientHandles(cid); */
        CapClientInitReq(TrapToOxygenTask((Task) &cap_client_taskdata.task_data),
                         &cap_init_data,
                         CAP_CLIENT_INITIATOR | CAP_CLIENT_COMMANDER);

        status = TRUE;
    }

    return status;
}

bool CapProfileClient_ConfigureForGaming(ServiceHandle group_handle,
                                         uint32 sink_capability,
                                         uint32 source_capability,
                                         uint8 latency,
                                         uint8 mic_count,
                                         uint8 cap_config_mode,
                                         const CapClientQhsConfig *cig_qhs_config)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance;
    cap_profile_client_device_info_t *instance;

    DEBUG_LOG("CapProfileClient_ConfigureForGaming: group_handle=0x%04X, mic_count: %d, sink_capability 0x%x, source_capability 0x%x", group_handle, mic_count, sink_capability, source_capability);

    group_instance = CapProfileClient_GetGroupInstance(group_handle);
    PanicNull(group_instance);
    instance = CapProfileClient_GetDeviceInstance(group_instance, cap_profile_client_compare_by_state, cap_profile_client_state_connected);
    PanicNull(instance);

    if (capProfileClient_IsSampleRateSupported(group_instance, BAP_AUDIO_DIRECTION_SINK, sink_capability) &&
        (mic_count == 0 || capProfileClient_IsSampleRateSupported(group_instance, BAP_AUDIO_DIRECTION_SOURCE, source_capability)))
    {
        if (capProfileClient_IsCapabilitySupported(sink_capability, CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_48_1) ||
            capProfileClient_IsCapabilitySupported(source_capability, CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_16_1))
        {
            DEBUG_LOG("Aptx Lite Configured for Gaming");

            if (mic_count == 0)
            {
                CapClientUnicastSetVsConfigDataReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                                  group_handle, ARRAY_DIM(aptx_lite_gaming_qhs_config_type), aptx_lite_gaming_qhs_config_type);
            }
            else
            {
                CapClientUnicastSetVsConfigDataReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                                  group_handle, ARRAY_DIM(aptx_lite_gaming_with_vbc_qhs_config_type), aptx_lite_gaming_with_vbc_qhs_config_type);
            }
        }

        CapClientUnicastConnectReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                             group_handle,
                             sink_capability,
                             source_capability,
                             latency,
                             mic_count != 0 ? CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC : CAP_CLIENT_CONTEXT_TYPE_GAME,
                             CAP_CLIENT_AUDIO_LOCATION_FL | CAP_CLIENT_AUDIO_LOCATION_FR,
                             capProfileClient_GetSourceAudioLocation(group_handle),
                             mic_count,
                             cap_config_mode,
                             cig_qhs_config);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_RemoveGamingConfiguration(ServiceHandle group_handle, uint16 use_case)
{
    cap_profile_group_instance_t *group_instance;

    DEBUG_LOG("CapProfileClient_RemoveGamingConfiguration: group_handle=0x%04X, use_case: %x", group_handle, use_case);

    group_instance = CapProfileClient_GetGroupInstance(group_handle);
    PanicNull(group_instance);

    CapClientUnicastDisConnectReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                  group_instance->cap_group_handle, use_case);

    return TRUE;
}

bool CapProfileClient_StartUnicastStreaming(ServiceHandle group_handle, uint16 audio_context)
{
    cap_profile_group_instance_t *group_instance = CapProfileClient_GetGroupInstance(group_handle);
    bool status = FALSE;

    DEBUG_LOG("CapProfileClient_StartUnicastStreaming: group_handle=0x%04X", group_handle);

    if (group_instance != NULL)
    {
        CapClientUnicastStartStreamReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                 group_instance->cap_group_handle,
                                 audio_context,
                                 0,
                                 NULL);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_StopUnicastStreaming(ServiceHandle group_handle, bool remove_configured_context)
{
    cap_profile_group_instance_t *group_instance = CapProfileClient_GetGroupInstance(group_handle);
    bool status = FALSE;

    DEBUG_LOG("CapProfileClient_StopUnicastStreaming: group_handle=0x%04X", group_handle);

    if (group_instance != NULL)
    {
        CapClientUnicastStopStreamReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                group_instance->cap_group_handle,
                                remove_configured_context);
        status = TRUE;
    }

    return status;
}

/*! \brief Set the absolute volume on the remote device */
void CapProfileClient_SetAbsoluteVolume(ServiceHandle group_handle, uint8 volume)
{
    cap_profile_group_instance_t *group_instance;

    DEBUG_LOG("CapProfileClient_SetAbsoluteVolume: group_handle=0x%04X, volume = %d", group_handle, volume);
    group_instance = CapProfileClient_GetGroupInstance(group_handle);

    if (group_instance != NULL)
    {
        CapClientChangeVolumeReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                 group_handle,
                                 volume);
    }
}

/*! \brief Mute/Unmute the audio stream on the remote device */
void CapProfileClient_SetMute(ServiceHandle group_handle, bool mute)
{
    cap_profile_group_instance_t *group_instance;

    DEBUG_LOG("CapProfileClient_SetMute: group_handle=0x%04X, mute = %d", group_handle, mute);
    group_instance = CapProfileClient_GetGroupInstance(group_handle);

    if (group_instance != NULL)
    {
        CapClientMuteReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                         group_handle,
                         mute);
    }
}

bool CapProfileClient_AddDeviceToGroup(ServiceHandle group_handle, gatt_cid_t cid)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = NULL;
    cap_profile_client_device_info_t *instance = NULL;

    DEBUG_LOG("CapProfileClient_AddDeviceToGroup group_handle 0x%04X cid 0x%04X", group_handle, cid);

    /* Get a device instance with in the group which is not set before */
    group_instance = CapProfileClient_GetGroupInstance(group_handle);
    instance = CapProfileClient_GetDeviceInstance(group_instance, cap_profile_client_compare_by_state, cap_profile_client_state_idle);

    if (instance != NULL)
    {
        CapClientInitData cap_init_data;

        cap_init_data.cid = cid;
        cap_init_data.handles = NULL;

        instance->cid = cid;
        instance->state = cap_profile_client_state_discovery;

        CapClientAddNewDevReq(group_handle, &cap_init_data, FALSE);

        status = TRUE;
    }

    return status;
}

bool CapProfileClient_CompleteInitWithExistingDevices(ServiceHandle group_handle)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = NULL;

    DEBUG_LOG("CapProfileClient_CompleteInitWithExistingDevices group_handle 0x%04X", group_handle);

    group_instance = CapProfileClient_GetGroupInstance(group_handle);

    if (group_instance != NULL && group_instance->device_count != group_instance->coordinated_set_size)
    {
        /* Proceed with further initialisation */
        CapClientInitStreamControlReq(group_instance->cap_group_handle);
        status = TRUE;
    }

    return status;
}

void CapProfileClient_Configure(uint32 sink_capability,
                                uint32 source_capability,
                                uint8 latency,
                                uint16 use_case,
                                uint32 sink_audio_location,
                                uint32 src_audio_location,
                                uint8 mic_count,
                                uint8 cap_config_mode)
{
    cap_profile_group_instance_t *group_instance;

    DEBUG_LOG("CapProfileClient_Configure: sink_cap:0x%x source_cap:0x%x use_case:0x%x sink_loc:0x%x src_loc:0x%x mic:%d mode:%d",
               sink_capability, source_capability, use_case, sink_audio_location, src_audio_location, mic_count, cap_config_mode);

    group_instance = &cap_client_taskdata.cap_group_instance[0];

    CapClientUnicastConnectReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                               group_instance->cap_group_handle,
                               sink_capability,
                               source_capability,
                               latency,
                               use_case,
                               sink_audio_location,
                               src_audio_location,
                               mic_count,
                               cap_config_mode,
                               NULL);

}

bool CapProfileClient_ConfigurePtsForStreaming(uint16 preferred_audio_context, uint8 latency, uint8 cap_config_mode)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];
    CapClientSreamCapability sink_capability;
    CapClientSreamCapability source_capability;
    CapClientContext audio_context;
    CapClientAudioLocation sink_audio_location;
    CapClientAudioLocation source_audio_location;
    uint8 mic_count;

    if (group_instance->cap_group_handle)
    {
        /* Get sink audio context first. If it not available, try to get the source audio context */
        audio_context = CapProfileClient_GetAvailableSinkAudioContext(group_instance->cap_group_handle);

        if (audio_context == CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)
        {
            /* If sink audio context is unavailable, then use source audio context */
            audio_context = CapProfileClient_GetAvailableSourceAudioContext(group_instance->cap_group_handle);
        }

        /* If the available audio context indicates multiple contexts are available, then use the preferred audio
           context provided. Otherwise use the available context */
        if (CapProfileClient_IsGivenContextsHaveMultipleContexts(audio_context))
        {
            audio_context = preferred_audio_context;
        }

        sink_capability = capProfileClient_GetFirstStreamCapability(group_instance->cap_group_handle, CAP_CLIENT_ASE_SINK);
        source_capability = capProfileClient_GetFirstStreamCapability(group_instance->cap_group_handle, CAP_CLIENT_ASE_SOURCE);
        mic_count = capProfileClient_GetSourceAseCount(group_instance->cap_group_handle);
        sink_audio_location = capProfileClient_GetAudioLocationForAseType(group_instance->cap_group_handle, CAP_CLIENT_ASE_SINK);
        source_audio_location = capProfileClient_GetAudioLocationForAseType(group_instance->cap_group_handle, CAP_CLIENT_ASE_SOURCE);


        CapProfileClient_Configure(sink_capability,
                                   source_capability,
                                   latency,
                                   audio_context,
                                   sink_audio_location,
                                   source_audio_location,
                                   mic_count,
                                   cap_config_mode);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_StartPtsUnicastStreaming(uint16 preferred_audio_context, uint8 ccid_count)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance;
    CapClientContext audio_context = CAP_CLIENT_CONTEXT_TYPE_PROHIBITED;
    uint8 metadata_len = 0, *meta_data = NULL, ccid_value = 1, i;

    group_instance = &cap_client_taskdata.cap_group_instance[0];

    if (group_instance != NULL)
    {
        /* Get sink audio context first */
        audio_context = CapProfileClient_GetAvailableSinkAudioContext(group_instance->cap_group_handle);
        if (audio_context == CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)
        {
            /* If sink audio context is unavailable, then use source audio context */
            audio_context = CapProfileClient_GetAvailableSourceAudioContext(group_instance->cap_group_handle);
        }

        /* If the available audio context indicates multiple contexts are available, then use the preferred audio
           context provided. Otherwise use the available context */
        if (CapProfileClient_IsGivenContextsHaveMultipleContexts(audio_context))
        {
            audio_context = preferred_audio_context;
        }

        if (ccid_count != 0)
        {
            metadata_len = ccid_count * 3;
            meta_data = (uint8 *) CsrPmemAlloc(metadata_len * sizeof(uint8));

            for (i = 0; i < ccid_count; i++)
            {
                /* Fill the CCID value in LTV format */
                meta_data[i * 3]       = 0x02;
                meta_data[(i * 3) +1]  = 0x05;
                meta_data[(i * 3) +2]  = ccid_value++;
            }
        }

        CapClientUnicastStartStreamReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                 group_instance->cap_group_handle,
                                 audio_context,
                                 metadata_len,
                                 meta_data);
        status = TRUE;
        pfree(meta_data);
    }

    DEBUG_LOG("CapProfileClient_StartPtsUnicastStreaming: audio_context: 0x%x status: %d",
               audio_context, status);

    return status;
}

void CapProfileClient_UpdateAudioStreaming(uint16 use_case, uint8 ccid_count)
{
    uint8 metadata_len = 0;
    uint8 *meta_data = NULL, i = 0, ccid_value = 1;

    if (ccid_count != 0)
    {
        metadata_len = ccid_count * 3;
        meta_data = (uint8 *) CsrPmemAlloc(metadata_len);

        for (i = 0; i < ccid_count; i++)
        {
            /* Fill the CCID value in LTV format */
            meta_data[i * 3]       = 0x02;
            meta_data[(i * 3) +1]  = 0x05;
            meta_data[(i * 3) +2]  = ccid_value++;
        }
    }

    DEBUG_LOG("CapProfileClient_UpdateAudioStreaming: audio_context: 0x%x", use_case);

    CapClientUnicastUpdateAudioReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                   cap_client_taskdata.cap_group_instance[0].cap_group_handle,
                                   use_case,
                                   metadata_len,
                                   meta_data);

    pfree(meta_data);

}

bool CapProfileClient_StopPtsUnicastStreaming(bool release)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance;

    DEBUG_LOG("CapProfileClient_StopPtsUnicastStreaming: release %d", release);

    group_instance = &cap_client_taskdata.cap_group_instance[0];

    if (group_instance != NULL)
    {
        CapClientUnicastStopStreamReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                      group_instance->cap_group_handle,
                                      release);
        status = TRUE;
    }

    return status;
}

void CapProfileClient_ReadVolumeState(gatt_cid_t cid)
{
    DEBUG_LOG("CapProfileClient_ReadVolumeState cid:0x%x", cid);

    CapClientReadVolumeStateReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                cap_client_taskdata.cap_group_instance[0].cap_group_handle,
                                cid);
}

bool CapProfileClient_BroadcastAsstAddSource(uint32 cid, CapClientPaSyncState pa_sync_state, uint32 bis_index, bool is_collocated)
{
    CapClientSubgroupInfo subgroup_info[CAP_CLIENT_NUM_SUBGROUPS_SUPPORTED];
    typed_bdaddr bd_taddr;

    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("CapProfileClient_BroadcastAsstAddSource");

    if (is_collocated)
    {
        /* Use the dongle local address to add the source as source is collocated */
        LocalAddr_GetProgrammedBtAddress(&bd_taddr.addr);
        bd_taddr.type = TYPED_BDADDR_PUBLIC;
        BdaddrConvertTypedVmToBluestack(&broadcast_data.source_addr, &bd_taddr);
    }

    if (group_instance->cap_group_handle && cap_client_taskdata.cap_broadcast_data.source_found)
    {
        subgroup_info[0].bisIndex = bis_index;
        subgroup_info[0].metadataLen = 0;
        subgroup_info[0].metadataValue = NULL;

        CapClientBcastAsstAddSrcReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                    group_instance->cap_group_handle,
                                    cid,
                                    &broadcast_data.source_addr.addr,
                                    broadcast_data.source_addr.type,
                                    is_collocated,
                                    broadcast_data.sync_handle,
                                    broadcast_data.adv_sid,
                                    pa_sync_state,
                                    CAP_CLIENT__PA_INTERVAL_DEFAULT,
                                    broadcast_data.broadcast_id,
                                    CAP_CLIENT_NUM_SUBGROUPS_SUPPORTED,
                                    subgroup_info);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastAsstModifySource(uint32 cid, CapClientPaSyncState pa_sync_state, uint32 bis_index, bool is_collocated)
{
    CapClientSubgroupInfo subgroup_info[CAP_CLIENT_NUM_SUBGROUPS_SUPPORTED];
    CapClientDelegatorInfo sink_info;

    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;
    sink_info.sourceId = broadcast_data.source_id;
    sink_info.cid = cid;

    DEBUG_LOG("CapProfileClient_BroadcastAsstModifySource");

    if (group_instance->cap_group_handle)
    {
        subgroup_info[0].bisIndex = bis_index;
        subgroup_info[0].metadataLen = 0;
        subgroup_info[0].metadataValue = NULL;

        CapClientBcastAsstModifySrcReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                       group_instance->cap_group_handle,
                                       is_collocated,
                                       broadcast_data.sync_handle,
                                       broadcast_data.adv_sid,
                                       pa_sync_state,
                                       CAP_CLIENT__PA_INTERVAL_DEFAULT,
                                       CAP_CLIENT_NUMBER_OF_SINK_INFO_SUPPORTED,
                                       &sink_info,
                                       CAP_CLIENT_NUM_SUBGROUPS_SUPPORTED,
                                       subgroup_info);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastAsstRemoveSource(uint32 cid)
{
    CapClientDelegatorInfo brcast_sink_info[CAP_CLIENT_NUMBER_OF_SINK_INFO_SUPPORTED];

    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];

    DEBUG_LOG("CapProfileClient_BroadcastAsstRemoveSource");

    if (group_instance->cap_group_handle)
    {
        brcast_sink_info[0].cid = cid;
        brcast_sink_info[0].sourceId = cap_client_taskdata.cap_broadcast_data.source_id;

        CapClientBcastAsstRemoveSrcReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                       group_instance->cap_group_handle,
                                       CAP_CLIENT_NUMBER_OF_SINK_INFO_SUPPORTED,
                                       brcast_sink_info);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastAsstStartScan(uint32 cid, bool add_collocated)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];
    cap_client_taskdata.cap_broadcast_data.source_found = FALSE;

    DEBUG_LOG("CapProfileClient_BroadcastAsstStartScan");


    cap_client_taskdata.cap_broadcast_data.scan_for_colllocated = add_collocated;

    if (group_instance->cap_group_handle)
    {
        CapClientBcastAsstStartSrcScanReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                          group_instance->cap_group_handle,
                                          cid,
                                          add_collocated ? CAP_CLIENT_BCAST_SRC_COLLOCATED : CAP_CLIENT_BCAST_SRC_NON_COLLOCATED,
                                          HQ_PUBLIC_BROADCAST,
                                          CAP_CLIENT_CONTEXT_TYPE_MEDIA,
                                          CAP_CLIENT_SCAN_PARAM_DEFAULT,
                                          CAP_CLIENT_SCAN_PARAM_DEFAULT,
                                          CAP_CLIENT_SCAN_PARAM_DEFAULT);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastAsstStopScan(uint32 cid)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("CapProfileClient_BroadcastAsstStopScan");

    if (group_instance->cap_group_handle)
    {
        CapClientBcastAsstStopSrcScanReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                         group_instance->cap_group_handle,
                                         cid,
                                         broadcast_data.scan_handle);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastAsstRegisterForGattNotfn(uint32 cid)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("CapProfileClient_BroadcastAsstRegisterForGattNotfn");

    if (group_instance->cap_group_handle)
    {
        CapClientBcastAsstRegisterNotificationReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                                  group_instance->cap_group_handle,
                                                  cid,
                                                  broadcast_data.source_id,
                                                  TRUE,
                                                  TRUE);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastAsstReadBrs(uint32 cid)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];

    DEBUG_LOG("CapProfileClient_BroadcastAsstReadBrs");

    if (group_instance->cap_group_handle)
    {
        CapClientBcastAsstReadReceiveStateReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                              group_instance->cap_group_handle,
                                              cid);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastAsstSyncToSrc(uint32 cid)
{
    TYPED_BD_ADDR_T bd_addr;

    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("CapProfileClient_BroadcastAsstSyncToSrc");

    bd_addr.addr = broadcast_data.source_addr.addr;
    bd_addr.type = broadcast_data.source_addr.type;

    if (group_instance->cap_group_handle && broadcast_data.source_found)
    {
        CapClientBcastAsstSyncToSrcStartReq(TrapToOxygenTask((Task)&cap_client_taskdata.task_data),
                                            group_instance->cap_group_handle,
                                            &bd_addr,
                                            cid);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastAsstSetCode(gatt_cid_t cid, uint8 *broadcast_code)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];

    DEBUG_LOG("CapProfileClient_AssistantBcastSetCode");

    if (group_instance->cap_group_handle)
    {

        CapClientBcastAsstSetCodeRsp(group_instance->cap_group_handle,
                                     cid,
                                     cap_client_taskdata.cap_broadcast_data.source_id,
                                     broadcast_code);
        status = TRUE;
    }

    return status;
}
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Retrieve CAS handle data from NVM for the provided connection identifier */
void * CapProfileClient_RetrieveClientHandles(gatt_cid_t cid)
{
    UNUSED(cid);
    return NULL;
}

/*! \brief Store CAS handle data to NVM for the provided connection identifier */
bool CapProfileClient_StoreClientHandles(gatt_cid_t cid, void *config, uint8 size)
{
    UNUSED(cid);
    UNUSED(config);
    UNUSED(size);

    return FALSE;
}

/*! \brief Reset the provided CAP client instance */
void CapProfileClient_ResetCapClientInstance(cap_profile_client_device_info_t *device_instance)
{
    if (device_instance != NULL)
    {
        memset(device_instance, 0, sizeof(cap_profile_client_device_info_t));
        device_instance->cid =  INVALID_CONNID;
    }
}

bool CapProfileClient_IsAudioContextAvailable(ServiceHandle group_handle, uint16 audio_context)
{
    uint16 context_available;
    cap_profile_group_instance_t *group_instance = CapProfileClient_GetGroupInstance(group_handle);

    context_available = group_instance == NULL ? 0 : group_instance->available_audio_context;

    DEBUG_LOG("CapProfileClient_IsAudioContextAvailable: group_handle=0x%04X, audio_context requested: 0x%x, available: 0x%x",
               group_handle, audio_context, context_available);

    return (audio_context & context_available) != 0;
}

/*! \brief Destroy CAP profile if any established for this connection */
bool CapProfileClient_DestroyInstance(ServiceHandle group_handle, gatt_cid_t cid)
{
    bool status = FALSE;
    cap_profile_client_device_info_t *instance;
    cap_profile_group_instance_t *group_instance;

    DEBUG_LOG("CapProfileClient_DestroyInstance: cid=0x%04X", cid);
    group_instance = CapProfileClient_GetGroupInstance(group_handle);
    PanicNull(group_instance);
    instance = CapProfileClient_GetDeviceInstance(group_instance, cap_profile_client_compare_by_cid, (unsigned)cid);

    if (instance != NULL || cid == 0)
    {
        if (cid == 0)
        {
            /* cid is zero means group destroy operation. Setting the device count as 0 here to indicate this */
            group_instance->device_count = 0;
        }
        else
        {
            instance->state = cap_profile_client_state_disconnecting;
        }
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        CapClientRemoveDevReq(group_handle, cid);
        status = TRUE;
#endif
    }

    return status;
}

static bool capProfileClient_IsCapConnectedForGivenCid(gatt_cid_t cid)
{
    int i;
    cap_profile_client_device_info_t *instance;
    bool status = FALSE;

    /* Check if CAP is connected in atleast one group */
    for (i = 0; i < MAX_CAP_GROUP_SUPPORTED; i++)
    {
        instance = CapProfileClient_GetDeviceInstance(&cap_client_taskdata.cap_group_instance[i],
                                                      cap_profile_client_compare_by_valid_invalid_cid,
                                                      (unsigned)cid);
        if (instance != NULL)
        {
            status = TRUE;
            break;
        }
    }

    return status;
}
bool CapProfileClient_IsCapConnectedForCid(gatt_cid_t cid)
{
    return capProfileClient_IsCapConnectedForGivenCid(cid);
}

/*! \brief Check If CAP is connected or not */
bool CapProfileClient_IsCapConnected(void)
{
    return capProfileClient_IsCapConnectedForGivenCid(INVALID_CID);
}

CapClientAudioLocation CapProfileClient_GetSourceAudioLocation(ServiceHandle group_handle)
{
    return capProfileClient_GetSourceAudioLocation(group_handle);
}

bool CapProfileClient_IsStreamCapabilitySupported(ServiceHandle group_handle, uint32 stream_capability)
{
    int i;
    cap_profile_group_instance_t *group_instance = CapProfileClient_GetGroupInstance(group_handle);
    PanicNull(group_instance);

    DEBUG_LOG("CapProfileClient_IsStreamCapabilitySupported: stream_capability to check =0x%x", stream_capability);

    for (i=0; i < group_instance->cap_info.stream_cap_count; i++)
    {
        if ((group_instance->cap_info.capability[i].capability & stream_capability) == stream_capability)
        {
            DEBUG_LOG("CapProfileClient_IsStreamCapabilitySupported: FOUND =0x%04x", group_instance->cap_info.capability[i].capability);
            return TRUE;
        }
    }

    return FALSE;
}

bool CapProfileClient_IsAdvertFromSetMember(ServiceHandle group_handle, uint8 *adv_data, uint16 adv_data_len)
{
    bool status = FALSE;
    uint8 rsi_data[6];
    cap_profile_group_instance_t *group_instance = NULL;

    group_instance = CapProfileClient_GetGroupInstance(group_handle);

    if (group_instance != NULL)
    {
        if (CsipGetRsiFromAdvData(adv_data, adv_data_len, &rsi_data[0]))
        {
            if (CsipIsSetMember(&rsi_data[0], sizeof(rsi_data), &group_instance->sirk[0],
                                sizeof(group_instance->sirk)))
            {
                status = TRUE;
            }
        }
    }

    DEBUG_LOG("CapProfileClient_IsAdvertFromSetMember status %d", status);

    return status;
}

/*! \brief Get the audio context available for sink or source */
static CapClientContext capProfileClient_GetAvailableAudioContext(ServiceHandle group_handle, bool is_sink)
{
    cap_profile_group_instance_t *group_instance;
    uint32 audio_context = CAP_CLIENT_CONTEXT_TYPE_PROHIBITED;

    group_instance = CapProfileClient_GetGroupInstance(group_handle);
    PanicNull(group_instance);

    if (is_sink)
    {
        /* Get the LSB two bytes which have available sink audio context */
        audio_context = group_instance->available_audio_context & 0xffff;
    }
    else
    {
        /* Get the MSB two bytes which have available source audio context */
        audio_context = (group_instance->available_audio_context >> 16) & 0xffff;
    }

    return (uint16)audio_context;
}

CapClientContext CapProfileClient_GetAvailableSinkAudioContext(ServiceHandle group_handle)
{
    CapClientContext audio_context;

    audio_context = capProfileClient_GetAvailableAudioContext(group_handle, TRUE);

    DEBUG_LOG("capProfileClient_GetAvailableSinkAudioContext 0x%x", audio_context);

    return audio_context;
}

CapClientContext CapProfileClient_GetAvailableSourceAudioContext(ServiceHandle group_handle)
{
    CapClientContext audio_context;

    audio_context = capProfileClient_GetAvailableAudioContext(group_handle, FALSE);

    DEBUG_LOG("CapProfileClient_GetAvailableSourceAudioContext 0x%x", audio_context);

    return audio_context;
}

void CapProfileClient_BroadcastSrcInit(void)
{
    DEBUG_LOG("CapProfileClient_BroadcastSrcInit");

    CapClientBcastSrcInitReq(TrapToOxygenTask((Task) &cap_client_taskdata.task_data));
}

bool CapProfileClient_BroadcastSrcConfigure(const BroadcastType bcast_type)
{
    CapClientBcastInfo bcast_info;
    CapClientBigSubGroup sub_group_info;

    bool status = FALSE;
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("CapProfileClient_BroadcastSrcConfigure");

    if (broadcast_data.bcast_handle)
    {
        bcast_info.broadcast = bcast_type;
        bcast_info.flags = 0;
        bcast_info.appearanceValue = CSR_BT_APPEARANCE_GENERIC_PHONE;
        bcast_info.bigNameLen = 0x0E;
        bcast_info.bigName = (uint8*)"UD_Broadcaster";

        sub_group_info.config = CAP_CLIENT_STREAM_CAPABILITY_48_2;
        sub_group_info.targetLatency = CAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY;
        sub_group_info.lc3BlocksPerSdu = CAP_CLIENT_BROADCAST_LC3_BLOCKS_PER_SDU;
        sub_group_info.useCase = CAP_CLIENT_CONTEXT_TYPE_MEDIA;
        sub_group_info.metadataLen = 0;
        sub_group_info.metadata = NULL;
        sub_group_info.numBis = CAP_NUMBER_OF_BIS_SUPPORTED;
        sub_group_info.bisInfo[0].audioLocation = CAP_CLIENT_AUDIO_LOCATION_FL;
        sub_group_info.bisInfo[1].audioLocation = CAP_CLIENT_AUDIO_LOCATION_FR;
        sub_group_info.bisInfo[0].config = CAP_CLIENT_STREAM_CAPABILITY_48_2;
        sub_group_info.bisInfo[1].config = CAP_CLIENT_STREAM_CAPABILITY_48_2;
        sub_group_info.bisInfo[0].targetLatency = CAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY;
        sub_group_info.bisInfo[1].targetLatency = CAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY;
        sub_group_info.bisInfo[0].lc3BlocksPerSdu = CAP_CLIENT_BROADCAST_LC3_BLOCKS_PER_SDU;
        sub_group_info.bisInfo[1].lc3BlocksPerSdu = CAP_CLIENT_BROADCAST_LC3_BLOCKS_PER_SDU;

        CapClientBcastSrcConfigReq(broadcast_data.bcast_handle,
                                   CSR_BT_ADDR_PUBLIC,
                                   CAP_CLIENT_PRESENTATION_DELAY,
                                   CAP_CLIENT_NUM_SUBGROUPS_SUPPORTED,
                                   &sub_group_info,
                                   &bcast_info,
                                   CAP_CLIENT_BIG_CONFIG_MODE_DEFAULT,
                                   NULL);

        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastSrcStartStreaming(bool encryption, uint8* broadcast_code)
{
    bool status = FALSE;
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("CapProfileClient_BroadcastSrcStartStreaming");

    if (broadcast_data.bcast_handle)
    {
        CapClientBcastSrcStartStreamReq(cap_client_taskdata.cap_broadcast_data.bcast_handle,
                                        encryption,
                                        broadcast_code);

        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastSrcStopStreaming(void)
{
    bool status = FALSE;
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("CapProfileClient_BroadcastSrcStopStreaming");

    if (broadcast_data.bcast_handle)
    {
        CapClientBcastSrcStopStreamReq(cap_client_taskdata.cap_broadcast_data.bcast_handle);

        status = TRUE;
    }

    return status;
}


bool CapProfileClient_BroadcastSrcUpdateStream(CapClientContext useCase)
{
    bool status = FALSE;
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;
    uint8 metadata_len = 0;
    const uint8* metadata = NULL;

    DEBUG_LOG("CapProfileClient_BroadcastSrcUpdateStream");

    if (broadcast_data.bcast_handle)
    {
        CapClientBcastSrcUpdateStreamReq(cap_client_taskdata.cap_broadcast_data.bcast_handle,
                                         useCase,
                                         CAP_CLIENT_NUM_SUBGROUPS_SUPPORTED,
                                         metadata_len,
                                         metadata);
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastSrcRemoveStream(void)
{
    bool status = FALSE;
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("CapProfileClient_BroadcastSrcRemoveStream");

    if (broadcast_data.bcast_handle)
    {
        CapClientBcastSrcRemoveStreamReq(cap_client_taskdata.cap_broadcast_data.bcast_handle);

        status = TRUE;
    }

    return status;
}

bool CapProfileClient_BroadcastSrcDeinit(void)
{
    bool status = FALSE;
    cap_profile_client_broadcast_data_t broadcast_data = cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("CapProfileClient_BroadcastSrcDeinit");

    if (broadcast_data.bcast_handle)
    {
        CapClientBcastSrcDeinitReq(cap_client_taskdata.cap_broadcast_data.bcast_handle);

        status = TRUE;
    }

    return status;
}

bool CapProfileClient_SetDeviceBdAddressToAdd(bdaddr *bd_addr)
{
    bool status = FALSE;
    cap_profile_group_instance_t *group_instance = &cap_client_taskdata.cap_group_instance[0];
    cap_profile_client_broadcast_data_t *broadcast_data = &cap_client_taskdata.cap_broadcast_data;

    DEBUG_LOG("CapProfileClient_SetDeviceBdAddressToAdd");

    if (group_instance->cap_group_handle)
    {
        broadcast_data->source_addr.addr.nap = bd_addr->nap;
        broadcast_data->source_addr.addr.uap = bd_addr->uap;
        broadcast_data->source_addr.addr.lap = bd_addr->lap;
        broadcast_data->source_addr.type = 0x0;
        status = TRUE;
    }

    return status;
}

bool CapProfileClient_PtsGetSpeakerPathConfig(cap_profile_client_media_config_t *media_config)
{
    bool status = FALSE;
    cap_profile_client_broadcast_data_t *bcast_data = &cap_client_taskdata.cap_broadcast_data;

    if (bcast_data != NULL && (bcast_data->bis_handles[0] || bcast_data->bis_handles[1]))
    {
        media_config->source_iso_handle = bcast_data->bis_handles[0];
        media_config->source_iso_handle_right = bcast_data->bis_handles[1];
        media_config->codec_frame_blocks_per_sdu = CAP_CLIENT_BROADCAST_LC3_BLOCKS_PER_SDU;
        media_config->codec_type = CAP_CLIENT_CODEC_PARAM_DEFAULT;
        media_config->codec_version = CAP_CLIENT_CODEC_PARAM_DEFAULT;
        media_config->stream_type = 0x04 ; /* KYMERA_LE_STREAM_DUAL_MONO */
        media_config->frame_duration = CAP_CLIENT_FRAME_DURATION_DEFAULT;
        media_config->frame_length = CAP_CLIENT_FRAME_LEN_DEFAULT;
        media_config->presentation_delay = CAP_CLIENT_PRESENTATION_DELAY;
        media_config->sample_rate = CAP_CLIENT_SAMPLE_RATE_DEFAULT;
        media_config->gaming_mode = FALSE;
        media_config->use_cvc = FALSE;
        media_config->start_muted = FALSE;
        status = TRUE;
    }

    return status;
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

