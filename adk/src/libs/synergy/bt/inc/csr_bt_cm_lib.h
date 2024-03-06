#ifndef CSR_BT_CM_LIB_H__
#define CSR_BT_CM_LIB_H__
/******************************************************************************
 Copyright 2001-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_profiles.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_msg_transport.h"
#include "csr_bt_cm_prim.h"
#include "csr_bt_tasks.h"
#include "csr_bt_addr.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Common put_message functions to reduce code size */
void CsrBtCmMsgTransport(void *msg);
void CsrBtCmPutMessageDownstream(void *msg);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetLocalNameReqSend
 *
 *  DESCRIPTION
 *      Set the device name in the local device, which can be retrieved by 
 *      other devices.
 *      CM sends CSR_BT_CM_SET_LOCAL_NAME_CFM message back to the application.
 *
 *  PARAMETERS
 *        phandle:            Identity of the calling process
 *        friendlyName:       new name of local device
 *----------------------------------------------------------------------------*/
#define CsrBtCmSetLocalNameReqSend(_phandle, _friendlyName) {           \
        CsrBtCmSetLocalNameReq *msg__ = (CsrBtCmSetLocalNameReq*) CsrPmemAlloc(sizeof(CsrBtCmSetLocalNameReq)); \
        msg__->type = CSR_BT_CM_SET_LOCAL_NAME_REQ;                     \
        msg__->phandle = _phandle;                                      \
        msg__->friendlyName = _friendlyName;                            \
        CsrBtCmMsgTransport(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadLocalBdAddrReqSend
 *
 *  DESCRIPTION
 *      Read the local device Bluetooth address.
 *      CM sends CSR_BT_CM_READ_LOCAL_BD_ADDR_CFM back to the application.
 *
 *  PARAMETERS
 *        phandle:            Identity of the calling process
 *----------------------------------------------------------------------------*/
CsrBtCmReadLocalBdAddrReq *CsrBtCmReadLocalBdAddrReq_struct(CsrSchedQid    thePhandle);

#define CsrBtCmReadLocalBdAddrReqSend(_phandle) {               \
        CsrBtCmReadLocalBdAddrReq *__msg;                       \
        __msg=CsrBtCmReadLocalBdAddrReq_struct(_phandle);       \
        CsrBtCmMsgTransport(__msg);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadLocalNameReqSend
 *
 *  DESCRIPTION
 *      Read the name of the local device.
 *      CM sends CSR_BT_CM_READ_LOCAL_NAME_CFM back to the application.
 *
 *  PARAMETERS
 *        phandle:            Identity of the calling process
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_READ_LOCAL_NAME 
CsrBtCmReadLocalNameReq *CsrBtCmReadLocalNameReq_struct(CsrSchedQid    thePhandle);

#define CsrBtCmReadLocalNameReqSend(_phandle) {         \
        CsrBtCmReadLocalNameReq *__msg;                 \
        __msg=CsrBtCmReadLocalNameReq_struct(_phandle); \
        CsrBtCmMsgTransport(__msg);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadRemoteNameReqSend
 *
 *  DESCRIPTION
 *      Read the name of the remote device, which is identified by its
 *      Bluetooth device address.
 *      CM sends CSR_BT_CM_READ_REMOTE_NAME_CFM back to the application.
 *
 *  PARAMETERS
 *        phandle:            Identity of the calling process
 *        deviceAddr:         BT address of the device to read remote name
 *----------------------------------------------------------------------------*/
void CsrBtCmReadRemoteNameReqSend(CsrSchedQid       phandle,
                                  CsrBtDeviceAddr   deviceAddr);




/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadRemoteVersionReqSend
 *
 *  DESCRIPTION
 *      Read the version of a remote device, which is identified by its 
 *      Bluetooth device address.
 *      CM sends CSR_BT_CM_READ_REMOTE_VERSION_CFM back to the application.
 *
 *  PARAMETERS
 *        phandle:            Identity of the calling process
 *        deviceAddr:         BT address of the device to read remote version
 *        addressType:        Address type of 'deviceAddr' (see CSR_BT_ADDR_ defines
 *                            in csr_bt_addr.h)
 *        transportType:      Transport type (see CSR_BT_TRANSPORT_ defines
 *                            in csr_bt_addr.h).
 *----------------------------------------------------------------------------*/
void CsrBtCmReadRemoteVersionReqSend(CsrSchedQid        phandle,
                                     CsrBtDeviceAddr    deviceAddr,
                                     CsrBtAddressType   addressType,
                                     CsrBtTransportType transportType);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtcmWriteLinkSuperVisionTimeoutReqSend
 *
 *  DESCRIPTION
 *      This command will write the value of the link supervision timeout
 *        parameter. It is used to monitor link loss. Value of 0x0000 disables it.
 *        The timeout value N range from 0x0001 - 0xffff. In seconds it means from
 *        N*0.625ms = 0,625ms - 40.9 seconds.
 *        CM sends CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_CFM back to the application.
 *
 *  PARAMETERS
 *        phandle:            Identity of the calling process
 *        deviceAddr:         BT address of the remote device
 *        timeout:            Link supervision timeout value.
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_WRITE_LINK_SUPERVISION_TIMEOUT
CsrBtCmWriteLinkSupervTimeoutReq *CsrBtCmWriteLinkSuperVisionTimeoutReq_struct(CsrSchedQid       thePhandle,
                                                                               CsrBtDeviceAddr    theDeviceAddr,
                                                                               CsrUint16        timeout);

#define CsrBtcmWriteLinkSuperVisionTimeoutReqSend(_phandle,_deviceAddr,_timeout) { \
        CsrBtCmWriteLinkSupervTimeoutReq *__msg;                        \
        __msg=CsrBtCmWriteLinkSuperVisionTimeoutReq_struct(_phandle,_deviceAddr,_timeout); \
        CsrBtCmMsgTransport(__msg);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadTxPowerLevelReqSend
 *
 *  DESCRIPTION
 *      Reads the transmission power level of the remote device.
 *      CM sends CSR_BT_CM_READ_TX_POWER_LEVEL_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:          Identity of the calling process
 *        deviceAddr:         BT address of the device to read Tx power level
 *        addressType:        Address type of 'deviceAddr' (see CSR_BT_ADDR_ defines
 *                            in csr_bt_addr.h)
 *        transportType:      The transport type (see CSR_BT_TRANSPORT_ defines
 *                            in csr_bt_addr.h).
 *        levelType:          The maximum power level as defined in the Bluetooth HCI
 *                            specification.
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_READ_TX_POWER_LEVEL
CsrBtCmReadTxPowerLevelReq *CsrBtCmReadTxPowerLevelReq_struct(CsrSchedQid thePhandle,
                                                              CsrBtDeviceAddr theDeviceAddr,
                                                              CsrBtAddressType theAddressType,
                                                              CsrBtTransportType theTransportType,
                                                              CsrUint8 theLevelType);

#define CsrBtCmReadTxPowerLevelReqSend(_appHandle,_deviceAddr,_addressType, _transportType, _levelType) { \
        CsrBtCmReadTxPowerLevelReq *__msg;                              \
        __msg=CsrBtCmReadTxPowerLevelReq_struct(_appHandle,_deviceAddr,_addressType, _transportType, _levelType); \
        CsrBtCmMsgTransport(__msg);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmGetLinkQualityReqSend
 *
 *  DESCRIPTION
 *      Gets the link quality of the device, which is identified by 'deviceAddr'.
 *      CM sends CSR_BT_CM_GET_LINK_QUALITY_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:          Identity of the calling process
 *        deviceAddr:         BT address of the device to get the link quality
 *                            parameter
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_GET_LINK_QUALITY
#define CsrBtCmGetLinkQualityReqSend(_appHandle,_deviceAddr) {            \
        CsrBtCmGetLinkQualityReq *__msg = (CsrBtCmGetLinkQualityReq*) CsrPmemAlloc(sizeof(CsrBtCmGetLinkQualityReq)); \
        __msg->type = CSR_BT_CM_GET_LINK_QUALITY_REQ;                   \
        __msg->appHandle = _appHandle;                                    \
        __msg->deviceAddr = _deviceAddr;                                \
        CsrBtCmMsgTransport(__msg);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadRssiReqSend
 *
 *  DESCRIPTION
 *      Reads the RSSI value of the link, which is identified by 'deviceAddr'.
 *      CM sends CSR_BT_CM_READ_RSSI_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:          Identity of the calling process
 *        deviceAddr:         BT address of the device to read RSSI value
 *        addressType:        Address type of 'deviceAddr' (see CSR_BT_ADDR_ defines
 *                            in csr_bt_addr.h)
 *        transportType:      The transport type (see CSR_BT_TRANSPORT_ defines
 *                            in csr_bt_addr.h).
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_READ_RSSI
#define CsrBtCmReadRssiReqSend(_appHandle,_deviceAddr,_addressType, _transportType) {   \
        CsrBtCmReadRssiReq *__msg = (CsrBtCmReadRssiReq*) CsrPmemAlloc(sizeof(CsrBtCmReadRssiReq)); \
        __msg->type = CSR_BT_CM_READ_RSSI_REQ;                          \
        __msg->appHandle = _appHandle;                                    \
        __msg->deviceAddr = _deviceAddr;                                \
        __msg->addressType = _addressType;                              \
        __msg->transportType = _transportType;                              \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteMajorMinorCodReqSend
 *
 *  DESCRIPTION
 *      Sets major/monor class of device.
 *      CM sends CSR_BT_CM_WRITE_COD_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:              Identity of the calling process
 *      majorClassOfDevice:     A Major Class of device value given from the application
 *      minorClassOfDevice:     A Minor Class of device value given from the application
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_WRITE_COD
CsrBtCmWriteCodReq *CsrBtCmWriteCodReq_struct(CsrSchedQid phandle,
                                              CsrBtCmUpdateFlags updateFlags,
                                              CsrBtClassOfDevice   service,
                                              CsrBtClassOfDevice   major,
                                              CsrBtClassOfDevice   minor);

#define CsrBtCmWriteMajorMinorCodReqSend(_appHandle,_majorClassOfDevice,_minorClassOfDevice) {    \
        CsrBtCmWriteCodReq *__msg;                                      \
        __msg=CsrBtCmWriteCodReq_struct(_appHandle, CSR_BT_CM_WRITE_COD_UPDATE_FLAG_MAJOR_MINOR_CLASS, 0, _majorClassOfDevice, _minorClassOfDevice); \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteServiceCodReqSend
 *
 *  DESCRIPTION
 *      Sets the service class of device value.
 *      CM sends CSR_BT_CM_WRITE_COD_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:               Identity of the calling process
 *        serviceClassOfDevice:    Service Class of device value given from the application
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_WRITE_COD
#define CsrBtCmWriteServiceCodReqSend(_appHandle,_serviceClassOfDevice) {              \
        CsrBtCmWriteCodReq *__msg;                                      \
        __msg=CsrBtCmWriteCodReq_struct(_appHandle, CSR_BT_CM_WRITE_COD_UPDATE_FLAG_SERVICE_CLASS, _serviceClassOfDevice, 0, 0); \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadCodReqSend
 *
 *  DESCRIPTION
 *      Reads the class of device value.
 *      CM sends CSR_BT_CM_READ_COD_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:    Identity of the calling process
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_READ_COD
#define CsrBtCmReadCodReqSend(_appHandle) {                               \
        CsrBtCmReadCodReq *__msg = (CsrBtCmReadCodReq*) CsrPmemAlloc(sizeof(CsrBtCmReadCodReq)); \
        __msg->type = CSR_BT_CM_READ_COD_REQ;                           \
        __msg->appHandle = _appHandle;                                    \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteScanEnableReqSend
 *
 *  DESCRIPTION
 *      Controls whether or not the local Bluetooth device periodically scans
 *      for page attempts and/or inquiry requests from other Bluetooth devices.
 *      CM sends CSR_BT_CM_WRITE_SCAN_ENABLE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:            Identity of the calling process
 *        disableInquiryScan:   TRUE disable inquiry scan
 *        disablePageScan:      TRUE disable page scan
 *----------------------------------------------------------------------------*/
#define CsrBtCmWriteScanEnableReqSend(_appHandle,_disableInquiryScan, _disablePageScan) { \
        CsrBtCmWriteScanEnableReq *__msg = (CsrBtCmWriteScanEnableReq*) CsrPmemAlloc(sizeof(CsrBtCmWriteScanEnableReq)); \
        __msg->type = CSR_BT_CM_WRITE_SCAN_ENABLE_REQ;                  \
        __msg->appHandle = _appHandle;                                    \
        __msg->disableInquiryScan = _disableInquiryScan;                \
        __msg->disablePageScan = _disablePageScan;                      \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadScanEnableReqSend
 *
 *  DESCRIPTION
 *      Reads scan enable configuration parameter value.
 *      CM sends CSR_BT_CM_READ_SCAN_ENABLE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:           Identity of the calling process
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_READ_SCAN_EANBLE
#define CsrBtCmReadScanEnableReqSend(_appHandle) {                        \
        CsrBtCmReadScanEnableReq *msg__ = (CsrBtCmReadScanEnableReq*) CsrPmemAlloc(sizeof(CsrBtCmReadScanEnableReq)); \
        msg__->type = CSR_BT_CM_READ_SCAN_ENABLE_REQ;                   \
        msg__->appHandle = _appHandle;                                    \
        CsrBtCmMsgTransport(msg__);}
#endif

#ifdef CSR_BT_INSTALL_CM_SET_LINK_BEHAVIOR
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetLinkBehaviorReqSend
 *
 *  DESCRIPTION
 *      Sets the behavior of the link.
 *      CM sends CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_CFM back to the application.
 *
 *      This API shall be deprecated, instead use CmSetLinkBehaviorReqSendExt
 *
 *  PARAMETERS
 *      apphandle:               app handle
 *      addrType:                address type
 *      addr                     device address
 *      l2capRetry              .whether to enable l2cap retry or not
 *----------------------------------------------------------------------------*/
void CsrBtCmSetLinkBehaviorReqSend(CsrSchedQid appHandle,
                                   CsrBtAddressType addrType,
                                   CsrBtDeviceAddr addr,
                                   CsrBool l2capRetry);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmSetLinkBehaviorReqSendExt
 *
 *  DESCRIPTION
 *      Sets the behavior of the link.
 *      CM sends CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_CFM back to the application.
 *
 *  PARAMETERS
 *      apphandle:               app handle
 *      addrType:                address type
 *      addr                     device address
 *      flags                    flags to enable or disable feature
 *----------------------------------------------------------------------------*/
void CmSetLinkBehaviorReqSendExt(CsrSchedQid appHandle,
                                   CsrBtAddressType addrType,
                                   CsrBtDeviceAddr addr,
                                   CmSetLinkBehaviorMask flags);
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcSearchReqSend
 *      CsrBtCmSdcSearchUuidReqSend
 *
 *  DESCRIPTION
 *      Submits a search request to the SDC sub-system
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      deviceAddr:          Bluetooth address of the peer device to be searched
 *      serviceList:         A list of Services (UUID) to search for
 *      serviceListSize:     Number of services to search for
 *----------------------------------------------------------------------------*/
void CsrBtCmSdcSearchReqSendFunc(CsrSchedQid     appHandle,
                                 CsrBtDeviceAddr deviceAddr,
                                 CsrBtUuid32    *serviceList,
                                 CsrUint8        serviceListSize,
                                 CsrBool         extendedUuidSearch);

#define CsrBtCmSdcSearchReqSend(_appHandle,_deviceAddr,_serviceList,_serviceListSize) CsrBtCmSdcSearchReqSendFunc(_appHandle, _deviceAddr, _serviceList, _serviceListSize, FALSE)

#define CsrBtCmSdcSearchUuidReqSend(_appHandle,_deviceAddr,_serviceList,_serviceListSize) CsrBtCmSdcSearchReqSendFunc(_appHandle, _deviceAddr, _serviceList, _serviceListSize, TRUE)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmSdcServiceSearchAttrReqSendFunc
 *
 *  DESCRIPTION
 *      Submits a service search attribute request to the SDC sub-system.
 *      This request allows the application to search n number of attributes for multiple
 *      service UUIDs in a single transaction.
 *      Application receives CM_SDC_SERVICE_SEARCH_ATTR_IND with the SDP result for each service,
 *      if request is made for more than one service UUID and if more than one instance is supported
 *      by the remote for the same service UUID.
 *      CM sends CM_SDC_SERVICE_SEARCH_ATTR_CFM to application with the last service UUID's attribute
 *      search result on the SDP procedure completion.
 *
 *  PARAMETERS
 *      appHandle:              Identity of the calling process
 *      deviceAddr:             Bluetooth address of the peer device to be searched
 *      svcSearchAttrInfoList:  A structure containing attribute info (number of attributes and
 *                              the list of attributes to request for), number of services and
 *                              the list of services (UUID) to search for
 *      extendedUuidSearch:     Boolean to enable/disable extended search
 *      uuidType:               Identifies the type of service uuid
 *      localServerChannel:     Holds the local server channel number in case of
 *                              outgoing connection. CSR_BT_NO_SERVER for incoming connections
 *----------------------------------------------------------------------------*/
void CmSdcServiceSearchAttrReqSendFunc(CsrSchedQid                  appHandle,
                                       CsrBtDeviceAddr              deviceAddr,
                                       cmSdcServiceSearchAttrInfo  *svcSearchAttrInfoList,
                                       CsrBool                      extendedUuidSearch,
                                       CsrUint8                     uuidType,
                                       CsrUint8                     localServerChannel);

#define CmSdcServiceSearchAttrReqSend(_phandle,_deviceAddr,_svcSearchAttrInfoList,_extendedUuidSearch,_uuidType,_localServerChannel) CmSdcServiceSearchAttrReqSendFunc(_phandle,_deviceAddr,_svcSearchAttrInfoList,_extendedUuidSearch,_uuidType,_localServerChannel)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcCancelSearchReqSend
 *
 *  DESCRIPTION
 *      Cancel a CSR_BT_CM_SDC_SEARCH_REQ.
 *      CM sends CSR_BT_CM_SDC_CLOSE_IND signal back to the application.
 *
 *  PARAMETERS
 *        appHandle:             Identity of the calling process
 *        deviceAddr:            BT address previously used in SDC serach request 
 *----------------------------------------------------------------------------*/
#define CsrBtCmSdcCancelSearchReqSend(_appHandle,_deviceAddr) {         \
        CsrBtCmSdcCancelSearchReq *__msg = (CsrBtCmSdcCancelSearchReq*) CsrPmemAlloc(sizeof(CsrBtCmSdcCancelSearchReq)); \
        __msg->type = CSR_BT_CM_SDC_CANCEL_SEARCH_REQ;                  \
        __msg->appHandle = _appHandle;                                  \
        __msg->deviceAddr = _deviceAddr;                                \
        __msg->typeToCancel = CSR_BT_CM_SDC_SEARCH_REQ;                 \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmSdcCancelServiceSearchAttrReqSend
 *
 *  DESCRIPTION
 *      Cancel a CM_SDC_SERVICE_SEARCH_ATTR_REQ.
 *      CM sends CSR_BT_CM_SDC_CLOSE_IND signal back to the application.
 *
 *  PARAMETERS
 *        appHandle:             Identity of the calling process
 *        deviceAddr:            BT address previously used in SDC serach request
 *----------------------------------------------------------------------------*/
#define CmSdcCancelServiceSearchAttrReqSend(_appHandle,_deviceAddr) {         \
        CsrBtCmSdcCancelSearchReq *__msg = (CsrBtCmSdcCancelSearchReq*) CsrPmemAlloc(sizeof(CsrBtCmSdcCancelSearchReq)); \
        __msg->type = CSR_BT_CM_SDC_CANCEL_SEARCH_REQ;                        \
        __msg->appHandle = _appHandle;                                        \
        __msg->deviceAddr = _deviceAddr;                                      \
        __msg->typeToCancel = CM_SDC_SERVICE_SEARCH_ATTR_REQ;                 \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcUuid128SearchReqSend
 *
 *  DESCRIPTION
 *      Submits a search request with a 128 bit uuid16_t to the SDC sub-system.
 *      CM sends CSR_BT_CM_SDC_CLOSE_IND in cases where either there are no services
 *      specified to search or already there is an existing search ongoing for given
 *      bd address from the same caller.
 *      If all parameters are fine and the search is started successfully CM sends
 *      CSR_BT_CM_SDC_SEARCH_CFM. When the search is not started successfully or is cancelled,
 *      CM sends CSR_BT_CM_SDC_CLOSE_IND with the appropriate result code indicating the failure.
 *
 *  PARAMETERS
 *        appHandle:         Identity of the calling process
 *        deviceAddr:        Bluetooth address of the peer device to be searched
 *        serviceList:       A list of 128 bit Services (UUID128) to search for
 *        serviceListSize:   Number of services of 128 bit to search for
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
CsrBtCmSdcUuid128SearchReq *CsrBtCmSdcUuid128SearchReq_struct(CsrSchedQid       appHandle,
                                                              CsrBtDeviceAddr    deviceAddr,
                                                              CsrBtUuid128       *serviceList,
                                                              CsrUint8         serviceListSize);

#define CsrBtCmSdcUuid128SearchReqSend(_appHandle,_deviceAddr,_serviceList,_serviceListSize) { \
        CsrBtCmSdcUuid128SearchReq *__msg;                              \
        __msg=CsrBtCmSdcUuid128SearchReq_struct(_appHandle,_deviceAddr,_serviceList,_serviceListSize); \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcCancelUuid128SearchReqSend
 *
 *  DESCRIPTION
 *      Cancel a CSR_BT_CM_SDC_UUID128_SEARCH_REQ
 *
 *  PARAMETERS
 *        appHandle:             Identity of the calling process
 *        deviceAddr:            BT address previously used in SDC serach request
 *----------------------------------------------------------------------------*/
#define CsrBtCmSdcCancelUuid128SearchReqSend(_appHandle,_deviceAddr) {  \
        CsrBtCmSdcCancelSearchReq *__msg = (CsrBtCmSdcCancelSearchReq*) CsrPmemAlloc(sizeof(CsrBtCmSdcCancelSearchReq)); \
        __msg->type = CSR_BT_CM_SDC_CANCEL_SEARCH_REQ;                  \
        __msg->appHandle = _appHandle;                                  \
        __msg->deviceAddr = _deviceAddr;                                \
        __msg->typeToCancel = CSR_BT_CM_SDC_UUID128_SEARCH_REQ;         \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcAttributeReqSend
 *      CsrBtCmSdcAttributeRangeReqSend
 *
 *  DESCRIPTION
 *      Submits an attribute request to the SDC sub-system.
 *      CM Sends CSR_BT_CM_SDC_ATTRIBUTE_CFM with the result of request if the attribute request
 *      is placed successfully, in cases when the request is not placed, the confirmation is not received.
 *
 *  PARAMETERS
 *        serviceHandle:                The handle of the service
 *        attributeIdentifier:          The attribute to retrieve. Note, if
 *                                      CsrBtCmSdcAttributeRangeReqSend is used
 *                                      then it defines the beginning attributeId
 *                                      of the range
 *        upperRangeAttributeIdentifier:The ending attributeId of the range
 *        maxBytesToReturn:             The maximum number of attribute bytes
 *                                      to be returned
 *----------------------------------------------------------------------------*/
void CsrBtCmSdcAttributeRangeReqSend(CsrBtUuid32 serviceHandle,
                                     CsrUint16   attributeIdentifier,
                                     CsrUint16   upperRangeAttributeIdentifier,
                                     CsrUint16   maxBytesToReturn);

#define CsrBtCmSdcAttributeReqSend(_serviceHandle,_attributeIdentifier,_maxBytesToReturn) CsrBtCmSdcAttributeRangeReqSend(_serviceHandle,_attributeIdentifier,0,_maxBytesToReturn)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcCloseReqSend
 *
 *  DESCRIPTION
 *      Closing the SDC channel.
 *      CM sends CSR_BT_CM_SDC_CLOSE_IND if the close request is placed, indicating
 *      to the application that it may now begin another search (if required).
 *
 *  PARAMETERS
 *        appHandle:            Identity of the calling process
 *----------------------------------------------------------------------------*/
void CsrBtCmSdcCloseReqSend(CsrSchedQid   appHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdsRegisterReqSend
 *
 *  DESCRIPTION
 *      Request to register a service with the service discovery server.
 *      If context is CSR_BT_CM_CONTEXT_UNUSED, CM sends CSR_BT_CM_SDS_REGISTER_CFM
 *      back to the application
 *      else CM sends CSR_BT_CM_SDS_EXT_REGISTER_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:            Identity of the calling process
 *        serviceRecord:        The service record
 *        serviceRecordSize:    Size of the service record
 *        context                Opaque context number
 *----------------------------------------------------------------------------*/
void CsrBtCmSdsRegisterReqSend(CsrSchedQid appHandle,
                               CsrUint8 *serviceRecord,
                               CsrUint16 serviceRecordSize,
                               CsrUint16 context);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdsUnRegisterReqSend
 *
 *  DESCRIPTION
 *      Request to unregister a service with the service discovery server.
 *      If context is CSR_BT_CM_CONTEXT_UNUSED, CM sends CSR_BT_CM_SDS_UNREGISTER_CFM
 *      back to the application
 *      else CM sends CSR_BT_CM_SDS_EXT_UNREGISTER_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:            Identity of the calling process
 *        serviceRecHandle:     The service record handle
 *        context               Opaque context number
 *----------------------------------------------------------------------------*/
void CsrBtCmSdsUnRegisterReqSend(CsrSchedQid appHandle,
                                 CsrUint32 serviceRecHandle,
                                 CsrUint16 context);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmEnableDutModeReqSend
 *
 *  DESCRIPTION
 *      Enables the device under test mode on the BlueCore chip.
 *      CM sends CSR_BT_CM_ENABLE_DUT_MODE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:        Identity of the calling process
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_DUT_MODE
#define CsrBtCmEnableDutModeReqSend(_appHandle) {                         \
        CsrBtCmEnableDutModeReq *__msg = (CsrBtCmEnableDutModeReq*) CsrPmemAlloc(sizeof(CsrBtCmEnableDutModeReq)); \
        __msg->type = CSR_BT_CM_ENABLE_DUT_MODE_REQ;                    \
        __msg->appHandle = _appHandle;                                     \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmDisableDutModeReqSend
 *
 *  DESCRIPTION
 *      Disables the device under test mode in the CM module.
 *      CM sends CSR_BT_CM_DISABLE_DUT_MODE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:        Identity of the calling process
 *----------------------------------------------------------------------------*/
#define CsrBtCmDisableDutModeReqSend(_appHandle) {                        \
        CsrBtCmDisableDutModeReq *__msg = (CsrBtCmDisableDutModeReq *) CsrPmemAlloc(sizeof(CsrBtCmDisableDutModeReq)); \
        __msg->type = CSR_BT_CM_DISABLE_DUT_MODE_REQ;                   \
        __msg->appHandle = _appHandle;                                    \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWritePageToReqSend
 *
 *  DESCRIPTION
 *      Writes the value of the page timeout window.
 *      CM sends CSR_BT_CM_WRITE_PAGE_TO_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:      Identity of the calling process
 *        pageTimeout:    the timeout value
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_WRITE_PAGE_TO
#define CsrBtCmWritePageToReqSend(_appHandle,_pageTimeout) {              \
        CsrBtCmWritePageToReq *__msg = (CsrBtCmWritePageToReq*) CsrPmemAlloc(sizeof(CsrBtCmWritePageToReq)); \
        __msg->type = CSR_BT_CM_WRITE_PAGE_TO_REQ;                      \
        __msg->appHandle = _appHandle;                                    \
        __msg->pageTimeout = _pageTimeout;                              \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmWritePageSettingsReqSend
 *
 *  DESCRIPTION
 *      Sets the page scan interval and page scan window timing used in
 *      page scan mode.
 *      CM sends CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:         Identity of the calling process
 *        interval:          the page scan interval
 *        window:            the page scan window
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_WRITE_PAGE_SCAN
#define CsrBtCmWritePageScanSettingsReqSend(_appHandle,_interval,_window) { \
        CsrBtCmWritePagescanSettingsReq *_msg = (CsrBtCmWritePagescanSettingsReq*)CsrPmemAlloc(sizeof(CsrBtCmWritePagescanSettingsReq)); \
        _msg->type = CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_REQ;             \
        _msg->appHandle = _appHandle;                                     \
        _msg->interval = _interval;                                     \
        _msg->window = _window;                                         \
        CsrBtCmMsgTransport( _msg);}
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWritePageScanTypeReqSend
 *
 *  DESCRIPTION
 *      Sets the page scan type mode to use.
 *      CM sends CSR_BT_CM_WRITE_PAGESCAN_TYPE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:         Identity of the calling process
 *        scanType:          normal/interlaced scan type
 *----------------------------------------------------------------------------*/
