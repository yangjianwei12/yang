/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 DESCRIPTION
     Header file providing mapping for Synergy SPP profile's public interfaces.
     Refer to csr_bt_spp_lib.h for APIs descriptions.

 REVISION:      $Revision: #59 $
******************************************************************************/

#ifndef COMMON_SYNERGY_INC_SPP_LIB_H_
#define COMMON_SYNERGY_INC_SPP_LIB_H_

#include "synergy.h"
#include "csr_bt_spp_lib.h"
#include "csr_bt_util.h"

#define SPP_PRIM             (SYNERGY_EVENT_BASE + CSR_BT_SPP_PRIM)

#define SppGetInstancesQidReqSend(_task)                                    \
    CsrBtSppGetInstancesQidReqSend(TrapToOxygenTask(_task))

#define SppActivateReqSend(_queueId,                                        \
                           _task,                                           \
                           _theTimeout,                                     \
                           _role,                                           \
                           _serviceName)                                    \
    CsrBtSppActivateReqSend(_queueId,                                       \
                            TrapToOxygenTask(_task),                        \
                            _theTimeout,                                    \
                            _role,                                          \
                            _serviceName)

#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
#define SppActivateReqSendExt(_queueId,                                     \
                              _task,                                        \
                              _theTimeout,                                  \
                              _serviceRecord,                               \
                              _serviceRecordSize,                           \
                              _serverChannelIndex,                          \
                              _secLevel,                                    \
                              _cod)                                         \
    CsrBtSppActivateReqSendExt(_queueId,                                    \
                               TrapToOxygenTask(_task),                     \
                               _theTimeout,                                 \
                               _serviceRecord,                              \
                               _serviceRecordSize,                          \
                               _serverChannelIndex,                         \
                               _secLevel,                                   \
                               _cod)

#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */

#define SppDeactivateReqSend(_queueId,                                      \
                             _task)                                         \
    CsrBtSppDeactivateReqSend(_queueId,                                     \
                              TrapToOxygenTask(_task))          

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
#define SppConnectReqSend(_queueId,                                         \
                          _task,                                            \
                          _deviceAddr,                                      \
                          _requestPortPar,                                  \
                          _portPar,                                         \
                          _role)                                            \
    CsrBtSppConnectReqSend(_queueId,                                        \
                           TrapToOxygenTask(_task),                         \
                           _deviceAddr,                                     \
                           _requestPortPar,                                 \
                           _portPar,                                        \
                           _role)

#define SppCancelConnectReqSend(_queueId)                                   \
    CsrBtSppCancelConnectReqSend(_queueId)

#define SppServiceNameResSend(_queueId,                                     \
                              _connect,                                     \
                              _theServiceHandle)                            \
    CsrBtSppServiceNameResSend(_queueId,                                    \
                               _connect,                                    \
                               _theServiceHandle)
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

#define SppDisconnectReqSend(_queueId,                                      \
                             _theServerChannel)                             \
    CsrBtSppDisconnectReqSend(_queueId,                                     \
                              _theServerChannel)

#ifdef INSTALL_SPP_REMOTE_PORT_NEGOTIATION
#define SppPortnegReqSend(_queueId,                                         \
                          _serverChannel,                                   \
                          _portPar)                                         \
    CsrBtSppPortnegReqSend(_queueId,                                        \
                           _serverChannel,                                  \
                           _portPar)
#endif /* INSTALL_SPP_REMOTE_PORT_NEGOTIATION */

#define SppPortnegResSend(_queueId,                                         \
                          _serverChannel,                                   \
                          _portPar)                                         \
    CsrBtSppPortnegResSend(_queueId,                                        \
                           _serverChannel,                                  \
                           _portPar)

#ifdef INSTALL_SPP_MODEM_STATUS_COMMAND
#define SppControlReqSend(_queueId,                                         \
                          _theServerChannel,                                \
                          _theModemStatus,                                  \
                          _theBreakSignal)                                  \
    CsrBtSppControlReqSend(_queueId,                                        \
                           _theServerChannel,                               \
                           _theModemStatus,                                 \
                           _theBreakSignal)
#endif /* INSTALL_SPP_MODEM_STATUS_COMMAND */

#define SppFreeUpstreamMessageContents(message)                             \
    CsrBtSppFreeUpstreamMessageContents(CSR_BT_SPP_PRIM, message)

#endif /* COMMON_SYNERGY_INC_SPP_LIB_H_ */

