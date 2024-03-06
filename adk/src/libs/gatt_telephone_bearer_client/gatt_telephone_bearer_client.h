/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file    gatt_telephone_bearer_client.h
@brief   Header file for the GATT Telephone Bearer client library.

        Telephone Bearer Service and Generic Telephone Bearer Service Clients are supported.

        This file provides documentation for the GATT Telephone Bearer client library
        API (library name: gatt_telephone_bearer_client).
*/

#ifndef GATT_TBS_CLIENT_H_
#define GATT_TBS_CLIENT_H_

#include <csrtypes.h>
#include <message.h>

#include <library.h>
#include <gatt.h>
#include <service_handle.h>

/*!
    @brief Call Control Point Opcodes
    These are used in writes to the Call Control Point and as Call Control Point
    indications.
*/
typedef uint8 GattTbsOpcode;
#define TBS_CCP_ACCEPT          (0x00U)
#define TBS_CCP_TERMINATE       (0x01U)
#define TBS_CCP_LOCAL_HOLD      (0x02U)
#define TBS_CCP_LOCAL_RETRIEVE  (0x03U)
#define TBS_CCP_ORIGINATE       (0x04U)
#define TBS_CCP_JOIN            (0x05U)
/* All further opcodes RFU */

/*! @brief Call states
 *
 *  Used in Call State Indications
 */
typedef uint8 GattTbsCallStates;
#define TBS_CALL_STATE_INCOMING                     (0x00U)
#define TBS_CALL_STATE_DIALLING                     (0x01U)
#define TBS_CALL_STATE_ALERTING                     (0x02U)
#define TBS_CALL_STATE_ACTIVE                       (0x03U)
#define TBS_CALL_STATE_LOCALLY_HELD                 (0x04U)
#define TBS_CALL_STATE_REMOTELY_HELD                (0x05U)
#define TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD    (0x06U)
#define TBS_CALL_STATE_INVALID                      (0xFFU)

/*! @brief Call flags bitfield
 *
 * Used in Call Flags as used in call state indications
 */
typedef uint8 GattTbsCallFlags;
#define TBS_CALL_FLAG_INCOMING_CALL                 (0x00U) /*1 incoming, 0 outgoing */


/*! @brief Status flags bitfield
 *
 * Used in Status Flags as used in status indications
 */
typedef uint8 GattTbsStatusFlags;
#define TBS_CALL_FLAG_INBAND_RING                   (0x00U)
#define TBS_CALL_FLAG_SERVER_SILENT                 (0x01U)

/*!
    @brief Status code returned from the GATT Telephone Bearer client library

    This status code indicates the outcome of a request.
*/
typedef uint16 GattTelephoneBearerClientStatus;
/*! { */
/*! Values of the GATT TBS Client status code. */
#define GATT_TBS_CLIENT_STATUS_SUCCESS                   (0x0000U) /*!> Request was a success*/
#define GATT_TBS_CLIENT_STATUS_INVALID_PARAMETER         (0x0001U) /*!> Invalid parameter was supplied*/
#define GATT_TBS_CLIENT_STATUS_DISCOVERY_ERR             (0x0002U) /*!> Error in discovery of Characteristics*/
#define GATT_TBS_CLIENT_STATUS_FAILED                    (0x0003U) /*!> Request has failed*/
#define GATT_TBS_CLIENT_STATUS_INSUFFICIENT_RESOURCES    (0x0004U) /*!> Insufficient Resources to complete
                                                                        the request. */
/*! } */

/*!
    @brief Persistent data for each known telephone bearer server device.

    Each Telephone Bearer server device that is bonded can have data associated against
    it so that re-connections are much faster in that no GATT discovery is 
    required.
*/
typedef struct
{
    uint16 startHandle;
    uint16 endHandle;

    uint16  signal_strength_handle;
    /*TODO*/
} GattTelephoneBearerClientDeviceData;


/*!
    @brief Contents of the GATT_TBS_CLIENT_INIT_CFM message that is sent by the library,
    as a response to the initialisation request.
 */
typedef struct __GATT_TBS_CLIENT_INIT_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus  status;
} GATT_TBS_CLIENT_INIT_CFM_T;

/* Read CFM Messages */

