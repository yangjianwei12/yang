/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

/*!
@file    gatt_telephone_bearer_client.h
@brief   Header file for the GATT Telephone Bearer client library.

        Telephone Bearer Service and Generic Telephone Bearer Service Clients are supported.

        This file provides documentation for the GATT Telephone Bearer client library
        API (library name: gatt_telephone_bearer_client).
*/

#ifndef GATT_TELEPHONE_BEARER_CLIENT_H_
#define GATT_TELEPHONE_BEARER_CLIENT_H_

#include <service_handle.h>
#include "csr_bt_gatt_prim.h"
#include "csr_bt_tasks.h"

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
#define TBS_CALL_TERMINATION_NO_SERVICE         (0x07)
#define TBS_CALL_TERMINATION_NO_ANSWER          (0x08)
#define TBS_CALL_TERMINATION_UNSPECIFIED        (0x09)

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


/*! @brief Bearer Technology Types, value range <0x00-0x09>
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
/* Bearer technology value "IP" is removed from Bluetooth Assigned numbers, Last Modified: 2022­12­06 */
/* Recomended not to use  TBS_TECHNOLOGY_IP */
#define TBS_TECHNOLOGY_IP      (0x0a)

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
#define GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE                (0x00)

/*!
    @brief Status code returned from the GATT Telephone Bearer client library

    This status code indicates the outcome of a request.
*/
typedef uint16 GattTelephoneBearerClientStatus;
/*! { */
/*! Values of the GATT TBS Client status code. */
#define GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS                   (0x0000U) /*!> Request was a success*/
#define GATT_TELEPHONE_BEARER_CLIENT_STATUS_INVALID_PARAMETER         (0x0001U) /*!> Invalid parameter was supplied*/
#define GATT_TELEPHONE_BEARER_CLIENT_STATUS_DISCOVERY_ERR             (0x0002U) /*!> Error in discovery of Characteristics*/
#define GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED                    (0x0003U) /*!> Request has failed*/
#define GATT_TELEPHONE_BEARER_CLIENT_STATUS_INSUFFICIENT_RESOURCES    (0x0004U) /*!> Insufficient Resources to complete
                                                                        the request. */
#define GATT_TELEPHONE_BEARER_CLIENT_STATUS_READ_ERR                  (0x0005u) /*!> Error in reading of Characteristics*/
/*! } */


/*! @brief Defines of messages a client task may receive from the TBS client library.
 */

typedef uint16 GattTelephoneBearerClientMessageId;

#define GATT_TELEPHONE_BEARER_CLIENT_INIT_CFM                                   (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE)
#define GATT_TELEPHONE_BEARER_CLIENT_TERMINATE_CFM                              (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0001u)
/* Characteristic Read Confirmation messages */
#define GATT_TELEPHONE_BEARER_CLIENT_READ_PROVIDER_NAME_CFM                     (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0002u)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_UCI_CFM                        (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0003u)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_TECHNOLOGY_CFM                 (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0004u)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0005u)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_SIGNAL_STRENGTH_CFM                   (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0006u)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_SIGNAL_STRENGTH_INTERVAL_CFM          (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0007u)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_CURRENT_CALLS_LIST_CFM                (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0008u)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_CONTENT_CONTROL_ID_CFM                (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0009u)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM          (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x000au)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM   (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x000bu)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_CALL_STATE_CFM                        (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x000cu)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_INCOMING_CALL_CFM                     (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x000du)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_CALL_FRIENDLY_NAME_CFM                (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x000eu)
#define GATT_TELEPHONE_BEARER_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM              (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x000fu)

