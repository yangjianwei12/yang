/******************************************************************************
 Copyright (c) 2021-23 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 DESCRIPTION
     Header file providing mapping for Synergy PBAP Client profile's public
     interfaces. Refer to csr_bt_pac_lib.h for APIs descriptions.
******************************************************************************/

#ifndef COMMON_SYNERGY_INC_PAC_LIB_H_
#define COMMON_SYNERGY_INC_PAC_LIB_H_

#include "synergy.h"
#include "csr_bt_pac_lib.h"

#define PAC_PRIM             (SYNERGY_EVENT_BASE + CSR_BT_PAC_PRIM)

#define PacGetInstanceIdsRequestSend(_task)                      \
    PacGetInstanceIdsReqSend(TrapToOxygenTask(_task))

#define PacRegisterRequestSend(_instanceId,                      \
                               _task)                            \
    PacRegisterReqSend(_instanceId,                              \
                       TrapToOxygenTask(_task))

#define PacConnectRequestSend(_instanceId,                       \
                              _task,                             \
                              _maxPacketSize,                    \
                              _destination,                      \
                              _windowSize)                       \
    PacConnectReqSendExt(_instanceId,                            \
                         TrapToOxygenTask(_task),                \
                         _maxPacketSize,                         \
                         _destination,                           \
                         _windowSize)

#define PacCancelConnectRequestSend(_instanceId)                 \
    PacCancelConnectReqSendExt(_instanceId)

#define PacDisconnectRequestSend(_instanceId,                    \
                                 _normal_disconnect)             \
    PacDisconnectReqSendExt(_instanceId,                         \
                            _normal_disconnect)

#define PacSetFolderRequestSend(_instanceId,                     \
                                _folder)                         \
    PacSetFolderReqSendExt(_instanceId,                          \
                           _folder)

#define PacSetBackFolderRequestSend(_instanceId)                 \
    PacSetBackFolderReqSendExt(_instanceId)

#define PacSetRootFolderRequestSend(_instanceId)                 \
    PacSetRootFolderReqSendExt(_instanceId)

#define PacAuthenticateResponseSend(_instanceId,                 \
                                    _password,                   \
                                    _passwordLength,             \
                                    _userId)                     \
    PacAuthenticateResSendExt(_instanceId,                       \
                              _password,                         \
                              _passwordLength,                   \
                              _userId)

#define PacPullPbRequestSend(_instanceId,                        \
                             _ucs2name,                          \
                             _src,                               \
                             _filter,                            \
                             _format,                            \
                             _maxLstCnt,                         \
                             _listStartOffset,                   \
                             _resetNewMissedCalls,               \
                             _vCardSelector,                     \
                             _vCardSelectorOperator,             \
                             _srmpOn)                            \
    PacPullPbReqSendExt(_instanceId,                             \
                        _ucs2name,                               \
                        _src,                                    \
                        _filter,                                 \
                        _format,                                 \
                        _maxLstCnt,                              \
                        _listStartOffset,                        \
                        _resetNewMissedCalls,                    \
                        _vCardSelector,                          \
                        _vCardSelectorOperator,                  \
                        _srmpOn)

#define PacPullPbResponseSend(_instanceId,                       \
                              _srmpOn)                           \
    PacPullPbResSendExt(_instanceId,                             \
                       _srmpOn)

#define PacPullVcardListRequestSend(_instanceId,                 \
                                    _ucs2name,                   \
                                    _order,                      \
                                    _searchVal,                  \
                                    _searchAtt,                  \
                                    _maxListCnt,                 \
                                    _listStartOffset,            \
                                    _resetNewMissedCalls,        \
                                    _vCardSelector,              \
                                    _vCardSelectorOperator,      \
                                    _srmpOn)                     \
    PacPullVcardListReqSendExt(_instanceId,                      \
                               _ucs2name,                        \
                               _order,                           \
                               _searchVal,                       \
                               _searchAtt,                       \
                               _maxListCnt,                      \
                               _listStartOffset,                 \
                               _resetNewMissedCalls,             \
                               _vCardSelector,                   \
                               _vCardSelectorOperator,           \
                               _srmpOn)

#define PacPullVcardListResponseSend(_instanceId,                \
                                     _srmpOn)                    \
    PacPullVcardListResSendExt(_instanceId,                      \
                               _srmpOn)

#define PacPullVcardEntryRequestSend(_instanceId,                \
                                 _ucs2name,                      \
                                 _filter,                        \
                                 _format,                        \
                                 _srmpOn)                        \
    PacPullVcardEntryReqSendExt(_instanceId,                     \
                                _ucs2name,                       \
                                _filter,                         \
                                _format,                         \
                                _srmpOn)

