/******************************************************************************
 Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 DESCRIPTION
    Header file providing mapping for synergy CM's public interfaces.
    For API descriptions, refer to files:
    csr_bt_cm_lib.h
    csr_bt_cm_private_lib.h
    csr_bt_cm_dm_sc_lib.h
    csr_bt_cm_dm_sc_ssp_lib.h

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef COMMON_SYNERGY_INC_CM_LIB_H_
#define COMMON_SYNERGY_INC_CM_LIB_H_

#include "synergy.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_dm_sc_ssp_lib.h"


#define CM_PRIM             (SYNERGY_EVENT_BASE + CSR_BT_CM_PRIM)


/*! \name Bluestack Key Value Pair macros

    The Bluestack Key / Value table for L2CAP configuration is constructed from
    an array of uint16 values. These macros help format values to uint16.
*/


/*! \{ */
/*!
    @brief Returns a uint16 of 'value'.
*/
#define BKV_UINT16V(value)  ((uint16)((value)&0xFFFF))
/*!
    @brief Get the high uint16 of a uint32 bit value
*/
#define BKV_UINT32H(value)  ((uint16)(((uint32)(value) >> 16) & 0xFFFF))
/*!
    @brief Get the low uint16 of a uint32 bit value
*/
#define BKV_UINT32L(value)  BKV_UINT16V(value)
/*!
    @brief Turn a uint32 value into 2, comma separated uint16 values in
    big-endian order (high uint16, low uint16).
*/
#define BKV_UINT32V(value)    BKV_UINT32H(value),BKV_UINT32L(value)
/*!
    @brief Specify a uint16 range in the order most preferred value, least
    preferred value, results in two comma separated uint16 values.
 */
#define BKV_UINT16R(most_preferred,least_preferred) \
    BKV_UINT16V(least_preferred), BKV_UINT16V(most_preferred)
/*!
    @brief Specify a uint32 range in the order most preferred value, least
    preferred value, results in four comma separated uint16 values.
*/
#define BKV_UINT32R(most_preferred,least_preferred) \
    BKV_UINT32V(least_preferred),BKV_UINT32V(most_preferred)
/*!
    @brief Macro to combine Flow and Error Control Mode and the Mode fallback
    mask into a single uint16 value
*/
#define BKV_16_FLOW_MODE(mode,fallback_mask) \
    ((((mode)<<8)&0xFF00)|(fallback_mask))
/*! \} */


/*! \name Bluestack Keys for the L2CAP Configuration Table (conftab)
    These are Key / Value pairs.

    Advanced L2CAP configuration can be performed using a Key / Value table of
    uint16 values. This configuration table is used by Bluestack to negotiate
    the L2CAP parameters with a peer automatically.

    Note 1 - QOS_SERVICE: This uint16_t is encoded as two uint8_ts's:
    The MSO defines the required service type using the L2CA_QOS_TYPE
    defines. Specifying guaranteed mode or no traffic mode requires peer
    to support QoS. If LSO is zero and the service type is best effort,
    the option may be ignored completely. If non-zero and best effort
    the option will always be sent.

    Note 2 - FS_SERVICE: Same encoding as for QOS_SERVICE above, except
    that this key will use the extended flow specification instead of
    the old QoS. Also note that LSO *may* be ignored if both parties
    claim support for flowspecs and are using non-basic mode.

    Note 3 - FLOW_MODE: This uint16_t is encoded as two uint8_t's. The
    MSO defines the preferred mode (using the exact L2CA_FLOW_MODE_*
    value). LSO sets the allowed backoff modes in case the peer doesn't
    support the preferred one. This is encoded as a bitmask using the
    (L2CA_MODE_MASK_* flags). The algorithm described in CSA1 will be
    used for backing off. The old FEC modes can only be used by
    selecting them in the MSO.

    Note 4 - FCS: Three values can be used: 0: Don't care. 1: Require
    FCS and 2: Avoid if possible. Note that if one side requires FCS,
    both parties have to use it, so a value of 2 doesn't guarantee that
    FCS is disabled.

    Note that the conftab size is the number of uint16 entries, not the
    size returned by sizeof. Use CONFTAB_LEN to portably get the length
    of a constant structure.
*/
/*! \{ */
/*!
    @brief This value indicates the start of a configuration data set in the
    key value pairs
*/
#define L2CAP_AUTOPT_SEPARATOR           (0x8000)
/*!
    @brief 16 bit, exact - incoming MTU
*/
#define L2CAP_AUTOPT_MTU_IN               ((uint16)0x0001)
/*!
    @brief 16 bit, minimum - peer MTU
*/
#define L2CAP_AUTOPT_MTU_OUT              ((uint16)0x0102)
/*!
    @brief 32 bit, range - peer flush (us)  - note that HCI limit still applies
*/
#define L2CAP_AUTOPT_FLUSH_IN             ((uint16)0x0703)
/*!
    @brief 32 bit, range - local flush (us) - note that HCI limit still applies
*/
#define L2CAP_AUTOPT_FLUSH_OUT            ((uint16)0x0704)
/*!
    @brief 16 bit, exact - shared service type (note1)
*/
#define L2CAP_AUTOPT_QOS_SERVICE          ((uint16)0x0005)
/*!
    @brief 32 bit, range - incoming token rate/flowspec interarrival
*/
#define L2CAP_AUTOPT_QOS_RATE_IN          ((uint16)0x0706)
/*!
    @brief 32 bit, range - outgoing token rate/flowspec interarrival
*/
#define L2CAP_AUTOPT_QOS_RATE_OUT         ((uint16)0x0707)
/*!
    @brief 32 bit, range - incoming token bucket
*/
#define L2CAP_AUTOPT_QOS_BUCKET_IN        ((uint16)0x0708)
/*!
    @brief 32 bit, range - outgoing token bucket
*/
#define L2CAP_AUTOPT_QOS_BUCKET_OUT       ((uint16)0x0709)
/*!
    @brief 32 bit, range - incoming peak bandwidth
*/
#define L2CAP_AUTOPT_QOS_PEAK_IN          ((uint16)0x070a)
/*!
    @brief 32 bit, range - outgoing peak bandwidth
*/
#define L2CAP_AUTOPT_QOS_PEAK_OUT         ((uint16)0x070b)
/*!
    @brief 32 bit, range - incoming qos/flowspec access latency
*/
#define L2CAP_AUTOPT_QOS_LATENCY_IN       ((uint16)0x070c)
/*!
    @brief 32 bit, range - outgoing qos/flowspec access latency
*/
#define L2CAP_AUTOPT_QOS_LATENCY_OUT      ((uint16)0x070d)
/*!
    @brief 32 bit, range - incoming delay variation
*/
#define L2CAP_AUTOPT_QOS_DELAY_IN         ((uint16)0x070e)
/*!
    @brief 32 bit, range - outgoing delay variation
*/
#define L2CAP_AUTOPT_QOS_DELAY_OUT        ((uint16)0x070f)
/*!
    @brief 16 bit, range - incoming max SDU size
*/
#define L2CAP_AUTOPT_FS_SDU_SIZE_IN       ((uint16)0x0310)
/*!
    @brief 16 bit, range - incoming max SDU size
*/
#define L2CAP_AUTOPT_FS_SDU_SIZE_OUT      ((uint16)0x0311)
/*!
    @brief 16 bit, exact - shared flow control mode (note3)
*/
#define L2CAP_AUTOPT_FLOW_MODE            ((uint16)0x0012)
/*!
    @brief 16 bit, range - incoming window size
*/
#define L2CAP_AUTOPT_FLOW_WINDOW_IN       ((uint16)0x0313)
/*!
    @brief 16 bit, range - peer window size
*/
#define L2CAP_AUTOPT_FLOW_WINDOW_OUT      ((uint16)0x0314)
/*!
    @brief 16 bit, range - peer maximum retransmit
*/
#define L2CAP_AUTOPT_FLOW_MAX_RETX_IN     ((uint16)0x0315)
/*!
    @brief 16 bit, range - local maximum retransmit
*/
#define L2CAP_AUTOPT_FLOW_MAX_RETX_OUT    ((uint16)0x0316)
/*!
    @brief 16 bit, range - incoming max PDU payload size
*/
#define L2CAP_AUTOPT_FLOW_MAX_PDU_IN      ((uint16)0x0317)
/*!
    @brief 16 bit, range - outgoing maximum PDU size
*/
#define L2CAP_AUTOPT_FLOW_MAX_PDU_OUT     ((uint16)0x0318)
/*!
    @brief 16 bit, exact - use FCS or not (note4)
*/
#define L2CAP_AUTOPT_FCS                  ((uint16)0x0019)
/*!
    @brief 16 bit, exact - shared flowspec service type (note2)
*/
#define L2CAP_AUTOPT_FS_SERVICE           ((uint16)0x001A)

/*!
    @brief 32 bit, exact - cached getinfo ext.feats
*/
#define L2CAP_AUTOPT_EXT_FEATS            ((uint16)0x0420)
/*!
    @brief 16 bit, exact - ward off reconfiguration attempts
*/
#define L2CAP_AUTOPT_DISABLE_RECONF       ((uint16)0x0021)

/*!
....@brief 16-bit, exact - intial credits given to the remote device while creating connection
*/
#define L2CAP_AUTOPT_CREDITS              ((uint16)0x0025)
/*!
....@brief 16-bit, exact - l2ca_conflags_t parameter indicating the connection options to bring up the link.
*/
#define L2CAP_AUTOPT_CONN_FLAGS           ((uint16)0x0026)
/*!
    *brief Indicates the end of a configuration table (conftab)
*/
#define L2CAP_AUTOPT_TERMINATOR           (0xFF00)

/*! \} */

/*! Helper macro to get the length of a static conftab array */

