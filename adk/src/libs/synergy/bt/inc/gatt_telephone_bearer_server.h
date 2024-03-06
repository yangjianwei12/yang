/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_TELEPHONE_BEARER_SERVER_H_
#define GATT_TELEPHONE_BEARER_SERVER_H_

#include <service_handle.h>
#include "csr_bt_gatt_prim.h"
#include "csr_bt_tasks.h"

#define GTBS_SERVICE_PRIM (SYNERGY_EVENT_BASE  + TBS_SERVER_PRIM )

/* Size of the characteristic sizes (number of octets)*/
#define PROVIDER_NAME_SIZE  (64)
#define BEARER_UCI_SIZE            (6)
#define BEARER_URI_PREFIXES_SIZE   (16)
#define BEARER_TECHNOLOGY_SIZE   (1)
#define CONTENT_CONTROL_ID_SIZE (1)
#define SUPPORTED_FEATURES_SIZE (2)
#define CALL_STATE_SIZE (2) /* Of type tbs_call_state_chracteristic */
#define CCP_OPCODES_SIZE (2)
#define SIGNAL_STRENGTH_SIZE (1)
#define SIGNAL_STRENGTH_INTERVAL_SIZE (1)
#define TERMINATION_REASON_SIZE (2)
#define BEARER_CALL_LIST_ELEMENT_SIZE (4) /* Size plus the call URI string */
#define MAX_CALL_URI_SIZE (32) /* Maximum size of Call URI */
#define MAX_CALL_FRIENDLY_NAME_SIZE (32) /* Maximum size of Call Friendly Name */
#define TBS_MAX_CALL_ID (255)

#define TBS_CURRENT_CALLS_LIST_SIZE (8) /* of type tbs_current_call_list_chracteristic */

#define TBS_MAX_CONNECTIONS (3)

#define TBS_MAX_CALL_ID (255)

/*! To save space client Client configs are stored as 2 bits only, this macro
 * ensures the client config only takes 2 bits */
#define TBS_CCC_MASK(ccc) (ccc & (0x0B))


/*! @brief TBS Feature and Status bits
           2 octet bitfield defines features supported by the server
           Set the bit to enable, clear to disable
 */
/* Status Flags */
#define TBS_STATUS_FLAGS_INBAND_RINGTONE    (1<<0)
#define TBS_STATUS_FLAGS_SILENT_MODE        (1<<1)
#define TBS_STATUS_FLAGS_ALL (TBS_STATUS_FLAGS_INBAND_RINGTONE|TBS_STATUS_FLAGS_SILENT_MODE)

/* Call Control Point Optional Opcodes */
#define TBS_CCP_OPTIONAL_LOCAL_HOLD         (1<<0)
#define TBS_CCP_OPTIONAL_JOIN               (1<<1)
#define TBS_CCP_OPTIONAL_ALL (TBS_CCP_OPTIONAL_LOCAL_HOLD|TBS_CCP_OPTIONAL_JOIN)

/* Call Flags */
#define TBS_CALL_FLAGS_INCOMING_OUTGOING       (1<<0) /* 1 = incoming, 0 = outgoing */
#define TBS_CALL_FLAGS_INFO_WITHHELD_SERVER    (1<<1) /* 1 = withheld, 0 = not withheld */
#define TBS_CALL_FLAGS_INFO_WITHHELD_NETWORK   (1<<2) /* 1 = withheld, 0 = not withheld */
 /* All bits not defined are reserved for future use */
#define TBS_CALL_FLAGS_RFU_MASK (TBS_CALL_FLAGS_INCOMING_OUTGOING |  \
                                 TBS_CALL_FLAGS_INFO_WITHHELD_SERVER |  \
                                 TBS_CALL_FLAGS_INFO_WITHHELD_NETWORK)


/* Signal Strength */
#define TBS_SIGNAL_STRENGTH_MAX          (100)
#define TBS_SIGNAL_STRENGTH_UNAVAILABLE  (255)

/*! @brief Enumeration of call states
 */
