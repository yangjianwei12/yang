/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    sink_service
    \brief      Sink service pairing state implementation
*/

#include "sink_service_pairing.h"
#include "sink_service_private.h"
#include "sink_service_sm.h"
#include "sink_service_le_sm.h"
#include "sink_service_util.h"
#include "sink_service_logging.h"
#include "sink_service_config.h"
#include "device_properties.h"

#ifdef ENABLE_LE_SINK_SERVICE
#include "le_audio_client.h"
#include "gatt_service_discovery.h"
#endif

#include "bt_device_class.h"
#include "unexpected_message.h"
#include "device_db_serialiser.h"

/*! \brief COD filter to identify from inquiry results that the device is LEA supported or not */
#define SINK_SERVICE_DUAL_SM_LEA_COD_FILTER         LE_AUDIO_MAJOR_SERV_CLASS

#ifdef ENABLE_LE_SINK_SERVICE

/*! \brief Do a custom validation of LE Extended advertising report */
static bool sinkServicePairing_AdvertHookCallback(const uint8 *adv_data, uint16 adv_data_len);

/*! \brief Handle the pairing confirmation for LE device */
static void sinkServicePairing_HandlePairingCfmForLe(sink_service_state_machine_t * sm, RSSI_PAIRING_PAIR_CFM_T *pair_cfm);

/*! \brief Get the RSSI pairing mode to use based on sink service mode */
static rssi_pairing_mode_t sinkServicePairing_GetPairingMode(void);
#else

#define sinkServicePairing_AdvertHookCallback(adv_data, adv_data_len)   (FALSE)
#define sinkServicePairing_HandlePairingCfmForLe(sm, pair_cfm)
#define sinkServicePairing_GetPairingMode()                             (RSSI_PAIRING_MODE_BREDR)

#endif /* ENABLE_LE_SINK_SERVICE */

static bool sinkServicePairing_Start(sink_service_state_machine_t *sm,
                                     rssi_pairing_mode_t rssi_pairing_mode,
                                     bool register_hook_cb)
{
    rssi_pairing_le_scan_parameters_t le_scan_params = { 0 };
#ifdef ENABLE_LE_SINK_SERVICE
    le_extended_advertising_filter_t extended_advertising_filter;
    le_scan_manager_uuid_t scan_filter[SINK_SERVICE_MAX_LEA_UUID_FILTER];
    uint8 size_ad_types = 1;
    uint8 ad_types[] = {ble_ad_type_service_16bit_uuid}, filter_index;

    /* Set adv filter. If any of these UUID present, advert will be considered for pairing  */
    for (filter_index = 0; filter_index < SinkService_GetTaskData()->filter_data.num_of_uuids; filter_index++)
    {
        scan_filter[filter_index].type = UUID_TYPE_16;
        scan_filter[filter_index].uuid[0] = SinkService_GetTaskData()->filter_data.uuid_list[filter_index];
    }

    extended_advertising_filter.size_ad_types = size_ad_types;
    extended_advertising_filter.ad_types = ad_types;
    extended_advertising_filter.uuid_list_size = SinkService_GetTaskData()->filter_data.num_of_uuids;
    extended_advertising_filter.uuid_list = SinkService_GetTaskData()->filter_data.num_of_uuids ? &scan_filter[0] : NULL;

    /* Fill LE scan parameters */
    le_scan_params.filter_param = &extended_advertising_filter;
    le_scan_params.cb = register_hook_cb ? sinkServicePairing_AdvertHookCallback : NULL;
    if (rssi_pairing_mode == RSSI_PAIRING_MODE_DUAL_PREF_LE || rssi_pairing_mode == RSSI_PAIRING_MODE_DUAL_PREF_BREDR)
    {
        /* Enable COD filter in dual mode to distuinguish dual mode devices */
        le_scan_params.cod_filter = SINK_SERVICE_DUAL_SM_LEA_COD_FILTER;
    }
#else
    UNUSED(register_hook_cb);
#endif

    /* Request low latency during initial connection setup.*/
    ConManagerRequestDefaultQos(cm_transport_ble, cm_qos_low_latency);

    if (!RssiPairing_Start(sinkService_GetTaskForSm(sm),
                           rssi_pairing_mode,
                           SinkServiceConfig_GetConfig()->rssi_pairing_params,
                           rssi_pairing_mode != RSSI_PAIRING_MODE_BREDR ? &le_scan_params : NULL))
    {
        /* Don't panic if BREDR pairing fails */
        if (rssi_pairing_mode == RSSI_PAIRING_MODE_BREDR)
        {
            DEBUG_LOG_ERROR("SinkService: Failed to start RSSI Pairing");
            SinkService_SendConnectedCfm(sm->sink_device, SINK_SERVICE_TRANSPORT_UNKNOWN, sink_service_status_failed);
            sinkServiceSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
            return FALSE;
        }

        /* Unable to start the scan when requested is a problem. Panic when this condition is hit */
        Panic();
    }

    DEBUG_LOG_INFO("sinkServicePairing_Start success");

    return TRUE;
}