#define CONFTAB_LEN(conftab_) (sizeof(conftab_)/sizeof(uint16))

/*! \name L2CAP Flow & error control modes

  Basic Mode, Enhanced Retransmission Mode and Streaming Mode are supported.
  Retransmission Mode and Flow Control Mode are NOT supported.
*/

/*! \{  */
/*!
    @brief L2CAP Basic Mode
*/
#define FLOW_MODE_BASIC             0x00
/*!
    @brief L2CAP Enhanced Retransmission Mode
*/
#define FLOW_MODE_ENHANCED_RETRANS  0x03
/*!
    @brief L2CAP Streaming Mode
*/
#define FLOW_MODE_STREAMING         0x04
/*! \} */


/* \name Mode Fallback Masks

  Combining the mode masks, allows configuration of other mode to Fallback to
  if the Flow and Error Control mode selected is not supported by a peer
  (Note 3).

  Fallback mode precedence is Basic, then Enhanced Retransmission and then
  Streaming.
*/
/*! \{*/
#define MODE_MASK(mode)             (1<<(mode))
/*!
    @brief L2CAP No Fallback Mode Mask
*/
#define MODE_MASK_NONE              0
/*!
    @brief L2CAP Basic Mode Mask
*/
#define MODE_MASK_BASIC             MODE_MASK(FLOW_MODE_BASIC)
/*!
    @brief L2CAP Enhanced Retransmission Mode Mask
*/
#define MODE_MASK_ENHANCED_RETRANS  MODE_MASK(FLOW_MODE_ENHANCED_RETRANS)
/*!
    @brief L2CAP Streaming Mode Mask
*/
#define MODE_MASK_STREAMING         MODE_MASK(FLOW_MODE_STREAMING)
/*! \} */


/*! \{ \brief L2CAP flush timeout (32-bit) - Infinite Flush Timeout
*/
#define DEFAULT_L2CAP_FLUSH_TIMEOUT 0xFFFFFFFFu
/*! \} */


/* Local state */
#define CmReadLocalNameReqSend(_task)                                       \
    CsrBtCmReadLocalNameReqSend(TrapToOxygenTask(_task))

#define CmReadRemoteVersionReqSend(_task, _addr, _addrType, _transportType) \
    CsrBtCmReadRemoteVersionReqSend(TrapToOxygenTask(_task), _addr,         \
                                    _addrType, _transportType)

#define CmSetLocalNameReqSend(_task, _friendlyName)                         \
    CsrBtCmSetLocalNameReqSend(TrapToOxygenTask(_task), _friendlyName)

#define CmReadLocalBdAddrReqSend(_task)                                     \
    CsrBtCmReadLocalBdAddrReqSend(TrapToOxygenTask(_task))

#define CmSetEirDataReqSend(_task, _fec, _length, _data)                    \
    CsrBtCmSetEirDataReqSend(TrapToOxygenTask(_task), _fec, _length, _data)

#define CmWritePageScanTypeReqSend(_task,_scanType)                         \
    CsrBtCmWritePageScanTypeReqSend(TrapToOxygenTask(_task),_scanType)

#define CmWriteInquiryScanTypeReqSend(_task,_scanType)                      \
    CsrBtCmWriteInquiryScanTypeReqSend(TrapToOxygenTask(_task),_scanType)

#define CmWriteScanEnableReqSend(_task,                                     \
                                 _disableInquiryScan,                       \
                                 _disablePageScan)                          \
    CsrBtCmWriteScanEnableReqSend(TrapToOxygenTask(_task),                  \
                                  _disableInquiryScan,                      \
                                  _disablePageScan)

#define CmWritePageScanSettingsReqSend(_task, _interval, _window)           \
    CsrBtCmWritePageScanSettingsReqSend(TrapToOxygenTask(_task),            \
                                        _interval,                          \
                                        _window)

#define CmWriteInquiryScanSettingsReqSend(_task, _interval, _window)        \
    CsrBtCmWriteInquiryScanSettingsReqSend(TrapToOxygenTask(_task),         \
                                           _interval,                       \
                                           _window)

#define CmWriteIacReqSend(_task, _iac)                                      \
    CsrBtCmWriteIacReqSend(TrapToOxygenTask(_task), _iac)

#define CmWriteCodReqSend(_task, _service, _major, _minor)                  \
    CsrBtCmWriteCodReqSend(TrapToOxygenTask(_task), _service, _major, _minor)

#define CmSetEventMaskReqSend(_task, _eventMask, _conditionMask)            \
    CsrBtCmSetEventMaskReqSend(TrapToOxygenTask(_task),                     \
                               _eventMask,                                  \
                               _conditionMask)

/* ACL connection/disconnection */
#define CmAclOpenReqSend(_task, _deviceAddr, _flags)                        \
    CsrBtCmAclOpenReqSend(TrapToOxygenTask(_task),                          \
                          _deviceAddr,                                      \
                          _flags)

#define CmAclDetachReqSend(_task, _addrType, _addr, _flags)                 \
    CsrBtCmAclDetachReqSend(TrapToOxygenTask(_task), _addrType, _addr, _flags)

#define CmAclCloseReqSend(_task, _deviceAddr, _flags, _reason)              \
    CsrBtCmAclCloseReqSend(TrapToOxygenTask(_task),                         \
                           _deviceAddr,                                     \
                           _flags,                                          \
                           _reason)


/* SDP server records */
#define CmSdsRegisterReqSend(_task,                                         \
                             _serviceRecordSize,                            \
                             _serviceRecord,                                \
                             _context)                                      \
    CsrBtCmSdsRegisterReqSend(TrapToOxygenTask(_task),                      \
                              _serviceRecord,                               \
                              _serviceRecordSize,                           \
                              _context)

#define CmSdsUnRegisterReqSend(_task, _serviceRecHandle, _context)          \
    CsrBtCmSdsUnRegisterReqSend(TrapToOxygenTask(_task),                    \
                                _serviceRecHandle,                          \
                                _context)


/* L2CAP connection */
#define CmL2caRegisterReqSend(_task,                                        \
                              _localPsm,                                    \
                              _mode_mask,                                   \
                              _flags,                                       \
                              _context,                                     \
                              _optionsMask)                                 \
    CsrBtCmContextl2caRegisterExtReqSend(TrapToOxygenTask(_task),           \
                                         _localPsm,                         \
                                         _mode_mask,                        \
                                         _flags,                            \
                                         _context,                          \
                                         _optionsMask)

#define CmL2caConnectReqConftabSend(_task,                                  \
                                    _deviceAddr,                            \
                                    _localPsm,                              \
                                    _remotePsm,                             \
                                    _secLevel,                              \
                                    _context,                               \
                                    _conftabCount,                          \
                                    _conftab,                               \
                                    _minKeySize)                            \
    CsrBtCmL2caConnectReqConftabSend(TrapToOxygenTask(_task),               \
                                     _deviceAddr,                           \
                                     _localPsm,                             \
                                     _remotePsm,                            \
                                     _secLevel,                             \
                                     _context,                              \
                                     _conftabCount,                         \
                                     _conftab,                              \
                                     _minKeySize)

#define CmL2caTpConnectRequest(_task,                                       \
                                    _tpdAddrT,                              \
                                    _localPsm,                              \
                                    _remotePsm,                             \
                                    _frameSize,                             \
                                    _secLevel,                              \
                                    _minEncKeySize,                         \
                                    _credits,                               \
                                    _flags)                                 \
    CmL2caTpConnectReqSend(TrapToOxygenTask(_task),                         \
                                    _tpdAddrT,                              \
                                    _localPsm,                              \
                                    _remotePsm,                             \
                                    _frameSize,                             \
                                    _secLevel,                              \
                                    _minEncKeySize,                         \
                                    _credits,                               \
                                    _flags)

#define Cml2caAddCreditRequest(_task,                                       \
                                    _btConnId,                              \
                                    _context,                               \
                                    _credits)                               \
    Cml2caAddCreditReqSend(TrapToOxygenTask(_task),                         \
                                    _btConnId,                              \
                                    _context,                               \
                                    _credits)

#define CmL2caTpConnectAcceptResponse(_task,                                \
                                        _accept,                            \
                                        _btConnId,                          \
                                        _localPsm,                          \
                                        _tpdAddrT,                          \
                                        _identifier,                        \
                                        _frameSize,                         \
                                        _transmitMtu,                       \
                                        _minEncKeySize,                     \
                                        _credits,                           \
                                        _flags)                             \
    CmL2caTpConnectAcceptRspSend(TrapToOxygenTask(_task),                   \
                                        _accept,                            \
                                        _btConnId,                          \
                                        _localPsm,                          \
                                        _tpdAddrT,                          \
                                        _identifier,                        \
                                        _frameSize,                         \
                                        _transmitMtu,                       \
                                        _minEncKeySize,                     \
                                        _credits,                           \
                                        _flags)                             \

#define CmL2caCancelConnectReqSend(_task,                                   \
                                   _deviceAddr,                             \
                                   _localPsm)                               \
    CsrBtCml2caCancelConnectReqSend(TrapToOxygenTask(_task),                \
                                    _deviceAddr,                            \
                                    _localPsm)

#define CmL2caConnectAcceptReqConftabSend(_task,                            \
                                          _localPsm,                        \
                                          _classOfDevice,                   \
                                          _secLevel,                        \
                                          _profileUuid,                     \
                                          _primaryAcceptor,                 \
                                          _context,                         \
                                          _conftabCount,                    \
                                          _conftab,                         \
                                          _devAddr,                         \
                                          _minKeySize)                      \
    CsrBtCmL2caConnectAcceptReqConftabSend(TrapToOxygenTask(_task),         \
                                           _localPsm,                       \
                                           _classOfDevice,                  \
                                           _secLevel,                       \
                                           _profileUuid,                    \
                                           _primaryAcceptor,                \
                                           _context,                        \
                                           _conftabCount,                   \
                                           _conftab,                        \
                                           _devAddr,                        \
                                           _minKeySize)