/*! @brief Contents of the GATT_TBS_CLIENT_READ_PROVIDER_NAME_CFM message that is sent by the library,
    as response to a read of the Bearer Provider Name.
 */
typedef struct __GATT_TBS_CLIENT_READ_PROVIDER_NAME_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 providerNameSize;
    char  providerName[1];
} GATT_TBS_CLIENT_READ_PROVIDER_NAME_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_READ_BEARER_UCI_CFM message that is sent by the library,
    as response to a read of the Bearer UCI.
 */
typedef struct __GATT_TBS_CLIENT_READ_BEARER_UCI_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 bearerUciSize;
    char  bearerUci[1];
} GATT_TBS_CLIENT_READ_BEARER_UCI_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_READ_BEARER_TECHNOLOGY_CFM message that is sent by the library,
    as response to a read of the Bearer Technology.
 */
typedef struct __GATT_TBS_CLIENT_READ_BEARER_TECHNOLOGY_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 bearerTechSize;
    char  bearerTech[1];
} GATT_TBS_CLIENT_READ_BEARER_TECHNOLOGY_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM message that is sent by the library,
    as response to a read of the Bearer URI Schemes Supported List.
 */
typedef struct __GATT_TBS_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 uriListSize;
    char uriList[1];
} GATT_TBS_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_CFM message that is sent by the library,
    as response to a read of the Signal Strength.
 */
typedef struct __GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 signalStrength;
} GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_READ_SINGAL_STRENGTH_INTERVAL_CFM message that is sent by the library,
    as response to a read of the Signal Strength Reporting Interval.
 */
typedef struct __GATT_TBS_CLIENT_READ_SINGAL_STRENGTH_INTERVAL_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 interval; /* in seconds */
} GATT_TBS_CLIENT_READ_SINGAL_STRENGTH_INTERVAL_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_READ_CURRENT_CALLS_LIST_CFM message that is sent by the library,
    as response to a read of the Current Calls List.
 */
typedef struct __GATT_TBS_CLIENT_READ_CURRENT_CALLS_LIST_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 currentCallsListSize;
    uint8 currentCallsList[1];
} GATT_TBS_CLIENT_READ_CURRENT_CALLS_LIST_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_READ_CONTENT_CONTROL_ID_CFM message that is sent by the library,
    as response to a read of the Content Control Id.
 */
typedef struct __GATT_TBS_CLIENT_READ_CONTENT_CONTROL_ID_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint16 contentControlId;
} GATT_TBS_CLIENT_READ_CONTENT_CONTROL_ID_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM message that is sent by the library,
    as response to a read of the Status and Feature Flags.
 */
typedef struct __GATT_TBS_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM
{
    ServiceHandle             tbsHandle;
    GattTelephoneBearerClientStatus status;
    GattTbsStatusFlags           flags;
} GATT_TBS_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM message that is sent by the library,
    as response to a read of the Incoming Call Target Bearer URI.
 */
typedef struct __GATT_TBS_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 uriSize;
    char uri[1];
} GATT_TBS_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM_T;

/*! @brief Definition of call state characteristic element.
 */
typedef struct
{
    uint8                   callId;   /* index of the call */
    GattTbsCallStates       callState;/* Call State */
    GattTbsCallFlags        callFlags;/* Flags of the call */
} TbsCallState;

/*! @brief Contents of the GATT_TBS_CLIENT_MSG_READ_CALL_STATE_CFM message that is sent by the library,
    as response to a read of the Call State.
 */
typedef struct __GATT_TBS_CLIENT_MSG_READ_CALL_STATE_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 callStateListSize;
    TbsCallState callStateList[1];
} GATT_TBS_CLIENT_MSG_READ_CALL_STATE_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_MSG_READ_INCOMING_CALL_CFM message that is sent by the library,
    as response to a read of the Incoming Call.
 */
typedef struct __GATT_TBS_CLIENT_MSG_READ_INCOMING_CALL_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 callId;
    uint8 callUriSize;
    uint8 callUri[1];
} GATT_TBS_CLIENT_MSG_READ_INCOMING_CALL_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_MSG_READ_CALL_FRIENDLY_NAME_CFM message that is sent by the library,
    as response to a read of the Call Friendly Name.
 */