/*! \brief Handle the pairing confirmation for BREDR device */
static void sinkServicePairing_HandlePairingCfmForBredr(sink_service_state_machine_t * sm, RSSI_PAIRING_PAIR_CFM_T *pair_cfm)
{
    const bdaddr * bd_addr = &pair_cfm->bd_addr;

    DEBUG_LOG_DEBUG("sinkServicePairing_HandlePairingCfmForBredr RSSI_PAIRING_PAIR_CFM status %d dev_type %d",
                     pair_cfm->status, pair_cfm->device_type);

    if (pair_cfm->status)
    {
        DEBUG_LOG_INFO("SinkService: Sink paired successfully: 0x%04x,%02x,%06lx",
                       bd_addr->nap, bd_addr->uap, bd_addr->lap);

        /* Pairing request is locally initiated, so the device is a sink. */
        device_t sink = BtDevice_GetDeviceCreateIfNew(bd_addr, DEVICE_TYPE_SINK);
        BtDevice_SetDefaultProperties(sink);

        /* The device has paired. Clear the NOT_PAIRED Flag */
        PanicFalse(BtDevice_SetFlags(sink, DEVICE_FLAGS_NOT_PAIRED, 0));

        /* Now that we have successfully paired,
         * we can set the link behavior within bluestack to disable connection retries */
        BtDevice_SetLinkBehavior(bd_addr);

#ifdef ENABLE_LE_SINK_SERVICE
        if (pair_cfm->device_type == RSSI_PAIRING_DEVICE_TYPE_DUAL)
        {
            /* For dual mode devices, set the LE profiles to supported profiles list */
            BtDevice_AddSupportedProfilesToDevice(sink, DEVICE_PROFILE_LE_AUDIO);
        }
#endif /* ENABLE_LE_SINK_SERVICE */

        /* Add the supported profiles to the device. The device will be auto serialized immediately */
        BtDevice_AddSupportedProfilesToDevice(sink, SinkServiceConfig_GetConfig()->supported_profile_mask);

        if (ConManagerIsConnected(bd_addr))
        {
            DEBUG_LOG_INFO("SinkService: Connecting profiles...");

            /* Set the newly created device as the current connected_sink. */
            sm->sink_device = sink;

            /* Pairing is finished, ACL up, so move to the connecting profiles state.*/
            sinkServiceSm_SetState(sm, SINK_SERVICE_STATE_CONNECTING_PROFILES);
        }
        else
        {
            /* If there is a link loss or remote disconnection while the RSSI Pairing
             * is sending its confirmation the SM needs to move to disconnected */
            DEBUG_LOG_WARN("SinkService: ACL unexpectedly dropped after pairing");
            SinkService_SendConnectedCfm(sm->sink_device, SINK_SERVICE_TRANSPORT_UNKNOWN, sink_service_status_failed);
            sinkServiceSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
        }
    }
    else
    {
        DEBUG_LOG_ERROR("SinkService: Pairing failed");
        SinkService_SendConnectedCfm(sm->sink_device, SINK_SERVICE_TRANSPORT_UNKNOWN, sink_service_status_failed);
        sinkServiceSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
    }
}

