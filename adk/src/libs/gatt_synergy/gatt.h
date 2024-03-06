/*****************************************************************************
Copyright (c) 2011 - 2022 Qualcomm Technologies International, Ltd.


FILE NAME
    gatt.h

DESCRIPTION
    Header file for the GATT library
*/

/*!
\defgroup gatt gatt
\ingroup vm_libs

\brief      Header file for the GATT library

\section gatt_intro INTRODUCTION
    This file provides documentation for the GATT library API.

@{

*/


#ifndef GATT_H_
#define GATT_H_

#include <bdaddr_.h>
#include <library.h>
#include <message_.h>


/*****************************************************************************
GATT Service UUID's see (https://developer.bluetooth.org/gatt/services/Pages/ServicesHome.aspx)
*****************************************************************************/
#define GATT_SERVICE_UUID_ALERT_NOTIFICATION_SERVICE                        (0x1811)
#define GATT_SERVICE_UUID_BATTERY_SERVICE                                   (0x180F)
#define GATT_SERVICE_UUID_BLOOD_PRESSURE                                    (0x1810)
#define GATT_SERVICE_UUID_CURRENT_TIME_SERVICE                              (0x1805)
#define GATT_SERVICE_UUID_CYCLING_POWER                                     (0x1818)
#define GATT_SERVICE_UUID_CYCLING_SPEED_AND_CADENCE                         (0x1816)
#define GATT_SERVICE_UUID_DEVICE_INFORMATION                                (0x180A)
#define GATT_SERVICE_UUID_GENERIC_ACCESS                                    (0x1800)
#define GATT_SERVICE_UUID_GENERIC_ATTRIBUTE                                 (0x1801)
#define GATT_SERVICE_UUID_GLUCOSE                                           (0x1808)
#define GATT_SERVICE_UUID_HEALTH_THERMOMETE                                 (0x1809)
#define GATT_SERVICE_UUID_HEART_RATE                                        (0x180D)
#define GATT_SERVICE_UUID_HUMAN_INTERFACE_DEVICE                            (0x1812)
#define GATT_SERVICE_UUID_IMMEDIATE_ALERT                                   (0x1802)
#define GATT_SERVICE_UUID_LINK_LOSS                                         (0x1803)
#define GATT_SERVICE_UUID_LOCATION_AND_NAVIGATION                           (0x1819)
#define GATT_SERVICE_UUID_NEXT_DSG_CHANGE_SERVICE                           (0x1807)
#define GATT_SERVICE_UUID_PHONE_ALERT_STATUS_SERVICE                        (0x180E)
#define GATT_SERVICE_UUID_REFERENCE_TIME_UPDATE_SERVICE                     (0x1806)
#define GATT_SERVICE_UUID_RUNNING_SPEED_AND_CADENCE                         (0x1814)
#define GATT_SERVICE_UUID_SCAN_PARAMETERS                                   (0x1813)
#define GATT_SERVICE_UUID_TX_POWER                                          (0x1804)


