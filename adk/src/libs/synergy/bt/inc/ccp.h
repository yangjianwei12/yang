/****************************************************************************
Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    ccp.h
    
DESCRIPTION
    Header file for the Call Control Profile (CCP) library.
*/

/*!
@file    ccp.h
@brief   Header file for the GATT CCP library.

        This file provides documentation for the GATT CCP library
        API (library name: ccp).
*/

#ifndef CCP_H
#define CCP_H

#include "csr_bt_gatt_lib.h"
#include "service_handle.h"

#include "gatt_telephone_bearer_client.h"

/*!
    @brief Profile handle type.
*/
typedef ServiceHandle CcpProfileHandle;

#define     CCP_MESSAGE_BASE        0x66D0


/*!
    @brief CCP handles.

*/
typedef struct _CcpHandles
{
    GattTelephoneBearerClientDeviceData tbsHandle;
}CcpHandles;

/*!
    @brief Initialisation parameters.

*/
typedef struct _CcpInitData
{
    connection_id_t cid;
}CcpInitData;


/*!
    \brief CCP status code type.
*/
typedef uint16 CcpStatus;

/*! { */
/*! Values for the CCP status code */
#define CCP_STATUS_SUCCESS              ((CcpStatus)0x0000u)  /*!> Request was a success*/
#define CCP_STATUS_IN_PROGRESS          ((CcpStatus)0x0001u)  /*!> Request in progress*/
#define CCP_STATUS_INVALID_PARAMETER    ((CcpStatus)0x0002u)  /*!> Invalid parameter was supplied*/
#define CCP_STATUS_DISCOVERY_ERR        ((CcpStatus)0x0003u)  /*!> Error in discovery of one of the services*/
#define CCP_STATUS_FAILED               ((CcpStatus)0x0004u)  /*!> Request has failed*/
/*! } */

/*! @brief Messages a client task may receive from the profile library.
 */
typedef uint16 CcpMessageId;

#define CCP_INIT_CFM                                ((CcpMessageId)CCP_MESSAGE_BASE)
#define CCP_DESTROY_CFM                             ((CcpMessageId)CCP_MESSAGE_BASE + 0x0001u)
#define CCP_TBS_TERMINATE_CFM                       ((CcpMessageId)CCP_MESSAGE_BASE + 0x0002u)
#define CCP_SET_CFM                                 ((CcpMessageId)CCP_MESSAGE_BASE + 0x0003u)


    /* Characteristic Read Confirmation messages */
#define CCP_READ_PROVIDER_NAME_CFM                  ((CcpMessageId)CCP_MESSAGE_BASE + 0x0004u)
#define CCP_READ_BEARER_UCI_CFM                     ((CcpMessageId)CCP_MESSAGE_BASE + 0x0005u)
#define CCP_READ_BEARER_TECHNOLOGY_CFM              ((CcpMessageId)CCP_MESSAGE_BASE + 0x0006u)
#define CCP_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM ((CcpMessageId)CCP_MESSAGE_BASE + 0x0007u)
#define CCP_READ_SIGNAL_STRENGTH_CFM                ((CcpMessageId)CCP_MESSAGE_BASE + 0x0008u)
#define CCP_READ_SIGNAL_STRENGTH_INTERVAL_CFM       ((CcpMessageId)CCP_MESSAGE_BASE + 0x0009u)
#define CCP_READ_CURRENT_CALLS_LIST_CFM             ((CcpMessageId)CCP_MESSAGE_BASE + 0x000au)
#define CCP_READ_CONTENT_CONTROL_ID_CFM             ((CcpMessageId)CCP_MESSAGE_BASE + 0x000bu)
#define CCP_READ_FEATURE_AND_STATUS_FLAGS_CFM       ((CcpMessageId)CCP_MESSAGE_BASE + 0x000cu)
#define CCP_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM ((CcpMessageId)CCP_MESSAGE_BASE + 0x000du)
#define CCP_READ_CALL_STATE_CFM                     ((CcpMessageId)CCP_MESSAGE_BASE + 0x000eu)
#define CCP_READ_INCOMING_CALL_CFM                  ((CcpMessageId)CCP_MESSAGE_BASE + 0x000fu)
#define CCP_READ_CALL_FRIENDLY_NAME_CFM             ((CcpMessageId)CCP_MESSAGE_BASE + 0x0010u)
#define CCP_READ_CCP_OPTIONAL_OPCODES_CFM           ((CcpMessageId)CCP_MESSAGE_BASE + 0x0011u)

    /* Characteristic Notification Confirmation messages */