#define CsrBtCmWritePageScanTypeReqSend(_appHandle,_scanType) {           \
        CsrBtCmWritePagescanTypeReq *_msg = (CsrBtCmWritePagescanTypeReq*)CsrPmemAlloc(sizeof(CsrBtCmWritePagescanTypeReq)); \
        _msg->type = CSR_BT_CM_WRITE_PAGESCAN_TYPE_REQ;                 \
        _msg->appHandle = _appHandle;                                     \
        _msg->scanType = _scanType;                                     \
        CsrBtCmMsgTransport( _msg);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteInquiryScanSettingsReqSend
 *
 *  DESCRIPTION
 *      Sets the inquiry scan interval and inquiry scan window timing.
 *      CM sends CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:         Identity of the calling process
 *        interval:          the inquiry scan interval
 *        window:            the inquiry scan window
 *----------------------------------------------------------------------------*/
void CsrBtCmWriteInquiryScanSettingsReqSend(CsrSchedQid  appHandle,
                                            CsrUint16   interval,
                                            CsrUint16   window);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteInquiryScanTypeReqSend
 *
 *  DESCRIPTION
 *      Sets the inquiry scan type.
 *      CM sends CSR_BT_CM_WRITE_INQUIRYSCAN_TYPE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:         Identity of the calling process
 *        scanType:          normal/interlaced scan type
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_WRITE_INQUIRY_SCAN_TYPE
#define CsrBtCmWriteInquiryScanTypeReqSend(_appHandle,_scanType) {        \
        CsrBtCmWriteInquiryscanTypeReq *_msg = (CsrBtCmWriteInquiryscanTypeReq*)CsrPmemAlloc(sizeof(CsrBtCmWriteInquiryscanTypeReq)); \
        _msg->type = CSR_BT_CM_WRITE_INQUIRYSCAN_TYPE_REQ;              \
        _msg->appHandle = _appHandle;                                     \
        _msg->scanType = _scanType;                                     \
        CsrBtCmMsgTransport( _msg);}
#endif

#ifdef INSTALL_CM_REMOTE_EXT_FEATURES
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadRemoteExtFeaturesReqSend
 *
 *  DESCRIPTION
 *      Reads the extended features of a remote device during a connect procedure.
 *      CM sends CSR_BT_CM_READ_REMOTE_EXT_FEATURES_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:      Identity of the calling process
 *        pageNum:        the requested feature page number
 *        bd_addr:        the device address of the device for which features is
 *                        requested
 *----------------------------------------------------------------------------*/
#define CsrBtCmReadRemoteExtFeaturesReqSend(_appHandle,_pageNum,_bd_addr) { \
        CsrBtCmReadRemoteExtFeaturesReq *__msg = (CsrBtCmReadRemoteExtFeaturesReq*) CsrPmemAlloc(sizeof(CsrBtCmReadRemoteExtFeaturesReq)); \
        __msg->type = CSR_BT_CM_READ_REMOTE_EXT_FEATURES_REQ;           \
        __msg->appHandle = _appHandle;                                    \
        __msg->pageNum = _pageNum;                                      \
        __msg->bd_addr = _bd_addr;                                   \
        CsrBtCmMsgTransport(__msg);}
#endif /* INSTALL_CM_REMOTE_EXT_FEATURES */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadLocalExtFeaturesReqSend
 *
 *  DESCRIPTION
 *      Reads the extended features of a local Bluetooth device.
 *      CM sends CSR_BT_CM_READ_LOCAL_EXT_FEATURES_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:      Identity of the calling process
 *        pageNum:        the requested feature page
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_READ_LOCAL_EXT_FEATURES
#define CsrBtCmReadLocalExtFeaturesReqSend(_appHandle,_pageNum) {         \
        CsrBtCmReadLocalExtFeaturesReq *__msg = (CsrBtCmReadLocalExtFeaturesReq*) CsrPmemAlloc(sizeof(CsrBtCmReadLocalExtFeaturesReq)); \
        __msg->type = CSR_BT_CM_READ_LOCAL_EXT_FEATURES_REQ;            \
        __msg->appHandle = _appHandle;                                    \
        __msg->pageNum = _pageNum;                                      \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetAfhChannelClassReqSend
 *
 *  DESCRIPTION
 *      Specifies a channel classification based on local information held by 
 *      the host.
 *      CM sends CSR_BT_CM_SET_AFH_CHANNEL_CLASS_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:        Identity of the calling process
 *        map:              BR/EDR channel map (NB: array copied, not consumed)
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_AFH
#define CsrBtCmSetAfhChannelClassReqSend(_appHandle,_map) {             \
        CsrBtCmSetAfhChannelClassReq *__msg = (CsrBtCmSetAfhChannelClassReq*) CsrPmemAlloc(sizeof(CsrBtCmSetAfhChannelClassReq)); \
        __msg->type = CSR_BT_CM_SET_AFH_CHANNEL_CLASS_REQ;              \
        __msg->appHandle = _appHandle;                                  \
        SynMemCpyS(__msg->map, 10, _map, 10);                                \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadAfhChannelAssessmentModeReqSend
 *
 *  DESCRIPTION
 *      Reads the status of the controller's channel assessment scheme.
 *      CM sends CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:        Identity of the calling process
 *----------------------------------------------------------------------------*/
#define CsrBtCmReadAfhChannelAssessmentModeReqSend(_appHandle) {                \
        CsrBtCmReadAfhChannelAssessmentModeReq *__msg = (CsrBtCmReadAfhChannelAssessmentModeReq*) CsrPmemAlloc(sizeof(CsrBtCmReadAfhChannelAssessmentModeReq)); \
        __msg->type = CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_REQ;   \
        __msg->appHandle = _appHandle;                                    \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteAfhChannelAssessmentModeReqSend
 *
 *  DESCRIPTION
 *      Activates or deactivates the channel assessment mode.
 *      CM sends CSR_BT_CM_WRITE_AFH_CHANNEL_ASSESSMENT_MODE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:        Identity of the calling process
 *        classMode:        The requested Afh Assessment mode
 *----------------------------------------------------------------------------*/
#define CsrBtCmWriteAfhChannelAssessmentModeReqSend(_appHandle,_classMode) { \
        CsrBtCmWriteAfhChannelAssessmentModeReq *__msg = (CsrBtCmWriteAfhChannelAssessmentModeReq*) CsrPmemAlloc(sizeof(CsrBtCmWriteAfhChannelAssessmentModeReq)); \
        __msg->type = CSR_BT_CM_WRITE_AFH_CHANNEL_ASSESSMENT_MODE_REQ;  \
        __msg->appHandle = _appHandle;                                    \
        __msg->classMode = _classMode;                                  \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadAfhChannelMapReqSend
 *
 *  DESCRIPTION
 *      Reads the channel classification map for the connection specified by
 *      the 'bd_addr'.
 *      CM sends CSR_BT_CM_READ_AFH_CHANNEL_MAP_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:        Identity of the calling process
 *        bd_addr:          Device address that specifies the connection for 
 *                          which to read the classification map
 *----------------------------------------------------------------------------*/
#define CsrBtCmReadAfhChannelMapReqSend(_appHandle,_bd_addr) {                \
        CsrBtCmReadAfhChannelMapReq *__msg = (CsrBtCmReadAfhChannelMapReq*) CsrPmemAlloc(sizeof(CsrBtCmReadAfhChannelMapReq)); \
        __msg->type = CSR_BT_CM_READ_AFH_CHANNEL_MAP_REQ;               \
        __msg->appHandle = _appHandle;                                    \
        __msg->bd_addr = _bd_addr;                                   \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadClockReqSend
 *
 *  DESCRIPTION
 *      Reads the estimate of the Bluetooth clock.
 *      CM sends CSR_BT_CM_READ_CLOCK_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:        Identity of the calling process
 *        whichClock:       Specifies which clock should be read (local or piconet)
 *        bd_addr :         Device address of the other device in the piconet
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_READ_CLOCK
#define CsrBtCmReadClockReqSend(_appHandle,_whichClock,_bd_addr ) {             \
        CsrBtCmReadClockReq *__msg = (CsrBtCmReadClockReq*) CsrPmemAlloc(sizeof(CsrBtCmReadClockReq)); \
        __msg->type = CSR_BT_CM_READ_CLOCK_REQ;                         \
        __msg->appHandle = _appHandle;                                    \
        __msg->bd_addr = _bd_addr ;                                   \
        __msg->whichClock = _whichClock;                                     \
        CsrBtCmMsgTransport(__msg);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadLocalVersionReqSend
 *
 *  DESCRIPTION
 *      Reads the local version information.
 *      CM sends CSR_BT_CM_READ_LOCAL_VERSION_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:        Identity of the calling process
 *----------------------------------------------------------------------------*/
#define CsrBtCmReadLocalVersionReqSend(_appHandle) {                      \
        CsrBtCmReadLocalVersionReq *__msg = (CsrBtCmReadLocalVersionReq*) CsrPmemAlloc(sizeof(CsrBtCmReadLocalVersionReq)); \
        __msg->type = CSR_BT_CM_READ_LOCAL_VERSION_REQ;                 \
        __msg->appHandle = _appHandle;                                    \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmRoleDiscoveryReqSend
 *
 *  DESCRIPTION
 *      Discover the current role (Master or Slave)
 *      CM sends CSR_BT_CM_ROLE_DISCOVERY_CFM back to the application.
 *
 *  PARAMETERS
 *        phandle:             Identity of the calling process
 *        deviceAddr:          BT address
 *----------------------------------------------------------------------------*/
void CsrBtCmRoleDiscoveryReqSend(CsrSchedQid       phandle,
                                 CsrBtDeviceAddr   deviceAddr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteLinkPolicyReqSend
 *
 *  DESCRIPTION
 *      Sets the link policy settings for the device identified by 'deviceAddr'.
 *      CM sends CSR_BT_CM_WRITE_LINK_POLICY_ERROR_IND with appropriate result if the command fails.
 *
 *  PARAMETERS
 *        appHandle:              Identity of the calling process
 *        deviceAddr:             BT address of the device to write link policy
 *        linkPolicySetting       Link policy setting value
 *        setupLinkPolicySetting  Value of the 'linkPolicySetting' parameter is 
 *                                valid if this parameter is set to TRUE
 *        sniffSettings           Sniff interval parameters
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_LINK_POLICY
#define CsrBtCmWriteLinkPolicyReqSend(_appHandle, _deviceAddr, _linkPolicySetting, _setupLinkPolicySetting, _sniffSettings) { \
        CsrBtCmWriteLinkPolicyReq *__msg = (CsrBtCmWriteLinkPolicyReq*) CsrPmemZalloc(sizeof(CsrBtCmWriteLinkPolicyReq)); \
        __msg->type = CM_DM_WRITE_LINK_POLICY_REQ;                      \
        __msg->appHandle = _appHandle;                                  \
        __msg->deviceAddr = _deviceAddr;                                \
        __msg->linkPolicySetting = _linkPolicySetting;                  \
        __msg->setupLinkPolicySetting = _setupLinkPolicySetting;        \
        if (_sniffSettings)                                             \
        {                                                               \
            __msg->sniffSettingsCount = 1;                              \
            __msg->sniffSettings = _sniffSettings;                      \
        }                                                               \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadLinkPolicyReqSend
 *
 *  DESCRIPTION
 *      Reads the policy settings (sniff) allowed.
 *      CM sends CSR_BT_CM_READ_LINK_POLICY_CFM back to the application.
 *
 *  PARAMETERS
 *        apphandle:        Identity of the calling process
 *        deviceAddr:       BT address of the device to read link policy
 *----------------------------------------------------------------------------*/
#define CsrBtCmReadLinkPolicyReqSend(_appHandle, _deviceAddr) {         \
        CsrBtCmReadLinkPolicyReq *__msg = (CsrBtCmReadLinkPolicyReq*) CsrPmemAlloc(sizeof(CsrBtCmReadLinkPolicyReq)); \
        __msg->type = CM_DM_READ_LINK_POLICY_REQ;                       \
        __msg->appHandle = _appHandle;                                  \
        __msg->deviceAddr = _deviceAddr;                                \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCancelReadRemoteNameReqSend
 *
 *  DESCRIPTION
 *      Cancels previously sent read remote name request by the given appHandle.
 *      CM sends CSR_BT_CM_READ_REMOTE_NAME_CFM with status as CSR_BT_RESULT_CODE_CM_CANCELLED
 *      if a previous request is cancelled successfully.
 *
 *  PARAMETERS
 *        apphandle:        Identity of the calling process
 *        deviceAddr:       BT address of the device that is being read
 *----------------------------------------------------------------------------*/
void CsrBtCmCancelReadRemoteNameReqSend(CsrSchedQid      appHandle,
                                        CsrBtDeviceAddr  deviceAddr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmEirUpdateManufacturerDataReqSend
 *
 *  DESCRIPTION
 *      Used for setting a the manufacturer specific data in an Extended Inquiry
 *      Response.
 *      CM sends CSR_BT_CM_EIR_UPDATE_MANUFACTURER_DATA_CFM back to the application.
 *
 *    PARAMETERS
 *      appHandle:                  Identity of the calling process
 *      manufacturerDataSettings:   Settings for handling the manufacturer data.
 *                                  Refer to the documentation for further details.
 *      manufacturerDataLength:     Length of the data in *manufacturerData
 *      manufacturerData:           The actual manufacturer data as it will
 *                                  appear in the EIR.
 *----------------------------------------------------------------------------*/
void CsrBtCmEirUpdateManufacturerDataReqSend(CsrSchedQid appHandle,
                                             CsrUint8        manufacturerDataSettings,
                                             CsrUint8        manufacturerDataLength,
                                             CsrUint8        *manufacturerData);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetEirDataReqSend
 *
 *  DESCRIPTION
 *      Used for setting data in an Extended Inquiry response.
 *      CM sends CSR_BT_CM_SET_EIR_DATA_CFM back to the application.
 *
 *    PARAMETERS
 *      appHandle:                  Identity of the calling process
 *      fec:                        FEC required
 *      length:                     Length of the data
 *      data:                       The actual data in TLV format as it will
 *                                  appear in the EIR.
 *----------------------------------------------------------------------------*/
void CsrBtCmSetEirDataReqSend(CsrSchedQid appHandle,
                              CsrBool fec,
                              CsrUint8 length,
                              CsrUint8 *data);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmEirFlagsReqSend
 *
 *  DESCRIPTION
 *       Update the EIR 'flags' with eg. discoverable mode (general/limited).
 *       CM sends CSR_BT_CM_EIR_FLAGS_CFM back to the application.
 *
 *    PARAMETERS
 *      appHandle:                  Identity of the calling process
 *      eirFlags:                   EIR flags bitmask, see CSR_BT_EIR_FLAG_
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_EIR_FLAGS
#define CsrBtCmEirFlagsReqSend(_appHandle,_eirFlags) {         \
        CsrBtCmEirFlagsReq *msg__ = (CsrBtCmEirFlagsReq*)CsrPmemAlloc(sizeof(CsrBtCmEirFlagsReq)); \
        msg__->type = CSR_BT_CM_EIR_FLAGS_REQ;                          \
        msg__->appHandle = _appHandle;                                   \
        msg__->eirFlags = _eirFlags;                                        \
        CsrBtCmMsgTransport(msg__);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmRoleSwitchConfigReqSend
 *
 *  DESCRIPTION
 *        Specfies the role switch behaviour of CM.
 *        This API doesn't generate any response back to the application.
 *
 *  PARAMETERS
 *        config:    role switch config. See csr_bt_cm_prim.h
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_ROLE_SWITCH_CONFIG
#define CsrBtCmRoleSwitchConfigReqSend(_config) {            \
        CsrBtCmRoleSwitchConfigReq *__msg = (CsrBtCmRoleSwitchConfigReq*) CsrPmemAlloc(sizeof(CsrBtCmRoleSwitchConfigReq)); \
        __msg->type    = CSR_BT_CM_ROLE_SWITCH_CONFIG_REQ;              \
        __msg->config  = _config;                                       \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadFailedContactCounterReqSend
 *
 *  DESCRIPTION
 *      Reads the failed contact counter of a specific device and monitors the 
 *      quality of the link.
 *      CM sends CSR_BT_CM_READ_FAILED_CONTACT_COUNTER_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:    Identity of the calling process
 *        deviceAddr:   BT address of the device for which failed contact 
 *                      counter should be read
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_READ_FAILED_CONTACT_COUNTER
#define CsrBtCmReadFailedContactCounterReqSend(_appHandle, _deviceAddr) {         \
        CsrBtCmReadFailedContactCounterReq *__msg = (CsrBtCmReadFailedContactCounterReq*) CsrPmemAlloc(sizeof(CsrBtCmReadFailedContactCounterReq)); \
        __msg->type        = CSR_BT_CM_READ_FAILED_CONTACT_COUNTER_REQ; \
        __msg->appHandle   = _appHandle;                                  \
        __msg->deviceAddr  = _deviceAddr;                               \
        CsrBtCmMsgTransport(__msg);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetEventMaskReqSend
 *
 *  DESCRIPTION
 *        The API is used for subscribing to extended information (in terms of various events) using
 *        the subscription mask CSR_BT_CM_EVENT_MASK_SUBSCRIBE_X.
 *        CM returns various events based on the subscription done from the application.
 *        E.g. if an application is subscribed to CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SECURITY_EVENT_IND
 *        it will receive CSR_BT_CM_SECURITY_EVENT_IND indication, as and when the event takes place.
 *
 *  PARAMETERS
 *        phandle        :   Identity of the calling process
 *        eventMask      :   Describes which extended information an application
 *                           will subscribe for
 *        conditionMask  :   Filter condition
 *----------------------------------------------------------------------------*/
void CsrBtCmSetEventMaskReqSend(CsrSchedQid phandle, CsrUint32 eventMask, CsrUint32 conditionMask);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSwitchRoleReqSend
 *
 *  DESCRIPTION
 *        Requests the role change for a given ACL connection.
 *        CM sends CSR_BT_CM_SWITCH_ROLE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle     :    Identity of the calling process
 *        deviceAddr    :    BT address of the remote device
 *        role          :    Requested role (master/slave/unknown)
 *        roleType      :    Requested role (only CSR_BT_CM_SWITCH_ROLE_TYPE_ONESHOT supported)
 *        config        :    RFU - shall be zero
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC
void CsrBtCmSwitchRoleReqSend(CsrSchedQid appHandle,
                              CsrBtDeviceAddr deviceAddr,
                              CsrUint8 role,
                              CsrBtCmRoleType roleType,
                              CsrUint32 config);
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSniffModeReqSend
 *
 *  DESCRIPTION
 *        This API is used for altering/entering SNIFF mode on the given ACL
 *        link identified with the deviceAddr.
 *        CM sends CSR_BT_CM_MODE_CHANGE_CFM back to the application.
 *
 *        Note that default low power handling is done from synergy. In order to
 *        use this API successfully, API CsrBtCmModeChangeConfigReqSend
 *        needs to be called with config parameter set to CSR_BT_CM_MODE_CHANGE_ENABLE
 *        to indicate to synergy that for the given link, the low power management 
 *        will now be controlled from application. For more information, refer to
 *        description of API CsrBtCmModeChangeConfigReqSend.
 *
 *  PARAMETERS
 *        phandle            : Identity of the calling process
 *        deviceAddr         : Use to identify the ACL connection which must be
 *                             put in sniff mode
 *        maxInterval        : Specify the acceptable maximum period in sniff mode.
 *                             Note that maxInterval must be greater than
 *                             minInterval and that maxInterval shall be less than
 *                             the Link  Supervision Timeout
 *        minInterval        : Specify the acceptable minimum period in sniff mode
 *        attempt            : Specify the number of baseband received slots
 *                             for sniff attempts
 *        timeout            : Specify the number of baseband received slots for
 *                             sniff timeout
 *        forceSniffSettings : If TRUE and the current mode is Sniff then
 *                             will the CM Exit Sniff mode and enter sniff mode
 *                             again with the given sniff setiings. If FALSE and
 *                             current mode is sniff, it will just stay in sniff
 *                             mode
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_LOW_POWER_CONFIG_PUBLIC

#define CsrBtCmSniffModeReqSend(_phandle, _deviceAddr, _maxInterval, _minInterval, _attempt, _timeout, _forceSniffSettings) {     \
        CsrBtCmModeChangeReq *__msg = (CsrBtCmModeChangeReq *) CsrPmemAlloc(sizeof(CsrBtCmModeChangeReq));  \
        __msg->type                          = CSR_BT_CM_MODE_CHANGE_REQ;                                   \
        __msg->phandle                       = _phandle;                                                    \
        __msg->deviceAddr                    = _deviceAddr;                                                 \
        __msg->mode                          = CSR_BT_SNIFF_MODE;                                           \
        __msg->sniffSettings.attempt         = _attempt;                                                    \
        __msg->sniffSettings.max_interval    = _maxInterval;                                                \
        __msg->sniffSettings.min_interval    = _minInterval;                                                \
        __msg->sniffSettings.timeout         = _timeout;                                                    \
        __msg->forceSniffSettings            = _forceSniffSettings;                                         \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmExitSniffModeReqSend
 *
 *  DESCRIPTION
 *        This API is used to put the ACL link identified by deviceAddr to ACTIVE.
 *        CM sends CSR_BT_CM_MODE_CHANGE_CFM back to the application.
 *
 *        Note that default low power handling is done from synergy. In order to
 *        use this API successfully, API CsrBtCmModeChangeConfigReqSend
 *        needs to be called with config parameter set to CSR_BT_CM_MODE_CHANGE_ENABLE
 *        to indicate to synergy that for the given link, the low power management 
 *        will now be controlled from application. For more information, refer to
 *        description of API CsrBtCmModeChangeConfigReqSend.
 *
 *  PARAMETERS
 *        phandle       : Identity of the calling process
 *        deviceAddr    : Use to identify the ACL connection to exit the sniff mode 
 *                       
 *----------------------------------------------------------------------------*/
#define CsrBtCmExitSniffModeReqSend(_phandle, _deviceAddr) {                    \
    CsrBtCmModeChangeReq *__msg ; \
    __msg=(CsrBtCmModeChangeReq *)CsrPmemAlloc(sizeof(CsrBtCmModeChangeReq));   \
    __msg->type                          = CSR_BT_CM_MODE_CHANGE_REQ;           \
    __msg->phandle                       = _phandle;                            \
    __msg->deviceAddr                    = _deviceAddr;                         \
    __msg->mode                          = CSR_BT_ACTIVE_MODE;                  \
    __msg->sniffSettings.attempt         = 0;                                   \
    __msg->sniffSettings.max_interval    = 0;                                   \
    __msg->sniffSettings.min_interval    = 0;                                   \
    __msg->sniffSettings.timeout         = 0;                                   \
    __msg->forceSniffSettings            = FALSE;                               \
    CsrBtCmMsgTransport(__msg);}
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmModeChangeConfigReqSend
 *
 *  DESCRIPTION
 *        This API is used to configure low power handling. It can either be 
 *        handled by synergy library or the application based on the config
 *        provided. If the config is done for all links it will be applied
 *        for all currently established links and future links. This setting
 *        is preserved till the device reboots after which it is required to 
 *        be set again. If the confg is done for individual links, it will be
 *        applied only for that link and the setting is preserved till the
 *        disconnection of the link after which it is required to be set again.
 *
 *        CM sends CSR_BT_CM_MODE_CHANGE_CONFIG_CFM back to the application.
 *
 *
 *  PARAMETERS
 *        phandle       : Identity of the calling process
 *        deviceAddr    : Use to identify the ACL connection for which low
 *                        power mode handling shall be configured. Note setting
 *                        the deviceAddr to 0 will change the default setting.
 *                        E.g. the config parameter is set to CSR_BT_CM_MODE_CHANGE_ENABLE
 *                        the Application will always have 100% control over the
 *                        low power mode handling
 *
 *        config        : Setting this parameter to CSR_BT_CM_MODE_CHANGE_DISABLE (Default
 *                        Setting) means that Synergy BT is controlling low power modes.
 *                        Setting this parameter to CSR_BT_CM_MODE_CHANGE_ENABLE means that
 *                        the application is controlling low power modes
 *----------------------------------------------------------------------------*/
#define CsrBtCmModeChangeConfigReqSend(_phandle, _deviceAddr, _config) {            \
        CsrBtCmModeChangeConfigReq *__msg = (CsrBtCmModeChangeConfigReq *) CsrPmemAlloc(sizeof(CsrBtCmModeChangeConfigReq)); \
        __msg->type                       = CSR_BT_CM_MODE_CHANGE_CONFIG_REQ; \
        __msg->phandle                    = _phandle;                   \
        __msg->deviceAddr                 = _deviceAddr;                \
        __msg->config                     = _config;                    \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmSniffSubRateReqSend
 *
 *  DESCRIPTION
 *        This message is used to set the sniff subrating for the given remote device.
 *        Sniff interval and latency parameter determine the interval.
 *        Cm sends CM_DM_SNIFF_SUB_RATE_CFM back to application.
 *
 *  PARAMETERS
 *        appHandle         : Identity of the calling process.
 *        deviceAddr        : Remote device address for which the sniff subrate 
 *        maxRemoteLatency  : Maximum latency to calculate the maximum sniff subrate
 *                            that remote device use.
 *        minRemoteTimeout  : Minimum sniff subrate timeout for remote device.
 *        minLocalTimeout   : Minimum sniff subrate timeout for local device.
 *----------------------------------------------------------------------------*/
#define CmDmSniffSubRateReqSend(_appHandle,                                                           \
                                _deviceAddr,                                                          \
                                _maxRemoteLatency,                                                    \
                                _minRemoteTimeout,                                                    \
                                _minLocalTimeout)                                                     \
        do                                                                                            \
        {                                                                                             \
            CmDmSniffSubRateReq *__msg       = (CmDmSniffSubRateReq *) CsrPmemAlloc(sizeof(*__msg));  \
            __msg->type                         = CM_DM_SNIFF_SUB_RATE_REQ;                           \
            __msg->appHandle                    = _appHandle;                                         \
            __msg->deviceAddr                   = _deviceAddr;                                        \
            __msg->ssrSettings.maxRemoteLatency = _maxRemoteLatency;                                  \
            __msg->ssrSettings.minRemoteTimeout = _minRemoteTimeout;                                  \
            __msg->ssrSettings.minLocalTimeout  = _minLocalTimeout;                                   \
            CsrBtCmPutMessageDownstream(__msg);                                                       \
        } while(0)

