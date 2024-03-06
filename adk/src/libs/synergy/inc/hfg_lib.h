/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef COMMON_SYNERGY_INC_HFG_LIB_H_
#define COMMON_SYNERGY_INC_HFG_LIB_H_

#include "synergy.h"
#include "csr_bt_hfg_lib.h"

#define HFG_PRIM             (SYNERGY_EVENT_BASE + CSR_BT_HFG_PRIM)

#define HfgActivateReqSend(_task, _atMode,                                                  \
                          _numConnections, _serviceName,                                    \
                          _supportedFeatures, _callConfig, _hfgConfig,                      \
                          _hfgSupportedHfIndicators,                                        \
                          _hfgSupportedHfIndicatorsCount)                                   \
    CsrBtHfgActivateReqSend(TrapToOxygenTask(_task), _atMode,                               \
                           _numConnections, _serviceName,                                   \
                           _supportedFeatures, _callConfig, _hfgConfig,                     \
                           _hfgSupportedHfIndicators,                                       \
                           _hfgSupportedHfIndicatorsCount)

#define HfgDeactivateReqSend()                                                              \
    CsrBtHfgDeactivateReqSend()

#define HfgServiceConnectReqSend(_deviceAddr, _connectionType)                              \
    CsrBtHfgServiceConnectReqSend(_deviceAddr, _connectionType)

#define HfgCancelConnectReqSend(_deviceAddr)                                                \
    CsrBtHfgCancelConnectReqSend(_deviceAddr)

#define HfgDisconnectReqSend(_connectionId)                                                 \
    CsrBtHfgDisconnectReqSend(_connectionId)

#define HfgAudioConnectReqSend(_connectionId, _pcmSlot, _pcmRealloc)                        \
    CsrBtHfgAudioConnectReqSend(_connectionId, _pcmSlot, _pcmRealloc)

#define HfgAudioConnectExtReqSend(_connectionId, _pcmSlot, _pcmRealloc, _qceCodecId)        \
    CsrBtHfgAudioConnectExtReqSend(_connectionId, _pcmSlot, _pcmRealloc, _qceCodecId)

#define HfgAudioAcceptConnectResSend(_connectionId, _acceptResponse,                        \
                             _acceptParameters, _pcmSlot, _pcmReassign)                     \
    CsrBtHfgAudioAcceptConnectResSend(_connectionId, _acceptResponse,                       \
                              _acceptParameters, _pcmSlot, _pcmReassign)

#define HfgAudioDisconnectReqSend(_connectionId)                                            \
    CsrBtHfgAudioDisconnectReqSend(_connectionId)

#define HfgSecurityInReqSend(_appHandle, _secLevel)                                         \
    CsrBtHfgSecurityInReqSend(_appHandle, _secLevel)

#define HfgSecurityOutReqSend(_appHandle, _secLevel)                                        \
        CsrBtHfgSecurityOutReqSend(_appHandle, _secLevel)

#define HfgConfigSniffReqSend(_mask)                                                        \
    CsrBtHfgConfigSniffReqSend(_mask)

#define HfgConfigAudioReqSend(_connectionId, _audioType,                                    \
                            _audioSetting, _audioSettingLen)                                \
    CsrBtHfgConfigAudioReqSend(_connectionId, _audioType,                                   \
                            _audioSetting, _audioSettingLen)

#define HfgRingReqSend(_connectionId, _repetitionRate,                                      \
                        _numOfRings, _number, _name, _numType)                              \
    CsrBtHfgRingReqSend(_connectionId, _repetitionRate,                                     \
                        _numOfRings, _number, _name, _numType)

#define HfgCallWaitingReqSend(_connectionId, _number,                                       \
                            _name, _numType)                                                \
    CsrBtHfgCallWaitingReqSend(_connectionId, _number, _name, _numType)

#define HfgCallHandlingReqSend(_connectionId, _btrh)                                        \
    CsrBtHfgCallHandlingReqSend(_connectionId, _btrh) 

#define HfgCallHandlingResSend(_connectionId, _cmeeCode, _btrh)                             \
    CsrBtHfgCallHandlingResSend(_connectionId, _cmeeCode, _btrh) 

#define HfgDialResSend(_connectionId, _cmeeCode)                                            \
    CsrBtHfgDialResSend(_connectionId, _cmeeCode)
    
#define HfgSpeakerGainReqSend(_connectionId, _gain)                                         \
    CsrBtHfgSpeakerGainReqSend(_connectionId, _gain)
    