typedef uint8 GattTbsCallStates;
#define TBS_CALL_STATE_INCOMING                  ((GattTbsCallStates)0x00)
#define TBS_CALL_STATE_DIALING                   ((GattTbsCallStates)0x01)
#define TBS_CALL_STATE_ALERTING                  ((GattTbsCallStates)0x02)
#define TBS_CALL_STATE_ACTIVE                    ((GattTbsCallStates)0x03)
#define TBS_CALL_STATE_LOCALLY_HELD              ((GattTbsCallStates)0x04)
#define TBS_CALL_STATE_REMOTELY_HELD             ((GattTbsCallStates)0x05)
#define TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD ((GattTbsCallStates)0x06)
/* Internal Error or Transitional states (not defined in TBS Spec.) */
#define TBS_CALL_STATE_TERMINATING               ((GattTbsCallStates)0xFE)
#define TBS_CALL_STATE_INVALID                   ((GattTbsCallStates)0xFF)


/*! @brief Enumeration of call flags
 * Relates to bits within the Call Flag field
 */
typedef uint8 GattTbsCallFlags;
#define TBS_CALL_FLAG_CALL_DIRECTION_INCOMING     ((GattTbsCallFlags)0x00) /* Bit 0:  0 = incoming call, 1 = outgoing call */
#define TBS_CALL_FLAG_CALL_DIRECTION_OUTGOING     ((GattTbsCallFlags)0x01) /* Bit 0:  0 = incoming call, 1 = outgoing call */
#define TBS_CALL_FLAG_INFO_WITHHELD_BY_SERVER     ((GattTbsCallFlags)0x02) /* Bit 1:  0 = not withheld, 1 = withheld */
#define TBS_CALL_FLAG_INFO_WITHELD_BY_NETWORK     ((GattTbsCallFlags)0x04) /* Bit 2:  0 = not withheld, 1 = withheld */


/*! @brief Call Control Point Opcodes
 */
typedef uint8 GattTbsCallControlPointOpcode;
#define TBS_OPCODE_ACCEPT           ((GattTbsCallControlPointOpcode)0x00)
#define TBS_OPCODE_TERMINATE        ((GattTbsCallControlPointOpcode)0x01)
#define TBS_OPCODE_LOCAL_HOLD       ((GattTbsCallControlPointOpcode)0x02)
#define TBS_OPCODE_LOCAL_RETRIEVE   ((GattTbsCallControlPointOpcode)0x03)
#define TBS_OPCODE_ORIGINATE        ((GattTbsCallControlPointOpcode)0x04)
#define TBS_OPCODE_JOIN             ((GattTbsCallControlPointOpcode)0x05)
#define TBS_OPCODE_RFU              ((GattTbsCallControlPointOpcode)0x06) /* Any opcodes equal or greater than this are RFU */


/*! @brief Enumeration of call termination reasons
 */
typedef uint8 GattTbsCallTerminationReason;
#define TBS_CALL_TERMINATION_INVALID_CALL_URI   ((GattTbsCallTerminationReason)0x00)
#define TBS_CALL_TERMINATION_CALL_FAIL          ((GattTbsCallTerminationReason)0x01)
#define TBS_CALL_TERMINATION_REMOTE_ENDED_CALL  ((GattTbsCallTerminationReason)0x02)
#define TBS_CALL_TERMINATION_SERVER_ENDED_CALL  ((GattTbsCallTerminationReason)0x03)
#define TBS_CALL_TERMINATION_LINE_BUSY          ((GattTbsCallTerminationReason)0x04)
#define TBS_CALL_TERMINATION_NETWORK_CONGESTED  ((GattTbsCallTerminationReason)0x05)
#define TBS_CALL_TERMINATION_CLIENT_TERMINATED  ((GattTbsCallTerminationReason)0x06)
#define TBS_CALL_TERMINATION_NO_SERVICE         ((GattTbsCallTerminationReason)0x07)
#define TBS_CALL_TERMINATION_NO_ANSWER          ((GattTbsCallTerminationReason)0x08)
#define TBS_CALL_TERMINATION_UNSPECIFIED        ((GattTbsCallTerminationReason)0x09)

/*! @brief Call Control Point Notification Result Codes
 */