/* Characteristic Notification Confirmation messages */
#define GATT_TELEPHONE_BEARER_CLIENT_SET_CFM                                    (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0010u)
#define GATT_TELEPHONE_BEARER_CLIENT_PROVIDER_NAME_SET_NOTIFICATION_CFM         (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0011u)
#define GATT_TELEPHONE_BEARER_CLIENT_TECHNOLOGY_SET_NOTIFICATION_CFM            (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0012u)
#define GATT_TELEPHONE_BEARER_CLIENT_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM       (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0013u)
#define GATT_TELEPHONE_BEARER_CLIENT_CURRENT_CALLS_SET_NOTIFICATION_CFM         (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0014u)
#define GATT_TELEPHONE_BEARER_CLIENT_FLAGS_SET_NOTIFICATION_CFM                 (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0015u)
#define GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0016u)
#define GATT_TELEPHONE_BEARER_CLIENT_CALL_STATE_SET_NOTIFICATION_CFM            (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0017u)
#define GATT_TELEPHONE_BEARER_CLIENT_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM    (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0018u)
#define GATT_TELEPHONE_BEARER_CLIENT_TERMINATION_REASON_SET_NOTIFICATION_CFM    (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0019u)
#define GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_SET_NOTIFICATION_CFM         (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x001au)
#define GATT_TELEPHONE_BEARER_CLIENT_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM    (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x001bu)

/* Write Characteristic CFMs */
#define GATT_TELEPHONE_BEARER_CLIENT_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM         (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x001cu)
#define GATT_TELEPHONE_BEARER_CLIENT_WRITE_CALL_CONTROL_POINT_CFM               (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x001du)

/* Characteristic Indication messages */
#define GATT_TELEPHONE_BEARER_CLIENT_PROVIDER_NAME_IND                          (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x001eu)
#define GATT_TELEPHONE_BEARER_CLIENT_BEARER_TECHNOLOGY_IND                      (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x001fu)
#define GATT_TELEPHONE_BEARER_CLIENT_SIGNAL_STRENGTH_IND                        (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0020u)
#define GATT_TELEPHONE_BEARER_CLIENT_CURRENT_CALLS_IND                          (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0021u)
#define GATT_TELEPHONE_BEARER_CLIENT_FLAGS_IND                                  (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0022u)
#define GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND        (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0023u)
#define GATT_TELEPHONE_BEARER_CLIENT_CALL_STATE_IND                             (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0024u)
#define GATT_TELEPHONE_BEARER_CLIENT_CALL_CONTROL_POINT_IND                     (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0025u)
#define GATT_TELEPHONE_BEARER_CLIENT_TERMINATION_REASON_IND                     (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0026u)
#define GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_IND                          (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0027u)
#define GATT_TELEPHONE_BEARER_CLIENT_CALL_FRIENDLY_NAME_IND                     (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0028u)
/* Library message limit */
#define GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_TOP                                (GATT_TELEPHONE_BEARER_CLIENT_MESSAGE_BASE+0x0029u)


/*!
    @brief Parameters used by the Initialisation API
*/
typedef struct
{
     connection_id_t cid;       /*! Connection ID. */
     uint16 startHandle;       /*! The first handle of the service that needs to be accessed */
     uint16 endHandle;         /*! The last handle of the service that needs to be accessed */
} GattTelephoneBearerClientInitData;


/*!
    @brief Persistent data for each known telephone bearer server device.

    Each Telephone Bearer server device that is bonded can have data associated against
    it so that re-connections are much faster in that no GATT discovery is 
    required.
*/
typedef struct
{
    uint16  startHandle;
    uint16  endHandle;

    uint16  bearerNameHandle;                       /* Handle of the Bearer Provider Name characteristic value */
    uint16  bearerNameCccHandle;                    /* Handle of the Bearer Provider Name Client characteristic config value */
    uint16  bearerUciHandle;                        /* Handle of the Bearer UCI Characteristic value */
    uint16  bearerTechHandle;                       /* Handle of the Bearer technology characteristic value */
    uint16  bearerTechCccHandle;                    /* Handle of the Bearer technology client characteristic config value */
    uint16  bearerUriPrefixListHandle;              /* Handle of the Bearer URI Supported List Characteristic value */
    uint16  signalStrengthHandle;                   /* Handle of the Signal Strength characteristic */
    uint16  signalStrengthCccHandle;                /* Handle of the Signal Strength Client characteristic config */
    uint16  signalStrengthIntervalHandle;           /* Handle of the Signal Strength Interval characteristic */
    uint16  listCurrentCallsHandle;                 /* Handle of the List Current Calls characteristic */
    uint16  listCurrentCallsCccHandle;              /* Handle of the List Current Calls Client characteristic config */
    uint16  contentControlIdHandle;                 /* Handle of the Content Control ID Characteristic */
    uint16  statusFlagsHandle;                      /* Handle of the Status Flags characteristic */
    uint16  statusFlagsCccHandle;                   /* Handle of the Status Flags Client characteristic config */
    uint16  incomingTargetBearerUriHandle;          /* Handle of the Target Bearer URI Characteristic */
    uint16  incomingTargetBearerUriCccHandle;       /* Handle of the Target Bearer URI Client Characteristic Configuration */
    uint16  callStateHandle;                        /* Handle of the Call State characteristic */
    uint16  callStateCccHandle;                     /* Handle of the Call State Client characteristic config */
    uint16  callControlPointHandle;                 /* Handle of the Call Control Point Characteristic */
    uint16  callControlPointCccHandle;              /* Handle of the Call Control Point Client Characteristic config*/
    uint16  callControlPointOptionalOpcodesHandle;  /* Handle of the CCP optional opcodes */
    uint16  terminationReasonHandle;                /* Handle of the Termination Reason Characteristic */
    uint16  terminationReasonCccHandle;             /* Handle of the Termination Reason client Characteristic config */
    uint16  incomingCallHandle;                     /* Handle of the Incoming call characteristic */
    uint16  incomingCallCccHandle;                  /* Handle of the Incoming call client characteristic config */
    uint16  remoteFriendlyNameHandle;               /* Handle of the call remote friendly name characteristic */
    uint16  remoteFriendlyNameCccHandle;            /* Handle of the call remote friendly name client characteristic config */

} GattTelephoneBearerClientDeviceData;


