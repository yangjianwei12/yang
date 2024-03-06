#ifndef CSR_BT_HF_LIB_H__
#define CSR_BT_HF_LIB_H__

/******************************************************************************
 Copyright (c) 2002-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_profiles.h"
#include "csr_bt_hf_prim.h"
#include "csr_msg_transport.h"
#include "csr_bt_tasks.h"
#include "csr_pmem.h"
#include "csr_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Common put_message function to reduce code size */
void CsrBtHfMsgTransport(void* msg);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfServiceConnectReqSend
 *
 *  DESCRIPTION
 *        This request sends CSR_BT_HF_SERVICE_CONNECT_REQ primitive to HF and
 *        initiates a connection towards a device specified by the Bluetooth
 *        device address. The HF sends a CSR_BT_HF_SERVICE_CONNECT_CFM back to
 *        the initiator with the result of the connection attempt. For an
 *        incoming connection, the HF sends a CSR_BT_HF_SERVICE_CONNECT_IND instead.
 *        If requesting for HS connection, this request additionally imply a
 *        user action as cause of connection request. This request sets
 *        userAction to TRUE.
 *
 *    PARAMETERS
 *        deviceAddr:            address of device to connect to
 *        connectionType:        Indicates which type of connection that has been
 *                               established. A connection to a HFG is indicated
 *                               by the value CSR_BT_HF_CONNECTION_HF and a connection
 *                               to a HAG is indicated by CSR_BT_HF_CONNECTION_HS.
 *                               If the connect request fails an error code is
 *                               returned in the result parameter, with the connectionType
 *                               CSR_BT_HF_CONNECTION_UNKNOWN.
 *                               The value CSR_BT_HF_CONNECTION_UNKNOWN is used
 *                               if both HF and HS connections are allowed to be
 *                               established. The values used in the connectionType
 *                               parameter is defined in csr_bt_hf_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtHfServiceConnectReqSend(_deviceAddr, _connectionType) {    \
        CsrBtHfServiceConnectReq *msg = (CsrBtHfServiceConnectReq *)CsrPmemAlloc(sizeof(CsrBtHfServiceConnectReq)); \
        msg->type        = CSR_BT_HF_SERVICE_CONNECT_REQ;               \
        msg->deviceAddr  = _deviceAddr;                                 \
        msg->connectionType = _connectionType;                          \
        msg->userAction  = TRUE;                                        \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfServiceReconnectReqSend
 *
 *  DESCRIPTION
 *        For HF connection, this request is equivalent to CsrBtHfServiceConnectReqSend().
 *        If requesting HS connection, this request implies that it is a
 *        reconnection attempt due to some internal event to a known device and
 *        user actions are not involved.
 *        This request sets userAction to FALSE
 *
 *    PARAMETERS
 *        deviceAddr:            address of device to connect to
 *        connectionType:        Indicates which type of connection that has been
 *                               established. A connection to a HFG is indicated
 *                               by the value CSR_BT_HF_CONNECTION_HF and a connection
 *                               to a HAG is indicated by CSR_BT_HF_CONNECTION_HS.
 *                               If the connect request fails an error code is
 *                               returned in the result parameter, with the connectionType
 *                               CSR_BT_HF_CONNECTION_UNKNOWN.
 *                               The value CSR_BT_HF_CONNECTION_UNKNOWN is used
 *                               if both HF and HS connections are allowed to be
 *                               established. The values used in the connectionType
 *                               parameter is defined in csr_bt_hf_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtHfServiceReconnectReqSend(_deviceAddr, _connectionType) {    \
        CsrBtHfServiceConnectReq *msg = (CsrBtHfServiceConnectReq *)CsrPmemAlloc(sizeof(CsrBtHfServiceConnectReq)); \
        msg->type        = CSR_BT_HF_SERVICE_CONNECT_REQ;               \
        msg->deviceAddr  = _deviceAddr;                                 \
        msg->connectionType = _connectionType;                          \
        msg->userAction  = FALSE;                                       \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *  CsrBtHfActivateReqSend
 *
 *  DESCRIPTION
 *        Activate the HF service. This signal is used to allow remote devices
 *        to find and discover the HF service and subsequently connect to it.
 *        The HF sends a CSR_BT_HF_ACTIVATE_CFM to the initiator with the result
 *        of the activation attempt.
 *
 *        If device supports SWB LC3 codec, application needs to enable the codec
 *        before profile activation using CsrBtHfUpdateSupportedCodecReqSend.
 *
 *    PARAMETERS
 *        phandle:                    application handle
 *        maxNumberOfHfConnections    CSRMAX number of simultaneous Hf connections
 *        maxNumberOfHsConnections    CSRMAX number of simultaneous Hs connections
 *        maxSimultaneousConnections  CSRMAX total number of sim. connections
 *        supportedFeatures           bitmap with supported features
 *        hfConfig                    bitmap used to enable/disable additional features
 *        atResponseTime              Time in seconds to wait for response for a sent AT command.
 *        hfSupportedHfIndicators     pointer to list of local supported HF Indicators,
 *                                    Indicator ID's are 16 bit unsigned integer values
 *                                    defined on the Bluetooth SIG Assigned Number page.
 *       hfSupportedHfIndicatorsCount Number of HF Indicotrs in 'hfSupportedHfIndicators' list. 
 *----------------------------------------------------------------------------*/
#define CsrBtHfActivateReqSend(_phandle, _maxNumberOfHfConnections, _maxNumberOfHsConnections,           \
                               _maxSimultaneousConnections, _supportedFeatures, _hfConfig,                 \
                               _atResponseTime, _hfSupportedHfIndicators, _hfSupportedHfIndicatorsCount)   \
do{                                                                                                          \
        CsrBtHfActivateReq *msg = (CsrBtHfActivateReq *)CsrPmemAlloc(sizeof(CsrBtHfActivateReq));            \
        msg->type              = CSR_BT_HF_ACTIVATE_REQ;                                                     \
        msg->phandle           = _phandle;                                                                   \
        msg->supportedFeatures = _supportedFeatures;                                                         \
        msg->hfConfig          = _hfConfig;                                                                  \
        msg->maxHFConnections  = _maxNumberOfHfConnections;                                                  \
        msg->maxHSConnections  = _maxNumberOfHsConnections;                                                  \
        msg->maxSimultaneousConnections = _maxSimultaneousConnections;                                       \
        msg->atResponseTime    = _atResponseTime;                                                            \
        msg->hfSupportedHfIndicators = _hfSupportedHfIndicators;                                             \
        msg->hfSupportedHfIndicatorsCount = _hfSupportedHfIndicatorsCount;                                   \
        CsrBtHfMsgTransport(msg);                                                                            \
}while(0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfDeactivateReqSend
 *
 *  DESCRIPTION
 *        Deactivate the HF service. This signal deactivates the HF service and
 *        removes the service records. The HF sends a CSR_BT_HF_DEACTIVATE_CFM
 *        to the initiator with the result of the deactivation attempt.
 *
 *    PARAMETERS
 *----------------------------------------------------------------------------*/
#define CsrBtHfDeactivateReqSend() {                                    \
        CsrBtHfDeactivateReq *msg = (CsrBtHfDeactivateReq *)CsrPmemAlloc(sizeof(CsrBtHfDeactivateReq)); \
        msg->type = CSR_BT_HF_DEACTIVATE_REQ;                           \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfCancelConnectReqSend
 *
 *  DESCRIPTION
 *        Cancel an ongoing SLC connection. Action is taken depending upon the 
 *        state when the cancel signal is received i.e. if the RFC connection
 *        is already established it will be disconnected; else the ongoing
 *        connection operation will be cancelled. When the connection establishment 
 *        is stopped successfully, the profile returns a CSR_BT_HF_SERVICE_CONNECT_IND 
 *        message with an appropriate result code.
 *
 *    PARAMETERS
 *        deviceAddr:            address of device for which the SLC connection
 *                               should be cancelled
 *----------------------------------------------------------------------------*/
#define CsrBtHfCancelConnectReqSend(_deviceAddr) {                      \
        CsrBtHfCancelConnectReq *msg = (CsrBtHfCancelConnectReq *)CsrPmemAlloc(sizeof(CsrBtHfCancelConnectReq)); \
        msg->type        = CSR_BT_HF_CANCEL_CONNECT_REQ;                \
        msg->deviceAddr  = _deviceAddr;                                 \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfDisconnectReqSend
 *
 *  DESCRIPTION
 *        Disconnect a HF or HS connection, indicated in the connectionId parameter.
 *        Action is taken depending upon the state when the disconnect signal is 
 *        received i.e. if there is an ongoing connection, the connection operation
 *        will be cancelled. If a connection is established it will be disconnected.
 *        When the connection is closed, the profile returns the CSR_BT_HF_DISCONNECT_CFM 
 *        message with a result code and a connectionId parameter. If the connection is 
 *        disconnected without the application asking for it, the message used is the
 *        CSR_BT_HF_DISCONNECT_IND instead.
 *
 *    PARAMETERS
 *        connectionId:      The connection to use
 *----------------------------------------------------------------------------*/
#define CsrBtHfDisconnectReqSend(_connectionId) {                       \
        CsrBtHfDisconnectReq *msg = (CsrBtHfDisconnectReq *)CsrPmemAlloc(sizeof(CsrBtHfDisconnectReq)); \
        msg->type           = CSR_BT_HF_DISCONNECT_REQ;                 \
        msg->connectionId = _connectionId;                              \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfGetAllStatusIndicatorsReqSend
 *
 *  DESCRIPTION
 *        Retrieve all the status indicatiors for the given connection ID.
 *        The HF sends a CSR_BT_HF_GET_ALL_STATUS_INDICATORS_CFM with the 
 *        result.
 *
 *    PARAMETERS
 *        connectionId:      The connection to use
 *----------------------------------------------------------------------------*/
#define CsrBtHfGetAllStatusIndicatorsReqSend(_connectionId) {           \
        CsrBtHfGetAllStatusIndicatorsReq *msg = (CsrBtHfGetAllStatusIndicatorsReq *)CsrPmemAlloc(sizeof(CsrBtHfGetAllStatusIndicatorsReq)); \
        msg->type           = CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ;  \
        msg->connectionId = _connectionId;                              \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfCallEndReqSend
 *
 *  DESCRIPTION
 *        Reject an incoming call or disconnect an ongoing call. The application
 *        receives a CSR_BT_HF_CALL_END_CFM with the appropriate result code on
 *        completion of the procedure.
 *
 *    PARAMETERS
 *        connectionId:      The connection to use
 *----------------------------------------------------------------------------*/
#define CsrBtHfCallEndReqSend(_connectionId) {                          \
        CsrBtHfCallEndReq *msg = (CsrBtHfCallEndReq *)CsrPmemAlloc(sizeof(CsrBtHfCallEndReq)); \
        msg->type = CSR_BT_HF_CALL_END_REQ;                             \
        msg->connectionId = _connectionId;                              \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfAnswerReqSend
 *
 *  DESCRIPTION
 *        Accept an incoming call in the HF profile. On completion, the application
 *        receives a CSR_BT_HF_CALL_ANSWER_CFM with the appropriate result code.
 *
 *    PARAMETERS
 *        connectionId:      The connection to use
 *----------------------------------------------------------------------------*/
#define CsrBtHfAnswerReqSend(_connectionId) {                           \
        CsrBtHfCallAnswerReq *msg = (CsrBtHfCallAnswerReq *)CsrPmemAlloc(sizeof(CsrBtHfCallAnswerReq)); \
        msg->type           = CSR_BT_HF_CALL_ANSWER_REQ;                \
        msg->connectionId = _connectionId;                              \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfAudioConnectReqSend
 *
 *  DESCRIPTION
 *        Connect the audio path on the established service level connection.
 *
 *        The audio switching is confirmed by a CSR_BT_HF_AUDIO_CONNECT_CFM.
 *        HF sends result code "CSR_BT_RESULT_CODE_HF_SUCCESS" to application if
 *        audio connection is established successfully.
 *
 *        In case of failure, HF module's result codes (defined in csr_bt_hf_prim.h)
 *        are sent if audio connect request is failed or rejected at Handsfree level,
 *        else lower layer result codes are forwarded to application.
 *
 *        Note that HF sends CSR_BT_HF_AUDIO_CONNECT_IND for the incoming audio connection.
 *
 *    PARAMETERS
 *        connectionId:             connectionID to connect the audio path
 *        audioParametersLength:    number of entries in the audioParameters pointer
 *        audioParameters:          Specifies which SCO/eSCO parameters to use in the
 *                                  connection establishment.If NULL the default Audio
 *                                  parameters from csr_bt_usr_config.h or
 *                                  CSR_BT_HF_CONFIG_AUDIO_REQ are used
 *        pcmSlot:                  pcm slot to use
 *        pcmRealloc:               automatically reallocate another pcm-slot if pcmSlot
 *                                  given in this request is already in use. The resulting
 *                                  pcm-slot will be informed in the CSR_BT_HF_AUDIO_CONNECT_CFM
 *----------------------------------------------------------------------------*/
#define CsrBtHfAudioConnectReqSend(_connectionId, _audioParametersLength, _audioParameters, _pcmSlot, _pcmRealloc) { \
        CsrBtHfAudioConnectReq *msg = (CsrBtHfAudioConnectReq *)CsrPmemAlloc(sizeof(CsrBtHfAudioConnectReq)); \
        msg->type              = CSR_BT_HF_AUDIO_CONNECT_REQ;           \
        msg->connectionId      = _connectionId;                         \
        msg->audioParameters   = _audioParameters;                      \
        msg->audioParametersLength = _audioParametersLength;            \
        msg->pcmSlot           = _pcmSlot;                              \
        msg->pcmRealloc        = _pcmRealloc;                           \
        CsrBtHfMsgTransport(msg);}

#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfAudioConfigReqSend
 *
 *  DESCRIPTION
 *        Configure audio settings. The settings to configure are determined
 *        by the "audioType" parameter. The HF sends a CSR_BT_HF_CONFIG_AUDIO_CFM
 *        to the initiator with the result of the configuration attempt.
 *
 *    PARAMETERS
 *        connectionId:       connectionID to apply the settings to
 *        audioType:          type of settings to set
 *        audioSetting:       pointer to data
 *        audioSettingLen   : length in bytes of the data given
 *----------------------------------------------------------------------------*/
#define CsrBtHfAudioConfigReqSend(_connectionId, _audioType, _audioSetting, _audioSettingLen) { \
        CsrBtHfConfigAudioReq *msg = (CsrBtHfConfigAudioReq *)CsrPmemAlloc(sizeof(CsrBtHfConfigAudioReq)); \
        msg->type = CSR_BT_HF_CONFIG_AUDIO_REQ;                         \
        msg->connectionId       = _connectionId;                        \
        msg->audioType          = _audioType;                           \
        msg->audioSetting       = _audioSetting;                        \
        msg->audioSettingLen    = _audioSettingLen;                     \
        CsrBtHfMsgTransport(msg);}