#endif /*  CSR_BT_INSTALL_CM_LOW_POWER_CONFIG_PUBLIC */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmAlwaysMasterDevicesReqSend
 *
 *  DESCRIPTION
 *        This message can be use to maintain list of remote devices for which
 *        the Device Manager will always try to become master during any ACL
 *        connection creation,  even if there are no existing ACLs connected.
 *        For locally-initiated connection requests to devices in the list,
 *        the Device Manager will prohibit role-switch, thus ensuring that
 *        the local device becomes master. For remotely-initiated connection
 *        requests to devices in the list, the Device Manager will request a
 *        role-switch during connection creation. This may or may not be
 *        accepted by the remote device. Please note that this message should
 *        be used only to work around problems with severely misbehaving remote
 *        devices. Any other use is likely to produce a severely misbehaving
 *        local device and lead to major interoperability problems.
 *        E.g. this primitive should only be used when it is necessary to
 *        become master, even when there are no existing connections, because
 *        the remote device is badly behaved and will not role-switch after
 *        connection creation and it is likely that further ACLs will soon
 *        be connected.
 *        CM sends CSR_BT_CM_ALWAYS_MASTER_DEVICES_CFM back to the application.
 *
 *  PARAMETERS
 *        phandle       : Identity of the calling process
 *        deviceAddr    : The Bluetooth address of the peer device, which
 *                        shall be added to, or delete from, the list
 *
 *        operation     : CSR_BT_CM_ALWAYS_MASTER_DEVICES_CLEAR - CLEAR the entire list
 *                        CSR_BT_CM_ALWAYS_MASTER_DEVICES_ADD - ADD a new device to the list
 *                        CSR_BT_CM_ALWAYS_MASTER_DEVICES_DELETE - DELETE a device from the list
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC
#define CsrBtCmAlwaysMasterDevicesReqSend(_phandle, _deviceAddr, _operation) { \
        CsrBtCmAlwaysMasterDevicesReq *__msg = (CsrBtCmAlwaysMasterDevicesReq *) CsrPmemAlloc(sizeof(CsrBtCmAlwaysMasterDevicesReq)); \
        __msg->type          = CSR_BT_CM_ALWAYS_MASTER_DEVICES_REQ;     \
        __msg->phandle       = _phandle;                                \
        __msg->deviceAddr    = _deviceAddr;                             \
        __msg->operation     = _operation;                              \
        CsrBtCmMsgTransport(__msg);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmPublicRegisterReqSend
 *
 *  DESCRIPTION
 *      This message can be used by an application to make use of a specific 
 *      RFCOMM server channel.
 *      CM sends CSR_BT_CM_REGISTER_CFM back to the application.
 *
 *  PARAMETERS
 *        phandle:            Identity of the calling process
 *        context             Opaque context number returned in CsrBtCmRegisterCfm
 *        serverChannel       Server channel requested. See the behavior below if application sets its value as:
 *                            1. If value is set to CSR_BT_CM_SERVER_CHANNEL_DONT_CARE: Next available server channel is assigned.
 *                            2. If any other value is set, there can be two possibilities based on "optionMask" value:
 *                               1. If CM_REGISTER_OPTION_ALLOW_DYNAMIC_SERVER_CHANNEL is set in optionsMask:
 *                                  Will register the requested server channel number (if not already allocated), or
 *                                  else next available server channel is assigned.
 *                               2. If CM_REGISTER_OPTION_ALLOW_DYNAMIC_SERVER_CHANNEL is not set in optionsMask:
 *                                  Will register the requested server channel number (if not already allocated), or
 *                                  else request is failed and failure confirmation is sent to application.
 *        optionsMask         Bitmask options associated with the rfc registration, options can be set as below:
 *                             1. CM_REGISTER_OPTION_UNUSED:
 *                                No option is set
 *                             2. CM_REGISTER_OPTION_APP_CONNECTION_HANDLING:
 *                                If set, incoming connect request on the assigned server channel would be sent to registered
 *                                application(phandle) to accept/reject the connection through CsrBtCmRfcConnectAcceptRspSend API.
 *                                If not set, CsrBtCmContextConnectAcceptReqSend API can be used to allow Synergy Connection Manager
 *                                to accept any incoming connection from remote on registered server channel and final confirmation of
 *                                connection establishment would be sent to registered application(phandle).
 *                             3. CM_REGISTER_OPTION_ALLOW_DYNAMIC_SERVER_CHANNEL:
 *                                Would be used in conjunction with serverChannel parameter's value.
 *                                See above description of "serverChannel" param for more info.
 *----------------------------------------------------------------------------*/
CsrBtCmRegisterReq *CsrBtCmRegisterReq_struct(CsrSchedQid phandle,
                                              CsrUint16 context,
                                              CsrUint8 serverChannel,
                                              CsrBtCmRegisterOptions optionsMask);

#define CsrBtCmPublicRegisterExtReqSend(_phandle, _context, _serverChannel, _optionsMask) { \
        CsrBtCmRegisterReq *msg__;                                                          \
        msg__=CsrBtCmRegisterReq_struct(_phandle, _context, _serverChannel, _optionsMask);  \
        CsrBtCmPutMessageDownstream( msg__);}

#define CsrBtCmPublicRegisterReqSend(_phandle, _context, _serverChannel)                    \
    CsrBtCmPublicRegisterExtReqSend(_phandle, _context, _serverChannel, CM_REGISTER_OPTION_UNUSED)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmUnRegisterReqSend
 *
 *  DESCRIPTION
 *      This message can be used by an application to release a registered 
 *      server channel that it no longer requires.
 *      This API doesn't generate any response back to the application.
 *
 *  PARAMETERS
 *        serverChannel:        local server channel to unregister
 *----------------------------------------------------------------------------*/
void CsrBtCmUnRegisterReqSend(CsrUint8    serverChannel);

#ifdef CSR_BT_LE_ENABLE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadAdvertisingChTxPowerReqSend
 *
 *  DESCRIPTION
 *      Read Tx power for low energy advertising channel.
 *      CM sends CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle :         Identity of the calling process
 *        context             Opaque context number returned in CsrBtCmReadAdvertisingChTxPowerCfm
 *----------------------------------------------------------------------------*/
#define CsrBtCmReadAdvertisingChTxPowerReqSend(_appHandle,_context) {                \
        CsrBtCmReadAdvertisingChTxPowerReq *msg__ = CsrPmemAlloc(sizeof(CsrBtCmReadAdvertisingChTxPowerReq)); \
        msg__->type = CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_REQ;       \
        msg__->appHandle = _appHandle;                                  \
        msg__->context = _context;                                      \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeReceiverTestReqSend
 *
 *  DESCRIPTION
 *      Send LE receiver test command.
 *      CM sends CSR_BT_CM_LE_RECEIVER_TEST_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:         Identity of the calling process
 *        rxFrequency:       Rx frequency to perform test on
 *----------------------------------------------------------------------------*/