#define CmL2caDisconnectReqSend(_btConnId, _context)                        \
    CsrBtCmContextl2caDisconnectReqSend(_btConnId, _context)


/* RFCOMM connection */
#define CmRegisterReqSend(_task,                                            \
                         _context,                                          \
                         _serverChannel,                                    \
                         _optionsMask)                                      \
    CsrBtCmPublicRegisterExtReqSend(TrapToOxygenTask(_task),                \
                                    _context,                               \
                                    _serverChannel,                         \
                                    _optionsMask)

#define CmUnRegisterReqSend(_serverChannel)                                 \
    CsrBtCmUnRegisterReqSend(_serverChannel)

#define CmContextConnectAcceptReqSend(_task,                                \
                                      _classOfDevice,                       \
                                      _timeout,                             \
                                      _profileMaxFrameSize,                 \
                                      _serverChannel,                       \
                                      _secLevel,                            \
                                      _profileUuid,                         \
                                      _context,                             \
                                      _modemStatus,                         \
                                      _breakSignal,                         \
                                      _mscTimeout,                          \
                                      _devAddr,                             \
                                      _minEncKeySize)                       \
    CsrBtCmContextConnectAcceptReqSend(TrapToOxygenTask(_task),             \
                                       _classOfDevice,                      \
                                       _timeout,                            \
                                       _profileMaxFrameSize,                \
                                       _serverChannel,                      \
                                       _secLevel,                           \
                                       _profileUuid,                        \
                                       _context,                            \
                                       _modemStatus,                        \
                                       _breakSignal,                        \
                                       _mscTimeout,                         \
                                       _devAddr,                            \
                                       _minEncKeySize)

#define CmContextCancelAcceptConnectReqSend(_task,                          \
                                            _serverChannel,                 \
                                            _context)                       \
    CsrBtCmContextCancelAcceptConnectReqSend(TrapToOxygenTask(_task),       \
                                             _serverChannel,                \
                                             _context)

#define CmContextConnectReqSend(_task,                                      \
                                _localServerCh,                             \
                                _serviceHandle,                             \
                                _profileMaxFrameSize,                       \
                                _requestPortPar,                            \
                                _validportPar,                              \
                                _portPar,                                   \
                                _secLevel,                                  \
                                _deviceAddr,                                \
                                _context,                                   \
                                _modemStatus,                               \
                                _breakSignal,                               \
                                _mscTimeout,                                \
                                _minEncKeySize)                             \
    CsrBtCmContextConnectReqSend(TrapToOxygenTask(_task),                   \
                                 _localServerCh,                            \
                                 _serviceHandle,                            \
                                 _profileMaxFrameSize,                      \
                                 _requestPortPar,                           \
                                 _validportPar,                             \
                                 _portPar,                                  \
                                 _secLevel,                                 \
                                 _deviceAddr,                               \
                                 _context,                                  \
                                 _modemStatus,                              \
                                 _breakSignal,                              \
                                 _mscTimeout,                               \
                                 _minEncKeySize)

#define CmContextl2caCancelConnectAcceptReqSend(_task, _localPsm, _context) \
    CsrBtCmContextl2caCancelConnectAcceptReqSend(TrapToOxygenTask(_task),   \
                                                 _localPsm,                 \
                                                 _context)

#define CmConnectExtReqSend(_task,                                          \
                            _localServerChannel,                            \
                            _remoteServerChannel,                           \
                            _profileMaxFrameSize,                           \
                            _requestPortPar,                                \
                            _validportPar,                                  \
                            _portPar,                                       \
                            _secLevel,                                      \
                            _deviceAddr,                                    \
                            _modemStatus,                                   \
                            _breakSignal,                                   \
                            _mscTimeout,                                    \
                            _minEncKeySize)                                 \
    CsrBtCmConnectExtReqSend(TrapToOxygenTask(_task),                       \
                             _localServerChannel,                           \
                             _remoteServerChannel,                          \
                             _profileMaxFrameSize,                          \
                             _requestPortPar,                               \
                             _validportPar,                                 \
                             _portPar,                                      \
                             _secLevel,                                     \
                             _deviceAddr,                                   \
                             _modemStatus,                                  \
                             _breakSignal,                                  \
                             _mscTimeout,                                   \
                             _minEncKeySize)

#define CmContextDisconnectReqSend(_btConnId, _context)                     \
    CsrBtCmContextDisconnectReqSend(_btConnId, _context)


/* Link policy */
#define CmSwitchRoleReqSend(_task, _deviceAddr, _role)                      \
    CsrBtCmSwitchRoleReqSend(TrapToOxygenTask(_task),                       \
                             _deviceAddr,                                   \
                             _role,                                         \
                             CSR_BT_CM_SWITCH_ROLE_TYPE_ONESHOT,            \
                             0)

#define CmAlwaysMasterDevicesReqSend(_task, _deviceAddr, _operation)        \
        CsrBtCmAlwaysMasterDevicesReqSend(TrapToOxygenTask(_task),          \
                                          _deviceAddr,                      \
                                          _operation)

#define CmRoleDiscoveryReqSend(_task, _deviceAddr)                          \
    CsrBtCmRoleDiscoveryReqSend(TrapToOxygenTask(_task), _deviceAddr)

#define CmWriteLinkSuperVisionTimeoutReqSend(_task, _deviceAddr, _timeout)  \
    CsrBtcmWriteLinkSuperVisionTimeoutReqSend(TrapToOxygenTask(_task),      \
                                              _deviceAddr,                  \
                                              _timeout)

#define CmReadRssiReqSend(_task, _transportType, _addressType, _deviceAddr) \
    CsrBtCmReadRssiReqSend(TrapToOxygenTask(_task),                         \
                           _deviceAddr,                                     \
                           _addressType,                                    \
                           _transportType)

#define CmGetLinkQualityReqSend(_task, _deviceAddr)                         \
    CsrBtCmGetLinkQualityReqSend(TrapToOxygenTask(_task), _deviceAddr)

#define CmWritePageToReqSend(_task, _pageTimeout)                           \
    CsrBtCmWritePageToReqSend(TrapToOxygenTask(_task), _pageTimeout)

#define CmDmPowerSettingsReqSend(_addr, _powerTableSize, _powerTable)       \
    CsrBtCmDmPowerSettingsReqSend(_addr, _powerTableSize, _powerTable)

#define CmWriteLinkPolicyReqSend(_task,                                     \
                                 _deviceAddr,                               \
                                 _linkPolicySetting,                        \
                                 _setupLinkPolicySetting,                   \
                                 _sniffSettings)                            \
    CsrBtCmWriteLinkPolicyReqSend(TrapToOxygenTask(_task),                  \
                                  _deviceAddr,                              \
                                  _linkPolicySetting,                       \
                                  _setupLinkPolicySetting,                  \
                                  _sniffSettings)


/* LE */
#define CmLeSetOwnAddressTypeReqSend(_task, _addressType)                   \
    CsrBtCmLeSetOwnAddressTypeReqSend(TrapToOxygenTask(_task), _addressType)

#define CmLeReadRandomAddressReqSend(_task, _idAddress, _flag)              \
    CsrBtCmLeReadRandomAddressReqSend(TrapToOxygenTask(_task),              \
                                      _idAddress,                           \
                                      _flag)

#define CmReadAdvertisingChTxPowerReqSend(_task, _context)                  \
    CsrBtCmReadAdvertisingChTxPowerReqSend(TrapToOxygenTask(_task),_context)

#define CmLeSetPvtAddrTimeoutReqSend(_task, _timeOut)                       \
    CsrBtCmLeSetPvtAddrTimeoutReqSend(TrapToOxygenTask(_task), _timeOut)

#define CmLeSetPrivacyModeReqSend(_task, _addr, _privacyMode)               \
    CsrBtCmLeSetPrivacyModeReqSend(TrapToOxygenTask(_task),                 \
                                   _addr,                                   \
                                   _privacyMode)

#define CmLeScanReqSend(_task,                                              \
                        _context,                                           \
                        _mode,                                              \
                        _scanType,                                          \
                        _scanInterval,                                      \
                        _scanWindow,                                        \
                        _scanningFilterPolicy,                              \
                        _filterDuplicates,                                  \
                        _addressCount,                                      \
                        _addressList)                                       \
    CsrBtCmLeScanReqSend(TrapToOxygenTask(_task),                           \
                         _context,                                          \
                        _mode,                                              \
                        _scanType,                                          \
                        _scanInterval,                                      \
                        _scanWindow,                                        \
                        _scanningFilterPolicy,                              \
                        _filterDuplicates,                                  \
                        _addressCount,                                      \
                        _addressList)

#define CmLeScanReqOffSend(_task, _context)                                 \
    CsrBtCmLeScanReqOffSend(TrapToOxygenTask(_task), _context)

#define CmRegisterHandlerReqSend(_handlerType,_task,_flags)                 \
    CsrBtCmRegisterHandlerReqSend(_handlerType,                             \
                                  TrapToOxygenTask(_task),                  \
                                  _flags)