#define CCP_PROVIDER_NAME_SET_NOTIFICATION_CFM      ((CcpMessageId)CCP_MESSAGE_BASE + 0x0012u)
#define CCP_TECHNOLOGY_SET_NOTIFICATION_CFM         ((CcpMessageId)CCP_MESSAGE_BASE + 0x0013u)
#define CCP_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM    ((CcpMessageId)CCP_MESSAGE_BASE + 0x0014u)
#define CCP_CURRENT_CALLS_SET_NOTIFICATION_CFM      ((CcpMessageId)CCP_MESSAGE_BASE + 0x0015u)
#define CCP_FLAGS_SET_NOTIFICATION_CFM              ((CcpMessageId)CCP_MESSAGE_BASE + 0x0016u)
#define CCP_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM    ((CcpMessageId)CCP_MESSAGE_BASE + 0x0017u)
#define CCP_CALL_STATE_SET_NOTIFICATION_CFM         ((CcpMessageId)CCP_MESSAGE_BASE + 0x0018u)
#define CCP_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM ((CcpMessageId)CCP_MESSAGE_BASE + 0x0019u)
#define CCP_TERMINATION_REASON_SET_NOTIFICATION_CFM ((CcpMessageId)CCP_MESSAGE_BASE + 0x001au)
#define CCP_INCOMING_CALL_SET_NOTIFICATION_CFM      ((CcpMessageId)CCP_MESSAGE_BASE + 0x001bu)
#define CCP_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM ((CcpMessageId)CCP_MESSAGE_BASE + 0x001cu)
    /* Write Characteristic CFMs */
#define CCP_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM      ((CcpMessageId)CCP_MESSAGE_BASE + 0x001du)
#define CCP_WRITE_CALL_CONTROL_POINT_CFM            ((CcpMessageId)CCP_MESSAGE_BASE + 0x001eu)

    /* Characteristic Indication messages */
#define CCP_PROVIDER_NAME_IND                       ((CcpMessageId)CCP_MESSAGE_BASE + 0x001fu)
#define CCP_BEARER_TECHNOLOGY_IND                   ((CcpMessageId)CCP_MESSAGE_BASE + 0x0020u)
#define CCP_SIGNAL_STRENGTH_IND                     ((CcpMessageId)CCP_MESSAGE_BASE + 0x0021u)
#define CCP_CURRENT_CALLS_IND                       ((CcpMessageId)CCP_MESSAGE_BASE + 0x0022u)
#define CCP_FLAGS_IND                               ((CcpMessageId)CCP_MESSAGE_BASE + 0x0023u)
#define CCP_INCOMING_CALL_TARGET_BEARER_URI_IND     ((CcpMessageId)CCP_MESSAGE_BASE + 0x0024u)
#define CCP_CALL_STATE_IND                          ((CcpMessageId)CCP_MESSAGE_BASE + 0x0025u)
#define CCP_CALL_CONTROL_POINT_IND                  ((CcpMessageId)CCP_MESSAGE_BASE + 0x0026u)
#define CCP_TERMINATION_REASON_IND                  ((CcpMessageId)CCP_MESSAGE_BASE + 0x0027u)
#define CCP_INCOMING_CALL_IND                       ((CcpMessageId)CCP_MESSAGE_BASE + 0x0028u)
#define CCP_CALL_FRIENDLY_NAME_IND                  ((CcpMessageId)CCP_MESSAGE_BASE + 0x0029u)
    /* Library message limit */
