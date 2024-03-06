#ifndef CSR_BT_AV_LIB_H__
#define CSR_BT_AV_LIB_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_bt_profiles.h"
#include "csr_bt_av_prim.h"
#include "csr_bt_tasks.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Common put_message function to reduce code size */
void CsrBtAvMsgTransport(void* msg__);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvActivateReqSend
 *
 *  DESCRIPTION
 *      This signal is used to activate a service and make it accessible from a
 *      remote device. AV sends CSR_BT_AV_ACTIVATE_CFM message back to the
 *      application with the activation result.
 *
 *  PARAMETERS
 *      phandle:            application handle
 *      localRole:          local role of the device to activate. Synergy uses this
 *                          value just for the role specific SDP record registration.
 *                          Note that, "localRole" is not associated with AV connection.
 *----------------------------------------------------------------------------*/
#define CsrBtAvActivateReqSend(_phandle, _localRole) {                  \
        CsrBtAvActivateReq *msg__ = (CsrBtAvActivateReq *) CsrPmemAlloc(sizeof(CsrBtAvActivateReq)); \
        msg__->type = CSR_BT_AV_ACTIVATE_REQ;                           \
        msg__->phandle = _phandle;                                      \
        msg__->localRole = _localRole;                                  \
        CsrBtAvMsgTransport(msg__); }

#ifdef INSTALL_AV_DEACTIVATE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDeactivateReqSend
 *
 *  DESCRIPTION
 *      This signal is used to deactivate a service and make it inaccessible from
 *      other devices. AV sends CSR_BT_AV_DEACTIVATE_CFM message back to the
 *      application with the deactivation result.
 *
 *  PARAMETERS
 *      localRole:          local role of the device to deactivate
 *----------------------------------------------------------------------------*/
#define CsrBtAvDeactivateReqSend(_localRole) {                          \
        CsrBtAvDeactivateReq *msg__ = (CsrBtAvDeactivateReq *) CsrPmemAlloc(sizeof(CsrBtAvDeactivateReq)); \
        msg__->type = CSR_BT_AV_DEACTIVATE_REQ;                         \
        msg__->localRole = _localRole;                                  \
        CsrBtAvMsgTransport(msg__); }
#endif /* INSTALL_AV_DEACTIVATE */

#ifdef INSTALL_AV_STREAM_DATA_APP_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvRegisterStreamHandleReqSend
 *
 *  DESCRIPTION
 *      Register the application to handle AV stream data. The registered
 *      application would receive AV stream data messages. AV sends
 *      CSR_BT_AV_REGISTER_STREAM_HANDLE_CFM message back to the
 *      application with the result.
 *
 *  PARAMETERS
 *      streamHandle:     stream (data) application handle
 *----------------------------------------------------------------------------*/
#define CsrBtAvRegisterStreamHandleReqSend(_streamHandle) {              \
        CsrBtAvRegisterStreamHandleReq *msg__ = (CsrBtAvRegisterStreamHandleReq *) CsrPmemAlloc(sizeof(CsrBtAvRegisterStreamHandleReq)); \
        msg__->type = CSR_BT_AV_REGISTER_STREAM_HANDLE_REQ;             \
        msg__->streamHandle = _streamHandle;                            \
        CsrBtAvMsgTransport(msg__); }
#endif /* INSTALL_AV_STREAM_DATA_APP_SUPPORT */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvConnectReqSend
 *
 *  DESCRIPTION
 *      Request to initiate A2DP SLC connection towards a device specified
 *      by the device address. AV will send CSR_BT_AV_CONNECT_CFM message back
 *      to the application with the connection attempt result.
 *
 *  PARAMETERS
 *      phandle:            application handle
 *      deviceAddr:         bluetooth address of remote device to connect
 *      remoteRole:         AV role of the remote device to connect
 *----------------------------------------------------------------------------*/
#define CsrBtAvConnectReqSend(_phandle, _deviceAddr, _remoteRole) {     \
        CsrBtAvConnectReq *msg__ = (CsrBtAvConnectReq *) CsrPmemAlloc(sizeof(CsrBtAvConnectReq)); \
        msg__->type = CSR_BT_AV_CONNECT_REQ;                            \
        msg__->phandle = _phandle;                                      \
        msg__->deviceAddr = _deviceAddr;                                \
        msg__->remoteRole = _remoteRole;                                \
        switch(_remoteRole)                                                      \
        {                                                               \
            case CSR_BT_AV_AUDIO_SOURCE:                                \
                msg__->localRole = CSR_BT_AV_AUDIO_SINK;                \
                break;                                                  \
            case CSR_BT_AV_AUDIO_SINK:                                  \
                msg__->localRole = CSR_BT_AV_AUDIO_SOURCE;              \
                break;                                                  \
            case CSR_BT_AV_VIDEO_SOURCE:                                \
                msg__->localRole = CSR_BT_AV_VIDEO_SINK;                \
                break;                                                  \
            case CSR_BT_AV_VIDEO_SINK:                                  \
                msg__->localRole = CSR_BT_AV_VIDEO_SOURCE;              \
                break;                                                  \
        }                                                               \
        CsrBtAvMsgTransport(msg__); }

#ifdef INSTALL_AV_CANCEL_CONNECT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCancelConnectReqSend
 *
 *  DESCRIPTION
 *      Request to cancel a prior request for outgoing connection establishment.
 *      If the cancel request is successful, a CSR_BT_AV_CONNECT_CFM is returned
 *      with the result set to CSR_BT_AV_CANCEL_CONNECT_ATTEMPT. If the connection
 *      setup is completed before the AV receives the CSR_BT_AV_CANCEL_CONNECT_REQ,
 *      the connection is disconnected and a CSR_BT_AV_DISCONNECT_IND returned
 *      to the application.
 *
 *  PARAMETERS
 *      deviceAddr:         bluetooth address of remote device for which connect
 *                          request was initiated by application
 *----------------------------------------------------------------------------*/
#define CsrBtAvCancelConnectReqSend(_deviceAddr) {                      \
        CsrBtAvCancelConnectReq *msg__ = (CsrBtAvCancelConnectReq *) CsrPmemAlloc(sizeof(CsrBtAvCancelConnectReq)); \
        msg__->type = CSR_BT_AV_CANCEL_CONNECT_REQ;                     \
        msg__->deviceAddr = _deviceAddr;                                \
        CsrBtAvMsgTransport(msg__); }
#endif /* INSTALL_AV_CANCEL_CONNECT */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDisconnectReqSend
 *
 *  DESCRIPTION
 *      Request to initiate a release of a previously established connection.
 *      When the connection is released, or if the remote device releases the
 *      connection, AV indicates application through CSR_BT_AV_DISCONNECT_IND.
 *
 *  PARAMETERS
 *      connectionId:       connection identifier
 *----------------------------------------------------------------------------*/