typedef struct __GATT_TBS_CLIENT_MSG_READ_CALL_FRIENDLY_NAME_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 callId;
    uint8 friendlyNameSize;
    char friendlyName[1];
} GATT_TBS_CLIENT_MSG_READ_CALL_FRIENDLY_NAME_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM message that is sent by the library,
    as response to a read of the Call Control Point optional opcodes */
typedef struct __GATT_TBS_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 opcodes;
} GATT_TBS_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM_T;

/* Notification Messages */

/*! @brief Contents of the GATT_TBS_CLIENT_PROVIDER_NAME_IND message that is sent by the library,
    as a result of a notification of the Provider Name.
 */
typedef struct __GATT_TBS_CLIENT_PROVIDER_NAME_IND
{
    ServiceHandle tbsHandle;
    uint8 providerNameSize;
    char  providerName[1];
} GATT_TBS_CLIENT_PROVIDER_NAME_IND_T;

/*! @brief Contents of the GATT_TBS_CLIENT_BEARER_TECHNOLOGY_IND message that is sent by the library,
    as a result of a notification of the Bearer Technology.
 */
typedef struct __GATT_TBS_CLIENT_BEARER_TECHNOLOGY_IND
{
    ServiceHandle tbsHandle;
    uint8 bearerTechSize;
    char  bearerTech[1];
} GATT_TBS_CLIENT_BEARER_TECHNOLOGY_IND_T;

/*! @brief Contents of the GATT_TBS_CLIENT_SIGNAL_STRENGTH_IND message that is sent by the library,
    as a result of a notification of the Signal Strength.
 */
typedef struct __GATT_TBS_CLIENT_SIGNAL_STRENGTH_IND
{
    ServiceHandle tbsHandle;
    uint8 signalStrength;
} GATT_TBS_CLIENT_SIGNAL_STRENGTH_IND_T;

/*! @brief Contents of the GATT_TBS_CLIENT_CURRENT_CALLS_IND message that is sent by the library,
    as a result of a notification of the Current Calls List.
 */
typedef struct __GATT_TBS_CLIENT_CURRENT_CALLS_IND
{
    ServiceHandle tbsHandle;
    uint8 currentCallsListSize;
    uint8 currentCallsList[1];
} GATT_TBS_CLIENT_CURRENT_CALLS_IND_T;

/*! @brief Contents of the GATT_TBS_CLIENT_FLAGS_IND message that is sent by the library,
    as a result of a notification of the Feature and Status Flags.
 */
typedef struct __GATT_TBS_CLIENT_FLAGS_IND
{
    ServiceHandle tbsHandle;
    uint16 flags;
} GATT_TBS_CLIENT_FLAGS_IND_T;

/*! @brief Contents of the GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND message that is sent by the library,
    as a result of a notification of the Incoming Call Target Bearer URI.
 */
typedef struct __GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND
{
    ServiceHandle tbsHandle;
    uint8 uriSize;
    char uri[1];
} GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND_T;

/*! @brief Contents of the GATT_TBS_CLIENT_CALL_STATE_IND message that is sent by the library,
    as a result of a notification of the Call State.
 */
typedef struct __GATT_TBS_CLIENT_CALL_STATE_IND
{
    ServiceHandle tbsHandle;
    uint8 callStateListSize;
    TbsCallState callStateList[1];
} GATT_TBS_CLIENT_CALL_STATE_IND_T;

/*! @brief Contents of the GATT_TBS_CLIENT_CALL_CONTROL_POINT_IND message that is sent by the library,
    as a result of a notification of the Call Control Point.
 */
typedef struct __GATT_TBS_CLIENT_CALL_CONTROL_POINT_IND
{
    ServiceHandle tbsHandle;
    GattTbsOpcode opcode;
    uint8 callId;
    uint8 resultCode;
} GATT_TBS_CLIENT_CALL_CONTROL_POINT_IND_T;

/*! @brief Contents of the GATT_TBS_CLIENT_TERMINATION_REASON_IND message that is sent by the library,
    as a result of a notification of the Termination Reason
 */
typedef struct __GATT_TBS_CLIENT_TERMINATION_REASON_IND
{
    ServiceHandle tbsHandle;
    uint8 callId;
    uint8 reasonCode;
} GATT_TBS_CLIENT_TERMINATION_REASON_IND_T;

