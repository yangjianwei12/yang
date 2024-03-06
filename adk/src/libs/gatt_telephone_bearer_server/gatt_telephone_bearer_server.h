/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */ /*  */
/*!
@file    gatt_telephone_bearer_server.h
@brief   Header file for the GATT Telephone Bearer Server library.

        This file provides documentation for the GATT Telephone Bearer Server library
        API (library name: gatt_telephone_bearer_server).
*/

#ifndef GATT_TELEPHONE_BEARER_SERVER_H_
#define GATT_TELEPHONE_BEARER_SERVER_H_

#include <gatt.h>
#include <library.h>
#include <service_handle.h>

/* Size of the characteristic sizes (number of octects)*/
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
#define MAX_CALL_URI_SIZE (32) /* Maximum size of Call URI we will store */
#define TBS_MAX_CALL_ID (255)

#define TBS_CURRENT_CALLS_LIST_SIZE (8) /* of type tbs_current_call_list_chracteristic */
#define TBS_MAX_CONNECTIONS (4)

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
#define TBS_CALL_FLAGS_INCOMING_OUTGOING    (1<<0) /* 1 = incoming, 0 = outgoing */
/* All bits not defined are reserved for future use */

/* Signal Strength */
#define TBS_SIGNAL_STRENGTH_MAX          (100)
#define TBS_SIGNAL_STRENGTH_UNAVAILABLE  (255)

/*! @brief Enumeration of call states
 */
typedef uint8 GattTbsCallStates;
#define TBS_CALL_STATE_INCOMING                  (0x00)
#define TBS_CALL_STATE_DIALLING                  (0x01)
#define TBS_CALL_STATE_ALERTING                  (0x02)
#define TBS_CALL_STATE_ACTIVE                    (0x03)
#define TBS_CALL_STATE_LOCALLY_HELD              (0x04)
#define TBS_CALL_STATE_REMOTELY_HELD             (0x05)
#define TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD (0x06)
#define TBS_CALL_STATE_INVALID                   (0xFF)


/*! @brief Enumeration of call flags
 * Relates to bits within the Call Flag field
 */
typedef uint8 GattTbsCallFlags;
#define TBS_CALL_FLAG_CALL_DIRECTION             (0x00) /* 0 = incoming call, 1 = outgoing call */
#define TBS_CALL_FLAG_INFO_WITHHELD_BY_SERVER    (0x01) /* 0 = not withheld, 1 = withheld */
#define TBS_CALL_FLAG_INFO_WITHELD_BY_NETWORK    (0x02) /* 0 = not withheld, 1 = withheld */


/*! @brief Call Control Point Opcodes
 */
typedef uint8 GattTbsCallControlPointOpcode;
#define TBS_OPCODE_ACCEPT           (0x00)
#define TBS_OPCODE_TERMINATE        (0x01)
#define TBS_OPCODE_LOCAL_HOLD       (0x02)
#define TBS_OPCODE_LOCAL_RETREIVE   (0x03)
#define TBS_OPCODE_ORIGINATE        (0x04)
#define TBS_OPCODE_JOIN             (0x05)


/*! @brief Enumeration of call termination reasons
 */
typedef uint8 GattTbsCallTerminationReason;
#define TBS_CALL_TERMINATION_INVALID_CALL_URI   (0x00)
#define TBS_CALL_TERMINATION_CALL_FAIL          (0x01)
#define TBS_CALL_TERMINATION_REMOTE_ENDED_CALL  (0x02)
#define TBS_CALL_TERMINATION_SERVER_ENDED_CALL  (0x03)
#define TBS_CALL_TERMINATION_LINE_BUSY          (0x04)
#define TBS_CALL_TERMINATION_NETWORK_CONGESTED  (0x05)
#define TBS_CALL_TERMINATION_CLIENT_TERMINATED  (0x06)
#define TBS_CALL_TERMINATION_UNSPECIFIED        (0x07)

/*! @brief Call Control Point Notification Result Codes
 */