/*****************************************************************************
GATT Characteristic UUID's see (https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicsHome.aspx)
*****************************************************************************/
#define GATT_CHARACTERISTIC_UUID_ALERT_CATEGORY_ID                          (0x2A43)
#define GATT_CHARACTERISTIC_UUID_ALERT_CATEGORY_ID_BIT_MASK                 (0x2A42)
#define GATT_CHARACTERISTIC_UUID_ALERT_LEVEL                                (0x2A06)
#define GATT_CHARACTERISTIC_UUID_ALERT_NOTIFICATION_CONTROL_POINT           (0x2A44)
#define GATT_CHARACTERISTIC_UUID_ALERT_STATUS                               (0x2A3F)
#define GATT_CHARACTERISTIC_UUID_APPERANCE                                  (0x2A01)
#define GATT_CHARACTERISTIC_UUID_BATTERY_LEVEL                              (0x2A19)
#define GATT_CHARACTERISTIC_UUID_BLOOD_PRESSURE_FEATURE                     (0x2A49)
#define GATT_CHARACTERISTIC_UUID_BLOOD_PRESSURE_MEASUREMENT                 (0x2A35)
#define GATT_CHARACTERISTIC_UUID_BODY_SENSOR_LOCATION                       (0x2A38)
#define GATT_CHARACTERISTIC_UUID_BOOT_KEYBOARD_INPUT_REPORT                 (0x2A22)
#define GATT_CHARACTERISTIC_UUID_BOOT_KEYBOARD_OUTPUT_REPORT                (0x2A32)
#define GATT_CHARACTERISTIC_UUID_BOOT_MOUSE_INPUT_REPORT                    (0x2A33)
#define GATT_CHARACTERISTIC_UUID_CLIENT_SUPPORTED_FEATURES                  (0x2B29)
#define GATT_CHARACTERISTIC_UUID_CSC_FEATURE                                (0x2A5C)
#define GATT_CHARACTERISTIC_UUID_CSC_MEASUREMENT                            (0x2A5B)
#define GATT_CHARACTERISTIC_UUID_CURRENT_TIME                               (0x2A2B)
#define GATT_CHARACTERISTIC_UUID_CYCLING_POWER                              (0x2A66)
#define GATT_CHARACTERISTIC_UUID_CYCLING_POWER_FEATURE                      (0x2A65)
#define GATT_CHARACTERISTIC_UUID_CYCLING                                    (0x2A63)
#define GATT_CHARACTERISTIC_UUID_CYCLING_POWER_VECTOR                       (0x2A64)
#define GATT_CHARACTERISTIC_UUID_DATABASE_HASH                              (0x2B2A)
#define GATT_CHARACTERISTIC_UUID_DATE_TIME                                  (0x2A08)
#define GATT_CHARACTERISTIC_UUID_DAY_DATE_TIME                              (0x2A0A)
#define GATT_CHARACTERISTIC_UUID_DAY_OF_WEEK                                (0x2A09)
#define GATT_CHARACTERISTIC_UUID_DEVICE_NAME                                (0x2A00)
#define GATT_CHARACTERISTIC_UUID_DST_OFFSET                                 (0x2A0D)
#define GATT_CHARACTERISTIC_UUID_EXACT_TIME_256                             (0x2A0C)
#define GATT_CHARACTERISTIC_UUID_FIRMWARE_REVISION_STRING                   (0x2A26)
#define GATT_CHARACTERISTIC_UUID_GLUCOSE_FEATURE                            (0x2A51)
#define GATT_CHARACTERISTIC_UUID_GLUCOSE_MEASUREMENT                        (0x2A18)
#define GATT_CHARACTERISTIC_UUID_GLUCOSE_MEASUREMENT_CONTEXT                (0x2A34)
#define GATT_CHARACTERISTIC_UUID_HARDWARE_REVISION_STRING                   (0x2A27)
#define GATT_CHARACTERISTIC_UUID_HEART_RATE_CONTROL_POINT                   (0x2A39)
#define GATT_CHARACTERISTIC_UUID_HEART_RATE_MEASUREMENT                     (0x2A37)
#define GATT_CHARACTERISTIC_UUID_HID_CONTROL_POINT                          (0x2A4C)
#define GATT_CHARACTERISTIC_UUID_HID_INFORMATION                            (0x2A4A)
#define GATT_CHARACTERISTIC_UUID_IEEE_11073                                 (0x2A2A)
#define GATT_CHARACTERISTIC_UUID_INTERMEDIATE_CUFF_PRESSURE                 (0x2A36)
#define GATT_CHARACTERISTIC_UUID_INTERMEDIATE_TEMPERATURE                   (0x2A1E)
#define GATT_CHARACTERISTIC_UUID_LN                                         (0x2A6B)
#define GATT_CHARACTERISTIC_UUID_LN_FEATURE                                 (0x2A6A)
#define GATT_CHARACTERISTIC_UUID_LOCAL_TIME_INFORMATION                     (0x2A0F)
#define GATT_CHARACTERISTIC_UUID_LOCATION                                   (0x2A67)
#define GATT_CHARACTERISTIC_UUID_MANUFACTURER_NAME_STRING                   (0x2A29)
#define GATT_CHARACTERISTIC_UUID_MEASUREMENT_INTERVAL                       (0x2A21)
#define GATT_CHARACTERISTIC_UUID_MODEL_NUMBER_STRING                        (0x2A24)
#define GATT_CHARACTERISTIC_UUID_NAVIGATION                                 (0x2A68)
#define GATT_CHARACTERISTIC_UUID_NEW_ALERT                                  (0x2A46)
#define GATT_CHARACTERISTIC_UUID_PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS (0x2A04)
#define GATT_CHARACTERISTIC_UUID_PERIPHERAL_PRIVACY_FLAG                    (0x2A02)
#define GATT_CHARACTERISTIC_UUID_PNP_ID                                     (0x2A50)
#define GATT_CHARACTERISTIC_UUID_POSITION_QUALITY                           (0x2A69)
#define GATT_CHARACTERISTIC_UUID_PROTOCOL_MODE                              (0x2A4E)
#define GATT_CHARACTERISTIC_UUID_RECONNECTION_ADDRESS                       (0x2A03)
#define GATT_CHARACTERISTIC_UUID_RECORD_ACCESS_CONTROL_POINT                (0x2A52)
#define GATT_CHARACTERISTIC_UUID_REFERENCE_TIME_INFORMATION                 (0x2A14)
#define GATT_CHARACTERISTIC_UUID_REPORT                                     (0x2A4D)
#define GATT_CHARACTERISTIC_UUID_REPORT_MAP                                 (0x2A4B)
#define GATT_CHARACTERISTIC_UUID_RINGER_CONTROL_POINT                       (0x2A40)
#define GATT_CHARACTERISTIC_UUID_RINGER_SETTING                             (0x2A41)
#define GATT_CHARACTERISTIC_UUID_RSC_FEATURE                                (0x2A54)
#define GATT_CHARACTERISTIC_UUID_RSC_MEASUREMENT                            (0x2A53)
#define GATT_CHARACTERISTIC_UUID_SC_CONTROL_POINT                           (0x2A55)
#define GATT_CHARACTERISTIC_UUID_SCAN_INTERVAL_WINDOW                       (0x2A4F)
#define GATT_CHARACTERISTIC_UUID_SCAN_REFRESH                               (0x2A31)
#define GATT_CHARACTERISTIC_UUID_SENSOR_LOCATION                            (0x2A5D)
#define GATT_CHARACTERISTIC_UUID_SERIAL_NUMBER_STRING                       (0x2A25)
#define GATT_CHARACTERISTIC_UUID_SERVICE_CHANGED                            (0x2A05)
#define GATT_CHARACTERISTIC_UUID_SOFTWARE_REVISION_STRING                   (0x2A28)
#define GATT_CHARACTERISTIC_UUID_SUPPORTED_NEW_ALERT_CATEGORY               (0x2A47)
#define GATT_CHARACTERISTIC_UUID_SUPPORTED_UNREAD_ALERT_CATEGORY            (0x2A48)
#define GATT_CHARACTERISTIC_UUID_SYSTEM_ID                                  (0x2A23)
#define GATT_CHARACTERISTIC_UUID_TEMPERATURE_MEASUREMENT                    (0x2A1C)
#define GATT_CHARACTERISTIC_UUID_TEMPERATURE_TYPE                           (0x2A1D)
#define GATT_CHARACTERISTIC_UUID_TIME_ACCURACY                              (0x2A12)
#define GATT_CHARACTERISTIC_UUID_TIME_SOURCE                                (0x2A13)
#define GATT_CHARACTERISTIC_UUID_TIME_UPDATE_CONTROL_POINT                  (0x2A16)
#define GATT_CHARACTERISTIC_UUID_TIME_UPDATE_STATE                          (0x2A17)
#define GATT_CHARACTERISTIC_UUID_TIME_WITH_DST                              (0x2A11)
#define GATT_CHARACTERISTIC_UUID_TIME_ZONE                                  (0x2A0E)
#define GATT_CHARACTERISTIC_UUID_TX_POWER_LEVEL                             (0x2A07)
#define GATT_CHARACTERISTIC_UUID_UNREAD_ALERT_STATUS                        (0x2A45)