/*! @brief Contents of the GATT_TBS_CLIENT_INCOMING_CALL_IND message that is sent by the library,
    as a result of a notification of the Incoming Call.
 */
typedef struct __GATT_TBS_CLIENT_INCOMING_CALL_IND
{
    ServiceHandle tbsHandle;
    uint8 callId;
    uint8 uriSize;
    uint8 uri[1];
} GATT_TBS_CLIENT_INCOMING_CALL_IND_T;

/*! @brief Contents of the GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_IND message that is sent by the library,
    as a result of a notification of the Call Friendly Name indication
 */
typedef struct __GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_IND
{
    ServiceHandle tbsHandle;
    uint8 callId;
    uint8 friendlyNameSize;
    char  friendlyName[1];
} GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_IND_T;



/* Notification Enable Cfm Messages */
/*! @brief Contents of the GATT_TBS_CLIENT_SET_CFM is a template used for characteristic
 *  write and set confirmation messages.
 */
typedef struct __GATT_TBS_CLIENT_SET_CFM
{
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
} GATT_TBS_CLIENT_SET_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_PROVIDER_NAME_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Bearer Provider Name characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_PROVIDER_NAME_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_TECHNOLOGY_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Bearer technology characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_TECHNOLOGY_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Signal Strength characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_CURRENT_CALLS_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Current Calls characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_CURRENT_CALLS_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_FLAGS_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Feature and Status Flags characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_FLAGS_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Incoming Call Target Bearer URI characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_CALL_STATE_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Call State characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_CALL_STATE_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Call Control Point characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_TERMINATION_REASON_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Termination Reason characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_TERMINATION_REASON_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_INCOMING_CALL_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Incoming Call characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_INCOMING_CALL_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_INCOMING_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Incoming Call friendly name characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_INCOMING_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_OUTGOING_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM message that is sent by the library,
    as a result of configuring notifications on the server for the Outgoing Call friendly name characteristic.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_OUTGOING_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM_T;


/* Write CFM Messages */

/*! @brief Contents of the GATT_TBS_CLIENT_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM message that is sent by the library,
    as a result of writing the Signl Strength Interval characteristic on the server.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM_T;

/*! @brief Contents of the GATT_TBS_CLIENT_WRITE_CALL_CONTROL_POINT_CFM message that is sent by the library,
    as a result of writing the Call Control point characteristic on the server.
 */
typedef struct GATT_TBS_CLIENT_SET_CFM_T GATT_TBS_CLIENT_WRITE_CALL_CONTROL_POINT_CFM_T;


/*! @brief Enumeration of messages a client task may receive from the TBS client library.
 */
