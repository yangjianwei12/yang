/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 DESCRIPTION
     Header file providing mapping for Synergy MAP Client profile's public 
     interfaces. Refer to csr_bt_mapc_lib.h for APIs descriptions.
******************************************************************************/

#ifndef COMMON_SYNERGY_INC_MAPC_LIB_H_
#define COMMON_SYNERGY_INC_MAPC_LIB_H_

#include "synergy.h"
#include "csr_bt_mapc_lib.h"

#define MAPC_PRIM             (SYNERGY_EVENT_BASE + CSR_BT_MAPC_PRIM)

#define MapcGetInstanceIdsReqSend(_task)                                    \
        CsrBtMapcGetInstanceIdsReqSend(TrapToOxygenTask(_task))

#define MapcConnectReqSend(_mapcInstanceId,                                 \
                            _task,                                          \
                            _maxPacketSize,                                 \
                            _deviceAddr,                                    \
                            _windowSize)                                    \
    CsrBtMapcConnectReqSend(_mapcInstanceId,                                \
                            TrapToOxygenTask(_task),                        \
                           _maxPacketSize,                                  \
                           _deviceAddr,                                     \
                           _windowSize)


#define MapcCancelConnectReqSend(_mapcInstanceId)                           \
        CsrBtMapcCancelConnectReqSend(_mapcInstanceId) 


#define MapcDisconnectReqSend(_mapcInstanceId,                              \
                            _normalObexDisconnect)                          \
        CsrBtMapcDisconnectReqSend(_mapcInstanceId, _normalObexDisconnect)

#define MapcSelectMasInstanceResSend(_mapcInstanceId,                       \
                                    _proceedWithConnection,                 \
                                    _masInstanceId)                         \
        CsrBtMapcSelectMasInstanceResSend(_mapcInstanceId,                  \
                                        _proceedWithConnection,             \
                                        _masInstanceId) 

#define MapcSetFolderReqSend(_mapcInstanceId,                               \
                            _folderName)                                    \
        CsrBtMapcSetFolderReqSend(_mapcInstanceId, _folderName) 

#define MapcSetBackFolderReqSend(_mapcInstanceId)                           \
        CsrBtMapcSetBackFolderReqSend(_mapcInstanceId) 

#define MapcSetRootFolderReqSend(_mapcInstanceId)                           \
        CsrBtMapcSetRootFolderReqSend(_mapcInstanceId) 


#define MapcGetFolderListingReqSend(_mapcInstanceId,                        \
                                    _maxListCount,                          \
                                    _listStartOffset,                       \
                                    _srmpOn)                                \
        CsrBtMapcGetFolderListingReqSend(_mapcInstanceId,                   \
                                         _maxListCount,                     \
                                        _listStartOffset,                   \
                                        _srmpOn)



#define MapcGetFolderListingResSend(_mapcInstanceId,                        \
                                    _srmpOn)                                \
        CsrBtMapcGetFolderListingResSend(_mapcInstanceId, _srmpOn)

#define MapcGetMessageListingReqSend(_mapcInstanceId, _folderName,          \
                    _maxListCount, _listStartOffset, _maxSubjectLength,     \
                    _parameterMask, _filterMessageType, _filterPeriodBegin, \
                    _filterPeriodEnd, _filterReadStatus, _filterRecipient,  \
                    _filterOriginator, _filterPriority, _conversationId,    \
                    _filterMessageHandle, _srmpOn)                          \
        CsrBtMapcGetMessageListingReqSend(_mapcInstanceId,_folderName,      \
                        _maxListCount, _listStartOffset,                    \
                        _maxSubjectLength,_parameterMask,                   \
                        _filterMessageType, _filterPeriodBegin,             \
                        _filterPeriodEnd, _filterReadStatus,                \
                        _filterRecipient, _filterOriginator,                \
                        _filterPriority, _conversationId,                   \
                        _filterMessageHandle, _srmpOn) 


#define MapcGetMessageListingResSend(_mapcInstanceId, _srmpOn)              \
        CsrBtMapcGetMessageListingResSend(_mapcInstanceId, _srmpOn)

#define MapcGetMessageReqSend(_mapcInstanceId, _messageHandle,              \
                                _attachment,                                \
                                _charset,                                   \
                                _fractionRequest,                           \
                                _srmpOn)                                    \
        CsrBtMapcGetMessageReqSend(_mapcInstanceId, _messageHandle,         \
                                _attachment,                                \
                                _charset,                                   \
                                _fractionRequest,                           \
                                _srmpOn) 
        
#define MapcGetMessageResSend(_mapcInstanceId, _srmpOn)                     \
        CsrBtMapcGetMessageResSend(_mapcInstanceId, _srmpOn)