#endif /* CSR_BT_INSTALL_HF_CONFIG_AUDIO */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfAudioAcceptResSend
 *
 *  DESCRIPTION
 *      Send a response to the incoming CSR_BT_HF_AUDIO_ACCEPT_CONNECT_IND
 *
 *    PARAMETERS
 *      connectionId:           connectionID to accept the audio connection
 *      acceptResponse:         The HCI response code from profile. If this is
 *                              != HCI_SUCCESS then the incoming SCO/eSCO
 *                              connection will be rejected
 *      acceptParameters:       Specifies which SCO/eSCO parameters accept.
 *                              If NULL the default Audio parameters are used from
 *                              csr_bt_usr_config.h or CSR_BT_HF_CONFIG_AUDIO_REQ
 *      pcmSlot:                pcm slot to use
 *      pcmReassign:            automatically assign another pcm-slot if pcmSlot
 *                              given in this response is already in use.
 *                              The resulting pcm-slot will be informed
 *                              in the CSR_BT_HF_AUDIO_CONNECT_IND
 *----------------------------------------------------------------------------*/
#define CsrBtHfAudioAcceptResSend(_connectionId, _acceptResponse, _acceptParameters, _pcmSlot, _pcmReassign) { \
        CsrBtHfAudioAcceptConnectRes *msg = (CsrBtHfAudioAcceptConnectRes *)CsrPmemAlloc(sizeof(CsrBtHfAudioAcceptConnectRes)); \
        msg->type           = CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES;       \
        msg->connectionId   = _connectionId;                            \
        msg->acceptResponse = _acceptResponse;                          \
        msg->acceptParameters = _acceptParameters;                      \
        msg->acceptParametersLength = (_acceptParameters ? 1 : 0);      \
        msg->pcmSlot        = _pcmSlot;                                 \
        msg->pcmReassign    = _pcmReassign;                             \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfAudioDisconnectReqSend
 *
 *  DESCRIPTION
 *        Disconnect the audio path on the established service level connection.
 *        The audio switching is confirmed by a CSR_BT_HF_AUDIO_DISCONNECT_CFM.
 *        However, if the remote device releases the audio connection, or due to 
 *        a link loss, the profile sends a CSR_BT_HF_DISCONNECT_IND message to 
 *        notify the application instead.
 *
 *    PARAMETERS
 *      connectionId:           The connection to use
 *      scoHandle:              sco handle if routed internally
 *----------------------------------------------------------------------------*/