#ifdef ENABLE_LE_SINK_SERVICE

/*! \brief Get the RSSI pairing mode to use based on sink service mode */
static rssi_pairing_mode_t sinkServicePairing_GetPairingMode(void)
{
    rssi_pairing_mode_t mode = RSSI_PAIRING_MODE_BREDR;

    if (sinkService_IsInLeMode())
    {
        mode = RSSI_PAIRING_MODE_LE;
    }

    if (sinkService_IsInDualModePreferredLe())
    {
        mode = RSSI_PAIRING_MODE_DUAL_PREF_LE;
    }

    if (sinkService_IsInDualModePreferredBredr())
    {
        mode = RSSI_PAIRING_MODE_DUAL_PREF_BREDR;
    }

    return mode;
}

/*! \brief Do a custom validation of LE Extended advertising report */
static bool sinkServicePairing_AdvertHookCallback(const uint8 *adv_data, uint16 adv_data_len)
{
    DEBUG_LOG_INFO("sinkServiceLeSm_AdvertHookCallback");

    return LeAudioClient_IsAdvertFromSetMember((uint8*)adv_data, adv_data_len);
}

/*! \brief Handle the RSSI pair confirmation in dual mode */
static void sinkServicePairing_HandlePairingCfmForDualMode(sink_service_state_machine_t *sm, RSSI_PAIRING_PAIR_CFM_T *pair_cfm)
{
    lea_device_info_t *sink_device_info;

    if (pair_cfm->status)
    {
        if (pair_cfm->device_type == RSSI_PAIRING_DEVICE_TYPE_BREDR ||
           (pair_cfm->device_type == RSSI_PAIRING_DEVICE_TYPE_DUAL && sinkService_IsInDualModePreferredBredr()))
        {
            sinkServicePairing_HandlePairingCfmForBredr(sm, pair_cfm);
        }
        else if (pair_cfm->device_type == RSSI_PAIRING_DEVICE_TYPE_LE ||
                (pair_cfm->device_type == RSSI_PAIRING_DEVICE_TYPE_DUAL && sinkService_IsInDualModePreferredLe()))
        {
            sinkServicePairing_HandlePairingCfmForLe(sm, pair_cfm);
        }
        else
        {
            /* Not expected to reach here */
            Panic();
        }
    }
    else
    {
        DEBUG_LOG_ERROR("SinkService: Pairing failed");

        SinkService_SendConnectedCfm(sm->sink_device, SINK_SERVICE_TRANSPORT_UNKNOWN, sink_service_status_failed);

        sink_device_info = sinkServiceLeSm_GetLeDeviceInfoByAddr(sm, (bdaddr*)&pair_cfm->bd_addr);

        if (!BdaddrIsZero(&pair_cfm->bd_addr) && sink_device_info != NULL)
        {
            /* Release the LE ACL(Decrement the user count since we incremented it when ACL was connected) */
            ConManagerReleaseTpAcl(&sink_device_info->tp_acl_hold_addr);
        }

        /* Release the ACL if there is BREDR ACL */
        if (!BdaddrIsZero(&sm->acl_hold_addr))
        {
            ConManagerReleaseAcl(&sm->acl_hold_addr);
        }

        SinkService_ConnectableEnableBredr(TRUE);

        ProfileManager_ClientUnregister(sinkService_GetTaskForSm(sm));
        ConManagerUnregisterConnectionsClient(sinkService_GetTaskForSm(sm));

        /* The device which we tried to pair failed/no devices found. Move the sink service to disconnected state.
         * When entering disconnected state, sink service will delete the non-paired device(if any),
         * and terminates the LE-ACL link.
         */
        if (sm->lea_device[0].sink_device != NULL)
        {
            if (Device_IsPropertySet(sm->lea_device[0].sink_device, device_property_audio_source))
            {
                /* Remove the audio and voice sources */
                DeviceProperties_RemoveAudioSource(sm->lea_device[0].sink_device);
                DeviceProperties_RemoveVoiceSource(sm->lea_device[0].sink_device);
            }

            sinkServiceLeSm_DeleteDeviceIfNotPaired(sm);
        }

        /* Clear the entire state machine */
        SinkServiceLeSm_ClearInstance(sm);
    }
}

