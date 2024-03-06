/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup sink_service
    \brief      Utility functions for sink service
    @{
*/

#ifndef SINK_SERVICE_UTIL_H
#define SINK_SERVICE_UTIL_H

#include "bt_device.h"
#include "sink_service_private.h"
#include "rssi_pairing.h"

/*! \brief Check if given device type only support BREDR transport */
#define sinkServiceUtil_IsDeviceBredrOnly(device_type)  (device_type == SINK_SERVICE_DEVICE_BREDR)

#ifdef ENABLE_LE_SINK_SERVICE

/*! \brief Check if given device type only support LE transport */
#define sinkServiceUtil_IsDeviceLeOnly(device_type)     (device_type == SINK_SERVICE_DEVICE_LE)

/*! \brief Check if given device type supports both the transport. ie, LE & BREDR */
#define sinkServiceUtil_IsDeviceDual(device_type)       (device_type == SINK_SERVICE_DEVICE_DUAL)

/*! \brief Get the transport to connect for given device based on current sink service mode

    \param[in] device Device

    \return Sink service transport to connect
*/
sink_service_transport_t sinkServiceUtil_GetTargetTransportBasedOnModeForDevice(device_t device);

/*! \brief Check if transport disconnect is needed or not. Disconnect is needed if
     current sink service mode and connected device does not matches.

    \return TRUE if current transport is not appropriate for current mode, FALSE otherwise
*/
bool sinkServiceUtil_IsTransportDisconnectNeeded(void);

/*! \brief Transition the Sink Service State Machine from DISABLED to DISCONNECTED */
void sinkService_EnableAllSmForDualMode(void);

/*! \brief Transition the Sink Service State Machine from DISCONNECTED to DISABLED */
void sinkService_DisableAllSmForDualMode(void);

#else

#define sinkServiceUtil_GetTargetTransportBasedOnModeForDevice(device)      (SINK_SERVICE_TRANSPORT_BREDR)
#define sinkService_EnableAllSmForDualMode()
#define sinkService_DisableAllSmForDualMode()
#define sinkServiceUtil_IsTransportDisconnectNeeded()               (FALSE)
#define sinkServiceUtil_IsDeviceLeOnly(device_type)                 (FALSE)
#define sinkServiceUtil_IsDeviceDual(device_type)                   (FALSE)

#endif /* ENABLE_LE_SINK_SERVICE */

/*! \brief Determine the sink device to connect with.
           If there is an MRU that will be used. If no MRU then the first sink device
           in the device list will be used.

    \return Device to connect. NULL if not able to find any device
*/
device_t sinkServiceUtil_DetermineSinkDevice(void);

/*! \brief Get the connected transport type

    \param[in] sm State machine

    \return connected transport type
*/
sink_service_transport_t sinkServiceUtil_GetConnectedTransport(sink_service_state_machine_t *sm);

#endif /* SINK_SERVICE_UTIL_H */

/*! @} */