#define CsrBtHfAudioDisconnectReqSend(_connectionId, _scoHandle) {      \
        CsrBtHfAudioDisconnectReq *msg = (CsrBtHfAudioDisconnectReq *)CsrPmemAlloc(sizeof(CsrBtHfAudioDisconnectReq)); \
        msg->type              = CSR_BT_HF_AUDIO_DISCONNECT_REQ;        \
        msg->connectionId      = _connectionId;                         \
        msg->scoHandle         = _scoHandle;                            \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfAtCmdReqSend
 *
 *  DESCRIPTION
 *        Send an at-command defined by the higher layer to HF.
 *        When the remote device responds with a result code to a command issued 
 *        by the local device, the HF sends a CSR_BT_HF_AT_CMD_CFM message to the application.
 *        When the remote device sends the response codes to the local device, the application
 *        receives a CSR_BT_HF_AT_CMD_IND containing a string with the response code.
 *
 *    PARAMETERS
 *        len:                number of chars in the payload.
 *        payload:            pointer to a at-command string. The pointer will be
 *                            handed over to the HF which eventually will free it.
 *        connectionId:       The connection through which to send the at command
 *----------------------------------------------------------------------------*/
#define CsrBtHfAtCmdReqSend(_len, _payload, _connectionId) {            \
        CsrBtHfAtCmdReq *msg = (CsrBtHfAtCmdReq *)CsrPmemAlloc(sizeof(CsrBtHfAtCmdReq)); \
        msg->type           = CSR_BT_HF_AT_CMD_REQ;                     \
        msg->atCmdString    = (CsrCharString *)_payload;                \
        msg->connectionId   = _connectionId;                            \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfGetSubscriberNumberInformationReqSend
 *
 *  DESCRIPTION
 *        Send the AT+CNUM at-command to the remote device to inquire about the 
 *        subscriber number information.
 *        For each subscriber identity received from the gateway, the HF delivers it to 
 *        the application in a separate CSR_BT_HF_GET_SUBSCRIBER_NUMBER_IND message.
 *        When the gateway sends "OK" or "ERROR", the HF sends a CSR_BT_HF_GET_SUBSCRIBER_NUMBER_CFM.
 *
 *    PARAMETERS
 *        connectionId:       The connection through which to send the at command
 *----------------------------------------------------------------------------*/