typedef uint8 GattTbsCcpNotificationResultCodes;
#define TBS_CCP_RESULT_SUCCESS                      ((GattTbsCcpNotificationResultCodes)0x00)
#define TBS_CCP_RESULT_OPCODE_NOT_SUPPORTED         ((GattTbsCcpNotificationResultCodes)0x01)
#define TBS_CCP_RESULT_OPERATION_NOT_POSSIBLE       ((GattTbsCcpNotificationResultCodes)0x02)
#define TBS_CCP_RESULT_INVALID_CALL_INDEX           ((GattTbsCcpNotificationResultCodes)0x03)
#define TBS_CCP_RESULT_STATE_MISMATCH               ((GattTbsCcpNotificationResultCodes)0x04)
#define TBS_CCP_RESULT_LACK_OF_RESOURCES            ((GattTbsCcpNotificationResultCodes)0x05)
#define TBS_CCP_RESULT_INVALID_OUTGOING_URI         ((GattTbsCcpNotificationResultCodes)0x06)


/*! @brief Bearer Technology Types, value range <0x00-0x09>
 */
typedef uint16 GattTbsTechnology;
#define TBS_TECHNOLOGY_UNKNOWN ((GattTbsTechnology)0x00)
#define TBS_TECHNOLOGY_3G      ((GattTbsTechnology)0x01)
#define TBS_TECHNOLOGY_4G      ((GattTbsTechnology)0x02)
#define TBS_TECHNOLOGY_LTE     ((GattTbsTechnology)0x03)
#define TBS_TECHNOLOGY_WIFI    ((GattTbsTechnology)0x04)
#define TBS_TECHNOLOGY_5G      ((GattTbsTechnology)0x05)
#define TBS_TECHNOLOGY_gGSM    ((GattTbsTechnology)0x06)
#define TBS_TECHNOLOGY_CDMA    ((GattTbsTechnology)0x07)
#define TBS_TECHNOLOGY_2G      ((GattTbsTechnology)0x08)
#define TBS_TECHNOLOGY_WCDMA   ((GattTbsTechnology)0x09)
/* Bearer technology value "IP" is removed from Bluetooth Assigned numbers, Last Modified: 2022­12­06 */
/* Recomended not to use  TBS_TECHNOLOGY_IP */
#define TBS_TECHNOLOGY_IP      ((GattTbsTechnology)0x0a)

/*! To save space client Client configs are stored as 2 bits only, this macro
 * ensures the client config only takes 2 bits */
#define TBS_CCC_MASK(ccc) (ccc & (0x0B))

/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of the Telephone Bearer Service
 */
typedef struct
{
    unsigned   providerNameClientCfg:2;
    unsigned   bearerTechnologyClientCfg:2;
    unsigned   signalStrengthClientCfg:2;
    unsigned   currentCallsListClientCfg:2;

    unsigned   incomingTargetUriClientCfg:2;
    unsigned   statusFlagsClientCfg:2;
    unsigned   callStateClientCfg:2;
    unsigned   callControlPointClientCfg:2;

    unsigned   terminationReasonClientCfg:2;
    unsigned   callFriendlyNameClientCfg:2;
    unsigned   incomingCallClientCfg:2;
    unsigned   bearerUriPrefixListClientCfg:2;

} TbsClientCfgData;

/*! @brief Client data.

    This structure contains data for each connected client
 */
typedef struct
{
    connection_id_t        cid;
    TbsClientCfgData  clientCfg;
} TbsClientData;

/*! @brief Definition of current call list characteristic element.
 */
typedef struct
{
    uint8           callId;             /* index of the call */
    uint8           callState;          /* call state */
    uint8           callFlags;          /* call flags */
    uint16          callUriLen;         /* Size of remote caller id (callUri) */
    char*           callUri;            /* Remote caller id (variable length) */
    bool            allowJoin;          /* TRUE if this calls bearer allows Join opcode */
} TbsCurrentCallListChracteristic;

/*! @brief Definition of incoming call ID target characteristic element.
 */
typedef struct
{
    uint8           callId;  /* index of the call */
    uint16          uriLen;  /* Length of the URI */
    char*           uri;     /* Incoming caller id target(variable length) */
} TbsIncomingCallTargetUriChracteristic;

/*! @brief Definition of call friendly name characteristic element.
 */
typedef struct
{
    uint8           callId;                 /* index of the call */
    uint16          nameLen;                /* Length of the friendly name */
    char*           friendlyName;           /* call friendly name(variable length) */
} TbsCallFriendlyNameChracteristic;