#define CmLeAdvertiseReqStartSend(_task,                                    \
                                  _context,                                 \
                                  _mode,                                    \
                                  _paramChange,                             \
                                  _advertisingDataLength,                   \
                                  _advertisingData,                         \
                                  _scanResponseDataLength,                  \
                                  _scanResponseData,                        \
                                  _advertIntervalMin,                       \
                                  _advertIntervalMax,                       \
                                  _advertisingType,                         \
                                  _advertisingChannelMap,                   \
                                  _advertisingFilterPolicy,                 \
                                  _whitelistAddrCount,                      \
                                  _whitelistAddrList)                       \
    CsrBtCmLeAdvertiseReqStartSend(TrapToOxygenTask(_task),                 \
                                   _context,                                \
                                  _mode,                                    \
                                  _paramChange,                             \
                                  _advertisingDataLength,                   \
                                  _advertisingData,                         \
                                  _scanResponseDataLength,                  \
                                  _scanResponseData,                        \
                                  _advertIntervalMin,                       \
                                  _advertIntervalMax,                       \
                                  _advertisingType,                         \
                                  _advertisingChannelMap,                   \
                                  _advertisingFilterPolicy,                 \
                                  _whitelistAddrCount,                      \
                                  _whitelistAddrList)

#define CmLeAdvertiseReqStopSend(_task, _context)                           \
    CsrBtCmLeAdvertiseReqStopSend(TrapToOxygenTask(_task), _context)

#define CmUpdateInternalPeerAddrReqSend(_newPeerDeviceAddr,                 \
                                        _oldPeerDeviceAddr)                 \
    CsrBtCmUpdateInternalPeerAddrReqSend(_newPeerDeviceAddr,                \
                                         _oldPeerDeviceAddr)

#define CmUpdateScoHandle(_scoHandle)                                       \
    CsrBtCmUpdateScoHandleRequest(_scoHandle)

#define CmRegisterHandlerReqSend(_handlerType, _task, _flags)               \
    CsrBtCmRegisterHandlerReqSend(_handlerType,                             \
                                  TrapToOxygenTask(_task),                  \
                                  _flags)

/* Security */
#define ScActivateReqSend(_task)                                            \
    CsrBtScActivateReqSend(TrapToOxygenTask(_task))

#define CmScDmBondingReq(_deviceAddr)   CsrBtCmScDmBondingReq(_deviceAddr, 0)

#define CmSmLeSecurityReqSend(_addr,                                        \
                              _l2caConFlags,                                \
                              _context,                                     \
                              _securityRequirements)                        \
    CsrBtCmSmLeSecurityReqSend(_addr,                                       \
                               _l2caConFlags,                               \
                               _context,                                    \
                               _securityRequirements)

#define CmDatabaseReqSend(_task,                                            \
                          _addressType,                                     \
                          _deviceAddr,                                      \
                          _opcode,                                          \
                          _keyType,                                         \
                          _key)                                             \
    CsrBtCmDatabaseReqSend(TrapToOxygenTask(_task),                         \
                           _addressType,                                    \
                           _deviceAddr,                                     \
                           _opcode,                                         \
                           _keyType,                                        \
                           _key)

#define CmReadBredrKeysReqSend(_task,                                       \
                               _addressType,                                \
                               _deviceAddr)                                 \
    CmDatabaseReqSend(_task,                                                \
                      _addressType,                                         \
                      _deviceAddr,                                          \
                      CSR_BT_CM_DB_OP_READ,                                 \
                      CSR_BT_CM_KEY_TYPE_BREDR,                             \
                      NULL)

#define CmReadLeKeysReqSend(_task,                                          \
                            _addressType,                                   \
                            _deviceAddr)                                    \
    CmDatabaseReqSend(_task,                                                \
                      _addressType,                                         \
                      _deviceAddr,                                          \
                      CSR_BT_CM_DB_OP_READ,                                 \
                      CSR_BT_CM_KEY_TYPE_LE,                                \
                      NULL)

#define CmWriteBredrKeysReqSend(_task,                                      \
                                _addressType,                               \
                                _deviceAddr,                                \
                                _keys)                                      \
    CmDatabaseReqSend(_task,                                                \
                      _addressType,                                         \
                      _deviceAddr,                                          \
                      CSR_BT_CM_DB_OP_WRITE,                                \
                      CSR_BT_CM_KEY_TYPE_BREDR,                             \
                      _keys)

#define CmWriteLeKeysReqSend(_task,                                         \
                             _addressType,                                  \
                             _deviceAddr,                                   \
                             _keys)                                         \
    CmDatabaseReqSend(_task,                                                \
                      _addressType,                                         \
                      _deviceAddr,                                          \
                      CSR_BT_CM_DB_OP_WRITE,                                \
                      CSR_BT_CM_KEY_TYPE_LE,                                \
                      _keys)

#define CmLeConnparamUpdateReqSend(_task,                                   \
                                   _address,                                \
                                   _connIntervalMin,                        \
                                   _connIntervalMax,                        \
                                   _connLatency,                            \
                                   _supervisionTimeout,                     \
                                   _minimumCeLength,                        \
                                   _maximumCeLength)                        \
    CsrBtCmLeConnparamUpdateReqSend(TrapToOxygenTask(_task),                \
                                    _address,                               \
                                    _connIntervalMin,                       \
                                    _connIntervalMax,                       \
                                    _connLatency,                           \
                                    _supervisionTimeout,                    \
                                    _minimumCeLength,                       \
                                    _maximumCeLength)

#define CmLeConnparamReqSend(_task,                                         \
                             _scanInterval,                                 \
                             _scanWindow,                                   \
                             _connIntervalMin,                              \
                             _connIntervalMax,                              \
                             _connLatency,                                  \
                             _supervisionTimeout,                           \
                             _connLatencyMax,                               \
                             _supervisionTimeoutMin,                        \
                             _supervisionTimeoutMax)                        \
    CsrBtCmLeConnparamReqSend(TrapToOxygenTask(_task),                      \
                             _scanInterval,                                 \
                             _scanWindow,                                   \
                             _connIntervalMin,                              \
                             _connIntervalMax,                              \
                             _connLatency,                                  \
                             _supervisionTimeout,                           \
                             _connLatencyMax,                               \
                             _supervisionTimeoutMin,                        \
                             _supervisionTimeoutMax)

#define CmExtScanSetGlobalParamsRequest(_appHandle,                         \
                                        _flags,                             \
                                        _own_address_type,                  \
                                        _scanning_filter_policy,            \
                                        _filter_duplicates,                 \
                                        _scanning_phys,                     \
                                        _phys)                              \
    CmExtScanSetGlobalParamsReqSend(TrapToOxygenTask(_appHandle),           \
                                    _flags,                                 \
                                    _own_address_type,                      \
                                    _scanning_filter_policy,                \
                                    _filter_duplicates,                     \
                                    _scanning_phys,                         \
                                    _phys)

#define CmExtScanSetGlobalParamsReq(_appHandle,                             \
                                    _flags,                                 \
                                    _own_address_type,                      \
                                    _scanning_filter_policy,                \
                                    _filter_duplicates,                     \
                                    _scanning_phys,                         \
                                    _phys)                                  \
    CmExtScanSetGlobalParamsReqSend(TrapToOxygenTask(_appHandle),           \
                                    _flags,                                 \
                                    _own_address_type,                      \
                                    _scanning_filter_policy,                \
                                    _filter_duplicates,                     \
                                    _scanning_phys,                         \
                                    _phys)

#define CmExtScanRegisterScannerReq(_appHandle,                             \
                                    _flags,                                 \
                                    _adv_filter,                            \
                                    _adv_filter_sub_field1,                 \
                                    _adv_filter_sub_field2,                 \
                                    _ad_structure_filter,                   \
                                    _ad_structure_filter_sub_field1,        \
                                    _ad_structure_filter_sub_field2,        \
                                    _num_reg_ad_types,                      \
                                    _reg_ad_types)                          \
    CmExtScanRegisterScannerReqSend(TrapToOxygenTask(_appHandle),           \
                                    _flags,                                 \
                                    _adv_filter,                            \
                                    _adv_filter_sub_field1,                 \
                                    _adv_filter_sub_field2,                 \
                                    _ad_structure_filter,                   \
                                    _ad_structure_filter_sub_field1,        \
                                    _ad_structure_filter_sub_field2,        \
                                    _num_reg_ad_types,                      \
                                    _reg_ad_types)

#define CmExtScanEnableScannersReq(_appHandle,                              \
                                   _enable,                                 \
                                   _num_of_scanners,                        \
                                   _scanners)                               \
    CmExtScanEnableScannersReqSend(TrapToOxygenTask(_appHandle),            \
                                   _enable,                                 \
                                   _num_of_scanners,                        \
                                   _scanners)

#define CmExtScanUnregisterScannerReq(_appHandle,                           \
                                      _scan_handle)                         \
    CmExtScanUnregisterScannerReqSend(TrapToOxygenTask(_appHandle),         \
                                      _scan_handle)

#define CmExtScanGetGlobalParamsRequest(_appHandle)                         \
    CmExtScanGetGlobalParamsReqSend(TrapToOxygenTask(_appHandle))

#define CmExtScanGetGlobalParamsReq(_appHandle)                             \
    CmExtScanGetGlobalParamsReqSend(TrapToOxygenTask(_appHandle))

#define CmExtScanConfigureScannerReq(_appHandle,                            \
                                    _scan_handle,                           \
                                    _use_only_global_params,                \
                                    _scanning_phys,                         \
                                    _phys)                                  \
    CmExtScanConfigureScannerReqSend(TrapToOxygenTask(_appHandle),          \
                                     _scan_handle,                          \
                                     _use_only_global_params,               \
                                     _scanning_phys,                        \
                                     _phys)

#define CmExtScanGetCtrlScanInfoReq(_appHandle)                             \
    CmExtScanGetCtrlScanInfoReqSend(TrapToOxygenTask(_appHandle))

#define CmExtAdvReadMaxAdvDataLenRequest(_appHandle,                        \
                                         _advHandle)                        \
    CmExtAdvReadMaxAdvDataLenReqSend(TrapToOxygenTask(_appHandle),          \
                                     _advHandle)