#define CsrBtCmLeReceiverTestReqSend(_appHandle,_rxFrequency) {         \
        CsrBtCmLeReceiverTestReq *msg__ = CsrPmemAlloc(sizeof(CsrBtCmLeReceiverTestReq)); \
        msg__->type = CSR_BT_CM_LE_RECEIVER_TEST_REQ;                   \
        msg__->appHandle = _appHandle;                                  \
        msg__->rxFrequency = _rxFrequency;                              \
        CsrBtCmPutMessageDownstream( msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeTransmitterTestReqSend
 *
 *  DESCRIPTION
 *      Send LE transmitter test command.
 *      CM sends CSR_BT_CM_LE_TRANSMITTER_TEST_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:         Identity of the calling process
 *        txFrequency:       Tx frequency to perform test on
 *        lengthOfTestData   Size of the packets to transmit
 *        packetPayload      Payload pattern
 *----------------------------------------------------------------------------*/
#define CsrBtCmLeTransmitterTestReqSend(_appHandle,_txFrequency,_lengthOfTestData,_packetPayload) { \
        CsrBtCmLeTransmitterTestReq *msg__ = CsrPmemAlloc(sizeof(CsrBtCmLeTransmitterTestReq)); \
        msg__->type = CSR_BT_CM_LE_TRANSMITTER_TEST_REQ;                \
        msg__->appHandle = _appHandle;                                  \
        msg__->txFrequency = _txFrequency;                              \
        msg__->lengthOfTestData = _lengthOfTestData;                    \
        msg__->packetPayload = _packetPayload;                          \
        CsrBtCmPutMessageDownstream( msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeTestEndReqSend
 *
 *  DESCRIPTION
 *      Send LE test end command.
 *      CM sends CSR_BT_CM_LE_TEST_END_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:          Identity of the calling process
 *----------------------------------------------------------------------------*/
#define CsrBtCmLeTestEndReqSend(_appHandle) {           \
        CsrBtCmLeTestEndReq *msg__ = CsrPmemAlloc(sizeof(CsrBtCmLeTestEndReq)); \
        msg__->type = CSR_BT_CM_LE_TEST_END_REQ;                        \
        msg__->appHandle = _appHandle;                                  \
        CsrBtCmPutMessageDownstream( msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmLeEnhancedReceiverTestReqSend
 *
 *  DESCRIPTION
 *      Send LE receiver test command.
 *      CM sends CM_LE_ENHANCED_RECEIVER_TEST_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:         Identity of the calling process
 *        rxFrequency:       Rx frequency to perform test on
 *        phy:               PHY channel to be used by the receiver
 *                           0x01 - LE 1M PHY,
 *                           0x02 - LE 2M PHY,
 *                           0x03 - LE Coded PHY 
 *        modIndex:          Assume transmitter will have 
 *                           0x00 - Standard modulation index
 *                           0x01 - Stable modulation index
 *----------------------------------------------------------------------------*/
#define CmLeEnhancedReceiverTestReqSend(_appHandle,_rxFrequency,_phy,_modIndex) {         \
        CmLeEnhancedReceiverTestReq *msg__ = CsrPmemAlloc(sizeof(CmLeEnhancedReceiverTestReq)); \
        msg__->type = CM_LE_ENHANCED_RECEIVER_TEST_REQ;                   \
        msg__->appHandle = _appHandle;                                  \
        msg__->rxFrequency = _rxFrequency;                              \
        msg__->phy = _phy;                                              \
        msg__->modIndex = _modIndex;                                    \
        CsrBtCmPutMessageDownstream( msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmLeEnhancedTransmitterTestReqSend
 *
 *  DESCRIPTION
 *      Send LE transmitter test command.
 *      CM sends CM_LE_ENHANCED_TRANSMITTER_TEST_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:         Identity of the calling process
 *        txFrequency:       Tx frequency to perform test on
 *        lengthOfTestData:  Size of the packets to transmit
 *        packetPayload:     Payload pattern
 *        phy:               PHY channel to be used by the transmitter
 *                           0x01 - LE 1M PHY,
 *                           0x02 - LE 2M PHY,
 *                           0x03 - LE Coded PHY with S=8 data coding,
 *                           0x04 - LE Coded PHY with S=2 data coding
 *----------------------------------------------------------------------------*/
#define CmLeEnhancedTransmitterTestReqSend(_appHandle,_txFrequency,_lengthOfTestData,_packetPayload,_phy) { \
        CmLeEnhancedTransmitterTestReq *msg__ = CsrPmemAlloc(sizeof(CmLeEnhancedTransmitterTestReq)); \
        msg__->type = CM_LE_ENHANCED_TRANSMITTER_TEST_REQ;                \
        msg__->appHandle = _appHandle;                                  \
        msg__->txFrequency = _txFrequency;                              \
        msg__->lengthOfTestData = _lengthOfTestData;                    \
        msg__->packetPayload = _packetPayload;                          \
        msg__->phy = phy;                                               \
        CsrBtCmPutMessageDownstream( msg__);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeSetOwnAddressTypeReqSend
 *
 *  DESCRIPTION
 *      Sets the value of own address type to be used for LE GAP procedures.
 *      If application does not set own address type, default public address
 *      type shall be used as own address type for LE GAP procedures.
 *      CM sends CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle       :  Identity of the calling process
 *      ownAddressType  :  Own address type to be set. Below values are allowed.
 *                         0x00 : Public address
 *                         0x01 : Random address
 *                         0x02 : Controller generated RPA, use public address
 *                                if controller can't generate RPA.
 *                         0x03 : Controller generated RPA, use random address
 *                                from LE_Set_Random_Address if controller can't
 *                                generate RPA.
 * Note : 1. Setting own address type's value to 0x02 and 0x03 is allowed only
 *           if controller supports LL_PRIVACY.
 *        2. If CMake configuration flag "CSR_BT_LE_RANDOM_ADDRESS_TYPE" set as
 *           "STATIC", changing own address type is not allowed.
 *----------------------------------------------------------------------------*/
CsrBtCmLeSetOwnAddressTypeReq *CsrBtCmLeSetOwnAddressTypeReq_struct(CsrSchedQid appHandle,
                                                                    CsrUint8 ownAddressType);

#define CsrBtCmLeSetOwnAddressTypeReqSend(_appHandle,           \
                                          _ownAddressType)      \
    do                                                          \
    {                                                           \
        CsrBtCmLeSetOwnAddressTypeReq *msg__;                   \
        msg__ = CsrBtCmLeSetOwnAddressTypeReq_struct(_appHandle, _ownAddressType); \
        CsrBtCmPutMessageDownstream(msg__);                     \
    } while(0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeSetPvtAddrTimeoutReqSend
 *
 *  DESCRIPTION
 *      Sets the value of Resolvable/Non-resolvable Private Address timeout.
 *      Length of the time Synergy uses a RPA/NRPA before a new RPA/NRPA is
 *      generated & starts being used.
 *      CM sends CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle       :  Identity of the calling process
 *      timeout         :  Sets the value of private address time out.
 *                         Range : 0x0001(1 sec) - 0xA1B8(~11.5 hours)
 *----------------------------------------------------------------------------*/
CsrBtCmLeSetPvtAddrTimeoutReq *CsrBtCmLeSetPvtAddrTimeoutReq_struct(CsrSchedQid appHandle,
                                                                    CsrUint16 timeout);

#define CsrBtCmLeSetPvtAddrTimeoutReqSend(_appHandle,               \
                                          _timeOut)                 \
    do                                                              \
    {                                                               \
        CsrBtCmLeSetPvtAddrTimeoutReq *msg__;                       \
        msg__ = CsrBtCmLeSetPvtAddrTimeoutReq_struct(_appHandle, _timeOut); \
        CsrBtCmPutMessageDownstream(msg__);                         \
    } while(0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeSetStaticAddressReqSend
 *
 *  DESCRIPTION
 *      Configures local static random address for current power cycle. This
 *      API configures application's provided static address as local random
 *      address for current power cycle only if CMake configuration flag
 *      "CSR_BT_LE_RANDOM_ADDRESS_TYPE" set as "STATIC".
 *      CM sends CSR_BT_CM_LE_SET_STATIC_ADDRESS_CFM back to the application.
 *
 *  Note: Static address shall be configured only once for current power cycle.
 *
 *  PARAMETERS
 *      appHandle       :  Identity of the calling process
 *      staticAddress   :  Static address to be used as local device random
 *                         address.
 *----------------------------------------------------------------------------*/
CsrBtCmLeSetStaticAddressReq *CsrBtCmLeSetStaticAddressReq_struct(CsrSchedQid appHandle,
                                                                  CsrBtDeviceAddr staticAddress);
#define CsrBtCmLeSetStaticAddressReqSend(_appHandle,                             \
                                         _staticAddress)                 \
    do                                                                   \
    {                                                                    \
        CsrBtCmLeSetStaticAddressReq *msg__;                             \
        msg__ = CsrBtCmLeSetStaticAddressReq_struct(_appHandle, _staticAddress); \
        CsrBtCmPutMessageDownstream(msg__);                              \
    } while(0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeReadRandomAddressReqSend
 *
 *  DESCRIPTION
 *      Read local or peer device's random address for trusted devices. This API
 *      expects trusted peer device identity address(IA) and provides local or
 *      peer current available random address based on the value set for "flag".
 *      If given peer IA fields are all zeros, "flag" value is ignored and local
 *      random address is returned. Since local RPAs can be different for 
 *      multiple LE connections(multiple peer devices) so it is mendatory for
 *      application to provide peer's IA to retrieve actual local or peer RPA
 *      used for existing LE connection.
 *
 *      This API expects peer device's IA from subsequent connection with
 *      trusted devices. If it is called at new connection, application has to
 *      pass peer device's connected address as peer's "idAddress" even after
 *      device gets bonded in the same connection.
 *      CM sends CSR_BT_CM_LE_READ_RANDOM_ADDRESS_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle       :  Identity of the calling process
 *      idAddress       :  Identity address of the connected peer device.
 *      flag            :  Retrieve Random address either for local or peer
 *                         device. The allowed values are as below:
 *                         CSR_BT_DEVICE_FLAG_LOCAL: for local device
 *                         CSR_BT_DEVICE_FLAG_PEER: for peer device
 *----------------------------------------------------------------------------*/
CsrBtCmLeReadRandomAddressReq *CsrBtCmLeReadRandomAddressReq_struct(CsrSchedQid appHandle,
                                                                    CsrBtTypedAddr idAddress,
                                                                    CsrBtDeviceFlag flag);
#define CsrBtCmLeReadRandomAddressReqSend(_appHandle,                   \
                                          _idAddress,                   \
                                          _flag)                        \
    do                                                                  \
    {                                                                   \
        CsrBtCmLeReadRandomAddressReq *msg__;                           \
        msg__ = CsrBtCmLeReadRandomAddressReq_struct(_appHandle, _idAddress, _flag); \
        CsrBtCmPutMessageDownstream(msg__);                             \
    } while(0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeSirkOperationReqSend
 *
 *  DESCRIPTION
 *      SIRK operation request
 *      CM sends CSR_BT_CM_LE_SIRK_OPERATION_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle       :  Identity of the calling process
 *      tpAddrt         :  Peer device address
 *      flags           :  Reserved for future use
 *      sirkKey         :  Key to be encrypted or decrypted
 *----------------------------------------------------------------------------*/
#define CsrBtCmLeSirkOperationReqSend(_appHandle, _tpAddrt, _flags, _sirkKey) {  \
         CsrBtCmLeSirkOperationReq *msg__ = CsrPmemAlloc(sizeof(CsrBtCmLeSirkOperationReq)); \
         msg__->type = CSR_BT_CM_LE_SIRK_OPERATION_REQ; \
         msg__->appHandle = _appHandle;                 \
         msg__->tpAddrt = _tpAddrt;                     \
         msg__->flags = _flags;                         \
         SynMemCpyS(msg__->sirkKey, 16, _sirkKey, 16);  \
         CsrBtCmPutMessageDownstream( msg__);}
 
 /*----------------------------------------------------------------------------*
  *  NAME
  *      CsrBtCmLeSetDataRelatedAddressChangesReqSend
  *
  *  DESCRIPTION
  *      Set data related address changes request.Used to control if 
  *      advertiser's RPA changes when advertising data or scan response
  *      data is changed. This is configurable on a per advertising set bases.
  *      CM sends CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_CFM back to the application.
  *
  *  PARAMETERS
  *      appHandle:     Identity of the calling process
  *      advHandle:     Advertsing set
  *      flags:         Reserved always set to 0
  *      changeReasons: Specifies if the advertiser's address will change when 
  *                     using controller based RPA generation and data changes
  *----------------------------------------------------------------------------*/
#define CsrBtCmLeSetDataRelatedAddressChangesReqSend(_appHandle, _advHandle, _flags, _changeReasons) {  \
          CsrBtCmLeSetDataRelatedAddressChangesReq *msg__ = CsrPmemAlloc(sizeof(CsrBtCmLeSetDataRelatedAddressChangesReq)); \
          msg__->type = CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_REQ; \
          msg__->appHandle = _appHandle;                 \
          msg__->advHandle = _advHandle;                 \
          msg__->flags = _flags;                         \
          msg__->changeReasons = _changeReasons;         \
          CsrBtCmPutMessageDownstream( msg__);}

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetDefaultSubrateReqSend
 *
 *  DESCRIPTION
 *     Default subrate request primitive is used to set the acceptable
 *     parameters for the subrate requests sent from Peripheral for all future
 *     LE ACL connection where local device is Central.
 *
 *     Note that this primitive does not affect the existing connections.
 *
 *  PARAMETERS
 *      appHandle           :  Identity of the calling process
 *      subrate_min         :  Minimum subrate factor allowed in requests by
 *                             a Peripheral
 *      subrate_max         :  Maximum subrate factor allowed in requests by
 *                             a Peripheral
 *      max_latency         :  Maximum Peripheral latency allowed in requests
 *                             by a Peripheral
 *      continuation_num    :  Minimum number of underlying connection events
 *                             to remain active
 *      supervision_timeout :  Maximum supervision timeout allowed in requests
 *                             by a Peripheral
 *----------------------------------------------------------------------------*/
#define CsrBtCmLeSetDefaultSubrateReqSend(_appHandle, _subrate_min, _subrate_max, _max_latency, _continuation_num, _supervision_timeout) {  \
         CsrBtCmLeSetDefaultSubrateReq *msg__ = CsrPmemAlloc(sizeof(CsrBtCmLeSetDefaultSubrateReq)); \
         msg__->type = CSR_BT_CM_LE_SET_DEFAULT_SUBRATE_REQ;   \
         msg__->appHandle = _appHandle;                     \
         msg__->subrate_min = _subrate_min;                 \
         msg__->subrate_max = _subrate_max;                 \
         msg__->max_latency = _max_latency;                 \
         msg__->continuation_num = _continuation_num;       \
         msg__->supervision_timeout = _supervision_timeout; \
         CsrBtCmPutMessageDownstream( msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSubrateChangeReqSend
 *
 *  DESCRIPTION
 *     Subrate Change request primitive is used by Central or Peripheral 
 *     to change the subrate factor or other connection parameters to an 
 *     existing connection.
 *
 *     CSR_BT_CM_SUBRATE_CHANGE_IND will be sent once the subrate update is 
 *     completed with the updated parameters.
 *
 *  PARAMETERS
 *      appHandle           :  Identity of the calling process
 *      tpAddrt             :  Peer device address
 *      subrate_min         :  Minimum subrate factor allowed in requests by
 *                             a Peripheral
 *      subrate_max         :  Maximum subrate factor allowed in requests by
 *                             a Peripheral
 *      max_latency         :  Maximum Peripheral latency allowed in requests
 *                             by a Peripheral
 *      continuation_num    :  Minimum number of underlying connection events
 *                             to remain active
 *      supervision_timeout :  Maximum supervision timeout allowed in requests
 *                             by a Peripheral
 *----------------------------------------------------------------------------*/
#define CsrBtCmLeSubrateChangeReqSend(_appHandle, _tpAddrt, _subrate_min, _subrate_max, _max_latency, _continuation_num, _supervision_timeout) {  \
         CsrBtCmLeSubrateChangeReq *msg__ = CsrPmemAlloc(sizeof(CsrBtCmLeSubrateChangeReq)); \
         msg__->type = CSR_BT_CM_LE_SUBRATE_CHANGE_REQ;   \
         msg__->appHandle = _appHandle;                     \
         msg__->address = _tpAddrt;                         \
         msg__->subrate_min = _subrate_min;                 \
         msg__->subrate_max = _subrate_max;                 \
         msg__->max_latency = _max_latency;                 \
         msg__->continuation_num = _continuation_num;       \
         msg__->supervision_timeout = _supervision_timeout; \
         CsrBtCmPutMessageDownstream( msg__);}

#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE */
#endif /* End of CSR_BT_LE_ENABLE */


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmDmPowerSettingsReqSend
 *
 *  DESCRIPTION
 *      Sets a link policy power table for the specified 'addr'.
 *
 *  PARAMETERS
 *      addr             : Address for which power table needs to be set.
 *      powerTableSize   : The number of entries in the power table
 *      powerTable       : The power table
 *----------------------------------------------------------------------------*/
CsrBtCmDmPowerSettingsReq *CsrBtCmDmPowerSettingsReq_struct(CsrBtDeviceAddr addr,
                                                            CsrUint8 powerTableSize,
                                                            CsrBtCmPowerSetting *powerTable);

#define CsrBtCmDmPowerSettingsReqSend(_addr, _powerTableSize, _powerTable)      \
    do                                                                          \
    {                                                                           \
        CsrBtCmDmPowerSettingsReq *msg_;                                        \
        msg_ = CsrBtCmDmPowerSettingsReq_struct(_addr,                          \
                                                _powerTableSize,                \
                                                _powerTable);                   \
        CsrBtCmPutMessageDownstream(msg_);                                      \
    } while (0)


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmFreeUpstreamMessageContents
 *
 *  DESCRIPTION
 *      During Bluetooth shutdown all allocated payload in the Synergy BT CM
 *      message must be deallocated. This is done by this function
 *
 *
 *    PARAMETERS
 *      eventClass :  Must be CSR_BT_CM_PRIM,
 *      message:      The message received from Synergy BT CM
 *----------------------------------------------------------------------------*/
void CsrBtCmFreeUpstreamMessageContents(CsrUint16 eventClass, void * message);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmAclOpenReqSend
 *
 *  DESCRIPTION
 *      Request to establish ACL connection with a remote device.
 *      CM sends CSR_BT_CM_ACL_OPEN_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      deviceAddr:          BT address of the device to connect to
 *      flags:               0 - bredr, DM_ACL_FLAG_ULP - LE
 *
 *  UPSTREAM
 *     CSR_BT_CM_ACL_OPEN_CFM
 *----------------------------------------------------------------------------*/
void CsrBtCmAclOpenReqSend(CsrSchedQid appHandle,
                           CsrBtTypedAddr deviceAddr,
                           CsrUint16 flags);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeAclOpenReqSend
 *
 *  DESCRIPTION
 *      Request to establish LE ACL connection with a remote device.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      deviceAddr:          BT address of the device to connect to
 *
 *  UPSTREAM
 *     CSR_BT_CM_ACL_OPEN_CFM
 *----------------------------------------------------------------------------*/
void CsrBtCmLeAclOpenReqSend(CsrSchedQid appHandle,
                             CsrBtTypedAddr deviceAddr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmLeAclOpenUseFilterAcceptListReqSend
 *
 *  DESCRIPTION
 *      Request to establish LE ACL connection with remote device,
 *      present in Filter Accept List(previously called as white list). 
 *      Ensure the remote device is added to Filter Accept List using 
 *      CmLeAddDeviceToWhiteListReqSend(..) before using this API.
 *      By default conn_attempt_timeout is set to CSR_BT_LE_DEFAULT_CONN_ATTEMPT_TIMEOUT.
 *      The timeout value can be modified in csr_bt_usr_config.h if required.
 *
 *      Note: If there are multiple devices in Filter Accept List,
 *            connection will be attempted with one of the advertising devices present in the list.
 *            If there are no devices in Filter Accept List, connection will fail after timeout.
 *
 *  PARAMETERS
 *      appHandle: Identity of the calling process
 *
 *  UPSTREAM
 *     CSR_BT_CM_ACL_OPEN_CFM
 *----------------------------------------------------------------------------*/
void CmLeAclOpenUseFilterAcceptListReqSend(CsrSchedQid appHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmAclCloseReqSend
 *
 *  DESCRIPTION
 *      Request to disconnect a remote device.
 *      CM sends CSR_BT_CM_ACL_CLOSE_CFM back to the application.
 *      As closing of ACL closes all the other connections which are using it,
 *      the application may receive one or more disconnect indications for different
 *      services like RFCOMM, L2CAP etc.
 *      For BREDR ACL connections, CSR_BT_CM_ACL_DISCONNECT_IND event will be received
 *      if the application has subscribed for it.
 *      For LE ACL connections, CSR_BT_CM_LE_EVENT_CONNECTION_IND will be received if
 *      application has subscribed for it.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      deviceAddr:          BT address of the device to disconnect to
 *      flags:               Normal or force
 *      reason:              Reason to initiate disconnection
 *----------------------------------------------------------------------------*/
void CsrBtCmAclCloseReqSend(CsrSchedQid appHandle,
                            CsrBtTypedAddr deviceAddr,
                            CsrUint16 flags,
                            CsrUint8 reason);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeScanReqSend
 *      CsrBtCmLeScanReqOffSend
 *      CsrBtCmLeScanReqCoexOnSend
 *      CsrBtCmLeScanReqCoexOffSend
 *
 *  DESCRIPTION
 *      Toggle low energy scanning and parameter setup.
 *      CM sends CSR_BT_CM_LE_SCAN_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:            Identity of the calling process
 *      context:              Application context
 *      mode:                 Scan mode
 *      scanType:             Scan type
 *      scanInterval:         Scan interval (in slots)
 *      scanWindow:           Scan window
 *      scanningFilterPolicy: Scanning filter policy
 *      filterDuplicates:     Indicates whether duplicate advertisement should be filtered or not.
 *      addressCount:         Number of addresses present in the address list
 *      addressList:          Pointer to address list which application is looking for through this scan call.
 *----------------------------------------------------------------------------*/
void CsrBtCmLeScanReqSend(CsrSchedQid    appHandle,
                          CsrUint16      context,
                          CsrUint8       mode,
                          CsrUint8       scanType,
                          CsrUint16      scanInterval,
                          CsrUint16      scanWindow,
                          CsrUint8       scanningFilterPolicy,
                          CsrUint8       filterDuplicates,
                          CsrUint16      addressCount,
                          CsrBtTypedAddr *addressList);


void CsrBtCmLeScanReqOffSend(CsrSchedQid    appHandle,
                             CsrUint16      context) ;

void CsrBtCmLeScanReqCoexOnSend(CsrSchedQid    appHandle,
                                CsrUint16      scanInterval,
                                CsrUint16      scanWindow);

void CsrBtCmLeScanReqCoexOffSend(CsrSchedQid    appHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeAdvertiseReqStartSend
 *      CsrBtCmLeAdvertiseReqStopSend
 *      CsrBtCmLeAdvertiseReqCoexOnSend
 *      CsrBtCmLeAdvertiseReqCoexOffSend
 *
 *  DESCRIPTION
 *      Toggle low energy advertising and parameter setup.
 *      CM sends CSR_BT_CM_LE_ADVERTISE_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:               Identity of the calling process
 *      context:                 Application context
 *      mode:                    Advertising mode
 *      paramChange:             Indicates which data from the provided 
 *                               information should be updated.
 *      advertisingDataLength:   Length of the advertising data
 *      advertisingData:         Pointer to advertising data
 *      scanResponseDataLength:  Length of scan response data
 *      scanResponseData:        Pointer to scan response data
 *      advIntervalMin:          Minimum advertising interval
 *      advIntervalMax:          Maximum advertising interval
 *      advertisingType:         Type of advertising
 *      advertisingChannelMap:   Advertising channel map
 *      advertisingFilterPolicy: Advertising filter policy
 *      whitelistAddrCount:      Number of addresses in the whitelist
 *      whitelistAddrList:       Pointer to whitelist address list
 *----------------------------------------------------------------------------*/
void CsrBtCmLeAdvertiseReqStartSend(CsrSchedQid     appHandle,
                                    CsrUint16       context,
                                    CsrUint8        mode,
                                    CsrUint32        paramChange,
                                    CsrUint8        advertisingDataLength,
                                    CsrUint8        *advertisingData,
                                    CsrUint8        scanResponseDataLength,
                                    CsrUint8        *scanResponseData,
                                    CsrUint16       advIntervalMin,
                                    CsrUint16       advIntervalMax,
                                    CsrUint8        advertisingType,
                                    CsrUint8        advertisingChannelMap,
                                    CsrUint8        advertisingFilterPolicy,
                                    CsrUint16       whitelistAddrCount,
                                    CsrBtTypedAddr *whitelistAddrList);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeAdvertiseReqStartSend
 *      CsrBtCmLeAdvertiseReqStopSend
 *      CsrBtCmLeAdvertiseReqCoexOnSend
 *      CsrBtCmLeAdvertiseReqCoexOffSend
 *
 *  DESCRIPTION
 *      Toggle low energy advertising and parameter setup.
 *      CM sends CSR_BT_CM_LE_ADVERTISE_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:               Identity of the calling process
 *      context:                 Application context
 *      mode:                    Advertising mode
 *      paramChange:             Indicates which data from the provided 
 *                               information should be updated.
 *      advertisingDataLength:   Length of the advertising data
 *      advertisingData:         Pointer to advertising data
 *      scanResponseDataLength:  Length of scan response data
 *      scanResponseData:        Pointer to scan response data
 *      advIntervalMin:          Minimum advertising interval
 *      advIntervalMax:          Maximum advertising interval
 *      advertisingType:         Type of advertising
 *      advertisingChannelMap:   Advertising channel map
 *      advertisingFilterPolicy: Advertising filter policy
 *      whitelistAddrCount:      Number of addresses in the whitelist
 *      whitelistAddrList:       Pointer to whitelist address list
 *      address:                 Bluetooth device address
 *----------------------------------------------------------------------------*/

void CsrBtCmLeStartAdvertisement(CsrSchedQid     appHandle,
                                 CsrUint16       context,
                                 CsrUint8        mode,
                                 CsrUint32        paramChange,
                                 CsrUint8        advertisingDataLength,
                                 CsrUint8        *advertisingData,
                                 CsrUint8        scanResponseDataLength,
                                 CsrUint8        *scanResponseData,
                                 CsrUint16       advIntervalMin,
                                 CsrUint16       advIntervalMax,
                                 CsrUint8        advertisingType,
                                 CsrUint8        advertisingChannelMap,
                                 CsrUint8        advertisingFilterPolicy,
                                 CsrUint16       whitelistAddrCount,
                                 CsrBtTypedAddr *whitelistAddrList,
                                 CsrBtTypedAddr  address);

void CsrBtCmLeAdvertiseReqStopSend(CsrSchedQid appHandle,
                                   CsrUint16 context);

void CsrBtCmLeAdvertiseReqCoexOnSend(CsrUint8 advertisingType);

void CsrBtCmLeAdvertiseReqCoexOffSend(void);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeConnparamReq
 *
 *  DESCRIPTION
 *      Set default LE connection parameters.
 *      CM sends CSR_BT_CM_LE_CONNPARAM_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:             Identity of the calling process
 *      scanInterval:          Time interval from when Controller started its
 *                             last LE scan until it begins subsequent LE scan
 *      scanWindow:            Amount of time for the duration of the LE scan
 *      connIntervalMin:       Minimum value for the connection event interval
 *      connIntervalMax:       Maximum value for the connection event interval
 *      connLatency:           Slave latency for the connection in number of 
 *                             connection events
 *      supervisionTimeout:    Supervision timeout for the LE Link
 *      connLatencyMax:        Maximum allowed slave latency that is accepted if
 *                             slave requests connection parameter update once 
 *                             connected.
 *      supervisionTimeoutMin: Minimum allowed supervision timeout that is 
 *                             accepted if slave requests connection parameter
 *                             update once connected
 *      supervisionTimeoutMax: Maximum allowed supervision timeout that is 
 *                             accepted if slave requests connection parameter
 *                             update once connected
 *----------------------------------------------------------------------------*/
CsrBtCmLeConnparamReq *CsrBtCmLeConnparamReq_struct(CsrSchedQid appHandle,
                                                    CsrUint16 scanInterval,
                                                    CsrUint16 scanWindow,
                                                    CsrUint16 connIntervalMin,
                                                    CsrUint16 connIntervalMax,
                                                    CsrUint16 connLatency,
                                                    CsrUint16 supervisionTimeout,
                                                    CsrUint16 connLatencyMax,
                                                    CsrUint16 supervisionTimeoutMin,
                                                    CsrUint16 supervisionTimeoutMax);
#define CsrBtCmLeConnparamReqSend(_app,_si,_sw,_cii,_cia,_cl,_st,_cla,_sti,_sta) { \
        CsrBtCmLeConnparamReq *msg__; \
        msg__=CsrBtCmLeConnparamReq_struct(_app,_si,_sw,_cii,_cia,_cl,_st,_cla,_sti,_sta); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeConnparamUpdateReq
 *
 *  DESCRIPTION
 *      Update LE connection parameters for existing connection.
 *      CM sends CSR_BT_CM_LE_CONNPARAM_UPDATE_CMP_IND signal back to the application.
 *
 *  PARAMETERS
 *      appHandle:               Identity of the calling process
 *      address:                 Address of the master.
 *      connIntervalMin:         Minimum requested connection interval.
 *      connIntervalMax:         Maximum requested connection interval.
 *      connLatency:             Slave latency.
 *      supervisionTimeout:      Supervision timeout.
 *      minimumCeLength:         Minimum length of connection.
 *      maximumCeLength:         Maximum length of connection.
 *----------------------------------------------------------------------------*/
CsrBtCmLeConnparamUpdateReq *CsrBtCmLeConnparamUpdateReq_struct(CsrSchedQid appHandle,
                                                                CsrBtTypedAddr address,
                                                                CsrUint16 connIntervalMin,
                                                                CsrUint16 connIntervalMax,
                                                                CsrUint16 connLatency,
                                                                CsrUint16 supervisionTimeout,
                                                                CsrUint16 minimumCeLength,
                                                                CsrUint16 maximumCeLength);

#define CsrBtCmLeConnparamUpdateReqSend(_appHandle,_address,_connIntervalMin,_connIntervalMax,_connLatency,_supervisionTimeout,_minimumCeLength,_maximumCeLength) { \
        CsrBtCmLeConnparamUpdateReq *msg__;                             \
        msg__=CsrBtCmLeConnparamUpdateReq_struct(_appHandle,_address,_connIntervalMin,_connIntervalMax,_connLatency,_supervisionTimeout,_minimumCeLength,_maximumCeLength); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeAcceptConnparamUpdateRes
 *
 *  DESCRIPTION
 *      Accept or reject LE Slave Request to Update LE connection parameters 
 *      for existing connection.
 *      This API doesn't generate any response back to the application.
 *
 *  PARAMETERS
 *      address:                 Address of the master.
 *      l2caSignalId:            L2CAP Signal ID
 *      connIntervalMin:         Minimum requested connection interval as number
 *                               of 1.25 ms.
 *      connIntervalMax:         Maximum requested connection interval as number
 *                               of 1.25 ms 
 *      connLatency:             Connection slave latency.
 *      supervisionTimeout:      Link timeout value as number of 10 ms
 *      accept:                  Accept or reject LE Slave Request
 *----------------------------------------------------------------------------*/
CsrBtCmLeAcceptConnparamUpdateRes *CsrBtCmLeAcceptConnparamUpdateRes_struct(CsrBtTypedAddr      address, 
                                                                            l2ca_identifier_t   l2caSignalId, 
                                                                            CsrUint16           connIntervalMin, 
                                                                            CsrUint16           connIntervalMax, 
                                                                            CsrUint16           connLatency,
                                                                            CsrUint16           supervisionTimeout,
                                                                            CsrBool             accept);

#define CsrBtCmLeAcceptConnparamUpdateResSend(_ad,_id_,_cii,_cia,_cl,_st,_ac) { \
        CsrBtCmLeAcceptConnparamUpdateRes *msg__; \
        msg__=CsrBtCmLeAcceptConnparamUpdateRes_struct(_ad,_id_,_cii,_cia,_cl,_st,_ac); \
        CsrBtCmPutMessageDownstream(msg__);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeWhitelistSetReqSend
 *
 *  DESCRIPTION
 *      Set whitelist with the provided address list on low energy.
 *      CM sends CSR_BT_CM_LE_WHITELIST_SET_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:      Identity of the calling process
 *      addressCount:   Number of addresses to be added in BLE whitelist 
 *      addressList:    Pointer to address list to be added in BLE White list
 *                      stored in the controller
 *----------------------------------------------------------------------------*/
void CsrBtCmLeWhitelistSetReqSend(CsrSchedQid    appHandle,
                                  CsrUint16      addressCount,
                                  CsrBtTypedAddr *addressList);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmRegisterHandlerReqSend
 *
 *  DESCRIPTION
 *      Register upper layer handler in the CM.
 *      This API doesn't generate any response back to the application.
 *
 *  PARAMETERS
 *      handlerType:        CSR_BT_CM_HANDLER_TYPE_x
 *      handle:             Handle for the upper layer
 *      flags:              Unused, must be zero
 *----------------------------------------------------------------------------*/

void CsrBtCmRegisterHandlerReqSend(CsrUint8     handlerType,
                                   CsrSchedQid  handle,
                                   CsrUint32    flags);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocRegisterReqSend
 *
 *  DESCRIPTION
 *      Register a destination appHandle for Isochronous functionality.
 *      CM sends CSR_BT_CM_ISOC_REGISTER_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      isoc_type:           Type of isochronous stream for registration
 *                           Permitted values :
 *                           CM_ISOC_TYPE_UNICAST
 *----------------------------------------------------------------------------*/

#ifdef CSR_BT_ISOC_ENABLE
void CmIsocRegisterReqSend(CsrSchedQid    appHandle,
                           CsrUint16      isoc_type);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocConfigureCigReqSend
 *
 *  DESCRIPTION
 *     Request to configure a CIG.
 *
 *     Once configuration is successful user will get a cig_id, cis_handle for
 *     each registered CIS. Configured CIG can be re-configured using cig_id if
 *     none of the CISes in the CIG are created. Complete set of parameters are
 *     expected to be passed while re-configuration, as new parameters will
 *     replace the configured parameters for the CIG.
 *
 *     Sleep Clock Accuracy for CIG shall be configured considering worst case
 *     SCA value of all slaves participating in CIG. if application cannot get
 *     the SCA of all the slaves then it shall set the value to zero.
 *
 *     CM sends CSR_BT_CM_ISOC_CONFIGURE_CIG_CFM back to the application.
 *
 *     Note: cis_config shall contain a valid pointer to each CIS that is
 *     registered under the CIG and it shall be valid for cis_count elements,
 *     rest shall be set to NULL.
 *
 *  PARAMETERS
 *      appHandle:                       Identity of the calling process
 *      sdu_interval_m_to_s:             Time interval between the start of consecutive SDUs
 *      sdu_interval_s_to_m:             Time interval between the start of consecutive SDUs
 *      max_transport_latency_m_to_s:    Maximum transport latency from master
 *      max_transport_latency_s_to_m:    Maximum transport latency from slave
 *      cig_id:                          Zero for new configuration, valid for re-configuration
 *      sca:                             Sleep clock accuracy
 *      packing:                         Interleaved, Sequential placement of packets
 *      framing:                         Indicates the format: framed or unframed
 *      cis_count:                       Number of CIS under CIG
 *      cis_config:                      Array of pointers to cis configuration
 *----------------------------------------------------------------------------*/
void CmIsocConfigureCigReqSend(CsrSchedQid    appHandle,
                               CsrUint24      sdu_interval_m_to_s,
                               CsrUint24      sdu_interval_s_to_m,
                               CsrUint16      max_transport_latency_m_to_s,
                               CsrUint16      max_transport_latency_s_to_m,
                               CsrUint8       cig_id,
                               CsrUint8       sca,
                               CsrUint8       packing,
                               CsrUint8       framing,
                               CsrUint8       cis_count,
                               CmCisConfig    *cis_config[]);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocRemoveCigReqSend
 *
 *  DESCRIPTION
 *      Request to remove the connected isochronous streams which have previously
 *      been configured in the controller.
 *      CM sends CSR_BT_CM_ISOC_REMOVE_CIG_CFM back to the application.

 *     Note: This command should be issued when all Connected Isochronous
 *           Streams in the CIG are disconnected.
 *
 *  PARAMETERS
 *      appHandle:       Identity of the calling process
 *      cig_id:          CIG Identifier to be removed
 *----------------------------------------------------------------------------*/
void CmIsocRemoveCigReqSend(CsrSchedQid appHandle, CsrUint8 cig_id);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocCisConnectReqSend
 *
 *  DESCRIPTION
 *     Request to establish connected isochronous streams(CIS) with remote device.
 *
 *     Request can be sent for more than one CIS connection establishment to
 *     reduce the latency, in such cases CSR_BT_CM_ISOC_CIS_CONNECT_CFM is sent
 *     one by one for each CIS establishment procedure. Connection
 *     can be requested for up to CM_MAX_SUPPORTED_CIS number of CIS.
 *
 *     Each CIS shall be configured using CSR_BT_CM_ISOC_CONFIGURE_CIG_REQ before CIS
 *     connection establishment after which reconfiguration will be disallowed.
 *
 *     Result of CIS establishment is notified using CSR_BT_CM_ISOC_CIS_CONNECT_CFM
 *     primitive to the destination phandle.
 *
 *     CM sends CSR_BT_CM_ISOC_CIS_CONNECT_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      cis_count:           number of CIS connections
 *      cis_conn:            list of cis handle and addresses to connect
 *----------------------------------------------------------------------------*/
void CmIsocCisConnectReqSend(CsrSchedQid    appHandle,
                             CsrUint8 cis_count,
                             CmCisConnection *cis_conn[]);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocCisConnectRspSend
 *
 *  DESCRIPTION
 *      Used to accept or reject the CIS connection initiation from remote device.
 *
 *     CIS handle is used to identity CIS connection to accept or reject
 *     establishment with hci error code.
 *
 *     Result of CIS establishment is notified using CSR_BT_CM_ISOC_CIS_CONNECT_CFM
 *     primitive.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      cis_handle:          CIS handle from connect ind
 *      status:              hci status
 *----------------------------------------------------------------------------*/
void CmIsocCisConnectRspSend(CsrSchedQid             appHandle,
                             hci_connection_handle_t cis_handle,
                             hci_return_t             status);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocCisDisconnectReqSend
 *
 *  DESCRIPTION
 *     Request to terminate CIS connection
 *
 *     CIS connection can be disconnected using cis_handle and appropriate HCI
 *     reason code for disconnection.
 *     CM sends CSR_BT_CM_ISOC_CIS_DISCONNECT_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      cis_handle:          CIS handle for disconnection
 *      reason:              Reason for command
 *----------------------------------------------------------------------------*/
void CmIsocCisDisconnectReqSend(CsrSchedQid appHandle,
                                hci_connection_handle_t cis_handle,
                                hci_reason_t            reason);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocSetupIsoDataPathReqSend
 *
 *  DESCRIPTION
 *      Request to setup the ISOC over HCI transport or vendor specific transport
 *      for the data path direction for CIS.
 *
 *     Data_path_id when set to zero, the data_path shall be over HCI transport.

 *     Data path direction specifies the direction of data from or to controller.
 *     User has to send this command twice to setup the data path
 *     0x00 Input (Host to Controller) and
 *     0x01 Output (Controller to Host).
 *     CM sends CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:            Identity of the calling process
 *      handle:               CIS handle
 *      data_path_direction:  direction for which the data path is being configured.
 *      data_path_id:         data transport path used
 *      codec_id:             coding format used
 *      controller_delay:     Controller delay
 *      codec_config_length:  length of codec configuration
 *      codec_config_data:    Codec configuration data
 *----------------------------------------------------------------------------*/
void CmIsocSetupIsoDataPathReqSend(CsrSchedQid appHandle,
                                     hci_connection_handle_t handle,
                                     CsrUint8 data_path_direction,
                                     CsrUint8 data_path_id,
                                     CsrUint8 codec_id[],
                                     CsrUint24 controller_delay,
                                     CsrUint8 codec_config_length,
                                     CsrUint8 *codec_config_data);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocRemoveIsoDataPathReqSend
 *
 *  DESCRIPTION
 *      Request to remove the ISO data path associated with CIS.
 *      CM sends CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:             Identity of the calling process
 *      handle:                CIS handle
 *      data_path_direction:   Direction of the path to be removed
 *----------------------------------------------------------------------------*/
void CmIsocRemoveIsoDataPathReqSend(CsrSchedQid appHandle,
                                      hci_connection_handle_t handle,
                                      CsrUint8 data_path_direction);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocCreateBigReqSend
 *
 *  DESCRIPTION
 *     Request to create a Broadcast Identification group with one or more
 *     Broadcast isochronous streams(BIS). All BISes in the BIG will have the
 *     same parameter values
 *
 *     Result of BIG create is notified using CSR_BT_CM_ISOC_CREATE_BIG_CFM
 *     primitive to the destination phandle.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      big_handle:          Identifier of the BIG
 *      adv_handle:          Identifies the associated periodic advertising train of the BIG
 *      big_config:          Big configuration Parameter
 *      num_bis:             Total number of BISes in the BIG
 *      encryption:          Encryption mode of the BISes( 0- unencrypted 1-encrypted)
 *      broadcast_code:      Encryption key(size 16) for encrypting payloads of all BISes.
 *----------------------------------------------------------------------------*/
void CmIsocCreateBigReqSend(CsrSchedQid    appHandle,
                            CsrUint8 big_handle,
                            CsrUint8 adv_handle,
                            CmBigConfigParam *big_config,
                            CsrUint8 num_bis,
                            CsrUint8 encryption,
                            CsrUint8 *broadcast_code);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocTerminateBigReqSend
 *
 *  DESCRIPTION
 *     Request to terminate BIG idenfied by the BIG handle
 *
 *     It also terminates the transmission of all BISes of the BIG, destroys
 *     the associated connection handles of the BISes in the BIG and removes
 *     the data paths for all BISes in the BIG.
 *
 *     CM sends CSR_BT_CM_ISOC_TERMINATE_BIG_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      big_handle:          Big handle for disconnection
 *      reason:              Reason for command
 *----------------------------------------------------------------------------*/
void CmIsocTerminateBigReqSend(CsrSchedQid appHandle, CsrUint8 big_handle, hci_reason_t reason);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocBigCreateSyncReqSend
 *
 *  DESCRIPTION
 *     Request to synchronize to  BIG described in the periodic advertising
 *     train specified by the Sync_Handle parameter.
 *
 *     Result of BIG create is notified using CSR_BT_CM_ISOC_BIG_CREATE_BIG_SYNC_CFM
 *     primitive to the destination phandle.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      sync_handle:         Identifies the associated periodic advertising train of the BIG
 *      big_sync_timeout     Maximum permitted time between successful receptions of BIS PDUs
 *      big_handle:          Identifier of the BIG
 *      mse                  maximum number of subevents that a Controller
 *                           should use to receive data payloads in each
 *                           interval for a BIS.
 *      encryption:          Encryption mode of the BISes( 0- unencrypted 1-encrypted)
 *      broadcast_code:      Encryption key(size 16) for encrypting payloads of all BISes.
 *      num_bis              Total number of BISes indices specified in the BIS[i] parameter
 *      bis                  List of indices corresponding to BIS(es) in the synchronized BIG
 *----------------------------------------------------------------------------*/
void CmIsocBigCreateSyncReqSend(CsrSchedQid    appHandle,
                            CsrUint16 sync_handle,
                            CsrUint16 big_sync_timeout,
                            CsrUint8 big_handle,
                            CsrUint8 mse,
                            CsrUint8 encryption,
                            CsrUint8 *broadcast_code,
                            CsrUint8 num_bis,
                            CsrUint8 *bis);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocBigTerminateSyncReqSend
 *
 *  DESCRIPTION
 *      Request to terminate BIG idenfied by the BIG handle
 *
 *     It also terminates the transmission of all BISes of the BIG, destroys
 *     the associated connection handles of the BISes in the BIG and removes
 *     the data paths for all BISes in the BIG.
 *     CM sends CSR_BT_CM_ISOC_BIG_TERMINATE_SYNC_IND signal back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      big_handle:          Big handle for disconnection
 *----------------------------------------------------------------------------*/
void CmIsocBigTerminateSyncReqSend(CsrSchedQid appHandle, 
                            CsrUint8 big_handle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocReadIsoLinkQualityReqSend
 *
 *  DESCRIPTION
 *      Request to read the counters related to the link quality that are associated
 *      with the isochronous stream in the controller.
 *      CM sends CM_ISOC_READ_ISO_LINK_QUALITY_CFM signal back to the application.
 *
 *  PARAMETERS
 *      appHandle:             Identity of the calling process
 *      handle:                Connected Isochronous Stream (CIS) and Broadcast Isochronous Stream (BIS) 
 *                             handle of isoc connection
 *----------------------------------------------------------------------------*/
void CmIsocReadIsoLinkQualityReqSend(CsrSchedQid appHandle, 
                                     hci_connection_handle_t handle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocConfigureCigTestReqSend
 *
 *  DESCRIPTION
 *     Request to configure a CIG.
 *
 *     Once configuration is successful user will get a cig_id, cis_handle for
 *     each registered CIS. Configured CIG can be re-configured using cig_id if
 *     none of the CISes in the CIG are created. Complete set of parameters are
 *     expected to be passed while re-configuration, as new parameters will
 *     replace the configured parameters for the CIG.
 *
 *     Sleep Clock Accuracy for CIG shall be configured considering worst case
 *     SCA value of all slaves participating in CIG. if application cannot get
 *     the SCA of all the slaves then it shall set the value to zero.
 *
 *     CM sends CSR_BT_CM_ISOC_CONFIGURE_CIG_TEST_CFM back to the application.
 *
 *     Note: cis_config shall contain a valid pointer to each CIS that is
 *     registered under the CIG and it shall be valid for cis_count elements,
 *     rest shall be set to NULL.
 *
 *  PARAMETERS
 *      appHandle:                       Identity of the calling process
 *      sdu_interval_m_to_s:             Time interval between the start of consecutive SDUs
 *      sdu_interval_s_to_m:             Time interval between the start of consecutive SDUs
 *      iso_interval;                    Time b/w two consecutive CIS anchor points
 *      cig_id:                          Zero for new configuration, valid for re-configuration
 *      ft_m_to_s;                       Flush timeout at master side
 *      ft_s_to_m;                       Flush timeout at slave side
 *      sca:                             Sleep clock accuracy
 *      packing:                         Interleaved, Sequential placement of packets
 *      framing:                         Indicates the format: framed or unframed
 *      cis_count:                       Number of CIS under CIG
 *      cis_test_config:                 Array of pointers to cis configuration
 *----------------------------------------------------------------------------*/
void CmIsocConfigureCigTestReqSend(CsrSchedQid      appHandle,
                                   CsrUint24        sdu_interval_m_to_s,
                                   CsrUint24        sdu_interval_s_to_m,
                                   CsrUint16        iso_interval,
                                   CsrUint8         cig_id,
                                   CsrUint8         ft_m_to_s,
                                   CsrUint8         ft_s_to_m,
                                   CsrUint8         sca,
                                   CsrUint8         packing,
                                   CsrUint8         framing,
                                   CsrUint8         cis_count,
                                   CmCisTestConfig *cis_test_config[]);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocCreateBigTestReqSend
 *
 *  DESCRIPTION
 *     Request to create a Broadcast Identification group with one or more
 *     Broadcast isochronous streams(BIS). All BISes in the BIG will have the
 *     same parameter values
 *
 *     Result of BIG create is notified using CSR_BT_CM_ISOC_CREATE_BIG_TEST_CFM
 *     primitive to the destination phandle.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      big_handle:          Identifier of the BIG
 *      adv_handle:          Identifies the associated periodic advertising train of the BIG
 *      big_config:          Big Test configuration Parameter
 *      num_bis:             Total number of BISes in the BIG
 *      encryption:          Encryption mode of the BISes( 0- unencrypted 1-encrypted)
 *      broadcast_code:      Encryption key(size 16) for encrypting payloads of all BISes.
 *----------------------------------------------------------------------------*/
void CmIsocCreateBigTestReqSend(CsrSchedQid    appHandle,
                            CsrUint8 big_handle,
                            CsrUint8 adv_handle,
                            CmBigTestConfigParam *big_config,
                            CsrUint8 num_bis,
                            CsrUint8 encryption,
                            CsrUint8 *broadcast_code);

#endif /* End of CSR_BT_ISOC_ENABLE */

#if defined(CSR_BT_INSTALL_EXTENDED_SCANNING) || defined(CSR_BT_INSTALL_EXTENDED_ADVERTISING) \
    || defined(CSR_BT_INSTALL_PERIODIC_SCANNING) || defined(CSR_BT_INSTALL_PERIODIC_ADVERTISING)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmGetAdvScanCapabilitiesReqSend
 *
 *  DESCRIPTION
 *      Get information on what APIs are available and size limitations.
 *      CM sends CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *----------------------------------------------------------------------------*/
void CmGetAdvScanCapabilitiesReqSend(CsrSchedQid appHandle);

#endif

#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvRegisterAppAdvSetReqSend
 *
 *  DESCRIPTION
 *      Application register to use as an advertising set.
 *      CM sends CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *      advHandle         Advertising set preference from application, 1 to CM_EXT_ADV_MAX_ADV_HANDLES.
 *                        Application can set it to CM_ULP_EXT_ADV_HANDLE_INVALID if flag is set CM_ULP_EXT_ADV_REG_SET_ASSIGNED_BY_STACK
 *                        and application has no preference for advertising set.
 *      flags             Configurations options.
 *                        CM_EXT_ADV_REG_SET_ASSIGNED_BY_STACK: If set, stack to assign available advertising set if
 *                        application provided adv_handle is CM_EXT_ADV_HANDLE_INVALID or it is not available.
 *                        If not set, advertisng set regstration would fail if application provided adv_handle is not available.
 *----------------------------------------------------------------------------*/
void CmExtAdvRegisterAppAdvSetReqSend(CsrSchedQid appHandle,
                                      CsrUint8 advHandle,
                                      CsrUint32 flags);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvUnregisterAppAdvSetReqSend
 *
 *  DESCRIPTION
 *      Application un-register the advertising set.
 *      CM sends CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle        Identity of the calling process
 *      advHandle        A registered advertising set value
 *----------------------------------------------------------------------------*/
void CmExtAdvUnregisterAppAdvSetReqSend(CsrSchedQid appHandle,
                                        CsrUint8 advHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetParamsReqSend
 *
 *  DESCRIPTION
 *      Write the advertising set paramaters.
 *      CM sends CSR_BT_CM_EXT_ADV_SET_PARAMS_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle              Identity of the calling process
 *      advHandle              A registered advertising set value
 *      advEventProperties     Advertising type.
 *      primaryAdvIntervalMin  Minimum advertising interval N = 0x20 to 0xFFFFFF  (Time = N * 0.625 ms)
 *      primaryAdvIntervalMax  Maximum advertising interval N = 0x20 to 0xFFFFFF  (Time = N * 0.625 ms)
 *      primaryAdvChannelMap   Bit mask field (bit 0 = Channel 37, bit 1 = Channel 38 and bit 2 = Channel 39)
 *      ownAddrType            Local address type
 *      peerAddr               Remote device address
 *      advFilterPolicy        Policy for processing scan and connect requests
 *      primaryAdvPhy          PHY for advertising paackets on Primary advertising channels
 *                             1 - LE 1M, 3 - LE Coded
 *      secondaryAdvMaxSkip    Maximum advertising events on the primary advertising
 *                             channel that can be skipped before sending an AUX_ADV_IND.
 *      secondaryAdvPhy        PHY for advertising paackets on Secondary advertising channels 
 *                             1 - LE 1M, 2 - LE 2M, 3 - LE Coded
 *      advSid                 Advertsing set ID.
 *                             advSid can be set to:
 *                             CM_EXT_ADV_SID_INVALID            - For legacy advertising
 *                             CM_EXT_ADV_SID_ASSIGNED_BY_STACK  - Stack will assign unique value
 *                             0 to 15                           - Application decides unique value
 *                             CM_EXT_ADV_SID_SHARE + (0 to 15)  - More than 1 advertising set can have this value
 *      reserved               Always set to 0.
 *----------------------------------------------------------------------------*/
void CmExtAdvSetParamsReqSend(CsrSchedQid appHandle,
                                    CsrUint8 advHandle,
                                    CsrUint16 advEventProperties,
                                    CsrUint32 primaryAdvIntervalMin,
                                    CsrUint32 primaryAdvIntervalMax,
                                    CsrUint8 primaryAdvChannelMap,
                                    CsrUint8 ownAddrType,
                                    TYPED_BD_ADDR_T peerAddr,
                                    CsrUint8 advFilterPolicy,
                                    CsrUint16 primaryAdvPhy,
                                    CsrUint8 secondaryAdvMaxSkip,
                                    CsrUint16 secondaryAdvPhy,
                                    CsrUint16 advSid,
                                    CsrUint32 reserved);

#ifdef INSTALL_CM_EXT_ADV_SET_PARAM_V2
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetParamsV2ReqSend
 *
 *  DESCRIPTION
 *      Write the advertising set V2 paramaters. 
 *      Similar to CmExtAdvSetParamsReqSend, but additional params e.g adv tx power can be configured
 *      CM sends CM_EXT_ADV_SET_PARAMS_V2_CFM back to the application.
 * 
 *  PARAMETERS
 *      appHandle:             Application handle
 *      advHandle              A registered advertising set value
 *      advEventProperties     Advertising type.
 *      primaryAdvIntervalMin  Minimum advertising interval N = 0x20 to 0xFFFFFF  (Time = N * 0.625 ms)
 *      primaryAdvIntervalMax  Maximum advertising interval N = 0x20 to 0xFFFFFF  (Time = N * 0.625 ms)
 *      primaryAdvChannelMap   Bit mask field (bit 0 = Channel 37, bit 1 = Channel 38 and bit 2 = Channel 39)
 *      ownAddrType            Local address type
 *      peerAddr               Remote device address
 *      advFilterPolicy        
 *      primaryAdvPhy          PHY for advertising paackets on Primary advertising channels
 *                             1 - LE 1M, 3 - LE Coded
 *      secondaryAdvMaxSkip    Maximum advertising events on the primary advertising
 *                             channel that can be skipped before sending an AUX_ADV_IND.
 *      secondaryAdvPhy        PHY for advertising paackets on Secondary advertising channels 
 *                             1 - LE 1M, 2 - LE 2M, 3 - LE Coded
 *      advSid                 Advertsing set ID.
 *                             advSid can be set to:
 *                             CM_EXT_ADV_SID_INVALID            - For legacy advertising
 *                             CM_EXT_ADV_SID_ASSIGNED_BY_STACK  - Stack will assign unique value
 *                             0 to 15                           - Application decides unique value
 *                             CM_EXT_ADV_SID_SHARE + (0 to 15)  - More than 1 advertising set can have this value
 *      advTxPower             Max power level at which the adv packets are transmitted 
 *                             Range : -127 to +20 in dBm, 0x7F for Host has no preference
 *                             Controller can choose power level lower than or equal to the one specified by Host
 *      scanReqNotifyEnable    RFA. Always set to 0.
 *      primaryAdvPhyOptions   RFA. Always set to 0.
 *      secondaryAdvPhyOptions RFA. Always set to 0.
 *----------------------------------------------------------------------------*/
void CmExtAdvSetParamsV2ReqSend(CsrSchedQid appHandle,
                                    CsrUint8 advHandle,
                                    CsrUint16 advEventProperties,
                                    CsrUint32 primaryAdvIntervalMin,
                                    CsrUint32 primaryAdvIntervalMax,
                                    CsrUint8 primaryAdvChannelMap,
                                    CsrUint8 ownAddrType,
                                    TYPED_BD_ADDR_T peerAddr,
                                    CsrUint8 advFilterPolicy,
                                    CsrUint16 primaryAdvPhy,
                                    CsrUint8 secondaryAdvMaxSkip,
                                    CsrUint16 secondaryAdvPhy,
                                    CsrUint16 advSid,
                                    CsrInt8 advTxPower,
                                    CsrUint8 scanReqNotifyEnable,
                                    CsrUint8 primaryAdvPhyOptions,
                                    CsrUint8 secondaryAdvPhyOptions);
#endif /* INSTALL_CM_EXT_ADV_SET_PARAM_V2 */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetDataReqSend
 *
 *  DESCRIPTION
 *      Write the extended advertising data.
 *      CM sends CSR_BT_CM_EXT_ADV_SET_DATA_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *      advHandle         A registered advertising set value
 *      operation         Part of the data to be set. 0 : Intermittent Fragment,
 *                        1 : First Fragment, 2 : Last Fragment, 3 : Complete Data
 *      fragPreference    Fragmentation preference at controller. 0 : May fragment , 1 : No fragment
 *      dataLen           Advertising data length (0 to 251 octets).
 *      data              Array of pointers to a 32 octet dynamically allocated buffer,
 *                        Max array size is CM_EXT_ADV_DATA_BYTES_PTRS_MAX
 *----------------------------------------------------------------------------*/
void CmExtAdvSetDataReqSend(CsrSchedQid appHandle,
                                    CsrUint8 advHandle,
                                    CsrUint8 operation,
                                    CsrUint8 fragPreference,
                                    CsrUint8 dataLen,
                                    CsrUint8 *data[]);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetScanRespDataReqSend
 *
 *  DESCRIPTION
 *      Write the extended advertising's scan response data.
 *      CM sends CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *      advHandle         A registered advertising set value
 *      operation         Part of the data to be set. 0 : Intermittent Fragment,
 *                        1 : First Fragment, 2 : Last Fragment, 3 : Complete Data
 *      fragPreference    Fragmentation preference at controller. 0 : May fragment , 1 : No fragment
 *      dataLen           Scan response data length (0 to 251 octets).
 *      data              Array of pointers to a 32 octet dynamically allocated buffer,
 *                        Max array size is CM_EXT_ADV_SCAN_RESP_DATA_BYTES_PTRS_MAX
 *----------------------------------------------------------------------------*/
void CmExtAdvSetScanRespDataReqSend(CsrSchedQid appHandle,
                                    CsrUint8 advHandle,
                                    CsrUint8 operation,
                                    CsrUint8 fragPreference,
                                    CsrUint8 dataLen,
                                    CsrUint8 *data[]);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvEnableReqSend
 *
 *  DESCRIPTION
 *      Enable or Disable extended advertising.
 *      CM sends CSR_BT_CM_EXT_ADV_ENABLE_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle        Identity of the calling process
 *      advHandle        A registered advertising set value
 *      enable           Enable (1) or Disable (0) extended advertising
 *----------------------------------------------------------------------------*/
void CmExtAdvEnableReqSend(CsrSchedQid appHandle,
                           CsrUint8 advHandle,
                           CsrUint8 enable);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvReadMaxAdvDataLenReqSend
 *
 *  DESCRIPTION
 *      Read the extended advertising max data length supported by controller.
 *      CM sends CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle      Identity of the calling process
 *      advHandle      A registered advertising set value
 *----------------------------------------------------------------------------*/
void CmExtAdvReadMaxAdvDataLenReqSend(CsrSchedQid appHandle, CsrUint8 advHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetRandomAddrReqSend
 *
 *  DESCRIPTION
 *      Set the advertising set's random device address to be used.
 *      CM sends CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle        Identity of the calling process
 *      advHandle        A registered advertising set value
 *      action           Option for generating or setting a random address
 *      randomAddr       Random address to be set
 *----------------------------------------------------------------------------*/
void CmExtAdvSetRandomAddrReqSend(CsrSchedQid appHandle,
                                              CsrUint8 advHandle,
                                              CsrUint16 action,
                                              CsrBtDeviceAddr randomAddr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetsInfoReqSend
 *
 *  DESCRIPTION
 *      Reports advertising and registered states for all advertising sets 
 *      supported by a device.
 *      CM sends CSR_BT_CM_EXT_ADV_SETS_INFO_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle      Identity of the calling process
 *----------------------------------------------------------------------------*/
void CmExtAdvSetsInfoReqSend(CsrSchedQid appHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvMultiEnableReqSend
 *
 *  DESCRIPTION
 *      Enable/disable advertising for 1 to 4 advertising sets. Further info
 *      be found in BT5.1: command = LE Set Extended Advertising Enable Command.
 *      CM sends CSR_BT_CM_EXT_ADV_MULTI_ENABLE_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:      Identity of the calling process
 *      enable:         Enable/disable advertising
 *      numSets:        Number of advertising sets to be enabled or
 *                      disabled by this prim.
 *      config:         Advertising set config data
 *----------------------------------------------------------------------------*/
void CmExtAdvMultiEnableReqSend(CsrSchedQid appHandle,
                                           CsrUint8 enable,
                                           CsrUint8 numSets,
                                           CmEnableConfig config[CM_EXT_ADV_MAX_NUM_ENABLE]);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvGetAddrReqSend
 *
 *  DESCRIPTION
 *      Returns the address associated with the extended advertising set.
 *      CM sends CM_DM_EXT_ADV_GET_ADDR_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:            Identity of the calling process
 *      advHandle:            A registered advertising set value
 *----------------------------------------------------------------------------*/
void CmExtAdvGetAddrReqSend(CsrSchedQid appHandle,
                                CsrUint8 advHandle);


#endif /* End of CSR_BT_INSTALL_EXTENDED_ADVERTISING */

#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanGetGlobalParamsReqSend
 *
 *  DESCRIPTION
 *      Read the global parameters to be used during extended scanning.
 *      CM sends CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *----------------------------------------------------------------------------*/
void CmExtScanGetGlobalParamsReqSend(CsrSchedQid appHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanSetGlobalParamsReqSend
 *
 *  DESCRIPTION
 *      Write the global parameters to be used during extended scanning.
 *
 *      This function should be called once at startup to set the global 
 *      prarmeters to be used by scanners during scanning.
 *      Otherwise, default global paramters will be used by the scanners.
 *
 *      This function may be called when scanners are scanning. Then Scanners 
 *      have to adpat with the new global paramters set.
 *
 *      Parameter values should be set as described below : 
 *      flags : RFU. Should be set to 0.
 *      own_address_type:
 *           0 - Public
 *           1 - Random
 *           2 - Resolvable Private Address (If resolved list has no match use public)
 *           3 - Resolvable Private address (If resolved list has no match use random)
 *      scanning_filter_policy :
 *           0 - Accept all advertising packets, except directed not addressed to device
 *           1 - White list only
 *           2 - Initiators Identity address is not this device.
 *           3 - White list only and Initiators Identity address identifies this device.
 *      filter_duplicates : 0 - False, 1 - True
 *      scanning_phys : BIT_0 - LE 1M, BIT_1 - LE 2M, BIT_2 - LE Coded
 *      phys : Scan Parameter Values for each LE PHYs
 *             Scan Type [0 - Passive Scanning , 1 - Active Scanning]
 *             Scan Interval [ 0x04 to 0xFFFF (2.5 ms to 40.95 s), Default = 100 ms]
 *             Scan Window [ 0x04 to 0xFFFF (2.5 ms to 40.95 s), Default = 95 ms]
 *
 *      CM sends CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:              Identity of the calling process
 *      flags:                  Bit field (RFU)
 *      own_address_type:       Local address type
 *      scanning_filter_policy: Scanning filter policy
 *      filter_duplicates:      Filter duplicates
 *      scanning_phys:          Bit Field for LE PHYs to be used for Scanning
 *      phys:                   Scan Parameter Values to be used for each LE PHYs.
 *----------------------------------------------------------------------------*/
void CmExtScanSetGlobalParamsReqSend(CsrSchedQid appHandle,
                                                CsrUint8 flags,
                                                CsrUint8 own_address_type,
                                                CsrUint8 scanning_filter_policy,
                                                CsrUint8 filter_duplicates,
                                                CsrUint16 scanning_phys,
                                                CmScanningPhy *phys);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanRegisterScannerReqSend
 *
 *  DESCRIPTION
 *      Register a scanner and filter rules to be used during extended 
 *      scanning.
 *
 *      A scanner is configured with 2 stages of filtering.
 *          - adv_filter
 *          - ad_structure_filter.
 *      The adv_filter will be applied first and will throw away any extended 
 *      advertising reports that do not meet this filter.
 *      The ad_structure_filter will be applied next. The extended advertising 
 *      report will be thrown away if no ad_types of interest are found.
 *
 *      CM sends CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle: Identity of the calling process
 *      flags : RFU. Should be set to 0.
 *      adv_filter : 
 *         1 - Receive all advertising data. Uses sub field below.
 *         2 - Do not receive any advertising data.
 *         3 - Only receive legacy advertising data (Primary advertising channel). Uses sub field below
 *         4 - Only receive extended advertising data (Secondary adverting channel).
 *         5 - Only receive extended advertising data with info about periodic advertising.
 *         6 - Only receive advertising data with flags AD type and the LE Limited Discoverable Mode flag set.
 *         7 - Only receive advertising data with flags AD type and the LE Limited Discoverable Mode flag or LE General Discoverable Mode flag set.
 *         8 - Receive only connectable or scannable or non-connectable/non-scannable. Uses sub field below
 *      adv_filter_sub_field1 : (Future options always set to 0)
 *         For adv_filter value 1 :
 *           0 - Receive Valid
 *           1 - Receive corrupted and valid.(For debug purpose only)
 *         For adv_filter value 3 :
 *           0 - Use DM_HCI_ULP_ADVERTISING_REPORT_IND (legacy way to report advertising data)
 *           1 - Use DM_ULP_EXT_SCAN_FILTERED_ADV_REPORT_IND to report advertising data
 *         For adv_filter value 8 :
 *           BIT_0 - BIT_1 :
 *             0 - Connectable
 *             1 - Scannable
 *             2 - Non-connectable and Non-Scannable
 *           BIT_2 - BIT 3 :
 *             0 - Directed
 *             1 - Undirected
 *             2 - Directed or Undirected
 *      adv_filter_sub_field2 : (Future options always set to 0)
 *      ad_structure_filter :
 *        0 - Send whole report, if all registered ad_types are in advertising report.
 *        1 - Send whole report, if a registered ad_type is in advertising report.
 *        2 - Only report the AD Structure data for the ad_types registered in reg_ad_types.
 *        3 - Only report the AD Structure data for the ad_types registered in reg_ad_types and 
 *            if all ad_types registered are in the advertising report.
 *      ad_structure_filter_sub_field1 : RFU. Should be set to 0.
 *      ad_structure_filter_sub_field2 : RFU. Should be set to 0.
 *      num_reg_ad_types : Only Upto 10 AD types can be registered.
 *      reg_ad_types : AD types to be used by the ad_structure_filter.
 *----------------------------------------------------------------------------*/
void CmExtScanRegisterScannerReqSend(CsrSchedQid appHandle,
                                                CsrUint32 flags,
                                                CsrUint16 adv_filter,
                                                CsrUint16 adv_filter_sub_field1,
                                                CsrUint32 adv_filter_sub_field2,
                                                CsrUint16 ad_structure_filter,
                                                CsrUint16 ad_structure_filter_sub_field1,
                                                CsrUint32 ad_structure_filter_sub_field2,
                                                CsrUint8 num_reg_ad_types,
                                                CsrUint8 *reg_ad_types);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanUnregisterScannerReqSend
 *
 *  DESCRIPTION
 *      Unregister the scanner registered with the passed scan handle.
 *      CM sends CSR_BT_CM_EXT_SCAN_UNREGISTER_SCANNER_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      scan_handle:         Scan handle of the registered scanner
 *----------------------------------------------------------------------------*/
void CmExtScanUnregisterScannerReqSend(CsrSchedQid appHandle,
                                                CsrUint8 scan_handle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanConfigureScannerReqSend
 *
 *  DESCRIPTION
 *      Configure the registered scanner with the passed scan handle.
 *      CM sends CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:              Identity of the calling process
 *      scan_handle : Scanner to be configured.
 *      use_only_global_params : 
 *        0 - FALSE 
 *        1 - TRUE (Remaining field values will not be used)
 *      scanning_phys : BIT_0 - LE 1M, BIT_1 - LE 2M, BIT_2 - LE Coded
 *      phys : Scan Parameter Values for each LE PHYs
 *             Use Scan Defaults [Use value set in DM_ULP_EXT_SCAN_SET_GLOBAL_PARAMS_REQ]
 *             Scan Type [0 - Passive Scanning , 1 - Active Scanning]
 *             Scan Interval [ 0x04 to 0xFFFF (2.5 ms to 40.95 s), Default = 100 ms]
 *             Scan Window [ 0x04 to 0xFFFF (2.5 ms to 40.95 s), Default = 95 ms]
 *----------------------------------------------------------------------------*/
void CmExtScanConfigureScannerReqSend(CsrSchedQid appHandle,
                                                CsrUint8 scan_handle,
                                                CsrUint8 use_only_global_params,
                                                CsrUint16 scanning_phys,
                                                CmScanPhy *phys);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanEnableScannersReqSend
 *
 *  DESCRIPTION
 *      Enable or Disable registered scanners.
 *      CM sends CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_CFM back to the application.
 *
 *      Note : The Scan Interval and Scan Window used by the Contoller LE Scanner 
 *      will be the largest scan window and smallest scan interval of all the 
 *      enabled scanners, if more than 1 scanner is enabled.
 *      This includes previously enabled scanners also.
 *
 *  PARAMETERS
 *      appHandle:            Identity of the calling process
 *      enable:               Scan enable or disable
 *      num_of_scanners:      Upto 5 scanner will be enabled or disabled.
 *      scanners:             Scan handles of the scanner to enable or disable.
 *                            Duration - 0 Scanning until disabled.
 *                                      0x01 - 0xFFFF - RFU
 *----------------------------------------------------------------------------*/
void CmExtScanEnableScannersReqSend(CsrSchedQid appHandle,
                                                CsrUint8 enable,
                                                CsrUint8 num_of_scanners,
                                                CmScanners *scanners);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanGetCtrlScanInfoReqSend
 *
 *  DESCRIPTION
 *      Get Controller's scanners configuration information.
 *      CM sends CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM back to the application.
 *
 *      Note :- There is only 1 LE Controller shared by different scanners. So, 
 *      largest Scan Window and smallest Scan Interval values will be used from
 *      the active scanners.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *----------------------------------------------------------------------------*/
void CmExtScanGetCtrlScanInfoReqSend(CsrSchedQid appHandle);

#ifdef INSTALL_CM_EXT_SET_CONN_PARAM
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtSetConnParamsReqSend
 *
 *  DESCRIPTION
 *       Set extended connection parameters that the local device uses when creating 
 *  extended connections.
 *
 *  PARAMETERS
 *      appHandle:               Application handle
 *      advHandle                Reserved for future use. Shall be set to 0xFF
 *      subevent                 Reserved for future use. Shall be set to 0xFF
 *      connAttemptTimeout       Connection timeout
 *      ownAddressType           Own Address Type used in LE connection
 *      phyCount                 Number of entries in initPhys[]
 *      initPhys                 Initiating Phy parameters
 *----------------------------------------------------------------------------*/
void CmExtSetConnParamsReqSend(CsrSchedQid appHandle,
                                        CsrUint8 advHandle,
                                        CsrUint8 subevent,
                                        CsrUint16 connAttemptTimeout,
                                        CsrUint8 ownAddressType,
                                        CsrUint8 phyCount,
                                        CmInitiatingPhy *initPhys[]);
#endif /* End of INSTALL_CM_EXT_SET_CONN_PARAM */
#endif /* End of CSR_BT_INSTALL_EXTENDED_SCANNING */

#ifdef INSTALL_CM_DM_CONFIGURE_DATA_PATH
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmConfigureDataPathReqSend
 *
 *  DESCRIPTION
 *       Configure data path in a given direction between the Controller and the Host.
 *       CM sends CM_DM_HCI_CONFIGURE_DATA_PATH_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:               Identity of the calling process
 *      dataPathDirection:       Data Path direction.
 *                               0x00 - Input (Host to Controller)
 *                               0x01 - Output (Controller to Host)
 *      dataPathId:              Data Path ID
 *                               0x01 to 0xFE - Logical channel number; 
 *                                              the meaning is vendor-specific.
 *                               0x00 and 0xFF - RFU
 *      vendorSpecificConfigLen: Length of Vendor-specific config in octets
 *      vendorSpecificConfig:    Vendor-specific Configuration data for the 
 *                               data path being configured.
 *                               Each index can hold maximum of 
 *                               HCI_CONFIGURE_DATA_PATH_PER_PTR octets.
 *----------------------------------------------------------------------------*/
void CmDmConfigureDataPathReqSend(CsrSchedQid appHandle,
                                            CsrUint8 dataPathDirection,
                                            CsrUint8 dataPathId,
                                            CsrUint8 vendorSpecificConfigLen,
                                            CsrUint8 *vendorSpecificConfig[]);
#endif /* End of INSTALL_CM_DM_CONFIGURE_DATA_PATH */

#ifdef INSTALL_CM_DM_LE_READ_CHANNEL_MAP
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmLeReadChannelMapReqSend
 *
 *  DESCRIPTION
 *       Read the current Channel map for the peer connection.
 *       CM sends CM_DM_LE_READ_CHANNEL_MAP_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:      Application handle
 *      peerAddr:       Remote device address
 *----------------------------------------------------------------------------*/
void CmDmLeReadChannelMapReqSend(CsrSchedQid appHandle,
                                      TYPED_BD_ADDR_T peerAddr);
#endif /* End of INSTALL_CM_DM_LE_READ_CHANNEL_MAP */


#ifdef CSR_BT_INSTALL_PERIODIC_ADVERTISING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvSetParamsReqSend
 *
 *  DESCRIPTION
 *      Set the periodic advertising parameters for the registered advertising set.
 *      CM sends CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle                  Identity of the calling process
 *      advHandle                  A registered advertising set value
 *      flags                      Always set to 0. RFU
 *      periodicAdvIntervalMin     Minimum periodic advertising interval 
 *                                 [N = 0x6 to 0xFFFFFF  (Time = N * 1.25 ms)]
 *                                 Default = 0x320 (Time = 1 second)
 *      periodicAdvIntervalMax     Maximum periodic advertising interval.
 *                                 [N = 0x6 to 0xFFFFFF  (Time = N * 1.25 ms)]
 *                                 Default = 0x640 (Time = 2 second)
 *      periodicAdvProperties      Always set to 0
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvSetParamsReqSend(CsrSchedQid appHandle,
                                   CsrUint8 advHandle,
                                   CsrUint32 flags,
                                   CsrUint16 periodicAdvIntervalMin,
                                   CsrUint16 periodicAdvIntervalMax,
                                   CsrUint16 periodicAdvProperties);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvSetDataReqSend
 *
 *  DESCRIPTION
 *      Set the periodic advertising data.
 *      CM sends CSR_BT_CM_PERIODIC_ADV_SET_DATA_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *      advHandle         A registered advertising set value
 *      operation         Part of the data to be set. 0 : Intermittent Fragment,
 *                        1 : First Fragment, 2 : Last Fragment, 3 : Complete Data
 *      dataLength        Advertising data length (0 to 251 octets).
 *      data              Array of pointers to a 32 octet buffer. Max size is 8.
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvSetDataReqSend(CsrSchedQid appHandle,
                                 CsrUint8 advHandle,
                                 CsrUint8 operation,
                                 CsrUint8 dataLength,
                                 CsrUint8 *data[]);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvReadMaxAdvDataLenReqSend
 *
 *  DESCRIPTION
 *      Read the max allowed periodic advertising data for an advertising set.
 *      CM sends CSR_BT_CM_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *      advHandle         A registered advertising set value
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvReadMaxAdvDataLenReqSend(CsrSchedQid appHandle,
                                           CsrUint8 advHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvStartReqSend
 *
 *  DESCRIPTION
 *      Starts a periodic advertising train.
 *      CM sends CSR_BT_CM_PERIODIC_ADV_START_CFM back to the application.
 *
 *  Note: This primitive will enable ADI field in AUX_SYNC_IND PDU if the controller
 *        supports Periodic Advertising ADI feature.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *      advHandle         A registered advertising set value
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvStartReqSend(CsrSchedQid appHandle, CsrUint8 advHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvStopReqSend
 *
 *  DESCRIPTION
 *      Stops a periodic advertising train or just the associated extended advertising.
 *      CM sends CSR_BT_CM_PERIODIC_ADV_STOP_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *      advHandle         A registered advertising set value
 *      stopAdvertising   Stop periodic advertising and associated extended 
 *                        advertising
 *                        0 - stop periodic advertising,
 *                        1 - stop extended advertising and then stop periodic advertising
 *                        2 - stop extended advertising
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvStopReqSend(CsrSchedQid appHandle, CsrUint8 advHandle, CsrUint8 stopAdvertising);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvSetTransferReqSend
 *
 *  DESCRIPTION
 *      Send synchronization information about the periodic advertising in an 
 *      advertising set to a connected peer device.
 *      CM sends CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *      addrt             Connected peer device address
 *      serviceData       service data.
 *      advHandle         a registered advertising set value
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvSetTransferReqSend(CsrSchedQid appHandle, 
                                        TYPED_BD_ADDR_T addrt,
                                        CsrUint16 serviceData,
                                        CsrUint8 advHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvEnableReqSend
 *
 *  DESCRIPTION
 *      Send request to enable periodic advertising.
 *      CM sends CSR_BT_CM_PERIODIC_ADV_ENABLE_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *      advHandle         a registered advertising set value
 *      flags             Flags field (bit defines below)
                          bit 0: If set enable extended advertising while enabling periodic
                          advertising.
                          bit 1: If set disable extended advertising while disabling periodic
                          advertising
 *      enable            Enable field (bit defines below)
                          bit 0: If set enable periodic advertising, else disable.
                          bit 1: If set add ADI field to AUX_SYNC_IND PDU when enabling
                          periodic advertising.
 *
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvEnableReqSend(CsrSchedQid appHandle,
                                    CsrUint8 advHandle,
                                    CsrUint16 flags,
                                    CsrUint8 enable);

#endif /* End of CSR_BT_INSTALL_PERIODIC_ADVERTISING */

#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanStartFindTrainsReqSend
 *
 *  DESCRIPTION
 *      Search for periodic trains that meet a specified ad_structure filter.
 *      CM sends CSR_BT_CM_PERIODIC_SCAN_START_FIND_TRAINS_CFM back to the application.
 *      Also, CM sends CSR_BT_CM_EXT_SCAN_DURATION_EXPIRED_IND signal back to the application
 *      when the scanner is disabled due to the end of the period of time.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      flags                Bit field (RFU)
 *      scanForXSeconds      Always set to 0. RFU
 *      adStructureFilter    AD structure filter
 *      adStructureFilterSubField1 AD structure filter sub field 1
 *      adStructureFilterSubField2 AD structure filter sub field 2
 *      adStructureInfoLen   AD Structure info length
 *      adStructureInfo      Pointer to AD Structure info
 *----------------------------------------------------------------------------*/
void CmPeriodicScanStartFindTrainsReqSend(CsrSchedQid appHandle,
                                        CsrUint32 flags,
                                        CsrUint16 scanForXSeconds,
                                        CsrUint16 adStructureFilter,
                                        CsrUint16 adStructureFilterSubField1,
                                        CsrUint32 adStructureFilterSubField2,
                                        CsrUint8  adStructureInfoLen,
                                        CsrUint8* adStructureInfo);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanStopFindTrainsReqSend
 *
 *  DESCRIPTION
 *      Stop scanning for periodic trains.
 *      CM sends CSR_BT_CM_PERIODIC_SCAN_STOP_FIND_TRAINS_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      scanHandle:          Scan handle return in DM_ULP_PERIODIC_SCAN_START_FIND_TRAINS_CFM
 *----------------------------------------------------------------------------*/
void CmPeriodicScanStopFindTrainsReqSend(CsrSchedQid appHandle,
                                         CsrUint8 scanHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncToTrainReqSend
 *
 *  DESCRIPTION
 *      Establish sync to one of the periodic trains in the primitive.
 *      CM sends CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:              Identity of the calling process
 *      reportPeriodic:         Report periodic advetisement data [0 - no, 1 - yes]
 *      skip:                   Max number of periodic advertising events that can be skipped after a successful receive
 *      syncTimeout:            Synchronization timeout for the periodic advertising train once synchronised. [0x000A to 0x4000 (Time = n * 10 ms)]
 *      syncCteType:            Always set to 0. RFU
 *      attemptSyncForXSeconds: Always set to 0. RFU
 *      numberOfPeriodicTrains: Number of periodic train records. [Min - 1, Max - 3]
 *      periodicTrains:         Periodic trains info i.e. ADV SID and TYPED_BD_ADDR_T
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncToTrainReqSend(CsrSchedQid appHandle,
                                      CsrUint8 reportPeriodic,
                                      CsrUint16 skip,
                                      CsrUint16 syncTimeout,
                                      CsrUint8 syncCteType,
                                      CsrUint16 attemptSyncForXSeconds,
                                      CsrUint8 numberOfPeriodicTrains,
                                      CmPeriodicScanTrains *periodicTrains);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncToTrainCancelReqSend
 *
 *  DESCRIPTION
 *      Cancel an attempt to synchronise on to a periodic train.
 *      CM sends CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncToTrainCancelReqSend(CsrSchedQid appHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncAdvReportEnableReqSend
 *
 *  DESCRIPTION
 *      Enable or disable receiving of periodic advertising data report i.e. 
 *      DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_IND  events.
 *      CM sends CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      syncHandle:          Sync handle for the periodic train synced by controller
 *      enable:              Enable (1) or disable (0) periodic advertising data report
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncAdvReportEnableReqSend(CsrSchedQid appHandle,
                                              CsrUint16 syncHandle,
                                              CsrUint8 enable);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncTerminateReqSend
 *
 *  DESCRIPTION
 *      Terminate sync to a currently synced periodic train.
 *      CM sends CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      syncHandle:          Sync handle for the periodic train synced by controller
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncTerminateReqSend(CsrSchedQid appHandle,
                                        CsrUint16 syncHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncLostRspSend
 *
 *  DESCRIPTION
 *      Application inform the Periodic Scanning Sync manager to stop reading
 *      periodic train advert data for this train.
 *      Must be sent when a DM_HCI_ULP_PERIODIC_SCAN_SYNC_LOSS_IND is received.
 *      This API doesn't generate any response back to the application.
 *
 *  PARAMETERS
 *      syncHandle           Sync handle for the periodic train synced by controller
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncLostRspSend(CsrUint16 syncHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncTransferReqSend
 *
 *  DESCRIPTION
 *      Send synchronization information about the periodic advertising train
 *      identified by the Sync Handle parameter to a connected device.
 *      CM sends CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:            Identity of the calling process
 *      addrt:                Typed BD Addr of the connected peer device
 *      serviceData:          Always set to 0. RFU
 *      syncHandle:           Sync handle for the periodic train synced by controller
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncTransferReqSend(CsrSchedQid appHandle,
                                       TYPED_BD_ADDR_T addrt,
                                       CsrUint16 serviceData,
                                       CsrUint16 syncHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncTransferParamsReqSend
 *
 *  DESCRIPTION
 *      Send synchronization information about the periodic advertising train
 *      identified by the Sync Handle parameter to a connected device.
 *      CM sends CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      addrt                Typed BD Addr of the connected peer device
 *      skip                 Max number of periodic advertising events that can be skipped after a successful receive
 *      syncTimeout          Synchronization timeout for the periodic advertising train once synchronised. [0x000A to 0x4000 (Time = n * 10 ms)]
 *      mode                 Action to be taken when periodic advertising synchronization information is received
 *      cteType              Constant tone extension
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncTransferParamsReqSend(CsrSchedQid appHandle,
                                            TYPED_BD_ADDR_T addrt,
                                            CsrUint16 skip,
                                            CsrUint16 syncTimeout,
                                            CsrUint8 mode,
                                            CsrUint8 cteType);

#endif /* End of CSR_BT_INSTALL_PERIODIC_SCANNING */

#ifdef CSR_BT_INSTALL_CRYPTO_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoGeneratePublicPrivateKeyReqSend
 *
 *  DESCRIPTION
 *      Request for the generation of a public/
 *      private encryption key pair using ECC.
 *      CM sends CSR_BT_CM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      keyType              Type of encryption required
                             CSR_BT_CM_CRYPTO_ECC_P192/CSR_BT_CM_CRYPTO_ECC_P256
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoGeneratePublicPrivateKeyReqSend(CsrSchedQid appHandle,
                                                  CsrBtCmCryptoEccType keyType);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoGenerateSharedSecretKeyReqSend
 *
 *  DESCRIPTION
 *      Request for the generation of a shared secret encryption key 
 *      from a public/private key pair using ECHD.
 *      CM sends CSR_BT_CM_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      keyType              Type of encryption required
 *                           CSR_BT_CM_CRYPTO_ECC_P192/CSR_BT_CM_CRYPTO_ECC_P256
 *      privateKey           Private Key array used to generate shared secret key
 *                           The array must be 16 words large (32 bytes)
 *      publicKey            Public Key array used to generate shared secret key
 *                           The array must be 32 words large (64 bytes)
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoGenerateSharedSecretKeyReqSend(CsrSchedQid appHandle,
                                                 CsrBtCmCryptoEccType keyType,
                                                 CsrUint16 *privateKey,
                                                 CsrUint16 *publicKey);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoEncryptReqSend
 *
 *  DESCRIPTION
 *      Request for the encryption of a 128-bitblock of data 
 *      using a 128-bit encryption key using AES.
 *      CM sends CSR_BT_CM_CRYPTO_ENCRYPT_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      dataArray            Array containing the data to be encrypted
 *                           The array must be 8 words large (16 bytes)
 *      keyArray             Key array used to encrypt the given data
 *                           The array must be 8 words large (16 bytes)
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoEncryptReqSend(CsrSchedQid appHandle,
                                 CsrUint16 *dataArray,
                                 CsrUint16 *keyArray);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoHashReqSend
 *
 *  DESCRIPTION
 *      Request for the hashing of a data block 
 *      of arbitrary length using SHA256.
 *      CM sends CSR_BT_CM_CRYPTO_HASH_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      dataArray            Array containing the data to be hashed
 *                           The array is of variable length
 *      arraySize            Size of the data array in octets
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoHashReqSend(CsrSchedQid appHandle,
                              CsrUint16 *dataArray,
                              CsrUint16 arraySize);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoHashContinueReqSend
 *
 *  DESCRIPTION
 *      Request the continuation of the SHA256 encryption operation 
 *      for the remaining data.
 *      CM sends CSR_BT_CM_CRYPTO_HASH_CFM with appropriate result code back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      dataArray            Array containing the data to be hashed.
 *                           The array is of variable length.
 *                           This must be the same array used in the initial
 *                           call for hashing
 *      arraySize            Size of the data array in octets
 *      currentIndex         Index pointing to the first character of the
 *                           next hash block
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoHashContinueReqSend(CsrSchedQid appHandle,
                                      CsrUint16 *dataArray,
                                      CsrUint16 arraySize,
                                      CsrUint16 currentIndex);

 /*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoDecryptReqSend
 *
 *  DESCRIPTION
 *      Request for the decryption of a 128-bitblock of data 
 *      using a 128-bit decryption key using AES.
 *      CM sends CSR_BT_CM_CRYPTO_DECRYPT_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      dataArray            Array containing the data to be encrypted
 *                           The array must be 8 words large (16 bytes)
 *      keyArray             Key array used to encrypt the given data
 *                           The array must be 8 words large (16 bytes)
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoDecryptReqSend(CsrSchedQid appHandle,
                                 CsrUint16 *dataArray,
                                 CsrUint16 *keyArray);

 /*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoAesCtrReqSend
 *
 *  DESCRIPTION
 *      Encrypt or decrypt data using the AES128-CTR algorithm.
 *      CM sends CSR_BT_CM_CRYPTO_AES_CTR_CFM back to the application.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      counter              Initial counter value
 *      flags                Crypto flags
 *      secretKey            128-bit secret key
 *      nonce                128-bit nonce array
 *      dataLen              Number of uint16 input data length; Max 32
 *      data                 Pointer to uint16 data
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoAesCtrReqSend(CsrSchedQid appHandle,
                                CsrUint32 counter,
                                CsrBtCmCryptoAesCtrFlags flags,
                                CsrUint16 *secretKey,
                                CsrUint16 *nonce,
                                CsrUint16 dataLen,
                                CsrUint16 *data);
#endif /* End of CSR_BT_INSTALL_CRYPTO_SUPPORT */

#ifdef CSR_BT_INSTALL_CM_QHS_PHY_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmQhsPhySet
 *
 *  DESCRIPTION
 *      Set QHS PHY for a connection identified by deviceAddr.
 *      This API doesn't generate any response back to the application.
 *
 *  PARAMETERS
 *      deviceAddr:         Address for which QHS is enabled/disabled.
 *      qhsPhyConnected     Indicates whether QHS phy is getting used for
 *                          this connection or not.
 *----------------------------------------------------------------------------*/
void CsrBtCmQhsPhySet(CsrBtDeviceAddr *deviceAddr, CsrBool qhsPhyConnected);
#endif /* CSR_BT_INSTALL_CM_QHS_PHY_SUPPORT */

#ifdef CSR_BT_INSTALL_CM_SWB_DISABLE_STATE
/*----------------------------------------------------------------------------*
 *  NAME
 *  CmDisableSWBSupport
 *
 *  DESCRIPTION
 *      Disable local support for SWB for a particular handset.
 *      This API doesn't generate any response back to the application.
 *
 *  PARAMETERS
 *      deviceAddr:         Address for which SWB is disabled.
 *----------------------------------------------------------------------------*/
void CmDisableSWBSupport(CsrBtDeviceAddr *deviceAddr);
#endif /* CSR_BT_INSTALL_CM_SWB_DISABLE_STATE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmRfcConnectAcceptRspSend
 *
 *  DESCRIPTION
 *      Application uses this API to accept or reject the RFC connection which has
 *      been indicated to it by CSR_BT_CM_RFC_CONNECT_ACCEPT_IND. All the 
 *      parameters except btConnId are only used when the connection is accepted.
 *
 *      CM Sends CSR_BT_CM_CONNECT_ACCEPT_CFM as a response if accept is TRUE, 
 *      otherwise there is no response to this API.
 *
 * PARAMETERS
 *      appHandle:      Identity of the calling process.
 *      btConnId:       Global bluetooth connection identifier.
 *      deviceAddr:     Bluetooth Device Address 
 *      accept:         connection is accepted (TRUE) or rejected (FALSE).
 *      serverChannel:  local server channel.
 *      modemStatus:    RFC modem Status.
 *      breakSignal:    Break Signal.
 *      mscTimeout:     RFC Modem Status Timeout in milliseconds.
 *----------------------------------------------------------------------------*/
#define CsrBtCmRfcConnectAcceptRspSend(_appHandle,                                      \
                                       _btConnId,                                       \
                                       _deviceAddr,                                     \
                                       _accept,                                         \
                                       _serverChannel,                                  \
                                       _modemStatus,                                    \
                                       _breakSignal,                                    \
                                       _mscTimeout)                                     \
    do                                                                                  \
    {                                                                                   \
        CsrBtCmRfcConnectAcceptRsp *rsp;                                                \
        rsp = (CsrBtCmRfcConnectAcceptRsp *) CsrPmemZalloc(sizeof(*rsp));               \
        rsp->type               = CSR_BT_CM_RFC_CONNECT_ACCEPT_RSP;                     \
        rsp->appHandle          = _appHandle;                                           \
        rsp->btConnId           = _btConnId;                                            \
        rsp->deviceAddr         = _deviceAddr;                                          \
        rsp->accept             = _accept;                                              \
        rsp->serverChannel      = _serverChannel;                                       \
        rsp->modemStatus        = _modemStatus;                                         \
        rsp->breakSignal        = _breakSignal;                                         \
        rsp->mscTimeout         = _mscTimeout;                                          \
        CsrBtCmPutMessageDownstream(rsp);                                               \
    } while (0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmL2caConnectAcceptRspSend
 *
 *  DESCRIPTION
 *      Application uses this API to accept or reject the L2CAP connection which
 *      has been indicated to it by CSR_BT_CM_L2CA_CONNECT_ACCEPT_IND.
 *
 *      CM Sends CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM as a response if accept is TRUE, 
 *      otherwise there is no response to this API.
 *
 * PARAMETERS
 *      appHandle:      Identity of the calling process
 *      accept:         connection is accepted (TRUE) or rejected (FALSE).
 *      btConnId:       Global bluetooth connection identifier.
 *      localPsm:       Local PSM for which the connection is requested.
 *      deviceAddr:     Bluetooth address of the device.
 *      identifier:     L2CA identifier for the requested connection.
 *      conftabCount:   configuration tab count.
 *      conftab:        configuration tab pointer.
 *      minEncKeySize:  Minimum Encryption key size.
 *----------------------------------------------------------------------------*/
#define CsrBtCmL2caConnectAcceptRspSend(_appHandle,                                     \
                                        _accept,                                        \
                                        _btConnId,                                      \
                                        _localPsm,                                      \
                                        _deviceAddr,                                    \
                                        _identifier,                                    \
                                        _conftabCount,                                  \
                                        _conftab,                                       \
                                        _minEncKeySize)                                 \
    do                                                                                  \
    {                                                                                   \
        CsrBtCmL2caConnectAcceptRsp *rsp;                                               \
        rsp = (CsrBtCmL2caConnectAcceptRsp *) CsrPmemZalloc(sizeof(*rsp));              \
        rsp->type           = CSR_BT_CM_L2CA_CONNECT_ACCEPT_RSP;                        \
        rsp->phandle        = _appHandle;                                               \
        rsp->accept         = _accept;                                                  \
        rsp->btConnId       = _btConnId;                                                \
        rsp->localPsm       = _localPsm;                                                \
        rsp->deviceAddr     = _deviceAddr;                                              \
        rsp->identifier     = _identifier;                                              \
        rsp->conftabCount   = _conftabCount;                                            \
        rsp->conftab        = _conftab;                                                 \
        rsp->minEncKeySize  = _minEncKeySize;                                           \
        CsrBtCmPutMessageDownstream(rsp);                                               \
    } while (0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmL2caTpConnectReqSend
 *
 *  DESCRIPTION
 *      Application uses this API to establish LE Connection Oriented 
 *      Channel (COC) over L2CAP.
 *      CM sends CM_L2CA_TP_CONNECT_CFM back to the application on Success or Failure.
 *
 *      Note: The API is tested only for establising L2CAP COC over LE transport.
 *            Using this API for establising L2CAP COC over BR/EDR transport shows
 *            unexpected behavior.
 *
 *  PARAMETERS
 *      appHandle:          Protocol handle
 *      tpdAddrT:           BT address with transport and address type of device to bonded with
 *      localPsm:           The local PSM channel
 *      remotePsm:          Remote PSM channel to connect to
 *      localMtu:           Requested Local MTU size (bytes) (Incoming PDU)
 *      secLevel:           Level of security to be applied. Refer to 
 *                          "LE Service Security Requirements" in dm_prim.h.
 *      minEncKeySize       Minimum encryption key size
 *      credits             Initial Credits
 *      connFlags           Connection Flags
 *----------------------------------------------------------------------------*/
CmL2caTpConnectReq *CmL2caTpConnectReq_struct(CsrSchedQid         appHandle,
                                                    CsrBtTpdAddrT       tpdAddrT,
                                                    psm_t               localPsm,
                                                    psm_t               remotePsm,
                                                    l2ca_mtu_t          localMtu,
                                                    l2ca_mtu_t          transmitMtu,
                                                    CsrUint8            dataPriority,
                                                    dm_security_level_t secLevel,
                                                    CsrUint8            minEncKeySize,
                                                    CsrUint16           credits,
                                                    CsrUint16           connFlags,
                                                    CsrUint16           context);

#define CmL2caTpConnectReqSend(_appHandle,_tpdAddrT,_localPsm,_remotePsm,_localMtu,_secLevel, _minEncKeySize, _credits, _connFlags) { \
            CmL2caTpConnectReq *msg__;                                   \
            msg__=CmL2caTpConnectReq_struct(_appHandle,_tpdAddrT,_localPsm,_remotePsm,_localMtu, 0, CSR_BT_CM_PRIORITY_NORMAL,_secLevel,_minEncKeySize,_credits,_connFlags, CSR_BT_CM_CONTEXT_UNUSED); \
            CsrBtCmPutMessageDownstream(msg__);}

#define CmL2caTpConnectReqExtSend(_appHandle,_tpdAddrT,_localPsm,_remotePsm,_localMtu,_secLevel, _minEncKeySize, _credits, _connFlags, _context)  { \
           CmL2caTpConnectReq *msg__;                                   \
           msg__=CmL2caTpConnectReq_struct(_appHandle,_tpdAddrT,_localPsm,_remotePsm,_localMtu, 0, CSR_BT_CM_PRIORITY_NORMAL,_secLevel,_minEncKeySize,_credits,_connFlags, _context); \
           CsrBtCmPutMessageDownstream(msg__);}


/*----------------------------------------------------------------------------*
 *  NAME
 *     Cml2caAddCreditReqSend
 *
 *  DESCRIPTION
 *      Application uses this API to increase the credits of LE L2CAP Connection 
 *      Oriented Channel (COC) established with the remote device.
 *      CM sends CM_L2CA_ADD_CREDIT_CFM back to the application which has
 *      established the LE L2CAP Connection Oriented Channel (COC).
 *
 *  PARAMETERS
 *        appHandle:       Application handle
 *        btConnId:        Channel identifier for the LE L2CAP COC
 *        context:         a context value (passed back in the confirm)
 *        credits:         Credits to be added
 *----------------------------------------------------------------------------*/
CmL2caAddCreditReq *Cml2caAddCreditReq_struct(CsrSchedQid     appHandle,
                                                    CsrBtConnId         btConnId,
                                                    CsrUint16           context,
                                                    CsrUint16           credits);

#define Cml2caAddCreditReqSend(_appHandle,_btConnId,_context,_credits) { \
        CmL2caAddCreditReq *msg__; \
        msg__=Cml2caAddCreditReq_struct(_appHandle,_btConnId,_context,_credits);\
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmL2caTpConnectAcceptRspSend
 *
 *  DESCRIPTION
 *      Application uses this API to accept or reject the L2CAP Connection Oriented
 *      Channel (COC) establishment indicated by CM_L2CA_TP_CONNECT_ACCEPT_IND event.
 *      Application must respond to the CM_L2CA_TP_CONNECT_ACCEPT_IND event with accept
 *      or reject within specific time i.e. CONNECTION_TIMEOUT. Otherwise the connection
 *      request gets timeout on the remote device.
 *
 *      CM Sends CM_L2CA_TP_CONNECT_ACCEPT_CFM as a response if accept is TRUE, 
 *      otherwise there is no response to this API.
 *
 * PARAMETERS
 *      appHandle:      Application Handle
 *      accept:         connection is accepted (TRUE) or rejected (FALSE).
 *      btConnId:       Global bluetooth connection identifier.
 *      localPsm:       Local PSM for which the connection is requested.
 *      tpdAddrT:       Bluetooth address of the device with type and transport.
 *      identifier:     L2CA identifier for the requested connection.
 *      localMtu:       Requested Local MTU size (bytes) (Incoming PDU)
 *      transmitMtu     Maximum payload size (bytes) (Outgoing PDU)
 *                      In the case that transmitMtu < than the Mtu
 *                      received from the remote device in a L2CA_CONFIG_IND
 *      minEncKeySize   Minimum encryption key size 
 *      credits         Initial Credit value in the range 0 to 0xFFFF
 *----------------------------------------------------------------------------*/
CmL2caTpConnectAcceptRsp *CmL2caTpConnectAcceptRsp_struct(CsrSchedQid         appHandle,
                                                        CsrBool             accept,
                                                        CsrBtConnId         btConnId,
                                                        psm_t               localPsm,
                                                        CsrBtTpdAddrT       tpdAddrT,
                                                        l2ca_identifier_t   identifier,
                                                        l2ca_mtu_t          localMtu,
                                                        l2ca_mtu_t          transmitMtu,
                                                        CsrUint8            minEncKeySize,
                                                        CsrUint16           credits);

#define CmL2caTpConnectAcceptRspSend(_appHandle,                              \
                                            _accept,                          \
                                            _btConnId,                        \
                                            _localPsm,                        \
                                            _tpdAddrT,                        \
                                            _identifier,                      \
                                            _localMtu,                       \
                                            _transmitMtu,                     \
                                            _minEncKeySize,                   \
                                            _credits,                         \
                                            _flags) {                     \
        CmL2caTpConnectAcceptRsp *msg__; \
        msg__=CmL2caTpConnectAcceptRsp_struct(_appHandle,_accept,_btConnId,_localPsm,_tpdAddrT,_identifier,_localMtu, _transmitMtu, _minEncKeySize,_credits);\
        CsrBtCmPutMessageDownstream(msg__);}

#define CmL2caTpConnectAcceptRspExtSend(_appHandle, _accept, _btConnId, _localPsm, _tpdAddrT, _identifier, _localMtu, _credits)  \
    CmL2caTpConnectAcceptRspSend(_appHandle, _accept, _btConnId, _localPsm, _tpdAddrT, _identifier, _localMtu, 0, 0, _credits, 0)  \

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmContextl2caRegisterFixedCidReqSend
 *  DESCRIPTION
 *      Register fixed cid to handle the related events. Post registering the 
 *      fixed CID, the application shall receive the event 
 *      CM_L2CA_REGISTER_FIXED_CID_CFM.
 *
 *      At the moment the functionality is limited to L2CA_CID_SIGNAL for 
 *      handling CM_L2CAP_PING_IND.
 *
 *  PARAMETERS
 *        appHandle:           Protocol handle
 *        fixedCid:            The local PSM channel
 *        context:             a context value (passed back in the confirm)
 *        config:              L2CAP configuration parameters, see L2CA_CONFIG_T
 *----------------------------------------------------------------------------*/
CmL2caRegisterFixedCidReq *Cml2caRegisterFixedCidReq_struct(CsrSchedQid appHandle,
                                                      CsrUint16 fixedCid,
                                                      CsrUint16 context,
                                                      L2CA_CONFIG_T config);


#define CmContextl2caRegisterFixedCidReqSend(_appHandle, _fixedCid, _context, _config) {            \
                CmL2caRegisterFixedCidReq *msg__;                                                   \
                msg__=Cml2caRegisterFixedCidReq_struct(_appHandle, _fixedCid, _context, _config);   \
                CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmL2caPingReqSend
 *
 *  DESCRIPTION
 *      Send Ping request to remote device. Post completion the application 
 *      shall receive the CM_L2CA_PING_CFM. Application needs to seriliaze the 
 *      ping request.
 *
 *  PARAMETERS
 *        address              Bluetooth device address
 *        appHandle:           application handle to answer to
 *        context              Reserved for future use.
 *        flags                Unused at present
 *        lengthData           Length of data
 *        data                 Pointer to data
 *----------------------------------------------------------------------------*/
CmL2caPingReq *CmL2caPingReq_struct(CsrBdAddr address,
                                                      CsrSchedQid appHandle,
                                                      CsrUint16 context,
                                                      CsrUint32 flags,
                                                      CsrUint16 lengthData,
                                                      CsrUint8 *data);

#define CmL2caPingReqSend(_addr, _appHandle, _cntxt, _flags, _length, _data) {  \
        CmL2caPingReq *msg__; \
        msg__=CmL2caPingReq_struct(_addr, _appHandle, _cntxt, _flags, _length, _data); \
        CsrBtCmPutMessageDownstream( msg__);}

#ifdef CSR_BT_INSTALL_CM_SET_LINK_BEHAVIOR
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmL2caPingRspSend
 *
 *  DESCRIPTION
 *      Send Ping response to remote device. On receiving the event CM_L2CA_PING_IND,
 *      the application shall use this function to send ping response.
 *
 *      The application shall verify for the connection existance and then send
 *      the ping response with the identifier received in CM_L2CA_PING_IND.
 *
 *  PARAMETERS
 *        address              Bluetooth device address
 *        identifier           Identifer from ping indication
 *        lengthData           Length of data
 *        data                 Pointer to data
 *----------------------------------------------------------------------------*/
CmL2caPingRsp *CmL2caPingRsp_struct(CsrBdAddr address,
                                                        CsrUint8 identifier,
                                                        CsrUint16 lengthData,
                                                        CsrUint8 *data);

#define CmL2caPingRspSend(_addr, _identifier, _length, _data) {  \
        CmL2caPingRsp *msg__; \
        msg__=CmL2caPingRsp_struct(_addr, _identifier, _length, _data); \
        CsrBtCmPutMessageDownstream( msg__);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmWriteScHostSupportOverrideReqSend
 *
 *  DESCRIPTION
 *      This API allows applications to override the default behavior of the BR/EDR
 *      secure connections host support for the indicated BD Address.
 *      CM sends CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM back to the application.
 *
 * PARAMETERS
 *      appHandle:      Identity of the calling process
 *      deviceAddr:     Bluetooth address of the device.
 *      overrideAction  SC override action to be carried out.
 *----------------------------------------------------------------------------*/
void CmWriteScHostSupportOverrideReqSend(CsrSchedQid        appHandle,
                                         CsrBtDeviceAddr   *deviceAddr,
                                         CmScOverrideAction overrideAction);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmReadScHostSupportOverrideMaxBdAddrReqSend
 *
 *  DESCRIPTION
 *      This API allows applications to read the maximum number of supported
 *      override BD Address supported in the controller.
 *      CM sends CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM back to the application.
 *
 * PARAMETERS
 *      appHandle:      Identity of the calling process
 *----------------------------------------------------------------------------*/
void CmReadScHostSupportOverrideMaxBdAddrReqSend(CsrSchedQid appHandle);

#ifdef EXCLUDE_CSR_BT_SC_MODULE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmScSetSecurityConfigReq
 *
 *  DESCRIPTION
 *      Set security configuration related options. Refer to CmSecurityConfigOptions
 *      inside csr_bt_cm_prim.h for the available options. e.g. enabling secure
 *      connections, enable selective CTKD etc.
 *
 *      This API doesn't generate any response back to the application.
 *
 *      Notes:
 *            This API is intentionally kept synchronous as the security settings
 *            needs to be propagated to the CM before the CM initialization compeltes.
 *
 *            The value provided by the caller is directly set in the CM, so its 
 *            recommended to set all the required flags in one shot in order 
 *            to avoid flag values getting overridden.
 *
 * PARAMETERS
 *      securityConfigOptions:      Security configuration specified by application.
 *----------------------------------------------------------------------------*/
void CmScSetSecurityConfigReq(CmSecurityConfigOptions securityConfigOptions);
#else
#define CmScSetSecurityConfigReq(_securityConfigOptions)                    \
    do                                                                      \
    {                                                                       \
        CSR_UNUSED(_securityConfigOptions);                                 \
    }                                                                       \
while (0)
#endif /* EXCLUDE_CSR_BT_SC_MODULE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmSmRefreshEncryptionKeyReqSend
 *
 *  DESCRIPTION
 *      This API is used to refresh the encryption on a given link identified
 *      by the provided deviceAddr.
 *
 *      CSR_BT_CM_HCI_REFRESH_ENCRYPTION_KEY_IND will be received by security handler.
 *
 *      CSR_BT_CM_ENCRYPT_CHANGE_IND with encryptType as CSR_BT_CM_ENC_TYPE_KEY_REFRESH 
 *      will be received by modules which has subscribed for
 *      CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE.
 *
 * PARAMETERS
 *      deviceAddr:     Bluetooth Device Address.
 *----------------------------------------------------------------------------*/
#define CmSmRefreshEncryptionKeyReqSend(_deviceAddr)                            \
    do                                                                          \
    {                                                                           \
        CmSmRefreshEncryptionKeyReq *req;                                       \
        req = (CmSmRefreshEncryptionKeyReq *) CsrPmemZalloc(sizeof(*req));      \
        req->type               = CM_SM_REFRESH_ENCRYPTION_KEY_REQ;             \
        req->deviceAddr         = _deviceAddr;                                  \
        CsrBtCmPutMessageDownstream(req);                                       \
    } while (0)

#ifdef INSTALL_CM_LE_PHY_UPDATE_FEATURE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmLeReadPhyReqSend
 *
 *  DESCRIPTION
 *      This API allows applications to read the PHY being used for the connection.
 *
 *      CM_DM_LE_PHY_READ_CFM will be received by calling process as a result
 *      of this command.
 *
 *
 * PARAMETERS
 *      appHandle:      Identity of the calling process.
 *      tpAddr:         Bluetooth device address with transport and address type.
 *----------------------------------------------------------------------------*/
#define CmDmLeReadPhyReqSend(_appHandle,                                        \
                            _tpAddr)                                            \
        do                                                                      \
        {                                                                       \
            CmDmLeReadPhyReq *req;                                              \
            req = (CmDmLeReadPhyReq *) CsrPmemZalloc(sizeof(*req));             \
            req->type                   = CM_DM_LE_READ_PHY_REQ;                \
            req->appHandle              = _appHandle;                           \
            req->tpAddr                 = _tpAddr;                              \
            CsrBtCmPutMessageDownstream(req);                                   \
        } while (0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmLeSetPhyReqSend
 *
 *  DESCRIPTION
 *      This API allows applications to provide PHY preferences for a given 
 *      connection on LE transport. Preferences for physical rate can be set for 
 *      both transmission and receiving side, this would help the controller to
 *      decide upon an appropriate PHY.
 *
 *      CM_DM_LE_PHY_SET_CFM will be received by calling process as a result
 *      of this command.
 *
 *      Note: for the physical rates refer to DM_ULP_PHY_RATE_X in dm_prim.h.
 *
 * PARAMETERS
 *      appHandle:      Identity of the calling process.
 *      tpAddr:         Bluetooth device address with transport and address type.
 *      minTxRate:      Minimum preferred transmission rate.
 *      maxTxRate:      Maximum preferred transmission rate.
 *      minRxRate:      Minimum preferred receiving rate.
 *      maxRxRate:      Maximum preferred receiving rate.
 *      flags:          Flags for additional purpose. RFU. Always set to zero.
 *----------------------------------------------------------------------------*/
#define CmDmLeSetPhyReqSend(_appHandle,                                       \
                            _tpAddr,                                          \
                            _minTxRate,                                       \
                            _maxTxRate,                                       \
                            _minRxRate,                                       \
                            _maxRxRate,                                       \
                            _flags)                                           \
        do                                                                    \
        {                                                                     \
            CmDmLeSetPhyReq *req;                                             \
            req = (CmDmLeSetPhyReq *) CsrPmemZalloc(sizeof(*req));            \
            req->type                   = CM_DM_LE_SET_PHY_REQ;               \
            req->appHandle              = _appHandle;                         \
            req->tpAddr                 = _tpAddr;                            \
            req->phyInfo.minTxRate      = _minTxRate;                         \
            req->phyInfo.maxTxRate      = _maxTxRate;                         \
            req->phyInfo.minRxRate      = _minRxRate;                         \
            req->phyInfo.maxRxRate      = _maxRxRate;                         \
            req->phyInfo.flags          = _flags;                             \
            CsrBtCmPutMessageDownstream(req);                                 \
        } while (0)

/*----------------------------------------------------------------------------*
*  NAME
*      CmDmLeSetDefaultPhyReqSend
*
*  DESCRIPTION
*      This API allows applications to provide PHY preferences for all upcoming 
*      connections after this request is processed successfully. Usage of this API 
*      is same as CmDmLeSetPhyReqSend, but the preference would be applied to all 
*      subsequent connections.
*
*      CM_DM_LE_DEFAULT_PHY_SET_CFM will be received by calling process as a result
*      of this command.
*
*      Note: for the physical rates refer to DM_ULP_PHY_RATE_X in dm_prim.h.
*
* PARAMETERS
*      appHandle:      Identity of the calling process.
*      minTxRate:      Minimum preferred transmission rate.
*      maxTxRate:      Maximum preferred transmission rate.
*      minRxRate:      Minimum preferred receiving rate.
*      maxRxRate:      Maximum preferred receiving rate.
*      flags:          Flags for additional purpose. RFU. Always set to zero.
*----------------------------------------------------------------------------*/
#define CmDmLeSetDefaultPhyReqSend(_appHandle,                                  \
                                   _minTxRate,                                  \
                                   _maxTxRate,                                  \
                                   _minRxRate,                                  \
                                   _maxRxRate,                                  \
                                   _flags)                                      \
            do                                                                  \
            {                                                                   \
                CmDmLeSetDefaultPhyReq *req;                                    \
                req = (CmDmLeSetDefaultPhyReq *) CsrPmemZalloc(sizeof(*req));   \
                req->type                   = CM_DM_LE_SET_DEFAULT_PHY_REQ;     \
                req->appHandle              = _appHandle;                       \
                req->phyInfo.minTxRate      = _minTxRate;                       \
                req->phyInfo.maxTxRate      = _maxTxRate;                       \
                req->phyInfo.minRxRate      = _minRxRate;                       \
                req->phyInfo.maxRxRate      = _maxRxRate;                       \
                req->phyInfo.flags          = _flags;                           \
                CsrBtCmPutMessageDownstream(req);                               \
            } while (0)

#else /* INSTALL_CM_LE_PHY_UPDATE_FEATURE */
#define CmDmLeSetPhyReqSend(_appHandle,                                       \
                            _tpAddr,                                          \
                            _minTxRate,                                       \
                            _maxTxRate,                                       \
                            _minRxRate,                                       \
                            _maxRxRate,                                       \
                            _flags)                                           \
        do                                                                    \
        {                                                                     \
            CSR_UNUSED(_appHandle);                                           \
            CSR_UNUSED(_tpAddr);                                              \
            CSR_UNUSED(_minTxRate);                                           \
            CSR_UNUSED(_maxTxRate);                                           \
            CSR_UNUSED(_minRxRate);                                           \
            CSR_UNUSED(_maxRxRate);                                           \
            CSR_UNUSED(_flags);                                               \
        } while (0)

#define CmDmLeReadPhyReqSend(_appHandle,                                        \
                           _tpAddr)                                            \
       do                                                                    \
       {                                                                     \
           CSR_UNUSED(_appHandle);                                           \
           CSR_UNUSED(_tpAddr);                                              \
       } while (0)

#define CmDmLeSetDefaultPhyReqSend(_appHandle,                                \
                                   _minTxRate,                                \
                                   _maxTxRate,                                \
                                   _minRxRate,                                \
                                   _maxRxRate,                                \
                                   _flags)                                    \
        do                                                                    \
        {                                                                     \
            CSR_UNUSED(_appHandle);                                           \
            CSR_UNUSED(_minTxRate);                                           \
            CSR_UNUSED(_maxTxRate);                                           \
            CSR_UNUSED(_minRxRate);                                           \
            CSR_UNUSED(_maxRxRate);                                           \
            CSR_UNUSED(_flags);                                               \
        } while (0)
#endif /* INSTALL_CM_LE_PHY_UPDATE_FEATURE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmSmGenerateCrossTransKeyRequestRsp
 *
 *  DESCRIPTION
 *      This API is used to respond to CM_SM_GENERATE_CROSS_TRANS_KEY_IND.
 *      value of the flags parameter indicates if the selective CTKD is
 *      enabled or disabled.
 *
 * PARAMETERS
 *      tpAddr:        Remote device's bluetooth address
 *      identifier:    Same value as received in CM_SM_GENERATE_CROSS_TRANS_KEY_IND.
 *      flags:         Flag indicating selective CTKD to 
 *                     enable - CM_SM_FLAG_GENERATE_CROSS_TRANS_KEY_ENABLE
 *                     disable - CM_SM_FLAG_GENERATE_CROSS_TRANS_KEY_DISABLE
 *----------------------------------------------------------------------------*/
#define CmSmGenerateCrossTransKeyRequestRspSend(_tpAddr,                                \
                                                _identifier,                            \
                                                _flags)                                 \
    do                                                                                  \
    {                                                                                   \
        CmSmGenerateCrossTransKeyRequestRsp *rsp;                                       \
        rsp = (CmSmGenerateCrossTransKeyRequestRsp *) CsrPmemZalloc(sizeof(*rsp));      \
        rsp->type       = CM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_RSP;                   \
        rsp->tpAddr     = _tpAddr;                                                      \
        rsp->identifier = _identifier;                                                  \
        rsp->flags      = _flags;                                                       \
        CsrBtCmPutMessageDownstream(rsp);                                               \
    } while (0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmWriteAutoFlushTimeoutReqSend
 *
 *  DESCRIPTION
 *      This API is used to set automatic flush timeout value for the connections
 *      to device identified by deviceAddr.
 *
 *      The flush timeout allows ACL packets to be flushed automatically by the
 *      baseband if the timeout expires and the packet has not been transmitted (for
 *      a more comprehensive explanation please refer to the Bluetooth
 *      specification). By default the flush timeout is set to be infinite which
 *      means that the retransmissions are carried out until physical link loss
 *      occurs.
 *
 *      CM_DM_WRITE_AUTO_FLUSH_TIMEOUT_CFM will be received by the caller as a
 *      result of this API.
 *
 * PARAMETERS
 *      appHandle:  Identity of the calling process.
 *      deviceAddr: Remote device's bluetooth address
 *      flushTo:    A flush timeout value of zero means no automatic flush.
 *                  The allowed range of values for this is 0x0001 - 0x07FF
 *                  where the timeout is calculated in multiples of 625us.
 *                  This gives an allowed range for the flush timeout of
 *                  0.625ms -1279.375ms.
 *----------------------------------------------------------------------------*/
#define CmDmWriteAutoFlushTimeoutReqSend(_appHandle,                                    \
                                         _deviceAddr,                                   \
                                         _flushTimeout)                                 \
    do                                                                                  \
    {                                                                                   \
        CsrBtCmDmWriteAutoFlushTimeoutReq *req;                                         \
        req = (CsrBtCmDmWriteAutoFlushTimeoutReq *) CsrPmemZalloc(sizeof(*req));        \
        req->type           = CSR_BT_CM_DM_WRITE_AUTO_FLUSH_TIMEOUT_REQ;                \
        req->appHandle      = _appHandle;                                               \
        req->deviceAddr     = _deviceAddr;                                              \
        req->flushTo        = _flushTimeout;                                            \
        CsrBtCmPutMessageDownstream(req);                                               \
    } while (0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmChangeConnLinkKeyReqSend
 *
 *  DESCRIPTION
 *      This API is used to change the link key for a connection with the
 *      specified device.
 *
 *      CM_DM_CHANGE_CONNECTION_LINK_KEY_CFM will be received by the calling process
 *      as a result of calling this API.
 *
 * PARAMETERS
 *      appHandle:  Identity of the calling process.
 *      deviceAddr: Bluetooth address of the device with which the connection
 *                  link key needs to change.
 *----------------------------------------------------------------------------*/
#define CmDmChangeConnLinkKeyReqSend(_appHandle,                                        \
                                     _deviceAddr)                                       \
    do                                                                                  \
    {                                                                                   \
        CmDmChangeConnectionLinkKeyReq *req;                                            \
        req = (CmDmChangeConnectionLinkKeyReq *) CsrPmemZalloc(sizeof(*req));           \
        req->type       = CM_DM_CHANGE_CONNECTION_LINK_KEY_REQ;                         \
        req->appHandle  = _appHandle;                                                   \
        req->deviceAddr = _deviceAddr;                                                  \
        CsrBtCmPutMessageDownstream(req);                                               \
    } while (0)

#ifdef INSTALL_CM_READ_INQUIRY_MODE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmReadInquiryModeReqSend
 *
 *  DESCRIPTION
 *      Read the local Inquiry_Mode configuration parameter of the local BR/EDR Controller.
 *      CM sends CM_DM_READ_INQUIRY_MODE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:            Identity of the calling process
 *----------------------------------------------------------------------------*/

#define CmDmReadInquiryModeReqSend(_appHandle)                                      \
    do                                                                              \
    {                                                                               \
        CmDmReadInquiryModeReq *req;                                                \
        req = (CmDmReadInquiryModeReq*) CsrPmemZalloc(sizeof(*req));                \
        req->type = CM_DM_READ_INQUIRY_MODE_REQ;                                    \
        req->appHandle = _appHandle;                                                \
        CsrBtCmPutMessageDownstream(req);                                           \
    } while (0)
#endif /* INSTALL_CM_READ_INQUIRY_MODE */

#ifdef INSTALL_CM_REGISTER_APP_HANDLE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmRegisterApplicationHandle
 *
 *  DESCRIPTION
 *      This API is used to register the application handle with synergy
 *      connection manager. Applications needs to use this API after the
 *      completion of the handover in order to update the application handles
 *      on the new primary.
 *
 *      Note: In order to successfully update the application handles, the CM
 *            handover must be completed before the handover of application
 *            which is trying to update the application handles.
 *
 *      This is a synchronous API and doesn't send any response back
 *      to the application.
 *
 * PARAMETERS
 *      appHandle:  Identity of the calling process.
 *      btConnId:   Connection identifier for which the appHandle needs
 *                  to be registered.
 *      protocol:   Service level protocol identifier
 *                  0 - L2CAP, 1 - RFCOMM, see CmProtocolType for more info.
 *----------------------------------------------------------------------------*/
void CmRegisterApplicationHandle(CsrSchedQid appHandle,
                                 CsrBtConnId btConnId,
                                 CmProtocolType protocol);
#endif /* INSTALL_CM_REGISTER_APP_HANDLE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmLeAddDeviceToWhiteListReqSend
 *
 *  DESCRIPTION
 *      Request to add a device to BLE White List stored in the controller.
 *      CM sends CM_LE_ADD_DEVICE_TO_WHITE_LIST_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:  Identity of the calling process
 *        deviceAddr: Remote device's bluetooth address
 *        addrType:   address type
 *----------------------------------------------------------------------------*/
#define CmLeAddDeviceToWhiteListReqSend(_appHandle,                                     \
                                        _deviceAddr,                                    \
                                        _addrType)                                      \
    do                                                                                  \
    {                                                                                   \
        CmLeAddDeviceToWhiteListReq *req;                                               \
        req = (CmLeAddDeviceToWhiteListReq *) CsrPmemZalloc(sizeof(*req));              \
        req->type       = CM_LE_ADD_DEVICE_TO_WHITE_LIST_REQ;                           \
        req->appHandle  = _appHandle;                                                   \
        req->deviceAddr = _deviceAddr;                                                  \
        req->addrType   = _addrType;                                                    \
        CsrBtCmPutMessageDownstream(req);                                               \
    } while (0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmLeRemoveDeviceFromWhiteListReqSend
 *
 *  DESCRIPTION
 *      Request to remove a device from BLE White List stored in the controller.
 *      CM sends CM_LE_REMOVE_DEVICE_FROM_WHITE_LIST_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:  Identity of the calling process
 *        deviceAddr: Remote device's bluetooth address
 *        addrType:   address type
 *----------------------------------------------------------------------------*/
#define CmLeRemoveDeviceFromWhiteListReqSend(_appHandle,                                \
                                             _deviceAddr,                               \
                                             _addrType)                                 \
    do                                                                                  \
    {                                                                                   \
        CmLeRemoveDeviceFromWhiteListReq *req;                                          \
        req = (CmLeRemoveDeviceFromWhiteListReq *) CsrPmemZalloc(sizeof(*req));         \
        req->type       = CM_LE_REMOVE_DEVICE_FROM_WHITE_LIST_REQ;                      \
        req->appHandle  = _appHandle;                                                   \
        req->deviceAddr = _deviceAddr;                                                  \
        req->addrType   = _addrType;                                                    \
        CsrBtCmPutMessageDownstream(req);                                               \
    } while (0)

#ifdef INSTALL_CM_READ_INQUIRY_TX
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmReadInquiryTxReqSend
 *
 *  DESCRIPTION
 *      Reads the power level used to transmit the FHS(Frequency Hop Sync)
 *      and EIR(Extented Inquiry Response) Data Packets.
 *
 *      CM sends CM_DM_READ_INQUIRY_TX_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:  Identity of the calling process
 *----------------------------------------------------------------------------*/
#define CmDmReadInquiryTxReqSend(_appHandle)                                                \
    do                                                                                      \
    {                                                                                       \
        CmDmReadInquiryTxReq *req = (CmDmReadInquiryTxReq *)CsrPmemZalloc(sizeof(*req));    \
        req->type           = CM_DM_READ_INQUIRY_TX_REQ;                                    \
        req->appHandle      = _appHandle;                                                   \
        CsrBtCmPutMessageDownstream(req);                                                   \
    } while (0)
#endif /* INSTALL_CM_READ_INQUIRY_TX */

#ifdef INSTALL_CM_READ_APT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmReadAuthPayloadTimeoutReqSend
 *
 *  DESCRIPTION
 *      Reads the Authenticated Payload Timeout parameter in the Controller 
 *      on the Specified Connection Handle.
 *
 *      CM sends CM_DM_READ_AUTH_PAYLOAD_TIMEOUT_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle: Identity of the calling process
 *        tpAddrt  : Pointer to Bluetooth Device Address with Transport
 *                   and Address type.
 *----------------------------------------------------------------------------*/
#define CmDmReadAuthPayloadTimeoutReqSend(_appHandle,                                                               \
                                          _tpAddrt)                                                                 \
        do                                                                                                          \
        {                                                                                                           \
            CmDmReadAuthPayloadTimeoutReq *req = (CmDmReadAuthPayloadTimeoutReq *)CsrPmemZalloc(sizeof(*req));      \
            req->type           = CM_DM_READ_AUTH_PAYLOAD_TIMEOUT_REQ;                                              \
            req->appHandle      = _appHandle;                                                                       \
            req->tpAddrt        = *(_tpAddrt);                                                                      \
            CsrBtCmPutMessageDownstream(req);                                                                       \
        } while (0)
#endif /* INSTALL_CM_READ_APT */

#ifdef INSTALL_CM_LE_SET_TRANSMIT_POWER_REPORTING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmLeSetTransmitPowerReportingEnableReqSend
 *
 *  DESCRIPTION
 *      Used to Enable or disable the reporting to the local host of transmit
 *      power level changes in the local and remote controllers for the
 *      Specified Bluetooth Device Address.
 *
 *      CM sends CM_DM_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE_CFM back
 *      to the application.
 *
 *  PARAMETERS
 *        appHandle     : Identity of the calling process
 *        tpAddrt       : Bluetooth Device Address with Transport and
 *                        Address type.
 *        localEnable   : Enables/Disables local transmit power reports.
 *        remoteEnable  : Enables/Disables remote transmit power reports.
 *----------------------------------------------------------------------------*/
#define CmDmLeSetTransmitPowerReportingEnableReqSend(_appHandle,                                                                          \
                                                     _tpAddrt,                                                                            \
                                                     _localEnable,                                                                        \
                                                     _remoteEnable)                                                                       \
        do                                                                                                                                \
        {                                                                                                                                 \
            CmDmLeSetTransmitPowerReportingEnableReq *req = (CmDmLeSetTransmitPowerReportingEnableReq *)CsrPmemZalloc(sizeof(*req));      \
            req->type           = CM_DM_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE_REQ;                                                       \
            req->appHandle      = _appHandle;                                                                                             \
            req->tpAddrt        = _tpAddrt;                                                                                               \
            req->localEnable    = _localEnable;                                                                                           \
            req->remoteEnable   = _remoteEnable;                                                                                          \
            CsrBtCmPutMessageDownstream(req);                                                                                             \
        } while (0)
#endif /* INSTALL_CM_LE_SET_TRANSMIT_POWER_REPORTING */


#ifdef INSTALL_CM_LE_PATH_LOSS_REPORTING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmLeSetPathLossReportingParametersReqSend
 *
 *  DESCRIPTION
 *      Used to set the path loss threshold reporting parameters for the
 *      connection identified by the Bluetooth Device Address.
 *
 *      CM sends CM_DM_LE_SET_PATH_LOSS_REPORTING_PARAMETERS_CFM back to the
 *      application.
 *
 *  PARAMETERS
 *        appHandle         : Identity of the calling process
 *        tpAddrt           : Bluetooth Device Address with Transport and
 *                            Address type
 *        highThreshold     : The high threshold for the path loss in dB
 *        highHysteresis    : Hysteresis value for the high threshold
 *        lowThreshold      : The low threshold for the path loss in dB
 *        lowHysteresis     : Hysteresis value for the low threshold
 *        minTimeSpent      : Minimum time spent in number of connection events
 *                            to be obsereved once the path crossed the
 *                            theshold before an event is generated
 *----------------------------------------------------------------------------*/

 #define CmDmLeSetPathLossReportingParametersReqSend(_appHandle,                                                                          \
                                                     _tpAddrt,                                                                            \
                                                     _highThreshold,                                                                      \
                                                     _highHysteresis,                                                                     \
                                                     _lowThreshold,                                                                       \
                                                     _lowHysteresis,                                                                      \
                                                     _minTimeSpent)                                                                       \
        do                                                                                                                                \
        {                                                                                                                                 \
            CmDmLeSetPathLossReportingParametersReq *req = (CmDmLeSetPathLossReportingParametersReq *)CsrPmemZalloc(sizeof(*req));        \
            req->type = CM_DM_LE_SET_PATH_LOSS_REPORTING_PARAMETERS_REQ;                                                                  \
            req->appHandle = _appHandle;                                                                                                  \
            req->tpAddrt = _tpAddrt;                                                                                                      \
            req->highThreshold = _highThreshold;                                                                                          \
            req->highHysteresis = _highHysteresis;                                                                                        \
            req->lowThreshold = _lowThreshold;                                                                                            \
            req->lowHysteresis = _lowHysteresis;                                                                                          \
            req->minTimeSpent = _minTimeSpent;                                                                                            \
            CsrBtCmPutMessageDownstream(req);                                                                                             \
        } while (0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmLeSetPathLossReportingEnableReqSend
 *
 *  DESCRIPTION
 *      Used to enable or disable the reporting to the local host of LE Path
 *      Loss for a specific peer device identified by its Bluetooth Device
 *      Address.
 *
 *      CM sends CM_DM_LE_SET_PATH_LOSS_REPORTING_ENABLE_CFM back to the
 *      application.
 *
 *  PARAMETERS
 *        appHandle     : Identity of the calling process
 *        tpAddrt       : Bluetooth Device Address with Transport and
 *                        Address type
 *        enable        : Enables/Disables LE Power Loss Reporting.
 *----------------------------------------------------------------------------*/
#define CmDmLeSetPathLossReportingEnableReqSend(_appHandle,                                                                               \
                                                _tpAddrt,                                                                                 \
                                                _enable)                                                                                  \
        do                                                                                                                                \
        {                                                                                                                                 \
            CmDmLeSetPathLossReportingEnableReq *req = (CmDmLeSetPathLossReportingEnableReq *)CsrPmemZalloc(sizeof(*req));                \
            req->type       = CM_DM_LE_SET_PATH_LOSS_REPORTING_ENABLE_REQ;                                                                \
            req->appHandle  = _appHandle;                                                                                                 \
            req->tpAddrt    = _tpAddrt;                                                                                                   \
            req->enable     = _enable;                                                                                                    \
            CsrBtCmPutMessageDownstream(req);                                                                                             \
        } while (0)

#endif /* INSTALL_CM_LE_PATH_LOSS_REPORTING */


#ifdef INSTALL_CM_KEY_REQUEST_INDICATION
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmSmKeyRequestRspSend
 *
 *  DESCRIPTION
 *      Response to CM_SM_KEY_REQUEST_IND. 
 *
 *  PARAMETERS
 *      tpAddrt                 Remote device address
 *      secRequirements         Security Requirements
 *      keyType                 (DM_SM_KEY_TYPE_T) Key Type
 *      keyAvailable            True: Key is available, False: Key is not available
 *      key                     (DM_SM_UKEY_T) Union of pointers to key structures
 *----------------------------------------------------------------------------*/
#define CmSmKeyRequestRspSend(_tpAddrt,                                              \
                              _secRequirements,                                      \
                              _keyType,                                              \
                              _keyAvailable,                                         \
                              _key)                                                  \
    do                                                                               \
    {                                                                                \
        CmSmKeyRequestRsp *prim = (CmSmKeyRequestRsp*)CsrPmemZalloc(sizeof(*prim));  \
        prim->type              = CM_SM_KEY_REQUEST_RSP;                             \
        prim->tpAddrt           = _tpAddrt;                                          \
        prim->secRequirements   = _secRequirements;                                  \
        prim->keyAvailable      = _keyAvailable;                                     \
        prim->keyType           = _keyType;                                          \
        prim->key               = _key;                                              \
        CsrBtCmPutMessageDownstream(prim);                                           \
    } while (0)
#endif /* INSTALL_CM_KEY_REQUEST_INDICATION */

#ifdef INSTALL_CM_READ_EIR_DATA
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmReadEIRDataReqSend
 *
 *  DESCRIPTION
 *      Read the extended inquiry response to be sent during the 
 *      extended inquiry response procedure.
 *
 *      CM sends CM_DM_READ_EIR_DATA_CFM back to the
 *      application.
 *
 *  PARAMETERS
 *        appHandle:  Identity of the calling process
 *----------------------------------------------------------------------------*/
#define CmDmReadEIRDataReqSend(_appHandle)                                              \
    do                                                                                  \
    {                                                                                   \
        CmDmReadEIRDataReq *req = (CmDmReadEIRDataReq *) CsrPmemZalloc(sizeof(*req));   \
        req->type               = CM_DM_READ_EIR_DATA_REQ;                              \
        req->appHandle          = _appHandle;                                           \
        CsrBtCmPutMessageDownstream(req);                                               \
    } while(0)
#endif /* INSTALL_CM_READ_EIR_DATA */

#ifdef INSTALL_CM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmLeReadRemoteTransmitPowerLevelReqSend
 *
 *  DESCRIPTION
 *      Reads the transmit power levels used by the remote
 *      Controller on the Specified Bluetooth Device Address.
 *
 *      CM sends CM_DM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL_CFM back to the 
 *      application.
 *
 *  PARAMETERS
 *        appHandle: Identity of the calling process
 *        tpAddrt  : Bluetooth Device Address with Transport
 *                   and Address type.
 *        phy      : Indicates whether the link is on 1M, 2M or Coded PHY
 *----------------------------------------------------------------------------*/
#define CmDmLeReadRemoteTransmitPowerLevelReqSend(_appHandle,                                                                          \
                                                  _tpAddrt,                                                                            \
                                                  _phy)                                                                                \
        do                                                                                                                             \
        {                                                                                                                              \
            CmDmLeReadRemoteTransmitPowerLevelReq *req = (CmDmLeReadRemoteTransmitPowerLevelReq *)CsrPmemZalloc(sizeof(*req));         \
            req->type           = CM_DM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL_REQ;                                                       \
            req->appHandle      = _appHandle;                                                                                          \
            req->tpAddrt        = _tpAddrt;                                                                                            \
            req->phy            = _phy;                                                                                                \
            CsrBtCmPutMessageDownstream(req);                                                                                          \
        } while (0)
#endif /* INSTALL_CM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL */

#ifdef INSTALL_CM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmLeEnhancedReadTransmitPowerLevelReqSend
 *
 *  DESCRIPTION
 *      Reads the Current and Maximum transmit power levels used by the local
 *      Controller of the Specified Bluetooth Address.
 *
 *      CM sends CM_DM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL_CFM back to the
 *      application.
 *
 *  PARAMETERS
 *        appHandle: Identity of the calling process
 *        tpAddrt  : Bluetooth Device Address with Transport
 *                   and Address type.
 *        phy      : Indicates whether the link is on 1M, 2M or Coded PHY
 *----------------------------------------------------------------------------*/
#define CmDmLeEnhancedReadTransmitPowerLevelReqSend(_appHandle,                                                                         \
                                                    _tpAddrt,                                                                           \
                                                    _phy)                                                                               \
        do                                                                                                                              \
        {                                                                                                                               \
            CmDmLeEnhancedReadTransmitPowerLevelReq *req = (CmDmLeEnhancedReadTransmitPowerLevelReq *)CsrPmemZalloc(sizeof(*req));      \
            req->type           = CM_DM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL_REQ;                                                      \
            req->appHandle      = _appHandle;                                                                                           \
            req->tpAddrt        = _tpAddrt;                                                                                             \
            req->phy            = _phy;                                                                                                 \
            CsrBtCmPutMessageDownstream(req);                                                                                           \
        } while (0)
#endif /* INSTALL_CM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL */

#ifdef INSTALL_CM_WRITE_INQUIRY_MODE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmWriteInquiryModeReqSend
 *
 *  DESCRIPTION
 *      Writes the Inquiry Mode Configuration Parameter of the local
 *      BR/EDR Controller
 *
 *      CM sends CM_DM_WRITE_INQUIRY_MODE_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle: Identity of the calling process
 *        mode     : Indicates which mode, the Inquiry Mode has to be set.
 *----------------------------------------------------------------------------*/
#define CmDmWriteInquiryModeReqSend(_appHandle,                                                         \
                                    _mode)                                                              \
        do                                                                                              \
        {                                                                                               \
            CmDmWriteInquiryModeReq *req = (CmDmWriteInquiryModeReq *)CsrPmemZalloc(sizeof(*req));      \
            req->type           = CM_DM_WRITE_INQUIRY_MODE_REQ;                                         \
            req->appHandle      = _appHandle;                                                           \
            req->mode           = _mode;                                                                \
            CsrBtCmPutMessageDownstream(req);                                                           \
        } while (0)
#endif /* INSTALL_CM_WRITE_INQUIRY_MODE */

#ifndef CSR_BT_EXCLUDE_HCI_READ_CONN_ACCEPT_TIMEOUT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmReadConnAcceptTimeoutReqSend
 *
 *  DESCRIPTION
 *      Reads the connect accept timeout of the local
 *      Controller. 
 *
 *      CM sends CM_DM_READ_CONN_ACCEPT_TIMEOUT_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle: Identity of the calling process
 *----------------------------------------------------------------------------*/
#define CmDmReadConnAcceptTimeoutReqSend(_appHandle)                                                    \
        do                                                                                              \
        {                                                                                               \
            CmDmReadConnAcceptTimeoutReq *req = (CmDmReadConnAcceptTimeoutReq *)CsrPmemZalloc(sizeof(*req));      \
            req->type           = CM_DM_READ_CONN_ACCEPT_TIMEOUT_REQ;                                        \
            req->appHandle      = _appHandle;                                                           \
            CsrBtCmPutMessageDownstream(req);                                                           \
        } while (0)
#endif /* !CSR_BT_EXCLUDE_HCI_READ_CONN_ACCEPT_TIMEOUT */

#ifndef CSR_BT_EXCLUDE_HCI_WRITE_CONN_ACCEPT_TIMEOUT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmWriteConnAcceptTimeoutReqSend
 *
 *  DESCRIPTION
 *      Writes the connect accept timeout to the local Controller.
 *      This configuration parameter allows the Controller to automatically 
 *      deny a incoming connection request after a specified time period has 
 *      occurred and the new connection is not accepted. 
 *
 *      CM sends CM_DM_WRITE_CONN_ACCEPT_TIMEOUT_CFM back to the application.
 *
 *  PARAMETERS
 *        appHandle:         Identity of the calling process
 *        connAcceptTimeout: The connection timeout value to be set in the range 
 *                           of 0x1 (0.625 ms) to 0xB540(29s). Default is 0x1F40 (5s)
 *----------------------------------------------------------------------------*/
#define CmDmWriteConnAcceptTimeoutReqSend(_appHandle,                                                    \
                                    _connAcceptTimeout)                                                 \
        do                                                                                              \
        {                                                                                               \
            CmDmWriteConnAcceptTimeoutReq *req = (CmDmWriteConnAcceptTimeoutReq*)CsrPmemZalloc(sizeof(*req));      \
            req->type           = CM_DM_WRITE_CONN_ACCEPT_TIMEOUT_REQ;                                        \
            req->appHandle      = _appHandle;                                                           \
            req->connAcceptTimeout      = _connAcceptTimeout;                                                           \
            CsrBtCmPutMessageDownstream(req);                                                           \
        } while (0)
#endif /* !CSR_BT_EXCLUDE_HCI_WRITE_CONN_ACCEPT_TIMEOUT */

#ifdef EXCLUDE_CSR_BT_SC_MODULE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmRemoveDeviceKeyReqSend
 *
 *  DESCRIPTION
 *      This API is used to remove requested key information from the trusted device
 *      list, indicated by keyType parameter.
 *
 *      CM sends CM_DM_REMOVE_DEVICE_KEY_CFM as a response to this API.
 *
 *      Note: If only the requested key is present for the device, this API
 *            deletes only key information, complete device is not removed.
 *            API CmScDmRemoveDeviceReqSend can be used to remove complete device.
 *
 *            Though it takes keyType as a parameter, currently this API is
 *            intended to remove LE keys information only. Trying to remove
 *            BREDR key information will yield confirmation with result 
 *            CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE.
 *
 *  PARAMETERS
 *        appHandle:    Identity of the calling process
 *        deviceAddr:   Bluetooth device address for which the key information
 *                      needs to be removed.
 *        addressType:  address type of the bluetooth device address.
 *        keyType:      Type of key to be removed.
 *----------------------------------------------------------------------------*/
#define CmDmRemoveDeviceKeyReqSend(_appHandle,                                                      \
                                   _deviceAddr,                                                     \
                                   _addressType,                                                    \
                                   _keyType)                                                        \
    do                                                                                              \
    {                                                                                               \
        CmDmRemoveDeviceKeyReq *req = (CmDmRemoveDeviceKeyReq *)CsrPmemZalloc(sizeof(*req));        \
        req->type                   = CM_DM_REMOVE_DEVICE_KEY_REQ;                                  \
        req->appHandle              = _appHandle;                                                   \
        req->deviceAddr             = _deviceAddr;                                                  \
        req->addressType            = _addressType;                                                 \
        req->keyType                = _keyType;                                                     \
        CsrBtCmPutMessageDownstream(req);                                                           \
    } while (0)

/* ------------------------------------------------------------------------------------------------*
 *  NAME
 *      CmDmRemoveDeviceOptionsReqSend
 *
 *  DESCRIPTION
 *      This API is used to remove authenticated devices from the paired device list. This API
 *      can be used either to delete individual authenticated device or all authenticated devices.
 *      Application can further cofigure to remove devices from either Bluestack SM DB or both 
 *      PS and Bluestack SM DB depending upon the options specified. 
 *      Application will receive below confirmations depending upon the deviceAddr and option 
 *      configured.
 *
 *      1) Deleting individual authenticated device
 *         In the default configuration, device will be removed from both PS and Bluestack SM DB.
 *         CM sends security indication CSR_BT_CM_SECURITY_EVENT_IND with event as 
 *         CSR_BT_CM_SECURITY_EVENT_DEBOND If application has subscribed for
 *         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SECURITY_EVENT_IND once the non-priority
 *         device gets removed from PS.
 *         CM sends CM_DM_REMOVE_DEVICE_OPTIONS_CFM once the procedure completes.
 *
 *      2) Deleting all authenticated devices
 *         In the default configuration, device will be removed from both PS and Bluestack SM DB.
 *         For each device being removed from PS, CM sends security indication
 *         CSR_BT_CM_SECURITY_EVENT_IND with event as CSR_BT_CM_SECURITY_EVENT_DEBOND if application 
 *         has subscribed for CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SECURITY_EVENT_IND.
 *         Once the procedure completes, CM sends a single CM_DM_REMOVE_DEVICE_OPTIONS_CFM to the 
 *         application with the deviceAddr set to all 0s and  address type as CSR_BT_ADDR_INVALID.
 *         ResultCode will be set to CSR_BT_RESULT_CODE_CM_SUCCESS.
 *
 *      3) Deleting individual authenticated device from Bluestack SM DB only
 *         CM sends CM_DM_REMOVE_DEVICE_OPTIONS_CFM as a response once the procedure completes.
 *
 *      4) Deleting all authenticated devices from Bluestack SM DB only
 *         CM sends CM_DM_REMOVE_DEVICE_OPTIONS_CFM with the deviceAddr set to all 0s and address type
 *         as CSR_BT_ADDR_INVALID once the procedure completes.
 *
 *
 *      Note: For (1) and (2) the devices which are marked priority will not get deleted. In order to 
 *            delete such devices, the priority needs to be reset to FALSE before calling this API.
 *            If the device to be deleted doesn't exists, CM_DM_REMOVE_DEVICE_OPTIONS_CFM will be sent to 
 *            the application with the appropriate resultCode.
 *
 *  PARAMETERS
 *      appHandle:    Identity of the calling process
 *      deviceAddr:   Device address to be removed. All zeros shall be passed to remove all authenticated 
 *                    devices.
 *      addressType:  Address type of the device. Invalid address type CSR_BT_ADDR_INVALID shall be passed 
 *                    for removing all devices.
 *      option:       Remove device configuration. Refer CmDeviceRemovalOption for more details.
 * -----------------------------------------------------------------------------------------------------*/

#define CmDmRemoveDeviceOptionsReqSend(_appHandle,                                                   \
                                       _deviceAddr,                                                  \
                                       _addressType,                                                 \
                                       _option)                                                      \
    do                                                                                               \
    {                                                                                                \
        CmDmRemoveDeviceOptionsReq *req = (CmDmRemoveDeviceOptionsReq *)CsrPmemZalloc(sizeof(*req)); \
        req->type                   = CM_DM_REMOVE_DEVICE_OPTIONS_REQ;                               \
        req->appHandle              = _appHandle;                                                    \
        req->deviceAddr             = _deviceAddr;                                                   \
        req->addressType            = _addressType;                                                  \
        req->option                 = _option;                                                       \
        CsrBtCmPutMessageDownstream(req);                                                            \
    } while(0)

