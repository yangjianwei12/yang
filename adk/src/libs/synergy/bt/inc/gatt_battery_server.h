/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

/*!
@file    gatt_battery_server.h
@brief   Header file for the GATT battery server library.

        This file provides documentation for the GATT battery server library
        API (library name: gatt_battery_server).
*/

#ifndef GATT_BATTERY_SERVER_H_
#define GATT_BATTERY_SERVER_H_

#include "csr_bt_tasks.h"
#include "service_handle.h"

/*! @brief The battery server internal structure for the server role.

    This structure is visible to the application as it is responsible for managing resource to pass to the battery library.
    The elements of this structure are only modified by the battery library.
    The application SHOULD NOT modify the values at any time as it could lead to undefined behaviour.

 */
typedef struct __GBASS
{
    AppTaskData lib_task;
    AppTask app_task;
    bool notifications_enabled;

    CsrBtGattId gattId;
    uint16 start_handle;
    uint16 end_handle;
} GBASS;


/*!
    @brief Status code returned following a GattBatteryServerInit().
*/
typedef enum
{
        /*! Indicates that the GATT Battery server initialisation has completed successfully */
    gatt_battery_server_init_success,
        /*! Indicates that the GATT Battery server initialisation has failed */
    gatt_battery_server_init_failed
} gatt_battery_server_init_status_t;

/*!
    @brief Status code returned from the GATT battery server library

    This status code indicates the outcome of the request.
*/
typedef enum
{
    gatt_battery_server_status_success,
    gatt_battery_server_status_registration_failed,
    gatt_battery_server_status_invalid_parameter,
    gatt_battery_server_status_not_allowed,
    gatt_battery_server_status_failed
} gatt_battery_server_status_t;

typedef uint16 GattBatteryServerMessageId;

/*! @brief Contents of the GATT_BATTERY_SERVER_READ_LEVEL_IND message that is sent by the library,
    due to a read of the local battery level characteristic.
 */
typedef struct  __GATT_BATTERY_SERVER_READ_LEVEL_IND
{
    GattBatteryServerMessageId id;    /* GATT_BATTERY_SERVER_READ_LEVEL_IND */
    const GBASS *battery_server;
    uint32 cid;
} GATT_BATTERY_SERVER_READ_LEVEL_IND_T;

/*! @brief Contents of the GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND message that is sent by the library,
    due to a read of the local battery level client configuration characteristic.
 */
typedef struct __GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND
{
    GattBatteryServerMessageId id;    /* GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND */
    const GBASS *battery_server;
    uint32 cid;
} GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND_T;

/*! @brief Contents of the GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND message that is sent by the library,
    due to a write of the local battery level client configuration characteristic.
 */
typedef struct __GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND
{
    GattBatteryServerMessageId id;   /* GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND */
    const GBASS *battery_server;
    uint32 cid;
    uint16 config_value;
} GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND_T;

/*! @brief Contents of the GATT_BATTERY_SERVER_READ_PRESENTATION_IND message that is sent by the library,
    due to a read of the local presentation characteristic.
 */
typedef struct __GATT_BATTERY_SERVER_READ_PRESENTATION_IND
{
    GattBatteryServerMessageId id;   /* GATT_BATTERY_SERVER_READ_PRESENTATION_IND */
    const GBASS *battery_server;
    uint32 cid;
} GATT_BATTERY_SERVER_READ_PRESENTATION_IND_T;

typedef struct __GATT_BATTERY_SERVER_INIT_IND
{
    GattBatteryServerMessageId id;    /* GATT_BATTERY_SERVER_INIT_IND */
    const GBASS *battery_server;
    gatt_battery_server_init_status_t  status;

} GATT_BATTERY_SERVER_INIT_IND_T;

/*! @brief Enumeration of messages an application task can receive from the battery server library.
 */
typedef enum
{
    /* Server messages */
    GATT_BATTERY_SERVER_INIT_IND,        /* 00 */
    GATT_BATTERY_SERVER_INIT_CFM,                                           /* 01 */
    GATT_BATTERY_SERVER_READ_LEVEL_IND,                                     /* 02 */
    GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND,                             /* 03 */        
    GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND,                            /* 04 */
    GATT_BATTERY_SERVER_READ_PRESENTATION_IND,                              /* 05 */

    /* Library message limit */
    GATT_BATTERY_SERVER_MESSAGE_TOP
} gatt_battery_server_message_id_t;

/*!
    @brief Optional parameters used by the Initialisation API

    Parameters that can define how the battery server library is initialised.
 */
typedef struct
{
   bool enable_notifications;     /*! Flag that can be used to enable or disable notifications */
} gatt_battery_server_init_params_t;

/*!
    @brief Initialises the Battery Service Library in the Server role.

    @param battery_server pointer to server role data
    @param app_task The Task that will receive the messages sent from this battery server library.
    @param init_params pointer to battery server initialization parameters
    @param start_handle start handle
    @param end_handle end handle

    @return TRUE if successful, FALSE otherwise

*/
bool GattBatteryServerInit(GBASS *battery_server,
                           AppTask app_task,
                           const gatt_battery_server_init_params_t *init_params,
                           uint16 start_handle,
                           uint16 end_handle);

/*!
    @brief This API is used to return a battery level to the library when a GATT_BATTERY_SERVER_READ_LEVEL_IND message is received.

    @param battery_server The instance pointer that was in the payload of the GATT_BATTERY_SERVER_READ_LEVEL_IND message.
    @param cid The connection identifier from the GATT_BATTERY_SERVER_READ_LEVEL_IND message.
    @param battery_level The battery level to return to the library. Valid values from 0-100, a value greater than 100% indicates error.

    @return TRUE if successful, FALSE otherwise

*/
bool GattBatteryServerReadLevelResponse(const GBASS *battery_server, uint32 cid, uint8 battery_level);

/*!
    @brief This API is used to return a battery level client configuration value to the library when a GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND message is received.

    @param battery_server The pointer that was in the payload of the GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND message.
    @param cid The connection identifier from the GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND message.
    @param client_config The client configuration to return to the library.

    @return TRUE if successful, FALSE otherwise

*/
bool GattBatteryServerReadClientConfigResponse(const GBASS *battery_server, uint32 cid, uint16 client_config);

/*!
    @brief This API is used to return a battery level to the library when a GATT_BATTERY_SERVER_READ_PRESENTATION_IND message is received.

    @param battery_server The instance pointer that was in the payload of the GATT_BATTERY_SERVER_READ_PRESENTATION_IND message.
    @param cid The connection identifier from the GATT_BATTERY_SERVER_READ_PRESENTATION_IND message.
    @param name_space The name space of the description (see Bluetooth.org Assigned Numbers for allowed values)
    @param description The description of this characteristic (see Bluetooth.org Assigned Numbers for Bluetooth SIG descriptions)

    @return TRUE if successful, FALSE otherwise

*/
bool GattBatteryServerReadPresentationResponse(const GBASS *battery_server, uint32 cid, uint8 name_space, uint16 description);

/*!
    @brief This API is used to notify a remote client of the battery level.
    This will only be allowed if notifications have been enabled by the remote device.

    @param battery_server The instance pointer that was passed into the GattBatteryServerInit API.
    @param number_cids The number of connection ID values passed in the cids parameter.
    @param cids The connection ID list. The battery level notification will be sent on each connection that is supplied in the list.
    @param battery_level The battery level to send in the notification.

    @return TRUE if successful, FALSE otherwise

*/
bool GattBatteryServerSendLevelNotification(const GBASS *battery_server, uint16 number_cids, const uint32 *cids, uint8 battery_level);


#endif