/*!
    @brief Contents of the GattTelephoneBearerClientInitCfm message that is sent by the library,
    as a response to the initialisation request.
 */
typedef struct __GattTelephoneBearerClientInitCfm
{
    GattTelephoneBearerClientMessageId id;
    connection_id_t  cid;                       /*! Connection ID. */
    ServiceHandle tbsHandle;                    /*! Reference handle for the instance */
    GattTelephoneBearerClientStatus  status;    /*! Status of the initialisation attempt */
} GattTelephoneBearerClientInitCfm;


/*!
    @brief Gatt client library message sent as a result of calling the GattTelephoneBearerClientTerminateReq API.
*/
typedef struct
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle srvcHndl;             /*! Reference handle for the instance */
    GattTelephoneBearerClientStatus status;         /*! Status of the initialisation attempt */
    GattTelephoneBearerClientDeviceData deviceData; /*! Device data: handles used for the peer device. */
} GattTelephoneBearerClientTerminateCfm;


/* Read CFM Messages */

/*! @brief Contents of the GattTelephoneBearerClientReadProviderNameCfm message that is sent by the library,
    as response to a read of the Bearer Provider Name.
 */
typedef struct __GattTelephoneBearerClientReadProviderNameCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint16 providerNameSize;
    char  providerName[1];
} GattTelephoneBearerClientReadProviderNameCfm;

/*! @brief Contents of the GattTelephoneBearerClientReadBearerUciCfm message that is sent by the library,
    as response to a read of the Bearer UCI.
 */
typedef struct __GattTelephoneBearerClientReadBearerUciCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint16 bearerUciSize;
    char  bearerUci[1];
} GattTelephoneBearerClientReadBearerUciCfm;

/*! @brief Contents of the GattTelephoneBearerClientReadBearerTechnologyCfm message that is sent by the library,
    as response to a read of the Bearer Technology.
 */
typedef struct __GattTelephoneBearerClientReadBearerTechnologyCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint16 bearerTechSize;
    char bearerTech[1];
} GattTelephoneBearerClientReadBearerTechnologyCfm;

/*! @brief Contents of the GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm message that is sent by the library,
    as response to a read of the Bearer URI Schemes Supported List.
 */
typedef struct __GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint16 uriListSize;
    char uriList[1];
} GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm;

/*! @brief Contents of the GattTelephoneBearerClientReadSignalStrengthCfm message that is sent by the library,
    as response to a read of the Signal Strength.
 */
typedef struct __GattTelephoneBearerClientReadSignalStrengthCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 signalStrength;
} GattTelephoneBearerClientReadSignalStrengthCfm;

/*! @brief Contents of the GattTelephoneBearerClientReadSignalStrengthIntervalCfm message that is sent by the library,
    as response to a read of the Signal Strength Reporting Interval.
 */