#define CsrBtHfGetSubscriberNumberInformationReqSend(_connectionId) {   \
        CsrBtHfGetSubscriberNumberInformationReq * msg = (CsrBtHfGetSubscriberNumberInformationReq *)CsrPmemAlloc(sizeof(CsrBtHfGetSubscriberNumberInformationReq)); \
        msg->type  = CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ;   \
        msg->connectionId  = _connectionId;                             \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfGetCurrentCallListReqSend
 *
 *  DESCRIPTION
 *        Send the AT+CLCC at-command to the remote device to get information about
 *        the calls present at the HFG and their respective status.
 *        For each call present at the HFG, the application receives a CSR_BT_HF_GET_CURRENT_CALL_LIST_IND
 *        with information about the call. When information about all existing calls has been received,
 *        it receives a CSR_BT_HF_GET_CURRENT_CALL_LIST_CFM message.
 *
 *    PARAMETERS
 *        connectionId:       The connection through which to send the at command
 *----------------------------------------------------------------------------*/
#define CsrBtHfGetCurrentCallListReqSend(_connectionId) {               \
        CsrBtHfGetCurrentCallListReq *msg = (CsrBtHfGetCurrentCallListReq *)CsrPmemAlloc(sizeof(CsrBtHfGetCurrentCallListReq)); \
        msg->type          = CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ;       \
        msg->connectionId  = _connectionId;                             \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfSetCallNotificationIndicationReqSend
 *
 *  DESCRIPTION
 *        Send the AT+CLIP at-command to the remote device to enable or disable
 *        the calling line identification feature. When the enable or disable 
 *        operation has been performed, the application receives a 
 *        CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_CFM with the corresponding 
 *        result code. On receipt of an incoming call, if the feature is enabled,
 *        the application receives a CSR_BT_HF_CALL_NOTIFICATION_IND message
 *        containing a string with the calling party’s information.
 *
 *    PARAMETERS
 *        connectionId:      The connection to use
 *        enable:            Call notification indication ON or OFF.
 *----------------------------------------------------------------------------*/
#define CsrBtHfSetCallNotificationIndicationReqSend(_connectionId, _enable) { \
        CsrBtHfSetCallNotificationIndicationReq *msg = (CsrBtHfSetCallNotificationIndicationReq *)CsrPmemAlloc(sizeof(CsrBtHfSetCallNotificationIndicationReq)); \
        msg->type          = CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ; \
        msg->connectionId  = _connectionId;                             \
        msg->enable        = _enable;                                   \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfSetCallWaitingNotificationReqSend
 *
 *  DESCRIPTION
 *        Send the AT+CCWA at-command to enable or disable the ability at the HFG to
 *        indicate that an incoming call is waiting while engaged in another call already.
 *        When the enable or disable operation has been performed, the application receives a 
 *        CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_CFM with the corresponding 
 *        result code. On receipt of a waiting  call, if the feature is enabled,
 *        the application receives a CSR_BT_HF_CALL_WAITING_NOTIFICATION_IND message
 *        containing a string with the calling party’s information.
 *
 *    PARAMETERS
 *        connectionId:      The connection to use
 *        enable:            Call waiting notification ON or OFF.
 *----------------------------------------------------------------------------*/