typedef enum
{
    /* Client messages */
    GATT_TBS_CLIENT_INIT_CFM = GATT_TBS_CLIENT_MESSAGE_BASE,            /* 00 */
    /* Characteristic Read Confirmation messages */
    GATT_TBS_CLIENT_READ_PROVIDER_NAME_CFM,
    GATT_TBS_CLIENT_READ_BEARER_UCI_CFM,
    GATT_TBS_CLIENT_READ_BEARER_TECHNOLOGY_CFM,
    GATT_TBS_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM,
    GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_CFM,
    GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_INTERVAL_CFM,
    GATT_TBS_CLIENT_READ_CURRENT_CALLS_LIST_CFM,
    GATT_TBS_CLIENT_READ_CONTENT_CONTROL_ID_CFM,
    GATT_TBS_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM,
    GATT_TBS_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM,
    GATT_TBS_CLIENT_READ_CALL_STATE_CFM,
    GATT_TBS_CLIENT_READ_INCOMING_CALL_CFM,
    GATT_TBS_CLIENT_READ_CALL_FRIENDLY_NAME_CFM,
    GATT_TBS_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM,

    /* Characteristic Notification Confirmation messages */
    GATT_TBS_CLIENT_SET_CFM,
    GATT_TBS_CLIENT_PROVIDER_NAME_SET_NOTIFICATION_CFM,
    GATT_TBS_CLIENT_TECHNOLOGY_SET_NOTIFICATION_CFM,
    GATT_TBS_CLIENT_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM,
    GATT_TBS_CLIENT_CURRENT_CALLS_SET_NOTIFICATION_CFM,
    GATT_TBS_CLIENT_FLAGS_SET_NOTIFICATION_CFM,
    GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM,
    GATT_TBS_CLIENT_CALL_STATE_SET_NOTIFICATION_CFM,
    GATT_TBS_CLIENT_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM,
    GATT_TBS_CLIENT_TERMINATION_REASON_SET_NOTIFICATION_CFM,
    GATT_TBS_CLIENT_INCOMING_CALL_SET_NOTIFICATION_CFM,
    GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM,

    /* Write Characteristic CFMs */
    GATT_TBS_CLIENT_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM,
    GATT_TBS_CLIENT_WRITE_CALL_CONTROL_POINT_CFM,

    /* Characteristic Indication messages */
    GATT_TBS_CLIENT_PROVIDER_NAME_IND,
    GATT_TBS_CLIENT_BEARER_TECHNOLOGY_IND,
    GATT_TBS_CLIENT_SIGNAL_STRENGTH_IND,
    GATT_TBS_CLIENT_CURRENT_CALLS_IND,
    GATT_TBS_CLIENT_FLAGS_IND,
    GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND,
    GATT_TBS_CLIENT_CALL_STATE_IND,
    GATT_TBS_CLIENT_CALL_CONTROL_POINT_IND,
    GATT_TBS_CLIENT_TERMINATION_REASON_IND,
    GATT_TBS_CLIENT_INCOMING_CALL_IND,
    GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_IND,

    /* Library message limit */
    GATT_TBS_CLIENT_MESSAGE_TOP
} GattTelephoneBearerClientMessageId;


/*!
    @brief After the application has used the GATT manager to establish a connection to a discovered BLE device in the Client role,
    it can discover any supported services in which it has an interest. It should then register with the relevant client service library
    (passing the relevant CID and handles to the service). For the TBS client it will use this API. The GATT manager 
    will then route notifications and indications to the correct instance of the client service library for the CID.

    The registered task will receive a GATT_TBS_CLIENT_INIT_CFM message with gatt_tbs_client_status_t status code and instance handle.

    @param appTask The Task that will receive the messages sent from this TBS client library.
    @param cid The connection ID.
    @param startHandle The start handle of the TBS client instance.
    @param endHandle The end handle of the TBS client instance.
    @param deviceData Persistent data for the device.

    @return void

*/
void GattTelephoneBearerClientInit(
                           Task appTask,
                           uint16 cid,
                           uint16 startHandle,
                           uint16 endHandle,
                           GattTelephoneBearerClientDeviceData * deviceData);