#define CCP_MESSAGE_TOP                             ((CcpMessageId)CCP_MESSAGE_BASE + 0x002au)


/*!
    @brief Profile library message sent as a result of calling the CcpInitReq API.
*/
typedef struct __CcpInitCfm
{
    CcpMessageId      id;
    CcpStatus         status;      /*! Status of the initialisation attempt*/
    CcpProfileHandle  prflHndl;   /*! CCP profile handle*/
} CcpInitCfm;


/*!
    @brief Profile library message sent as a result of calling the CcpDestroyReq API.

    This message will send at first with the value of status of CCP_STATUS_IN_PROGRESS.
    Another CcpDestroyCfm message will be sent with the final status (success or fail),
    after CcpTbsTerminateCfm have been received.
*/
typedef struct __CcpDestroyCfm
{
    CcpMessageId      id;
    CcpProfileHandle  prflHndl;   /*! CCP profile handle */
    CcpStatus         status;     /*! Status of the attempt */
} CcpDestroyCfm;

/*!
    @brief Profile library message sent as a result of calling the CcpDestroyReq API and
           of the receiving of the CcpTbsTerminateCfm message from the Gatt TBS Client
           library.
*/
typedef struct __CcpTbsTerminateCfm
{
    CcpMessageId                         id;
    CcpProfileHandle                     prflHndl;        /*! CCP profile handle*/
    CcpStatus                            status;          /*! Status of the termination attempt*/
    GattTelephoneBearerClientDeviceData  tbsHandle;       /*! Characteristic handles of TBS*/
} CcpTbsTerminateCfm;




/* Notification Enable Cfm Messages */
/*! @brief Contents of the CcpSetCfm is a template used for characteristic
 *  write and set confirmation messages.
 */
typedef struct __CcpSetCfm
{
    CcpMessageId                    id;
    CcpProfileHandle                prflHndl;
    ServiceHandle                   srvcHndl;
    GattTelephoneBearerClientStatus status;
} CcpSetCfm;

/*! @brief Contents of the CcpProviderNameSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Bearer Provider Name characteristic.
 */
typedef CcpSetCfm CcpProviderNameSetNotificationCfm;

/*! @brief Contents of the CcpTechnologySetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Bearer technology characteristic.
 */
typedef CcpSetCfm CcpTechnologySetNotificationCfm;

/*! @brief Contents of the CcpSignalStrengthSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Signal Strength characteristic.
 */
typedef CcpSetCfm CcpSignalStrengthSetNotificationCfm;

/*! @brief Contents of the CcpCurrentCallsSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Current Calls characteristic.
 */
typedef CcpSetCfm CcpCurrentCallsSetNotificationCfm;

/*! @brief Contents of the CcpFlasgSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Feature and Status Flags characteristic.
 */
typedef CcpSetCfm CcpFlasgSetNotificationCfm;

/*! @brief Contents of the CcpIncomingCallTargetBearerUriSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Incoming Call Target Bearer URI characteristic.
 */
typedef CcpSetCfm CcpIncomingCallTargetBearerUriSetNotificationCfm;

/*! @brief Contents of the CcpCallStateSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Call State characteristic.
 */
typedef CcpSetCfm CcpCallStateSetNotificationCfm;

/*! @brief Contents of the CcpCallControlPointSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Call Control Point characteristic.
 */
typedef CcpSetCfm CcpCallControlPointSetNotificationCfm;

/*! @brief Contents of the CcpTerminationReasonSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Termination Reason characteristic.
 */
typedef CcpSetCfm CcpTerminationReasonSetNotificationCfm;

/*! @brief Contents of the CcpIncomingCallSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Incoming Call characteristic.
 */