/*****************************************************************************
Characteristic Declaration UUIDs see (https://developer.bluetooth.org/gatt/declarations/Pages/DeclarationsHome.aspx)
*****************************************************************************/
#define GATT_CHARACTERISTIC_DECLARATION_UUID                                (0x2803)
#define GATT_INCLUDE_DECLARATION_UUID                                       (0x2802)
#define GATT_PRIMARY_SERVICE_DECLARATION_UUID                               (0x2800)
#define GATT_SECONDARY_SERVICE_DECLARATION_UUID                             (0x2801)


/*****************************************************************************
Characteristic Descriptor UUIDs see (https://developer.bluetooth.org/gatt/descriptors/Pages/DescriptorsHomePage.aspx)
*****************************************************************************/
#define GATT_CHARACTERISTIC_EXTENDED_PROPERTIES_UUID                        (0x2900)
#define GATT_CHARACTERISTIC_USER_DESCRIPTION_UUID                           (0x2901)
#define GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID                       (0x2902)
#define GATT_CHARACTERISTIC_PRESENTATION_FORMAT_UUID                        (0x2904)
#define GATT_CHARACTERISTIC_AGGREGATE_FORMAT_UUID                           (0x2905)
#define GATT_VALID_RANGE_UUID                                               (0x2906)
#define GATT_EXTERNAL_REPORT_REFERENCE_UUID                                 (0x2907)
#define GATT_REPORT_REFERENCE_UUID                                          (0x2908)