typedef uint8 GattTbsCcpNotificationResultCodes;
#define TBS_CCP_RESULT_SUCCESS                      (0x00)
#define TBS_CCP_RESULT_OPCODE_NOT_SUPPORTED         (0x01)
#define TBS_CCP_RESULT_OPERATION_NOT_POSSIBLE       (0x02)
#define TBS_CCP_RESULT_INVALID_CALL_INDEX           (0x03)
#define TBS_CCP_RESULT_STATE_MISMATCH               (0x04)
#define TBS_CCP_RESULT_LACK_OF_RESOURCES            (0x05)
#define TBS_CCP_RESULT_INVALID_OUTGOING_URI         (0x06)


/*! @brief Bearer Technology Types
 */
typedef uint16 GattTbsTechnology;
#define TBS_TECHNOLOGY_UNKNOWN (0x00)
#define TBS_TECHNOLOGY_3G      (0x01)
#define TBS_TECHNOLOGY_4G      (0x02)
#define TBS_TECHNOLOGY_LTE     (0x03)
#define TBS_TECHNOLOGY_WIFI    (0x04)
#define TBS_TECHNOLOGY_5G      (0x05)
#define TBS_TECHNOLOGY_gGSM    (0x06)
#define TBS_TECHNOLOGY_CDMA    (0x07)
#define TBS_TECHNOLOGY_2G      (0x08)
#define TBS_TECHNOLOGY_WCDMA   (0x09)
#define TBS_TECHNOLOGY_IP      (0x0a)

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
    unsigned   unused:2;

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
} tbsCurrentCallListChracteristic;

/*! @brief Definition of incoming call ID target characteristic element.
 */
typedef struct
{
    uint8           callId;  /* index of the call */
    uint16          uriLen;  /* Length of the URI */
    char*           uri;     /* Incoming caller id target(variable length) */
} tbsIncomingCallTargetUriChracteristic;

/*! @brief Definition of call friendly name characteristic element.
 */
typedef struct
{
    uint8           callId;                 /* index of the call */
    uint16          nameLen;                /* Length of the friendly name */
    char*           friendlyName;           /* call friendly name(variable length) */
} tbsCallFriendlyNameChracteristic;

/*! @brief Definition of incoming call characteristic element.
 */
typedef struct
{
    uint8           callId;  /* index of the call */
} tbsIncomingCallChracteristic;

/*! @brief Definition of call state characteristic element.
 */
typedef struct
{
    uint8                   callId;   /* index of the call */
    GattTbsCallStates       callState;/* Call State */
    uint8                   callFlags;/* Flags of the call */
} tbsCallStateChracteristic;

/*! @brief Definition of call control point characteristic element.
 * The meaning of the parameters is dependant on the opcode
 */
typedef struct
{
    uint8   opcode;   /* opcode */
    uint8   param1[1];/* param1 (variable length) */
} tbsCallControlPointChracteristic;

/*! @brief Definition of call control point notification
 */
typedef struct
{
    uint8                                      opcode;     /* requested opcode */
    uint8                                      callId;     /* index of the call */
    GattTbsCcpNotificationResultCodes          resultCode; /* result */
} tbsCallControlPointNotification;

/*! @brief Definition of call termination reason characteristic element.
 */
typedef struct
{
    uint8                                callId;   /* index of the call */
    GattTbsCallTerminationReason         reason;   /* termination reason */
} tbsTerminationReasonChracteristic;


/*! @brief Contents of the GattTbsCallControlPointInd message that is sent by the library,
    due to a change of the Call Control Point attribute by the client.
 */
typedef struct __GATT_TBS_CALL_CONTROL_POINT_IND_T
{
    ServiceHandle srvcHndl;
    uint16      cid;
    uint16      cpLen;                  /* length of control point data */
    tbsCallControlPointChracteristic controlPoint;
} GattTbsCallControlPointInd_T;


/*! @brief Contents of the GattTbsBearerSignalStrengthIntervalInd message that is sent by the library,
    due to a change of the signal strength reporting interval attribute by the client.
 */
typedef struct
{
    ServiceHandle srvcHndl;
    uint16      cid;
    uint16      interval;
} GattTbsBearerSignalStrengthIntervalInd_T;


/*! @brief Enumeration of messages an application task can receive from the
    Telephone Bearer Server library
 */