/*! \brief Handle the pairing confirmation for LE device */
static void sinkServicePairing_HandlePairingCfmForLe(sink_service_state_machine_t * sm, RSSI_PAIRING_PAIR_CFM_T *pair_cfm)
{
    lea_device_info_t *sink_device_info;

    DEBUG_LOG_DEBUG("sinkServiceLeSm_StatePairingHandler: RSSI_PAIRING_PAIR_CFM %d device_type %d", pair_cfm->status, pair_cfm->device_type);

    MessageCancelFirst(sinkService_GetTaskForSm(sm), SINK_SERVICE_INTERNAL_LEA_DISCOVERY_TIMEOUT);
    sink_device_info = sinkServiceLeSm_GetLeDeviceInfoByAddr(sm, (bdaddr*)&pair_cfm->bd_addr);
    PanicNull(sink_device_info);

    if (pair_cfm->status)
    {
        /* The device has paired. Clear the NOT_PAIRED Flag */
        PanicNull(sink_device_info->sink_device);
        PanicFalse(BtDevice_SetFlags(sink_device_info->sink_device, DEVICE_FLAGS_NOT_PAIRED, 0));

        if (pair_cfm->device_type == RSSI_PAIRING_DEVICE_TYPE_DUAL)
        {
            /* For dual mode devices, set the BREDR profiles to supported profiles list */
            BtDevice_AddSupportedProfilesToDevice(sink_device_info->sink_device, SinkServiceConfig_GetConfig()->supported_profile_mask);
        }

        /* Set LE audio profiles also as supported profile */
        BtDevice_AddSupportedProfilesToDevice(sink_device_info->sink_device, DEVICE_PROFILE_LE_AUDIO);
        DeviceDbSerialiser_SerialiseDevice(sink_device_info->sink_device);
    }
    else
    {
        if (!BdaddrIsZero(&pair_cfm->bd_addr))
        {
            /* Release the ACL(Decrement the user count since we incremented it when ACL was connected) */
            ConManagerReleaseTpAcl(&sink_device_info->tp_acl_hold_addr);
        }

        if (sinkServiceLeSm_IsAnyLeDeviceConnected(sm))
        {
            /* There are other devices connected out of the coordinated set. So delete the device which
             *  we created for the current connected sink which failed to pair
             */
            if (!sinkServiceLeSm_DeviceIsPaired(sink_device_info->sink_device))
            {
                appDeviceDelete(&pair_cfm->bd_addr);
            }

            /* Clear the sink device info */
            sinkServiceLeSm_ClearLeDeviceInfoByAddr(sm, &sink_device_info->tp_acl_hold_addr.taddr.addr);

            /* Go ahead and configure the existing connected devices for the coordinated set.*/
            (void) LeAudioClient_DeviceDiscoveryFailed();
        }
        else
        {
            /* The device which we tried to pair failed/no devices found. Move the sink service to disconnected state.
             * When entering disconnected state, sink service will delete the non-paired device(if any),
             * and terminates the LE-ACL link.
             */
            sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
        }
    }
}

bool SinkService_SetUuidAdvFilter(uint8 num_of_uuids, uint16 uuid_list[])
{
    bool status = FALSE;
    uint8 filter_index;

    if (num_of_uuids <= SINK_SERVICE_MAX_LEA_UUID_FILTER)
    {
        SinkService_GetTaskData()->filter_data.num_of_uuids = num_of_uuids;

        for (filter_index = 0; filter_index < num_of_uuids; filter_index++)
        {
            SinkService_GetTaskData()->filter_data.uuid_list[filter_index] = uuid_list[filter_index];
        }

        status = TRUE;
    }

    return status;
}

#endif /* ENABLE_LE_SINK_SERVICE */

