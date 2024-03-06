/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    sink_service
    \brief      Utility functions for sink service
*/

#include "sink_service_util.h"
#include "sink_service_logging.h"
#include "sink_service_protected.h"
#include "sink_service_config.h"
#include "device_list.h"
#include "device_properties.h"
#include "gatt.h"
#include <connection_manager.h>

#ifdef ENABLE_LE_SINK_SERVICE
#include "gatt_service_discovery.h"
#endif

#ifdef ENABLE_LE_SINK_SERVICE

/*! \brief Get the LE device with which sink service is connected */
static device_t sinkServiceUtil_GetConnectedLeDevice(sink_service_state_machine_t *sm)
{
    device_t sink_device = NULL;
    uint8 i;

    for (i = 0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
    {
        if (sm->lea_device[i].gatt_cid != INVALID_CID)
        {
            /* A valid LE device is connected */
            sink_device = sm->lea_device[i].sink_device;
            break;
        }
    }

    return sink_device;
}

/*! \brief Check if sink service is connected over LE transport or not */
static bool sinkServiceUtil_IsLeTransportConnected(sink_service_state_machine_t *sm)
{
    return sinkServiceUtil_GetConnectedLeDevice(sm) != NULL;
}

/*! \brief Get the sink device type based on the profiles supported by the given device */
static sink_service_device_type_t sinkServiceUtil_GetSinkDeviceType(device_t device)
{
    sink_service_device_type_t sink_type = SINK_SERVICE_DEVICE_UNKNOWN;
    bool is_bredr_supported, is_lea_supported;

    /* Check if BREDR/LE profiles are supported for the given device */
    is_bredr_supported = BtDevice_IsProfileSupportedForDevice(device, SinkServiceConfig_GetConfig()->supported_profile_mask);
    is_lea_supported = BtDevice_IsProfileSupportedForDevice(device, DEVICE_PROFILE_LE_AUDIO);

    if (is_bredr_supported && is_lea_supported)
    {
        sink_type = SINK_SERVICE_DEVICE_DUAL;
    }
    else if (is_bredr_supported)
    {
        sink_type = SINK_SERVICE_DEVICE_BREDR;
    }
    else if (is_lea_supported)
    {
        sink_type = SINK_SERVICE_DEVICE_LE;
    }

    DEBUG_LOG_INFO("sinkServiceUtil_GetSinkDeviceType enum:sink_service_device_type_t:%d", sink_type);

    return sink_type;
}

/*! \brief Find the transport to connect for the given device by considering current sink service
           mode and the transports supported by the device */
sink_service_transport_t sinkServiceUtil_GetTargetTransportBasedOnModeForDevice(device_t device)
{
    sink_service_transport_t target_transport = SINK_SERVICE_TRANSPORT_UNKNOWN;
    sink_service_device_type_t device_type = sinkServiceUtil_GetSinkDeviceType(device);

    switch (sinkService_GetMode())
    {
        case SINK_SERVICE_MODE_BREDR:
        {
            /* Current sink service mode is BREDR and device also supports BREDR */
            if (sinkServiceUtil_IsDeviceBredrOnly(device_type) || sinkServiceUtil_IsDeviceDual(device_type))
            {
                target_transport = SINK_SERVICE_TRANSPORT_BREDR;
            }
        }
        break;

        case SINK_SERVICE_MODE_LE:
        {
            /* Current sink service mode is LE and device also supports LE */
            if (sinkServiceUtil_IsDeviceLeOnly(device_type) || sinkServiceUtil_IsDeviceDual(device_type))
            {
                target_transport = SINK_SERVICE_TRANSPORT_LE;
            }
        }
        break;

        case SINK_SERVICE_MODE_DUAL_PREF_LE:
        case SINK_SERVICE_MODE_DUAL_PREF_BREDR:
        {
            /* Current sink service mode is dual */
            if (sinkServiceUtil_IsDeviceDual(device_type))
            {
                /* Sink device supports both the transport. Choose transport based on dual mode preference */
                target_transport = sinkService_GetMode() == SINK_SERVICE_MODE_DUAL_PREF_LE ? SINK_SERVICE_TRANSPORT_LE :
                                                                                             SINK_SERVICE_TRANSPORT_BREDR;
            }
            else
            {
                /* Sink device only supports one transport */
                target_transport = sinkServiceUtil_IsDeviceLeOnly(device_type) ? SINK_SERVICE_TRANSPORT_LE :
                                                                                     SINK_SERVICE_TRANSPORT_BREDR;
            }
        }
        break;

        default:
        break;
    }

    DEBUG_LOG_INFO("sinkServiceUtil_GetTargetTransportBasedOnModeForDevice %d", target_transport);

    return target_transport;
}

bool sinkServiceUtil_IsTransportDisconnectNeeded(void)
{
    bool transport_disconnect = FALSE;
    device_t connected_sink_device;

    FOR_EACH_SINK_SM(sm)
    {
        /* Check if there is any SM in connected state */
        if (sm->state == SINK_SERVICE_STATE_CONNECTED)
        {
            /* Get the connected sink device */
            connected_sink_device = sm->sink_device != NULL ? sm->sink_device :
                                                              sinkServiceUtil_GetConnectedLeDevice(sm);

            /* Current transport needs to be disconnected if it no longer matches with expected transport as
               per current sink service mode and connected sink device capabilities */
            if (sinkServiceUtil_GetConnectedTransport(sm) !=
                    sinkServiceUtil_GetTargetTransportBasedOnModeForDevice(connected_sink_device))
            {
                transport_disconnect = TRUE;
            }

            break;
        }
    }

    DEBUG_LOG_INFO("sinkServiceUtil_IsTransportDisconnectNeeded %d", transport_disconnect);

    return transport_disconnect;
}

void sinkService_EnableAllSmForDualMode(void)
{
    uint8 i;

    FOR_EACH_SINK_SM(sm)
    {
        if (sm->state == SINK_SERVICE_STATE_DISABLED)
        {
            /* Move the state to disconnected */
            memset(sm, 0, sizeof(*sm));
            sm->state = SINK_SERVICE_STATE_DISCONNECTED;
            sm->task_data.handler = sinkService_MainMessageHandler;

            for (i=0; i < SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET; i++)
            {
                sm->lea_device[i].gatt_cid = INVALID_CID;
            }

            SinkService_ConnectableEnableBredr(TRUE);

            if (SinkService_GetTaskData()->pairing_request_pending)
            {
                SinkService_PairRequest();
                SinkService_GetTaskData()->pairing_request_pending = FALSE;
            }

            SinkService_UpdateUi();
        }
    }
}

void sinkService_DisableAllSmForDualMode(void)
{
    FOR_EACH_SINK_SM(sm)
    {
        if (sm->state == SINK_SERVICE_STATE_DISCONNECTED)
        {
            sm->state = SINK_SERVICE_STATE_DISABLED;

            ConManagerRegisterConnectionsClient(sinkService_GetTaskForSm(sm));
            SinkService_ConnectableEnableBredr(FALSE);

            ConManagerRegisterTpConnectionsObserver(cm_transport_ble, sinkService_GetTaskForSm(sm));
            GattServiceDiscovery_ClientRegister(sinkService_GetTaskForSm(sm));

            SinkService_UpdateUi();
        }
    }
}

#endif /* ENABLE_LE_SINK_SERVICE */

device_t sinkServiceUtil_DetermineSinkDevice(void)
{
    device_t sink = NULL;

    DEBUG_LOG_FN_ENTRY("sinkServiceUtil_DetermineSinkDevice");

    /* Try most recently used (MRU) device first */
    sink = BtDevice_GetMruDevice();

    if (!sink)
    {
        DEBUG_LOG_DEBUG("sinkServiceUtil_DetermineSinkDevice: No MRU device, fall back to first entry on the device list instead");

        /* No MRU device, fall back to first entry on the device list instead */
        deviceType type = DEVICE_TYPE_SINK;
        sink = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type,
                                                          &type, sizeof(deviceType));
    }

    return sink;
}

/*! \brief Get the tranport given sink service SM currently connected with */
sink_service_transport_t sinkServiceUtil_GetConnectedTransport(sink_service_state_machine_t *sm)
{
    sink_service_transport_t connected_transport = SINK_SERVICE_TRANSPORT_UNKNOWN;

    if (sm->sink_device)
    {
        /* A valid BREDR address exists */
        connected_transport = SINK_SERVICE_TRANSPORT_BREDR;
    }
#ifdef ENABLE_LE_SINK_SERVICE
    else
    {
        if (sinkServiceUtil_IsLeTransportConnected(sm))
        {
            connected_transport = SINK_SERVICE_TRANSPORT_LE;
        }
    }
#endif /* ENABLE_LE_SINK_SERVICE */

    return connected_transport;
}