#define MapcSetMessageStatusReqSend(_mapcInstanceId, _messageHandle,        \
                                _statusIndicator,                           \
                                _statusValue,                               \
                                _extendedData)                              \
        CsrBtMapcSetMessageStatusReqSend(_mapcInstanceId,                   \
                                _messageHandle,                             \
                                _statusIndicator,                           \
                                _statusValue,                               \
                                _extendedData) 

#define MapcPushMessageReqSend(_mapcInstanceId,                             \
                                _folderName,                                \
                                _lengthOfObject,                            \
                                _transparent,                               \
                                _retry,                                     \
                                _charset,                                   \
                                _conversationId,                            \
                                _messageHandle,                             \
                                _attachment,                                \
                                _modifyText)                                \
        CsrBtMapcPushMessageReqSend(_mapcInstanceId,                        \
                                    _folderName,                            \
                                _lengthOfObject,                            \
                                _transparent,                               \
                                _retry,                                     \
                                _charset,                                   \
                                _conversationId,                            \
                                _messageHandle,                             \
                                _attachment,                                \
                                _modifyText)

#define MapcPushMessageResSend(_mapcInstanceId,                             \
                                _finalFlag,                                 \
                                _payloadLength,                             \
                                _payload)                                   \
        CsrBtMapcPushMessageResSend(_mapcInstanceId, _finalFlag,            \
                                _payloadLength,                             \
                                _payload) 


#define MapcUpdateInboxReqSend(_mapcInstanceId)                             \
        CsrBtMapcUpdateInboxReqSend(_mapcInstanceId) 


#define MapcAbortReqSend(_mapcInstanceId)                                   \
        CsrBtMapcAbortReqSend(_mapcInstanceId) 


#define MapcNotificationRegistrationReqSend(_mapcInstanceId,                \
                                        _enableNotifications)               \
        CsrBtMapcNotificationRegistrationReqSend(_mapcInstanceId,           \
                                        _enableNotifications) 

#define MapcEventNotificationResSend(_mapcInstanceId, _response,            \
                                    _srmpOn)                                \
        CsrBtMapcEventNotificationResSend(_mapcInstanceId,                  \
                                            _response,                      \
                                            _srmpOn) 


#define MapcSecurityInReqSend(_mapcInstanceId, _appHandle,                  \
                              _secLevel)                                    \
        CsrBtMapcSecurityInReqSend(_mapcInstanceId, _appHandle,             \
                              _secLevel) 


#define MapcSecurityOutReqSend(_mapcInstanceId, _appHandle,                 \
                               _secLevel)                                   \
        CsrBtMapcSecurityOutReqSend(_mapcInstanceId, _appHandle,            \
                                _secLevel)

#define MapcGetMasInstanceInformationReqSend(_mapcInstanceId)               \
        CsrBtMapcGetMasInstanceInformationReqSend(_mapcInstanceId) 


#define MapcGetConversationListingReqSend(_mapcInstanceId,                  \
                                          _maxListCount,                    \
                                          _listStartOffset,                 \
                                          _filterLastActivityBegin,         \
                                          _filterLastActivityEnd,           \
                                          _filterReadStatus,                \
                                          _filterRecipient,                 \
                                          _conversationId,                  \
                                          _convParameterMask,               \
                                          _srmpOn)                          \
        CsrBtMapcGetConversationListingReqSend(_mapcInstanceId,             \
                                          _maxListCount,                    \
                                          _listStartOffset,                 \
                                          _filterLastActivityBegin,         \
                                          _filterLastActivityEnd,           \
                                          _filterReadStatus,                \
                                          _filterRecipient,                 \
                                          _conversationId,                  \
                                          _convParameterMask,               \
                                          _srmpOn) 



#define MapcGetConversationListingResSend(_mapcInstanceId, _srmpOn)         \
        CsrBtMapcGetConversationListingResSend(_mapcInstanceId, _srmpOn) 


#define MapcGetOwnerStatusReqSend(_mapcInstanceId, _conversationId)         \
        CsrBtMapcGetOwnerStatusReqSend(_mapcInstanceId, _conversationId) 


#define MapcSetOwnerStatusReqSend(_mapcInstanceId,                          \
                                    _presenceAvailability,                  \
                                    _presenceText,                          \
                                    _lastActivity,                          \
                                    _chatState,                             \
                                    _conversationId)                        \
        CsrBtMapcSetOwnerStatusReqSend(_mapcInstanceId,                     \
                                    _presenceAvailability,                  \
                                    _presenceText,                          \
                                    _lastActivity,                          \
                                    _chatState,                             \
                                    _conversationId) 


#define MapcSetNotificationFilterReqSend(_mapcInstanceId, _notiFilterMask)  \
    CsrBtMapcSetNotificationFilterReqSend(_mapcInstanceId, _notiFilterMask) 

#define MapcFreeUpstreamMessageContents(_msg)                               \
    CsrBtMapcFreeUpstreamMessageContents(CSR_BT_MAPC_PRIM, _msg)


#endif /* COMMON_SYNERGY_INC_MAPC_LIB_H_ */