/*! @brief Definition of incoming call characteristic element.
 */
typedef struct
{
    uint8           callId;  /* index of the call */
    /* Call URI - to be taken from the current Calls List */
} TbsIncomingCallChracteristic;

/*! @brief Definition of call state characteristic element.
 */
typedef struct
{
    uint8                   callId;   /* index of the call */
    GattTbsCallStates       callState;/* Call State */
    uint8                   callFlags;/* Flags of the call */
} TbsCallStateChracteristic;

/*! @brief Definition of call control point characteristic element.
 * The meaning of the parameters is dependant on the opcode
 */
typedef struct
{
    uint8   opcode;   /* opcode */
    uint8   param1[1];/* param1 (variable length) */
} TbsCallControlPointChracteristic;

/*! @brief Definition of call control point notification
 */
typedef struct
{
    uint8                                      opcode;     /* requested opcode */
    uint8                                      callId;     /* index of the call */
    GattTbsCcpNotificationResultCodes          resultCode; /* result */
} TbsCallControlPointNotification;

/*! @brief Definition of call termination reason characteristic element.
 */
typedef struct
{
    uint8                                callId;   /* index of the call */
    GattTbsCallTerminationReason         reason;   /* termination reason */
} TbsTerminationReasonChracteristic;



/*! @brief Messages an application task can receive from the
    Telephone Bearer Server library
 */

typedef uint16 GattTelephoneBearerServerMessageId;

#define GATT_TELEPHONE_BEARER_SERVER_CALL_CONTROL_POINT_IND         ((GattTelephoneBearerServerMessageId)(0x0000u))
#define GATT_TELEPHONE_BEARER_SERVER_SIGNAL_STRENGTH_INTERVAL_IND   ((GattTelephoneBearerServerMessageId)(0x0001u))
#define GATT_TELEPHONE_BEARER_SERVER_MESSAGE_TOP                    ((GattTelephoneBearerServerMessageId)(0x0002u))


/*! @brief Contents of the GattTelephoneBearerServerCallControlPointInd message that is sent by the library,
    due to a change of the Call Control Point attribute by the client.
 */
typedef struct
{
    GattTelephoneBearerServerMessageId  id;
    ServiceHandle                       srvcHndl;
    connection_id_t                     cid;
    uint16                              cpLen;                  /* length of control point data */
    TbsCallControlPointChracteristic    controlPoint;
} GattTelephoneBearerServerCallControlPointInd;


/*! @brief Contents of the GattTelephoneBearerServerSignalStrengthIntervalInd message that is sent by the library,
    due to a change of the signal strength reporting interval attribute by the client.
 */
typedef struct
{
    GattTelephoneBearerServerMessageId  id;
    ServiceHandle                       srvcHndl;
    connection_id_t                     cid;
    uint16                              interval;
} GattTelephoneBearerServerSignalStrengthIntervalInd;


/*! @brief Definition of data required for the initialisation
 *         of the Telephone Bearer Server Library.
 */
typedef struct
{
    char*                                 providerName;
    uint16                                providerNameLen;
    char*                                 uci;
    uint16                                uciLen;
    uint8                                 technology;
    char*                                 uriPrefixesList;
    uint16                                uriPrefixesLen;
    uint8                                 signalStrength;
    uint8                                 signalStrengthNotified;
    uint8                                 signalStrengthReportingInterval;
    uint8                                 contentControlId;
    uint16                                statusFlags;
    uint16                                callControlPointOpcodes;
    TbsCurrentCallListChracteristic       currentCallsList[TBS_CURRENT_CALLS_LIST_SIZE];
    TbsIncomingCallTargetUriChracteristic incomingTargetBearerUri;
    TbsCallFriendlyNameChracteristic      callFriendlyName;
    TbsIncomingCallChracteristic          incomingCall;
    uint8                                 nextCallId;
} GattTbsInitData;


/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of TBS
 */
typedef struct
{
    TbsClientCfgData clientCfgs;
} GattTbsServerConfig;


/*! @brief Initialise the Telephone Bearer Service Library in the Server role.
 *
    @param appTask The Task that will receive the messages from this library.
    @param startHandle First handle in the GATT database for this service.
    @param endHandle Last handle in the GATT database for this service.

    @return handle to the TBS server data.
*/
ServiceHandle GattTelephoneBearerServerInit(
        AppTask    appTask,
        uint16  startHandle,
        uint16  endHandle,
        GattTbsInitData* initData);

