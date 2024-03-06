#ifndef CSR_BT_MAPC_LIB_H__
#define CSR_BT_MAPC_LIB_H__
/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_bt_profiles.h"
#include "csr_bt_mapc_prim.h"
#include "csr_bt_tasks.h"
#include "csr_pmem.h"
#include "csr_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Common put_message function to reduce code size */
void CsrBtMapcMsgTransport(CsrSchedQid phandle, void *msg);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcGetInstanceIdsReqSend
 *
 *  DESCRIPTION
 *      This signal is used to get the list of registered MAPC instances
 *      from the MAPC instance that is also running as MAPC-manager
 *
 *      MAP client sends CSR_BT_MAPC_GET_INSTANCE_IDS_CFM message back
 *      to the application with the list of supported instant ids.
 *
 *  PARAMETERS
 *      appHandle:    Application handle, so the MAPC-manager
 *                    knows where to return the result to.
 *---------------------------------------------------------------------------*/
#define CsrBtMapcGetInstanceIdsReqSend(_appHandle) {                                                   \
        CsrBtMapcGetInstanceIdsReq *msg = (CsrBtMapcGetInstanceIdsReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                       = CSR_BT_MAPC_GET_INSTANCE_IDS_REQ;                            \
        msg->appHandle                  = _appHandle;                                                  \
        CsrBtMapcMsgTransport(CSR_BT_MAPC_IFACEQUEUE, msg);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcConnectReqSend
 *
 *  DESCRIPTION
 *      Try to make a connection the a peer device.
 * 
 *      MAP client sends CSR_BT_MAPC_CONNECT_CFM message back
 *      to the application with the result of the connection.
 *
 *    PARAMETERS
 *      mapcInstanceId        The instance id of the client 
 *      appHandle:            application handle to which the response will be sent
 *      maxPacketSize:        maximum OBEX packet size that can be supported
 *      deviceAddr:           address of the device to connect
 *      windowSize            size of the tx queue. Controls how many packets the 
 *                            OBEX profile (and lower protocol layers) is allowed 
 *                            to cache on the data receive side. A value of 0
 *                            causes the system to auto-detect this value.
 *----------------------------------------------------------------------------*/
#define CsrBtMapcConnectReqSend(_mapcInstanceId, _appHandle,_maxPacketSize,_deviceAddr,_windowSize) {    \
        CsrBtMapcConnectReq *msg = (CsrBtMapcConnectReq*) CsrPmemAlloc(sizeof(*msg));                    \
        msg->type           = CSR_BT_MAPC_CONNECT_REQ;                                                   \
        msg->appHandle      = _appHandle;                                                                \
        msg->maxPacketSize  = _maxPacketSize;                                                            \
        msg->deviceAddr     = _deviceAddr;                                                               \
        msg->windowSize     = _windowSize;                                                               \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcCancelConnectReqSend
 *
 *  DESCRIPTION
 *      Try to cancel the connection which is being established with a peer device.
 *      if already connected and the application's request CSR_BT_MAPC_CANCEL_CONNECT_REQ 
 *      is processed after the MAPC has sent CSR_BT_MAPC_CONNECT_CFM with 
 *      CSR_BT_OBEX_SUCCESS_RESPONSE_CODE response code to the application,
 *      then MAPC considers CSR_BT_MAPC_CANCEL_CONNECT_REQ as a CSR_BT_MAPC_DISCONNECT_REQ 
 *      request and the application receives a CSR_BT_MAPC_DISCONNECT_IND message.
 *
 *      MAP client sends CSR_BT_MAPC_CONNECT_CFM message back
 *      to the application with the result of the connection. (see above for more details)
 *
 *    PARAMETERS
 *      mapcInstanceId        The instance id of the client 
 *----------------------------------------------------------------------------*/
#define CsrBtMapcCancelConnectReqSend(_mapcInstanceId) {                                             \
        CsrBtMapcCancelConnectReq *msg = (CsrBtMapcCancelConnectReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type    = CSR_BT_MAPC_CANCEL_CONNECT_REQ;                                               \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcDisconnectReqSend
 *
 *  DESCRIPTION
 *      The OBEX - and the Bluetooth connection is released
 *
 *      MAP client sends CSR_BT_MAPC_DISCONNECT_IND message back
 *      to the application with the result of the operation.
 *
 *    PARAMETERS
 *      mapcInstanceId          The instance id of the client 
 *      normalObexDisconnect :  FALSE Underlying transport channel (RFCOMM) is disconnected 
 *                              without sending the OBEX disconnect (abrupt disconnection). 
 *                              TRUE defines a normal disconnect sequence where the OBEX
 *                              connection is released before the Bluetooth connection
 *----------------------------------------------------------------------------*/
#define CsrBtMapcDisconnectReqSend(_mapcInstanceId, _normalObexDisconnect) {                   \
        CsrBtMapcDisconnectReq *msg = (CsrBtMapcDisconnectReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                  = CSR_BT_MAPC_DISCONNECT_REQ;                               \
        msg->normalObexDisconnect  = _normalObexDisconnect;                                    \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcSelectMasInstanceResSend
 *
 *  DESCRIPTION
 *      This signal is used in response to the CSR_BT_MAPC_SELECT_MAS_INSTANCE_IND
 *      where the application can specify which of the available masInstanceId's
 *      it wants to connect (if any).
 *
 *  PARAMETERS
 *      mapcInstanceId          The instance id of the client 
 *      proceedWithConnection:  specifies whether to proceed with the connection
 *                              establishment or not.
 *      masInstanceId:          the mas instance which the application wishes to connect
 *---------------------------------------------------------------------------*/
#define CsrBtMapcSelectMasInstanceResSend(_mapcInstanceId,                                                   \
                                          _proceedWithConnection,                                            \
                                          _masInstanceId) {                                                  \
        CsrBtMapcSelectMasInstanceRes *msg = (CsrBtMapcSelectMasInstanceRes*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                  = CSR_BT_MAPC_SELECT_MAS_INSTANCE_RES;                                    \
        msg->proceedWithConnection = _proceedWithConnection;                                                 \
        msg->masInstanceId         = _masInstanceId;                                                         \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}



/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcSetFolderReqSend
 *
 *  DESCRIPTION
 * 
 *      This signal changes the current folder on the MAP server to a specified 
 *      folder with the folderName parameter. This signal can navigate down the 
 *      directory hierarchy on the server.
 *
 *      MAP client sends CSR_BT_MAPC_SET_FOLDER_CFM message back
 *      to the application with the result of the operation.
 *
 *    PARAMETERS
 *      mapcInstanceId      The instance id of the client 
 *      folderName:         Null terminated name string of the folder to change. 
 *----------------------------------------------------------------------------*/
#define CsrBtMapcSetFolderReqSend(_mapcInstanceId,_folderName) {                             \
        CsrBtMapcSetFolderReq *msg = (CsrBtMapcSetFolderReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                  = CSR_BT_MAPC_SET_FOLDER_REQ;                             \
        msg->folderName            = _folderName;                                            \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcSetBackFolderReqSend
 *      
 *  DESCRIPTION
 *      This signal sets the current folder on the MAP server back to the parent folder
 *
 *      MAP client sends CSR_BT_MAPC_SET_BACK_FOLDER_CFM message back
 *      to the application with the result of the operation.
 *
 *    PARAMETERS
 *      mapcInstanceId          The instance id of the client 
 *----------------------------------------------------------------------------*/
#define CsrBtMapcSetBackFolderReqSend(_mapcInstanceId) {                                             \
        CsrBtMapcSetBackFolderReq *msg = (CsrBtMapcSetBackFolderReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                      = CSR_BT_MAPC_SET_BACK_FOLDER_REQ;                            \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcSetRootFolderReqSend
 *
 *  DESCRIPTION
 *      This signal sets the current folder on the MAP server to the root folder
 *
 *      MAP client sends CSR_BT_MAPC_SET_ROOT_FOLDER_CFM message back
 *      to the application with the result of the operation.
 *
 *    PARAMETERS
 *      mapcInstanceId          The instance id of the client 
 *----------------------------------------------------------------------------*/
#define CsrBtMapcSetRootFolderReqSend(_mapcInstanceId) {                                             \
        CsrBtMapcSetRootFolderReq *msg = (CsrBtMapcSetRootFolderReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                      = CSR_BT_MAPC_SET_ROOT_FOLDER_REQ;                            \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcGetFolderListingReqSend
 *
 *  DESCRIPTION
 *      This signal retrieves the folder listing object from the current folder of MSE
 *
 *      MAP client sends CSR_BT_MAPC_GET_FOLDER_LISTING_IND messages concluding 
 *      with CSR_BT_MAPC_GET_FOLDER_LISTING_CFM back to the application with 
 *      the result of the operation.
 *
 *  PARAMETERS
 *      mapcInstanceId:       map client instance id
 *      maxListCount:         Maximum number of folders to be listed in the folder listing.
 *      listStartOffset:      Offset from where the listing needs to be started.
 *      srmpOn:               Temporarily suspends the SRM, if TRUE. It is applicable 
 *                            only if peer device supports SRM.
 *----------------------------------------------------------------------------*/
#define CsrBtMapcGetFolderListingReqSend(_mapcInstanceId, _maxListCount,                                   \
                                        _listStartOffset, _srmpOn) {                                       \
        CsrBtMapcGetFolderListingReq *msg = (CsrBtMapcGetFolderListingReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type              = CSR_BT_MAPC_GET_FOLDER_LISTING_REQ;                                       \
        msg->maxListCount      = _maxListCount;                                                            \
        msg->listStartOffset   = _listStartOffset;                                                         \
        msg->srmpOn            = _srmpOn;                                                                  \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcGetFolderListingResSend
 *
 *  DESCRIPTION
 *      This signal is sent in response to the CSR_BT_MAPC_GET_FOLDER_LISTING_IND 
 *      message from the MSE.
 *
 *  PARAMETERS
 *      mapcInstanceId:       map client instance id
 *      srmpOn:               Temporarily suspends the SRM, if TRUE. It is applicable 
 *                            only if peer device supports SRM.
 *----------------------------------------------------------------------------*/
#define CsrBtMapcGetFolderListingResSend(_mapcInstanceId, _srmpOn) {                                       \
        CsrBtMapcGetFolderListingRes *msg = (CsrBtMapcGetFolderListingRes*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                         = CSR_BT_MAPC_GET_FOLDER_LISTING_RES;                            \
        msg->srmpOn                       = _srmpOn;                                                       \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcGetMessageListingReqSend
 *
 *  DESCRIPTION
 *      This signal retrieves the messages-listing from the MSE of the folder 
 *      specified by folderName parameter or from all the folders if a conversation 
 *      identifier is specified in conversationId
 *
 *      MAP client sends CSR_BT_MAPC_GET_MESSAGE_LISTING_IND messages concluding 
 *      with CSR_BT_MAPC_GET_MESSAGE_LISTING_CFM  message back to the application 
 *      with the result of the operation.
 *
 *    PARAMETERS
 *       mapcInstanceId         Instance id of the map client
 *       folderName:            Null terminated name string of the folder from where 
 *                              the message listing is to be retrieved. The folderName 
 *                              specified shall be one of the child folders of the current 
 *                              folder of MSE. If folderName is set to NULL, the message 
 *                              listing of the current folder is retrieved.
 *       maxListCount:          Maximum number of folders in the listing. To retrieve the 
 *                              number of messages in the folder, maxListCount should be 
 *                              set to zero
 *       listStartOffset:       Offset from where to the listing should start.
 *       maxSubjectLength:      Maximum string length allowed on the subject field on 
 *                              each message.
 *       parameterMask:         Bitmask of relevant parameters for the message listing. 
 *                              NB: a bit value of 1 means that the parameter should be present 
 *                              and a value of 0 means it should be filtered out. The MSE is 
 *                              expected to adhere to parameterMask. The details of parameterMask 
 *                              is defined in MAP specification.
 *       filterMessageType:     Bitmask specifying which message types should be filtered in the listing. 
 *                              NB: a bit value of 1 means that the message type should be filtered and a 
 *                              value of 0 means that it should be present. 
 *       filterPeriodBegin:     Null terminated time string that may be used for filtering the messages 
 *                              by delivery date newer than specified. The MSE is expected to adhere to 
 *                              filterPeriodBegin
 *       filterPeriodEnd:       Null terminated time string that may be used for filtering the messages 
 *                              by delivery date older than specified. The MSE is expected to adhere 
 *                              to filterPeriodEnd
 *       filterReadStatus:      Bitmask specifying if filtering should be done based on read status. 
 *                              The MSE is expected to adhere to filterReadStatus
 *       filterRecipient:       Null terminated recipient string. The MSE filters the message listing 
 *                              based on the filterRecipient in respective vCard attributes. 
 *       filterOriginator:      Null terminated originator string. The MSE filters the message listing 
 *                              based on the filterOriginator in respective vCard attributes.
 *       filterPriority:        Bitmask specifying which priority type to be included in the message listing. 
 *       conversationId:        Conversation ID (optional). If this holds a value, then MessageListing 
 *                              object in the response shall contain all messages with this specific 
 *                              conversation ID from all available folders.
 *                              Note: If conversationId is specified then folderName shall be set to NULL 
 *                              by application and will be ignored by MAPC.
 *       filterMessageHandle    Message Handle. 
 *       srmpOn                 Temporarily suspends the SRM, if TRUE. It is applicable only if peer 
 *                              device supports SRM.
 *----------------------------------------------------------------------------*/
#define CsrBtMapcGetMessageListingReqSend(_mapcInstanceId, _folderName,                                      \
                                          _maxListCount, _listStartOffset, _maxSubjectLength,                \
                                          _parameterMask, _filterMessageType, _filterPeriodBegin,            \
                                          _filterPeriodEnd, _filterReadStatus, _filterRecipient,             \
                                          _filterOriginator, _filterPriority, _conversationId,               \
                                          _filterMessageHandle, _srmpOn) {                                   \
        CsrBtMapcGetMessageListingReq *msg = (CsrBtMapcGetMessageListingReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                   = CSR_BT_MAPC_GET_MESSAGE_LISTING_REQ;                                   \
        msg->folderName             = _folderName;                                                           \
        msg->maxListCount           = _maxListCount;                                                         \
        msg->listStartOffset        = _listStartOffset;                                                      \
        msg->maxSubjectLength       = _maxSubjectLength;                                                     \
        msg->parameterMask          = _parameterMask;                                                        \
        msg->filterMessageType      = _filterMessageType;                                                    \
        msg->filterPeriodBegin      = _filterPeriodBegin;                                                    \
        msg->filterPeriodEnd        = _filterPeriodEnd;                                                      \
        msg->filterReadStatus       = _filterReadStatus;                                                     \
        msg->filterRecipient        = _filterRecipient;                                                      \
        msg->filterOriginator       = _filterOriginator;                                                     \
        msg->filterPriority         = _filterPriority;                                                       \
        msg->conversationId         = _conversationId;                                                       \
        msg->filterMessageHandle    = _filterMessageHandle;                                                  \
        msg->srmpOn                 = _srmpOn;                                                               \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcGetMessageListingResSend
 *
 *  DESCRIPTION
 *      Map client responds to the message listing payloads received from the MSE.
 *      MAP client sends this message in response to CSR_BT_MAPC_GET_MESSAGE_LISTING_IND
 *
 *    PARAMETERS 
 *      mapcInstanceId:       map client instance id
 *      srmpOn:               Temporarily suspends the SRM, if TRUE. It is applicable 
 *                            only if peer device supports SRM.
 *----------------------------------------------------------------------------*/
#define CsrBtMapcGetMessageListingResSend(_mapcInstanceId, _srmpOn) {                                        \
        CsrBtMapcGetMessageListingRes *msg = (CsrBtMapcGetMessageListingRes*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                          = CSR_BT_MAPC_GET_MESSAGE_LISTING_RES;                            \
        msg->srmpOn                        = _srmpOn;                                                        \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      MapcGetMessageReqSend
 *
 *  DESCRIPTION
 *      Retrieves a message specified by messageHandle from MSE
 *
 *      MAP client sends CSR_BT_MAPC_GET_MESSAGE_IND message concluding with 
 *      CSR_BT_MAPC_GET_MESSAGE_CFM message back to the application with the 
 *      result of the operation.
 *
 *  PARAMETERS
 *      mapcInstanceId         instance id of the map client
 *      messageHandle:         Null terminated message handle string that the 
 *                             application is intended to retrieve from MSE
 *      attachment:            Bitmask specifying whether to include attachment 
 *                             of the message or not. The MSE is expected to adhere 
 *                             to attachment parameter.
 *      charset:               Bitmask to specify the desired trans-coding of the 
 *                             message requested. The supported charset is defined in MAP specification.
 *      fractionRequest:       Bitmask to request which fragment of the message to retrieve. 
 *                             This parameter is applicable only if the message in MSE is fractioned
 *      srmpOn                 Temporarily suspends the SRM, if TRUE. It is applicable only if 
 *                             peer device supports SRM.
 *----------------------------------------------------------------------------*/
#define CsrBtMapcGetMessageReqSend(_mapcInstanceId, _messageHandle,                            \
                                   _attachment, _charset,                                      \
                                   _fractionRequest,                                           \
                                   _srmpOn) {                                                  \
        CsrBtMapcGetMessageReq *msg = (CsrBtMapcGetMessageReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                   = CSR_BT_MAPC_GET_MESSAGE_REQ;                             \
        msg->messageHandle          = _messageHandle;                                          \
        msg->attachment             = _attachment;                                             \
        msg->charset                = _charset;                                                \
        msg->fractionRequest        = _fractionRequest;                                        \
        msg->srmpOn                 = _srmpOn;                                                 \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcGetMessageResSend
 *
 *  DESCRIPTION
 *      MAP client sends this response message as a response to the 
 *      CSR_BT_MAPC_GET_MESSAGE_IND message that was received by the application.
 *
 *  PARAMETERS
 *      mapcInstanceId:       map client instance id
 *      srmpOn:               Temporarily suspends the SRM, if TRUE. It is applicable 
 *                            only if peer device supports SRM.
 *----------------------------------------------------------------------------*/
#define CsrBtMapcGetMessageResSend(_mapcInstanceId, _srmpOn) {                                 \
        CsrBtMapcGetMessageRes *msg = (CsrBtMapcGetMessageRes*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                   = CSR_BT_MAPC_GET_MESSAGE_RES;                             \
        msg->srmpOn                 = _srmpOn;                                                 \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcSetMessageStatusReqSend
 *
 *  DESCRIPTION
 *      This signal allows the application to modify the message status on MSE, 
 *      for example, changing the message status from unread to read.
 *
 *      MAP client sends CSR_BT_MAPC_SET_MESSAGE_STATUS_CFM message back
 *      to the application with the result of the operation.
 *
 *  PARAMETERS
 *      mapcInstanceId          instance id of the map client
 *      messageHandle:          Null terminated message handle string that the 
 *                              application is intended to retrieve from MSE
 *      statusIndicator:        status indicator to be set
 *      statusValue:            status value to be set
 *      extendedData            extended data relating to the message
 *----------------------------------------------------------------------------*/
#define CsrBtMapcSetMessageStatusReqSend(_mapcInstanceId, _messageHandle, _statusIndicator, _statusValue, _extendedData) {    \
        CsrBtMapcSetMessageStatusReq *msg = (CsrBtMapcSetMessageStatusReq*) CsrPmemAlloc(sizeof(*msg));                       \
        msg->type                         = CSR_BT_MAPC_SET_MESSAGE_STATUS_REQ;                                               \
        msg->messageHandle                = _messageHandle;                                                                   \
        msg->statusIndicator              = _statusIndicator;                                                                 \
        msg->statusValue                  = _statusValue;                                                                     \
        msg->extendedData                 = _extendedData;                                                                    \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcPushMessageReqSend
 *
 *  DESCRIPTION
 *      Request to Push objects to MSE
 *
 *      MAP client sends CSR_BT_MAPC_PUSH_MESSAGE_CFM message back
 *      to the application with the result of the operation.
 *
 *  PARAMETERS
 *      folderName:          name of the folder where the message should be pushed
 *      lengthOfObject:      total length of the message to send
 *      transparent:         if the MSE should keep a copy of the message in the sent folder
 *      retry:               if the MSE should try to resend if first delivery to the network fails
 *      charset:             format of the content delivered
 *      conversationId       Conversation ID. Used to push a message to a specific conversation. 
 *                           The value of this application parameter should be the conversation ID 
 *                           of the conversation to which the message is added. If this is NULL, 
 *                           it is up to the MSE if a new conversation is created or the message is 
 *                           added to an existing conversation with the same recipients as contained 
 *                           in the bMessage.
 *      messageHandle        Unique handle of the message. This is used in the message forwarding feature
 *                           along with below two arguments (attachment and modifyText), to uniquely identify 
 *                           the message to be forwarded
 *      attachment           whether to include attachment while forwarding the message
 *      modifyText           whether to prepend (0) or replace (1) the message to be forwarded
 *
 *----------------------------------------------------------------------------*/
#define CsrBtMapcPushMessageReqSend(_mapcInstanceId, _folderName,                                \
                                    _lengthOfObject,                                             \
                                    _transparent,                                                \
                                    _retry,                                                      \
                                    _charset,                                                    \
                                    _conversationId,                                             \
                                    _messageHandle,                                              \
                                    _attachment,                                                 \
                                    _modifyText)                                                 \
do                                                                                               \
{                                                                                                \
        CsrBtMapcPushMessageReq *msg = (CsrBtMapcPushMessageReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                    = CSR_BT_MAPC_PUSH_MESSAGE_REQ;                             \
        msg->folderName              = _folderName;                                              \
        msg->lengthOfObject          = _lengthOfObject;                                          \
        msg->transparent             = _transparent;                                             \
        msg->retry                   = _retry;                                                   \
        msg->charset                 = _charset;                                                 \
        msg->conversationId          = _conversationId;                                          \
        msg->messageHandle           = _messageHandle;                                           \
        msg->attachment              = _attachment;                                              \
        msg->modifyText              = _modifyText;                                              \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);                                             \
}while (0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcPushMessageResSend
 *
 *  DESCRIPTION
 *
 *      The MAP client application sends the message payload in response to the 
 *      CSR_BT_MAPC_PUSH_MESSAGE_IND received from the MSE
 *
 *  PARAMETERS
 *      finalFlag:            parameter to indicate it is the final payload
 *      payloadLength:        length of the payload
 *      *payload:             payload 
 *----------------------------------------------------------------------------*/
#define CsrBtMapcPushMessageResSend(_mapcInstanceId, _finalFlag,                                 \
                                    _payloadLength,_payload) {                                   \
        CsrBtMapcPushMessageRes *msg = (CsrBtMapcPushMessageRes*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                    = CSR_BT_MAPC_PUSH_MESSAGE_RES;                             \
        msg->finalFlag               = _finalFlag;                                               \
        msg->payloadLength           = _payloadLength;                                           \
        msg->payload                 = _payload;                                                 \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcUpdateInboxReqSend
 * 
 *  DESCRIPTION
 *
 *      This signal requests the MSE to perform an inbox update. The MSE communicates 
 *      with the network to update the message server inbox.
 *
 *      MAP client sends CSR_BT_MAPC_UPDATE_INBOX_CFM message back
 *      to the application with the result of the operation.
 *
 *  PARAMETERS
 *      mapcInstanceId        instance id of the map client
 *----------------------------------------------------------------------------*/
#define CsrBtMapcUpdateInboxReqSend(_mapcInstanceId) {                                       \
    CsrBtMapcUpdateInboxReq *msg = (CsrBtMapcUpdateInboxReq*) CsrPmemAlloc(sizeof(*msg));    \
    msg->type                    = CSR_BT_MAPC_UPDATE_INBOX_REQ;                             \
    CsrBtMapcMsgTransport(_mapcInstanceId, msg);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcAbortReqSend
 *
 *  DESCRIPTION
 * 
 *      The CSR_BT_MAPC_ABORT is used when the application decides to 
 *      terminate a multi-packet operation (such as GET/PUT) before it normally ends. 
 *
 *      MAP client sends CSR_BT_MAPC_ABORT_CFM message back
 *      to the application with the result of the operation.
 *
 *
 *  PARAMETERS
 *      mapcInstanceId        instance id of the map client
 *----------------------------------------------------------------------------*/
#define CsrBtMapcAbortReqSend(_mapcInstanceId) {                                     \
        CsrBtMapcAbortReq *msg = (CsrBtMapcAbortReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type              = CSR_BT_MAPC_ABORT_REQ;                              \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcNotificationRegistrationReqSend
 *
 *  DESCRIPTION
 *      The application uses this signal to register itself for being notified about the 
 *      arrival of new messages in the MSE or to be indicated by the MSE when the message 
 *      pushed to outbox was sent successfully to the network. If the MSE disconnects the 
 *      notification service without MAPC requesting to disable the notification service, 
 *      the MAPC notifies the application through CSR_BT_MAPC_NOTIFICATION_REGISTRATION_OFF_IND.
 
 *      MAP client sends CSR_BT_MAPC_NOTIFICATION_REGISTRATION_CFM message back
 *      to the application with the result of the operation.
 *
 *    PARAMETERS
 *        mapcInstanceId            map client instance id
 *        enableNotifications:      flag to set notification state
 *----------------------------------------------------------------------------*/
#define CsrBtMapcNotificationRegistrationReqSend(_mapcInstanceId, _enableNotifications) {                                  \
        CsrBtMapcNotificationRegistrationReq *msg = (CsrBtMapcNotificationRegistrationReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                                 = CSR_BT_MAPC_NOTIFICATION_REGISTRATION_REQ;                             \
        msg->enableNotifications                  = _enableNotifications;                                                  \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcEventNotificationResSend
 *
 *  DESCRIPTION
 *      This signal is in response to the CSR_BT_MAPC_EVENT_NOTIFICATION_IND that the  
 *      application receives from the MSE
 *
 *  PARAMETERS
 *      mapcInstanceId  map client instance id
 *      response:       The application can respond with OBEX_CONTINUE_RESPONSE_CODE 
 *                      to continue to receive the complete message or OBEX_SUCCESS_RESPONSE_CODE 
 *                      in the case the message is successfully received from the peer.
 *                      In the case of error, the application responds with an error 
 *                      response code (refer to csr_bt_obex.h)
 *      srmpOn          Temporarily suspends the SRM, if TRUE. It is applicable 
 *                      only if peer device supports SRM.
 *----------------------------------------------------------------------------*/
#define CsrBtMapcEventNotificationResSend(_mapcInstanceId, _response, _srmpOn) {                             \
        CsrBtMapcEventNotificationRes *msg = (CsrBtMapcEventNotificationRes*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                  = CSR_BT_MAPC_EVENT_NOTIFICATION_RES;                                     \
        msg->instanceId            = _mapcInstanceId;                                                        \
        msg->response              = _response;                                                              \
        msg->srmpOn                = _srmpOn;                                                                \
        CsrBtMapcMsgTransport(CSR_BT_MAPC_IFACEQUEUE, msg);}

#ifdef INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcSecurityInReqSend
 *
 *  DESCRIPTION
 *      Set the default security settings for new incoming/outgoing connections
 * 
 *      MAP client sends CSR_BT_MAPC_SECURITY_IN_CFM message back
 *      to the application with the result of the operation.
 *
 *  PARAMETERS
 *       mapcIntanceId   The instance id of the map client instance
 *       appHandle       The application handle which will receive the response
 *       secLevel        The security level to use
 *
 *----------------------------------------------------------------------------*/
#define CsrBtMapcSecurityInReqSend(_mapcInstanceId, _appHandle, _secLevel) {                    \
        CsrBtMapcSecurityInReq  *msg = (CsrBtMapcSecurityInReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type          = CSR_BT_MAPC_SECURITY_IN_REQ;                                       \
        msg->appHandle     = _appHandle;                                                        \
        msg->secLevel      = _secLevel;                                                         \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcSecurityOutReqSend
 *
 *  DESCRIPTION
 *      Set the default security settings for new incoming/outgoing connections
 *
 *      MAP client sends CSR_BT_MAPC_SECURITY_OUT_CFM message back
 *      to the application with the result of the operation.
 *
 *  PARAMETERS
 *       mapcIntanceId   The instance id of the map client instance
 *       appHandle       The application handle which will receive the response
 *       secLevel        The security level to use
 *
 *----------------------------------------------------------------------------*/
#define CsrBtMapcSecurityOutReqSend(_mapcInstanceId, _appHandle, _secLevel) {                    \
        CsrBtMapcSecurityOutReq *msg = (CsrBtMapcSecurityOutReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type          = CSR_BT_MAPC_SECURITY_OUT_REQ;                                       \
        msg->appHandle     = _appHandle;                                                         \
        msg->secLevel      = _secLevel;                                                          \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}
#endif /* INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcGetMasInstanceInformationReqSend
 *
 *  DESCRIPTION
 *      This allows MCE to get additional information about any advertised instance.
 *      The result would be text field containing user-readable information 
 *      about a given MAS Instance.
 * 
 *      MAP client sends CSR_BT_MAPC_GET_MAS_INSTANCE_INFORMATION_CFM message back
 *      to the application with the result of the operation.
 *
 *    PARAMETERS
 *        mapcInatnceid     the instance id of the map instance
 *----------------------------------------------------------------------------*/
#define CsrBtMapcGetMasInstanceInformationReqSend(_mapcInstanceId) {                                                         \
        CsrBtMapcGetMasInstanceInformationReq *msg = (CsrBtMapcGetMasInstanceInformationReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type              = CSR_BT_MAPC_GET_MAS_INSTANCE_INFORMATION_REQ;                                               \
        CsrBtMapcMsgTransport( _mapcInstanceId, msg);}

/********************************************************************************/
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcGetConversationListingReqSend
 *
 *  DESCRIPTION
 *      This signal retrieves the conversation-listing from the MSE.
 * 
 *      MAP client sends a number of CSR_BT_MAPC_GET_CONVERSATION_LISTING_IND messages back
 *      to the application followed by CSR_BT_MAPC_GET_CONVERSATION_LISTING_CFM with the 
 *      result of the operation.
 *
 *  PARAMETERS
 *      mapcInstanceId          instance id of the map client
 *      maxListCount            maximum number of conversations listed in the conversation-
 *                              Listing object to be returned.
 *      listStartOffset         Offset from where the listing should start.
 *      filterLastActivityBegin Filter the conversations that are returned in the Conversation-
 *                              Listing object by LastActivity.
 *      filterLastActivityEnd   Filter the conversations that are returned in the Conversation-
 *                              Listing object by LastActivity.
 *      filterReadStatus        Filter the conversations that are returned in the Conversation-
 *                              Listing object by readstatus
 *      filterRecipient         Filter the conversations that are returned in the Conversation-
 *                              Listing object by conversation-recipient.
 *      conversationId          Filter the messages that are returned in the Conversation-Listing 
 *                              object by a conversationId. If this holds a value, the Conversation- 
 *                              Listing object in the response contains only the conversation with 
 *                              this specific conversationId
 *      convParameterMask       Indicates the parameters contained in the requested Conversation-
 *                              Listing objects.
 *      srmpOn                  Temporarily suspends the SRM, if TRUE. It is applicable only if 
 *                              peer device supports SRM.
 *----------------------------------------------------------------------------*/
#define CsrBtMapcGetConversationListingReqSend(_mapcInstanceId, _maxListCount,                                             \
            _listStartOffset, _filterLastActivityBegin, _filterLastActivityEnd,                                            \
            _filterReadStatus, _filterRecipient, _conversationId,                                                          \
            _convParameterMask, _srmpOn) {                                                                                 \
            CsrBtMapcGetConversationListingReq *msg = (CsrBtMapcGetConversationListingReq*) CsrPmemAlloc(sizeof(*msg));    \
            msg->type                    = CSR_BT_MAPC_GET_CONVERSATION_LISTING_REQ;                                       \
            msg->maxListCount            = _maxListCount;                                                                  \
            msg->listStartOffset         = _listStartOffset;                                                               \
            msg->filterLastActivityBegin = _filterLastActivityBegin;                                                       \
            msg->filterLastActivityEnd   = _filterLastActivityEnd;                                                         \
            msg->filterReadStatus        = _filterReadStatus;                                                              \
            msg->filterRecipient         = _filterRecipient;                                                               \
            msg->conversationId          = _conversationId;                                                                \
            msg->convParameterMask       = _convParameterMask;                                                             \
            msg->srmpOn                  = _srmpOn;                                                                        \
            CsrBtMapcMsgTransport(_mapcInstanceId, msg);}
    
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcGetConversationListingResSend
 *
 *  DESCRIPTION
 *      This signal is sent by the application in response to the conversation-listing 
 *      indication (CSR_BT_MAPC_GET_CONVERSATION_LISTING_IND) that was received 
 *      from the MSE. 
 *
 *  PARAMETERS
 *      mapcInstanceId      map client instance id
 *      srmpOn              flag to set the SRM 
 *----------------------------------------------------------------------------*/
#define CsrBtMapcGetConversationListingResSend(_mapcInstanceId, _srmpOn) {                                                 \
            CsrBtMapcGetConversationListingRes *msg = (CsrBtMapcGetConversationListingRes*) CsrPmemAlloc(sizeof(*msg));    \
            msg->type                               = CSR_BT_MAPC_GET_CONVERSATION_LISTING_RES;                            \
            msg->srmpOn                             = _srmpOn;                                                             \
            CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcGetOwnerStatusReqSend
 *
 *  DESCRIPTION
 *
 *      This signal allows application to request the Presence, Chat State, or Last Activity 
 *      of the owner on the MSE.
 * 
 *      MAP client sends CSR_BT_MAPC_GET_OWNER_STATUS_CFM message back
 *      to the application with the result of the operation.
 *
 *
 *  PARAMETERS
 *      mapcInstanceId          instance if of the client
 *      conversationId          conversation id
 *----------------------------------------------------------------------------*/
#define CsrBtMapcGetOwnerStatusReqSend(_mapcInstanceId, _conversationId) {                             \
        CsrBtMapcGetOwnerStatusReq *msg = (CsrBtMapcGetOwnerStatusReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                       = CSR_BT_MAPC_GET_OWNER_STATUS_REQ;                            \
        msg->conversationId             = _conversationId;                                             \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcSetOwnerStatusReqSend
 *
 *  DESCRIPTION
 * 
 *      This signal allows application to change the Presence, Chat State, or Last Activity 
 *      of the owner on the MSE.
 *
 *      MAP client sends CSR_BT_MAPC_SET_OWNER_STATUS_CFM message back
 *      to the application with the result of the operation.
 *
 *
 *  PARAMETERS
 *      mapcInstanceId          instance if of the client
 *      presenceAvailability    presence availability state
 *      presenceText            presence text state
 *      lastActivity            last activity state
 *      chatState               chat state
 *      conversationId          conversation id
 *----------------------------------------------------------------------------*/
#define CsrBtMapcSetOwnerStatusReqSend(_mapcInstanceId,                                                \
                                        _presenceAvailability,                                         \
                                        _presenceText,                                                 \
                                        _lastActivity,                                                 \
                                        _chatState,                                                    \
                                        _conversationId) {                                             \
        CsrBtMapcSetOwnerStatusReq *msg = (CsrBtMapcSetOwnerStatusReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                       = CSR_BT_MAPC_SET_OWNER_STATUS_REQ  ;                          \
        msg->presenceAvailability       = _presenceAvailability;                                       \
        msg->presenceText               = _presenceText;                                               \
        msg->lastActivity               = _lastActivity;                                               \
        msg->chatState                  = _chatState;                                                  \
        msg->conversationId             = _conversationId;                                             \
        CsrBtMapcMsgTransport(_mapcInstanceId, msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcSetNotificationFilterReqSend
 *
 *  DESCRIPTION
 *
 *      If the remote MSE supports the Messages Notification and Notification Filtering 
 *      features, application can use this request to specify which notifications to receive 
 *      from the MSE. If this request is not used, then all events are sent as received by 
 *      the application when notification registration is ON.
 *
 *      MAP client sends CSR_BT_MAPC_SET_NOTIFICATION_FILTER_CFM message back
 *      to the application with the result of the operation.
 *
 *
 *  PARAMETERS
 *      mapcInstanceId      instance id of the client
 *      notiFilterMask      notification filter mask to set. Indicates which 
 *                          notifications the applications wants from the MSE 
 *                          when notifications are turned “on” or “off”.
 *----------------------------------------------------------------------------*/
#define CsrBtMapcSetNotificationFilterReqSend(_mapcInstanceId, _notiFilterMask) {                                    \
        CsrBtMapcSetNotificationFilterReq *msg = (CsrBtMapcSetNotificationFilterReq*) CsrPmemAlloc(sizeof(*msg));    \
        msg->type                              = CSR_BT_MAPC_SET_NOTIFICATION_FILTER_REQ;                            \
        msg->notiFilterMask                    = _notiFilterMask;                                                    \
        CsrBtMapcMsgTransport( _mapcInstanceId, msg);}

/********************************************************************************/

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtMapcFreeUpstreamMessageContents
 *
 *  DESCRIPTION
 *      During Bluetooth shutdown all allocated payload in the Synergy BT MAPC
 *      message must be deallocated. This is done by this function
 * 
 *  PARAMETERS
 *      eventClass :  Must be CSR_BT_MAPC_PRIM,
 *      msg:          The message received from Synergy BT MAPC
 *----------------------------------------------------------------------------*/
void CsrBtMapcFreeUpstreamMessageContents(CsrUint16 eventClass, void * message);


#ifdef __cplusplus
}
#endif

#endif