/*****************************************************************************
Characteristic properties (as defined in the Bluetooth Core Specification)
*****************************************************************************/
typedef enum
{
    gatt_char_prop_broadcast          = 0x1,      /* If set, permits broadcast of the characteristic value using characteristic configuration descriptor */
    gatt_char_prop_read               = 0x2,      /* If set, permits reads of the characteristic value */
    gatt_char_prop_write_no_response  = 0x4,      /* If set, permits writes of the characteristic value without response */
    gatt_char_prop_write              = 0x8,      /* If set, permits writes of the characteristic value with response */
    gatt_char_prop_notify             = 0x10,     /* If set, permits notifications of a characteristic value without acknowledgement*/
    gatt_char_prop_indicate           = 0x20,     /* If set, permits indications of a characteristic value with acknowledgement */
    gatt_char_prop_auth_signed_writes = 0x40,     /* If set, permits signed writes to the characteristic value */
    gatt_char_prop_extended_props     = 0x80      /* If set, additional characteristic properties are defined in the characteristic extended properties descriptor */
} gatt_characteristic_properties_t;


/*****************************************************************************
Report Reference - report characteristic types (as defined in the Bluetooth Core Specification)
*****************************************************************************/
typedef enum
{
    gatt_report_reference_reserved       = 0,     /* Key value '0' is reserved */
    gatt_report_reference_input_report   = 1,     /* If set, the associated report characteristic is an input report */
    gatt_report_reference_output_report  = 2,     /* If set, the associated report characteristic is an output report */
    gatt_report_reference_feature_report = 3      /* If set, the associated report characteristic is a feature report */
    /* Key values: 4-255 are also reserved */
} gatt_report_reference_report_types_t;


/*****************************************************************************
Client Characteristic Configuration properties (16bit format, as defined in the Bluetooth Core Specification)
*****************************************************************************/
#define GATT_CLIENT_CHAR_CONFIG_SIZE (2)

typedef enum
{
    gatt_client_char_config_notifications_enabled = (1 << 0), /* If set, notifications for client characteristic are enabled */
    gatt_client_char_config_indications_enabled   = (1 << 1)  /* If set, indications for client characteristic are enabled */
} gatt_client_characteristic_configuration_properties_t;


/*****************************************************************************
Constants, enumerations, types
*****************************************************************************/
typedef enum
{
    csf_robust_caching_enable           = 0x01   /* Enables robust caching. 0b00000001 */
} gatt_client_supported_features;

/*!
    \brief GATT library return status 
*/

/*****************************************************************************
These status codes are defined in the Bluetooth Core Specification (Vol 3, Part F, Section 3.4.1.1), and the
Core Specification Supplement (Part B, Section 1.2).
*****************************************************************************/

