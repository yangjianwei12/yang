/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup sink_service
    \brief      LE state machine handler interfaces.
    @{
*/

#ifndef SINK_SERVICE_LE_SM_H_
#define SINK_SERVICE_LE_SM_H_

#ifdef ENABLE_LE_SINK_SERVICE

#include <device.h>
#include <message.h>
#include <bdaddr.h>
#include <connection_manager.h>
#include "sink_service_protected.h"
#include "sink_service_private.h"

/*! \brief Register for GATT Notifications
*/
void SinkServiceLeSm_RegisterForGattNotifications(void);

/*! \brief Clear the data for an SM instance and reset it

    \param sm The SM to reset
*/
void SinkServiceLeSm_ClearInstance(sink_service_state_machine_t *sm);

/*! \brief Request a SM instance attempts to connect to a device

    \param sm The SM to connect
    \return TRUE if the request was successfully made
*/
bool SinkServiceLeSm_ConnectRequest(sink_service_state_machine_t *sm);

/*! \brief Request a SM instance attempts to disconnect all profiles from its device.
           This function will disconnect all configured profiles from the SM device

    \param sm The SM to disconnect
    \return TRUE if the request was successfully made
*/
bool SinkServiceLeSm_DisconnectRequest(sink_service_state_machine_t *sm);

/*! \brief Have max supported LE connections been established?

    \return TRUE if max LE connections reached, otherwise FALSE
*/
bool SinkServiceLeSm_MaxLeAclConnectionsReached(void);

/*! \brief Get the number of LE ACL connections
    \return Number of LE ACL connections.
*/
unsigned SinkServiceLeSm_GetLeAclConnectionCount(void);

/*! \brief Create a new instance of a sink state machine.

    This will return NULL if a new state machine cannot be created,
    for example if the maximum number of sinks already exists.

    \param device Device to create state machine for.

    \return Pointer to new state machine, or NULL if it couldn't be created.
*/
sink_service_state_machine_t *SinkServiceLeSm_CreateSm(device_t device);

/*! \brief Try to find an active sink state machine for a device_t.

    \param device Device to search for.
    \return Pointer to the matching state machine, or NULL if no match.
*/
sink_service_state_machine_t *SinkServiceLeSm_GetSmForDevice(device_t device);

/*! \brief Try to find an active LE sink state machine for an address.

    \param[in] addr LE address to search for.

    \return Pointer to the matching state machine, or NULL if no match.
*/
sink_service_state_machine_t *SinkServiceLeSm_GetSmFromTpaddr(tp_bdaddr *addr);

/*! \brief Transition the Sink Service State Machine from DISCONNECTED to DISABLED */
void SinkServiceLeSm_DisableAll(void);

/*! \brief Transition the Sink Service State Machine from DISABLED to DISCONNECTED */
void SinkServiceLeSm_EnableAll(void);

/*! \brief Message handler to route all messages when sink service is using LE SM

    \param[in] sm State machine
    \param[in] id Received message id
    \param[in] message Received message data
*/
void SinkServiceLeSm_HandleMessage(sink_service_state_machine_t *sm, MessageId id, Message message);

/*! \brief Used to sets the state of given sm when sink service is using BREDR SM

    \param[in] sm State machine
    \param[in] id Received message id
    \param[in] message Received message data
*/
void sinkServiceLeSm_SetState(sink_service_state_machine_t *sm, sink_service_state_t state);

/*! \brief Check if sink service is connected with any LE device or not

    \param[in] sm State machine

    \return TRUE if LE device connected, FALSE otherwise
*/
bool sinkServiceLeSm_IsAnyLeDeviceConnected(sink_service_state_machine_t *sm);

/*! \brief Get the LE device info of given BD address in the given sm (if it exists)

    \param[in] sm State machine
    \param[in] addr Address of device to find

    \return Pointer to the matching device info, or NULL if no match.
*/
lea_device_info_t * sinkServiceLeSm_GetLeDeviceInfoByAddr(sink_service_state_machine_t *sm, bdaddr *addr);

/*! \brief Check if given device is paired or nor

    \param[in] device Device to check

    \return TRUE if device is paired, FALSE otherwise
*/
bool sinkServiceLeSm_DeviceIsPaired(device_t device);

/*! \brief Clear the LE device info of given BD address in the given sm (if it exists)

    \param[in] sm State machine
    \param[in] addr Address of device to find
*/
void sinkServiceLeSm_ClearLeDeviceInfoByAddr(sink_service_state_machine_t *sm, bdaddr *addr);

/*! \brief Add the LE device info for given BD address in the given sm (if it exists)

    \param[in] sm State machine
    \param[in] addr Address of device

    \return TRUE if able to add the device info, FALSE otherwise
*/
bool sinkServiceLeSm_AddLeDeviceInfo(sink_service_state_machine_t *sm, tp_bdaddr tp_addr);

/*! \brief Handle service discovery complete indication for the given cid

    \param[in] addr Address of device
    \param[in] sm State machine
*/
void sinkServiceLeSm_HandleServiceDiscoveryComplete(gatt_cid_t cid, sink_service_state_machine_t *sm);

/*! \brief Iterate through all the LE devices and delete the device if not paired

    \param[in] sm State machine
*/
void sinkServiceLeSm_DeleteDeviceIfNotPaired(sink_service_state_machine_t *sm);

/*! \brief Remove all devices from whitelist
*/
void SinkServiceLeSm_RemoveAllDevicesFromWhitelist(void);

#else

#define SinkServiceLeSm_ConnectRequest(sm)              (FALSE)
#define SinkServiceLeSm_DisconnectRequest(sm)           (FALSE)
#define SinkServiceLeSm_EnableAll()
#define SinkServiceLeSm_DisableAll()
#define SinkServiceLeSm_HandleMessage(sm, id, message)
#define SinkServiceLeSm_RemoveAllDevicesFromWhitelist()

#endif /* ENABLE_LE_SINK_SERVICE */

#endif /* SINK_SERVICE_LE_SM_H_ */

/*! @} */