#define CmExtAdvSetDataReq(_appHandle,                                      \
                           _advHandle,                                      \
                           _operation,                                      \
                           _fragPreference,                                 \
                           _dataLen,                                        \
                           _data)                                           \
    CmExtAdvSetDataReqSend(TrapToOxygenTask(_appHandle),                    \
                           _advHandle,                                      \
                           _operation,                                      \
                           _fragPreference,                                 \
                           _dataLen,                                        \
                           _data)

#define CmExtAdvEnableReq(_appHandle,                                       \
                           _advHandle,                                      \
                           _enable)                                         \
    CmExtAdvEnableReqSend(TrapToOxygenTask(_appHandle),                     \
                           _advHandle,                                      \
                           _enable)

#define CmExtAdvSetParamsReq(_appHandle,                                    \
                             _advHandle,                                    \
                             _advEventProperties,                           \
                             _primaryAdvIntervalMin,                        \
                             _primaryAdvIntervalMax,                        \
                             _primaryAdvChannelMap,                         \
                             _ownAddrType,                                  \
                             _peerAddr,                                     \
                             _advFilterPolicy,                              \
                             _primaryAdvPhy,                                \
                             _secondaryAdvMaxSkip,                          \
                             _secondaryAdvPhy,                              \
                             _advSid,                                       \
                             _reserved)                                     \
    CmExtAdvSetParamsReqSend(TrapToOxygenTask(_appHandle),                  \
                             _advHandle,                                    \
                             _advEventProperties,                           \
                             _primaryAdvIntervalMin,                        \
                             _primaryAdvIntervalMax,                        \
                             _primaryAdvChannelMap,                         \
                             _ownAddrType,                                  \
                             _peerAddr,                                     \
                             _advFilterPolicy,                              \
                             _primaryAdvPhy,                                \
                             _secondaryAdvMaxSkip,                          \
                             _secondaryAdvPhy,                              \
                             _advSid,                                       \
                             _reserved)

#define CmExtAdvSetParamsV2Req(_theAppTask,                                  \
                               _advHandle,                                  \
                               _advEventProperties,                         \
                               _primaryAdvIntervalMin,                      \
                               _primaryAdvIntervalMax,                      \
                               _primaryAdvChannelMap,                       \
                               _ownAddrType,                                \
                               _peerAddr,                                   \
                               _advFilterPolicy,                            \
                               _primaryAdvPhy,                              \
                               _secondaryAdvMaxSkip,                        \
                               _secondaryAdvPhy,                            \
                               _advSid,                                     \
                               _advTxPower,                                 \
                               _scanReqNotifyEnable,                        \
                               _primaryAdvPhyOptions,                       \
                               _secondaryAdvPhyOptions)                     \
      CmExtAdvSetParamsV2ReqSend(TrapToOxygenTask(_theAppTask),              \
                               _advHandle,                                  \
                               _advEventProperties,                         \
                               _primaryAdvIntervalMin,                      \
                               _primaryAdvIntervalMax,                      \
                               _primaryAdvChannelMap,                       \
                               _ownAddrType,                                \
                               _peerAddr,                                   \
                               _advFilterPolicy,                            \
                               _primaryAdvPhy,                              \
                               _secondaryAdvMaxSkip,                        \
                               _secondaryAdvPhy,                            \
                               _advSid,                                     \
                               _advTxPower,                                 \
                               _scanReqNotifyEnable,                        \
                               _primaryAdvPhyOptions,                       \
                               _secondaryAdvPhyOptions)

#define CmExtAdvRegisterAppAdvSetReq(_appHandle, _advHandle, _flags)        \
    CmExtAdvRegisterAppAdvSetReqSend(TrapToOxygenTask(_appHandle),          \
                                     _advHandle, _flags)

#define CmExtAdvUnregisterAppAdvSetReq(_appHandle, _advHandle)              \
    CmExtAdvUnregisterAppAdvSetReqSend(TrapToOxygenTask(_appHandle),        \
                                       _advHandle)

#define CmExtAdvSetScanRespDataReq(_appHandle,                              \
                                   _advHandle,                              \
                                   _operation,                              \
                                   _fragPreference,                         \
                                   _dataLen,                                \
                                   _data)                                   \
    CmExtAdvSetScanRespDataReqSend(TrapToOxygenTask(_appHandle),            \
                                   _advHandle,                              \
                                   _operation,                              \
                                   _fragPreference,                         \
                                   _dataLen,                                \
                                   _data)

#define CmExtAdvSetRandomAddrReq(_appHandle,                                \
                                 _advHandle,                                \
                                 _action,                                   \
                                 _randomAddr)                               \
    CmExtAdvSetRandomAddrReqSend(TrapToOxygenTask(_appHandle),              \
                                 _advHandle,                                \
                                 _action,                                   \
                                 _randomAddr)

#define CmIsocCisConnectRsp(_task, _cisHandle, _status)                     \
    CmIsocCisConnectRspSend(TrapToOxygenTask(_task), _cisHandle, _status)

#define CmIsocSetupIsoDataPathReq(_appHandle,                               \
                                  _handle,                                  \
                                  _data_path_direction,                     \
                                  _data_path_id,                            \
                                  _codec_id,                                \
                                  _controller_delay,                        \
                                  _codec_config_length,                     \
                                  _codec_config_data)                       \
    CmIsocSetupIsoDataPathReqSend(TrapToOxygenTask(_appHandle),             \
                                  _handle,                                  \
                                  _data_path_direction,                     \
                                  _data_path_id,                            \
                                  _codec_id,                                \
                                  _controller_delay,                        \
                                  _codec_config_length,                     \
                                  _codec_config_data)

#define CmPeriodicScanStartFindTrainsReq(_appHandle,                        \
                                         _flags,                            \
                                         _scanForXSeconds,                  \
                                         _adStructureFilter,                \
                                         _adStructureFilterSubField1,       \
                                         _adStructureFilterSubField2,       \
                                         _adStructureInfoLen,               \
                                         _adStructureInfo)                  \
    CmPeriodicScanStartFindTrainsReqSend(TrapToOxygenTask(_appHandle),      \
                                         _flags,                            \
                                         _scanForXSeconds,                  \
                                         _adStructureFilter,                \
                                         _adStructureFilterSubField1,       \
                                         _adStructureFilterSubField2,       \
                                         _adStructureInfoLen,               \
                                         _adStructureInfo)                  \

#define CmPeriodicScanStopFindTrainsReq(_appHandle, _scanHandle)            \
    CmPeriodicScanStopFindTrainsReqSend(TrapToOxygenTask(_appHandle), _scanHandle)

#define CmPeriodicScanSyncAdvReportEnableReq(_appHandle,                    \
                                             _syncHandle,                   \
                                             _enable)                       \
    CmPeriodicScanSyncAdvReportEnableReqSend(TrapToOxygenTask(_appHandle),  \
                                             _syncHandle,                   \
                                             _enable)

#define CmPeriodicScanSyncTransferParamsReq(_theAppTask,                    \
                                            _taddr,                         \
                                            _skip,                          \
                                            _syncTimeout,                   \
                                            _mode,                          \
                                            _cteType)                       \
    CmPeriodicScanSyncTransferParamsReqSend(TrapToOxygenTask(_theAppTask),  \
                                            _taddr,                         \
                                            _skip,                          \
                                            _syncTimeout,                   \
                                            _mode,                          \
                                            _cteType)

#define CmPeriodicScanSyncToTrainReq(_theAppTask,                           \
                                     _reportPeriodic,                       \
                                     _skip,                                 \
                                     _syncTimeout,                          \
                                     _syncCteType,                          \
                                     _attemptSyncForXSeconds,               \
                                     _numberOfPeriodicTrains,               \
                                     _PeriodicTrains)                       \
    CmPeriodicScanSyncToTrainReqSend(TrapToOxygenTask(_theAppTask),         \
                                     _reportPeriodic,                       \
                                     _skip,                                 \
                                     _syncTimeout,                          \
                                     _syncCteType,                          \
                                     _attemptSyncForXSeconds,               \
                                     _numberOfPeriodicTrains,               \
                                     _PeriodicTrains)

#define CmPeriodicScanSyncToTrainCancelReq(_theAppTask)                     \
    CmPeriodicScanSyncToTrainCancelReqSend(TrapToOxygenTask(_theAppTask))

#define CmIsocBigCreateSyncReq(_theAppTask,                                 \
                               _syncHandle,                                 \
                               _bigSyncTimeout,                             \
                               _bigHandle,                                  \
                               _mse,                                        \
                               _encryption,                                 \
                               _broadcastCode,                              \
                               _numBis,                                     \
                               _bis)                                        \
    CmIsocBigCreateSyncReqSend(TrapToOxygenTask(_theAppTask),               \
                               _syncHandle,                                 \
                               _bigSyncTimeout,                             \
                               _bigHandle,                                  \
                               _mse,                                        \
                               _encryption,                                 \
                               _broadcastCode,                              \
                               _numBis,                                     \
                               _bis)

#define CmPeriodicScanSyncTerminateReq(_theAppTask,                         \
                                       _syncHandle)                         \
    CmPeriodicScanSyncTerminateReqSend(TrapToOxygenTask(_theAppTask),       \
                                       _syncHandle)

#define CmPeriodicScanSyncTransferRequest(_theAppTask,                      \
                                          _addrt,                           \
                                          _serviceData,                     \
                                          _syncHandle)                      \
    CmPeriodicScanSyncTransferReqSend(TrapToOxygenTask(_theAppTask),        \
                                      _addrt,                               \
                                      _serviceData,                         \
                                      _syncHandle)