/*!
    @brief When a GATT connection is removed, the application must remove all client service instances that were
    associated with the connection (using the CID value).
    This is the clean up routine as a result of calling the GattTelephoneBearerClientInit API. That is,
    the GattTelephoneBearerClientInit API is called when a connection is made, and the GattTelephoneBearerClientDestroy is called
    when the connection is removed.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerClientDestroy(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Provider Name from a remote device.
           A GATT_TBS_CLIENT_READ_PROVIDER_NAME_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadProviderNameRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Bearer UCI from a remote device.
           A GATT_TBS_CLIENT_READ_BEARER_UCI_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadBearerUciRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Bearer Technology from a remote device.
           A GATT_TBS_CLIENT_READ_BEARER_TECHNOLOGY_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadBearerTechnologyRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Bearer Supported URI Schemes from a remote device.
           A GATT_TBS_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM message will be sent
           to the registered application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadBearerUriRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Signal Strength from a remote device.
           A GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadSignalStrengthRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Signal Strength Reporting Interval from a remote device.
           A GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_INTERVAL_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadSignalStrengthIntervalRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Current Calls from a remote device.
           A GATT_TBS_CLIENT_READ_CURRENT_CALLS_LIST_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadCurrentCallsRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Content Control ID List from a remote device.
           A GATT_TBS_CLIENT_READ_CONTENT_CONTROL_ID_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadContentControlIdRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Feature and Status Flags from a remote device.
           A GATT_TBS_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadStatusAndFeatureFlagsRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read Incoming Call Target Bearer URI from a remote device.
           A GATT_TBS_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM message will be sent
           to the registered application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadIncomingTargetBearerUriRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Call State from a remote device.
           A GATT_TBS_CLIENT_READ_CALL_STATE_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadCallStateRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Incoming Call information from a remote device.
           A GATT_TBS_CLIENT_READ_INCOMING_CALL_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadIncomingCallRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Call Friendly Name from a remote device.
           A GATT_TBS_CLIENT_READ_CALL_FRIENDLY_NAME_CFM message will be sent to
           the registered application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadCallFriendlyNameRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Call Control Point Optional Opcodes from a remote device.
           A GATT_TBS_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM message will be sent to
           the registered application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadCallControlPointOptionalOpcodesRequest(const ServiceHandle tbsHandle);


/*!
    @brief This API is used to read the Outgoing Call Friendly Name from a remote device.
           A GATT_TBS_CLIENT_READ_OUTGOING_CALL_FRIENDLY_NAME_CFM message will be sent to the registered
           application Task with the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadOutgoingCallFriendlyNameRequest(const ServiceHandle tbsHandle);


/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Provider Name notifications with the server.
    A GATT_TBS_CLIENT_PROVIDER_NAME_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetProviderNameNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Bearer Technology notifications with the server.
    A GATT_TBS_CLIENT_TECHNOLOGY_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetTechnologyNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Signal Strength notifications with the server.
    A GATT_TBS_CLIENT_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetSignalStrengthNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Current Calls List notifications with the server.
    A GATT_TBS_CLIENT_CURRENT_CALLS_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetListCurrentCallsNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Status and Feature Flags notifications with the server.
    A GATT_TBS_CLIENT_FLAGS_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetFlagsNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Incoming Call Target Bearer URI notifications with the server.
    A GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetIncomingCallTargetBearerUriNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Call State notifications with the server.
    A GATT_TBS_CLIENT_CALL_STATE_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetCallStateNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Call Control Point notifications with the server.
    A GATT_TBS_CLIENT_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetCallControlPointNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Termination Reason notifications with the server.
    A GATT_TBS_CLIENT_TERMINATION_REASON_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetTerminationReasonNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Incoming Call notifications with the server.
    A GATT_TBS_CLIENT_INCOMING_CALL_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetIncomingCallNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Call Friendly Name notifications with the server.
    A GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param ServiceHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetCallFriendlyNameNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);


/*!
    @brief This API is used to write the Signal Strength reporting interval on the remote device.
    A GATT_TBS_CLIENT_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM message will be sent to the registered application Task
    indicating the result of the write request.

    @param ServiceHandle The client instance that was passed into the GattTelephoneBearerClientInit API.
    @param uint8   Reporting interval to set in seconds.
    @param bool    If true GATT Write Without Response is used, else GATT Write Characteristic Value is used
    @return void
*/
void GattTelephoneBearerClientWriteSignalStrengthIntervalRequest(const ServiceHandle tbsHandle,
                                                                        const uint8 interval,
                                                                        bool writeWithoutResponse);

/*!
    @brief This API is used to write to the Call Control Point the remote device.
    This function only works for simple two octet commands that require only an opcode and call index
    A GATT_TBS_CLIENT_WRITE_CALL_CONTROL_POINT_CFM message will be sent to the registered application Task
    indicating the result of the write request.

    @param ServiceHandle The client instance that was passed into the GattTelephoneBearerClientInit API.
    @param GattTbsOpcode   opcode to use
    @param uint8           call index
    @return void
*/
void GattTelephoneBearerClientWriteCallControlPointSimpleRequest(const ServiceHandle tbsHandle, const GattTbsOpcode opcode, const uint8 callIndex);

/*!
    @brief This API is used to write to the Call Control Point the remote device.
    A GATT_TBS_CLIENT_WRITE_CALL_CONTROL_POINT_CFM message will be sent to the registered application Task
    indicating the result of the write request.

    @param ServiceHandle The client instance that was passed into the GattTelephoneBearerClientInit API.
    @param GattTbsOpcode   opcode to use
    @param uint8   size of opcode parameter
    @param uint8*  opcode parameter
    @return void
*/
void GattTelephoneBearerClientWriteCallControlPointRequest(const ServiceHandle tbsHandle, const GattTbsOpcode opcode, const uint8 size, const uint8* param);



#endif