typedef enum {
    GattTbsCallControlPointInd = GATT_TELEPHONE_BEARER_SERVER_MESSAGE_BASE,
    GattTbsBearerSignalStrengthIntervalInd,
    GattTbsMessageTop
} GattTbsServerMessageId;


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
    tbsCurrentCallListChracteristic       currentCallsList[TBS_CURRENT_CALLS_LIST_SIZE];
    tbsIncomingCallTargetUriChracteristic incomingTargetBearerUri;
    tbsCallFriendlyNameChracteristic      callFriendlyName;
    tbsIncomingCallChracteristic          incomingCall;
    uint8                                 nextCallId;
} gattTbsInitData;


/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of TBS
 */
typedef struct
{
    TbsClientCfgData clientCfgs;
} gattTbsServerConfig;


/*! @brief Initialise the Telephone Bearer Service Library in the Server role.
 *
    @param appTask The Task that will receive the messages from this library.
    @param startHandle First handle in the GATT database for this service.
    @param endHandle Last handle in the GATT database for this service.

    @return handle to the TBS server data.
*/
ServiceHandle GattTelephoneBearerServerInit(
        Task    appTask,
        uint16  startHandle,
        uint16  endHandle,
        gattTbsInitData* initData);

/*!
    @brief Add configuration for a paired peer device, identified by its
    Connection ID (CID).

    @param srvcHndl Instance handle for the service.
    @param cid The Connection ID to the peer device.
    @param client_configs Client characteristic configurations for this connection.
           If this is NULL, this indicates a default config should be used for the
           peer device identified by the CID.
    @return gatt_status_t status of the Add Configuration operation.
*/
gatt_status_t GattTelephoneBearerServerAddConfig(
                  ServiceHandle               srvcHndl,
                  connection_id_t                cid,
                  const gattTbsServerConfig *config);

/*!
    @brief Remove the configuration for a peer device, identified by its
           Connection ID.

    This removes the configuration for that peer device from the
    service library, freeing the resources used for that config.
    This should only be done when the peer device is disconnecting.

    @param srvcHndl The GATT service instance handle.
    @param cid A Connection ID for the peer device.

    @return gattTbsServerConfig Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the connection_id_t is not found, the function will return NULL.
*/
gattTbsServerConfig * GattTelephoneBearerServerRemoveConfig(ServiceHandle srvcHndl,
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

    @return TRUE if successful, FALSE otherwise

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
    @brief This API is used when the server application needs to get the call info.

    @param srvcHndl Instance handle for the service.

    @return pointer to the call list.

    The call to this function will return immediately with the call info 
*/
tbsCurrentCallListChracteristic* GattTelephoneBearerServerGetCurrentCalls(const ServiceHandle srvcHndl);

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

    @return tbsIncomingCallTargetUriChracteristic pointer

    The call to this function will return immediately with the incoming target bearer URI.
    The caller is responsible for freeing this pointer and also any pointers contained
    within the tbsIncomingCallTargetUriChracteristic structure.
*/
tbsIncomingCallTargetUriChracteristic* GattTelephoneBearerServerGetIncomingCallTargetUri(const ServiceHandle srvcHndl);


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
    @param callFlags call status flags
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
    @brief This API is used to set the state of an existing calls following a write to the
    Call Control Point opcode.

    Any exisiting calls state will be updated as appropriate. Clients will be then
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
    @brief This API is used by the client in response to the message GATT_TBS_CALL_CONTROL_POINT_IND

    @param srvcHndl Instance handle for the service.
    @param ccpn pointer to array of tbsCallControlPointNotification, caller must free this

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerCallControlPointResponse(const ServiceHandle srvcHndl, tbsCallControlPointNotification *ccpn);

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
    @param len length of call friendly name
    @param name call friendly name

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerServerSetCallFriendlyName(const ServiceHandle srvcHndl, uint8 callId, uint16 len, char* name);


/*!
    @brief This API is used when the server application needs to get the remote friendly name

    @param srvcHndl Instance handle for the service.

    @return pointer to tbsCallFriendlyNameChracteristic

    The call to this function will return immediately with the remote friendly name.
    The caller is responsible for freeing this pointer and also any pointers contained
    within the tbsCallFriendlyNameChracteristic structure.
*/
tbsCallFriendlyNameChracteristic* GattTelephoneBearerServerGetRemoteFriendlyName(const ServiceHandle srvcHndl);


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
            TBS_CCP_RESULT_OPERATION_NOT_POSSIBLE   if join opcode not supported
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

#endif /* GATT_TELEPHONE_BEARER_SERVER_H_ */