#define PacPullVcardEntryResponseSend(_instanceId,               \
                                 _srmpOn)                        \
    PacPullVcardEntryResSendExt(_instanceId,                     \
                                _srmpOn)

#define PacAbortRequestSend(_instanceId)                         \
    PacAbortReqSendExt(_instanceId)

#ifdef INSTALL_PAC_CUSTOM_SECURITY_SETTINGS
#define PacSecurityOutRequestSend(_instanceId,                   \
                                  _appHandle,                    \
                                  _secLevel)                     \
    PacSecurityOutReqSendExt(_instanceId,                        \
                             _appHandle,                         \
                             _secLevel)
#endif

/*********** Deprecated APIs: To be removed in future ***********/

 /* Use PacConnectRequestSend() instead */
#define PacConnectReqSend(_task,                                 \
                          _maxPacketSize,                        \
                          _destination,                          \
                          _windowSize)                           \
    CsrBtPacConnectReqSend(TrapToOxygenTask(_task),              \
                           _maxPacketSize,                       \
                           _destination,                         \
                           _windowSize)

/* Use PacDisconnectRequestSend() instead */
#define PacDisconnectReqSend(_normal_disconnect)                 \
    CsrBtPacDisconnectReqSend(_normal_disconnect)

/* Use PacSetFolderRequestSend() instead */
#define PacSetFolderReqSend(_folder)                             \
    CsrBtPacSetFolderReqSend(_folder)

/* Use PacAuthenticateResponseSend() instead */
#define PacAuthenticateResSend(_password,                        \
                               _passwordLength,                  \
                               _userId)                          \
    CsrBtPacAuthenticateResSend(_password,                       \
                                _passwordLength,                 \
                                _userId)

/* Use PacPullPbRequestSend() instead */
#define PacPullPbReqSend(_ucs2name,                              \
                         _src,                                   \
                         _filter,                                \
                         _format,                                \
                         _maxLstCnt,                             \
                         _listStartOffset,                       \
                         _resetNewMissedCalls,                   \
                         _vCardSelector,                         \
                         _vCardSelectorOperator,                 \
                         _srmpOn)                                \
    CsrBtPacPullPbReqSend(_ucs2name,                             \
                          _src,                                  \
                          _filter,                               \
                          _format,                               \
                          _maxLstCnt,                            \
                         _listStartOffset,                       \
                         _resetNewMissedCalls,                   \
                         _vCardSelector,                         \
                         _vCardSelectorOperator,                 \
                         _srmpOn)

/* Use PacPullVcardListRequestSend() instead */
#define PacPullVcardListReqSend(_ucs2name,                       \
                                _order,                          \
                                _searchVal,                      \
                                _searchAtt,                      \
                                _maxListCnt,                     \
                                _listStartOffset,                \
                                _resetNewMissedCalls,            \
                                _vCardSelector,                  \
                                _vCardSelectorOperator,          \
                                _srmpOn)                         \
    CsrBtPacPullVcardListReqSend(_ucs2name,                      \
                                 _order,                         \
                                 _searchVal,                     \
                                 _searchAtt,                     \
                                 _maxListCnt,                    \
                                 _listStartOffset,               \
                                 _resetNewMissedCalls,           \
                                 _vCardSelector,                 \
                                 _vCardSelectorOperator,         \
                                 _srmpOn)

/* Use PacPullVcardListResponseSend() instead */
#define PacPullVcardListResSend(_srmpOn)                         \
    CsrBtPacPullVcardListResSend(_srmpOn)

/* Use PacPullVcardEntryRequestSend() instead */
#define PacPullVcardEntryReqSend(_ucs2name,                      \
                                 _filter,                        \
                                 _format,                        \
                                 _srmpOn)                        \
    CsrBtPacPullVcardEntryReqSend(_ucs2name,                     \
                                  _filter,                       \
                                  _format,                       \
                                  _srmpOn)

/* Use PacPullVcardEntryResponseSend() instead */
#define PacPullVcardEntryResSend(_srmpOn)                        \
    CsrBtPacPullVcardEntryResSend(_srmpOn)

/********************** Deprecated APIs: END ********************/

#define PacFreeUpstreamMessageContents(_msg)                     \
    CsrBtPacFreeUpstreamMessageContents(CSR_BT_PAC_PRIM, _msg)

#endif /* COMMON_SYNERGY_INC_PAC_LIB_H_ */