typedef CcpSetCfm CcpIncomingCallSetNotificationCfm;

/*! @brief Contents of the CcpIncomingCallFriendlyNameSetNotificationCfm message that is sent by the library,
    as a result of configuring notifications on the server for the Incoming Call friendly name characteristic.
 */
typedef CcpSetCfm CcpIncomingCallFriendlyNameSetNotificationCfm;



/* Write CFM Messages */

/*! @brief Contents of the CcpWriteSignalStrengthIntervalCfm message that is sent by the library,
    as a result of writing the Signal Strength Interval characteristic on the server.
 */
typedef CcpSetCfm CcpWriteSignalStrengthIntervalCfm;

/*! @brief Contents of the CcpWriteCallControlPointCfm message that is sent by the library,
    as a result of writing the Call Control point characteristic on the server.
 */
typedef CcpSetCfm CcpWriteCallControlPointCfm;


/* Read CFM Messages */

/*! @brief Contents of the CcpReadProviderNameCfm message that is sent by the library,
    as response to a read of the Bearer Provider Name.
 */
typedef struct __CcpReadProviderNameCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 providerNameSize;
    char  providerName[1];
} CcpReadProviderNameCfm;

/*! @brief Contents of the CcpReadBearerUciCfm message that is sent by the library,
    as response to a read of the Bearer UCI.
 */
typedef struct __CcpReadBearerUciCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 bearerUciSize;
    char  bearerUci[1];
} CcpReadBearerUciCfm;

/*! @brief Contents of the CcpReadBearerTechnologyCfm message that is sent by the library,
    as response to a read of the Bearer Technology.
 */
typedef struct __CcpReadBearerTechnologyCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 bearerTechSize;
    char  bearerTech[1];
} CcpReadBearerTechnologyCfm;

/*! @brief Contents of the CcpReadBearerUriSchemesSupportedListCfm message that is sent by the library,
    as response to a read of the Bearer URI Schemes Supported List.
 */
typedef struct __CcpReadBearerUriSchemesSupportedListCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 uriListSize;
    char uriList[1];
} CcpReadBearerUriSchemesSupportedListCfm;

/*! @brief Contents of the CcpReadSignalStrengthCfm message that is sent by the library,
    as response to a read of the Signal Strength.
 */
typedef struct __CcpReadSignalStrengthCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 signalStrength;
} CcpReadSignalStrengthCfm;

/*! @brief Contents of the CcpReadSignalStrengthIntervalCfm message that is sent by the library,
    as response to a read of the Signal Strength Reporting Interval.
 */
typedef struct __CcpReadSignalStrengthIntervalCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 interval; /* in seconds */
} CcpReadSignalStrengthIntervalCfm;

/*! @brief Contents of the CcpReadCurrentCallsListCfm message that is sent by the library,
    as response to a read of the Current Calls List.
 */
typedef struct __CcpReadCurrentCallsListCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 currentCallsListSize;
    uint8 currentCallsList[1];
} CcpReadCurrentCallsListCfm;

/*! @brief Contents of the CcpReadContentControlIdCfm message that is sent by the library,
    as response to a read of the Content Control Id.
 */
typedef struct __CcpReadContentControlIdCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint16 contentControlId;
} CcpReadContentControlIdCfm;

/*! @brief Contents of the CcpReadFlagsCfm message that is sent by the library,
    as response to a read of the Status and Feature Flags.
 */
typedef struct __CcpReadFlagsCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    GattTbsStatusFlags flags;
} CcpReadFlagsCfm;

/*! @brief Contents of the CcpReadIncomingCallTargetBearerUriCfm message that is sent by the library,
    as response to a read of the Incoming Call Target Bearer URI.
 */
typedef struct __CcpReadIncomingCallTargetBearerUriCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 callId;
    uint8 uriSize;
    char uri[1];
} CcpReadIncomingCallTargetBearerUriCfm;