#else /* !EXCLUDE_CSR_BT_SC_MODULE */
#define CmDmRemoveDeviceKeyReqSend(_appHandle,                                                      \
                                   _deviceAddr,                                                     \
                                   _addressType,                                                    \
                                   _keyType)                                                        \
    do                                                                                              \
    {                                                                                               \
        CSR_UNUSED(_appHandle);                                                                     \
        CSR_UNUSED(_deviceAddr);                                                                    \
        CSR_UNUSED(_addressType);                                                                   \
        CSR_UNUSED(_keyType);                                                                       \
    } while (0)

#define CmDmRemoveDeviceOptionsReqSend(_appHandle,                                                  \
                                       _deviceAddr,                                                 \
                                       _addressType,                                                \
                                       _option)                                                     \
        do                                                                                          \
        {                                                                                           \
            CSR_UNUSED(_appHandle);                                                                 \
            CSR_UNUSED(_deviceAddr);                                                                \
            CSR_UNUSED(_addressType);                                                               \
            CSR_UNUSED(_option);                                                                    \
        } while (0)

#endif /* EXCLUDE_CSR_BT_SC_MODULE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caRegisterReqSend
 *      CsrBtCmContextl2caRegisterReqSend
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:           Protocol handle
 *        localPsm:            The local PSM channel
 *        mode_mask:           L2CAP flow control modes bitmask
 *        flags:               Special L2CAP registration flags
 *        context:             a context value (passed back in the confirm)
 *        optionsMask:         options associated with the registration, see CsrBtL2caRegisterOptions
 *----------------------------------------------------------------------------*/