#define CsrBtHfSetCallWaitingNotificationReqSend(_connectionId, _enable) { \
        CsrBtHfSetCallWaitingNotificationReq *msg = (CsrBtHfSetCallWaitingNotificationReq *)CsrPmemAlloc(sizeof(CsrBtHfSetCallWaitingNotificationReq)); \
        msg->type          = CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ; \
        msg->connectionId  = _connectionId;                             \
        msg->enable        = _enable;                                   \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfSetEchoAndNoiseReqSend
 *
 *  DESCRIPTION
 *        Send the AT+NREC at-command to enable/disable the echo & noise reduction settings.
 *        Completion of the process is informed to the application by a 
 *        CSR_BT_HF_SET_ECHO_AND_NOISE_CFM.
 *
 *    PARAMETERS
 *        connectionId:      The connection to use
 *        enable:            Echo&Noise reduction ON or OFF.
 *----------------------------------------------------------------------------*/
#define CsrBtHfSetEchoAndNoiseReqSend(_connectionId,_enable) {          \
        CsrBtHfSetEchoAndNoiseReq *msg = (CsrBtHfSetEchoAndNoiseReq *)CsrPmemAlloc(sizeof(CsrBtHfSetEchoAndNoiseReq)); \
        msg->type          = CSR_BT_HF_SET_ECHO_AND_NOISE_REQ;          \
        msg->connectionId  = _connectionId;                             \
        msg->enable        = _enable;                                   \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfSetVoiceRecognitionReqSend
 *
 *  DESCRIPTION
 *        Send the AT+BVRA at-command to the remote device to control the voice
 *        recognition operations. The application receives a 
 *        CSR_BT_HF_SET_VOICE_RECOGNITION_CFM with the corresponding result code.
 *        The application receives a CSR_BT_HF_SET_VOICE_RECOGNITION_IND when HF 
 *        receives unsolicited +BVRA from HFG.
 *
 *    PARAMETERS
 *        connectionId:      The connection to use
 *        value:             Values can be [0/1/2]
 *                           0 : Request AG to disable Voice Recognition feature.
 *                           1 : Request AG to enable Voice Recognition feature.
 *                           2 : Indicates AG that HF is ready to accept audio
 *                           OR
 *                           Instructs AG to terminate audio output(if any) and
 *                           prepare AG for new audio input during an ongoing VR session.
 *----------------------------------------------------------------------------*/
#define CsrBtHfSetVoiceRecognitionReqSend(_connectionId, _value) {        \
        CsrBtHfSetVoiceRecognitionReq *msg = (CsrBtHfSetVoiceRecognitionReq *)CsrPmemAlloc(sizeof(*msg));  \
        msg->type          = CSR_BT_HF_SET_VOICE_RECOGNITION_REQ;         \
        msg->connectionId  = _connectionId;                               \
        msg->value         = _value;                                      \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfGenerateDTMFReqSend
 *
 *  DESCRIPTION
 *        Send the AT+VTS at-command to generate a DTMF tone during a call.
 *        When the operation is performed, the application receives a 
 *        CSR_BT_HF_GENERATE_DTMF_CFM message with an appropriate result code.
 *
 *    PARAMETERS
 *        connectionId:      The connection to use
 *        value:             DTMF tone/value to generate
 *----------------------------------------------------------------------------*/
#define CsrBtHfGenerateDTMFReqSend(_connectionId, _value) {             \
        CsrBtHfGenerateDtmfReq *msg = (CsrBtHfGenerateDtmfReq *)CsrPmemAlloc(sizeof(CsrBtHfGenerateDtmfReq)); \
        msg->type          = CSR_BT_HF_GENERATE_DTMF_REQ;               \
        msg->connectionId  = _connectionId;                             \
        msg->dtmf          = _value;                                    \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfSpeakerGainStatusReqSend
 *
 *  DESCRIPTION
 *        Send the AT+VGS at-command to change the speaker gain.
 *        When the operation is performed, the application receives the 
 *        CSR_BT_HF_SPEAKER_GAIN_STATUS_CFM message with the appropriate result code.
 *        The CSR_BT_HF_SPEAKER_GAIN_IND indicates a change in speaker volume from the HFG.
 *
 *    PARAMETERS
 *        gain:                New speaker gain.
 *        connectionId:        The connection to use
 *----------------------------------------------------------------------------*/
#define CsrBtHfSpeakerGainStatusReqSend(_gain, _connectionId) {         \
        CsrBtHfSpeakerGainStatusReq *msg = (CsrBtHfSpeakerGainStatusReq *)CsrPmemAlloc(sizeof(CsrBtHfSpeakerGainStatusReq)); \
        msg->type           = CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ;        \
        msg->gain           = _gain;                                    \
        msg->connectionId   = _connectionId;                            \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfMicGainStatusReqSend
 *
 *  DESCRIPTION
 *        Send the AT+VGM at-command to change the microphone gain.
 *        When the operation is performed, the application receives the 
 *        CSR_BT_HF_MIC_GAIN_STATUS_CFM message with the appropriate result code.
 *        The CSR_BT_HF_MIC_GAIN_IND indicates a change in microphone volume from the HFG.
 *
 *    PARAMETERS
 *        gain:                New microphone gain.
 *        connectionId:        The connection to use
 *----------------------------------------------------------------------------*/
#define CsrBtHfMicGainStatusReqSend(_gain, _connectionId) {             \
        CsrBtHfMicGainStatusReq *msg = (CsrBtHfMicGainStatusReq *)CsrPmemAlloc(sizeof(CsrBtHfMicGainStatusReq)); \
        msg->type           = CSR_BT_HF_MIC_GAIN_STATUS_REQ;            \
        msg->gain           = _gain;                                    \
        msg->connectionId   = _connectionId;                            \
        CsrBtHfMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfCallHandlingReqSend
 *
 *  DESCRIPTION
 *        The application can perform outgoing calls, answer or reject incoming calls,
 *        put calls on hold and/or retrieve them, and so on. The outcome of these
 *        operations is communicated to the application using a CSR_BT_HF_CALL_HANDLING_CFM.
 *
 *    PARAMETERS
 *        command:      chld/btrh command
 *        index:        request Id
 *        connectionId: connection where the request shall be sent to.
 *----------------------------------------------------------------------------*/