#define HfgMicGainReqSend(_connectionId, _gain)                                             \
    CsrBtHfgMicGainReqSend(_connectionId, _gain)
    
#define HfgAtCmdReqSend(_connectionId, _command)                                            \
    CsrBtHfgAtCmdReqSend(_connectionId, _command) 
    
#define HfgOperatorResSend(_connectionId, _mode, _operatorName, _cmeeMode)                  \
    CsrBtHfgOperatorResSend(_connectionId, _mode, _operatorName, _cmeeMode) 

#define HfgCallListResSend(_connectionId, _final, _idx, _dir, _stat, _mode,                 \
                                _mpy, _number, _numType, _cmeeMode)                         \
    CsrBtHfgCallListResSend(_connectionId, _final, _idx, _dir, _stat, _mode,                \
                                _mpy, _number, _numType, _cmeeMode)

#define HfgSubscriberNumberResSend(_connectionId, _final, _number,                          \
                                        _numType, _service, _cmeeMode)                      \
    CsrBtHfgSubscriberNumberResSend(_connectionId, _final, _number,                         \
                                        _numType, _service, _cmeeMode) 
    
#define HfgStatusIndicatorSetReqSend(_connectionId, _indicator, _value)                     \
    CsrBtHfgStatusIndicatorSetReqSend(_connectionId, _indicator, _value) 
    
#define HfgInbandRingingReqSend(_connectionId, _inband)                                     \
    CsrBtHfgInbandRingingReqSend(_connectionId, _inband)
    
#define HfgBtInputResSend(_connectionId, _cmeeMode, _response)                              \
    CsrBtHfgBtInputResSend(_connectionId, _cmeeMode, _response)

#define HfgVoiceRecogReqSend(_connectionId,                                                 \
                                      _bvra,                                                \
                                      _vreState,                                            \
                                      _textId,                                              \
                                      _textType,                                            \
                                      _textOperation,                                       \
                                      _string)                                              \
    CsrBtHfgVoiceRecogReqSend(_connectionId,                                                \
                                          _bvra,                                            \
                                          _vreState,                                        \
                                          _textId,                                          \
                                          _textType,                                        \
                                          _textOperation,                                   \
                                          _string)

#define HfgVoiceRecogResSend(_connectionId, _cmeeMode)                                      \
    CsrBtHfgVoiceRecogResSend(_connectionId, _cmeeMode)

#define HfgManualIndicatorResSend(_connectionId, _serviceIndicator, _statusIndicator,       \
                                    _setupStatusIndicator, _heldIndicator,                  \
                                    _signalStrengthIndicator, _roamIndicator,               \
                                    _batteryIndicator)                                      \
    CsrBtHfgManualIndicatorResSend(_connectionId, _serviceIndicator, _statusIndicator,      \
                                        _setupStatusIndicator, _heldIndicator,              \
                                        _signalStrengthIndicator, _roamIndicator,           \
                                        _batteryIndicator)

#ifdef CSR_BT_INSTALL_HFG_CONFIG_SINGLE_ATCMD
#define HfgConfigSingleAtcmdReqSend(_phandle, _idx, _sendToApp)                             \
    CsrBtHfgConfigSingleAtcmdReqSend(_phandle, _idx, _sendToApp)
#endif

#ifdef CSR_BT_INSTALL_HFG_CONFIG_ATCMD_HANDLING
#define HfgConfigAtcmdHandlingReqSend(_phandle,                                             \
                                        _bitwiseIndicators,                                 \
                                        _bitwiseIndicatorsLength)                           \
        CsrBtHfgConfigAtcmdHandlingReqSend(_phandle,                                        \
                                            _bitwiseIndicators,                             \
                                            _bitwiseIndicatorsLength)
#endif

#define HfgFreeUpstreamMessageContents(_message)                                            \
    CsrBtHfgFreeUpstreamMessageContents(CSR_BT_HFG_PRIM, _message);

#define HfgSetDeregisterTimeReqSend(_waitSeconds)                                           \
    CsrBtHfgSetDeregisterTimeReqSend(_waitSeconds)
    
#define HfgSetHfIndicatorStatusReqSend(_connectionId, _indId, _status)                      \
    CsrBtHfgSetHfIndicatorStatusReqSend(_connectionId, _indId, _status)

#define HfgSwbRspSend(_connectionId,_len, _cmd)                                             \
    CsrBtHfgSwbRspSend(_connectionId,_len, _cmd)

#endif /* COMMON_SYNERGY_INC_HFG_LIB_H_ */