typedef struct __GattTelephoneBearerClientReadSignalStrengthIntervalCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 interval; /* in seconds */
} GattTelephoneBearerClientReadSignalStrengthIntervalCfm;

/*! @brief Contents of the GattTelephoneBearerClientReadCurrentCallsList message that is sent by the library,
    as response to a read of the Current Calls List.
 */
typedef struct __GattTelephoneBearerClientReadCurrentCallsListCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint16 currentCallsListSize;
    uint8 currentCallsList[1];
} GattTelephoneBearerClientReadCurrentCallsListCfm;

/*! @brief Contents of the GattTelephoneBearerClientReadContentControlIdCfm message that is sent by the library,
    as response to a read of the Content Control Id.
 */
typedef struct __GattTelephoneBearerClientReadContentControlIdCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint16 contentControlId;
} GattTelephoneBearerClientReadContentControlIdCfm;

/*! @brief Contents of the GattTelephoneBearerClientReadFeatureAndStatusFlagsCfm message that is sent by the library,
    as response to a read of the Status and Feature Flags.
 */
typedef struct __GattTelephoneBearerClientReadFeatureAndStatusFlagsCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    GattTbsStatusFlags           flags;
} GattTelephoneBearerClientReadFeatureAndStatusFlagsCfm;

/*! @brief Contents of the GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm message that is sent by the library,
    as response to a read of the Incoming Call Target Bearer URI.
 */
typedef struct __GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 callId;
    uint16 uriSize;
    char uri[1];
} GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm;

/*! @brief Definition of call state characteristic element.
 */
typedef struct
{
    uint8                   callId;   /* index of the call */
    GattTbsCallStates       callState;/* Call State */
    GattTbsCallFlags        callFlags;/* Flags of the call */
} TbsCallState;

/*! @brief Contents of the GattTelephoneBearerClientMsgReadCallStateCfm message that is sent by the library,
    as response to a read of the Call State.
 */
typedef struct __GattTelephoneBearerClientMsgReadCallStateCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint16 callStateListSize; /* Number of TbsCallState elements in callStateList */
    TbsCallState callStateList[1];
} GattTelephoneBearerClientMsgReadCallStateCfm;

/*! @brief Contents of the GattTelephoneBearerClientMsgReadIncomingCallCfm message that is sent by the library,
    as response to a read of the Incoming Call.
 */
typedef struct __GattTelephoneBearerClientMsgReadIncomingCallCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 callId;
    uint16 callUriSize;
    uint8 callUri[1];
} GattTelephoneBearerClientMsgReadIncomingCallCfm;

/*! @brief Contents of the GattTelephoneBearerClientMsgReadCallFriendlyNameCfm message that is sent by the library,
    as response to a read of the Call Friendly Name.
 */
typedef struct __GattTelephoneBearerClientMsgReadCallFriendlyNameCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint8 callId;
    uint16 friendlyNameSize;
    char friendlyName[1];
} GattTelephoneBearerClientMsgReadCallFriendlyNameCfm;

/*! @brief Contents of the GattTelephoneBearerClientReadOptionalOpcodesCfm message that is sent by the library,
    as response to a read of the Call Control Point optional opcodes */
typedef struct __GattTelephoneBearerClientReadOptionalOpcodesCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTelephoneBearerClientStatus status;
    uint16 opcodes;
} GattTelephoneBearerClientReadOptionalOpcodesCfm;

/* Notification Messages */

/*! @brief Contents of the GattTelephoneBearerClientProviderNameInd message that is sent by the library,
    as a result of a notification of the Provider Name.
 */
typedef struct __GattTelephoneBearerClientProviderNameInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    uint16 providerNameSize;
    char  providerName[1];
} GattTelephoneBearerClientProviderNameInd;

/*! @brief Contents of the GattTelephoneBearerClientBearerTechnologyInd message that is sent by the library,
    as a result of a notification of the Bearer Technology.
 */
typedef struct __GattTelephoneBearerClientBearerTechnologyInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    uint16 bearerTechSize;
    char  bearerTech[1];
} GattTelephoneBearerClientBearerTechnologyInd;