typedef enum
{
    /*! The operation was successful. */
    gatt_status_success                         = 0x00,
    /*! The attribute handle given was not valid */
    gatt_status_invalid_handle,
    /*! The attribute cannot be read */
    gatt_status_read_not_permitted,
    /*! The attribute cannot be written */
    gatt_status_write_not_permitted,
    /*! The attribute PDU was invalid */
    gatt_status_invalid_pdu,
    /*! The attribute requires an authentication before it can be read or
        written */
    gatt_status_insufficient_authentication,
    /*! Target device doesn't support request */
    gatt_status_request_not_supported,
    /*! Offset specified was past the end of the long attribute */
    gatt_status_invalid_offset,
    /*! The attribute requires authorization before it can be read or
        written */
    gatt_status_insufficient_authorization,
    /*! Too many prepare writes have been queued */
    gatt_status_prepare_queue_full,
    /*! No attribute found within the given attribute handle range. */
    gatt_status_attr_not_found,
    /*! This attribute cannot be read or written using the Read Blob Request
        or Write Blob Requests. */
    gatt_status_not_long,
    /*! The Encryption Key Size used for encrypting this link is
        insufficient. */
    gatt_status_insufficient_encr_key_size,
    /*! The attribute value length is invalid for the operation. */
    gatt_status_invalid_length,
    /*! The attribute request that was requested has encountered an error
        that was very unlikely, and therefore could not be completed as
        requested. */
    gatt_status_unlikely_error,
    /*! The attribute requires encryption before it can be read or written */
    gatt_status_insufficient_encryption,
    /*! The attribute type is not a supported grouping attribute as defined
        by a higher layer specification. */
    gatt_status_unsupported_group_type,
    /*! Insufficient Resources to complete the request. */
    gatt_status_insufficient_resources,
    /*! The server requests the client to rediscover the database. */
    gatt_status_db_out_of_sync,
    /*! The attribute parameter value was not allowed. */
    gatt_status_value_not_allowed,
    
    /*! Error Codes 0x14 - 0x7F are reserved for future use. */
    
    /*! Application error to indicate a attribute request not valid for the
        current radio type */
    gatt_status_application_error = 0x0080,
    
    /*! Error Codes 0x80 - 0x9F are Application error codes defined by a higher layer specification. */
    
    /*! Error Codes 0xA0 - 0xDF are reserved for future use. */
    
    /*! Error Codes 0xE0 - 0xFF are common profile and service error codes defined in the Core Specification Supplement, Part B, Section 1.2. */
    /*! Error Codes 0xE0 - 0xFB are reserved for future use. */
    
    /*! The write request was rejected. */
    gatt_status_write_request_rejected = 0x00FC,
    /*! The Client Characteristic Configuration Descriptor is improperly configured. */
    gatt_status_cccd_improper_config,
    /*! The attempted procedure is already in progress. */
    gatt_status_procedure_already_in_progress,
    /*! The connected device is out of range. */
    gatt_status_out_of_range,
    
    
    /*! Connection is initialising */
    gatt_status_initialising,

    /*! Generic failure status. */
    gatt_status_failure,
    /*! Failed to register with the ATT protocol (initialisation). */
    gatt_status_att_reg_failure,
    /*! ATT Database registration failed (initialisation). */
    gatt_status_att_db_failure,
    /*! Max Number of ATT connections have already been made. */
    gatt_status_max_connections,
    /*! ATT disconnected abnormally (L2CAP Disconnection). */
    gatt_status_abnormal_disconnection,
    /*! ATT disconnected because of Link Loss. */
    gatt_status_link_loss,
    /*! MTU can only be exchanged once per connection. */
    gatt_status_mtu_already_exchanged,
    /*! Characteristic Value returned by the server did not match the
      requested one. */
    gatt_status_value_mismatch,

    /*! Connection was rejected because of PSM */
    gatt_status_rej_psm,
    /*! Connection was rejected because of security */
    gatt_status_rej_security,
    /*! Connection was rejected because of missing link key */    
    gatt_status_key_missing,
    /*! Connection timed out */
    gatt_status_connection_timeout,
    /*! Connection retrying */
    gatt_status_retrying,
    /*! Peer aborted the connection */
    gatt_status_peer_aborted,
    
    /*! Error to indicate that request to DM can not be completed because
        device ACL entity is not found */
    gatt_status_device_not_found = 0x7f73,
    /*! Attribute signing failed. */
    gatt_status_sign_failed,
    /*! Operation can't be done now. */
    gatt_status_busy,
    /*! Current operation timed out. */
    gatt_status_timeout,
    /*! Invalid MTU */
    gatt_status_invalid_mtu,
    /*! Invalid UUID type */
    gatt_status_invalid_uuid,
    /*! Operation was successful, and more responses will follow */
    gatt_status_success_more,
    /*! Indication sent, awaiting confirmation from the client */
    gatt_status_success_sent,
    /*! Invalid connection identifier */
    gatt_status_invalid_cid,
    /*! Attribute database is invalid */
    gatt_status_invalid_db,
    /*! Attribute server database is full */
    gatt_status_db_full,
    /*! Requested server instance is not valid */
    gatt_status_invalid_phandle,
    /*! Attribute permissions are not valid */
    gatt_status_invalid_permissions,    
    /*! Signed write done on an encrypted link */
    gatt_result_signed_disallowed,
#ifdef BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE
    /*! It indicates that encryption is in process */
    gatt_result_encryption_pending,
#endif /* BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE */
    /*! The invalid parameters */
    gatt_result_invalid_params,
    /*! Connection to remote device already exists */
    gatt_status_already_connected
} gatt_status_t;