#define CsrBtHfCallHandlingReqSend(_command, _index, _connectionId) {   \
        CsrBtHfCallHandlingReq *msg = (CsrBtHfCallHandlingReq *) CsrPmemAlloc(sizeof(CsrBtHfCallHandlingReq)); \
        msg->type  = CSR_BT_HF_CALL_HANDLING_REQ;                       \
        msg->command = _command;                                        \
        msg->index = _index;                                            \
        msg->connectionId = _connectionId;                              \
        CsrBtHfMsgTransport(msg); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfCopsReqSend
 *
 *  DESCRIPTION
 *        Initiate cops procedure to request for the operator network name.
 *        Completion of the process is informed to the application by a 
 *        CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_CFM.
 *
 *  PARAMETERS
 *      mode:                   The mode of the current cop signal
 *      format:                 The format for later query results
 *      forceResendFormat:      If TRUE AT+COPS=<mode>,<format> will be sent regardless 
 *                              if it has already been sent once
 *      connectionId:           The connection through which to send the at command
 *----------------------------------------------------------------------------*/
#define CsrBtHfCopsReqSend(_mode, _format, _forceResendFormat, _connectionId) { \
        CsrBtHfGetCurrentOperatorSelectionReq *msg = (CsrBtHfGetCurrentOperatorSelectionReq*) CsrPmemAlloc(sizeof(CsrBtHfGetCurrentOperatorSelectionReq)); \
        msg->type    = CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ;    \
        msg->mode    = _mode;                                           \
        msg->format  = _format;                                         \
        msg->connectionId = _connectionId;                              \
        msg->forceResendingFormat = _forceResendFormat;                 \
        CsrBtHfMsgTransport(msg); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfSetExtendedAgErrorResultCodeReqSend
 *
 *  DESCRIPTION
 *        Send AT+CMEE=X to enable or disable the extended error
 *        feature in the remote device. Completion of the process is informed to
 *        the application by a CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_CFM.
 *
 *  PARAMETERS
 *      connectionId:           The connection through which to send the at command
 *      enable:                 Whether to enable or disable the extended error mode
 *----------------------------------------------------------------------------*/
#define CsrBtHfSetExtendedAgErrorResultCodeReqSend(_connectionId, _enable) { \
        CsrBtHfSetExtendedAgErrorResultCodeReq *msg = (CsrBtHfSetExtendedAgErrorResultCodeReq *)CsrPmemAlloc(sizeof(CsrBtHfSetExtendedAgErrorResultCodeReq)); \
        msg->type          = CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ; \
        msg->connectionId  = _connectionId;                             \
        msg->enable        = _enable;                                   \
        CsrBtHfMsgTransport(msg); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfSetStatusIndicatorUpdateReqSend
 *
 *  DESCRIPTION
 *      To enable or disable the status indication feature at the remote HFG device.
 *      Completion of the process is informed to the application by a 
 *      CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_CFM.
 *
 *  PARAMETERS
 *       connectionId:        The connection to use
 *       enable:              Enable/disable standard status indicator update
 *----------------------------------------------------------------------------*/
#define CsrBtHfSetStatusIndicatorUpdateReqSend(_connectionId,_enable) { \
        CsrBtHfSetStatusIndicatorUpdateReq *msg = (CsrBtHfSetStatusIndicatorUpdateReq *)CsrPmemAlloc(sizeof(CsrBtHfSetStatusIndicatorUpdateReq)); \
        msg->type          = CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ; \
        msg->connectionId  = _connectionId;                             \
        msg->enable        = _enable;                                   \
        CsrBtHfMsgTransport(msg); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfBtInputReqSend
 *
 *  DESCRIPTION
 *      Send the AT+BINP command to request data input from the AG (so far only
 *      phone number is specified: AT+BINP=1"). Application receives CSR_BT_HF_BT_INPUT_CFM
 *      on completion of the process.
 *
 *  PARAMETERS
 *       connectionId        The connection to use
 *       dataRequest         The type of data to request (only 1 specified so far)
 *----------------------------------------------------------------------------*/
#define CsrBtHfBtInputReqSend(_connectionId,_dataRequest) {             \
        CsrBtHfBtInputReq *msg = (CsrBtHfBtInputReq *)CsrPmemAlloc(sizeof(CsrBtHfBtInputReq)); \
        msg->type          = CSR_BT_HF_BT_INPUT_REQ;                    \
        msg->connectionId  = _connectionId;                             \
        msg->dataRequest   = _dataRequest;                              \
        CsrBtHfMsgTransport(msg); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfDialReqSend
 *
 *  DESCRIPTION
 *      Send the ATD command to the AG to make an outgoing call.
 *      Once the operation is performed, the application receives a CSR_BT_HF_DIAL_CFM 
 *      message with an appropriate result code.
 *
 *  PARAMETERS
 *       connectionId        The connection to use
 *       command             dial, memory-dial or re-dial
 *       number              number or memory index to dial
 *----------------------------------------------------------------------------*/
#define CsrBtHfDialReqSend(_connectionId,_command,_number) {            \
        CsrBtHfDialReq *msg = (CsrBtHfDialReq *)CsrPmemAlloc(sizeof(CsrBtHfDialReq)); \
        msg->type          = CSR_BT_HF_DIAL_REQ;                        \
        msg->connectionId  = _connectionId;                             \
        msg->command       = _command;                                  \
        msg->number = _number;                                          \
        CsrBtHfMsgTransport(msg); }