/*!
    @brief Add configuration for a paired peer device, identified by its
    Connection ID (CID).

    @param srvcHndl Instance handle for the service.
    @param cid The Connection ID to the peer device.
    @param client_configs Client characteristic configurations for this connection.
           If this is NULL, this indicates a default config should be used for the
           peer device identified by the CID.
    @return status_t status of the Add Configuration operation.
*/
status_t GattTelephoneBearerServerAddConfig(
                  ServiceHandle               srvcHndl,
                  connection_id_t                cid,
                  const GattTbsServerConfig *config);

/*!
    @brief Remove the configuration for a peer device, identified by its
           Connection ID.

    This removes the configuration for that peer device from the
    service library, freeing the resources used for that config.
    This should only be done when the peer device is disconnecting.

    @param srvcHndl The GATT service instance handle.
    @param cid A Connection ID for the peer device.

    @return GattTbsServerConfig Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the connection_id_t is not found, the function will return NULL.
*/
GattTbsServerConfig * GattTelephoneBearerServerRemoveConfig(ServiceHandle srvcHndl,
                                                                      connection_id_t  cid);




/*!
    @brief This API is used to set the provider name when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param providerName Value of provider name to set
    @param len length of provider name to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetProviderName(const ServiceHandle srvcHndl, char* providerName, uint16 len);


/*!
    @brief This API is used when the server application needs to get the call state.

    @param srvcHndl Instance handle for the service.
    @param providerName Value of provider name to set
    @return uint16 length of provider name

    The call to this function will return immediately with a pointer to an array containing
    the provider name value.
    It is expected that the application will free the memory associated with this pointer once it has finished with it.
*/
uint16 GattTelephoneBearerServerGetProviderName(const ServiceHandle srvcHndl, char** providerName);

/*!
    @brief This API is used to set the Uniform Caller Identifier(UCI) when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param uci Value of UCI to set
    @param len length of UCI to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetUci(const ServiceHandle srvcHndl, char* uci, uint16 len);


/*!
    @brief This API is used when the server application needs to get the Uniform Caller Identifier(UCI).

    @param srvcHndl Instance handle for the service.
    @param uci pointer to provider name to set
    @return uint16 length of provider name

    The call to this function will return immediately with a pointer to an array containing
    the UCI value.
    It is expected that the application will free the memory associated with this pointer once it has finished with it.
*/
uint16 GattTelephoneBearerServerGetUci(const ServiceHandle srvcHndl, char** uci);

/*!
    @brief This API is used to set the Technology type when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param technology Value of technology to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetTechnology(const ServiceHandle srvcHndl, GattTbsTechnology technology);


/*!
    @brief This API is used when the server application needs to get the technology type.

    @param srvcHndl Instance handle for the service.

    @return GattTbsTechnology technology type.

    The call to this function will return immediately with the current technology
*/
GattTbsTechnology GattTelephoneBearerServerGetTechnology(const ServiceHandle srvcHndl);


/*!
    @brief This API is used to set the URI prefixes when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param prefixList Value string to set
    @param length length of value string

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetUriPrefixList(const ServiceHandle srvcHndl, char* prefixList, uint16 length);


/*!
    @brief This API is used when the server application needs to get the Uri Prefixes.

    @param srvcHndl Instance handle for the service.
    @param uriList pointer to that will contain URI list
    @return list length.

    The call to this function will return immediately with a pointer to an array containing
    the URI list value.
    It is expected that the application will free the memory associated with this pointer once it has finished with it.
*/
uint16 GattTelephoneBearerServerGetUriPrefix(const ServiceHandle srvcHndl, char** uriList);

/*!
    @brief This API is used to find a URI prefix from within the list of prefixes set
           using GattTelephoneBearerServerSetUriPrefix

    @param srvcHndl Instance handle for the service.
    @param prefix Value string to find

    @return TRUE if successfully found, FALSE otherwise

*/
bool GattTelephoneBearerServerFindUriPrefix(const ServiceHandle srvcHndl, char* prefix);


