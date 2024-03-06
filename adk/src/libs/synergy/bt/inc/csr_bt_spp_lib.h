#ifndef CSR_BT_SPP_LIB_H__
#define CSR_BT_SPP_LIB_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_bt_spp_prim.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_bt_tasks.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Common put_message function to reduce code size */
void CsrBtSppMsgTransport(CsrSchedQid appHandle, void* msg);

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppConnectReqSend
 *
 *  DESCRIPTION
 *        Initiate a connection towards a device specified by the Bluetooth device address. 
 *        SPP profile starts service discovery to retrieve the remote data terminal service name(s), 
 *        indicated with CSR_BT_SPP_SERVICE_NAME_IND. The application must then select the service name
 *        it wants to initiate a connection to by sending a CSR_BT_SPP_SERVICE_NAME_RES. 
 *        Completion of the connection is indicated with a CSR_BT_SPP_CONNECT_IND to the initiator,
 *        with the result of the connection attempt.
 *
 *  PARAMETERS
 *        queueId:            queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        appHandle:          control application handle to return signal to
 *        deviceAddr:         address of device to connect to
 *        requestPortPar:     request synergy connection manager to execute port negotiation
 *        portPar:            parameters to send to peer entity, NULL if parameters
 *                            are not valid and port negotiation is not executed
 *                            NOTE: the lib function will copy the portPar so the
 *                            app layer must free the parameter itself (if applicable)
 *        role:               is the role CSR_BT_DTE or CSR_BT_DCE
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppConnectReqSend(_queueId, _appHandle, _deviceAddr, _requestPortPar, _portPar, _role) { \
        CsrBtSppConnectReq *msg = (CsrBtSppConnectReq *)CsrPmemAlloc(sizeof(CsrBtSppConnectReq)); \
        msg->type           = CSR_BT_SPP_CONNECT_REQ;                   \
        msg->phandle        = _appHandle;                               \
        msg->deviceAddr     = _deviceAddr;                              \
        msg->role           = _role;                                    \
        msg->requestPortPar = _requestPortPar;                          \
        if (_portPar != NULL)                                           \
        {                                                               \
            msg->validPortPar = TRUE;                                   \
            msg->portPar      = *((RFC_PORTNEG_VALUES_T *) _portPar);   \
        }                                                               \
        else                                                            \
        {                                                               \
            CsrBtPortParDefault(&(msg->portPar) );                      \
            msg->validPortPar = FALSE;                                  \
        }                                                               \
        CsrBtSppMsgTransport(_queueId,msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppServiceNameResSend
 *
 *  DESCRIPTION
 *      Give response to a received servicename indication.
 *      During outgoing connection, SPP profile starts service discovery,
 *      to retrieve the remote data terminal service name(s), indicated with CSR_BT_SPP_SERVICE_NAME_IND.
 *      The application must select a service name it wants to connect to,
 *      by sending a CSR_BT_SPP_SERVICE_NAME_RES.
 *
 *  PARAMETERS
 *        queueId:          queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        connect:          TRUE will start initializing a connection, FALSE will
 *                          cancel connect procedure
 *        theServiceHandle: 32-bit service record handle, which is attached to the service name the application connects to.
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppServiceNameResSend(_queueId, _connect, _theServiceHandle) { \
        CsrBtSppServiceNameRes *msg = (CsrBtSppServiceNameRes *)CsrPmemAlloc(sizeof(CsrBtSppServiceNameRes)); \
        msg->type = CSR_BT_SPP_SERVICE_NAME_RES;                        \
        msg->accept = _connect;                                         \
        msg->serviceHandle = _theServiceHandle;                         \
        CsrBtSppMsgTransport(_queueId,msg);}
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

#ifndef CSR_STREAMS_ENABLE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppDataReqSend
 *
 *  DESCRIPTION
 *      The SPP will forward the received data from application to the lower layers of the Bluetooth stack,
 *      for transmission to the data terminal side. Once the data is sent to the lower layers,
 *      a CSR_BT_SPP_DATA_CFM is issued to the application layer, which is now allowed to send a new data packet.
 *      Note that the CSR_BT_SPP_DATA_CFM does not imply that the data has been received on the data terminal side.
 *
 *  PARAMETERS
 *        queueId:          queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        theServerChannel: local server channel number
 *        thePayloadLength: number of data bytes in payload data
 *        *payload:         pointer to allocated payload data
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppDataReqSend(_queueId, _theServerChannel, _thePayloadLength, _thePayload) { \
        CsrBtSppDataReq *msg = (CsrBtSppDataReq *)CsrPmemAlloc(sizeof(CsrBtSppDataReq)); \
        msg->type = CSR_BT_SPP_DATA_REQ;                                \
        msg->serverChannel = _theServerChannel;                         \
        msg->payloadLength = _thePayloadLength;                         \
        msg->payload = _thePayload;                                     \
        CsrBtSppMsgTransport(_queueId,msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppDataResSend
 *
 *  DESCRIPTION
 *      Give response to received data through CSR_BT_SPP_DATA_IND from SPP.
 *      The CSR_BT_SPP_DATA_RES is used for informing the SPP that data is handled 
 *      and that the received data frame can be released.
 *
 *  PARAMETERS
 *        queueId:          queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        theServerChannel: local server channel number
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppDataResSend(_queueId, _theServerChannel) {              \
        CsrBtSppDataRes *msg = (CsrBtSppDataRes *)CsrPmemAlloc(sizeof(CsrBtSppDataRes)); \
        msg->type = CSR_BT_SPP_DATA_RES;                                \
        msg->serverChannel = _theServerChannel;                         \
        CsrBtSppMsgTransport(_queueId,msg);}

/*---------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppRegisterDataPathHandleReqSend
 *
 *  DESCRIPTION
 *      Register special application handle to which all future data-related messages 
 *      will be sent to instead of the standard application handle. 
 *      SPP returns the result in CSR_BT_SPP_REGISTER_DATA_PATH_HANDLE_CFM.
 *
 *  PARAMETERS
 *        queueId:           queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        dataHandle:        data application handle
 *
 *---------------------------------------------------------------------------*/
#define CsrBtSppRegisterDataPathHandleReqSend(_queueId, _dataHandle) {  \
        CsrBtSppRegisterDataPathHandleReq *msg = (CsrBtSppRegisterDataPathHandleReq*)CsrPmemAlloc(sizeof(CsrBtSppRegisterDataPathHandleReq)); \
        msg->type = CSR_BT_SPP_REGISTER_DATA_PATH_HANDLE_REQ;           \
        msg->dataAppHandle = _dataHandle;                               \
        CsrBtSppMsgTransport(_queueId,msg);}

/*---------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppDataPathStatusReqSend
 *
 *  DESCRIPTION
 *      When a data path has been registered using the CSR_BT_SPP_REGISTER_DATA_PATH_HANDLE_REQ/CFM signals,
 *      data application may need to notify the control application of changes with regard to data
 *      availability (open, closed or lost), data application requests SPP (this request) and
 *      SPP notifies control application through CSR_BT_SPP_DATA_PATH_STATUS_IND.
 *
 *  PARAMETERS
 *     queueId:           queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *     status:            new data application status
 *
 *---------------------------------------------------------------------------*/
#define CsrBtSppDataPathStatusReqSend(_queueId, _status) {              \
        CsrBtSppDataPathStatusReq *msg = (CsrBtSppDataPathStatusReq*)CsrPmemAlloc(sizeof(CsrBtSppDataPathStatusReq)); \
        msg->type = CSR_BT_SPP_DATA_PATH_STATUS_REQ;                    \
        msg->status = _status;                                          \
        CsrBtSppMsgTransport(_queueId,msg);}
#endif /* !CSR_STREAMS_ENABLE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppDisconnectReqSend
 *
 *  DESCRIPTION
 *      Release a connection between the local and remote device.
 *      CSR_BT_SPP_DISCONNECT_IND with result is returned to the initiator.
 *
 *  PARAMETERS
 *        queueId:          queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        theServerChannel: local server channel number
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppDisconnectReqSend(_queueId, _theServerChannel) {        \
        CsrBtSppDisconnectReq *msg = (CsrBtSppDisconnectReq *)CsrPmemAlloc(sizeof(CsrBtSppDisconnectReq)); \
        msg->type = CSR_BT_SPP_DISCONNECT_REQ;                          \
        msg->serverChannel = _theServerChannel;                         \
        CsrBtSppMsgTransport(_queueId,msg);}

#ifdef INSTALL_SPP_MODEM_STATUS_COMMAND
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppControlReqSend
 *
 *  DESCRIPTION
 *      Send local modem status information to the remote side.
 *
 *  PARAMETERS
 *        queueId:          queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        theServerChannel: local server channel number
 *        theModemStatus:   modemstatus
 *        theBreakSignal:   modem break signal
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppControlReqSend(_queueId, _theServerChannel, _theModemStatus, _theBreakSignal) { \
        CsrBtSppControlReq *msg = (CsrBtSppControlReq *)CsrPmemAlloc(sizeof(CsrBtSppControlReq)); \
        msg->type = CSR_BT_SPP_CONTROL_REQ;                             \
        msg->serverChannel = _theServerChannel;                         \
        msg->modemstatus = _theModemStatus;                             \
        msg->break_signal = _theBreakSignal;                            \
        CsrBtSppMsgTransport(_queueId,msg);}
#endif /* INSTALL_SPP_MODEM_STATUS_COMMAND */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppActivateReqSend
 *
 *  DESCRIPTION
 *      This signal is used to activate a service and make it accessible from a
 *      remote device. The application receives CSR_BT_SPP_ACTIVATE_CFM
 *      and it remains connectable until remote device has connected or 
 *      timer expires (on page scan timeout).
 *      Note: The CSR_BT_SPP_ACTIVATE_REQ signal may fail in which case the 
 *      application does not receive a CSR_BT_SPP_ACTIVATE_CFM signal. 
 *      However, a CSR_BT_SPP_CONNECT_IND signal is sent to notify that 
 *      the request has failed.
 *
 *  PARAMETERS
 *        queueId:        queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        appHandle:      control application handle
 *        timeout:        holds the pageScanTimeOut time, in number of seconds, 
 *                        where the service should be available for remote connections.
 *                        A timeout value of 0 (zero) or the define CSR_BT_INFINITE_TIME 
 *                        in the file csr_bt_profiles.h indicates an infinity timeout
 *                        time. When a connection is made, the service available stops.
 *        role:           is the role CSR_BT_DTE or CSR_BT_DCE
 *        serviceName:    '/0' char terminated name of the service to be registered
 *                        in sds record. If NULL the service name string is empty.
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppActivateReqSend(_queueId, _appHandle, _theTimeout, _role, _serviceName) { \
        CsrBtSppActivateReq *msg = (CsrBtSppActivateReq *)CsrPmemAlloc(sizeof(CsrBtSppActivateReq)); \
        msg->type = CSR_BT_SPP_ACTIVATE_REQ;                            \
        msg->phandle = _appHandle;                                      \
        msg->timeout = _theTimeout;                                     \
        msg->role = _role;                                              \
        msg->serviceName = _serviceName;                                \
        CsrBtSppMsgTransport(_queueId,msg);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppDeactivateReqSend
 *
 *  DESCRIPTION
 *      Deactivate a service that has been activated previously.
 *      The service will no longer be connectable.
 *      The application will receive CSR_BT_SPP_DEACTIVATE_CFM on deactivation.
 *
 *  PARAMETERS
 *        queueId:        queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        appHandle:      control application handle
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppDeactivateReqSend(_queueId, _appHandle) {               \
        CsrBtSppDeactivateReq *msg = (CsrBtSppDeactivateReq *)CsrPmemAlloc(sizeof(CsrBtSppDeactivateReq)); \
        msg->type = CSR_BT_SPP_DEACTIVATE_REQ;                          \
        msg->phandle = _appHandle;                                      \
        CsrBtSppMsgTransport(_queueId,msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppPortnegResSend
 *
 *  DESCRIPTION
 *      If the remote device changes the port settings or requests the current settings, 
 *      the SPP will send a CSR_BT_SPP_PORTNEG_IND to the application. 
 *      The application shall then answer with a CSR_BT_SPP_PORTNEG_RES.
 *
 *  PARAMETERS
 *        queueId:       queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        serverChannel: local server channel number
 *        portPar:       port negotiation parameters
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppPortnegResSend(_queueId, _serverChannel, _portPar) {    \
        CsrBtSppPortnegRes *msg = (CsrBtSppPortnegRes *)CsrPmemAlloc(sizeof(CsrBtSppPortnegRes)); \
        msg->type = CSR_BT_SPP_PORTNEG_RES;                             \
        msg->serverChannel = _serverChannel;                            \
        msg->portPar = *((RFC_PORTNEG_VALUES_T *) _portPar);            \
        CsrBtSppMsgTransport(_queueId,msg);}

#ifdef INSTALL_SPP_REMOTE_PORT_NEGOTIATION
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppPortnegReqSend
 *
 *  DESCRIPTION
 *      Send port negotiation request to change port settings.
 *      Once the operation is performed application will receive a
 *      CSR_BT_SPP_PORTNEG_CFM with the outcome.

 *
 *  PARAMETERS
 *        queueId:       queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *        serverChannel: local server channel number
 *        portPar:       port negotiation parameters
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppPortnegReqSend(_queueId, _serverChannel, _portPar) {    \
        CsrBtSppPortnegReq *msg = (CsrBtSppPortnegReq*)CsrPmemAlloc(sizeof(CsrBtSppPortnegReq)); \
        msg->type = CSR_BT_SPP_PORTNEG_REQ;                             \
        msg->serverChannel = _serverChannel;                            \
        msg->portPar = *((RFC_PORTNEG_VALUES_T *) _portPar);            \
        CsrBtSppMsgTransport(_queueId,msg);}
#endif /* INSTALL_SPP_REMOTE_PORT_NEGOTIATION */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppGetInstancesQidReqSend
 *
 *  DESCRIPTION
 *      This signal is used to get the list of registered SPP instances
 *      from the SPP instance that is also running as SPP-manager. 
 *      The SPP manager returns registered SPP instances in CSR_BT_SPP_GET_INSTANCES_QID_CFM.
 *
 *  PARAMETERS
 *        appHandle:    Application handle, so the SPP-manager
 *                      knows where to return the result to.
 *---------------------------------------------------------------------------*/
#define CsrBtSppGetInstancesQidReqSend(_appHandle) {                    \
        CsrBtSppGetInstancesQidReq *msg = (CsrBtSppGetInstancesQidReq *)CsrPmemAlloc(sizeof(CsrBtSppGetInstancesQidReq)); \
        msg->type = CSR_BT_SPP_GET_INSTANCES_QID_REQ;                   \
        msg->phandle = _appHandle;                                      \
        CsrBtSppMsgTransport(CSR_BT_SPP_IFACEQUEUE,msg);}

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
/*---------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppCancelConnectReqSend
 *
 *  DESCRIPTION
 *      Request SPP to cancel the ongoing connection establishment.
 *      When connection has been cancelled, SPP will send CSR_BT_SPP_CONNECT_IND,
 *      with resultCode CSR_BT_RESULT_CODE_SPP_CANCELLED_CONNECT_ATTEMPT.
 *      If a CSR_BT_SPP_CONNECT_IND has been sent before the connection could be cancelled, 
 *      CSR_BT_SPP_CANCEL_CONNECT_REQ will cause the SPP to disconnect the connection to the device and 
 *      send a CSR_BT_SPP_DISCONNECT_IND with resultcode CSR_BT_RESULT_CODE_CM_CANCELLED.
 *
 *  PARAMETERS
 *      queueId:        queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *
 *---------------------------------------------------------------------------*/
#define CsrBtSppCancelConnectReqSend(_queueId) {                        \
        CsrBtSppCancelConnectReq *msg = (CsrBtSppCancelConnectReq*)CsrPmemAlloc(sizeof(CsrBtSppCancelConnectReq)); \
        msg->type = CSR_BT_SPP_CANCEL_CONNECT_REQ;                      \
        CsrBtSppMsgTransport(_queueId,msg);}
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppSecurityInReqSend
 *      CsrBtSppSecurityOutReqSend
 *
 *  DESCRIPTION
 *      Set the default security settings for new incoming/outgoing connections
 *
 *  PARAMETERS
 *      queueId:          queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *      appHandle:        queue id of the sending task which will receive the CFM
 *      secLevel:         The security level to use
 *                        The application must specify one of the following values:
 *                        1. CSR_BT_SEC_DEFAULT : Use default security settings
 *                        2. CSR_BT_SEC_MANDATORY : Use mandatory security settings
 *                        3. CSR_BT_SEC_SPECIFY : Specify new security settings
 *
 *                        If CSR_BT_SEC_SPECIFY is set, the following values can be OR'ed additionally:
 *                        1. CSR_BT_SEC_AUTHORISATION: Require authorisation
 *                        2. CSR_BT_SEC_AUTHENTICATION: Require authentication
 *                        3. CSR_BT_SEC_SEC_ENCRYPTION: Require encryption (implies authentication)
 *                        4. CSR_BT_SEC_MITM: Require MITM protection (implies encryption)
 *                        A default security level (CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_OUTGOING or
                          CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_INCOMING) is defined in csr_bt_usr_config.h.
 *----------------------------------------------------------------------------*/
#define CsrBtSppSecurityInReqSend(_queueId, _app, _sec) {               \
        CsrBtSppSecurityInReq *msg = (CsrBtSppSecurityInReq*)CsrPmemAlloc(sizeof(CsrBtSppSecurityInReq)); \
        msg->type = CSR_BT_SPP_SECURITY_IN_REQ;                         \
        msg->appHandle = _app;                                          \
        msg->secLevel = _sec;                                           \
        CsrBtSppMsgTransport(_queueId,msg);}

#define CsrBtSppSecurityOutReqSend(_queueId, _app, _sec) {              \
        CsrBtSppSecurityOutReq *msg = (CsrBtSppSecurityOutReq*)CsrPmemAlloc(sizeof(CsrBtSppSecurityOutReq)); \
        msg->type = CSR_BT_SPP_SECURITY_OUT_REQ;                        \
        msg->appHandle = _app;                                          \
        msg->secLevel = _sec;                                           \
        CsrBtSppMsgTransport(_queueId,msg);}

#endif /* INSTALL_SPP_CUSTOM_SECURITY_SETTINGS */

#ifdef CSR_BT_INSTALL_SPP_EXTENDED
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppAudioConnectReqSend
 *
 *  DESCRIPTION
 *      Application requests SPP to establish the SCO connection after SPP connection is established.
 *      SPP sends CSR_BT_SPP_AUDIO_CONNECT_CFM with result to the application.
 *
 *  PARAMETERS
 *      queueId:               queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *      theServerChannel:      local server channel number
 *      audioParametersLength: number of entries in the audioParameters pointer.
 *      audioParameters:       Specifies which SCO/eSCO parameters to use in the connection establishment,
 *                             If NULL default Audio parameters (CSR_BT_ESCO_DEFAULT_CONNECT_XXX parameters), 
 *                             from csr_bt_usr_config.h are used.
 *      pcmSlot:               PCM slot to use
 *      pcmReassign:           Automatically assign another PCM-slot if given pcmSlot is already in use.
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppAudioConnectReqSend(_queueId, _theServerChannel, _audioParametersLength, _audioParameters, _pcmSlot, _pcmReassign) { \
        CsrBtSppAudioConnectReq *msg = (CsrBtSppAudioConnectReq *)CsrPmemAlloc(sizeof(CsrBtSppAudioConnectReq)); \
        msg->type = CSR_BT_SPP_AUDIO_CONNECT_REQ;                       \
        msg->serverChannel = _theServerChannel;                         \
        msg->audioParameters = _audioParameters;                        \
        msg->audioParametersLength = _audioParametersLength;            \
        msg->pcmSlot = _pcmSlot;                                        \
        msg->pcmReassign = _pcmReassign;                                \
        CsrBtSppMsgTransport(_queueId,msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppAudioAcceptConnectResSend
 *
 *  DESCRIPTION
 *      On incoming SCO/eSCO connection, CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_IND is sent to application.
 *      The application responds with CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES.
 *
 *  PARAMETERS
 *      queueId:               queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *      theServerChannel:      local server channel number
 *      acceptResponse:        HCI response code, 
 *                             if this is != HCI_SUCCESS then the incoming SCO/eSCO connection will be rejected
 *      acceptParameters:      Specifies which SCO/eSCO parameters to accept,
 *                             If NULL default ACCEPT SCO parameters given in CsrBtSppAcceptAudioReq will be used.
 *      pcmSlot:               PCM slot to use
 *      pcmReassign:           Automatically assign another PCM-slot if given pcmSlot is already in use.
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppAudioAcceptConnectResSend(_queueId, _theServerChannel, _acceptResponse, _acceptParameters, _pcmSlot, _pcmReassign) { \
        CsrBtSppAudioAcceptConnectRes *msg = (CsrBtSppAudioAcceptConnectRes *)CsrPmemAlloc(sizeof(CsrBtSppAudioAcceptConnectRes)); \
        msg->type = CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES;                \
        msg->serverChannel = _theServerChannel;                         \
        msg->acceptParameters = _acceptParameters;                      \
        msg->acceptParametersLength = (_acceptParameters ? 1 : 0);      \
        msg->acceptResponse = _acceptResponse;                          \
        msg->pcmSlot = _pcmSlot;                                        \
        msg->pcmReassign = _pcmReassign;                                \
        CsrBtSppMsgTransport(_queueId,msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppAcceptAudioReqSend
 *
 *  DESCRIPTION
 *      Request to accept incoming SCO/eSCO connection.
 *      SPP sends CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_IND to application,
 *      on establishment of incoming SCO/eSCO connection.
 *
 *  PARAMETERS
 *      queueId:               queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *      theServerChannel:      local server channel number
 *      theAudioQuality:       HCI packet type (refer ESCO packet types in hci.h) 
 *      theTxBandwidth:        Transmit bandwidth
 *      theRxBandwidth:        Receive bandwidth
 *      theMaxLatency:         Maximum latency
 *      theVoiceSettings:      voice coding used in the transmission. For example,. CSVD or Transparent.
 *      theReTxEffort:         Retransmission setting to use
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppAcceptAudioReqSend(_queueId, _theServerChannel, _theAudioQuality, _theTxBandwidth, _theRxBandwidth, _theMaxLatency, _theVoiceSettings, _theReTxEffort) { \
        CsrBtSppAcceptAudioReq *msg = (CsrBtSppAcceptAudioReq *)CsrPmemAlloc(sizeof(CsrBtSppAcceptAudioReq)); \
        msg->type = CSR_BT_SPP_ACCEPT_AUDIO_REQ;                        \
        msg->serverChannel = _theServerChannel;                         \
        msg->audioQuality = _theAudioQuality;                           \
        msg->txBandwidth = _theTxBandwidth;                             \
        msg->rxBandwidth = _theRxBandwidth;                             \
        msg->maxLatency = _theMaxLatency;                               \
        msg->voiceSettings = _theVoiceSettings;                         \
        msg->reTxEffort = _theReTxEffort;                               \
        CsrBtSppMsgTransport(_queueId,msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppAudioRenegotiateReqSend
 *
 *  DESCRIPTION
 *      Request to renegotiate SCO/eSCO connection. SPP sends CSR_BT_SPP_AUDIO_RENEGOTIATE_CFM,
 *      to the application with the result.
 *
 *  PARAMETERS
 *      queueId:               queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *      theServerChannel:      local server channel number
 *      theAudioQuality:       HCI packet type (refer ESCO packet types in hci.h) 
 *      theMaxLatency:         Maximum latency
 *      theReTxEffort:         Retransmission setting to use
 *      theScoHandle:          SCO handle
 *
 *----------------------------------------------------------------------------*/
#define CsrBtSppAudioRenegotiateReqSend(_queueId, _theServerChannel, _theAudioQuality, _theMaxLatency, _theReTxEffort, _theScoHandle) { \
        CsrBtSppAudioRenegotiateReq *msg = (CsrBtSppAudioRenegotiateReq *)CsrPmemAlloc(sizeof(CsrBtSppAudioRenegotiateReq)); \
        msg->type = CSR_BT_SPP_AUDIO_RENEGOTIATE_REQ;                   \
        msg->serverChannel = _theServerChannel;                         \
        msg->audioQuality = _theAudioQuality;                           \
        msg->maxLatency = _theMaxLatency;                               \
        msg->reTxEffort = _theReTxEffort;                               \
        msg->scoHandle = _theScoHandle;                                 \
        CsrBtSppMsgTransport(_queueId,msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppCancelAcceptAudioReqSend
 *
 *  DESCRIPTION
 *      Request to not accept incoming SCO/eSCO connection.
 *
 *  PARAMETERS
 *      queueId:               queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *      theServerChannel:      local server channel number
 * 
 *----------------------------------------------------------------------------*/
#define CsrBtSppCancelAcceptAudioReqSend(_queueId, _theServerChannel) { \
        CsrBtSppCancelAcceptAudioReq *msg = (CsrBtSppCancelAcceptAudioReq *)CsrPmemAlloc(sizeof(CsrBtSppCancelAcceptAudioReq)); \
        msg->type = CSR_BT_SPP_CANCEL_ACCEPT_AUDIO_REQ;                 \
        msg->serverChannel = _theServerChannel;                         \
        CsrBtSppMsgTransport(_queueId,msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppAudioDisconnectReqSend
 *
 *  DESCRIPTION
 *      Request to disconnect the SCO/eSCO connection. SPP sends CSR_BT_SPP_AUDIO_DISCONNECT_CFM to the application.
 *
 *  PARAMETERS
 *      queueId:               queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *      theServerChannel:      local server channel number
 *      scoHandle:             SCO handle
 *----------------------------------------------------------------------------*/
#define CsrBtSppAudioDisconnectReqSend(_queueId, _theServerChannel, _scoHandle) { \
        CsrBtSppAudioDisconnectReq *msg = (CsrBtSppAudioDisconnectReq *)CsrPmemAlloc(sizeof(CsrBtSppAudioDisconnectReq)); \
        msg->type = CSR_BT_SPP_AUDIO_DISCONNECT_REQ;                    \
        msg->serverChannel = _theServerChannel;                         \
        msg->scoHandle = _scoHandle;                                    \
        CsrBtSppMsgTransport(_queueId,msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppExtendedConnectReqSend
 *
 *  DESCRIPTION
 *        CSR_BT_SPP_EXTENDED_CONNECT_REQ will initiate a connection towards a device specified
 *        by the Bluetooth device address. The SPP will send an CSR_BT_SPP_CONNECT_IND back
 *        to the initiator with the result of the connection attempt.
 *
 *  PARAMETERS
 *      queueId:             queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *      appHandle:           control application handle to return signal to
 *      deviceAddr:          address of device to connect to
 *      requestPortPar:      request CM to execute port negotiation
 *      portPar:             parameters to send to peer entity, NULL if parameters
 *                           are not valid and port negotiation is not executed
 *                           NOTE: the lib function will copy the portPar so the
 *                           app layer must free the parameter itself (if applicable)
 *      role:                is the role CSR_BT_DTE or CSR_BT_DCE
 *      profileUuid:         The local profile Uuid (16 bit)
 *      secLevel:            Level of outgoing security to be applied
 *                           The application must specify one of the following values:
 *                            For Authentication and Encryption,
 *                           1. SECL4_OUT_LEVEL_0 : No Security (Not Recommended)
 *                           2. SECL4_OUT_LEVEL_1 : Low Security
 *                           3. SECL4_OUT_LEVEL_2 : Medium Security
 *                           4. SECL4_OUT_LEVEL_3 : High Security
 *
 *                            For Authorisation, the below value can be OR'ed additionally:
 *                           1. SECL_OUT_AUTHORISATION
 *                           For more information refer to dm_prim.h.
 *----------------------------------------------------------------------------*/
#define CsrBtSppExtendedConnectReqSend(_queueId, _phandle, _deviceAddr, _requestPortPar, _portPar, _role, _profileUuid, _secLevel) { \
        CsrBtSppExtendedConnectReq *msg = (CsrBtSppExtendedConnectReq *)CsrPmemAlloc(sizeof(CsrBtSppExtendedConnectReq)); \
        msg->type = CSR_BT_SPP_EXTENDED_CONNECT_REQ;                    \
        msg->phandle = _phandle;                                        \
        msg->deviceAddr = _deviceAddr;                                  \
        msg->role = _role;                                              \
        msg->requestPortPar = _requestPortPar;                          \
        if (_portPar != NULL)                                           \
        {                                                               \
            msg->validPortPar = TRUE;                                   \
            msg->portPar = *((RFC_PORTNEG_VALUES_T *) _portPar);        \
        }                                                               \
        else                                                            \
        {                                                               \
            CsrBtPortParDefault(&(msg->portPar) );                      \
            msg->validPortPar = FALSE;                                  \
        }                                                               \
        msg->profileUuid = _profileUuid;                                \
        msg->secLevel = _secLevel;                                      \
        CsrBtSppMsgTransport(_queueId,msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppExtendedUuidConnectReqSend
 *
 *  DESCRIPTION
 *        CSR_BT_SPP_EXTENDED_UUID_CONNECT_REQ will initiate a connection towards a device specified
 *        by the Bluetooth device address. The SPP will send an CSR_BT_SPP_CONNECT_IND back
 *        to the initiator with the result of the connection attempt.
 *
 *  PARAMETERS
 *      queueId:            queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *      appHandle:          application handle to return signal to
 *      deviceAddr:         address of device to connect to
 *      requestPortPar:     request CM to execute port negotiation
 *      portPar:            parameters to send to peer entity, NULL if parameters
 *                          are not valid and port negotiation is not executed
 *                          NOTE: the lib function will copy the portPar so the
 *                          app layer must free the parameter itself (if applicable)
 *      role:               is the role CSR_BT_DTE or CSR_BT_DCE
 *      profileUuid:        The local profile Uuid (128 bit)
 *      secLevel:           Level of outgoing security to be applied
 *                          The application must specify one of the following values:
 *                           For Authentication and Encryption,
 *                           1. SECL4_OUT_LEVEL_0 : No Security (Not Recommended)
 *                           2. SECL4_OUT_LEVEL_1 : Low Security
 *                           3. SECL4_OUT_LEVEL_2 : Medium Security
 *                           4. SECL4_OUT_LEVEL_3 : High Security
 *
 *                           For Authorisation, the below value can be OR'ed additionally:
 *                          1. SECL_OUT_AUTHORISATION
 *                          For more information refer to dm_prim.h.
 *----------------------------------------------------------------------------*/
#define CsrBtSppExtendedUuidConnectReqSend(_queueId, _phandle, _deviceAddr, _requestPortPar, _portPar, _role, _profileUuid, _secLevel) { \
        CsrBtSppExtendedUuidConnectReq *msg = (CsrBtSppExtendedUuidConnectReq *)CsrPmemAlloc(sizeof(CsrBtSppExtendedUuidConnectReq)); \
        msg->type = CSR_BT_SPP_EXTENDED_UUID_CONNECT_REQ;               \
        msg->phandle = _phandle;                                        \
        msg->deviceAddr = _deviceAddr;                                  \
        msg->role = _role;                                              \
        msg->requestPortPar = _requestPortPar;                          \
        if (_portPar != NULL)                                           \
        {                                                               \
            msg->validPortPar = TRUE;                                   \
            msg->portPar = *((RFC_PORTNEG_VALUES_T *) _portPar);        \
        }                                                               \
        else                                                            \
        {                                                               \
            CsrBtPortParDefault(&(msg->portPar) );                      \
            msg->validPortPar = FALSE;                                  \
        }                                                               \
        CsrMemCpy(msg->profileUuid, _profileUuid, sizeof(CsrBtUuid128)); \
        msg->secLevel = _secLevel;                                      \
        CsrBtSppMsgTransport(_queueId,msg);}
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */

#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppActivateReqSendExt
 *
 *  DESCRIPTION
 *      This signal is used to activate a service with given service record and 
 *      security level and make it accessible from a remote device. The application 
 *      receives CSR_BT_SPP_ACTIVATE_CFM and it remains connectable until remote device 
 *      has connected or timer expires (on page scan timeout).
 *      Note: The CSR_BT_SPP_ACTIVATE_REQ signal may fail in which case the 
 *      application does not receive a CSR_BT_SPP_ACTIVATE_CFM signal. 
 *      However, a CSR_BT_SPP_CONNECT_IND signal is sent to notify that 
 *      the request has failed.
 *
 *  PARAMETERS
 *      queueId:              queue-id of this spp instance, see csr_bt_tasks.h for possible queue-ids
 *      appHandle:            control application handle
 *      timeout:              holds the pageScanTimeOut time, in number of seconds, 
 *                            where the service should be available for remote connections.
 *                            A timeout value of 0 (zero) or the define CSR_BT_INFINITE_TIME 
 *                            in the file csr_bt_profiles.h indicates an infinity timeout
 *                            time. When a connection is made, the service available stops
 *      serviceRecord:        Contains the alternative service record
 *      serviceRecordSize:    Length of the alternative service record
 *      serverChannelIndex:   Index to location of server channel in service record
 *      secLevel:             Level of incoming security to be applied
 *                            The application must specify one of the following values:
 *                             For Authentication and Encryption,
 *                            1. SECL4_IN_LEVEL_0 : No Security (Not Recommended)
 *                            2. SECL4_IN_LEVEL_1 : Low Security
 *                            3. SECL4_IN_LEVEL_2 : Medium Security
 *                            4. SECL4_IN_LEVEL_3 : High Security
 *
 *                             For Authorisation, the below value can be OR'ed additionally:
 *                            1. SECL_IN_AUTHORISATION
 *                            For more information refer to dm_prim.h.
 *      classOfDevice:        The Class Of Device of the profile
 *----------------------------------------------------------------------------*/
#define CsrBtSppActivateReqSendExt(_queueId, _thePhandle, _theTimeout, _serviceRecord, _serviceRecordSize, _serverChannelIndex, _secLevel, _classOfDevice) { \
        CsrBtSppExtendedActivateReq *msg = (CsrBtSppExtendedActivateReq *)CsrPmemAlloc(sizeof(CsrBtSppExtendedActivateReq)); \
        msg->type = CSR_BT_SPP_EXTENDED_ACTIVATE_REQ;                   \
        msg->phandle = _thePhandle;                                     \
        msg->timeout = _theTimeout;                                     \
        msg->serviceRecordSize = _serviceRecordSize;                    \
        msg->serviceRecord = _serviceRecord;                            \
        msg->serverChannelIndex = _serverChannelIndex;                  \
        msg->secLevel = _secLevel;                                      \
        msg->classOfDevice = _classOfDevice;                            \
        CsrBtSppMsgTransport(_queueId,msg);}
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtSppFreeUpstreamMessageContents
 *
 *  DESCRIPTION
 *      Application should use this function to deallocate payload allocated in the Synergy BT SPP message. 
 *      This function should be called by the application after the upstream message has been processed.
 *
 *    PARAMETERS
 *      eventClass :  Must be CSR_BT_SPP_PRIM,
 *      msg:          The message received from Synergy BT SPP
 *----------------------------------------------------------------------------*/
void CsrBtSppFreeUpstreamMessageContents(CsrUint16 eventClass, void * message);


#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_SPP_LIB_H__ */