#define CmIsocBigTerminateSyncReq(_theAppTask,_bigHandle)                   \
    CmIsocBigTerminateSyncReqSend(TrapToOxygenTask(_theAppTask),_bigHandle)

#define CmIsocRegisterReq(_theAppTask,                                      \
                          _isocType)                                        \
    CmIsocRegisterReqSend(TrapToOxygenTask(_theAppTask),                    \
                          _isocType)

#define CmIsocReadIsoLinkQualityReq(_theAppTask, _handle)                   \
    CmIsocReadIsoLinkQualityReqSend(TrapToOxygenTask(_theAppTask), _handle)

#define CmReadRemoteNameReqSend(_theAppTask,                                \
                                _deviceAddr)                                \
        CsrBtCmReadRemoteNameReqSend(TrapToOxygenTask(_theAppTask),         \
                                     _deviceAddr)

#define CmCancelReadRemoteNameReqSend(_theAppTask,                          \
                                      _deviceAddr)                          \
        CsrBtCmCancelReadRemoteNameReqSend(TrapToOxygenTask(_theAppTask),   \
                                       _deviceAddr)

#define CmLeAclOpenReqSend(_theAppTask, _deviceAddr)                        \
    CsrBtCmLeAclOpenReqSend(TrapToOxygenTask(_theAppTask),                  \
                             _deviceAddr)

#define CmLeAclOpenUseFilterAcceptListRequest(_theAppTask)                  \
    CmLeAclOpenUseFilterAcceptListReqSend(TrapToOxygenTask(_theAppTask))

#define CmEnableDutModeReqSend(_task)                                       \
    CsrBtCmEnableDutModeReqSend(TrapToOxygenTask(_task))

#define CmLeTransmitterTestReqSend(_task,                                   \
                                   _txFrequency,                            \
                                   _lengthOfTestData,                       \
                                   _packetPayload)                          \
    CsrBtCmLeTransmitterTestReqSend(TrapToOxygenTask(_task),                \
                                    _txFrequency,                           \
                                    _lengthOfTestData,                      \
                                    _packetPayload)

#define CmLeReceiverTestReqSend(_task,_rxFrequency)                         \
    CsrBtCmLeReceiverTestReqSend(TrapToOxygenTask(_task),_rxFrequency)

#define CmLeTestEndReqSend(_task)                                           \
    CsrBtCmLeTestEndReqSend(TrapToOxygenTask(_task))

#define CmLeEnhancedTransmitterTestRequest(_task,                           \
                                           _txFrequency,                    \
                                           _lengthOfTestData,               \
                                           _packetPayload,                  \
                                           _phy)                            \
         CmLeEnhancedTransmitterTestReqSend(TrapToOxygenTask(_task),        \
                                            _txFrequency,                   \
                                            _lengthOfTestData,              \
                                            _packetPayload,                 \
                                            _phy)

#define CmLeEnhancedReceiverTestRequest(_task,                              \
                                        _rxFrequency,                       \
                                        _phy,                               \
                                        _modIndex)                          \
         CmLeEnhancedReceiverTestReqSend(TrapToOxygenTask(_task),           \
                                         _rxFrequency,                      \
                                         _phy,                              \
                                         _modIndex)


/* Primitive free function */
#define CmFreeUpstreamMessageContents(_message)                             \
    CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, (void *) _message)

#define CmPortParDefault(_thePortPar)                                       \
    CsrBtPortParDefault(_thePortPar)

#define CmPeriodicScanSyncLostRsp(_syncHandle)                              \
    CmPeriodicScanSyncLostRspSend(_syncHandle)
#define CmWriteKeysNow(_address,                                            \
                       _addrType,                                           \
                       _keyType,                                            \
                       _key)                                                \
    CsrBtCmDatabaseReqSendNow(_address,                                     \
                              _addrType,                                    \
                              _keyType,                                     \
                              _key)

#define CmSetLinkBehaviorReqSend(_theAppTask,                               \
                                 _addrType,                                 \
                                 _addr,                                     \
                                 _l2capRetry)                               \
    CsrBtCmSetLinkBehaviorReqSend(TrapToOxygenTask(_theAppTask),            \
                                  _addrType,                                \
                                  _addr,                                    \
                                  _l2capRetry)

#define CmSetLinkBehaviorExtReq(_theAppTask,                                \
                                _addrType,                                  \
                                _addr,                                      \
                                _flags)                                     \
    CmSetLinkBehaviorReqSendExt(TrapToOxygenTask(_theAppTask),              \
                                _addrType,                                  \
                                _addr,                                      \
                                _flags)

#define CmLeSirkOperationReq(_theAppTask, _tbdAddr, _flags, _sirk_key)      \
    CsrBtCmLeSirkOperationReqSend(TrapToOxygenTask(_theAppTask), _tbdAddr,  \
                                    _flags, _sirk_key)

#define CmLeSetDataRelatedAddressChangesReq(_theAppTask,                    \
                                            _advHandle,                     \
                                            _flags,                         \
                                            _changeReasons)                 \
    CsrBtCmLeSetDataRelatedAddressChangesReqSend(TrapToOxygenTask(_theAppTask), \
                                               _advHandle,                  \
                                               _flags,                      \
                                               _changeReasons)

#define CmGetAdvScanCapabilitiesReq(_theAppTask)                            \
    CmGetAdvScanCapabilitiesReqSend(TrapToOxygenTask(_theAppTask))

#define CmExtAdvSetsInfoReq(_theAppTask)                                    \
    CmExtAdvSetsInfoReqSend(TrapToOxygenTask(_theAppTask))

#define CmExtAdvMultiEnableReq(_theAppTask, _enable, _numSets, _config)     \
    CmExtAdvMultiEnableReqSend(TrapToOxygenTask(_theAppTask), _enable,      \
                                    _numSets, _config)

#define CmRefreshAllDevices()                                               \
    CsrBtCmScRefreshAllDevicesNow()

#define CmReadTxPowerLevelReqSend(_theAppTask,                              \
                                  _deviceAddr,                              \
                                  _addressType,                             \
                                  _transportType,                           \
                                  _levelType)                               \
    CsrBtCmReadTxPowerLevelReqSend(TrapToOxygenTask(_theAppTask),           \
                                   _deviceAddr,                             \
                                   _addressType,                            \
                                   _transportType,                          \
                                   _levelType)

#define CmRfcDisconnectRspSend(btConnId)                                    \
    CsrBtCmRfcDisconnectRspSend(btConnId)

#define CmL2caDisconnectRspSend(_l2caSignalId, btConnId)                    \
    CsrBtCmL2caDisconnectRspSend(_l2caSignalId, btConnId)

#define CmCryptoGeneratePublicPrivateKeyReqSend(_theAppTask,                      \
                                                _keyType)                         \
    CsrBtCmCryptoGeneratePublicPrivateKeyReqSend(TrapToOxygenTask(_theAppTask),   \
                                                 _keyType)

#define CmCryptoGenerateSharedSecretKeyReqSend(_theAppTask,                       \
                                               _keyType,                          \
                                               _privateKey,                       \
                                               _publicKey)                        \
    CsrBtCmCryptoGenerateSharedSecretKeyReqSend(TrapToOxygenTask(_theAppTask),    \
                                                _keyType,                         \
                                                _privateKey,                      \
                                                _publicKey)

#define CmCryptoEncryptReqSend(_theAppTask,                                       \
                               _dataArray,                                        \
                               _keyArray)                                         \
    CsrBtCmCryptoEncryptReqSend(TrapToOxygenTask(_theAppTask),                    \
                                _dataArray,                                       \
                                _keyArray)

#define CmCryptoHashReqSend(_theAppTask,                                          \
                            _dataArray,                                           \
                            _arraySize)                                           \
    CsrBtCmCryptoHashReqSend(TrapToOxygenTask(_theAppTask),                       \
                             _dataArray,                                          \
                             _arraySize)

#define CmCryptoHashContinueReqSend(_theAppTask,                                  \
                                    _dataArray,                                   \
                                    _arraySize,                                   \
                                    _currentIndex)                                \
    CsrBtCmCryptoHashContinueReqSend(TrapToOxygenTask(_theAppTask),               \
                                     _dataArray,                                  \
                                     _arraySize,                                  \
                                     _currentIndex)

#define CmCryptoDecryptReqSend(_theAppTask,                                       \
                               _dataArray,                                        \
                               _keyArray)                                         \
    CsrBtCmCryptoDecryptReqSend(TrapToOxygenTask(_theAppTask),                    \
                                _dataArray,                                       \
                                _keyArray)

#define CmCryptoAesCtrReqSend(_theAppTask,                                        \
                              _counter,                                           \
                              _flags,                                             \
                              _secretKey,                                         \
                              _nonce,                                             \
                              _dataLen,                                           \
                              _data)                                              \
    CsrBtCmCryptoAesCtrReqSend(TrapToOxygenTask(_theAppTask),                     \
                               _counter,                                          \
                               _flags,                                            \
                               _secretKey,                                        \
                               _nonce,                                            \
                               _dataLen,                                          \
                               _data)

#define CmRfcConnectAcceptRspSend(_theAppTask,                                    \
                                  _btConnId,                                      \
                                  _deviceAddr,                                    \
                                  _accept,                                        \
                                  _serverChannel,                                 \
                                  _modemStatus,                                   \
                                  _breakSignal,                                   \
                                  _mscTimeout)                                    \
    CsrBtCmRfcConnectAcceptRspSend(TrapToOxygenTask(_theAppTask),                 \
                                   _btConnId,                                     \
                                   _deviceAddr,                                   \
                                   _accept,                                       \
                                   _serverChannel,                                \
                                   _modemStatus,                                  \
                                   _breakSignal,                                  \
                                   _mscTimeout)