/*!
    @brief This API is used to set the signal strength when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param signalStrength Value to set

    @return TRUE if successfully set and notifications sent.  A FALSE result indicates either the
    Signal Strength value is out of range or that the value has been set internally but notifications will
    not be sent until the Signal Strength Interval timer period has elapsed.

*/
bool GattTelephoneBearerServerSetSignalStrength(const ServiceHandle srvcHndl, uint8 signalStrength);


/*!
    @brief This API is used when the server application needs to get the signal strength.

    @param srvcHndl Instance handle for the service.

    @return signal strength

    The call to this function will return immediately with the signal strength
*/
uint8 GattTelephoneBearerServerGetSignalStrength(const ServiceHandle srvcHndl);

/*!
    @brief This API is used to set the signal strength reporting interval when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param interval Value to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetSignalStrengthInterval(const ServiceHandle srvcHndl, uint8 interval);


/*!
    @brief This API is used when the server application needs to get the signal strength reporting interval

    @param srvcHndl Instance handle for the service.

    @return signal strength interval

    The call to this function will return immediately with the signal strength reporting interval
*/
uint8 GattTelephoneBearerServerGetSignalStrengthInterval(const ServiceHandle srvcHndl);


/*!
    @brief This API is used when the server application needs to get the raw call list, this will
           include list entries currently invalid.
           Provided for debugging purposes.

    @param srvcHndl Instance handle for the service.

    @return pointer to the call list that will have TBS_CURRENT_CALLS_LIST_SIZE elements.

    The call to this function will return immediately with the call info 
*/
TbsCurrentCallListChracteristic* GattTelephoneBearerServerGetCurrentCalls(const ServiceHandle srvcHndl);

/*!
    @brief This API is used to set the content control id when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param ccid Content control id value to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetContentControlId(const ServiceHandle srvcHndl, uint8 ccid);


/*!
    @brief This API is used when the server application needs to get the content control id

    @param srvcHndl Instance handle for the service.

    @return content control id.

    The call to this function will return immediately with the current content control id
*/
uint8 GattTelephoneBearerServerGetContentControlId(const ServiceHandle srvcHndl);


/*!
    @brief This API is used to set the incoming target URI when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param callId call id value to set, note this should be prefixed with a supported uri (e.g tel:)
    @param incomingUri pointer to the URI string
    @param len length of the URI string

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetIncomingCallTargetUri(const ServiceHandle srvcHndl, uint8 callId, char* incomingUri, uint16 len);


/*!
    @brief This API is used when the server application needs to get the incoming target URI

    @param srvcHndl Instance handle for the service.

    @return TbsIncomingCallTargetUriChracteristic pointer

    The call to this function will return immediately with the incoming target bearer URI.
    The caller is responsible for freeing this pointer and also any pointers contained
    within the TbsIncomingCallTargetUriChracteristic structure.
*/
TbsIncomingCallTargetUriChracteristic* GattTelephoneBearerServerGetIncomingCallTargetUri(const ServiceHandle srvcHndl);


/*!
    @brief This API is used to set the status flags when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param flags feature flags value to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetStatusFlags(const ServiceHandle srvcHndl, uint16 flags);


/*!
    @brief This API is used when the server application needs to get the current status flags

    @param srvcHndl Instance handle for the service.

    @return status flags.

    The call to this function will return immediately with the current status flags
*/
uint16 GattTelephoneBearerServerGetStatusFlags(const ServiceHandle srvcHndl);


/*!
    @brief This API is used to set the create a new call

    @param srvcHndl Instance handle for the service.
    @param callState initial state of the call
    @param callFlags call status flags. Any RFU flags will be ignored.
    @param callUriSize length of the caller id string
    @param callUri caller id of the call
    @param targetUriSize length of the target URI string
    @param targetUri target URI of an incoming call call
    @param joinAllowed if FALSE the bearer of this call does not support Join
    @param newCallId if successful contains call identifier of the new call.

    @return GattTbsCcpNotificationResultCodes result code

    Note the Target URI is an optional characteristic, if this is not exposed in the service
    database these params can be NULL.

    After a successful call creation the caller should call the GattTelephoneBearerServerSetCallFriendlyName
    function to set and notify the connected clients of this characteristic.
*/
GattTbsCcpNotificationResultCodes GattTelephoneBearerServerCreateCallEx(const ServiceHandle srvcHndl,
                                            GattTbsCallStates callState,
                                            GattTbsCallFlags callFlags,
                                            uint16 callUriSize,
                                            char* callUri,
                                            uint16 targetUriSize,
                                            char* targetUri,
                                            bool joinAllowed,
                                            uint8* newCallId);