#ifdef INSTALL_HF_CUSTOM_SECURITY_SETTINGS

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfSecurityInReqSend
 *      CsrBtHfSecurityOutReqSend
 *
 *  DESCRIPTION
 *      Set the default security settings for new incoming/outgoing connections.
 *      CSR_BT_HF_SECURITY_IN_CFM is received on setting up security for
 *      incoming connections.
 *      CSR_BT_HF_SECURITY_OUT_CFM is received on setting up security for
 *      outgoing connections.
 *
 *  PARAMETERS
 *       secLevel:      Minimum incoming/outgoing security level requested by the application.
 *                      The application must specify one of the following values:
 *                      CSR_BT_SEC_DEFAULT:        Uses the default security settings.
 *                      CSR_BT_SEC_MANDATORY:      Uses the mandatory security settings.
 *                      CSR_BT_SEC_SPECIFY:        Specifies new security settings.
 *                      If CSR_BT_SEC_SPECIFY is set, the following values can be OR'ed additionally,
 *                      i.e., combinations of these values can be used:
 *                      CSR_BT_SEC_AUTHORISATION:  Requires authorisation.
 *                      CSR_BT_SEC_AUTHENTICATION: Requires authentication.
 *                      CSR_BT_SEC_SEC_ENCRYPTION: Requires encryption (implies authentication).
 *                      CSR_BT_SEC_MITM:           Requires MITM protection (implies encryption).
 *----------------------------------------------------------------------------*/
#define CsrBtHfSecurityInReqSend(_appHandle, _secLevel) {               \
        CsrBtHfSecurityInReq *msg = (CsrBtHfSecurityInReq*)CsrPmemAlloc(sizeof(CsrBtHfSecurityInReq)); \
        msg->type = CSR_BT_HF_SECURITY_IN_REQ;                          \
        msg->appHandle = _appHandle;                                    \
        msg->secLevel = _secLevel;                                      \
        CsrBtHfMsgTransport(msg);}

#define CsrBtHfSecurityOutReqSend(_appHandle, _secLevel) {              \
        CsrBtHfSecurityOutReq *msg = (CsrBtHfSecurityOutReq*)CsrPmemAlloc(sizeof(CsrBtHfSecurityOutReq)); \
        msg->type = CSR_BT_HF_SECURITY_OUT_REQ;                         \
        msg->appHandle = _appHandle;                                    \
        msg->secLevel = _secLevel;                                      \
        CsrBtHfMsgTransport(msg);}

#endif /* INSTALL_HF_CUSTOM_SECURITY_SETTINGS */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfFreeUpstreamMessageContents
 *
 *  DESCRIPTION
 *      During Bluetooth shutdown all allocated payload in the Synergy BT HF
 *      message must be deallocated. This is done by this function
 *
 *
 *    PARAMETERS
 *      eventClass :  Must be CSR_BT_HF_PRIM,
 *      message:      The message received from Synergy BT HF
 *----------------------------------------------------------------------------*/
void CsrBtHfFreeUpstreamMessageContents(CsrUint16 eventClass, void * message);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfSetDeregisterTimeReqSend
 *
 *  DESCRIPTION
 *    Set the number of seconds to wait before deregistering a service record
 *    when an incoming SLC is established. On completion of this procedure,
 *    CSR_BT_HF_DEREGISTER_TIME_CFM is received by the application.
 *
 *  PARAMETERS
 *      waitSeconds:            number of seconds
 *----------------------------------------------------------------------------*/
#define CsrBtHfSetDeregisterTimeReqSend(_waitSeconds) {                 \
        CsrBtHfDeregisterTimeReq *msg = (CsrBtHfDeregisterTimeReq *)CsrPmemAlloc(sizeof(CsrBtHfDeregisterTimeReq)); \
        msg->type  = CSR_BT_HF_DEREGISTER_TIME_REQ;                     \
        msg->waitSeconds = _waitSeconds;                                \
        CsrBtHfMsgTransport(msg); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfIndicatorActivationReqSend
 *
 *  DESCRIPTION
 *    Enable and/or disable determined indicator indications. On completion
 *    of this procedure, CSR_BT_HF_INDICATOR_ACTIVATION_CFM is received
 *    by the application.
 *
 *  PARAMETERS
 *     indicatorBitMask:         bitmask with the indicators to enable/disable
 *     connectionId              The connection to use
 *----------------------------------------------------------------------------*/
#define CsrBtHfIndicatorActivationReqSend(_indicatorBitMask,_connectionId) {\
        CsrBtHfIndicatorActivationReq *msg = (CsrBtHfIndicatorActivationReq *)CsrPmemAlloc(sizeof(CsrBtHfIndicatorActivationReq)); \
        msg->type  = CSR_BT_HF_INDICATOR_ACTIVATION_REQ;                \
        msg->indicatorBitMask = _indicatorBitMask;                      \
        msg->connectionId     = _connectionId;                          \
        CsrBtHfMsgTransport(msg); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfUpdateSupportedCodecReqSend
 *
 *  DESCRIPTION
 *      Add support for Codec specified. On completion of this procedure,
 *      CSR_BT_HF_UPDATE_SUPPORTED_CODEC_CFM is received by the application if HF service is
 *      activated by application through CsrBtHfActivateReqSend API.
 *
 *      This API is used to enable/disable SIG defined codecs, i.e. mSBC and LC3.
 *
 *  PARAMETERS
 *     codec:       Codec to enable/disable.
 *                  CSR_BT_WBS_MSBC_CODEC_MASK
 *                  CSR_BT_WBS_LC3SWB_CODEC_MASK
 *     enable:      TRUE/FALSE
 *     update:      Whether the codec update should be sent to the hfg or not,
 *                  always recommended on codec upgrade
 *----------------------------------------------------------------------------*/
#define CsrBtHfUpdateSupportedCodecReqSend(_codec, _enable, _update) {\
        CsrBtHfUpdateSupportedCodecReq *msg = (CsrBtHfUpdateSupportedCodecReq *)CsrPmemAlloc(sizeof(CsrBtHfUpdateSupportedCodecReq)); \
        msg->type  = CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ;                \
        msg->codecMask = _codec;                  \
        msg->enable = _enable;                    \
        msg->sendUpdate = _update;                    \
        CsrBtHfMsgTransport(msg); }

