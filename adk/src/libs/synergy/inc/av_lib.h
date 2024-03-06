/******************************************************************************
 Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 DESCRIPTION
     Header file providing mapping for Synergy AV profile's public interfaces.
     Refer to csr_bt_av_lib.h for APIs descriptions.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef COMMON_SYNERGY_INC_AV_LIB_H_
#define COMMON_SYNERGY_INC_AV_LIB_H_

#include "synergy.h"
#include "csr_bt_av_lib.h"

#define AV_PRIM             (SYNERGY_EVENT_BASE + CSR_BT_AV_PRIM)

#define AvActivateReqSend(_task,                                            \
                          _role)                                            \
    CsrBtAvActivateReqSend(TrapToOxygenTask(_task),                         \
                           _role)

#define AvConnectReqSend(_task,                                             \
                         _deviceAddr,                                       \
                         _remoteRole)                                       \
    CsrBtAvConnectReqSend(TrapToOxygenTask(_task),                          \
                         _deviceAddr,                                       \
                         _remoteRole)

#define AvDisconnectReqSend(_connectionId)                                  \
    CsrBtAvDisconnectReqSend(_connectionId)

#define AvDiscoverResAcpSend(_connectionId,                                 \
                             _tLabel,                                       \
                             _seidInfoCount,                                \
                             _seidInfo)                                     \
    CsrBtAvDiscoverResAcpSend(_connectionId,                                \
                             _tLabel,                                       \
                             _seidInfoCount,                                \
                             _seidInfo)

#define AvDiscoverResRejSend(_connectionId,                                 \
                             _tLabel,                                       \
                             _avResponse)                                   \
    CsrBtAvDiscoverResRejSend(_connectionId,                                \
                              _tLabel,                                      \
                              _avResponse)

#define AvGetCapabilitiesResAcpSend(_connectionId,                          \
                                    _tLabel,                                \
                                    _servCapLen,                            \
                                    _servCapData)                           \
    CsrBtAvGetCapabilitiesResAcpSend(_connectionId,                         \
                                    _tLabel,                                \
                                    _servCapLen,                            \
                                    _servCapData)

#define AvGetAllCapabilitiesResAcpSend(_connectionId,                       \
                                       _tLabel,                             \
                                       _servCapLen,                         \
                                       _servCapData)                        \
    CsrBtAvGetAllCapabilitiesResAcpSend(_connectionId,                      \
                                        _tLabel,                            \
                                        _servCapLen,                        \
                                        _servCapData)

#define AvGetCapabilitiesResRejSend(_connectionId,                          \
                                    _tLabel,                                \
                                    _avResponse)                            \
    CsrBtAvGetCapabilitiesResRejSend(_connectionId,                         \
                                    _tLabel,                                \
                                    _avResponse)

#define AvGetAllCapabilitiesResRejSend(_connectionId,                       \
                                       _tLabel,                             \
                                       _avResponse)                         \
    CsrBtAvGetAllCapabilitiesResRejSend(_connectionId,                      \
                                        _tLabel,                            \
                                        _avResponse)

#define AvSetConfigResAcpSend(_shandle,                                     \
                              _tLabel)                                      \
    CsrBtAvSetConfigResAcpSend(_shandle,                                    \
                               _tLabel)

#define AvSetConfigResRejSend(_shandle,                                     \
                              _tLabel,                                      \
                              _avResponse,                                  \
                              _servCategory)                                \
    CsrBtAvSetConfigResRejSend(_shandle,                                    \
                               _tLabel,                                     \
                               _avResponse,                                 \
                               _servCategory)

#define AvReconfigReqSend(_shandle,                                         \
                          _tLabel,                                          \
                          _servCapLen,                                      \
                          _servCapData)                                     \
        CsrBtAvReconfigReqSend(_shandle,                                    \
                               _tLabel,                                     \
                               _servCapLen,                                 \
                               _servCapData)

#define AvReconfigResAcpSend(_shandle,                                      \
                             _tLabel)                                       \
    CsrBtAvReconfigResAcpSend(_shandle,                                     \
                              _tLabel)

#define AvReconfigResRejSend(_shandle,                                      \
                             _tLabel,                                       \
                             _avResponse,                                   \
                             _servCategory)                                 \
    CsrBtAvReconfigResRejSend(_shandle,                                     \
                              _tLabel,                                      \
                              _avResponse,                                  \
                              _servCategory)

#define AvValidateServiceCap(_capPtr)                                       \
    CsrBtAvValidateServiceCap(_capPtr)

#define AvGetServiceCap(_cap,                                               \
                        _list,                                              \
                        _length,                                            \
                        _index)                                             \
    CsrBtAvGetServiceCap(_cap,                                              \
                        _list,                                              \
                        _length,                                            \
                        _index)

#define AvDiscoverReqSend(_connectionId,                                    \
                          _tLabel)                                          \
    CsrBtAvDiscoverReqSend(_connectionId,                                   \
                          _tLabel)

#define AvGetCapabilitiesReqSend(_connectionId,                             \
                                 _acpSeid,                                  \
                                 _tLabel)                                   \
    CsrBtAvGetCapabilitiesReqSend(_connectionId,                            \
                                  _acpSeid,                                 \
                                  _tLabel)

#define AvSetConfigReqSend(_connectionId,                                   \
                           _tLabel,                                         \
                           _acpSeid,                                        \
                           _intSeid,                                        \
                           _appServCapLen,                                  \
                           _appServCapData)                                 \
    CsrBtAvSetConfigReqSend(_connectionId,                                  \
                           _tLabel,                                         \
                           _acpSeid,                                        \
                           _intSeid,                                        \
                           _appServCapLen,                                  \
                           _appServCapData)

#define AvGetConfigResAcpSend(_shandle,                                     \
                              _tLabel,                                      \
                              _servCapLen,                                  \
                              _servCapData)                                 \
        CsrBtAvGetConfigResAcpSend(_shandle,                                \
                                   _tLabel,                                 \
                                   _servCapLen,                             \
                                   _servCapData)

#define AvGetConfigResRejSend(_shandle,                                     \
                              _tLabel,                                      \
                              _avResponse)                                  \
        CsrBtAvGetConfigResRejSend(_shandle,                                \
                                   _tLabel,                                 \
                                   _avResponse)

#define AvAbortReqSend(_shandle,                                            \
                       _tLabel)                                             \
        CsrBtAvAbortReqSend(_shandle,                                       \
                            _tLabel)

#define AvAbortResSend(_shandle,                                            \
                       _tLabel)                                             \
        CsrBtAvAbortResSend(_shandle,                                       \
                            _tLabel)

#define AvSetStreamInfoReqSend(_sH,                                         \
                               _sI)                                         \
    CsrBtAvSetStreamInfoReqSend(_sH,                                        \
                               _sI)

#define AvOpenReqSend(_shandle,                                             \
                      _tLabel)                                              \
    CsrBtAvOpenReqSend(_shandle,                                            \
                      _tLabel)

#define AvOpenResRejSend(_shandle,                                          \
                         _tLabel,                                           \
                         _avResponse)                                       \
    CsrBtAvOpenResRejSend(_shandle,                                         \
                          _tLabel,                                          \
                          _avResponse)

#define AvOpenResAcpSend(_shandle,                                          \
                         _tLabel)                                           \
    CsrBtAvOpenResAcpSend(_shandle,                                         \
                          _tLabel)

#define AvCloseReqSend(_shandle,                                            \
                       _tLabel)                                             \
    CsrBtAvCloseReqSend(_shandle,                                           \
                        _tLabel)

#define AvCloseResAcpSend(_shandle,                                         \
                          _tLabel)                                          \
    CsrBtAvCloseResAcpSend(_shandle,                                        \
                           _tLabel)

#define AvCloseResRejSend(_shandle,                                         \
                          _tLabel,                                          \
                          _avResponse)                                      \
    CsrBtAvCloseResRejSend(_shandle,                                        \
                           _tLabel,                                         \
                           _avResponse)

#define AvStartReqSend(_tLabel,                                             \
                       _list,                                               \
                       _listLength)                                         \
    CsrBtAvStartReqSend(_listLength,                                        \
                        _tLabel,                                            \
                        _list)

#define AvStartResAcpSend(_tLabel,                                          \
                          _list,                                            \
                          _listLength)                                      \
    CsrBtAvStartResAcpSend(_tLabel,                                         \
                           _listLength,                                     \
                           _list)

#define AvStartResRejSend(_reject_shandle,                                  \
                          _tLabel,                                          \
                          _avResponse,                                      \
                          _list,                                            \
                          _listLength)                                      \
    CsrBtAvStartResRejSend(_reject_shandle,                                 \
                           _tLabel,                                         \
                           _avResponse,                                     \
                           _listLength,                                     \
                           _list)

#define AvSuspendReqSend(_tLabel,                                           \
                         _list,                                             \
                         _listLength)                                       \
    CsrBtAvSuspendReqSend(_listLength,                                      \
                          _tLabel,                                          \
                          _list)

#define AvSuspendResAcpSend(_tLabel,                                        \
                            _listLength,                                    \
                            _list)                                          \
    CsrBtAvSuspendResAcpSend(_tLabel,                                       \
                             _listLength,                                   \
                             _list)

#define AvSuspendResRejSend(_reject_shandle,                                \
                            _tLabel,                                        \
                            _avResponse,                                    \
                            _listLength,                                    \
                            _list)                                          \
    CsrBtAvSuspendResRejSend(_reject_shandle,                               \
                             _tLabel,                                       \
                             _avResponse,                                   \
                             _listLength,                                   \
                             _list)

#define AvDelayReportReqSend(_delay,                                        \
                             _shandle,                                      \
                             _tLabel)                                       \
    CsrBtAvDelayReportReqSend(_delay,                                       \
                              _shandle,                                     \
                              _tLabel)

#define AvDelayReportResAcpSend(_shandle,                                   \
                                _tLabel)                                    \
    CsrBtAvDelayReportResAcpSend(_shandle,                                  \
                                _tLabel)

#define AvDelayReportResRejSend(_shandle,                                   \
                                _tLabel,                                    \
                                _avResponse)                                \
    CsrBtAvDelayReportResRejSend(_shandle,                                  \
                                 _tLabel,                                   \
                                 _avResponse)

#define AvFreeUpstreamMessageContents(message)                              \
    CsrBtAvFreeUpstreamMessageContents(CSR_BT_AV_PRIM, message)

#define AvGetConId(addr, deviceId)                                          \
    CsrBtAvGetConId(addr, deviceId)

#define AvUseLargeMtu(connectionId, useLargeMtu)                            \
    CsrBtAvUseLargeMtu(connectionId, useLargeMtu)

#endif /* COMMON_SYNERGY_INC_AV_LIB_H_ */