/*!
    @brief This API is used to set the create a new call

    @param srvcHndl Instance handle for the service.
    @param callState initial state of the call
    @param callFlags call status flags. Any RFU flags will be ignored.
    @param callUriSize length of the caller id string
    @param callUri caller id of the call
    @param targetUriSize length of the target URI string
    @param targetUri target URI of an incoming call call
    @param newCallId if successful contains call identifier of the new call.

    @return GattTbsCcpNotificationResultCodes result code

    Note the Target URI is an optional characteristic, if this is not exposed in the service
    database these params can be NULL.

    After a successful call creation the caller should call the GattTelephoneBearerServerSetCallFriendlyName
    function to set and notify the connected clients of this characteristic.

    The new call's join allowed flag is set to TRUE by default.  If the bearer of this call does not support
    Join then GattTelephoneBearerServerSetCallJoin should be called to set this to FALSE.

*/
GattTbsCcpNotificationResultCodes GattTelephoneBearerServerCreateCall(const ServiceHandle srvcHndl,
                                            GattTbsCallStates callState,
                                            GattTbsCallFlags callFlags,
                                            uint16 callUriSize,
                                            char* callUri,
                                            uint16 targetUriSize,
                                            char* targetUri,
                                            uint8* newCallId);

/*!
    @brief This API is used to set the state of an existing call
    Clients will be then notified of any call state changes

    @param srvcHndl Instance handle for the service.
    @param callId call identifier of the call
    @param newCallState new state of the call to set
    @param notify true if the updated call state should be notified to the client

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetCallState(const ServiceHandle srvcHndl,
                                                uint8 callId,
                                                GattTbsCallStates newCallState,
                                                const bool notify);

/*!
    @brief This API is used to notify clients of call state changes

    @param srvcHndl Instance handle for the service.

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerNotifyCallState(const ServiceHandle srvcHndl);

/*!
    @brief This API is used to set the state of existing calls following receipt of
    GattTelephoneBearerServerCallControlPointInd containing a Call Control Point opcode
    that operates on a single call.

    For Join opcode instead call GattTelephoneBearerServerCallControlPointAcceptJoin.

    Any existing calls state will be updated as appropriate. Clients will be then
    notified of any call state changes.

    GattTelephoneBearerServerCallControlPointResponse should still be called to finalise the
    Write to the control point, this allows the application to make any additional call state
    changes as required.

    @param srvcHndl Instance handle for the service.
    @param callId if successful contains call identifier of the call.
    @param opcode opcode that is being accepted
    @param newCallState new state of the call to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerCallControlPointAcceptOpcode(const ServiceHandle srvcHndl,
                                            uint8 callId,
                                            GattTbsCallControlPointOpcode opcode,
                                            GattTbsCallStates newCallState);

/*!
    @brief This API is used to set the state of existing calls following a write to the
    Call Control Point with Join opcode.

    If GattTelephoneBearerServerCallControlPointInd is received with a Join opcode the
    library will already have validated that the call list is valid.

    Any existing calls state will be updated as appropriate. Clients will be then
    notified of any call state changes.

    GattTelephoneBearerServerCallControlPointResponse should still be called to finalise the
    Write to the control point, this allows the application to make any additional call state
    changes as required.

    @param srvcHndl Instance handle for the service
    @param numJoinCalls number of calls in the list to join
    @param joinCallsList pointer to list of calls to join

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerCallControlPointAcceptJoin(const ServiceHandle srvcHndl,
                                                               uint8 numJoinCalls,
                                                               uint8* joinCallsList);

/*!
    @brief This API is used by the client in response to the message GATT_TBS_CALL_CONTROL_POINT_IND

    @param srvcHndl Instance handle for the service.
    @param ccpn pointer to array of TbsCallControlPointNotification, caller must free this

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerCallControlPointResponse(const ServiceHandle srvcHndl, TbsCallControlPointNotification *ccpn);

/*!
    @brief This API is used to set the call control point optional opcodes when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param opcodes opcodes value to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetControlPointOpcodes(const ServiceHandle srvcHndl, uint16 opcodes);

/*!
    @brief This API is used when the server application needs to get the call control point optional opcodes

    @param srvcHndl Instance handle for the service.

    @return opcodes.

    The call to this function will return immediately with the call control point opcodes
*/
uint16 GattTelephoneBearerServerGetCallControlPointOpcodes(const ServiceHandle srvcHndl);