#define CmL2caConnectAcceptRspSend(_theAppTask,                                   \
                                   _accept,                                       \
                                   _btConnId,                                     \
                                   _localPsm,                                     \
                                   _deviceAddr,                                   \
                                   _identifier,                                   \
                                   _conftabCount,                                 \
                                   _conftab,                                      \
                                   _minEncKeySize)                                \
    CsrBtCmL2caConnectAcceptRspSend(TrapToOxygenTask(_theAppTask),                \
                                    _accept,                                      \
                                    _btConnId,                                    \
                                    _localPsm,                                    \
                                    _deviceAddr,                                  \
                                    _identifier,                                  \
                                    _conftabCount,                                \
                                    _conftab,                                     \
                                    _minEncKeySize)

#define CmWriteAuthPayloadTimeoutReqSend(_theAppTask,                             \
                                         _tpAddrt,                                \
                                         _authPayloadTimeout,                     \
                                         _aptRoute)                               \
    CsrBtCmWriteAuthPayloadTimeoutExtReqSend(TrapToOxygenTask(_theAppTask),       \
                                             _tpAddrt,                            \
                                             _authPayloadTimeout,                 \
                                             _aptRoute)

#define CmWriteScHostSupportOverrideRequest(_theAppTask,                          \
                                            _deviceAddr,                          \
                                            _overrideAction)                      \
    CmWriteScHostSupportOverrideReqSend(TrapToOxygenTask(_theAppTask),            \
                                        _deviceAddr,                              \
                                        _overrideAction)

#define CmReadScHostSupportOverrideMaxBdAddrRequest(_theAppTask)                  \
    CmReadScHostSupportOverrideMaxBdAddrReqSend(TrapToOxygenTask(_theAppTask))

#define CmPeriodicAdvEnableReq(_theAppTask,                                       \
                               _adv_handle,                                       \
                               _flags,                                            \
                               _enable)                                           \
    CmPeriodicAdvEnableReqSend(TrapToOxygenTask(_theAppTask),                     \
                               _adv_handle,                                       \
                               _flags,                                            \
                               _enable)

#define CmPeriodicAdvSetParamsReq(_theAppTask,                                    \
                                  _advHandle,                                     \
                                  _flags,                                         \
                                  _periodicAdvIntervalMin,                        \
                                  _periodicAdvIntervalMax,                        \
                                  _periodicAdvProperties)                         \
    CmPeriodicAdvSetParamsReqSend(TrapToOxygenTask(_theAppTask),                  \
                                  _advHandle,                                     \
                                  _flags,                                         \
                                  _periodicAdvIntervalMin,                        \
                                  _periodicAdvIntervalMax,                        \
                                  _periodicAdvProperties)                         \

#define CmPeriodicAdvSetDataReq(_theAppTask,                                      \
                                 _advHandle,                                      \
                                 _operation,                                      \
                                 _dataLength,                                     \
                                 _data)                                           \
    CmPeriodicAdvSetDataReqSend(TrapToOxygenTask(_theAppTask),                    \
                                                 _advHandle,                      \
                                                 _operation,                      \
                                                 _dataLength,                     \
                                                 _data)                           \

#define CmPeriodicAdvReadMaxAdvDataLenRequest(_theAppTask,                        \
                                              _advHandle)                         \
    CmPeriodicAdvReadMaxAdvDataLenReqSend(TrapToOxygenTask(_theAppTask),          \
                                          _advHandle)

#define CmPeriodicAdvStartRequest(_theAppTask,                                    \
                                  _advHandle)                                     \
    CmPeriodicAdvStartReqSend(TrapToOxygenTask(_theAppTask),                      \
                              _advHandle)

#define CmPeriodicAdvStopRequest(_theAppTask,                                     \
                                 _advHandle,                                      \
                                 _stopAdvertising)                                \
    CmPeriodicAdvStopReqSend(TrapToOxygenTask(_theAppTask),                       \
                             _advHandle,                                          \
                             _stopAdvertising)

#define CmPeriodicAdvSetTransferRequest(_theAppTask,                              \
                                        _addrt,                                   \
                                        _serviceData,                             \
                                        _advHandle)                               \
    CmPeriodicAdvSetTransferReqSend(TrapToOxygenTask(_theAppTask),                \
                                    _addrt,                                       \
                                    _serviceData,                                 \
                                    _advHandle)

#define CmCancelInquiryReqSend(_theAppTask)                                       \
    CsrBtCmCancelInquiryReqSend(TrapToOxygenTask(_theAppTask))

#define CmSetEventFilterCodReqSend(_theAppTask,                                   \
                                   _selectInquiryFilter,                          \
                                   _autoAccept,                                   \
                                   _cod,                                          \
                                   _codMask)                                      \
    CsrBtCmSetEventFilterCodReqSend(TrapToOxygenTask(_theAppTask),                \
                                    _selectInquiryFilter,                         \
                                    _autoAccept,                                  \
                                    _cod,                                         \
                                    _codMask)

#define CmClearEventFilterReqSend(_theAppTask,                                    \
                                  _filter)                                        \
    CsrBtCmClearEventFilterReqSend(TrapToOxygenTask(_theAppTask),                 \
                                   _filter)

#define CmInquiryExtReqSend(_theAppTask,                                          \
                            _inquiryAccessCode,                                   \
                            _inquiryTxpowerLevel,                                 \
                            _configMask,                                          \
                            _maxResponses,                                        \
                            _inquiryTimeout)                                      \
    CsrBtCmInquiryExtReqSend(TrapToOxygenTask(_theAppTask),                       \
                             _inquiryAccessCode,                                  \
                             _inquiryTxpowerLevel,                                \
                             _configMask,                                         \
                             _maxResponses,                                       \
                             _inquiryTimeout)

#define CmScDmRemoveDeviceReq(_deviceAddr, _addressType)                          \
    CsrBtCmScDmRemoveDeviceReq(_deviceAddr, _addressType)

/* API CmScDmRemoveDeviceReqSend(..) is deprecated and will likely be removed from future ADKs.
   Use CmScDmRemoveDeviceReq(..) instead of CmScDmRemoveDeviceReqSend(..) */
#define CmScDmRemoveDeviceReqSend(_deviceAddr,                                    \
                                 _addressType)                                    \
        CmScDmRemoveDeviceReq(_deviceAddr,                                        \
                               _addressType)

#define CmRefreshEncryptionKeyRequest(_deviceAddr) \
    CmSmRefreshEncryptionKeyReqSend(_deviceAddr)

#define CmLeSetPhyRequest(_theAppTask,                                      \
                          _tpAddr,                                          \
                          _minTxRate,                                       \
                          _maxTxRate,                                       \
                          _minRxRate,                                       \
                          _maxRxRate,                                       \
                          _flags)                                           \
    CmDmLeSetPhyReqSend(TrapToOxygenTask(_theAppTask),                      \
                        _tpAddr,                                            \
                        _minTxRate,                                         \
                        _maxTxRate,                                         \
                        _minRxRate,                                         \
                        _maxRxRate,                                         \
                        _flags)

#define CmLeSetDefaultPhyRequest(_theAppTask,                               \
                                 _minTxRate,                                \
                                 _maxTxRate,                                \
                                 _minRxRate,                                \
                                 _maxRxRate,                                \
                                 _flags)                                    \
    CmDmLeSetDefaultPhyReqSend(TrapToOxygenTask(_theAppTask),               \
                               _minTxRate,                                  \
                               _maxTxRate,                                  \
                               _minRxRate,                                  \
                               _maxRxRate,                                  \
                               _flags)

#define CmSecurityRegisterRequest(_protocolId,                              \
                                  _channel,                                 \
                                  _outgoingOk,                              \
                                  _securityLevel,                           \
                                  minEncKeySize)                            \
    CsrBtCmScDmRegisterReq(_protocolId,                                     \
                           _channel,                                        \
                           _outgoingOk,                                     \
                           _securityLevel,                                  \
                           minEncKeySize)

#define CmSecurityUnregisterRequest(_channel,                               \
                                    _protocolId)                            \
    CsrBtCmScDmUnRegisterReq(_channel,                                      \
                             _protocolId)
#define CmGenerateCrossTransKeyRequestResponse(_tpAddr,                     \
                                               _identifier,                 \
                                               _flags)                      \
    CmSmGenerateCrossTransKeyRequestRspSend(_tpAddr,                        \
                                            _identifier,                    \
                                            _flags)


#define CmSecurityModeConfigRequest(_writeAuthEnable, _config)              \
    CsrBtCmScDmSecModeConfigReq(_writeAuthEnable, _config)

#define CmL2caUnregisterRequest(_theAppTask,                                \
                                _localPsm)                                  \
    CsrBtCml2caUnRegisterReqSend(TrapToOxygenTask(_theAppTask),             \
                                 _localPsm)

#define CmWriteAutoFlushTimeoutRequest(_theAppTask,                         \
                                       _deviceAddr,                         \
                                       _flushTimeout)                       \
    CmDmWriteAutoFlushTimeoutReqSend(TrapToOxygenTask(_theAppTask),         \
                                     _deviceAddr,                           \
                                     _flushTimeout)

#define CmReadRemoteFeaturesRequest(_theAppTask,                            \
                                    _deviceAddr)                            \
    CsrBtCmReadRemoteFeaturesReqSend(TrapToOxygenTask(_theAppTask),         \
                                     _deviceAddr)

#define CmChangeConnLinkKeyRequest(_theAppTask,                             \
                                   _deviceAddr)                             \
    CmDmChangeConnLinkKeyReqSend(TrapToOxygenTask(_theAppTask),             \
                                 _deviceAddr)