/*! @brief Contents of the CcpReadCallStateCfm message that is sent by the library,
    as response to a read of the Call State.
 */
typedef struct __CcpReadCallStateCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 callStateListSize; /* Number of TbsCallState elements in callStateList */
    TbsCallState callStateList[1];
} CcpReadCallStateCfm;

/*! @brief Contents of the CcpReadIncomingCallCfm message that is sent by the library,
    as response to a read of the Incoming Call.
 */
typedef struct __CcpReadIncomingCallCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 callId;
    uint8 callUriSize;
    uint8 callUri[1];
} CcpReadIncomingCallCfm;

/*! @brief Contents of the CcpReadCallFriendlyNameCfm message that is sent by the library,
    as response to a read of the Call Friendly Name.
 */
typedef struct __CcpReadCallFriendlyNameCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 callId;
    uint8 friendlyNameSize;
    char friendlyName[1];
} CcpReadCallFriendlyNameCfm;

/*! @brief Contents of the CcpReadOptionalOpcodesCfm message that is sent by the library,
    as response to a read of the Call Control Point optional opcodes */
typedef struct __CcpReadOptionalOpcodesCfm
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTelephoneBearerClientStatus status;
    uint8 opcodes;
} CcpReadOptionalOpcodesCfm;

/* Notification Messages */

/*! @brief Contents of the CcpProviderNameInd message that is sent by the library,
    as a result of a notification of the Provider Name.
 */
typedef struct __CcpProviderNameInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    uint8 providerNameSize;
    char  providerName[1];
} CcpProviderNameInd;

/*! @brief Contents of the CcpBearerTechnologyInd message that is sent by the library,
    as a result of a notification of the Bearer Technology.
 */
typedef struct __CcpBearerTechnologyInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    uint8 bearerTechSize;
    char  bearerTech[1];
} CcpBearerTechnologyInd;

/*! @brief Contents of the CcpSignalStrengthInd message that is sent by the library,
    as a result of a notification of the Signal Strength.
 */
typedef struct __CcpSignalStrengthInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    uint8 signalStrength;
} CcpSignalStrengthInd;

/*! @brief Contents of the CcpCurrentCallsListInd message that is sent by the library,
    as a result of a notification of the Current Calls List.
 */
typedef struct __CcpCurrentCallsListInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    uint8 currentCallsListSize;
    uint8 currentCallsList[1];
} CcpCurrentCallsListInd;

/*! @brief Contents of the CcpFlagsInd message that is sent by the library,
    as a result of a notification of the Feature and Status Flags.
 */
typedef struct __CcpFlagsInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTbsStatusFlags flags;
} CcpFlagsInd;

/*! @brief Contents of the CcpIncomingCallTargetBearerUriInd message that is sent by the library,
    as a result of a notification of the Incoming Call Target Bearer URI.
 */
typedef struct __CcpIncomingCallTargetBearerUriInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    uint8 callId;
    uint8 uriSize;
    char uri[1];
} CcpIncomingCallTargetBearerUriInd;

/*! @brief Contents of the CcpCallStateInd message that is sent by the library,
    as a result of a notification of the Call State.
 */
typedef struct __CcpCallStateInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    uint8 callStateListSize; /* Number of TbsCallState elements in callStateList */
    TbsCallState callStateList[1];
} CcpCallStateInd;

/*! @brief Contents of the CcpCallControlPointInd message that is sent by the library,
    as a result of a notification of the Call Control Point.
 */
typedef struct __CcpCallControlPointInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    GattTbsOpcode opcode;
    uint8 callId;
    uint8 resultCode;
} CcpCallControlPointInd;

/*! @brief Contents of the CcpTerminationReasonInd message that is sent by the library,
    as a result of a notification of the Termination Reason
 */