/*!
    @brief This API is used to set the termination reason when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param call id of the call to terminate
    @param reason value to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerTerminateCall(const ServiceHandle srvcHndl, uint8 callId, GattTbsCallTerminationReason reason);



/*!
    @brief This API is used to set the friendly name of an incoming or outgoing call
    when it is changed by the server application.

    @param srvcHndl Instance handle for the service.
    @param callId call id value to set, note this should be prefixed with a supported uri (e.g tel:)
    @param len length of call friendly name, max length defined by MAX_CALL_FRIENDLY_NAME_SIZE
    @param name call friendly name

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetCallFriendlyName(const ServiceHandle srvcHndl, uint8 callId, uint16 len, char* name);


/*!
    @brief This API is used when the server application needs to get the remote friendly name

    @param srvcHndl Instance handle for the service.

    @return pointer to TbsCallFriendlyNameChracteristic

    The call to this function will return immediately with the remote friendly name.
    The caller is responsible for freeing this pointer and also any pointers contained
    within the TbsCallFriendlyNameChracteristic structure.
*/
TbsCallFriendlyNameChracteristic* GattTelephoneBearerServerGetRemoteFriendlyName(const ServiceHandle srvcHndl);


/*!
    @brief This API is used when the server application needs to validate a call id and get the call state

    @param srvcHndl Instance handle for the service.
    @param callId callId to check
    @param state state of the call (if call id is valid)

    @return true if call id valid, else false.
*/
bool GattTelephoneBearerServerGetCallState(const ServiceHandle srvcHndl, uint8 callId, GattTbsCallStates *state);

/*!
    @brief This API is used when the server application needs to validate if an opcode is supported

    @param srvcHndl Instance handle for the service.
    @param opcode to check

    @return GattTbsCcpNotificationResultCodes  will return one of the following:
            TBS_CCP_RESULT_SUCCESS                  if opcode is supported
            TBS_CCP_RESULT_OPCODE_NOT_SUPPORTED     if opcode not supported
*/
GattTbsCcpNotificationResultCodes GattTelephoneBearerServerCheckOpcodeSupport(const ServiceHandle srvcHndl, uint8 opcode);


/*!
    @brief This API is used when the server application needs to validate if an opcode is valid in the current state

    @param srvcHndl Instance handle for the service.
    @param callId callId to check
    @param opcode to check

    @return GattTbsCallStates -  will return the state the call would transition to.  Will be TBS_CALL_STATE_INVALID
    if the opcode is not allowed in the current state.
*/
GattTbsCallStates GattTelephoneBearerServerCheckOpcodeState(const ServiceHandle srvcHndl, uint8 callId, uint8 opcode);

/*!
    @brief This API is used to set the state of the join allowed flag associated with the given call ID.
           If the call bearer associated with the call ID does not support Join this flag should be set to false
           regardless of the setting of the servers Join optional opcode setting.

    @param srvcHndl Instance handle for the service.
    @param callId call identifier of the call
    @param joinAllowed if false then attempting the Join opcode with this call ID will fail regardless
           of the server supporting the Join optional opcode.

    @return true if call id valid and join allowed set succesfully, else false.

*/
bool GattTelephoneBearerServerSetCallJoin(const ServiceHandle srvcHndl,
                                                uint8 callId,
                                                const bool joinAllowed);


/*!
    @brief This API is used when the server application needs to retrieve the join allowed flag
           of a given call id

    @param srvcHndl Instance handle for the service.
    @param callId callId to check
    @param joinAllowed flag associated with the call ID

    @return true if call id valid, else false.
*/
bool GattTelephoneBearerServerGetCallJoin(const ServiceHandle srvcHndl, uint8 callId, bool *joinAllowed);


#endif /* GATT_TELEPHONE_BEARER_SERVER_H_ */