#define CmRegisterApplicationHandleRequest(_theAppTask,                     \
                                           _btConnId,                       \
                                           _protocol)                       \
    CmRegisterApplicationHandle(TrapToOxygenTask(_theAppTask),              \
                                _btConnId,                                  \
                                _protocol)

#define CmLeWhitelistSetRequest(_theAppTask,                                \
                                _addrCount,                                 \
                                _addrList)                                  \
    CsrBtCmLeWhitelistSetReqSend(TrapToOxygenTask(_theAppTask),             \
                                 _addrCount,                                \
                                 _addrList)

#define  CmLeGetControllerInfoRequest(_theAppTask)                          \
    CsrBtCmLeGetControllerInfoReqSend(TrapToOxygenTask(_theAppTask))

#define CmLeAddDeviceToWhiteListRequest(_theAppTask, _deviceAddr, _addrType) \
    CmLeAddDeviceToWhiteListReqSend(TrapToOxygenTask(_theAppTask), _deviceAddr, _addrType)

#define CmLeRemoveDeviceFromWhiteListRequest(_theAppTask, _deviceAddr, _addrType) \
    CmLeRemoveDeviceFromWhiteListReqSend(TrapToOxygenTask(_theAppTask), _deviceAddr, _addrType)

#ifdef INSTALL_CM_READ_INQUIRY_MODE
#define CmReadInquiryModeRequest(_theAppTask)                                     \
    CmDmReadInquiryModeReqSend(TrapToOxygenTask(_theAppTask))
#endif /* INSTALL_CM_READ_INQUIRY_MODE */

#ifdef INSTALL_CM_READ_INQUIRY_TX
#define CmReadInquiryTxRequest(_theAppTask)                                 \
    CmDmReadInquiryTxReqSend(TrapToOxygenTask(_theAppTask))
#endif /* INSTALL_CM_READ_INQUIRY_TX */

#ifdef INSTALL_CM_READ_APT
#define CmReadAuthPayloadTimeoutRequest(_theAppTask,                  \
                                        _tpAddrt)                     \
    CmDmReadAuthPayloadTimeoutReqSend(TrapToOxygenTask(_theAppTask),  \
                                      _tpAddrt)
#endif /* INSTALL_CM_READ_APT */

#ifdef INSTALL_CM_KEY_REQUEST_INDICATION
#define CmKeyRequestResponse(_tpAddr,                                      \
                             _secRequirements,                             \
                             _keyType,                                     \
                             _keyAvailable,                                \
                             _key)                                         \
    CmSmKeyRequestRspSend(_tpAddr, _secRequirements, _keyType, _keyAvailable, _key)
#endif /*INSTALL_CM_KEY_REQUEST_INDICATION*/

#ifdef INSTALL_CM_READ_EIR_DATA
#define CmReadEIRDataRequest(_theAppTask)                   \
    CmDmReadEIRDataReqSend(TrapToOxygenTask(_theAppTask))
#endif /* INSTALL_CM_READ_EIR_DATA */

#ifdef INSTALL_CM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL
#define CmLeReadRemoteTransmitPowerLevelRequest(_theAppTask,                                       \
                                                _tpAddrt,                                          \
                                                _phy)                                              \
    CmDmLeReadRemoteTransmitPowerLevelReqSend(TrapToOxygenTask(_theAppTask),                       \
                                              _tpAddrt,                                            \
                                              _phy)
#endif /* INSTALL_CM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL */

#ifdef INSTALL_CM_LE_SET_TRANSMIT_POWER_REPORTING
#define CmLeSetTransmitPowerReportingEnableRequest(_theAppTask,                     \
                                                   _tpAddrt,                        \
                                                   _localEnable,                    \
                                                   _remoteEnable)                   \
    CmDmLeSetTransmitPowerReportingEnableReqSend(TrapToOxygenTask(_theAppTask),     \
                                                 _tpAddrt,                          \
                                                 _localEnable,                      \
                                                 _remoteEnable)
#endif /* INSTALL_CM_LE_SET_TRANSMIT_POWER_REPORTING */

#ifdef INSTALL_CM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL
#define CmLeEnhancedReadTransmitPowerLevelRequest(_theAppTask,                                     \
                                                  _tpAddrt,                                        \
                                                  _phy)                                            \
    CmDmLeEnhancedReadTransmitPowerLevelReqSend(TrapToOxygenTask(_theAppTask),                     \
                                                _tpAddrt,                                          \
                                                _phy)
#endif /* INSTALL_CM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL */

#ifdef INSTALL_CM_LE_PATH_LOSS_REPORTING
#define CmDmLeSetPathLossReportingParametersRequest(_theAppTask,               \
                                                     _tpAddrt,                 \
                                                     _highThreshold,           \
                                                     _highHysteresis,          \
                                                     _lowThreshold,            \
                                                     _lowHysteresis,           \
                                                     _minTimeSpent)            \
    CmDmLeSetPathLossReportingParametersReqSend(TrapToOxygenTask(_theAppTask), \
                                                _tpAddrt,                      \
                                                _highThreshold,                \
                                                _highHysteresis,               \
                                                _lowThreshold,                 \
                                                _lowHysteresis,                \
                                                _minTimeSpent)

#define CmDmLeSetPathLossReportingEnableRequest(_theAppTask,                   \
                                                _tpAddrt,                      \
                                                _enable)                       \
    CmDmLeSetPathLossReportingEnableReqSend(TrapToOxygenTask(_theAppTask),     \
                                                _tpAddrt,                      \
                                                _enable)
#endif /* INSTALL_CM_LE_PATH_LOSS_REPORTING */

#ifdef INSTALL_CM_WRITE_INQUIRY_MODE
#define CmWriteInquiryModeRequest(_theAppTask,                  \
                                  _mode)                        \
    CmDmWriteInquiryModeReqSend(TrapToOxygenTask(_theAppTask),  \
                                _mode)
#endif /* INSTALL_CM_WRITE_INQUIRY_MODE */

#ifdef INSTALL_CM_DM_LE_READ_CHANNEL_MAP
#define CmLeReadChannelMapReqSendRequest(_theAppTask,               \
                                         _tpAddrt)                  \
    CmDmLeReadChannelMapReqSend(TrapToOxygenTask(_theAppTask),      \
                                    _tpAddrt)
#endif /* INSTALL_CM_DM_LE_READ_CHANNEL_MAP */

#define CmRemoveDeviceKeyRequest(_theAppTask,                   \
                                 _deviceAddr,                   \
                                 _addressType,                  \
                                 _keyType)                      \
    CmDmRemoveDeviceKeyReqSend(TrapToOxygenTask(_theAppTask),   \
                               _deviceAddr,                     \
                               _addressType,                    \
                               _keyType)

#define CmRemoveDeviceSmDbOnlyRequest(_theAppTask,                    \
                                      _deviceAddr,                    \
                                      _addressType)                   \
    CmDmRemoveDeviceOptionsReqSend(TrapToOxygenTask(_theAppTask),     \
                                   _deviceAddr,                       \
                                   _addressType,                      \
                                   CM_REMOVE_DEVICE_OPTION_SMDB_ONLY)

#ifdef CSR_BT_INSTALL_CM_LOW_POWER_CONFIG_PUBLIC
#define CmSniffSubRateRequest(_theAppTask,                            \
                              _deviceAddr,                            \
                              _maxRemoteLatency,                      \
                              _minRemoteTimeout,                      \
                              _minLocalTimeout)                       \
    CmDmSniffSubRateReqSend(TrapToOxygenTask(_theAppTask),            \
                            _deviceAddr,                              \
                            _maxRemoteLatency,                        \
                            _minRemoteTimeout,                        \
                            _minLocalTimeout)

#define CmSniffModeRequest(_theAppTask,                               \
                           _deviceAddr,                               \
                           _maxInterval,                              \
                           _minInterval,                              \
                           _attempt,                                  \
                           _timeout,                                  \
                           _forceSniffSettings)                       \
    CsrBtCmSniffModeReqSend(TrapToOxygenTask(_theAppTask),            \
                           _deviceAddr,                               \
                           _maxInterval,                              \
                           _minInterval,                              \
                           _attempt,                                  \
                           _timeout,                                  \
                           _forceSniffSettings)

#define CmExitSniffModeRequest(_theAppTask,                           \
                               _deviceAddr)                           \
    CsrBtCmExitSniffModeReqSend(TrapToOxygenTask(_theAppTask),        \
                                _deviceAddr)
#endif /* CSR_BT_INSTALL_CM_LOW_POWER_CONFIG_PUBLIC */

#define CmLeReadLocalSupportedFeaturesRequest(_theAppTask)                      \
    CsrBtCmLeReadLocalSupportedFeaturesReqSend(TrapToOxygenTask(_theAppTask))

#ifdef INSTALL_CM_DM_CONFIGURE_DATA_PATH
#define CmDmConfigureDataPathRequest(_theAppTask,                     \
                                     _dataPathDirection,              \
                                     _dataPathId,                     \
                                     _vendorSpecificConfigLen,        \
                                     _vendorSpecificConfig)           \
    CmDmConfigureDataPathReqSend(TrapToOxygenTask(_theAppTask),       \
                                     _dataPathDirection,              \
                                     _dataPathId,                     \
                                     _vendorSpecificConfigLen,        \
                                     _vendorSpecificConfig)
#endif /* INSTALL_CM_DM_CONFIGURE_DATA_PATH */

#ifdef INSTALL_CM_SM_CONFIG
#define CmSmConfigRequest(_theAppTask,                                \
                          _configMask)                                \
    CmSmConfigReqSend(TrapToOxygenTask(_theAppTask),                  \
                      _configMask,                                    \
                      0,                                              \
                      NULL)
#endif /* INSTALL_CM_SM_CONFIG */

#endif /* COMMON_SYNERGY_INC_CM_LIB_H_ */