/*!
    \brief Transport of ATT link
*/
typedef enum
{
    /*! BR/EDR Master. */
    gatt_connection_bredr_master,
    /*! BLE Master Directed. */
    gatt_connection_ble_master_directed,
    /*! BLE Master Whitelist. */
    gatt_connection_ble_master_whitelist,
    /*! BLE Slave Directed High Duty Cycle. */
    gatt_connection_ble_slave_directed_high_duty,
    /*! BLE Slave Directed - for legacy support - same as Directed High Duty. */
    gatt_connection_ble_slave_directed = 
        gatt_connection_ble_slave_directed_high_duty,
    /*! BLE Slave Whitelist. */
    gatt_connection_ble_slave_whitelist,
    /*! BLE Slave Undirected.*/
    gatt_connection_ble_slave_undirected,
    /*! BLE Slave Directed Low Duty Cycle. */
    gatt_connection_ble_slave_directed_low_duty
} gatt_connection_type;

/*! 
    \name GATT Access Permission Flags 

    \{
*/
/*! Request to read attribute value */
#define ATT_ACCESS_READ                         0x0001
/*! Request to write attribute value. */
#define ATT_ACCESS_WRITE                        0x0002
/*! Request for sufficient permission for r/w of attribute value. */
#define ATT_ACCESS_PERMISSION                   0x8000
/*! Indication of completed write, rsp mandatory */
#define ATT_ACCESS_WRITE_COMPLETE               0x4000
/*! Indication that GATT will request APP for enc key len*/
#define ATT_ACCESS_ENCRYPTION_KEY_LEN           0x2000
/*! \} */

/*!
    \brief UUID type definitions for GATT.
*/
typedef enum
{
    /*! UUID is not present */
    gatt_uuid_none,
    /*! UUID is a 16-bit Attribute UUID */
    gatt_uuid16,
    /*! UUID is a 128-bit UUID */
    gatt_uuid128,
    /*! UUID is a 32-bit Attribute UUID */
    gatt_uuid32
} gatt_uuid_type_t;

/*!
    \brief GATT UUID type

    All GATT UUID are stored gatt_uuid_t array of 4 elements and are Big Endian. 
    A 16-bit Attribute UUID and 32-bit UUID are stored in only the first 
    element of the array [0]. 

    For example, the 16-bit UUID 0x2093 is stored as...

        gatt_uuid_t [0] = 0x00002093 
        gatt_uuid_t [1] - ignored
        gatt_uuid_t [2] - ignored
        gatt_uuid_t [3] - ignored
    
    Note: The top 16-bits of the first element must be 0.

    The 32-bit UUID 0xA5054492 is stored as...

        gatt_uuid_t [0] = 0xA5054492
        gatt_uuid_t [1] - ignored
        gatt_uuid_t [2] - ignored
        gatt_uuid_t [3] - ignored
    
    A 128-bit UUID is stored in all four elementents [0..3].

    For example 128-bit UUID bit UUID 00112233-4455-6677-8899-aabbccddeeff is 
    stored as...

        gatt_uuid_t [0] = 0x00112233
        gatt_uuid_t [1] = 0x44556677
        gatt_uuid_t [2] = 0x8899aabb
        gatt_uuid_t [3] = 0xccddeeff

    Wherever the 4 element gatt_uuid_t parameter is used, there is also a
    parameter of the #gatt_uuid_type_t enum type which is used to explicitly 
    declare which UUID type is stored in the array.
*/
typedef uint32 gatt_uuid_t;

#define GATT_UUID_IS_16BIT(uuid) (((uuid) & 0xFFFF0000lu) == 0)


/*! \brief Minimum GATT handle number. */
#define GATT_HANDLE_MIN         0x0001
/*! \brief Maximum GATT handle number. */
#define GATT_HANDLE_MAX         0xffff


/*****************************************************************************
Public API Definition
*****************************************************************************/
#define GATT_FLAGS_DEFAULT  (0)
#define GATT_FLAGS_CONST_DB (1 << 0)

/*!
    \brief This constant depicts the Channel Identifier for accessing data 
    from the local device's GATT database.
*/
#define GATT_CID_LOCAL (0)

/*!
    \name GATT Invalid CID
*/
#define INVALID_CID (0xFFFF)

#endif  /* GATT_H_ */

/** @} */
