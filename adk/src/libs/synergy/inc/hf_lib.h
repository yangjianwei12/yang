/******************************************************************************
 Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 DESCRIPTION
     Header file providing mapping for Synergy HF profile's public interfaces.
     Refer to csr_bt_hf_lib.h for APIs descriptions.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef COMMON_SYNERGY_INC_HF_LIB_H_
#define COMMON_SYNERGY_INC_HF_LIB_H_

#include "synergy.h"
#include "csr_bt_hf_lib.h"

#define HF_PRIM             (SYNERGY_EVENT_BASE + CSR_BT_HF_PRIM)

#define HfActivateReqSend(_task, _maxNumberOfHfConnections,                           \
                          _maxNumberOfHsConnections, _maxSimultaneousConnections,     \
                          _supportedFeatures, _hfConfig,                              \
                          _atResponseTime, _hfSupportedHfIndicators,                  \
                          _hfSupportedHfIndicatorsCount)                              \
    CsrBtHfActivateReqSend(TrapToOxygenTask(_task), _maxNumberOfHfConnections,     \
                           _maxNumberOfHsConnections, _maxSimultaneousConnections, \
                           _supportedFeatures, _hfConfig,                          \
                           _atResponseTime, _hfSupportedHfIndicators,              \
                           _hfSupportedHfIndicatorsCount)

#define HfDeactivateReqSend()                                                         \
    CsrBtHfDeactivateReqSend()

#define HfServiceConnectReqSend(_deviceAddr, _connectionType)                         \
    CsrBtHfServiceConnectReqSend(_deviceAddr, _connectionType)

#define HfCancelConnectReqSend(_deviceAddr)                                           \
    CsrBtHfCancelConnectReqSend(_deviceAddr)

#define HfDisconnectReqSend(_connectionId)                                            \
    CsrBtHfDisconnectReqSend(_connectionId)

#define HfCallEndReqSend(_connectionId)                                               \
    CsrBtHfCallEndReqSend(_connectionId)

#define HfAnswerReqSend(_connectionId)                                                \
    CsrBtHfAnswerReqSend(_connectionId)

#define HfAudioConnectReqSend(_connectionId, _audioParametersLength,                  \
                              _audioParameters, _pcmSlot, _pcmRealloc)                \
    CsrBtHfAudioConnectReqSend(_connectionId, _audioParametersLength,                 \
                               _audioParameters, _pcmSlot, _pcmRealloc)

#define HfAudioConfigReqSend(_connectionId, _audioType,                               \
                             _audioSettingLen, _audioSetting)                         \
    CsrBtHfAudioConfigReqSend(_connectionId, _audioType,                              \
                             _audioSetting, _audioSettingLen)

#define HfAudioAcceptResSend(_connectionId, _acceptResponse,                          \
                             _acceptParameters, _pcmSlot, _pcmReassign)               \
    CsrBtHfAudioAcceptResSend(_connectionId, _acceptResponse,                         \
                              _acceptParameters, _pcmSlot, _pcmReassign)

#define HfAudioDisconnectReqSend(_connectionId, _scoHandle)                           \
    CsrBtHfAudioDisconnectReqSend(_connectionId, _scoHandle)

#define HfAtCmdReqSend(_connectionId, _len, _payload)                                 \
    CsrBtHfAtCmdReqSend(_len, _payload, _connectionId)

#define HfSetCallNotificationIndicationReqSend(_connectionId, _enable)                \
    CsrBtHfSetCallNotificationIndicationReqSend(_connectionId, _enable)

#define HfSetCallWaitingNotificationReqSend(_connectionId, _enable)                   \
    CsrBtHfSetCallWaitingNotificationReqSend(_connectionId, _enable)

#define HfSetEchoAndNoiseReqSend(_connectionId, _enable)                              \
    CsrBtHfSetEchoAndNoiseReqSend(_connectionId, _enable)

#define HfSetVoiceRecognitionReqSend(_connectionId, _value)                           \
    CsrBtHfSetVoiceRecognitionReqSend(_connectionId, _value)

#define HfSpeakerGainStatusReqSend(_connectionId, _gain)                              \
    CsrBtHfSpeakerGainStatusReqSend(_gain, _connectionId)

#define HfMicGainStatusReqSend(_connectionId, _gain)                                  \
    CsrBtHfMicGainStatusReqSend(_gain, _connectionId)

#define HfCallHandlingReqSend(_connectionId, _index, _command)                        \
    CsrBtHfCallHandlingReqSend(_command, _index, _connectionId)

#define HfSetExtendedAgErrorResultCodeReqSend(_connectionId, _enable)                 \
    CsrBtHfSetExtendedAgErrorResultCodeReqSend(_connectionId, _enable)

#define HfSetStatusIndicatorUpdateReqSend(_connectionId, _enable)                     \
    CsrBtHfSetStatusIndicatorUpdateReqSend(_connectionId, _enable)

#define HfDialReqSend(_connectionId, _command, _number)                               \
    CsrBtHfDialReqSend(_connectionId, _command, _number)

#define HfIndicatorActivationReqSend(_connectionId, _indicatorBitMask)                \
    CsrBtHfIndicatorActivationReqSend(_indicatorBitMask, _connectionId)

#define HfUpdateSupportedCodecReqSend(_codec, _enable, _update)                       \
    CsrBtHfUpdateSupportedCodecReqSend(_codec, _enable, _update)

#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
#define HfUpdateQceSupportReqSend(_codecMask, _enable)                                \
    CsrBtHfUpdateQceSupportReqSend(_codecMask, _enable)
#else
#define HfUpdateQceSupportReqSend(_codecMask, _enable)
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */

#define HfSetHfIndicatorValueReqSend(_connectionId, _indId, _value)                   \
    CsrBtHfSetHfIndicatorValueReqSend(_connectionId, _indId, _value)

#define HfUpdateScoHandleReq(_connectionId, _scoHandle)                               \
    CsrBtHfUpdateScoHandleRequest(_connectionId, _scoHandle)

#define HfUpdateScoHandleWithAddrReq(_deviceAddr, _scoHandle)                         \
    HfUpdateScoHandleWithAddrRequest(_deviceAddr, _scoHandle)

#define HfGetCurrentCallListReqSend(_connectionId)                                    \
    CsrBtHfGetCurrentCallListReqSend(_connectionId)

#define HfGetBdAddrFromConnectionIdReq(_connectionId, _deviceAddr)                    \
    HfGetBdAddrFromConnectionId(_connectionId, _deviceAddr)

#define HfFreeUpstreamMessageContents(_message)                                       \
    CsrBtHfFreeUpstreamMessageContents(CSR_BT_HF_PRIM, _message)

#define HfUpdateOptionalCodecRequest(_codecIdList, _codecCount)                        \
    HfUpdateOptionalCodecReqSend(_codecIdList, _codecCount)

#endif /* COMMON_SYNERGY_INC_HF_LIB_H_ */