/*! @brief Contents of the GattTelephoneBearerClientSignalStrengthInd message that is sent by the library,
    as a result of a notification of the Signal Strength.
 */
typedef struct __GattTelephoneBearerClientSignalStrengthInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    uint8 signalStrength;
} GattTelephoneBearerClientSignalStrengthInd;

/*! @brief Contents of the GattTelephoneBearerClientCurrentCallsInd message that is sent by the library,
    as a result of a notification of the Current Calls List.
 */
typedef struct __GattTelephoneBearerClientCurrentCallsInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    uint16 currentCallsListSize;
    uint8 currentCallsList[1];
} GattTelephoneBearerClientCurrentCallsInd;

/*! @brief Contents of the GattTelephoneBearerClientFlagsInd message that is sent by the library,
    as a result of a notification of the Feature and Status Flags.
 */
typedef struct __GattTelephoneBearerClientFlagsInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    uint16 flags;
} GattTelephoneBearerClientFlagsInd;

/*! @brief Contents of the GattTelephoneBearerClientIncomingCallTargetBearerUriInd message that is sent by the library,
    as a result of a notification of the Incoming Call Target Bearer URI.
 */
typedef struct __GattTelephoneBearerClientIncomingCallTargetBearerUriInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    uint8 callId;
    uint16 uriSize;
    char uri[1];
} GattTelephoneBearerClientIncomingCallTargetBearerUriInd;

/*! @brief Contents of the GattTelephoneBearerClientCallStateInd message that is sent by the library,
    as a result of a notification of the Call State.
 */
typedef struct __GattTelephoneBearerClientCallStateInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    uint16 callStateListSize; /* Number of TbsCallState elements in callStateList */
    TbsCallState callStateList[1];
} GattTelephoneBearerClientCallStateInd;

/*! @brief Contents of the GattTelephoneBearerClientCallControlPointInd message that is sent by the library,
    as a result of a notification of the Call Control Point.
 */
typedef struct __GattTelephoneBearerClientCallControlPointInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    GattTbsOpcode opcode;
    uint8 callId;
    uint8 resultCode;
} GattTelephoneBearerClientCallControlPointInd;

/*! @brief Contents of the GattTelephoneBearerClientTerminationReasonInd message that is sent by the library,
    as a result of a notification of the Termination Reason
 */
typedef struct __GattTelephoneBearerClientTerminationReasonInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    uint8 callId;
    uint8 reasonCode;
} GattTelephoneBearerClientTerminationReasonInd;

/*! @brief Contents of the GattTelephoneBearerClientIncomingCallInd message that is sent by the library,
    as a result of a notification of the Incoming Call.
 */
typedef struct __GattTelephoneBearerClientIncomingCallInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    uint8 callId;
    uint16 uriSize;
    uint8 uri[1];
} GattTelephoneBearerClientIncomingCallInd;

/*! @brief Contents of the GattTelephoneBearerClientCallFriendlyNameInd message that is sent by the library,
    as a result of a notification of the Call Friendly Name indication
 */
typedef struct __GattTelephoneBearerClientCallFriendlyNameInd
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    uint8 callId;
    uint16 friendlyNameSize;
    char  friendlyName[1];
} GattTelephoneBearerClientCallFriendlyNameInd;



/* Notification Enable Cfm Messages */
/*! @brief Contents of the GattTelephoneBearerClientSetCfm is a template used for characteristic
 *  write and set confirmation messages.
 */
typedef struct __GattTelephoneBearerClientSetCfm
{
    GattTelephoneBearerClientMessageId id;
    ServiceHandle tbsHandle;
    status_t status;
} GattTelephoneBearerClientSetCfm;