CsrBtCmL2caRegisterReq *CsrBtCml2caRegisterReq_struct(CsrSchedQid theAppHandle,
                                                      psm_t theLocalPsm,
                                                      CsrUint16 mode_mask,
                                                      CsrUint16 flags,
                                                      CsrUint16 context,
                                                      CsrBtL2caRegisterOptions optionsMask);

#define CsrBtCmContextl2caRegisterExtReqSend(_appHandle,_localPsm,_mode_mask,_flags,_context,_optionsMask) {            \
                CsrBtCmL2caRegisterReq *msg__;                                                                          \
                msg__=CsrBtCml2caRegisterReq_struct(_appHandle,_localPsm, _mode_mask, _flags,_context,_optionsMask);    \
                CsrBtCmPutMessageDownstream(msg__);}

#define CsrBtCmContextl2caRegisterReqSend(_appHandle,_localPsm,_mode_mask,_flags,_context)  \
    CsrBtCmContextl2caRegisterExtReqSend(_appHandle,_localPsm,_mode_mask,_flags,_context, CM_L2CA_REGISTER_OPTION_UNUSED)

#define CsrBtCml2caRegisterReqSend(_appHandle,_localPsm, _mode_mask, _flags) \
    CsrBtCmContextl2caRegisterReqSend(_appHandle,_localPsm,_mode_mask,_flags,CSR_BT_CM_CONTEXT_UNUSED)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caUnRegisterReqSend
 *
 *  DESCRIPTION
 *      This API is used for unregistering PSM which was registered using 
 *      CmL2caRegisterReqSend API.
 *
 *      CM_L2CA_UNREGISTER_CFM will be received by the calling process as a
 *      result of this API.
 *
 *  PARAMETERS
 *        phandle:             Identity of the calling process.
 *        localPsm:            The local PSM channel to be unregistered.
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_L2CA_UNREGISTER
CsrBtCmL2caUnregisterReq *CsrBtCml2caUnRegisterReq_struct(CsrSchedQid theAppHandle, psm_t theLocalPsm);