typedef struct __CcpTerminationReasonInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    uint8 callId;
    uint8 reasonCode;
} CcpTerminationReasonInd;

/*! @brief Contents of the CcpIncomingCallInd message that is sent by the library,
    as a result of a notification of the Incoming Call.
 */
typedef struct __CcpIncomingCallInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    uint8 callId;
    uint8 uriSize;
    uint8 uri[1];
} CcpIncomingCallInd;

/*! @brief Contents of the CcpCallFriendlyNameInd message that is sent by the library,
    as a result of a notification of the Call Friendly Name indication
 */
typedef struct __CcpCallFriendlyNameInd
{
    CcpMessageId      id;
    CcpProfileHandle prflHndl;
    uint8 callId;
    uint8 friendlyNameSize;
    char  friendlyName[1];
} CcpCallFriendlyNameInd;


/*!
    @brief Initialises the Gatt CCP Library.

    NOTE: This interface need to be invoked for every new gatt connection that wishes to use
    the Gatt CCP library.

    @param appTask           The Task that will receive the messages sent from this immediate alert profile library
    @param clientInitParams  Initialisation parameters
    @param deviceData        gTBS handles
    @param tbsRequired       set true if TBS required in addition to gTBS

    NOTE: A CCP_INIT_CFM with CcpStatus code equal to CCP_STATUS_IN_PROGRESS will be received as indication that
          the profile library initialisation started. Once completed CCP_INIT_CFM will be received with a CcpStatus
          that indicates the result of the initialisation.
*/
void CcpInitReq(AppTask appTask,
                CcpInitData *clientInitParams,
                CcpHandles *deviceData,
                bool tbsRequired);