/*! @brief Contents of the GattTelephoneBearerClientProviderNameSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Bearer Provider Name characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientProviderNameSetNotificationCfm;

/*! @brief Contents of the GattTelephoneBearerClientTechnologySetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Bearer technology characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientTechnologySetNotificationCfm;

/*! @brief Contents of the GattTelephoneBearerClientSignalStrengthSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Signal Strength characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientSignalStrengthSetNotificationCfm;

/*! @brief Contents of the GattTelephoneBearerClientCurrentCallsSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Current Calls characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientCurrentCallsSetNotificationCfm;

/*! @brief Contents of the GattTelephoneBearerClientFlagsSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Feature and Status Flags characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientFlagsSetNotificationCfm;

/*! @brief Contents of the GattTelephoneBearerClientIncomingCallTargetBearerUriSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Incoming Call Target Bearer URI characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientIncomingCallTargetBearerUriSetNotificationCfm;

/*! @brief Contents of the GattTelephoneBearerClientCallStateSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Call State characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientCallStateSetNotificationCfm;

/*! @brief Contents of the GattTelephoneBearerClientCallControlPointSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Call Control Point characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientCallControlPointSetNotificationCfm;

/*! @brief Contents of the GattTelephoneBearerClientTerminationReasonSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Termination Reason characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientTerminationReasonSetNotificationCfm;

/*! @brief Contents of the GattTelephoneBearerClientIncomingCallSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Incoming Call characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientIncomingCallSetNotificationCfm;

/*! @brief Contents of the GattTelephoneBearerClientCallFriendlyNameSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Call friendly name characteristic.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientCallFriendlyNameSetNotificationCfm;


/* Write CFM Messages */

/*! @brief Contents of the GattTelephoneBearerClientWriteSignalStrengthIntervalCfm message that is sent by the library,
    as a result of writing the Signal Strength Interval characteristic on the server.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientWriteSignalStrengthIntervalCfm;

/*! @brief Contents of the GattTelephoneBearerClientWriteCallControlPointCfm message that is sent by the library,
    as a result of writing the Call Control point characteristic on the server.
 */
typedef GattTelephoneBearerClientSetCfm GattTelephoneBearerClientWriteCallControlPointCfm;


/*!
    @brief After the application has used the GATT manager to establish a connection to a discovered BLE device in the Client role,
    it can discover any supported services in which it has an interest. It should then register with the relevant client service library
    (passing the relevant CID and handles to the service). For the TBS client it will use this API. The GATT manager 
    will then route notifications and indications to the correct instance of the client service library for the CID.

    The registered task will receive a GattTelephoneBearerClientInitCfm message with GattTelephoneBearerClientStatus status code and instance handle.

    @param appTask The Task that will receive the messages sent from this TBS client library.
    @param initData  Configuration data for client initialisation.
    @param deviceData Persistent data for the device.

    @return void

*/
void GattTelephoneBearerClientInit(
                           AppTask appTask,
                           const GattTelephoneBearerClientInitData *initData,
                           const GattTelephoneBearerClientDeviceData *deviceData);
                           

/*!
    @brief GATT TBS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    @param clntHndl    The service handle for this GATT Client Service.

    NOTE: GATT_TELEPHONE_BEARER_CLIENT_TERMINATE_CFM will be received with a GattTelephoneBearerClientStatus status code.
*/
void GattTelephoneBearerClientTerminateReq(ServiceHandle clntHndl);                           