#define CsrBtCml2caUnRegisterReqSend(_appHandle,_localPsm) {            \
        CsrBtCmL2caUnregisterReq *msg__;                                \
        msg__=CsrBtCml2caUnRegisterReq_struct(_appHandle,_localPsm);    \
        CsrBtCmPutMessageDownstream(msg__);}
#else
#define CsrBtCml2caUnRegisterReqSend(_appHandle,_localPsm)              \
        do                                                              \
        {                                                               \
            CSR_UNUSED(_appHandle);                                     \
            CSR_UNUSED(_localPsm);                                      \
        } while (0)
#endif /* CSR_BT_INSTALL_CM_PRI_L2CA_UNREGISTER */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caDisconnectReqSend
 *      CsrBtCmContextl2caDisconnectReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        btConnId:                Channel identifier
 *----------------------------------------------------------------------------*/
CsrBtCmL2caDisconnectReq *CsrBtCml2caDisconnectReq_struct(CsrBtConnId btConnId,
                                                          CsrUint16   context);

#define CsrBtCmContextl2caDisconnectReqSend(_btConnId,_context) {       \
        CsrBtCmL2caDisconnectReq *msg__;                                \
        msg__=CsrBtCml2caDisconnectReq_struct(_btConnId,_context);      \
        CsrBtCmPutMessageDownstream(msg__);}