/*!
    @brief When a GATT connection is removed, the application must remove all profile and
    client service instances that were associated with the connection.
    This is the clean up routine as a result of calling the CcpInitReq API.

    @param profileHandle The Profile handle.

    NOTE: A CCP_DESTROY_CFM with CcpStatus code equal to CCP_STATUS_IN_PROGRESS will be
    received as indication that the profile library destroy started. Once completed
    CCP_DESTROY_CFM will be received with a CcpStatus that indicates the result of the destroy.
*/
void CcpDestroyReq(CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Provider Name from a remote device.
           A CcpReadProviderNameCfm message will be sent to the registered
           application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadProviderNameRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Bearer UCI from a remote device.
           A CcpReadBearerUciCfm message will be sent to the registered
           application Task with the result of the request.

    @param profileHandle Handle to the client instance that was passed into the GattTelephoneBearerClientInit API.
    @return void
*/
void CcpReadBearerUciRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Bearer Technology from a remote device.
           A CcpReadBearerTechnologyCfm message will be sent to the registered
           application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadBearerTechnologyRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Bearer Supported URI Schemes from a remote device.
           A CcpReadBearerUriSchemesSupportedListCfm message will be sent
           to the registered application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadBearerUriRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Signal Strength from a remote device.
           A CcpReadSignalStrengthCfm message will be sent to the registered
           application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadSignalStrengthRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Signal Strength Reporting Interval from a remote device.
           A CcpReadSignalStrengthIntervalCfm message will be sent to the registered
           application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadSignalStrengthIntervalRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Current Calls from a remote device.
           A CcpReadCurrentCallsListCfm message will be sent to the registered
           application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadCurrentCallsRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Content Control ID List from a remote device.
           A CcpReadContentControlIdCfm message will be sent to the registered
           application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadContentControlIdRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Feature and Status Flags from a remote device.
           A CcpReadFlagsCfm message will be sent to the registered
           application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadStatusAndFeatureFlagsRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read Incoming Call Target Bearer URI from a remote device.
           A CcpReadIncomingCallTargetBearerUriCfm message will be sent
           to the registered application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadIncomingTargetBearerUriRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Call State from a remote device.
           A CcpReadCallStateCfm message will be sent to the registered
           application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadCallStateRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Incoming Call information from a remote device.
           A CcpReadIncomingCallCfm message will be sent to the registered
           application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadIncomingCallRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Call Friendly Name from a remote device.
           A CcpReadCallFriendlyNameCfm message will be sent to
           the registered application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadCallFriendlyNameRequest(const CcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Call Control Point Optional Opcodes from a remote device.
           A CcpReadOptionalOpcodesCfm message will be sent to
           the registered application Task with the result of the request.

    @param profileHandle       The Profile handle.
    @return void
*/
void CcpReadCallControlPointOptionalOpcodesRequest(const CcpProfileHandle profileHandle);


/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Provider Name notifications with the server.
    A CcpProviderNameSetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetProviderNameNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Bearer Technology notifications with the server.
    A CcpTechnologySetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetTechnologyNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Signal Strength notifications with the server.
    A CcpSignalStrengthSetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetSignalStrengthNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Current Calls List notifications with the server.
    A CcpCurrentCallsSetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetListCurrentCallsNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Status and Feature Flags notifications with the server.
    A CcpFlasgSetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetFlagsNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Incoming Call Target Bearer URI notifications with the server.
    A CcpIncomingCallTargetBearerUriSetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetIncomingCallTargetBearerUriNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Call State notifications with the server.
    A CcpCallStateSetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetCallStateNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Call Control Point notifications with the server.
    A CcpCallControlPointSetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetCallControlPointNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Termination Reason notifications with the server.
    A CcpTerminationReasonSetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetTerminationReasonNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Incoming Call notifications with the server.
    A CcpIncomingCallSetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetIncomingCallNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration on the remote device,
    to configure Call Friendly Name notifications with the server.
    A CcpIncomingCallFriendlyNameSetNotificationCfm message will be sent to the registered application Task
    indicating the result of the request.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @return void
*/
void CcpSetCallFriendlyNameNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable);


/*!
    @brief This API is used to write the Signal Strength reporting interval on the remote device.
    A CcpWriteSignalStrengthIntervalCfm message will be sent to the registered application Task
    indicating the result of the write request.

    @param profileHandle          The Profile handle.
    @param interval               Reporting interval to set in seconds.
    @param writeWithoutResponse   If true GATT Write Without Response is used, else GATT Write Characteristic Value is used
    @return void
*/
void CcpWriteSignalStrengthIntervalRequest(const CcpProfileHandle profileHandle,
                                                                        const uint8 interval,
                                                                        bool writeWithoutResponse);

/*!
    @brief This API is used to write to the Call Control Point the remote device.
    This function only works for simple two octet commands that require only an opcode and call index
    A CcpWriteCallControlPointCfm message will be sent to the registered application Task
    indicating the result of the write request.

    @param profileHandle   The Profile handle.
    @param opcode          opcode to use
    @param callIndex       call index
    @return void
*/
void CcpWriteCallControlPointSimpleRequest(const CcpProfileHandle profileHandle, const GattTbsOpcode opcode, const uint8 callIndex);

/*!
    @brief This API is used to write to the Call Control Point the remote device.
    A CcpWriteCallControlPointCfm message will be sent to the registered application Task
    indicating the result of the write request.

    @param profileHandle   The Profile handle.
    @param opcode   opcode to use
    @param size   size of opcode parameter
    @param param  opcode parameter
    @return void
*/
void CcpWriteCallControlPointRequest(const CcpProfileHandle profileHandle, const GattTbsOpcode opcode, const uint8 size, const uint8* param);


/*!
    @brief This API is used to retrieve the TBS characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param profileHandle       The Profile handle.
    @param tbsHandle           The TBS instance handle.

    @return GattTbsClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately.

*/
GattTelephoneBearerClientDeviceData *CcpGetTelephoneBearerAttributeHandles(CcpProfileHandle profileHandle,
                                                               ServiceHandle tbsHandle);

#endif /* CCP_H */