/*!
    @brief When a GATT connection is removed, the application must remove all client service instances that were
    associated with the connection (using the CID value).
    This is the clean up routine as a result of calling the GattTelephoneBearerClientInit API. That is,
    the GattTelephoneBearerClientInit API is called when a connection is made, and the GattTelephoneBearerClientDestroy is called
    when the connection is removed.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.

    @return TRUE if successful, FALSE otherwise

*/
bool GattTelephoneBearerClientDestroy(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Provider Name from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_PROVIDER_NAME_CFM message will be sent to the registered
           application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadProviderNameRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Bearer UCI from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_UCI_CFM message will be sent to the registered
           application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadBearerUciRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Bearer Technology from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_TECHNOLOGY_CFM message will be sent to the registered
           application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadBearerTechnologyRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Bearer Supported URI Schemes from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM message will be sent
           to the registered application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadBearerUriRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Signal Strength from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_SIGNAL_STRENGTH_CFM message will be sent to the registered
           application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadSignalStrengthRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Signal Strength Reporting Interval from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_SIGNAL_STRENGTH_INTERVAL_CFM message will be sent to the registered
           application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadSignalStrengthIntervalRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Current Calls from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_CURRENT_CALLS_LIST_CFM message will be sent to the registered
           application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadCurrentCallsRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Content Control ID List from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_CONTENT_CONTROL_ID_CFM message will be sent to the registered
           application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadContentControlIdRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Feature and Status Flags from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM message will be sent to the registered
           application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadStatusAndFeatureFlagsRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read Incoming Call Target Bearer URI from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM message will be sent
           to the registered application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadIncomingTargetBearerUriRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Call State from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_CALL_STATE_CFM message will be sent to the registered
           application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadCallStateRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Incoming Call information from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_INCOMING_CALL_CFM message will be sent to the registered
           application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadIncomingCallRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Call Friendly Name from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_CALL_FRIENDLY_NAME_CFM message will be sent to
           the registered application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadCallFriendlyNameRequest(const ServiceHandle tbsHandle);

/*!
    @brief This API is used to read the Call Control Point Optional Opcodes from a remote device.
           A GATT_TELEPHONE_BEARER_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM message will be sent to
           the registered application Task with the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void GattTelephoneBearerClientReadCallControlPointOptionalOpcodesRequest(const ServiceHandle tbsHandle);


/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Provider Name notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_PROVIDER_NAME_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetProviderNameNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Bearer Technology notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_TECHNOLOGY_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetTechnologyNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Signal Strength notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetSignalStrengthNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Current Calls List notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_CURRENT_CALLS_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetListCurrentCallsNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Status and Feature Flags notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_FLAGS_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetFlagsNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Incoming Call Target Bearer URI notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetIncomingCallTargetBearerUriNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Call State notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_CALL_STATE_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetCallStateNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Call Control Point notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetCallControlPointNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Termination Reason notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_TERMINATION_REASON_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetTerminationReasonNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Incoming Call notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetIncomingCallNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Call Friendly Name notifications with the server.
    A GATT_TELEPHONE_BEARER_CLIENT_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM message will be sent to the registered application Task
    indicating the result of the request.

    @param tbsHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void GattTelephoneBearerClientSetCallFriendlyNameNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable);


/*!
    @brief This API is used to write the Signal Strength reporting interval on the remote device.
    A GATT_TELEPHONE_BEARER_CLIENT_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM message will be sent to the registered application Task
    indicating the result of the write request.

    @param tbsHandle The client instance that was passed into the GattTelephoneBearerClientInit API.
    @param interval   Reporting interval to set in seconds.
    @param writeWithoutResponse    If true GATT Write Without Response is used, else GATT Write Characteristic Value is used
    @return void
*/
void GattTelephoneBearerClientWriteSignalStrengthIntervalRequest(const ServiceHandle tbsHandle,
                                                                        const uint8 interval,
                                                                        bool writeWithoutResponse);

/*!
    @brief This API is used to write to the Call Control Point the remote device.
    This function only works for simple two octet commands that require only an opcode and call index
    A GATT_TELEPHONE_BEARER_CLIENT_WRITE_CALL_CONTROL_POINT_CFM message will be sent to the registered application Task
    indicating the result of the write request.

    @param tbsHandle The client instance that was passed into the GattTelephoneBearerClientInit API.
    @param opcode   opcode to use
    @param callIndex           call index
    @return void
*/
void GattTelephoneBearerClientWriteCallControlPointSimpleRequest(const ServiceHandle tbsHandle, const GattTbsOpcode opcode, const uint8 callIndex);

/*!
    @brief This API is used to write to the Call Control Point the remote device.
    A GATT_TELEPHONE_BEARER_CLIENT_WRITE_CALL_CONTROL_POINT_CFM message will be sent to the registered application Task
    indicating the result of the write request.

    @param tbsHandle The client instance that was passed into the GattTelephoneBearerClientInit API.
    @param opcode   opcode to use
    @param size   size of opcode parameter
    @param param  opcode parameter
    @return void
*/
void GattTelephoneBearerClientWriteCallControlPointRequest(const ServiceHandle tbsHandle, const GattTbsOpcode opcode, const uint8 size, const uint8* param);

/*!
    @brief This API is used to retrieve the characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param clntHndl      The service handle for this GATT Client Service.

    @return GattTbsClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately
          by the function.

*/
GattTelephoneBearerClientDeviceData *GattTelephoneBearerClientGetHandlesReq(ServiceHandle clntHndl);

#endif