#ifdef HF_ENABLE_OPTIONAL_CODEC_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      HfUpdateOptionalCodecReqSend
 *
 *  DESCRIPTION
 *      Add support for optional codecs specified through "_codecIdList". Application
 *      can call this API before or after HF service activation. On completion of this
 *      procedure, HF_UPDATE_OPTIONAL_CODEC_CFM is received by the application if Hands-free
 *      service is already activated by the application through CsrBtHfActivateReqSend API.
 *
 *      On handling this request, available supported codec AT command would be sent to
 *      HFG with active Hands-free service connections.
 *      Do not remove support for a codec when selected for an active Synchronous connection.
 *
 *      This API is used to enable/disable any proprietary codecs to be negotiated over
 *      standard Hands-free AT commands.
 *
 *  PARAMETERS
 *     _codecIdList:    Pointer to list of optional codecs supported by the device.
 *     _codecCount:     Number of optional codecs in '_codecIdList' list.
 *----------------------------------------------------------------------------*/
#define HfUpdateOptionalCodecReqSend(_codecIdList, _codecCount) {\
        HfUpdateOptionalCodecReq *msg = (HfUpdateOptionalCodecReq *)CsrPmemAlloc(sizeof(HfUpdateOptionalCodecReq)); \
        msg->type  = HF_UPDATE_OPTIONAL_CODEC_REQ;                \
        msg->codecIdList = _codecIdList;                  \
        msg->codecCount = _codecCount;                    \
        CsrBtHfMsgTransport(msg); }
#endif

#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfUpdateQceSupportReqSend
 *
 *  DESCRIPTION
 *      Add support for Qualcomm Codec Extension
 *
 *  PARAMETERS
 *     codecMask:       Codec to enable/disable, See @CsrBtHfQceCodecMask
 *     enable:          TRUE/FALSE
 *----------------------------------------------------------------------------*/
#define CsrBtHfUpdateQceSupportReqSend(_codecMask, _enable) {                                                               \
        CsrBtHfUpdateQceSupportReq *msg = (CsrBtHfUpdateQceSupportReq *)CsrPmemAlloc(sizeof(CsrBtHfUpdateQceSupportReq));   \
        msg->type  = CSR_BT_HF_UPDATE_QCE_CODEC_REQ;                                                                        \
        msg->codecMask = _codecMask;                                                                                        \
        msg->enable = _enable;                                                                                              \
        CsrBtHfMsgTransport(msg); }
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfSetHfIndicatorValueReqSend
 *
 *  DESCRIPTION
 *      Send updated value of enabled HF indicator. On completion of this procedure,
 *      CSR_BT_HF_SET_HF_INDICATOR_VALUE_CFM is received by the application.
 *
 *  PARAMETERS
 *     connectionId The connection to use
 *     indId:       HF indicator assigned number,a 16 bit unsigned integer value
 *                  defined on the Bluetooth SIG Assigned Number page    
 *     value:       Indicator Value, a 16 bit unsigned integer value
 *                  defined on the Bluetooth SIG Assigned Number page.
 *----------------------------------------------------------------------------*/
#define CsrBtHfSetHfIndicatorValueReqSend(_connectionId, _indId, _value)                                                            \
do{                                                                                                                                 \
        CsrBtHfSetHfIndicatorValueReq *msg = (CsrBtHfSetHfIndicatorValueReq *)CsrPmemAlloc(sizeof(CsrBtHfSetHfIndicatorValueReq));  \
        msg->type = CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ;                                                                           \
        msg->connectionId = _connectionId;                                                                                          \
        msg->indId = _indId;                                                                                                        \
        msg->value = _value;                                                                                                        \
        CsrBtHfMsgTransport(msg);                                                                                                   \
}while(0)

#ifdef CSR_TARGET_PRODUCT_VM
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtHfUpdateScoHandleRequest
 *
 *  DESCRIPTION
 *      Synchronous API to update SCO handle for the connection id provided
 *
 *  PARAMETERS
 *      connectionId:   Connection id for which SCO handle to be updated
 *      scoHandle:      SCO handle to be used
 *----------------------------------------------------------------------------*/
void CsrBtHfUpdateScoHandle(CsrBtConnId connId,
                            hci_connection_handle_t scoHandle);

#define CsrBtHfUpdateScoHandleRequest(_connectionId, _scoHandle) \
do                                                               \
{                                                                \
    CsrBtHfUpdateScoHandle(_connectionId, _scoHandle);           \
} while (0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      HfUpdateScoHandleWithAddr
 *
 *  DESCRIPTION
 *      Synchronous API to update SCO handle for the device address provided
 *
 *  PARAMETERS
 *      addr:           Bdaddr of device for which SCO handle has to be updated
 *      scoHandle:      SCO handle to be used
 *----------------------------------------------------------------------------*/
void HfUpdateScoHandleWithAddr(CsrBtDeviceAddr addr, 
                               hci_connection_handle_t scoHandle);

#define HfUpdateScoHandleWithAddrRequest(_addr, _scoHandle) \
do                                                          \
{                                                           \
    HfUpdateScoHandleWithAddr(_addr, _scoHandle);           \
} while (0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      HfGetBdAddrFromConnectionId
 *
 *  DESCRIPTION
 *      Synchronous API to retrieve the connected remote device address for the requested connection ID.
 *
 *  PARAMETERS
 *     connectionId:   Connection identifier
 *     deviceAddr:     Out parameter to store the device address
 *----------------------------------------------------------------------------*/
CsrBtResultCode HfGetBdAddrFromConnectionId(CsrBtHfConnectionId connectionId,
                                            CsrBtDeviceAddr *deviceAddr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      HfGetLocalHfServerChannel
 *
 *  DESCRIPTION
 *      Synchronous API to retrieve local HF server channel.
 *
 *  PARAMETERS
 *     NA
 *----------------------------------------------------------------------------*/
CsrUint8 HfGetLocalHfServerChannel(void);

#endif /* CSR_TARGET_PRODUCT_VM */

#ifdef __cplusplus
}
#endif

#endif /* !CSR_BT_HF_LIB_H__ */