#define CsrBtCml2caDisconnectReqSend(_btConnId) CsrBtCmContextl2caDisconnectReqSend(_btConnId,CSR_BT_CM_CONTEXT_UNUSED)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caDataReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        btConnId:                Channel identifier
 *        payloadLength:        Length of the payload
 *        *payload:            Pointer to the data
 *        context:             a context value (passed back in the confirm)
 *----------------------------------------------------------------------------*/
CsrBtCmL2caDataReq *CsrBtCml2caDataReq_struct(CsrBtConnId btConnId,
                                              CsrUint16    payloadLength,
                                              void        *payload,
                                              CsrUint16    context);

#ifdef CSR_STREAMS_ENABLE
#define CsrBtCml2caDataReqSend(_btConnId,_payloadLength,_payload,_context) \
    CsrStreamsDataSend(CM_GET_UINT16ID_FROM_BTCONN_ID(_btConnId), L2CAP_ID,_payloadLength, _payload)
#else
#define CsrBtCml2caDataReqSend(_btConnId,_payloadLength,_payload,_context) {         \
        CsrBtCmL2caDataReq *msg__;                                                   \
        msg__=CsrBtCml2caDataReq_struct(_btConnId,_payloadLength,_payload,_context); \
        CsrBtCmPutMessageDownstream(msg__); }
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caDataResSend
 *
 *  DESCRIPTION
 *      Acknowledge data indication
 *
 *  PARAMETERS
 *        cid:                Channel identifier
 *----------------------------------------------------------------------------*/
CsrBtCmL2caDataRes *CsrBtCmL2caDataRes_struct(CsrBtConnId btConnId);
#ifdef CSR_STREAMS_ENABLE
#define CsrBtCmL2caDataResSend(_btConnId)
#else
#define CsrBtCmL2caDataResSend(_btConnId) {             \
        CsrBtCmL2caDataRes *msg__;                      \
        msg__=CsrBtCmL2caDataRes_struct(_btConnId);     \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmL2caDisconnectRspSend
 *
 *  DESCRIPTION
 *      When the flag CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP is enabled, application
 *      uses this API to respond to L2CA_DISCONNECT_IND. When the flag is disabled,
 *      synergy CM responds to L2CA_DISCONNECT_IND and sends notification to
 *      application.
 *
 * PARAMETERS
 *      l2caSignalId: l2cap signal identifier received in L2CA_DISCONNECT_IND.
 *      btConnId:     connection ID for which L2CA_DISCONNECT_IND is received.
 *----------------------------------------------------------------------------*/
#define CsrBtCmL2caDisconnectRspSend(_l2caSignalId, _btConnId)                  \
    do                                                                          \
    {                                                                           \
        CsrBtCmL2caDisconnectRsp *rsp;                                          \
        rsp = (CsrBtCmL2caDisconnectRsp *) CsrPmemZalloc(sizeof(*rsp));         \
        rsp->type       = CSR_BT_CM_L2CA_DISCONNECT_RSP;                        \
        rsp->identifier = _l2caSignalId;                                        \
        rsp->btConnId   = _btConnId;                                            \
        CsrBtCmPutMessageDownstream(rsp);                                       \
    } while (0)
#else
#define CsrBtCmL2caDisconnectRspSend(_l2caSignalId, _btConnId)                  \
    do                                                                          \
    {                                                                           \
        CSR_UNUSED(_l2caSignalId);                                              \
        CSR_UNUSED(_btConnId);                                                  \
    } while (0)
#endif /* CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP */

#ifdef INSTALL_CM_SM_CONFIG
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmSmConfigReqSend
 *
 *  DESCRIPTION
 *      This API is used to configure security manager to perform specific
 *      actions. Refer to CmSmConfigMask for more information.
 *
 *      CM_SM_CONFIG_CFM will be received by the calling process as a
 *      result of this API.
 *
 * PARAMETERS
 *      l2caSignalId: l2cap signal identifier received in L2CA_DISCONNECT_IND.
 *      btConnId:     connection ID for which L2CA_DISCONNECT_IND is received.
 *----------------------------------------------------------------------------*/
#define CmSmConfigReqSend(_appHandle, _configMask, _length, _params)                    \
    do                                                                                  \
    {                                                                                   \
        CmSmConfigReq *req = (CmSmConfigReq *) CsrPmemZalloc(sizeof(*req));  \
        req->type           = CM_SM_CONFIG_REQ;                                         \
        req->appHandle      = _appHandle;                                               \
        req->configMask     = _configMask;                                              \
        req->length         = _length;                                                  \
        req->params         = _params;                                                  \
        CsrBtCmPutMessageDownstream(req);                                               \
    } while (0)
#endif /* INSTALL_CM_SM_CONFIG */

#ifdef __cplusplus
}

#endif

#endif
