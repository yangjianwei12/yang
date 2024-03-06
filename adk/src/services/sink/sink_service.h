/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   sink_service    Sink
    @{
        \ingroup    services
        \brief      A service that provides procedures for connecting and managing
                    connections to sink(s).

                    The sink service is intended to be used with applications that act as a source of audio
                    such as a USB Dongle or Bluetooth transmitter. Its responsibility is to provide connections
                    and connect profiles to sink devices such as headphones, earbuds and speakers.

                    If there are no paired devices, the sink service will utilise the RSSI Pairing manager with
                    configured settings to seek out candidate sink devices.

                    Once a device is paired or an already paired device is retrieved from the device list, the
                    sink service will create a BR/EDR ACL connection with the device before using the profile
                    manager to connect the configured profiles.

                    The Sink Service will retain control of the ACL connection until it receives confirmation
                    of successful or failed profile connections, at which point it will release control of the ACL.

                    When Disabled, the service will disconnect from all devices, and move to a non-connectable state,
                    and all connect requests will be ignored.
*/

#ifndef SINK_SERVICE_H_
#define SINK_SERVICE_H_

#include <message.h>

#include <bdaddr.h>
#include <bt_device.h>

#include "domain_message.h"
#include "rssi_pairing.h"
#include "profile_manager.h"

/*!< \brief Default config define for the maximum number of simultaneous connections
            the Sink Service can support which can be used in sink_service_config_t
            in the application.
     \note This value might not be used in the application.
           In this case, changing this value will have no impact. */
#define SINK_SERVICE_CONFIG_BREDR_CONNECTIONS_MAX 4

#ifdef ENABLE_LE_SINK_SERVICE
/*!< \brief Default config define for the maximum number of simultaneous BLE connections
            the Sink Service can support which can be used in sink_service_config_t
            in the application.
     \note This value might not be used in the application.
           In this case, changing this value will have no impact. */
#define SINK_SERVICE_CONFIG_LE_CONNECTIONS_MAX 1

/*! Number of adv UUID filter supported */
#define SINK_SERVICE_MAX_LEA_UUID_FILTER 8

#endif /* ENABLE_LE_SINK_SERVICE */

/*! \brief Configuration of sink service. */
typedef struct
{
    /*! A bitmask for the supported profiles for sink devices */
    uint32 supported_profile_mask;

    /*! The list of profiles with the order determining the connect order */
    const profile_t * profile_list;

    /*! The length of the profile list */
    size_t profile_list_length;

    /*! The parameters that should be passed to the rssi pairing manager when
        discovering and pairing new devices */
    const rssi_pairing_parameters_t * rssi_pairing_params;

    /*! The maximum number of active bredr connections that the sink service supports */
    uint8 max_bredr_connections;

#ifdef ENABLE_LE_SINK_SERVICE
    /*! The maximum number of active le connections that the sink service supports */
    uint8 max_le_connections;
#endif

    /*! The page timeout (seconds) for connection requests. */
    unsigned page_timeout;

} sink_service_config_t;

/*! This is the config for the sink service. It should be defined in the application */
extern const sink_service_config_t sink_service_config;

/*! \brief Status codes for the sink service. */
typedef enum
{
    sink_service_status_success,
    sink_service_status_failed,
    sink_service_status_cancelled,
    sink_service_status_no_mru,
    sink_service_status_connected,
    sink_service_status_disconnected,
    sink_service_status_link_loss,
} sink_service_status_t;

/*! \brief Events sent by the sink service to other modules. */
typedef enum
{
    /*! Module initialisation complete */
    SINK_INIT_CFM = SINK_SERVICE_MESSAGE_BASE,
    SINK_SERVICE_CONNECTED_CFM,
    SINK_SERVICE_DISCONNECTED_CFM,

    /*! The first profile has been connected by the profile manager */
    SINK_SERVICE_FIRST_PROFILE_CONNECTED_IND,

    /*! This must be the final message */
    SINK_SERVICE_MESSAGE_END
} sink_service_msg_t;

/*! \brief Sink Service UI Provider contexts */
typedef enum
{
    context_sink_connected,
    context_sink_connecting,
    context_sink_disconnected,
    context_sink_pairing,
    context_sink_disabled
} sink_service_context_t;

/*! \brief Different modes supported by sink service */
typedef enum
{
    /*! Sink service should connect with a BREDR device. ie, either BREDR only or dual mode device */
    SINK_SERVICE_MODE_BREDR,

#ifdef ENABLE_LE_SINK_SERVICE
    /*! Sink service should connect with an LE device. ie, either LE only or dual mode device */
    SINK_SERVICE_MODE_LE,

    /*! Sink service should connect with either LE/BREDR/dual mode device. If device is dual
        mode, it should use LE transport */
    SINK_SERVICE_MODE_DUAL_PREF_LE,

    /*! Sink service should connect with either LE/BREDR/dual mode device. If device is dual
        mode, it should use BREDR transport. */
    SINK_SERVICE_MODE_DUAL_PREF_BREDR,
#endif /* ENABLE_LE_SINK_SERVICE */

    SINK_SERVICE_MODE_INVALID = 0xFF

} sink_service_mode_t;

/*! \brief Different transport types supported by sink service */
typedef enum
{
    /*! Transport type is not known */
    SINK_SERVICE_TRANSPORT_UNKNOWN,

    /*! Transport is over BREDR */
    SINK_SERVICE_TRANSPORT_BREDR,

    /*! Transport is over LE */
    SINK_SERVICE_TRANSPORT_LE,
} sink_service_transport_t;

/*! \brief Definition of the #SINK_SERVICE_CONNECTED_CFM_T message content */
typedef struct
{
    /*! Connected Device. */
    device_t              device;

    /*! Connected transport */
    sink_service_transport_t transport;

    /*! Status of the request */
    sink_service_status_t status;
} SINK_SERVICE_CONNECTED_CFM_T;

/*! \brief Definition of the #SINK_SERVICE_DISCONNECTED_CFM_T message content */
typedef struct
{
    /*! Disconnected Device. */
    device_t              device;
} SINK_SERVICE_DISCONNECTED_CFM_T;

/*! \brief Definition of the #SINK_SERVICE_FIRST_PROFILE_CONNECTED_IND_T message content */
typedef struct
{
    /*! Device connecting profile. */
    device_t              device;
} SINK_SERVICE_FIRST_PROFILE_CONNECTED_IND_T;

/*! \brief Initialise the sink_service module.

    \param task The init task to send SINK_SERVICE_INIT_CFM to.

    \return TRUE if initialisation is in progress; FALSE if it failed.
*/
bool SinkService_Init(Task task);

/*! \brief Register a Task to receive notifications from sink_service.

    Once registered, #client_task will receive #sink_msg_t messages.

    \param client_task Task to register to receive sink_service notifications.
*/
void SinkService_ClientRegister(Task client_task);

/*! \brief Enable Pairing in the Sink Service.

    If pairing is not enabled and there are no devices. A call to Connect will do nothing

    \param enable True if enable.
*/
void SinkService_EnablePairing(bool enable);

/*! \brief Disable the Sink Service.

    When disabled the sink service will hold it's state and not handle any incomming messages
*/
void SinkService_Disable(void);

/*! \brief Enable the Sink Service.

    When enabled the sink service will react to messages and ui inputs.

    \note The function itself does not perform any actions within in the sink service or
          notify the state machine instances. This simple stops all of the instances from
          reacting to messages and events.
*/
void SinkService_Enable(void);

/*! \brief Set Sink Service operating mode.

    \param mode Operation mode for sink service.

    \return True if there is no device connected or if the new operating mode doesn't require any change in
            currently connected transport with the sink device.
            FALSE if the new operating mode requires disconnection of current transport. In such case,
            SinkService_DisconnectAll() needs to be called followed by SinkService_Connect().

    \note The function needs to be called before \ref SinkService_Enable, otherwise undesired behviour would be seen.
*/
bool SinkService_SetMode(sink_service_mode_t mode);

/*! \brief Request the Sink Service to Connect to a device

    If there are no devices in the device list then this will start pairing (using RSSI pairing)
    If there is a device in the device list it will either use the MRU or the first device in the list.
    After pairing or creating a BREDR connection. The configured profiles will be connected.
    SINK_SERVICE_CONNECTED_CFM should be expected when a connection is successful.

    \note The Sink Service only supports initiating connection to a single device.

    \return True if the request was successfully made.
*/
bool SinkService_Connect(void);

/*! \brief Request the Sink Service to Disconnect from all devices

    This will request all instances to disconnect from their devices.

    \return True if the request was successfully made.
*/
void SinkService_DisconnectAll(void);

/*! \brief Un-register a Task that is receiving notifications from sink_service.

    If the task is not currently registered then nothing will be changed.

    \param client_task Task to un-register from handet_service notifications.
*/
void SinkService_ClientUnregister(Task client_task);

/*! \brief Check if any of the sink service instances are in the connected state

    \return TRUE if an instance is connected
*/
bool SinkService_IsConnected(void);

/*! \brief Check if any of the sink service instances are in the pairing state

    \return TRUE if an instance is in pairing state, FALSE otherwise
*/
bool SinkService_IsInPairingState(void);

/*! \brief Enable BR/EDR connections */
void SinkService_ConnectableEnableBredr(bool enable);

#ifdef ENABLE_LE_SINK_SERVICE

/*! \brief Set advertisement UUID filter to use during RSSI pairing.
           If any UUID in the provided list is present in advertisement,
           it should be considered.

    \param[in] num_of_uuids Number of UUID filters. This can be 0 if no filtering is
                            needed. For maximum value, \ref SINK_SERVICE_MAX_LEA_UUID_FILTER
    \param[in] uuid_list List of UUID's. This can be NULL if no filtering is needed.

    \return TRUE if success, FALSE otherwise
*/
bool SinkService_SetUuidAdvFilter(uint8 num_of_uuids, uint16 uuid_list[]);

#endif /* ENABLE_LE_SINK_SERVICE */

#endif /* SINK_SERVICE_H_ */

/*! @} */