/*! \brief Handle messages in pairing state */
void SinkServicePairing_HandleMessage(sink_service_state_machine_t *sm, MessageId id, Message message)
{
    switch (id)
    {
        case RSSI_PAIRING_PAIR_CFM:
        {
            if (sinkService_IsInBredrMode())
            {
                sinkServicePairing_HandlePairingCfmForBredr(sm, (RSSI_PAIRING_PAIR_CFM_T*)message);
            }
#ifdef ENABLE_LE_SINK_SERVICE
            else if (sinkService_IsInLeMode())
            {
                sinkServicePairing_HandlePairingCfmForLe(sm, (RSSI_PAIRING_PAIR_CFM_T*)message);
            }
            else if (sinkService_IsInDualMode())
            {
                sinkServicePairing_HandlePairingCfmForDualMode(sm, (RSSI_PAIRING_PAIR_CFM_T*)message);
            }
#endif /* ENABLE_LE_SINK_SERVICE */

        }
        break;

        case CON_MANAGER_CONNECTION_IND:
        {
            const CON_MANAGER_CONNECTION_IND_T *msg = message;

            DEBUG_LOG_DEBUG("sinkServicePairing_HandleBredrMessages: CON_MANAGER_CONNECTION_IND");

            if (msg->connected && ConManagerIsAclLocal(&msg->bd_addr))
            {
                /* DeviceType isn't known during pairing (device entry not created yet),
                   but if we initiated the connection, then it must be a sink device we
                   are about to pair with. Hold the ACL open to allow time for profiles
                   to connect after pairing completes. */
                DEBUG_LOG_INFO("SinkService: Connected, hold pairing ACL: %04x,%02x,%06lx",
                                msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap);

                if (BdaddrIsZero(&sm->acl_hold_addr))
                {
                    ConManagerCreateAcl(&msg->bd_addr);
                    sm->acl_hold_addr = msg->bd_addr;
                }
            }
        }
        break;

#ifdef ENABLE_LE_SINK_SERVICE
        case SINK_SERVICE_INTERNAL_LEA_DISCOVERY_TIMEOUT:
        {
            /* We have received internal discovery timeout error. We could not find a coordinated set device
             * within the specified T-CSIP time. Stop the LE RSSI Pairing immediately.
             */
            RssiPairing_Stop();

            if (sinkServiceLeSm_IsAnyLeDeviceConnected(sm))
            {
                /* If there are any LE Devices connected (if we are partially connected to any coordinated set members)
                 * goahead and configure them.
                 */
                (void) LeAudioClient_DeviceDiscoveryFailed();
            }
            else
            {
                /* There are no coordinated devices found Or the existing members are lost. Move to disconnected state */
                sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
            }
        }
        break;

        case LE_AUDIO_CLIENT_DISCONNECT_IND:
        {
            const LE_AUDIO_CLIENT_DISCONNECT_IND_T *msg = message;

            if (msg->status == LE_AUDIO_CLIENT_STATUS_SUCCESS)
            {
                /* All device(s) are removed. Moved to disconnected state */
                sinkServiceLeSm_SetState(sm, SINK_SERVICE_STATE_DISCONNECTED);
            }
        }
        break;

        case LE_AUDIO_CLIENT_CONNECT_IND:
        {
            const LE_AUDIO_CLIENT_CONNECT_IND_T *msg = message;

            DEBUG_LOG_DEBUG("sinkServiceLeSm_StatePairingHandler: LE_AUDIO_CLIENT_CONNECT Status %d total_devices:%d connected_devices:%d",
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

            DEBUG_LOG_DEBUG("sinkServiceLeSm_StatePairingHandler: LE_AUDIO_CLIENT_DEVICE_ADDED_IND_T Status %d cid:0x%x",
                             msg->status, msg->cid);

            if (msg->status == LE_AUDIO_CLIENT_STATUS_SUCCESS && msg->more_devices_needed)
            {
                /* Request low latency during initial connection setup.*/
                ConManagerRequestDefaultQos(cm_transport_ble, cm_qos_low_latency);

                /* Start LE RSSI pairing with registering advert hook callback */
                sinkServicePairing_Start(sm, sinkServicePairing_GetPairingMode(), TRUE);

                /* Self queue a internal discovery timeout message */
                MessageSendLater(sinkService_GetTaskForSm(sm),
                                 SINK_SERVICE_INTERNAL_LEA_DISCOVERY_TIMEOUT, NULL,
                                 sinkServiceLe_LeaDiscoveryTimeout());
            }
        }
        break;

        case CON_MANAGER_TP_CONNECT_IND:
        {
            const CON_MANAGER_TP_CONNECT_IND_T *msg = (CON_MANAGER_TP_CONNECT_IND_T *) message;

            if (msg->tpaddr.transport == TRANSPORT_BLE_ACL)
            {
                if (ConManagerIsTpAclLocal(&msg->tpaddr))
                {
                    DEBUG_LOG_INFO("SinkService: Connected, hold LE ACL: %06lx", msg->tpaddr.taddr.addr.lap);

                    MessageCancelFirst(sinkService_GetTaskForSm(sm), SINK_SERVICE_INTERNAL_LEA_DISCOVERY_TIMEOUT);

                    /* Panic if there is no slot available to store this device */
                    PanicFalse(sinkServiceLeSm_AddLeDeviceInfo(sm, msg->tpaddr));
                    ConManagerCreateTpAcl(&msg->tpaddr);
                }
                else
                {
                    DEBUG_LOG_INFO("sinkServiceLeSm_StatePairingHandler: Not a locally initiated connection");
                    Panic();
                }
            }
        }
        break;

        case CON_MANAGER_TP_DISCONNECT_IND:
        {
            lea_device_info_t *sink_device_info;
            const CON_MANAGER_TP_DISCONNECT_IND_T *msg = message;

            if (msg->tpaddr.transport == TRANSPORT_BLE_ACL)
            {
                DEBUG_LOG_DEBUG("sinkServiceLeSm_StatePairingHandler: CON_MANAGER_TP_DISCONNECT_IND for LAP : 0x%06lx",
                                msg->tpaddr.taddr.addr.lap);

                sink_device_info = sinkServiceLeSm_GetLeDeviceInfoByAddr(sm, (bdaddr*)&msg->tpaddr.taddr.addr);

                /* Ensure sink device is valid. If not, this disconnect indication could be for a LE-ACL close
                   request that the sink service had placed earlier. In such scenario, just skip this.  */
                if (sink_device_info != NULL)
                {
                    (void) LeAudioClient_Disconnect(sink_device_info->gatt_cid, ConManagerIsTpConnected(&msg->tpaddr));
                }
            }
        }
        break;

        case GATT_SERVICE_DISCOVERY_COMPLETE:
        {
            const GATT_SERVICE_DISCOVERY_COMPLETE_T *msg = (GATT_SERVICE_DISCOVERY_COMPLETE_T *) message;

            sinkServiceLeSm_HandleServiceDiscoveryComplete(msg->cid, sm);
        }
        break;
#endif /* ENABLE_LE_SINK_SERVICE */

        default:
        break;
    }
}

bool SinkServicePairing_PairingRequest(sink_service_state_machine_t *sm)
{
    if (SinkService_IsPairingEnabled())
    {
        DEBUG_LOG_INFO("SinkServicePairing_PairingRequest");

        if (sinkService_IsInBredrMode() || sinkService_IsInDualMode())
        {
            ConManagerRegisterConnectionsClient(sinkService_GetTaskForSm(sm));
            SinkService_ConnectableEnableBredr(FALSE);
        }

#ifdef ENABLE_LE_SINK_SERVICE
        if (sinkService_IsInLeMode() || sinkService_IsInDualMode())
        {
            ConManagerRegisterTpConnectionsObserver(cm_transport_ble, sinkService_GetTaskForSm(sm));
            GattServiceDiscovery_ClientRegister(sinkService_GetTaskForSm(sm));
        }
#endif /* ENABLE_LE_SINK_SERVICE */

        sm->state = SINK_SERVICE_STATE_PAIRING;

        /* Start LE RSSI pairing without registering advert hook callback */
        sinkServicePairing_Start(sm, sinkServicePairing_GetPairingMode(), FALSE);

        SinkService_UpdateUi();
    }
    else
    {
        DEBUG_LOG_WARN("SinkService: Pairing disabled, ignoring request");

        return FALSE;
    }

    return TRUE;
}