#define CsrBtAvDisconnectReqSend(_connectionId) {                       \
        CsrBtAvDisconnectReq *msg__ = (CsrBtAvDisconnectReq *) CsrPmemAlloc(sizeof(CsrBtAvDisconnectReq)); \
        msg__->type = CSR_BT_AV_DISCONNECT_REQ;                         \
        msg__->connectionId = _connectionId;                            \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDiscoverReqSend
 *
 *  DESCRIPTION
 *      Request to initiate the Stream end-points discovery procedure. On
 *      remote device response, AV responds the discovery results to application
 *      through CSR_BT_AV_DISCOVER_CFM message.
 *
 *  PARAMETERS
 *      connectionId:       connection identifier
 *      tLabel:             transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvDiscoverReqSend(_connectionId, _tLabel) {                \
        CsrBtAvDiscoverReq *msg__ = (CsrBtAvDiscoverReq *) CsrPmemAlloc(sizeof(CsrBtAvDiscoverReq)); \
        msg__->type = CSR_BT_AV_DISCOVER_REQ;                           \
        msg__->connectionId = _connectionId;                            \
        msg__->tLabel = _tLabel;                                        \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDiscoverResRejSend
 *
 *  DESCRIPTION
 *      When remote device initiates Stream end-points discovery procedure. AV indicates
 *      application through CSR_BT_AV_DISCOVER_IND. Application uses this API
 *      to send reject response to above indication.
 *
 *  PARAMETERS
 *      connectionId:       connection identifier
 *      tLabel:             transaction label
 *      avResponse:         cause of rejection
 *----------------------------------------------------------------------------*/
#define CsrBtAvDiscoverResRejSend(_connectionId, _tLabel, _avResponse) { \
        CsrBtAvDiscoverRes *msg__ = (CsrBtAvDiscoverRes *) CsrPmemAlloc(sizeof(CsrBtAvDiscoverRes)); \
        msg__->type = CSR_BT_AV_DISCOVER_RES;                           \
        msg__->connectionId = _connectionId;                            \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = _avResponse;                                \
        msg__->seidInfoCount = 0;                                       \
        msg__->seidInfo = NULL;                                         \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDiscoverResAcpSend
 *
 *  DESCRIPTION
 *      When remote device initiates Stream end-points discovery procedure. AV indicates
 *      application through CSR_BT_AV_DISCOVER_IND. Application uses this API
 *      to send accept response to above indication.
 *
 *  PARAMETERS
 *      connectionId:     connection identifier
 *      tLabel:           transaction label
 *      seidInfoCount:    number of stream end-points
 *      seidInfo:         pointer to stream end-point information structure(s) (CsrBtAvSeidInfo)
 *----------------------------------------------------------------------------*/
#define CsrBtAvDiscoverResAcpSend(_connectionId, _tLabel, _seidInfoCount, _seidInfo) { \
        CsrBtAvDiscoverRes *msg__ = (CsrBtAvDiscoverRes *) CsrPmemAlloc(sizeof(CsrBtAvDiscoverRes)); \
        msg__->type = CSR_BT_AV_DISCOVER_RES;                           \
        msg__->connectionId = _connectionId;                            \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        if((_seidInfoCount > 0) && (_seidInfo != NULL))                 \
        {                                                               \
            msg__->seidInfoCount = _seidInfoCount;                      \
            msg__->seidInfo = _seidInfo;                                \
        }                                                               \
        else                                                            \
        {                                                               \
            msg__->seidInfoCount = 0;                                   \
            msg__->seidInfo = NULL;                                     \
        }                                                               \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetCapabilitiesReqSend
 *
 *  DESCRIPTION
 *      Request to initiate Get Capabilities procedure of a stream end-point as
 *      specified in the "acpSeid". AV informs the result to application through
 *      CSR_BT_AV_GET_CAPABILITIES_CFM message containing the complete list of
 *      stream capabilities (both application and transport capabilities).
 *
 *  PARAMETERS
 *      connectionId:     connection identifier
 *      acpSeid:          acceptor stream end-point id
 *      tLabel:           transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetCapabilitiesReqSend(_connectionId, _acpSeid, _tLabel) { \
        CsrBtAvGetCapabilitiesReq *msg__ = (CsrBtAvGetCapabilitiesReq *) CsrPmemAlloc(sizeof(CsrBtAvGetCapabilitiesReq)); \
        msg__->type = CSR_BT_AV_GET_CAPABILITIES_REQ;                   \
        msg__->connectionId = _connectionId;                            \
        msg__->tLabel = _tLabel;                                        \
        msg__->acpSeid = _acpSeid;                                      \
        CsrBtAvMsgTransport(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetCapabilitiesResRejSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Get Capabilities procedure of a Strem end-point,
 *      AV indicates application through CSR_BT_AV_GET_CAPABILITIES_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      connectionId:     connection identifier
 *      tLabel:           transaction label
 *      avResponse:       cause for rejection
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetCapabilitiesResRejSend(_connectionId, _tLabel, _avResponse) { \
        CsrBtAvGetCapabilitiesRes *msg__ = (CsrBtAvGetCapabilitiesRes *) CsrPmemAlloc(sizeof(CsrBtAvGetCapabilitiesRes)); \
        msg__->type = CSR_BT_AV_GET_CAPABILITIES_RES;                   \
        msg__->connectionId = _connectionId;                            \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = _avResponse;                                \
        msg__->servCapLen = 0;                                          \
        msg__->servCapData = NULL;                                      \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetCapabilitiesResAcpSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Get Capabilities procedure of a Strem end-point,
 *      AV indicates the remote request to application through CSR_BT_AV_GET_CAPABILITIES_IND.
 *      Application uses this API to send accept response to above indication.
 *
 *  PARAMETERS
 *      connectionId:     connection identifier
 *      tLabel:           transaction label
 *      servCapLen:       length of service capabilities data
 *      servCapData:      pointer to service capabilities data
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetCapabilitiesResAcpSend(_connectionId, _tLabel, _servCapLen, _servCapData) { \
        CsrBtAvGetCapabilitiesRes *msg__ = (CsrBtAvGetCapabilitiesRes *) CsrPmemAlloc(sizeof(CsrBtAvGetCapabilitiesRes)); \
        msg__->type = CSR_BT_AV_GET_CAPABILITIES_RES;                   \
        msg__->connectionId = _connectionId;                            \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        msg__->servCapLen = _servCapLen;                                \
        msg__->servCapData = _servCapData;                              \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSetConfigReqSend
 *
 *  DESCRIPTION
 *      Request to initiate the Set Configuration procedure of a Stream identified
 *      by the pair of INT and ACP Stream end-points identifiers. The configuration
 *      pointed to by appServCapData contains the application's service categories.
 *      AV responds the result to application through CSR_BT_AV_SET_CONFIGURATION_CFM.
 *
 *  PARAMETERS
 *      connectionId:     connection identifier
 *      tLabel:           transaction label
 *      acpSeid:          acceptor stream end-point id
 *      intSeid:          initiator stream end-point id
 *      appServCapLen:    length of application service capabilities
 *      appServCapData:   pointer to app. service capabilities
 *----------------------------------------------------------------------------*/
#define CsrBtAvSetConfigReqSend(_connectionId, _tLabel, _acpSeid, _intSeid, _appServCapLen, _appServCapData) { \
        CsrBtAvSetConfigurationReq *msg__ = (CsrBtAvSetConfigurationReq *) CsrPmemAlloc(sizeof(CsrBtAvSetConfigurationReq)); \
        msg__->type = CSR_BT_AV_SET_CONFIGURATION_REQ;                  \
        msg__->connectionId = _connectionId;                            \
        msg__->tLabel = _tLabel;                                        \
        msg__->acpSeid = _acpSeid;                                      \
        msg__->intSeid = _intSeid;                                      \
        msg__->appServCapLen = _appServCapLen;                          \
        msg__->appServCapData = _appServCapData;                        \
        CsrBtAvMsgTransport(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSetConfigResAcpSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Set Configuration procedure of a Strem end-point,
 *      AV indicates the remote request to application through CSR_BT_AV_SET_CONFIGURATION_IND.
 *      Application uses this API to send accept response to above indication.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvSetConfigResAcpSend(_shandle, _tLabel) {                 \
        CsrBtAvSetConfigurationRes *msg__ = (CsrBtAvSetConfigurationRes *) CsrPmemAlloc(sizeof(CsrBtAvSetConfigurationRes)); \
        msg__->type = CSR_BT_AV_SET_CONFIGURATION_RES;                  \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->servCategory = 0;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSetConfigResRejSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Set Configuration procedure of a Strem end-point,
 *      AV indicates the remote request to application through CSR_BT_AV_SET_CONFIGURATION_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *      avResponse:       cause of rejection
 *      servCategory:     failing service category
 *----------------------------------------------------------------------------*/
#define CsrBtAvSetConfigResRejSend(_shandle, _tLabel, _avResponse, _servCategory) { \
        CsrBtAvSetConfigurationRes *msg__ = (CsrBtAvSetConfigurationRes *) CsrPmemAlloc(sizeof(CsrBtAvSetConfigurationRes)); \
        msg__->type = CSR_BT_AV_SET_CONFIGURATION_RES;                  \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->servCategory = _servCategory;                            \
        msg__->avResponse = _avResponse;                                \
        CsrBtAvMsgTransport(msg__); }

#ifdef INSTALL_AV_GET_CONFIGURATION
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetConfigReqSend
 *
 *  DESCRIPTION
 *      Request to initiate Get Configuration procedure of a Stream end-point.
 *      AV responds the result to INT application through
 *      CSR_BT_AV_GET_CONFIGURATION_CFM message.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetConfigReqSend(_shandle, _tLabel) {                    \
        CsrBtAvGetConfigurationReq *msg__ = (CsrBtAvGetConfigurationReq *) CsrPmemAlloc(sizeof(CsrBtAvGetConfigurationReq)); \
        msg__->type = CSR_BT_AV_GET_CONFIGURATION_REQ;                  \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        CsrBtAvMsgTransport(msg__); }
#endif /* INSTALL_AV_GET_CONFIGURATION */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetConfigResRejSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Get Configuration procedure of a Strem end-point,
 *      AV indicates the remote request to application through CSR_BT_AV_GET_CONFIGURATION_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      shandle           stream handle
 *      tLabel:           transaction label
 *      avResponse:       cause of rejection
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetConfigResRejSend(_shandle, _tLabel, _avResponse) {    \
        CsrBtAvGetConfigurationRes *msg__ = (CsrBtAvGetConfigurationRes *) CsrPmemAlloc(sizeof(CsrBtAvGetConfigurationRes)); \
        msg__->type = CSR_BT_AV_GET_CONFIGURATION_RES;                  \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = _avResponse;                                \
        msg__->servCapLen = 0;                                          \
        msg__->servCapData = NULL;                                      \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetConfigResAcpSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Get Configuration procedure of a Strem end-point,
 *      AV indicates the remote request to application through CSR_BT_AV_GET_CONFIGURATION_IND.
 *      Application uses this API to send accept response to above indication.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *      servCapLen:       length of application service capabilities data
 *      servCapData:      pointer to application service capabilities data
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetConfigResAcpSend(_shandle, _tLabel, _servCapLen, _servCapData) { \
        CsrBtAvGetConfigurationRes *msg__ = (CsrBtAvGetConfigurationRes *) CsrPmemAlloc(sizeof(CsrBtAvGetConfigurationRes)); \
        msg__->type = CSR_BT_AV_GET_CONFIGURATION_RES;                  \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        msg__->servCapLen = _servCapLen;                                \
        msg__->servCapData = _servCapData;                              \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvReconfigReqSend
 *
 *  DESCRIPTION
 *      Request to initiate Stream reconfiguration procedure of a Stream end-point.
 *      Note that reconfiguration procedure may only be initiated when the stream
 *      has previously been suspended. AV responds the result to INT application
 *      through CSR_BT_AV_RECONFIGURE_CFM message.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *      servCapLen:       length of application service capabilities data
 *      servCapData:      pointer to application service capabilities data
 *----------------------------------------------------------------------------*/
#define CsrBtAvReconfigReqSend(_shandle, _tLabel, _servCapLen, _servCapData) { \
        CsrBtAvReconfigureReq *msg__ = (CsrBtAvReconfigureReq *) CsrPmemAlloc(sizeof(CsrBtAvReconfigureReq)); \
        msg__->type = CSR_BT_AV_RECONFIGURE_REQ;                        \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->servCapLen = _servCapLen;                                \
        msg__->servCapData = _servCapData;                              \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvReconfigResAcpSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Stream reconfiguration procedure of a Strem end-point,
 *      AV indicates the remote request to application through CSR_BT_AV_RECONFIGURE_IND.
 *      Application uses this API to send accept response to above indication.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvReconfigResAcpSend(_shandle, _tLabel) {                  \
        CsrBtAvReconfigureRes *msg__ = (CsrBtAvReconfigureRes *) CsrPmemAlloc(sizeof(CsrBtAvReconfigureRes)); \
        msg__->type = CSR_BT_AV_RECONFIGURE_RES;                        \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->servCategory = 0;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvReconfigResRejSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Stream reconfiguration procedure of a Strem end-point,
 *      AV indicates the remote request to application through CSR_BT_AV_RECONFIGURE_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *      avResponse:       cause of rejection
 *      servCategory:     failing service category
 *----------------------------------------------------------------------------*/
#define CsrBtAvReconfigResRejSend(_shandle, _tLabel, _avResponse, _servCategory) { \
        CsrBtAvReconfigureRes *msg__ = (CsrBtAvReconfigureRes *) CsrPmemAlloc(sizeof(CsrBtAvReconfigureRes)); \
        msg__->type = CSR_BT_AV_RECONFIGURE_RES;                        \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->servCategory = _servCategory;                            \
        msg__->avResponse = _avResponse;                                \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvOpenReqSend
 *
 *  DESCRIPTION
 *      Request to initiate the media channel opening procedure of a stream.
 *      Note that the stream should previously have been configured before
 *      this procedure is initiated. AV responds the result to INT application
 *      through CSR_BT_AV_OPEN_CFM.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvOpenReqSend(_shandle, _tLabel) {                         \
        CsrBtAvOpenReq *msg__ = (CsrBtAvOpenReq *) CsrPmemAlloc(sizeof(CsrBtAvOpenReq)); \
        msg__->type = CSR_BT_AV_OPEN_REQ;                               \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        CsrBtAvMsgTransport(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvOpenResRejSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Open procedure of a Strem end-point,
 *      AV indicates the remote request to application through CSR_BT_AV_OPEN_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *      avResponse:       cause of rejection
 *----------------------------------------------------------------------------*/
#define CsrBtAvOpenResRejSend(_shandle, _tLabel, _avResponse) {         \
        CsrBtAvOpenRes *msg__ = (CsrBtAvOpenRes *) CsrPmemAlloc(sizeof(CsrBtAvOpenRes)); \
        msg__->type = CSR_BT_AV_OPEN_RES;                               \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = _avResponse;                                \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvOpenResAcpSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Open procedure of a Strem end-point,
 *      AV indicates the remote request to application through CSR_BT_AV_OPEN_IND.
 *      Application uses this API to send accept response to above indication.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvOpenResAcpSend(_shandle, _tLabel) {                      \
        CsrBtAvOpenRes *msg__ = (CsrBtAvOpenRes *) CsrPmemAlloc(sizeof(CsrBtAvOpenRes)); \
        msg__->type = CSR_BT_AV_OPEN_RES;                               \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStartReqSend
 *
 *  DESCRIPTION
 *       Request to initiate the starting of one or more streams. AV responds
 *       the result of Start request through CSR_BT_AV_START_CFM message.
 *
 *  PARAMETERS
 *       listLength:      number of list entries
 *       tLabel:          transaction label
 *       list:            pointer to list of stream handles
 *----------------------------------------------------------------------------*/
#define CsrBtAvStartReqSend(_listLength, _tLabel, _list) {              \
        CsrBtAvStartReq *msg__ = (CsrBtAvStartReq *) CsrPmemAlloc(sizeof(CsrBtAvStartReq)); \
        msg__->type = CSR_BT_AV_START_REQ;                              \
        msg__->tLabel = _tLabel;                                        \
        msg__->listLength = _listLength;                                      \
        msg__->list = _list;                                            \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStartResRejSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Start Streaming procedure,
 *      AV indicates the remote request to application through CSR_BT_AV_START_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      reject_shandle:   stream handle for the first stream that fails
 *      tLabel:           transaction label
 *      avResponse:       cause of rejection
 *      listLength:       number of list entries
 *      list:             pointer to list of stream handles
 *----------------------------------------------------------------------------*/
#define CsrBtAvStartResRejSend(_reject_shandle, _tLabel, _avResponse, _listLength, _list) { \
        CsrBtAvStartRes *msg__ = (CsrBtAvStartRes *) CsrPmemAlloc(sizeof(CsrBtAvStartRes)); \
        msg__->type = CSR_BT_AV_START_RES;                              \
        msg__->reject_shandle = _reject_shandle;                        \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = _avResponse;                                \
        msg__->listLength = _listLength;                                \
        msg__->list = _list;                                            \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStartResAcpSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Start Streaming procedure,
 *      AV indicates the remote request to application through CSR_BT_AV_START_IND.
 *      Application uses this API to send accept response to above indication.
 *
 *  PARAMETERS
 *      tLabel:           transaction label
 *      listLength:       number of list entries
 *      list:             pointer to list of stream handles
 *----------------------------------------------------------------------------*/
#define CsrBtAvStartResAcpSend(_tLabel, _listLength, _list) {           \
        CsrBtAvStartRes *msg__ = (CsrBtAvStartRes *) CsrPmemAlloc(sizeof(CsrBtAvStartRes)); \
        msg__->type = CSR_BT_AV_START_RES;                              \
        msg__->reject_shandle = 0;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        msg__->listLength = _listLength;                                \
        msg__->list = _list;                                            \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCloseReqSend
 *
 *  DESCRIPTION
 *      Request to initiate the closing of a stream. AV responds
 *      the result of Close request through CSR_BT_AV_CLOSE_CFM message.
 *
 *  PARAMETERS
 *      shandle:          stream handle
 *      tLabel:           transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvCloseReqSend(_shandle, _tLabel) {                        \
        CsrBtAvCloseReq *msg__ = (CsrBtAvCloseReq *) CsrPmemAlloc(sizeof(CsrBtAvCloseReq)); \
        msg__->type = CSR_BT_AV_CLOSE_REQ;                              \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        CsrBtAvMsgTransport(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCloseResRejSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Closing of a stream,
 *      AV indicates the remote request to application through CSR_BT_AV_CLOSE_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      shandle:           stream handle
 *      tLabel:            transaction label
 *      avResponse:        cause of rejection
 *----------------------------------------------------------------------------*/
#define CsrBtAvCloseResRejSend(_shandle, _tLabel, _avResponse) {        \
        CsrBtAvCloseRes *msg__ = (CsrBtAvCloseRes *) CsrPmemAlloc(sizeof(CsrBtAvCloseRes)); \
        msg__->type = CSR_BT_AV_CLOSE_RES;                              \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = _avResponse;                                \
        CsrBtAvMsgTransport(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCloseResAcpSend
 *
 *  DESCRIPTION
 *      When remote device initiates the Closing of a stream,
 *      AV indicates the remote request to application through CSR_BT_AV_CLOSE_IND.
 *      Application uses this API to send accept response to above indication.
 *
 *  PARAMETERS
 *      shandle:           stream handle
 *      tLabel:            transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvCloseResAcpSend(_shandle, _tLabel) {                     \
        CsrBtAvCloseRes *msg__ = (CsrBtAvCloseRes *) CsrPmemAlloc(sizeof(CsrBtAvCloseRes)); \
        msg__->type = CSR_BT_AV_CLOSE_RES;                              \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSuspendReqSend
 *
 *  DESCRIPTION
 *      Request to initiate the suspend procedure for one or more streams. AV responds
 *      the result of Suspend request through CSR_BT_AV_SUSPEND_CFM message.
 *
 *  PARAMETERS
 *      listLength:        number of list entries
 *      tLabel:            transaction label
 *      list:              pointer to list of stream handles
 *----------------------------------------------------------------------------*/
#define CsrBtAvSuspendReqSend(_listLength, _tLabel, _list) {            \
        CsrBtAvSuspendReq *msg__ = (CsrBtAvSuspendReq *) CsrPmemAlloc(sizeof(CsrBtAvSuspendReq)); \
        msg__->type = CSR_BT_AV_SUSPEND_REQ;                            \
        msg__->tLabel = _tLabel;                                        \
        msg__->listLength = _listLength;                                \
        msg__->list = _list;                                            \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSuspendResRejSend
 *
 *  DESCRIPTION
 *      When remote device initiates the suspend of one or more streams,
 *      AV indicates the remote request to application through CSR_BT_AV_SUSPEND_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      reject_shandle:    stream handle
 *      tLabel:            transaction label
 *      avResponse:        cause of rejection
 *      listLength:        number of list entries
 *      list:              pointer to list of stream handles
 *----------------------------------------------------------------------------*/
#define CsrBtAvSuspendResRejSend(_reject_shandle, _tLabel, _avResponse, _listLength, _list) { \
        CsrBtAvSuspendRes *msg__ = (CsrBtAvSuspendRes *) CsrPmemAlloc(sizeof(CsrBtAvSuspendRes)); \
        msg__->type = CSR_BT_AV_SUSPEND_RES;                            \
        msg__->reject_shandle = _reject_shandle;                        \
        msg__->tLabel = _tLabel;                                        \
        msg__->listLength = _listLength;                                \
        msg__->list = _list;                                            \
        msg__->avResponse= _avResponse;                                 \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSuspendResAcpSend
 *
 *  DESCRIPTION
 *      When remote device initiates the suspend of one or more streams,
 *      AV indicates the remote request to application through CSR_BT_AV_SUSPEND_IND.
 *      Application uses this API to send accept response to above indication.
 *
 *  PARAMETERS
 *      tLabel:            transaction label
 *      listLength:        number of list entries
 *      list:              pointer to list of stream handles
 *----------------------------------------------------------------------------*/
#define CsrBtAvSuspendResAcpSend(_tLabel, _listLength, _list) {         \
        CsrBtAvSuspendRes *msg__ = (CsrBtAvSuspendRes *) CsrPmemAlloc(sizeof(CsrBtAvSuspendRes)); \
        msg__->type = CSR_BT_AV_SUSPEND_RES;                            \
        msg__->reject_shandle = 0;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->listLength = _listLength;                                \
        msg__->list = _list;                                            \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvAbortReqSend
 *
 *  DESCRIPTION
 *      Request to initiate the procedure to abort a stream. AV responds
 *      the result of Abort request through CSR_BT_AV_ABORT_CFM message.
 *
 *  PARAMETERS
 *      shandle            stream handle
 *      tLabel:            transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvAbortReqSend(_shandle, _tLabel) {                        \
        CsrBtAvAbortReq *msg__ = (CsrBtAvAbortReq *) CsrPmemAlloc(sizeof(CsrBtAvAbortReq)); \
        msg__->type = CSR_BT_AV_ABORT_REQ;                              \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvAbortResSend
 *
 *  DESCRIPTION
 *      When remote device initiates the abort procedure for a stream,
 *      AV indicates the remote request to application through CSR_BT_AV_ABORT_IND.
 *      Application uses this API to send the response to above indication.
 *
 *  PARAMETERS
 *      shandle:           stream handle
 *      tLabel:            transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvAbortResSend(_shandle, _tLabel) {                        \
        CsrBtAvAbortRes *msg__ = (CsrBtAvAbortRes *) CsrPmemAlloc(sizeof(CsrBtAvAbortRes)); \
        msg__->type = CSR_BT_AV_ABORT_RES;                              \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        CsrBtAvMsgTransport(msg__); }

#ifdef INSTALL_AV_SECURITY_CONTROL
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityControlReqSend
 *
 *  DESCRIPTION
 *      Security control request for a stream. Request to initiate the procedure of
 *      exchanging content protection control data. AV responds
 *      the result of Security Control request through CSR_BT_AV_SECURITY_CONTROL_CFM message.
 *
 *  PARAMETERS
 *      shandle:                     stream handle
 *      tLabel:                      transaction label
 *      contProtectMethodLen:        length of content protection method data
 *      contProtectMethodData:       pointer to content protection method data
 *----------------------------------------------------------------------------*/
#define CsrBtAvSecurityControlReqSend(_shandle, _tLabel, _contProtMethodLen, _contProtMethodData) {\
        CsrBtAvSecurityControlReq *msg__ = (CsrBtAvSecurityControlReq *) CsrPmemAlloc(sizeof(CsrBtAvSecurityControlReq)); \
        msg__->type = CSR_BT_AV_SECURITY_CONTROL_REQ;                   \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->contProtMethodLen = _contProtMethodLen;                  \
        msg__->contProtMethodData = _contProtMethodData;                \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityControlResAcpSend
 *
 *  DESCRIPTION
 *      When remote device initiates the security control request for a stream,
 *      AV indicates the remote request to application through CSR_BT_AV_SECURITY_CONTROL_IND.
 *      Application uses this API to send accept response to above indication.
 *
 *  PARAMETERS
 *      shandle:                     stream handle
 *      tLabel:                      transaction label
 *      contProtectMethodLen:        length of content protection method data
 *      contProtectMethodData:       pointer to content protection method data
 *----------------------------------------------------------------------------*/
#define CsrBtAvSecurityControlResAcpSend(_shandle, _tLabel, _contProtMethodLen, _contProtMethodData) { \
        CsrBtAvSecurityControlRes *msg__ = (CsrBtAvSecurityControlRes *) CsrPmemAlloc(sizeof(CsrBtAvSecurityControlRes)); \
        msg__->type = CSR_BT_AV_SECURITY_CONTROL_RES;                   \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        msg__->contProtMethodLen = _contProtMethodLen;                  \
        msg__->contProtMethodData = _contProtMethodData;                \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityControlResRejSend
 *
 *  DESCRIPTION
 *      When remote device initiates the security control request for a stream,
 *      AV indicates the remote request to application through CSR_BT_AV_SECURITY_CONTROL_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      shandld            stream handle
 *      tLabel:            transaction label
 *      avResponse:        cause of rejection
 *----------------------------------------------------------------------------*/
#define CsrBtAvSecurityControlResRejSend(_shandle, _tLabel, _avResponse) { \
        CsrBtAvSecurityControlRes *msg__ = (CsrBtAvSecurityControlRes *) CsrPmemAlloc(sizeof(CsrBtAvSecurityControlRes)); \
        msg__->type = CSR_BT_AV_SECURITY_CONTROL_RES;                   \
        msg__->shandle = _shandle;                                      \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = _avResponse;                                \
        msg__->contProtMethodLen = 0;                                   \
        msg__->contProtMethodData= NULL;                                \
        CsrBtAvMsgTransport(msg__); }
#endif /* INSTALL_AV_SECURITY_CONTROL */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStreamDataReqSend
 *
 *  DESCRIPTION
 *      The source application uses this API to send media stream data to
 *      AV profile after the streaming start procedure starts, If AV cannot
 *      immediately transmit the stream data to the remote AV SINK device,
 *      the data is buffered internally by AV for later transmission.
 *      The internal AV buffer is flushed if the stream is closed, suspended, or aborted.
 *      The stream application receives a CSR_BT_AV_QOS_IND primitive each time a
 *      fixed number of CSR_BT_AV_STREAM_DATA_REQ primitives are submitted to the AV.
 *      The CSR_BT_AV_QOS_IND primitive contains information about the relative amount
 *      of data in the internal AV buffer, that is, the amount of data waiting to be
 *      transmitted to the remote AV device. 
 *      Note that CsrBtAvStreamDataReqSend sends media packets with real-time
 *      transport protocol (RTP)header (applies to all mandatory and optional codecs).
 *      To send the media packet for APTx Adaptive codec, CsrBtAvStreamAptxAdDataReqSend
 *      is used.
 *
 *  PARAMETERS
 *      shandle:           stream handle
 *      padding:           padding, packet is padded (media packet header bit field)
 *      marker:            marker (media packet header bit field)
 *      payload:           payload (media packet header field)
 *      timestamp:         packet timestamp (media packet header field)
 *      length:            length of data to be sent
 *      data               pointer to data
 *----------------------------------------------------------------------------*/
#ifndef CSR_STREAMS_ENABLE
#define CsrBtAvStreamDataReqSend(_shandle, _padding, _marker, _payload, _timeStamp,_length, _data) { \
        CsrBtAvStreamDataReq *msg__ = (CsrBtAvStreamDataReq *) CsrPmemZalloc(sizeof(CsrBtAvStreamDataReq)); \
        msg__->type = CSR_BT_AV_STREAM_DATA_REQ;                        \
        msg__->shandle = _shandle;                                      \
        CsrMemSet(_data, 0, CSR_BT_AV_FIXED_MEDIA_PACKET_HDR_SIZE);     \
        ((CsrUint8 *) _data)[0] = 0x80; /* RTP version = 2 */           \
        if(_padding)                                                    \
        {                                                               \
            ((CsrUint8 *) _data)[0] |= 0x20; /* add padding */          \
        }                                                               \
        ((CsrUint8 *) _data)[1] = (CsrUint8)((_marker<<7) | _payload); /* marker & payload */ \
        ((CsrUint8 *) _data)[4] = (CsrUint8) (0x000000FF & (_timeStamp>>24)); /* timestamp - big endian format */ \
        ((CsrUint8 *) _data)[5] = (CsrUint8) (0x000000FF & (_timeStamp>>16)); \
        ((CsrUint8 *) _data)[6] = (CsrUint8) (0x000000FF & (_timeStamp>>8)); \
        ((CsrUint8 *) _data)[7] = (CsrUint8) (0x000000FF & _timeStamp); \
        ((CsrUint8 *) _data)[8] = _shandle; /* SSRC */                  \
        msg__->hdr_type = CSR_BT_AV_MEDIA_PACKET_HDR_TYPE_RTP;          \
        msg__->length = _length;                                        \
        msg__->data = ((CsrUint8 *) _data);                             \
        CsrBtAvMsgTransport(msg__); }

#define CsrBtAvStreamAptxAdDataReqSend(_shandle, _padding, _marker, _payload, _timeStamp,_length, _data, aptx_ad_ssrc) { \
        CsrBtAvStreamDataReq *msg__ = (CsrBtAvStreamDataReq *) CsrPmemZalloc(sizeof(CsrBtAvStreamDataReq)); \
        msg__->type = CSR_BT_AV_STREAM_DATA_REQ;                        \
        msg__->shandle = _shandle;                                      \
        CsrMemSet(_data, 0, CSR_BT_AV_FIXED_MEDIA_PACKET_HDR_SIZE);     \
        ((CsrUint8 *) _data)[0] = 0x80; /* RTP version = 2 */           \
        if(_padding)                                                    \
        {                                                               \
            ((CsrUint8 *) _data)[0] |= 0x20; /* add padding */          \
        }                                                               \
        ((CsrUint8 *) _data)[1] = (CsrUint8)((_marker<<7) | _payload); /* marker & payload */ \
        ((CsrUint8 *) _data)[4] = (CsrUint8) (0x000000FF & (_timeStamp>>24)); /* timestamp - big endian format */ \
        ((CsrUint8 *) _data)[5] = (CsrUint8) (0x000000FF & (_timeStamp>>16)); \
        ((CsrUint8 *) _data)[6] = (CsrUint8) (0x000000FF & (_timeStamp>>8)); \
        ((CsrUint8 *) _data)[7] = (CsrUint8) (0x000000FF & _timeStamp); \
        ((CsrUint8 *) _data)[8] = 0; \
        ((CsrUint8 *) _data)[9] = 0; \
        ((CsrUint8 *) _data)[10] = (aptx_ad_ssrc>>8) & 0xff; \
        ((CsrUint8 *) _data)[11] = aptx_ad_ssrc & 0xff;		\
        msg__->hdr_type = CSR_BT_AV_MEDIA_PACKET_HDR_TYPE_RTP;          \
        msg__->length = _length;                                        \
        msg__->data = ((CsrUint8 *) _data);                             \
        CsrBtAvMsgTransport(msg__); }		

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStreamRawDataReqSend
 *
 *  DESCRIPTION
 *      Request for sending raw stream data transparently through AV.
 *      This API is used to send raw application specific media packets
 *      (for vendor specific codecs).
 *
 *  PARAMETERS
 *      shandle:           stream handle
 *      length:            length of data to be sent
 *      data               pointer to data
 *----------------------------------------------------------------------------*/
#define CsrBtAvStreamRawDataReqSend(_shandle, _length, _data) {         \
        CsrBtAvStreamDataReq *msg__ = (CsrBtAvStreamDataReq *) CsrPmemAlloc(sizeof(CsrBtAvStreamDataReq)); \
        msg__->type = CSR_BT_AV_STREAM_DATA_REQ;                        \
        msg__->shandle = _shandle;                                      \
        msg__->hdr_type = CSR_BT_AV_MEDIA_PACKET_HDR_TYPE_NONE;         \
        msg__->length = _length;                                        \
        msg__->data = _data;                                            \
        CsrBtAvMsgTransport(msg__); }
#endif /* ! CSR_STREAMS_ENABLE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetServiceCap
 *
 *  DESCRIPTION
 *      Search a list of service capabilities for a given serviceCap category.
 *
 *  PARAMETERS
 *      serviceCap:           service capability category to search for (or simply
 *                            the next service capability)
 *      list:                 pointer to service capability list
 *      length:               length of service capability list
 *      index:                index for current position in list
 *----------------------------------------------------------------------------*/
CsrUint8 *CsrBtAvGetServiceCap(CsrBtAvServCap  serviceCap,
                               CsrUint8        *list,
                               CsrUint16       length,
                               CsrUint16       *index);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvValidateServiceCap
 *
 *  DESCRIPTION
 *      Validate a service capability category.
 *
 *  PARAMETERS
 *      serviceCap_ptr:        pointer to service capability category
 *----------------------------------------------------------------------------*/
CsrBtAvResult CsrBtAvValidateServiceCap(CsrUint8 *serviceCap_ptr);


#ifdef INSTALL_AV_CUSTOM_SECURITY_SETTINGS
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityInReqSend
 *      CsrBtAvSecurityOutReqSend
 *
 *  DESCRIPTION
 *      Set the default security settings for new incoming/outgoing connections. AV responds
 *      the result of setting security request through CSR_BT_AV_SECURITY_IN/OUT_CFM message.
 *
 *  PARAMETERS
 *      secLevel:       Minimum incoming/outgoing security level requested by the application.
 *                      The application must specify one of the following values:
 *                      CSR_BT_SEC_DEFAULT:   Uses the default security settings.
 *                      CSR_BT_SEC_MANDATORY: Uses the mandatory security settings.
 *                      CSR_BT_SEC_SPECIFY:   Specifies new security settings.
 *                      If CSR_BT_SEC_SPECIFY is set, the following values can be OR'ed additionally,
 *                      i.e., combinations of these values can be used:
 *                      CSR_BT_SEC_AUTHORISATION:  Requires authorisation.
 *                      CSR_BT_SEC_AUTHENTICATION: Requires authentication.
 *                      CSR_BT_SEC_SEC_ENCRYPTION: Requires encryption (implies authentication).
 *                      CSR_BT_SEC_MITM:           Requires MITM protection (implies encryption).
 *----------------------------------------------------------------------------*/
#define CsrBtAvSecurityInReqSend(_appHandle, _secLevel) {               \
        CsrBtAvSecurityInReq *msg__ = (CsrBtAvSecurityInReq*)CsrPmemAlloc(sizeof(CsrBtAvSecurityInReq)); \
        msg__->type = CSR_BT_AV_SECURITY_IN_REQ;                        \
        msg__->appHandle = _appHandle;                                  \
        msg__->secLevel = _secLevel;                                    \
        CsrBtAvMsgTransport(msg__);}

#define CsrBtAvSecurityOutReqSend(_appHandle, _secLevel) {              \
        CsrBtAvSecurityOutReq *msg__ = (CsrBtAvSecurityOutReq*)CsrPmemAlloc(sizeof(CsrBtAvSecurityOutReq)); \
        msg__->type = CSR_BT_AV_SECURITY_OUT_REQ;                       \
        msg__->appHandle = _appHandle;                                  \
        msg__->secLevel = _secLevel;                                    \
        CsrBtAvMsgTransport(msg__);}
#endif /* INSTALL_AV_CUSTOM_SECURITY_SETTINGS */

#ifdef CSR_BT_INSTALL_AV_SET_QOS_INTERVAL
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSetQosIntervalReqSend
 *
 *  DESCRIPTION
 *      Request to set how often should AV report the buffer status (CSR_BT_AV_QOS_IND) in
 *      counts of CSR_BT_AV_STREAM_DATA_REQ primitives sent. If set to 0 (zero)
 *      an CSR_BT_AV_QOS_IND will only be sent in case the buffer is full
 *      and subsequently when the buffer again is emptied.
 *
 *  PARAMETERS
 *      qosInterval: The qos interval.
 *----------------------------------------------------------------------------*/
#define CsrBtAvSetQosIntervalReqSend(_qosInterval) {                        \
        CsrBtAvSetQosIntervalReq *msg__ = (CsrBtAvSetQosIntervalReq *) CsrPmemAlloc(sizeof(CsrBtAvSetQosIntervalReq)); \
        msg__->type  = CSR_BT_AV_SET_QOS_INTERVAL_REQ;                  \
        msg__->qosInterval   = _qosInterval;                            \
        CsrBtAvMsgTransport(msg__);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDelayReportReqSend
 *
 *  DESCRIPTION
 *      To allow synchronization between audio or video streaming, the sink
 *      device should let the source device know whether there is a delay in
 *      playing the audio or video at the sink. Hence, the source can compensate
 *      for this delay when streaming the data. 
 *      Sink application uses this API to configure the delay. AV responds the
 *      result of Delay Report request through CSR_BT_AV_DELAY_REPORT_CFM message.
 *
 *  PARAMETERS
 *      delay:      16-bit value to indicate the delay in 10ths 
 *                  of miliseconds (max 6 seconds delay)
 *      shandle:    stream handle
 *      tLabel:     transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvDelayReportReqSend(_delay, _shandle, _tLabel) {          \
        CsrBtAvDelayReportReq *msg__ = (CsrBtAvDelayReportReq *)CsrPmemAlloc(sizeof(CsrBtAvDelayReportReq)); \
        msg__->type = CSR_BT_AV_DELAY_REPORT_REQ;                       \
        msg__->delay = _delay;                                          \
        msg__->shandle   = _shandle;                                    \
        msg__->tLabel= _tLabel;                                         \
        CsrBtAvMsgTransport(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDelayReportResAcpSend
 *
 *  DESCRIPTION
 *      When remote Sink device sends the Delay report to configure the delay,
 *      AV indicates the remote request to application through CSR_BT_AV_DELAY_REPORT_IND.
 *      Application uses this API to send accept response to above indication.
 *
 *  PARAMETERS
 *      shandle:    stream handle
 *      tLabel:     transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvDelayReportResAcpSend(_shandle, _tLabel) {          \
        CsrBtAvDelayReportRes *msg__ = (CsrBtAvDelayReportRes *)CsrPmemAlloc(sizeof(CsrBtAvDelayReportRes)); \
        msg__->type = CSR_BT_AV_DELAY_REPORT_RES;                       \
        msg__->shandle   = _shandle;                                    \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        CsrBtAvMsgTransport(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDelayReportResRejSend
 *
 *  DESCRIPTION
 *      When remote Sink device sends the Delay report to configure the delay,
 *      AV indicates the remote request to application through CSR_BT_AV_DELAY_REPORT_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      shandle:    stream handle
 *      tLabel:     transaction label
 *      avResponse: cause of rejection
 *----------------------------------------------------------------------------*/
#define CsrBtAvDelayReportResRejSend(_shandle, _tLabel, _avResponse) {          \
        CsrBtAvDelayReportRes *msg__ = (CsrBtAvDelayReportRes *)CsrPmemAlloc(sizeof(CsrBtAvDelayReportRes)); \
        msg__->type = CSR_BT_AV_DELAY_REPORT_RES;                       \
        msg__->shandle   = _shandle;                                    \
        msg__->tLabel= _tLabel;                                         \
        msg__->avResponse = _avResponse;                                \
        CsrBtAvMsgTransport(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetAllCapabilitiesResRejSend
 *
 *  DESCRIPTION
 *      When remote device sends the request to Get All Capabilities for a stream end-point.
 *      AV indicates the remote request to application through CSR_BT_AV_GET_ALL_CAPABILITIES_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      connectionId:          connection identifier
 *      tLabel:                transaction label
 *      avResponse:            cause for rejection
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetAllCapabilitiesResRejSend(_connectionId, _tLabel, _avResponse) { \
        CsrBtAvGetAllCapabilitiesRes *msg__ = (CsrBtAvGetAllCapabilitiesRes *) CsrPmemAlloc(sizeof(CsrBtAvGetAllCapabilitiesRes)); \
        msg__->type = CSR_BT_AV_GET_ALL_CAPABILITIES_RES;               \
        msg__->connectionId = _connectionId;                            \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = _avResponse;                                \
        msg__->servCapLen = 0;                                          \
        msg__->servCapData = NULL;                                      \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetAllCapabilitiesResAcpSend
 *
 *  DESCRIPTION
 *      When remote device sends the request to Get All Capabilities for a stream end-point.
 *      AV indicates the remote request to application through CSR_BT_AV_GET_ALL_CAPABILITIES_IND.
 *      Application uses this API to send reject response to above indication.
 *
 *  PARAMETERS
 *      connectionId:          connection identifier
 *      tLabel:                transaction label
 *      servCapLen:            length of application service capabilities
 *      servCapData:           pointer to application service capabilities
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetAllCapabilitiesResAcpSend(_connectionId, _tLabel, _servCapLen, _servCapData) { \
        CsrBtAvGetAllCapabilitiesRes *msg__ = (CsrBtAvGetAllCapabilitiesRes *) CsrPmemAlloc(sizeof(CsrBtAvGetAllCapabilitiesRes)); \
        msg__->type = CSR_BT_AV_GET_ALL_CAPABILITIES_RES;               \
        msg__->connectionId = _connectionId;                            \
        msg__->tLabel = _tLabel;                                        \
        msg__->avResponse = CSR_BT_AV_ACCEPT;                           \
        msg__->servCapLen = _servCapLen;                                \
        msg__->servCapData = _servCapData;                              \
        CsrBtAvMsgTransport(msg__); }

#ifdef CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetChannelInfoReqSend
 *
 *  DESCRIPTION
 *      Get ACL link ID and remote CID information using Global Bluetooth connection ID
 *      received through AV Connect Indication/Confirmation message.
 *      AV responds the result of Get Channel Info request through
 *      CSR_BT_AV_GET_CHANNEL_INFO_CFM message.
 *
 *  PARAMETERS
 *      btConnId: Global bluetooth conneciton Id received through AV Connect Indication/Confirmation
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetChannelInfoReqSend(_btConnId) { \
        CsrBtAvGetChannelInfoReq *msg__ = (CsrBtAvGetChannelInfoReq *) CsrPmemAlloc(sizeof(CsrBtAvGetChannelInfoReq)); \
        msg__->type     = CSR_BT_AV_GET_CHANNEL_INFO_REQ; \
        msg__->btConnId = _btConnId;                      \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetStreamChannelInfoReqSend
 *
 *  DESCRIPTION
 *      Get ACL link ID and remote CID information using Stream handle.
 *      AV responds the result of Get Stream Channel Info request through
 *      CSR_BT_AV_GET_CHANNEL_INFO_CFM message.
 *
 *  PARAMETERS
 *      shandle: stream handle.
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetStreamChannelInfoReqSend(_shandle) {\
        CsrBtAvGetStreamChannelInfoReq *msg__ = (CsrBtAvGetStreamChannelInfoReq *) CsrPmemAlloc(sizeof(CsrBtAvGetStreamChannelInfoReq)); \
        msg__->type = CSR_BT_AV_GET_STREAM_CHANNEL_INFO_REQ; \
        msg__->shandle = _shandle;                                \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetMediaChannelInfoReqSend
 *
 *  DESCRIPTION
 *      Get media (stream) channel's ACL link ID along with local and remote
 *      CID information. AV responds the result of Get Media Channel Info request
 *      through CSR_BT_AV_GET_MEDIA_CHANNEL_INFO_CFM message.
 *
 *  PARAMETERS
 *      shandle: stream handle.
 *----------------------------------------------------------------------------*/
#define CsrBtAvGetMediaChannelInfoReqSend(_shandle) {\
        CsrBtAvGetMediaChannelInfoReq *msg__ = (CsrBtAvGetMediaChannelInfoReq *) CsrPmemAlloc(sizeof(*msg__)); \
        msg__->type = CSR_BT_AV_GET_MEDIA_CHANNEL_INFO_REQ; \
        msg__->shandle = _shandle;                                \
        CsrBtAvMsgTransport(msg__); }
#endif /* CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSetStreamInfoReqSend
 *
 *  DESCRIPTION
 *      After the stream is configured, the AV application updates stream related
 *      information such as codec location, when using CSR Combo chips. This is
 *      required to ensure acceptable audio quality during Bluetooth/WLAN coexistence.
 *      Application uses this API to update the stream related information.
 *
 *  PARAMETERS
 *      shandle: stream handle.
 *      sInfo:   Identifies the information related to stream, for instance codec location - on/off-chip
 *----------------------------------------------------------------------------*/
#define CsrBtAvSetStreamInfoReqSend(_shandle, _sInfo) {\
        CsrBtAvSetStreamInfoReq *msg__ = (CsrBtAvSetStreamInfoReq *) CsrPmemAlloc(sizeof(CsrBtAvSetStreamInfoReq)); \
        msg__->type    = CSR_BT_AV_SET_STREAM_INFO_REQ; \
        msg__->shandle = _shandle;                               \
        msg__->sInfo   = _sInfo;                         \
        CsrBtAvMsgTransport(msg__); }

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvFreeUpstreamMessageContents
 *
 *  DESCRIPTION
 *      During Bluetooth shutdown all allocated payload in the Synergy BT AV
 *      message must be deallocated. This is done by this function
 *
 *  PARAMETERS
 *      eventClass :  Must be CSR_BT_AV_PRIM,
 *      msg:          The message received from Synergy BT AV
 *----------------------------------------------------------------------------*/
void CsrBtAvFreeUpstreamMessageContents(CsrUint16 eventClass, void * message);

#ifdef CSR_TARGET_PRODUCT_VM
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetConId
 *
 *  DESCRIPTION
 *      Fetches correct av connection Id for given bluetooth device address.
 *      Initialize device_id in application to INVALID_DEVICE_ID(0XFF) before
 *      calling this function.
 *
 *  PARAMETERS
 *      addr :  bluetooth address of the device.
 *      conId:  output connection id information.
 *----------------------------------------------------------------------------*/
void CsrBtAvGetConId(CsrBtDeviceAddr *addr, CsrUint8 *conId);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvUseLargeMtu
 *
 *  DESCRIPTION
 *      Use large MTU for media channel creation for the requested connection id.
 *
 *  PARAMETERS
 *      conId:        connection id for which large mtu to be used.
 *      useLargeMtu : whether to use large MTU or not.
 *----------------------------------------------------------------------------*/
void CsrBtAvUseLargeMtu(CsrUint8 conId, CsrBool useLargeMtu);
#endif /* CSR_TARGET_PRODUCT_VM */

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_AV_LIB_H__ */
