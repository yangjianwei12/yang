/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_profiles.h"
#include "csr_pmem.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_l2cap_conftab.h"

/* Common put_message functions to reduce code size */
void CsrBtCmMsgTransport(void* __msg)
{
    CsrMsgTransport(CSR_BT_CM_IFACEQUEUE,CSR_BT_CM_PRIM,__msg);
}

void CsrBtCmPutMessageDownstream(void* __msg)
{
    CsrSchedMessagePut(CSR_BT_CM_IFACEQUEUE,CSR_BT_CM_PRIM,__msg);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtcmWriteLinkSuperVisionTimeoutReqSend
 *
 *  DESCRIPTION
 *      This command will write the value of the link supervision timeout
 *        parameter. It is used to monitor link loss. Value of 0x0000 disables it.
 *        The timeout value N range from 0x0001 - 0xffff. In seconds it means from
 *        N*0.625ms = 0,625ms - 40.9 seconds.
 *
 *  PARAMETERS
 *      phandle:         protocol handle
 *      CsrBtDeviceAddr:    theDeviceAddr
 *      CsrUint16:        Link supervision timeout value.
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_WRITE_LINK_SUPERVISION_TIMEOUT
CsrBtCmWriteLinkSupervTimeoutReq *CsrBtCmWriteLinkSuperVisionTimeoutReq_struct(CsrSchedQid       thePhandle,
                                                                               CsrBtDeviceAddr    theDeviceAddr,
                                                                               CsrUint16        timeout)
{
    CsrBtCmWriteLinkSupervTimeoutReq *prim;

    prim = (CsrBtCmWriteLinkSupervTimeoutReq*) CsrPmemAlloc(sizeof(CsrBtCmWriteLinkSupervTimeoutReq));
    prim->type = CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_REQ;
    prim->phandle = thePhandle;
    prim->deviceAddr = theDeviceAddr;
    prim->timeout = timeout;
    return(prim);
}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadLocalBdAddrReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      phandle:            protocol handle
 *----------------------------------------------------------------------------*/
CsrBtCmReadLocalBdAddrReq *CsrBtCmReadLocalBdAddrReq_struct(CsrSchedQid    thePhandle)
{
    CsrBtCmReadLocalBdAddrReq *prim;

    prim = (CsrBtCmReadLocalBdAddrReq*) CsrPmemAlloc(sizeof(CsrBtCmReadLocalBdAddrReq));
    prim->type = CSR_BT_CM_READ_LOCAL_BD_ADDR_REQ;
    prim->phandle = thePhandle;
    return(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadLocalNameReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      phandle:            protocol handle
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_READ_LOCAL_NAME 
CsrBtCmReadLocalNameReq *CsrBtCmReadLocalNameReq_struct(CsrSchedQid    thePhandle)
{
    CsrBtCmReadLocalNameReq *prim;

    prim = (CsrBtCmReadLocalNameReq*) CsrPmemAlloc(sizeof(CsrBtCmReadLocalNameReq));
    prim->type = CSR_BT_CM_READ_LOCAL_NAME_REQ;
    prim->phandle = thePhandle;
    return(prim);
}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadRemoteNameReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      phandle:               protocol handle
 *      deviceAddr:            BT address of the device to read remote name
 *----------------------------------------------------------------------------*/
void CsrBtCmReadRemoteNameReqSend(CsrSchedQid       thePhandle,
                                  CsrBtDeviceAddr    theDeviceAddr)
{
    CsrBtCmReadRemoteNameReq *prim;

    prim = (CsrBtCmReadRemoteNameReq*) CsrPmemAlloc(sizeof(CsrBtCmReadRemoteNameReq));
    prim->type = CSR_BT_CM_READ_REMOTE_NAME_REQ;
    prim->phandle = thePhandle;
    prim->deviceAddr = theDeviceAddr;
    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadRemoteVersionReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      phandle:               protocol handle
 *      deviceAddr:            BT address of the device to read remote version
 *----------------------------------------------------------------------------*/
void CsrBtCmReadRemoteVersionReqSend(CsrSchedQid       thePhandle,
                                     CsrBtDeviceAddr    theDeviceAddr,
                                     CsrBtAddressType   theAddressType,
                                     CsrBtTransportType theTransportType)
{
    CsrBtCmReadRemoteVersionReq *prim;

    prim = (CsrBtCmReadRemoteVersionReq*) CsrPmemAlloc(sizeof(CsrBtCmReadRemoteVersionReq));
    prim->type = CSR_BT_CM_READ_REMOTE_VERSION_REQ;
    prim->appHandle = thePhandle;
    prim->deviceAddr = theDeviceAddr;
    prim->addressType = theAddressType;
    prim->transportType = theTransportType;
    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadTxPowerLevelReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      phandle:            protocol handle
 *      deviceAddr:         BT address of the device to read remote name
 *      addressType:        Address type of 'deviceAddr' (see CSR_BT_ADDR_ defines
 *                          in csr_bt_addr.h)
 *      transportType:      The transport type (see CSR_BT_TRANSPORT_ defines
 *                          in csr_bt_addr.h).
 *      levelType:          The maximum power level as defined in the Bluetooth HCI
 *                          specification.
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_READ_TX_POWER_LEVEL
CsrBtCmReadTxPowerLevelReq *CsrBtCmReadTxPowerLevelReq_struct(CsrSchedQid thePhandle,
                                                              CsrBtDeviceAddr theDeviceAddr,
                                                              CsrBtAddressType theAddressType,
                                                              CsrBtTransportType theTransportType,
                                                              CsrUint8 theLevelType)
{
    CsrBtCmReadTxPowerLevelReq *prim;

    prim = (CsrBtCmReadTxPowerLevelReq*) CsrPmemAlloc(sizeof(CsrBtCmReadTxPowerLevelReq));
    prim->type          = CSR_BT_CM_READ_TX_POWER_LEVEL_REQ;
    prim->appHandle     = thePhandle;
    prim->deviceAddr    = theDeviceAddr;
    prim->addressType   = theAddressType;
    prim->transportType = theTransportType;
    prim->levelType     = theLevelType;
    return(prim);
}
#endif

#ifdef CSR_BT_INSTALL_CM_SET_LINK_BEHAVIOR

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetLinkBehaviorReqSend
 *
 *  DESCRIPTION
 *      .....
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
                                   CsrBool l2capRetry)
{
    CmSetLinkBehaviorMask flags=CM_SET_LINK_BEHAVIOR_NONE;
    if(l2capRetry)
        flags = CM_SET_LINK_BEHAVIOR_L2CAP_RETRY_ON;

    CmSetLinkBehaviorReqSendExt(appHandle, addrType, addr, flags);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmSetLinkBehaviorReqSendExt
 *
 *  DESCRIPTION
 *      .....
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
                                   CmSetLinkBehaviorMask flags)
{
    CsrBtCmDmSetLinkBehaviorReq *prim;

    prim = (CsrBtCmDmSetLinkBehaviorReq*) CsrPmemAlloc(sizeof(CsrBtCmDmSetLinkBehaviorReq));

    prim->type = CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_REQ;
    prim->appHandle  = appHandle;
    prim->addrType   = addrType;
    prim->addr       = addr;
    prim->flags      = flags;

    CsrBtCmMsgTransport(prim);
}
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
 *      appHandle:           protocol handle
 *      deviceAddr:          BT address of the device to read remote name
 *      serviceList:         A list of Services (UUID) to search for
 *      serviceListSize:     Number of services to search for
 *      extendedUuidSearch:  Defines when a UUID must be consider valid
 *----------------------------------------------------------------------------*/
void CsrBtCmSdcSearchReqSendFunc(CsrSchedQid          appHandle,
                                 CsrBtDeviceAddr deviceAddr,
                                 CsrBtUuid32     *serviceList,
                                 CsrUint8        serviceListSize,
                                 CsrBool         extendedUuidSearch)
{
    CsrBtCmSdcSearchReq *prim;

    prim = (CsrBtCmSdcSearchReq*) CsrPmemAlloc(sizeof(CsrBtCmSdcSearchReq));
    prim->type = CSR_BT_CM_SDC_SEARCH_REQ;
    prim->appHandle = appHandle;
    prim->deviceAddr = deviceAddr;
    prim->serviceList =  serviceList;
    prim->serviceListSize = serviceListSize;
    prim->extendedUuidSearch = extendedUuidSearch;
    CsrBtCmMsgTransport(prim);
}

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
                                       CsrUint8                     localServerChannel)
{
    CmSdcServiceSearchAttrReq *prim;

    prim = (CmSdcServiceSearchAttrReq*) CsrPmemAlloc(sizeof(CmSdcServiceSearchAttrReq));
    prim->type = CM_SDC_SERVICE_SEARCH_ATTR_REQ;
    prim->appHandle = appHandle;
    prim->deviceAddr = deviceAddr;
    prim->svcSearchAttrInfoList = svcSearchAttrInfoList;
    prim->extendedUuidSearch = extendedUuidSearch;
    prim->uuidType = uuidType;
    prim->localServerChannel = localServerChannel;
    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcUuid128SearchReqSend
 *
 *  DESCRIPTION
 *      Submits a search request with a 128 bit uuid16_t to the SDC sub-system
 *
 *  PARAMETERS
 *      appHandle:             protocol handle
 *      deviceAddr:            BT address of the device to read remote name
 *      serviceList:           A list of 128 bit Services (UUID128) to search for
 *      serviceListSize:       Number of services of 128 bit to search for
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
CsrBtCmSdcUuid128SearchReq *CsrBtCmSdcUuid128SearchReq_struct(CsrSchedQid      appHandle,
                                                              CsrBtDeviceAddr   deviceAddr,
                                                              CsrBtUuid128      *serviceList,
                                                              CsrUint8        serviceListSize)
{
    CsrBtCmSdcUuid128SearchReq *prim;

    prim = (CsrBtCmSdcUuid128SearchReq*) CsrPmemAlloc(sizeof(CsrBtCmSdcUuid128SearchReq));
    prim->type = CSR_BT_CM_SDC_UUID128_SEARCH_REQ;
    prim->appHandle = appHandle;
    prim->deviceAddr = deviceAddr;
    prim->serviceList = serviceList;
    prim->serviceListSize = serviceListSize;
    return(prim);
}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcAttributeReqSend
 *      CsrBtCmSdcAttributeRangeReqSend
 *
 *  DESCRIPTION
 *      Submits an attribute request to the SDC sub-system
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
                                     CsrUint16   maxBytesToReturn)
{
    CsrBtCmSdcAttributeReq *prim;

    prim = (CsrBtCmSdcAttributeReq*) CsrPmemAlloc(sizeof(CsrBtCmSdcAttributeReq));
    prim->type = CSR_BT_CM_SDC_ATTRIBUTE_REQ;
    prim->serviceHandle = serviceHandle;
    prim->attributeIdentifier = attributeIdentifier;
    prim->maxBytesToReturn = maxBytesToReturn;
    prim->upperRangeAttributeIdentifier = upperRangeAttributeIdentifier;
    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcCloseReqSend
 *
 *  DESCRIPTION
 *      Closing the SDC channel
 *
 *  PARAMETERS
 *      appHandle:            protocol handle
 *----------------------------------------------------------------------------*/
void CsrBtCmSdcCloseReqSend(CsrSchedQid appHandle)
{
    CsrBtCmSdcCloseReq *prim;
    prim = (CsrBtCmSdcCloseReq*) CsrPmemAlloc(sizeof(CsrBtCmSdcCloseReq));
    prim->type = CSR_BT_CM_SDC_CLOSE_REQ;
    prim->appHandle = appHandle;
    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdsRegisterReqSend
 *
 *  DESCRIPTION
 *      Request to register a service with the service discovery server
 *
 *  PARAMETERS
 *      appHandle:             protocol handle
 *      serviceRecord:         The service record
 *      serviceRecordSize:     Size of the service record
 *      context                Opaque context number
 *----------------------------------------------------------------------------*/
void CsrBtCmSdsRegisterReqSend(CsrSchedQid appHandle,
                               CsrUint8 *serviceRecord,
                               CsrUint16 serviceRecordSize,
                               CsrUint16 context)
{
    CsrBtCmSdsRegisterReq *prim;

    prim = (CsrBtCmSdsRegisterReq*) CsrPmemAlloc(sizeof(CsrBtCmSdsRegisterReq));
    prim->type = CSR_BT_CM_SDS_REGISTER_REQ;
    prim->appHandle = appHandle;
    prim->serviceRecord = serviceRecord;
    prim->serviceRecordSize = serviceRecordSize;
    prim->context = context;
    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdsUnRegisterReqSend
 *
 *  DESCRIPTION
 *      Request to unregister a service with the service discovery server
 *
 *  PARAMETERS
 *      appHandle:            protocol handle
 *      serviceRecHandle:     The service record handle
 *      context               Opaque context number
 *----------------------------------------------------------------------------*/
void CsrBtCmSdsUnRegisterReqSend(CsrSchedQid appHandle,
                                 CsrUint32 serviceRecHandle,
                                 CsrUint16 context)
{
    CsrBtCmSdsUnregisterReq *prim;

    prim = (CsrBtCmSdsUnregisterReq*) CsrPmemAlloc(sizeof(CsrBtCmSdsUnregisterReq));
    prim->type = CSR_BT_CM_SDS_UNREGISTER_REQ;
    prim->appHandle = appHandle;
    prim->serviceRecHandle = serviceRecHandle;
    prim->context = context;
    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmWriteInquirySettingsReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *      phandle:           protocol handle
 *      interval:          the inquiry scan interval
 *      window:            the inquiry scan window
 *----------------------------------------------------------------------------*/
void CsrBtCmWriteInquiryScanSettingsReqSend(CsrSchedQid  ph,
                                            CsrUint16   interval,
                                            CsrUint16   window)
{
    CsrBtCmWriteInquiryscanSettingsReq *prim;

    prim = (CsrBtCmWriteInquiryscanSettingsReq*)CsrPmemAlloc(sizeof(CsrBtCmWriteInquiryscanSettingsReq));
    prim->type = CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_REQ;
    prim->appHandle = ph;
    prim->interval = interval;
    prim->window = window;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadRemoteFeaturesReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *      phandle:          protocol handle
 *      theDeviceAddr:    the device address of the device which features is
 *                        requested
 *----------------------------------------------------------------------------*/
CsrBtCmReadRemoteFeaturesReq *CsrBtCmReadRemoteFeaturesReq_struct(CsrSchedQid     thePhandle,
                                                                  CsrBtDeviceAddr  theDeviceAddr)
{
    CsrBtCmReadRemoteFeaturesReq *prim;

    prim = (CsrBtCmReadRemoteFeaturesReq*) CsrPmemAlloc(sizeof(CsrBtCmReadRemoteFeaturesReq));
    prim->type = CSR_BT_CM_READ_REMOTE_FEATURES_REQ;
    prim->appHandle = thePhandle;
    prim->deviceAddr = theDeviceAddr;
    return(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmRoleDiscoveryReqSend
 *
 *  DESCRIPTION
 *      Discover the current role (Master or Slave)
 *
 *  PARAMETERS
 *      appHandle:             protocol handle
 *      deviceAddr:            BT address
 *----------------------------------------------------------------------------*/
void CsrBtCmRoleDiscoveryReqSend(CsrSchedQid       appHandle,
                                 CsrBtDeviceAddr    deviceAddr)
{
    CsrBtCmRoleDiscoveryReq *prim;

    prim = (CsrBtCmRoleDiscoveryReq*) CsrPmemAlloc(sizeof(CsrBtCmRoleDiscoveryReq));
    prim->type = CSR_BT_CM_ROLE_DISCOVERY_REQ;
    prim->phandle = appHandle;
    prim->deviceAddr = deviceAddr;
    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCancelReadRemoteNameReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      apphandle:        protocol handle
 *      deviceAddr:       BT address of the device that is being read
 *----------------------------------------------------------------------------*/
void CsrBtCmCancelReadRemoteNameReqSend(CsrSchedQid      theAppHandle,
                                        CsrBtDeviceAddr   theDeviceAddr)
{
    CsrBtCmCancelReadRemoteNameReq *prim;

    prim = (CsrBtCmCancelReadRemoteNameReq*) CsrPmemAlloc(sizeof(CsrBtCmCancelReadRemoteNameReq));
    prim->type = CSR_BT_CM_CANCEL_READ_REMOTE_NAME_REQ;
    prim->appHandle = theAppHandle;
    prim->deviceAddr = theDeviceAddr;
    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmEirUpdateManufacturerDataReqSend
 *
 *  DESCRIPTION
 *      Used for setting a the manufacturer specific data in an Extended Inquiry
 *      Response.
 *
 *    PARAMETERS
 *      appHandle:                  Application handle
 *        manufacturerDataSettings:   Settings for handling the manufacturer data.
 *                                  Refer to the documentation for further details.
 *      manufacturerDataLength:     Length of the data in *manufacturerData
 *      manufacturerData:           The actual manufacturer data as it will
 *                                  appear in the EIR.
 *----------------------------------------------------------------------------*/
void CsrBtCmEirUpdateManufacturerDataReqSend(CsrSchedQid     appHandle,
                                             CsrUint8        manufacturerDataSettings,
                                             CsrUint8        manufacturerDataLength,
                                             CsrUint8       *manufacturerData)
{
    CsrBtCmEirUpdateManufacturerDataReq *msg;

    msg                             = (CsrBtCmEirUpdateManufacturerDataReq *)CsrPmemAlloc(sizeof(CsrBtCmEirUpdateManufacturerDataReq));
    msg->type                       = CSR_BT_CM_EIR_UPDATE_MANUFACTURER_DATA_REQ;
    msg->appHandle                  = appHandle;
    msg->manufacturerDataSettings   = manufacturerDataSettings;
    msg->manufacturerDataLength     = manufacturerDataLength;

    if (manufacturerData != NULL && manufacturerDataLength != 0)
    {
        msg->manufacturerData       = manufacturerData;
    }
    else
    {
        msg->manufacturerData       = NULL;
        msg->manufacturerDataLength = 0;
    }

    CsrBtCmMsgTransport(msg);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetEirDataReqSend
 *
 *  DESCRIPTION
 *      Used for setting a the data in an Extended Inquiry
 *      Response.
 *
 *    PARAMETERS
 *      appHandle:                  Application handle
 *      fec:                        FEC required
 *      length:                     Length of the data
 *      data:                       EIR data
 *----------------------------------------------------------------------------*/
void CsrBtCmSetEirDataReqSend(CsrSchedQid appHandle,
                              CsrBool fec,
                              CsrUint8 length,
                              CsrUint8 *data)
{
    CsrBtCmSetEirDataReq *msg = (CsrBtCmSetEirDataReq *) CsrPmemAlloc(sizeof(*msg));

    msg->type = CSR_BT_CM_SET_EIR_DATA_REQ;
    msg->appHandle = appHandle;
    msg->fec = fec;

    if (data && length)
    {
        msg->data = data;
        msg->length = length;
    }
    else
    {
        msg->data = NULL;
        msg->length = 0;
    }

    CsrBtCmMsgTransport(msg);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteMajorMinorCodReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *      phandle:   protocol handle
 *      service:   A Class of device value given from the application
 *      major:     A Major Class of device value given from the application
 *      minor:     A Minor Class of device value given from the application
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_WRITE_COD
CsrBtCmWriteCodReq *CsrBtCmWriteCodReq_struct(CsrSchedQid          phandle,
                                              CsrBtCmUpdateFlags   updateFlags,
                                              CsrBtClassOfDevice   service,
                                              CsrBtClassOfDevice   major,
                                              CsrBtClassOfDevice   minor)
{
    CsrBtCmWriteCodReq *prim;

    prim                       = (CsrBtCmWriteCodReq*) CsrPmemAlloc(sizeof(CsrBtCmWriteCodReq));
    prim->type                 = CSR_BT_CM_WRITE_COD_REQ;
    prim->appHandle            = phandle;
    prim->updateFlags          = updateFlags;
    prim->serviceClassOfDevice = service;
    prim->majorClassOfDevice   = major;
    prim->minorClassOfDevice   = minor;

    return(prim);
}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetEventMaskReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        phandle    :  Protocol handle
 *        eventMask  :  Describes which extended information an application
 *                      will subscribe for
 *        condition  :  Filter condition
 *----------------------------------------------------------------------------*/
void CsrBtCmSetEventMaskReqSend(CsrSchedQid phandle, CsrUint32 eventMask, CsrUint32 conditionMask)
{
    CsrBtCmSetEventMaskReq *prim;

    prim = (CsrBtCmSetEventMaskReq *) CsrPmemAlloc(sizeof(CsrBtCmSetEventMaskReq));
    prim->type          = CSR_BT_CM_SET_EVENT_MASK_REQ;
    prim->phandle       = phandle;
    prim->eventMask     = eventMask;
    prim->conditionMask = conditionMask;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSwitchRoleReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        phandle       :    protocol handle
 *        deviceAddr    :    BT address of the device to read remote version
 *        role          :    Requested role (master/slave/unknown)
 *        roleType      :    Requested role (only CSR_BT_CM_SWITCH_ROLE_TYPE_ONESHOT supported)
 *        config        :    RFU - shall be zero
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC
void CsrBtCmSwitchRoleReqSend(CsrSchedQid     phandle,
                              CsrBtDeviceAddr deviceAddr,
                              CsrUint8        role,
                              CsrBtCmRoleType roleType,
                              CsrUint32       config)
{
    CsrBtCmSwitchRoleReq *prim;

    prim = (CsrBtCmSwitchRoleReq*) CsrPmemAlloc(sizeof(CsrBtCmSwitchRoleReq));
    prim->type        = CM_DM_SWITCH_ROLE_REQ;
    prim->appHandle   = phandle;
    prim->deviceAddr  = deviceAddr;
    prim->role        = role;
    prim->roleType    = roleType;
    prim->config      = config;
    CsrBtCmMsgTransport(prim);
}
#endif

#ifdef CSR_BT_LE_ENABLE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeSetOwnAddressTypeReqSend
 *
 *  DESCRIPTION
 *      Sets the value of own address type to be used for LE GAP procedures.
 *      If application does not set own address type, default public address
 *      and public address type shall be used for LE GAP procedures.
 *
 *  PARAMETERS
 *      appHandle       :  protocol handle
 *      ownAddressType  :  Own address type to be set. Below values are allowed.
 *                         0x00 : Public address
 *                         0x01 : Random address
 *                         0x02 : Controller generated RPA, use public address
 *                                if controller can't generate RPA.
 *                         0x03 : Controller generated RPA, use random address
 *                                from LE_Set_Random_Address if controller can't
 *                                generate RPA.
 * Note : Setting own address type's value to 0x02 and 0x03 is possible only if
 *        controller supports LL_PRIVACY.
 *----------------------------------------------------------------------------*/
CsrBtCmLeSetOwnAddressTypeReq *CsrBtCmLeSetOwnAddressTypeReq_struct(CsrSchedQid appHandle,
                                                                    CsrBtOwnAddressType ownAddressType)
{
    CsrBtCmLeSetOwnAddressTypeReq *prim;

    prim = (CsrBtCmLeSetOwnAddressTypeReq *) CsrPmemAlloc(sizeof(*prim));
    prim->type           = CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_REQ;
    prim->appHandle      = appHandle;
    prim->ownAddressType = ownAddressType;

    return prim;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLePvtAddrTimeoutReqSend
 *
 *  DESCRIPTION
 *      Sets the value of Resolvable/Non-resolvable Private Address timeout.
 *      Length of the time Synergy uses a RPA/NRPA before a new RPA/NRPA is
 *      generated & starts being used.
 *
 *  PARAMETERS
 *      appHandle       :  protocol handle
 *      timeout         :  Sets the value of Private address time out.
 *                         Range : 0x0001(1 sec) - 0xA1B8(~11.5 hours)
 *----------------------------------------------------------------------------*/
CsrBtCmLeSetPvtAddrTimeoutReq *CsrBtCmLeSetPvtAddrTimeoutReq_struct(CsrSchedQid appHandle,
                                                                    CsrUint16 timeout)
{
    CsrBtCmLeSetPvtAddrTimeoutReq *prim;

    prim = (CsrBtCmLeSetPvtAddrTimeoutReq *) CsrPmemAlloc(sizeof(*prim));
    prim->type           = CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_REQ;
    prim->appHandle      = appHandle;
    prim->timeout        = timeout;

    return prim;
}

 /*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeSetStaticAddressReqSend
 *
 *  DESCRIPTION
 *      Configures local static random address for current power cycle. This
 *      API configures application's provided static address as local random
 *      address for current power cycle only if cmake configuration flag
 *      "CSR_BT_LE_RANDOM_ADDRESS_TYPE" set as "STATIC".
 *
 *  Note: Static address shall be configured only once for current power cycle.
 *
 *  PARAMETERS
 *      appHandle       :  protocol handle
 *      staticAddress   :  Static address to be used as local device random
 *                          address.
 * ---------------------------------------------------------------------------*/
CsrBtCmLeSetStaticAddressReq *CsrBtCmLeSetStaticAddressReq_struct(CsrSchedQid appHandle,
                                                                  CsrBtDeviceAddr staticAddress)
{
    CsrBtCmLeSetStaticAddressReq *prim;

    prim = (CsrBtCmLeSetStaticAddressReq *) CsrPmemAlloc(sizeof(*prim));
    prim->type           = CSR_BT_CM_LE_SET_STATIC_ADDRESS_REQ;
    prim->appHandle      = appHandle;
    prim->staticAddress  = staticAddress;

    return prim;
}

 /*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeReadRandomAddressReqSend
 *
 *  DESCRIPTION
 *      Read local or peer device's random address for trusted devices. This API
 *      expects trusted peer device identity address(IA) and provides local or
 *      peer current available random address based on the value set for "flag".
 *      If given peer IA fields are all zeros, "flag" value is ignored and local
 *      random address is returned.
 *
 *      This API expects peer device's IA from subsequent connections with
 *      trusted devices. If it is called at new connection, application has to
 *      pass peer device's connected address as peer's "idAddress" even after
 *      device gets bonded in the same connection.
 *
 *  PARAMETERS
 *      appHandle       :  protocol handle
 *      idAddress       :  Identity address of the connected peer device.
 *      flag            :  Retrieve Random address either for local or peer
 *                         device. Values can be set as:
 *                         0x01 : local and 0x02 : peer device
 *----------------------------------------------------------------------------*/
CsrBtCmLeReadRandomAddressReq *CsrBtCmLeReadRandomAddressReq_struct(CsrSchedQid appHandle,
                                                                    CsrBtTypedAddr idAddress,
                                                                    CsrBtDeviceFlag flag)
{
    CsrBtCmLeReadRandomAddressReq *prim;

    prim = (CsrBtCmLeReadRandomAddressReq *) CsrPmemAlloc(sizeof(*prim));
    prim->type              = CSR_BT_CM_LE_READ_RANDOM_ADDRESS_REQ;
    prim->appHandle         = appHandle;
    prim->idAddress         = idAddress;
    prim->flag              = flag;

    return (prim);
}
#endif /* End of CSR_BT_LE_ENABLE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmAclOpenReqSend
 *
 *  DESCRIPTION
 *      Request to establish ACL connection with a remote device.
 *
 *  PARAMETERS
 *      appHandle:           protocol handle
 *      deviceAddr:          BT address of the device to connect to
 *----------------------------------------------------------------------------*/
void CsrBtCmAclOpenReqSend(CsrSchedQid    appHandle,
                           CsrBtTypedAddr deviceAddr,
                           CsrUint16      flags)
{
    CsrBtCmAclOpenReq *prim;

    prim = CsrPmemAlloc(sizeof(CsrBtCmAclOpenReq));
    prim->type = CSR_BT_CM_ACL_OPEN_REQ;
    prim->appHandle = appHandle;
    prim->address = deviceAddr;
    prim->flags = flags;

    CsrBtCmMsgTransport(prim);
}

void CsrBtCmLeAclOpenReqSend(CsrSchedQid    appHandle,
                             CsrBtTypedAddr deviceAddr)
{
    CsrBtCmAclOpenReqSend(appHandle, deviceAddr, DM_ACL_FLAG_ULP);
}

void CmLeAclOpenUseFilterAcceptListReqSend(CsrSchedQid appHandle)
{
    CsrBtTypedAddr deviceAddr = {0};
    CsrBtCmAclOpenReqSend(appHandle, 
                          deviceAddr, 
                          CM_ACL_FLAG_LE_CONNECT_ACCEPTOR_LIST);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmAclCloseReqSend
 *
 *  DESCRIPTION
 *      Request to disconnect a remote device.
 *
 *  PARAMETERS
 *      appHandle:           protocol handle
 *      deviceAddr:          BT address of the device to disconnect to
 *----------------------------------------------------------------------------*/
void CsrBtCmAclCloseReqSend(CsrSchedQid    appHandle,
                            CsrBtTypedAddr deviceAddr,
                            CsrUint16      flags,
                            CsrUint8       reason)
{
    CsrBtCmAclCloseReq *prim;

    prim = CsrPmemAlloc(sizeof(CsrBtCmAclCloseReq));
    prim->type = CSR_BT_CM_ACL_CLOSE_REQ;
    prim->flags = flags;
    prim->reason = reason;
    prim->appHandle = appHandle;
    prim->address = deviceAddr;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeScanReq
 *
 *  DESCRIPTION
 *      Toggle low energy scanning and parameter setup
 *
 *  PARAMETERS
 *
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
                          CsrBtTypedAddr *addressList)
{
    CsrBtCmLeScanReq *prim;

    prim = CsrPmemAlloc(sizeof(CsrBtCmLeScanReq));
    prim->type = CSR_BT_CM_LE_SCAN_REQ;
    prim->appHandle = appHandle;
    prim->context = context;
    prim->mode = mode;
    prim->scanType = scanType;
    prim->scanInterval = scanInterval;
    prim->scanWindow = scanWindow;
    prim->scanningFilterPolicy = scanningFilterPolicy;
    prim->filterDuplicates = filterDuplicates;
    prim->addressCount = addressCount;
    prim->addressList = addressList;

    CsrBtCmMsgTransport(prim);
}


void CsrBtCmLeScanReqOffSend(CsrSchedQid    appHandle,
                             CsrUint16      context)
{
    CsrBtCmLeScanReqSend(appHandle, context, CSR_BT_CM_LE_MODE_OFF, 0, 0, 0, 0, 0, 0, NULL);
}

void CsrBtCmLeScanReqCoexOnSend(CsrSchedQid    appHandle,
                                CsrUint16      scanInterval,
                                CsrUint16      scanWindow)
{
    CsrBtCmLeScanReqSend(appHandle, 0, CSR_BT_CM_LE_MODE_ON | CSR_BT_CM_LE_MODE_COEX_NOTIFY, 0, scanInterval, scanWindow, 0, 0, 0, NULL);
}

void CsrBtCmLeScanReqCoexOffSend(CsrSchedQid    appHandle)
{
    CsrBtCmLeScanReqSend(appHandle, 0, CSR_BT_CM_LE_MODE_OFF | CSR_BT_CM_LE_MODE_COEX_NOTIFY, 0, 0, 0, 0, 0, 0, NULL);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeAdvertiseReq
 *
 *  DESCRIPTION
 *      Toggle low energy advertising and parameter setup
 *
 *  PARAMETERS
 *
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
                                 CsrBtTypedAddr  address)
{
    CsrBtCmLeAdvertiseReq *prim;

    prim = CsrPmemAlloc(sizeof(CsrBtCmLeAdvertiseReq));
    prim->type = CSR_BT_CM_LE_ADVERTISE_REQ;
    prim->appHandle = appHandle;
    prim->context = context;
    prim->mode = mode;
    prim->paramChange = paramChange;
    prim->advertisingDataLength = advertisingDataLength;
    prim->advertisingData = advertisingData;
    prim->scanResponseDataLength = scanResponseDataLength;
    prim->scanResponseData = scanResponseData;
    prim->advIntervalMin = advIntervalMin;
    prim->advIntervalMax = advIntervalMax;
    prim->advertisingType = advertisingType;
    prim->advertisingChannelMap = advertisingChannelMap;
    prim->advertisingFilterPolicy = advertisingFilterPolicy;
    prim->whitelistAddrCount = whitelistAddrCount;
    prim->whitelistAddrList = whitelistAddrList;
    prim->address = address;

    CsrBtCmMsgTransport(prim);
}

void CsrBtCmLeAdvertiseReqStartSend(CsrSchedQid appHandle,
                                    CsrUint16 context,
                                    CsrUint8 mode,
                                    CsrUint32 paramChange,
                                    CsrUint8 advertisingDataLength,
                                    CsrUint8 *advertisingData,
                                    CsrUint8 scanResponseDataLength,
                                    CsrUint8 *scanResponseData,
                                    CsrUint16 advIntervalMin,
                                    CsrUint16 advIntervalMax,
                                    CsrUint8 advertisingType,
                                    CsrUint8 advertisingChannelMap,
                                    CsrUint8 advertisingFilterPolicy,
                                    CsrUint16 whitelistAddrCount,
                                    CsrBtTypedAddr *whitelistAddrList)
{
    CsrBtTypedAddr address;
    CsrBtAddrZero(&address);

    CsrBtCmLeStartAdvertisement(appHandle,
                                context,
                                mode,
                                paramChange,
                                advertisingDataLength,
                                advertisingData,
                                scanResponseDataLength,
                                scanResponseData,
                                advIntervalMin,
                                advIntervalMax,
                                advertisingType,
                                advertisingChannelMap,
                                advertisingFilterPolicy,
                                whitelistAddrCount,
                                whitelistAddrList,
                                address);

}

void CsrBtCmLeAdvertiseReqStopSend(CsrSchedQid appHandle,
                                   CsrUint16 context)
{
	CsrBtCmLeAdvertiseReqStartSend(appHandle, context, CSR_BT_CM_LE_MODE_OFF, FALSE, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0, NULL);
}

void CsrBtCmLeAdvertiseReqCoexOnSend(CsrUint8 advertisingType)
{
    CsrBtCmLeAdvertiseReqStartSend(CSR_BT_CM_IFACEQUEUE, 0, CSR_BT_CM_LE_MODE_ON|CSR_BT_CM_LE_MODE_COEX_NOTIFY, FALSE, 0, NULL, 0, NULL, 0, 0, advertisingType, 0, 0, 0, NULL);
}

void CsrBtCmLeAdvertiseReqCoexOffSend(void)
{
    CsrBtCmLeAdvertiseReqStartSend(CSR_BT_CM_IFACEQUEUE, 0, CSR_BT_CM_LE_MODE_OFF|CSR_BT_CM_LE_MODE_COEX_NOTIFY, FALSE, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0, NULL);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeWhitelistSetReq
 *
 *  DESCRIPTION
 *      Control whitelist on low energy
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
void CsrBtCmLeWhitelistSetReqSend(CsrSchedQid    appHandle,
                                  CsrUint16      addressCount,
                                  CsrBtTypedAddr *addressList)
{
    CsrBtCmLeWhitelistSetReq *prim;

    prim = CsrPmemAlloc(sizeof(CsrBtCmLeWhitelistSetReq));
    prim->type = CSR_BT_CM_LE_WHITELIST_SET_REQ;
    prim->appHandle = appHandle;
    prim->addressCount = addressCount;
    prim->addressList = addressList;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeConnparamReq
 *
 *  DESCRIPTION
 *      Set LE connection parameters and acceptable values
 *
 *  PARAMETERS
 *
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
                                                    CsrUint16 supervisionTimeoutMax)
{
    CsrBtCmLeConnparamReq *prim;
    prim = (CsrBtCmLeConnparamReq*)CsrPmemAlloc(sizeof(CsrBtCmLeConnparamReq));

    prim->type = CSR_BT_CM_LE_CONNPARAM_REQ;
    prim->appHandle = appHandle;
    prim->scanInterval = scanInterval;
    prim->scanWindow = scanWindow;
    prim->connIntervalMin = connIntervalMin;
    prim->connIntervalMax = connIntervalMax;
    prim->connLatency = connLatency;
    prim->supervisionTimeout = supervisionTimeout;
    prim->connLatencyMax = connLatencyMax;
    prim->supervisionTimeoutMin = supervisionTimeoutMin;
    prim->supervisionTimeoutMax = supervisionTimeoutMax;
    return prim;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeConnparamUpdateReq
 *
 *  DESCRIPTION
 *      Update LE connection parameters for existing connection
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmLeConnparamUpdateReq *CsrBtCmLeConnparamUpdateReq_struct(CsrSchedQid appHandle,
                                                                CsrBtTypedAddr address,
                                                                CsrUint16 connIntervalMin,
                                                                CsrUint16 connIntervalMax,
                                                                CsrUint16 connLatency,
                                                                CsrUint16 supervisionTimeout,
                                                                CsrUint16 minimumCeLength,
                                                                CsrUint16 maximumCeLength)
{
    CsrBtCmLeConnparamUpdateReq *prim;
    prim = (CsrBtCmLeConnparamUpdateReq*)CsrPmemAlloc(sizeof(CsrBtCmLeConnparamUpdateReq));
    prim->type = CSR_BT_CM_LE_CONNPARAM_UPDATE_REQ;

    prim->appHandle = appHandle;
    prim->address = address;
    prim->connIntervalMin = connIntervalMin;
    prim->connIntervalMax = connIntervalMax;
    prim->connLatency = connLatency;
    prim->supervisionTimeout = supervisionTimeout;
    prim->minimumCeLength = minimumCeLength;
    prim->maximumCeLength = maximumCeLength;
    return prim;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeAcceptConnparamUpdateRes
 *
 *  DESCRIPTION
 *      Accept or reject LE Slave REquest to Update LE connection parameters 
 *      for existing connection
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmLeAcceptConnparamUpdateRes *CsrBtCmLeAcceptConnparamUpdateRes_struct(CsrBtTypedAddr    address, 
                                                                            l2ca_identifier_t l2caSignalId, 
                                                                            CsrUint16         connIntervalMin, 
                                                                            CsrUint16         connIntervalMax, 
                                                                            CsrUint16         connLatency,
                                                                            CsrUint16         supervisionTimeout, 
                                                                            CsrBool           accept)
{
    CsrBtCmLeAcceptConnparamUpdateRes *prim = (CsrBtCmLeAcceptConnparamUpdateRes*)
                                               CsrPmemAlloc(sizeof(CsrBtCmLeAcceptConnparamUpdateRes));

    prim->type               = CSR_BT_CM_LE_ACCEPT_CONNPARAM_UPDATE_RES;
    prim->address            = address;
    prim->l2caSignalId       = l2caSignalId;  
    prim->connIntervalMin    = connIntervalMin;
    prim->connIntervalMax    = connIntervalMax;
    prim->connLatency        = connLatency;
    prim->supervisionTimeout = supervisionTimeout;
    prim->accept             = accept;
    return prim;
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmRegisterHandlerReqSend
 *
 *  DESCRIPTION
 *      Register upper layer handler in the CM
 *
 *  PARAMETERS
 *      handlerType:        CSR_BT_CM_HANDLER_TYPE_x
 *      handle:             Handle for the upper layer
 *      flags:              Unused, must be zero
 *----------------------------------------------------------------------------*/

void CsrBtCmRegisterHandlerReqSend(CsrUint8     handlerType,
                                   CsrSchedQid  handle,
                                   CsrUint32    flags)
{
    CsrBtCmRegisterHandlerReq *prim;

    prim = CsrPmemAlloc(sizeof(CsrBtCmRegisterHandlerReq));
    prim->type = CSR_BT_CM_REGISTER_HANDLER_REQ;
    prim->handlerType = handlerType;
    prim->handle = handle;
    prim->flags = flags;

    CsrBtCmMsgTransport(prim);
}
#ifdef CSR_BT_ISOC_ENABLE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocRegisterReqSend
 *
 *  DESCRIPTION
 *      Register a destination appHandle for Isochronous functionality.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      isoc_type:           Type of isochronous stream for registration
 *                           Permitted values :
 *                           CM_ISOC_TYPE_UNICAST
 *----------------------------------------------------------------------------*/
void CmIsocRegisterReqSend(CsrSchedQid    appHandle,
                           CsrUint16      isoc_type)
{
    CmIsocRegisterReq *prim;

    prim = CsrPmemAlloc(sizeof(CmIsocRegisterReq));
    prim->type = CSR_BT_CM_ISOC_REGISTER_REQ;
    prim->appHandle = appHandle;
    prim->isoc_type = isoc_type;

    CsrBtCmMsgTransport(prim);
}

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
 *     Note: cis_config shall contain a valid pointer to each CIS that is
 *     registered under the CIG and it shall be valid for cis_count elements,
 *     rest shall be set to NULL.
 *
 *  PARAMETERS
 *      appHandle:                       Application handle
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
                               CmCisConfig    *cis_config[])
{
    CmIsocConfigureCigReq *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(CmIsocConfigureCigReq));
    prim->type = CSR_BT_CM_ISOC_CONFIGURE_CIG_REQ;
    prim->appHandle = appHandle;
    prim->sdu_interval_m_to_s = sdu_interval_m_to_s;
    prim->sdu_interval_s_to_m = sdu_interval_s_to_m;
    prim->max_transport_latency_m_to_s = max_transport_latency_m_to_s;
    prim->max_transport_latency_s_to_m = max_transport_latency_s_to_m;
    prim->cig_id = cig_id;
    prim->sca = sca;
    prim->packing = packing;
    prim->framing = framing;
    prim->cis_count = cis_count;

    for (i = 0; i < cis_count; i++)
    {
        prim->cis_config[i] = cis_config[i];
        cis_config[i] = NULL;
    }

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocRemoveCigReqSend
 *
 *  DESCRIPTION
 *      Request to remove the connected isochronous streams which have previously
 *      been configured in the controller.

 *     Note: This command should be issued when all Connected Isochronous
 *           Streams in the CIG are disconnected.
 *
 *  PARAMETERS
 *      appHandle:        Application handle
 *      cig_id:           CIG Identifier to be removed
 *----------------------------------------------------------------------------*/
void CmIsocRemoveCigReqSend(CsrSchedQid appHandle, CsrUint8 cig_id)
{
    CmIsocRemoveCigReq *prim;

    prim = CsrPmemAlloc(sizeof(CmIsocRemoveCigReq));
    prim->type = CSR_BT_CM_ISOC_REMOVE_CIG_REQ;
    prim->appHandle = appHandle;
    prim->cig_id = cig_id;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocCisConnectReqSend
 *
 *  DESCRIPTION
 *      Request to establish connected isochronous streams(CIS) with remote device.
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
 *  PARAMETERS
 *      appHandle:           Application handle
 *      cis_count:           number of CIS connections
 *      cis_conn:            list of cis handle and addresses to connect
 *----------------------------------------------------------------------------*/
void CmIsocCisConnectReqSend(CsrSchedQid    appHandle,
                             CsrUint8 cis_count,
                             CmCisConnection *cis_conn[])
{
    CmIsocCisConnectReq *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(CmIsocCisConnectReq));
    prim->type = CSR_BT_CM_ISOC_CIS_CONNECT_REQ;
    prim->appHandle = appHandle;
    prim->cis_count = cis_count;

    for (i = 0; i < cis_count; i++)
    {
        prim->cis_conn[i] = cis_conn[i];
        cis_conn[i] = NULL;
    }

    CsrBtCmMsgTransport(prim);
}


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
 *      appHandle:           Application handle
 *      cis_handle:          CIS handle from connect ind
 *      status:              hci status
 *----------------------------------------------------------------------------*/
void CmIsocCisConnectRspSend(CsrSchedQid             appHandle,
                             hci_connection_handle_t cis_handle,
                             hci_return_t             status)
{
    CmIsocCisConnectRsp *prim;

    prim = CsrPmemAlloc(sizeof(CmIsocCisConnectRsp));
    prim->type = CSR_BT_CM_ISOC_CIS_CONNECT_RSP;
    prim->appHandle = appHandle;
    prim->cis_handle = cis_handle;
    prim->status = status;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocCisDisconnectReqSend
 *
 *  DESCRIPTION
 *      Request to terminate CIS connection
 *
 *     CIS connection can be disconnected using cis_handle and appropriate HCI
 *     reason code for disconnection.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      cis_handle:          CIS handle for disconnection
 *      reason:              Reason for command
 *----------------------------------------------------------------------------*/
void CmIsocCisDisconnectReqSend(CsrSchedQid appHandle,
                                hci_connection_handle_t cis_handle,
                                hci_reason_t reason)
{
    CmIsocCisDisconnectReq *prim;

    prim = CsrPmemAlloc(sizeof(CmIsocCisDisconnectReq));
    prim->type = CSR_BT_CM_ISOC_CIS_DISCONNECT_REQ;
    prim->appHandle = appHandle;
    prim->cis_handle = cis_handle;
    prim->reason = reason;

    CsrBtCmMsgTransport(prim);
}

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
 *
 *  PARAMETERS
 *      appHandle:            Application handle
 *      handle:               CIS or BIS handle
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
                                     CsrUint8 *codec_config_data)
{
    CmIsocSetupIsoDataPathReq *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(CmIsocSetupIsoDataPathReq));
    prim->type = CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_REQ;
    prim->appHandle = appHandle;
    prim->handle = handle;
    prim->data_path_direction = data_path_direction;
    prim->data_path_id = data_path_id;
    prim->controller_delay = controller_delay;
    prim->codec_config_length = codec_config_length;

    for (i = 0; i < CM_CODEC_ID_SIZE; i++)
    {
        prim->codec_id[i] = codec_id[i];
    }

    if (codec_config_length)
    {
        prim->codec_config_length = codec_config_length;
        prim->codec_config_data = CsrPmemAlloc((codec_config_length * sizeof(CsrUint8)));

        for (i = 0; i < codec_config_length; i++)
        {
            prim->codec_config_data[i] = codec_config_data[i];
        }
    }

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsocRemoveIsoDataPathReqSend
 *
 *  DESCRIPTION
 *      Request to remove the ISO data path associated with CIS.
 *
 *  PARAMETERS
 *      appHandle:             Application handle
 *      handle:                CIS or BIS handle
 *      data_path_direction:   Direction of the path to be removed
 *----------------------------------------------------------------------------*/
void CmIsocRemoveIsoDataPathReqSend(CsrSchedQid appHandle,
                                    hci_connection_handle_t handle,
                                    CsrUint8 data_path_direction)
{
    CmIsocRemoveIsoDataPathReq *prim;

    prim = CsrPmemAlloc(sizeof(CmIsocRemoveIsoDataPathReq));
    prim->type = CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_REQ;
    prim->appHandle = appHandle;
    prim->handle = handle;
    prim->data_path_direction = data_path_direction;

    CsrBtCmMsgTransport(prim);
}


void CmIsocCreateBigReqSend(CsrSchedQid    appHandle,
                            CsrUint8 big_handle,
                            CsrUint8 adv_handle,
                            CmBigConfigParam *big_config,
                            CsrUint8 num_bis,
                            CsrUint8 encryption,
                            CsrUint8 *broadcast_code)
{
    CmIsocCreateBigReq *prim;

    prim = CsrPmemZalloc(sizeof(CmIsocCreateBigReq));
    prim->type = CSR_BT_CM_ISOC_CREATE_BIG_REQ;
    prim->appHandle = appHandle;
    prim->big_handle = big_handle;
    prim->adv_handle = adv_handle;
    prim->num_bis = num_bis;
    prim->encryption = encryption;
    if(big_config)
        SynMemCpyS(&prim->big_config, sizeof(CmBigConfigParam), big_config, sizeof(CmBigConfigParam));
    if(encryption && broadcast_code)
        SynMemCpyS(&prim->broadcast_code, CM_DM_BROADCAST_CODE_SIZE, broadcast_code, CM_DM_BROADCAST_CODE_SIZE);

    CsrBtCmMsgTransport(prim);
}

void CmIsocTerminateBigReqSend(CsrSchedQid appHandle,
                    CsrUint8 big_handle,
                    hci_reason_t reason)
{
    CmIsocTerminateBigReq *prim;

    prim = CsrPmemZalloc(sizeof(CmIsocTerminateBigReq));
    prim->type = CSR_BT_CM_ISOC_TERMINATE_BIG_REQ;
    prim->appHandle = appHandle;
    prim->big_handle = big_handle;
    prim->reason = reason;

    CsrBtCmMsgTransport(prim);
}

void CmIsocBigCreateSyncReqSend(CsrSchedQid appHandle,
                            CsrUint16 sync_handle,
                            CsrUint16 big_sync_timeout,
                            CsrUint8 big_handle,
                            CsrUint8 mse,
                            CsrUint8 encryption,
                            CsrUint8 *broadcast_code,
                            CsrUint8 num_bis,
                            CsrUint8 *bis)
{
    CmIsocBigCreateSyncReq *prim;

    prim = CsrPmemZalloc(sizeof(CmIsocBigCreateSyncReq));
    prim->type = CSR_BT_CM_ISOC_BIG_CREATE_SYNC_REQ;
    prim->phandle = appHandle;
    prim->sync_handle = sync_handle;
    prim->big_sync_timeout = big_sync_timeout;
    prim->big_handle = big_handle;
    prim->mse = mse;
    prim->num_bis = num_bis;
    prim->encryption = encryption;
    if(encryption && broadcast_code)
        SynMemCpyS(&prim->broadcast_code, CM_DM_BROADCAST_CODE_SIZE, broadcast_code, CM_DM_BROADCAST_CODE_SIZE);

    if(num_bis && bis)
    {
        prim->bis = CsrPmemZalloc(num_bis*sizeof(uint8));
        SynMemCpyS(prim->bis, (num_bis*sizeof(uint8)), bis, (num_bis*sizeof(uint8)));
    }

    CsrBtCmMsgTransport(prim);
}

void CmIsocBigTerminateSyncReqSend(CsrSchedQid appHandle,
                                CsrUint8 bigHandle)
{
    CmIsocBigTerminateSyncReq *prim;

    prim = CsrPmemZalloc(sizeof(CmIsocBigTerminateSyncReq));
    prim->type = CSR_BT_CM_ISOC_BIG_TERMINATE_SYNC_REQ;
    prim->phandle = appHandle;
    prim->big_handle = bigHandle;

    CsrBtCmMsgTransport(prim);
}

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
 *     Note: cis_config shall contain a valid pointer to each CIS that is
 *     registered under the CIG and it shall be valid for cis_count elements,
 *     rest shall be set to NULL.
 *
 *  PARAMETERS
 *      appHandle:                       Application handle
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
 *      cis_config:                      Array of pointers to cis configuration
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
                                   CmCisTestConfig *cis_test_config[])
{
    CmIsocConfigureCigTestReq *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(CmIsocConfigureCigTestReq));
    prim->type = CSR_BT_CM_ISOC_CONFIGURE_CIG_TEST_REQ;
    prim->appHandle = appHandle;
    prim->sdu_interval_m_to_s = sdu_interval_m_to_s;
    prim->sdu_interval_s_to_m = sdu_interval_s_to_m;
    prim->iso_interval = iso_interval;
    prim->cig_id = cig_id;
    prim->ft_m_to_s = ft_m_to_s;
    prim->ft_s_to_m = ft_s_to_m;
    prim->sca = sca;
    prim->packing = packing;
    prim->framing = framing;
    prim->cis_count = cis_count;

    for (i = 0; i < cis_count; i++)
    {
        prim->cis_test_config[i] = cis_test_config[i];
        cis_test_config[i] = NULL;
    }

    CsrBtCmMsgTransport(prim);
}

void CmIsocCreateBigTestReqSend(CsrSchedQid    appHandle,
                            CsrUint8 big_handle,
                            CsrUint8 adv_handle,
                            CmBigTestConfigParam *big_config,
                            CsrUint8 num_bis,
                            CsrUint8 encryption,
                            CsrUint8 *broadcast_code)
{
    CmIsocCreateBigTestReq *prim;

    prim = CsrPmemZalloc(sizeof(CmIsocCreateBigTestReq));
    prim->type = CSR_BT_CM_ISOC_CREATE_BIG_TEST_REQ;
    prim->appHandle = appHandle;
    prim->big_handle = big_handle;
    prim->adv_handle = adv_handle;
    prim->num_bis = num_bis;
    prim->encryption = encryption;
    if(big_config)
        SynMemCpyS(&prim->big_config, sizeof(CmBigTestConfigParam), big_config, sizeof(CmBigTestConfigParam));
    if(encryption && broadcast_code)
        SynMemCpyS(&prim->broadcast_code, CM_DM_BROADCAST_CODE_SIZE, broadcast_code, CM_DM_BROADCAST_CODE_SIZE);

    CsrBtCmMsgTransport(prim);
}

void CmIsocReadIsoLinkQualityReqSend(CsrSchedQid appHandle, 
                                     hci_connection_handle_t handle)
{
    CmIsocReadIsoLinkQualityReq *prim;

    prim = CsrPmemZalloc(sizeof(CmIsocReadIsoLinkQualityReq));
    prim->type = CM_ISOC_READ_ISO_LINK_QUALITY_REQ;
    prim->appHandle = appHandle;
    prim->handle = handle;

    CsrBtCmMsgTransport(prim);
}
#endif /* End of CSR_BT_ISOC_ENABLE */

#if defined(CSR_BT_INSTALL_EXTENDED_SCANNING) || defined(CSR_BT_INSTALL_EXTENDED_ADVERTISING) \
    || defined(CSR_BT_INSTALL_PERIODIC_SCANNING) || defined(CSR_BT_INSTALL_PERIODIC_ADVERTISING)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmGetAdvScanCapabilitiesReqSend
 *
 *  DESCRIPTION
 *      Get information on what APIs are available and size limitations.
 *
 *  PARAMETERS
 *      appHandle         Application handle
 *----------------------------------------------------------------------------*/
void CmGetAdvScanCapabilitiesReqSend(CsrSchedQid appHandle)
{
    CmGetAdvScanCapabilitiesReq *prim;

    prim = CsrPmemAlloc(sizeof(CmGetAdvScanCapabilitiesReq));
    prim->type = CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_REQ;
    prim->appHandle = appHandle;

    CsrBtCmMsgTransport(prim);
}
#endif

#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING

void CmExtAdvRegisterAppAdvSetReqSend(CsrSchedQid appHandle, CsrUint8 advHandle, CsrUint32 flags)
{
    CmExtAdvRegisterAppAdvSetReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtAdvRegisterAppAdvSetReq));
    prim->type = CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->flags = flags;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvUnregisterAppAdvSetReqSend
 *
 *  DESCRIPTION
 *      Application un-register the advertising set.
 *
 *  PARAMETERS
 *      appHandle:       Application handle
 *      advHandle        A registered advertising set value
 *----------------------------------------------------------------------------*/
void CmExtAdvUnregisterAppAdvSetReqSend(CsrSchedQid appHandle, CsrUint8 advHandle)
{
    CmExtAdvUnregisterAppAdvSetReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtAdvUnregisterAppAdvSetReq));
    prim->type = CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetParamsReqSend
 *
 *  DESCRIPTION
 *      Write the advertising set paramaters.
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
                                    CsrUint32 reserved)
{
    CmExtAdvSetParamsReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtAdvSetParamsReq));
    CsrMemSet(prim, 0, sizeof(CmExtAdvSetParamsReq));
    prim->type = CSR_BT_CM_EXT_ADV_SET_PARAMS_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->advEventProperties = advEventProperties;
    prim->primaryAdvIntervalMin = primaryAdvIntervalMin;
    prim->primaryAdvIntervalMax = primaryAdvIntervalMax;
    prim->primaryAdvChannelMap = primaryAdvChannelMap;
    prim->ownAddrType = ownAddrType;
    CsrBtAddrCopy(&(prim->peerAddr), &(peerAddr));
    prim->advFilterPolicy = advFilterPolicy;
    prim->primaryAdvPhy = primaryAdvPhy;
    prim->secondaryAdvMaxSkip = secondaryAdvMaxSkip;
    prim->secondaryAdvPhy = secondaryAdvPhy;
    prim->advSid = advSid;
    prim->reserved = reserved;

    CsrBtCmMsgTransport(prim);
}

#ifdef INSTALL_CM_EXT_ADV_SET_PARAM_V2
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetParamsV2ReqSend
 *
 *  DESCRIPTION
 *      Write the advertising set V2 paramaters. 
 *      Similar to CmExtAdvSetParamsReqSend, but additional params e.g adv tx power can be configured
 *      Result of the operation available as part of CM_EXT_ADV_SET_PARAMS_V2_CFM
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
 *      advTxPower             Max power level at which the adv packets are transmitted (-127 to +20 in dBm)
 *                             Controller can choose power level lower than or equal to the one specified by Host
 *      scanReqNotifyEnable    RFA. Always set to 0.
 *      primaryAdvPhyOptions   RFA.Always set to 0.
 *      secondaryAdvPhyOptions RFA.Always set to 0.
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
                                    CsrUint8 secondaryAdvPhyOptions)
{
    CmDmExtAdvSetParamsV2Req *prim;

    prim = CsrPmemAlloc(sizeof(CmDmExtAdvSetParamsV2Req));
    CsrMemSet(prim, 0, sizeof(CmDmExtAdvSetParamsV2Req));
    prim->type = CM_DM_EXT_ADV_SET_PARAMS_V2_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->advEventProperties = advEventProperties;
    prim->primaryAdvIntervalMin = primaryAdvIntervalMin;
    prim->primaryAdvIntervalMax = primaryAdvIntervalMax;
    prim->primaryAdvChannelMap = primaryAdvChannelMap;
    prim->ownAddrType = ownAddrType;
    CsrBtAddrCopy(&(prim->peerAddr), &(peerAddr));
    prim->advFilterPolicy = advFilterPolicy;
    prim->primaryAdvPhy = primaryAdvPhy;
    prim->secondaryAdvMaxSkip = secondaryAdvMaxSkip;
    prim->secondaryAdvPhy = secondaryAdvPhy;
    prim->advSid = advSid;
    prim->advTxPower = advTxPower;
    prim->scanReqNotifyEnable = scanReqNotifyEnable;
    prim->primaryAdvPhyOptions = primaryAdvPhyOptions;
    prim->secondaryAdvPhyOptions = secondaryAdvPhyOptions;

    CsrBtCmMsgTransport(prim);
}
#endif /* INSTALL_CM_EXT_ADV_SET_PARAM_V2 */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetDataReqSend
 *
 *  DESCRIPTION
 *      Write the extended advertising data.
 *
 *  PARAMETERS
 *      appHandle:        Application handle
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
                                    CsrUint8 dataLength,
                                    CsrUint8 *data[])
{
    CsrUint8 i;
    CmExtAdvSetDataReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtAdvSetDataReq));
    CsrMemSet(prim, 0, sizeof(CmExtAdvSetDataReq));
    prim->type = CSR_BT_CM_EXT_ADV_SET_DATA_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->operation = operation;
    prim->fragPreference = fragPreference;
    prim->dataLength = (dataLength < CM_EXT_ADV_DATA_LENGTH_MAX) ? 
        dataLength : CM_EXT_ADV_DATA_LENGTH_MAX;
    for (i = 0; i < CM_EXT_ADV_DATA_BYTES_PTRS_MAX; i++)
    {
        prim->data[i] = data[i];
    }

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetScanRespDataReqSend
 *
 *  DESCRIPTION
 *      Write the extended advertising's scan response data.
 *
 *  PARAMETERS
 *      appHandle:        Application handle
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
                                    CsrUint8 dataLength,
                                    CsrUint8 *data[])
{
    CsrUint8 i;
    CmExtAdvSetScanRespDataReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtAdvSetScanRespDataReq));
    CsrMemSet(prim, 0, sizeof(CmExtAdvSetScanRespDataReq));
    prim->type = CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->operation = operation;
    prim->fragPreference = fragPreference;
    prim->dataLength = (dataLength < CM_EXT_ADV_SCAN_RESP_DATA_LENGTH_MAX) ? 
        dataLength : CM_EXT_ADV_SCAN_RESP_DATA_LENGTH_MAX;
    for (i = 0; i < CM_EXT_ADV_SCAN_RESP_DATA_BYTES_PTRS_MAX; i++)
    {
        prim->data[i] = data[i];
    }

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvEnableReqSend
 *
 *  DESCRIPTION
 *      Enable or Disable extended advertising.
 *
 *  PARAMETERS
 *      appHandle:       Application handle
 *      advHandle        A registered advertising set value
 *      enable           Enable (1) or Disable (0) extended advertising
 *----------------------------------------------------------------------------*/
void CmExtAdvEnableReqSend(CsrSchedQid appHandle, CsrUint8 advHandle, CsrUint8 enable)
{
    CmExtAdvEnableReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtAdvEnableReq));
    prim->type = CSR_BT_CM_EXT_ADV_ENABLE_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->enable = enable;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvReadMaxAdvDataLenReqSend
 *
 *  DESCRIPTION
 *      Read thr extended advertising max data length supported by controller.
 *
 *  PARAMETERS
 *      appHandle:     Application handle
 *      advHandle      A registered advertising set value
 *----------------------------------------------------------------------------*/
void CmExtAdvReadMaxAdvDataLenReqSend(CsrSchedQid appHandle, CsrUint8 advHandle)
{
    CmExtAdvReadMaxAdvDataLenReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtAdvReadMaxAdvDataLenReq));
    prim->type = CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetRandomAddrReqSend
 *
 *  DESCRIPTION
 *      Set the advertising set's random device address to be used.
 *
 *  PARAMETERS
 *      appHandle:       Application handle
 *      advHandle        A registered advertising set value
 *      action           Option for generating or setting a random address
 *      randomAddr       Random address to be set
 *----------------------------------------------------------------------------*/
void CmExtAdvSetRandomAddrReqSend(CsrSchedQid appHandle,
                                              CsrUint8 advHandle,
                                              CsrUint16 action,
                                              CsrBtDeviceAddr randomAddr)
{
    CmExtAdvSetRandomAddrReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtAdvSetRandomAddrReq));
    prim->type = CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->action = action;
    prim->randomAddr = randomAddr;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvSetsInfoReqSend
 *
 *  DESCRIPTION
 *      Reports advertising and registered states for all advertising sets 
 *      supported by a device
 *
 *  PARAMETERS
 *      appHandle      Application handle
 *----------------------------------------------------------------------------*/
void CmExtAdvSetsInfoReqSend(CsrSchedQid appHandle)
{
    CmExtAdvSetsInfoReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtAdvSetsInfoReq));
    prim->type = CSR_BT_CM_EXT_ADV_SETS_INFO_REQ;
    prim->appHandle = appHandle;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvMultiEnableReqSend
 *
 *  DESCRIPTION
 *      Enable/disable advertising for 1 to 4 advertising sets. Further info
 *      be found in BT5.1: command = LE Set Extended Advertising Enable Command
 *
 *  PARAMETERS
 *      appHandle      Application handle
 *----------------------------------------------------------------------------*/
void CmExtAdvMultiEnableReqSend(CsrSchedQid appHandle,
                                           CsrUint8 enable,
                                           CsrUint8 numSets,
                                           CmEnableConfig config[CM_EXT_ADV_MAX_NUM_ENABLE])
{
    CmExtAdvMultiEnableReq *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(CmExtAdvMultiEnableReq));
    prim->type = CSR_BT_CM_EXT_ADV_MULTI_ENABLE_REQ;
    prim->appHandle = appHandle;
    prim->enable = enable;
    prim->numSets = numSets;
    prim->numSets = (numSets <= CM_EXT_ADV_MAX_NUM_ENABLE) ?
        numSets : CM_EXT_ADV_MAX_NUM_ENABLE;
    for (i = 0; i < prim->numSets; i++)
    {
        prim->config[i] = config[i];
    }

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtAdvGetAddressReqSend
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
                                CsrUint8 advHandle)
{
    CmDmExtAdvGetAddrReq *prim;

    prim = CsrPmemAlloc(sizeof(CmDmExtAdvGetAddrReq));
    prim->type = CM_DM_EXT_ADV_GET_ADDR_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;

    CsrBtCmMsgTransport(prim);

}



#endif /* End of CSR_BT_INSTALL_EXTENDED_ADVERTISING */


#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanGetGlobalParamsReqSend
 *
 *  DESCRIPTION
 *      Read the global parameters to be used during extended scanning.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *----------------------------------------------------------------------------*/
void CmExtScanGetGlobalParamsReqSend(CsrSchedQid appHandle)
{
    CmExtScanGetGlobalParamsReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtScanGetGlobalParamsReq));
    prim->type = CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_REQ;
    prim->appHandle = appHandle;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanSetGlobalParamsReqSend
 *
 *  DESCRIPTION
 *      Write the global parameters to be used during extended scanning.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      flags                Scanning flags
 *      own_address_type     Local address type
 *      scanning_filter_policy Scanning filter policy
 *      filter_duplicates    Filter duplicates
 *      scanning_phys        Bit Field for LE PHYs to be used for Scanning
 *      phys                 Scan Parameter Values to be used for each LE PHYs
 *----------------------------------------------------------------------------*/
void CmExtScanSetGlobalParamsReqSend(CsrSchedQid appHandle,
                                                CsrUint8 flags,
                                                CsrUint8 own_address_type,
                                                CsrUint8 scanning_filter_policy,
                                                CsrUint8 filter_duplicates,
                                                CsrUint16 scanning_phys,
                                                CmScanningPhy *phys)
{
    CmExtScanSetGlobalParamsReq *prim;
    CsrUint8 index = 0;

    prim = CsrPmemAlloc(sizeof(CmExtScanSetGlobalParamsReq));
    prim->type = CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_REQ;
    prim->appHandle = appHandle;
    prim->flags = flags;
    prim->own_address_type = own_address_type;
    prim->scanning_filter_policy = scanning_filter_policy;
    prim->filter_duplicates = filter_duplicates;
    prim->scanning_phy = scanning_phys;
    if (CM_EXT_SCAN_LE_1M_PHY_BIT_MASK & scanning_phys)
    {
        prim->phys[index] = phys[index];
        index++;
    }
    if (CM_EXT_SCAN_LE_CODED_PHY_BIT_MASK & scanning_phys)
        prim->phys[index] = phys[index];

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanRegisterScannerReqSend
 *
 *  DESCRIPTION
 *      Register a scanner and filter rules to be used during extended 
 *      scanning.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      flags                Scanning flags
 *      adv_filter           Advertising filter
 *      adv_filter_sub_field1 Advertising filter sub fields 1
 *      adv_filter_sub_field2 Advertising filter sub fields 2
 *      ad_structure_filter  AD structure filter
 *      ad_structure_filter_sub_field1 AD structure filter sub field 1
 *      ad_structure_filter_sub_field2 AD structure filter sub field 2
 *      num_reg_ad_types     Num of registered AD Types
 *      reg_ad_types         Registered AD Types
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
                                                CsrUint8 *reg_ad_types)
{
    CmExtScanRegisterScannerReq *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(CmExtScanRegisterScannerReq));
    prim->type = CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_REQ;
    prim->appHandle = appHandle;
    prim->flags = flags;
    prim->adv_filter = adv_filter;
    prim->adv_filter_sub_field1 = adv_filter_sub_field1;
    prim->adv_filter_sub_field2 = adv_filter_sub_field2;
    prim->ad_structure_filter = ad_structure_filter;
    prim->ad_structure_filter_sub_field1 = ad_structure_filter_sub_field1;
    prim->ad_structure_filter_sub_field2 = ad_structure_filter_sub_field2;
    prim->num_reg_ad_types = (num_reg_ad_types <= CM_EXT_SCAN_MAX_REG_AD_TYPES) ?
        num_reg_ad_types : CM_EXT_SCAN_MAX_REG_AD_TYPES;
    for (i = 0; i < prim->num_reg_ad_types; i++)
    {
        prim->reg_ad_types[i] = reg_ad_types[i];
    }

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanUnregisterScannerReqSend
 *
 *  DESCRIPTION
 *      Unregister the scanner registered with the passed scan handle.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      scan_handle          Scan handle of the registered scanner
 *----------------------------------------------------------------------------*/
void CmExtScanUnregisterScannerReqSend(CsrSchedQid appHandle,
                                                CsrUint8 scan_handle)
{
    CmExtScanUnregisterScannerReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtScanUnregisterScannerReq));
    prim->type = CSR_BT_CM_EXT_SCAN_UNREGISTER_SCANNER_REQ;
    prim->appHandle = appHandle;
    prim->scan_handle = scan_handle;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanConfigureScannerReqSend
 *
 *  DESCRIPTION
 *      Configure the registered scanner.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      scan_handle          Scan handle of the registered scanner
 *      use_only_global_params Use only global params flag
 *      scanning_phys        Bit Field for LE PHYs to be used for Scanning
 *      phys                 Scan Parameter Values to be used for each LE PHYs
 *----------------------------------------------------------------------------*/
void CmExtScanConfigureScannerReqSend(CsrSchedQid appHandle,
                                                CsrUint8 scan_handle,
                                                CsrUint8 use_only_global_params,
                                                CsrUint16 scanning_phys,
                                                CmScanPhy *phys)
{
    CmExtScanConfigureScannerReq *prim;
    CsrUint8 index = 0;
    prim = CsrPmemAlloc(sizeof(CmExtScanConfigureScannerReq));
    prim->type = CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_REQ;
    prim->appHandle = appHandle;
    prim->scan_handle = scan_handle;
    prim->use_only_global_params = use_only_global_params;
    prim->scanning_phys = scanning_phys;
    if (CM_EXT_SCAN_LE_1M_PHY_BIT_MASK & scanning_phys)
    {
        prim->phys[index] = phys[index];
        index++;
    }
    if (CM_EXT_SCAN_LE_CODED_PHY_BIT_MASK & scanning_phys)
        prim->phys[index] = phys[index];

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanEnableScannersReqSend
 *
 *  DESCRIPTION
 *      Enable or Disable registered scanner
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      enable               Scan enable or disable
 *      num_of_scanners      Number of scanners
 *      scanners             Scan handle and duration of the registered scanners
 *----------------------------------------------------------------------------*/
void CmExtScanEnableScannersReqSend(CsrSchedQid appHandle,
                                                CsrUint8 enable,
                                                CsrUint8 num_of_scanners,
                                                CmScanners *scanners)
{
    CmExtScanEnableScannersReq *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(CmExtScanEnableScannersReq));
    prim->type = CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_REQ;
    prim->appHandle = appHandle;
    prim->enable = enable;
    prim->num_of_scanners = (num_of_scanners <= CM_EXT_SCAN_MAX_SCANNERS) ?
        num_of_scanners : CM_EXT_SCAN_MAX_SCANNERS;
    for (i = 0; i < prim->num_of_scanners; i++)
    {
        prim->scanners[i] = scanners[i];
    }

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmExtScanGetCtrlScanInfoReqSend
 *
 *  DESCRIPTION
 *      Get Controller's scan configuration information
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *----------------------------------------------------------------------------*/
void CmExtScanGetCtrlScanInfoReqSend(CsrSchedQid appHandle)
{
    CmExtScanGetCtrlScanInfoReq *prim;

    prim = CsrPmemAlloc(sizeof(CmExtScanGetCtrlScanInfoReq));
    prim->type = CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ;
    prim->appHandle = appHandle;

    CsrBtCmMsgTransport(prim);
}

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
                                      CmInitiatingPhy *initPhys[])
{
    CmDmExtSetConnParamsReq *prim;
    CsrUint8 count;

    prim = CsrPmemAlloc(sizeof(CmDmExtSetConnParamsReq));
    prim->type = CM_DM_EXT_SET_CONN_PARAMS_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->subevent = subevent;
    prim->connAttemptTimeout = connAttemptTimeout;
    prim->ownAddressType = ownAddressType;
    prim->phyCount = phyCount;

    for (count = 0; count < phyCount; count++)
    {
        prim->initPhys[count] = initPhys[count];
    }

    CsrBtCmMsgTransport(prim);
}
#endif /* End of INSTALL_CM_EXT_SET_CONN_PARAM */
#endif /* End of CSR_BT_INSTALL_EXTENDED_SCANNING */

#ifdef INSTALL_CM_DM_CONFIGURE_DATA_PATH
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDmConfigureDataPathReqSend
 *
 *  DESCRIPTION
 *       Configure data path in a given direction between the Controller and the
 *  Host.
 *
 *  PARAMETERS
 *      appHandle:               Application handle
 *      dataPathDirection        Data Path direction
 *      dataPathId               Data Path ID
 *      vendorSpecificConfigLen  Total length of vendor specific config in octets
 *      vendorSpecificConfig     Vendor Specific Configuration data.
 *                               Each index can hold maximum 
 *                               HCI_CONFIGURE_DATA_PATH_PER_PTR octets
 *----------------------------------------------------------------------------*/
void CmDmConfigureDataPathReqSend(CsrSchedQid appHandle,
                                      CsrUint8 dataPathDirection,
                                      CsrUint8 dataPathId,
                                      CsrUint8 vendorSpecificConfigLen,
                                      CsrUint8 *vendorSpecificConfig[])
{
    CmDmConfigureDataPathReq *prim;
    CsrUint8 index;

    prim = CsrPmemZalloc(sizeof(CmDmConfigureDataPathReq));

    prim->type = CM_DM_HCI_CONFIGURE_DATA_PATH_REQ;
    prim->appHandle = appHandle;
    prim->dataPathDirection = dataPathDirection;
    prim->dataPathId = dataPathId;
    prim->vendorSpecificConfigLen = vendorSpecificConfigLen;

    if(vendorSpecificConfig != NULL)
    {
        for (index = 0; index < CM_DM_CONFIGURE_DATA_PATH_MAX_INDEX; index++)
        {
            if (vendorSpecificConfig[index] != NULL)
            {
                prim->vendorSpecificConfig[index] = vendorSpecificConfig[index];
            }
        }
    }

    CsrBtCmMsgTransport(prim);
}
#endif /* End of INSTALL_CM_DM_CONFIGURE_DATA_PATH */

#ifdef INSTALL_CM_DM_LE_READ_CHANNEL_MAP
void CmDmLeReadChannelMapReqSend(CsrSchedQid appHandle,
                                      TYPED_BD_ADDR_T peerAddr)
{
    CmDmLeReadChannelMapReq *prim;

    prim = CsrPmemZalloc(sizeof(*prim));

    prim->type = CM_DM_LE_READ_CHANNEL_MAP_REQ;
    prim->appHandle = appHandle;
    CsrBtAddrCopy(&(prim->peerAddr), &(peerAddr));

    CsrBtCmMsgTransport(prim);
}
#endif /* End of INSTALL_CM_DM_LE_READ_CHANNEL_MAP */

#ifdef CSR_BT_INSTALL_PERIODIC_ADVERTISING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvSetParamsReqSend
 *
 *  DESCRIPTION
 *      Set the periodic advertising parameters for the registered advertising set.
 *
 *  PARAMETERS
 *      appHandle:                 Application handle
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
                                                CsrUint16 periodicAdvProperties)
{
    CmPeriodicAdvSetParamsReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicAdvSetParamsReq));
    prim->type = CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->flags = flags;
    prim->periodicAdvIntervalMin = periodicAdvIntervalMin;
    prim->periodicAdvIntervalMax = periodicAdvIntervalMax;
    prim->periodicAdvProperties = periodicAdvProperties;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvSetDataReqSend
 *
 *  DESCRIPTION
 *      Set the periodic advertising data.
 *
 *  PARAMETERS
 *      appHandle:        Application handle
 *      advHandle         A registered advertising set value
 *      operation         Part of the data to be set. 0 : Intermittent Fragment,
 *                        1 : First Fragment, 2 : Last Fragment, 3 : Complete Data
 *      dataLen           Advertising data length (0 to 251 octets).
 *      data              Array of pointers to a 32 octet buffer. Max size is 8.
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvSetDataReqSend(CsrSchedQid appHandle, 
                                                CsrUint8 advHandle,
                                                CsrUint8 operation,
                                                CsrUint8 dataLength,
                                                CsrUint8 *data[])
{
    CsrUint8 i;
    CmPeriodicAdvSetDataReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicAdvSetDataReq));
    CsrMemSet(prim, 0, sizeof(CmPeriodicAdvSetDataReq));
    prim->type = CSR_BT_CM_PERIODIC_ADV_SET_DATA_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->operation = operation;
    prim->dataLength = (dataLength < CM_PERIODIC_ADV_DATA_LENGTH_MAX) ? 
        dataLength : CM_PERIODIC_ADV_DATA_LENGTH_MAX;
    for (i = 0; i < CM_PERIODIC_ADV_DATA_BYTES_PTRS_MAX; i++)
    {
        prim->data[i] = data[i];
    }

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvReadMaxAdvDataLenReqSend
 *
 *  DESCRIPTION
 *      Read the max allowed periodic advertising data for an advertising set.
 *
 *  PARAMETERS
 *      appHandle:        Application handle
 *      advHandle         A registered advertising set value
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvReadMaxAdvDataLenReqSend(CsrSchedQid appHandle, CsrUint8 advHandle)
{
    CmPeriodicAdvReadMaxAdvDataLenReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicAdvReadMaxAdvDataLenReq));
    prim->type = CSR_BT_CM_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvStartReqSend
 *
 *  DESCRIPTION
 *      Starts a periodic advertising train.
 *
 *  PARAMETERS
 *      appHandle:        Application handle
 *      advHandle         A registered advertising set value
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvStartReqSend(CsrSchedQid appHandle, CsrUint8 advHandle)
{
    CmPeriodicAdvStartReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicAdvStartReq));
    prim->type = CSR_BT_CM_PERIODIC_ADV_START_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvStopReqSend
 *
 *  DESCRIPTION
 *      Stops a periodic advertising train or just the associated extended advertising.
 *
 *  PARAMETERS
 *      appHandle:        Application handle
 *      advHandle         A registered advertising set value
 *      stopAdvertising   Stop periodic advertising and associated extended 
 *                        advertising
 *                        0 - stop periodic advertising,
 *                        1 - stop extended advertising and then stop periodic advertising
 *                        2 - stop extended advertising
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvStopReqSend(CsrSchedQid appHandle, CsrUint8 advHandle, CsrUint8 stopAdvertising)
{
    CmPeriodicAdvStopReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicAdvStopReq));
    prim->type = CSR_BT_CM_PERIODIC_ADV_STOP_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->stopAdvertising = stopAdvertising;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvEnableReqSend
 *
 *  DESCRIPTION
 *      Enables a periodic advertising.
 *
 *  PARAMETERS
 *      appHandle:        Application handle
 *      advHandle         A registered advertising set value
 *      flags
 *      enable
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvEnableReqSend(CsrSchedQid appHandle, CsrUint8 advHandle, CsrUint16 flags, CsrUint8 enable)
{
    CmPeriodicAdvEnableReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicAdvEnableReq));
    prim->type = CSR_BT_CM_PERIODIC_ADV_ENABLE_REQ;
    prim->phandle= appHandle;
    prim->adv_handle = advHandle;
    prim->flags = flags;
    prim->enable = enable;

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicAdvSetTransferReqSend
 *
 *  DESCRIPTION
 *      Send synchronization information about the periodic advertising in an 
 *      advertising set to a connected peer device.
 *
 *  PARAMETERS
 *      appHandle         Application handle
 *      addrt             Connected peer device address
 *      serviceData       service data.
 *      advHandle         a registered advertising set value
 *----------------------------------------------------------------------------*/
void CmPeriodicAdvSetTransferReqSend(CsrSchedQid appHandle, 
                                    TYPED_BD_ADDR_T addrt,
                                    CsrUint16 serviceData,
                                    CsrUint8 advHandle)
{
    CmPeriodicAdvSetTransferReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicAdvSetTransferReq));
    prim->type = CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_REQ;
    prim->phandle = appHandle;
    prim->addrt = addrt;
    prim->serviceData = serviceData;
    prim->advHandle = advHandle;

    CsrBtCmMsgTransport(prim);
}

#endif /* End of CSR_BT_INSTALL_PERIODIC_ADVERTISING */

#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanStartFindTrainsReqSend
 *
 *  DESCRIPTION
 *      Search for periodic trains that meet a specified ad_structure filter.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
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
                                        CsrUint8* adStructureInfo)
{
    CmPeriodicScanStartFindTrainsReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicScanStartFindTrainsReq));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_START_FIND_TRAINS_REQ;
    prim->phandle = appHandle;
    prim->flags = flags;
    prim->scanForXSeconds = scanForXSeconds;
    prim->adStructureFilter = adStructureFilter;
    prim->adStructureFilterSubField1 = adStructureFilterSubField1;
    prim->adStructureFilterSubField2 = adStructureFilterSubField2;
    prim->adStructureInfoLen = adStructureInfoLen;
    prim->adStructureInfo = adStructureInfo;

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanStopFindTrainsReqSend
 *
 *  DESCRIPTION
 *      Stop scanning for periodic trains.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      scanHandle           Scan handle return in DM_ULP_PERIODIC_SCAN_START_FIND_TRAINS_CFM
 *----------------------------------------------------------------------------*/
void CmPeriodicScanStopFindTrainsReqSend(CsrSchedQid appHandle,
                                        CsrUint8 scanHandle)
{
    CmPeriodicScanStopFindTrainsReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicScanStopFindTrainsReq));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_STOP_FIND_TRAINS_REQ;
    prim->phandle = appHandle;
    prim->scanHandle = scanHandle;

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncToTrainReqSend
 *
 *  DESCRIPTION
 *      Establish sync to one of the periodic trains in the primitive.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      reportPeriodic       Report periodic advetisement data [0 - no, 1 - yes]
 *      skip                 Max number of periodic advertising events that can be skipped after a successful receive.
 *      syncTimeout          Synchronization timeout for the periodic advertising train once synchronised. [0x000A to 0x4000 (Time = n * 10 ms)]
 *      syncCteType          Always set to 0. RFU
 *      attemptSyncForXSeconds Always set to 0. RFU
 *      numberOfPeriodicTrains Number of periodic train records. [Min - 1, Max - 3]
 *      periodicTrains       Periodic trains info i.e. ADV SID and TYPED_BD_ADDR_T
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncToTrainReqSend(CsrSchedQid appHandle,
                                        CsrUint8 reportPeriodic,
                                        CsrUint16 skip,
                                        CsrUint16 syncTimeout,
                                        CsrUint8 syncCteType,
                                        CsrUint16 attemptSyncForXSeconds,
                                        CsrUint8 numberOfPeriodicTrains,
                                        CmPeriodicScanTrains *periodicTrains)
{
    CmPeriodicScanSyncToTrainReq *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(CmPeriodicScanSyncToTrainReq));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_REQ;
    prim->phandle = appHandle;
    prim->reportPeriodic = reportPeriodic;
    prim->skip = skip;
    prim->syncTimeout = syncTimeout;
    prim->syncCteType = syncCteType;
    prim->attemptSyncForXSeconds = attemptSyncForXSeconds;
    prim->numberOfPeriodicTrains = 
        (numberOfPeriodicTrains <= CM_MAX_PERIODIC_TRAIN_LIST_SIZE) ?
        numberOfPeriodicTrains : CM_MAX_PERIODIC_TRAIN_LIST_SIZE;
    for (i = 0; i < prim->numberOfPeriodicTrains; i++)
    {
        prim->periodicTrains[i] = periodicTrains[i];
    }

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncToTrainCancelReqSend
 *
 *  DESCRIPTION
 *      Cancel an attempt to synchronise on to a periodic train.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncToTrainCancelReqSend(CsrSchedQid appHandle)
{
    CmPeriodicScanSyncToTrainCancelReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicScanSyncToTrainCancelReq));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_REQ;
    prim->phandle = appHandle;

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncAdvReportEnableReqSend
 *
 *  DESCRIPTION
 *      Enable or disable receiving of periodic advertising data report i.e. 
 *      DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_IND  events.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      syncHandle           Sync handle for the periodic train synced by controller
 *      enable               Enable (1) or disable (0) periodic advertising data report
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncAdvReportEnableReqSend(CsrSchedQid appHandle,
                                            CsrUint16 syncHandle,
                                            CsrUint8 enable)
{
    CmPeriodicScanSyncAdvReportEnableReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicScanSyncAdvReportEnableReq));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_REQ;
    prim->phandle = appHandle;
    prim->syncHandle = syncHandle;
    prim->enable = enable;

    CsrBtCmMsgTransport(prim);

}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncTerminateReqSend
 *
 *  DESCRIPTION
 *      Terminate sync to a currently synced periodic train.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      syncHandle           Sync handle for the periodic train synced by controller
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncTerminateReqSend(CsrSchedQid appHandle,
                                            CsrUint16 syncHandle)
{
    CmPeriodicScanSyncTerminateReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicScanSyncTerminateReq));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_REQ;
    prim->phandle = appHandle;
    prim->syncHandle = syncHandle;

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncLostRspSend
 *
 *  DESCRIPTION
 *      Application inform the Periodic Scanning Sync manager to stop reading
 *      periodic train advert data for this train.
 *      Must be sent when a DM_HCI_ULP_PERIODIC_SCAN_SYNC_LOSS_IND is received.
 *
 *  PARAMETERS
 *      syncHandle           Sync handle for the periodic train synced by controller
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncLostRspSend(CsrUint16 syncHandle)
{
    CmPeriodicScanSyncLostRsp *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicScanSyncTerminateReq));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_RSP;
    prim->syncHandle = syncHandle;

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncTransferReqSend
 *
 *  DESCRIPTION
 *      Send synchronization information about the periodic advertising train
 *      identified by the Sync Handle parameter to a connected device.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      addrt                Typed BD Addr of the connected peer device
 *      serviceData          Always set to 0. RFU
 *      syncHandle           Sync handle for the periodic train synced by controller
 *----------------------------------------------------------------------------*/
void CmPeriodicScanSyncTransferReqSend(CsrSchedQid appHandle,
                                            TYPED_BD_ADDR_T addrt,
                                            CsrUint16 serviceData,
                                            CsrUint16 syncHandle)
{
    CmPeriodicScanSyncTransferReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicScanSyncTransferReq));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_REQ;
    prim->phandle = appHandle;
    prim->addrt = addrt;
    prim->serviceData= serviceData;
    prim->syncHandle = syncHandle;

    CsrBtCmMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CmPeriodicScanSyncTransferParamsReqSend
 *
 *  DESCRIPTION
 *      Send synchronization information about the periodic advertising train
 *      identified by the Sync Handle parameter to a connected device.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
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
                                            CsrUint8 cteType)
{
    CmPeriodicScanSyncTransferParamsReq *prim;

    prim = CsrPmemAlloc(sizeof(CmPeriodicScanSyncTransferParamsReq));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_REQ;
    prim->phandle = appHandle;
    prim->addrt = addrt;
    prim->skip = skip;
    prim->syncTimeout = syncTimeout;
    prim->mode = mode;
    prim->cteType = cteType;

    CsrBtCmMsgTransport(prim);
}

#endif /* End of CSR_BT_INSTALL_PERIODIC_SCANNING */

#ifdef CSR_BT_INSTALL_CRYPTO_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoGeneratePublicPrivateKeyReqSend
 *
 *  DESCRIPTION
 *      Request for the generation of a public/
 *      private encryption key pair using ECC
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      keyType              Type of encryption required
                             CSR_BT_CM_CRYPTO_ECC_P192/CSR_BT_CM_CRYPTO_ECC_P256
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoGeneratePublicPrivateKeyReqSend(CsrSchedQid appHandle,
                                                  CsrBtCmCryptoEccType keyType)
{
    CsrBtCmCryptoGeneratePublicPrivateKeyReq *prim = CsrPmemZalloc(sizeof(CsrBtCmCryptoGeneratePublicPrivateKeyReq));

    prim->type      = CSR_BT_CM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_REQ;
    prim->appHandle = appHandle;
    prim->keyType   = keyType;

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoGenerateSharedSecretKeyReqSend
 *
 *  DESCRIPTION
 *      Request for the generation of a shared secret encryption key 
 *      from a public/private key pair using ECHD
 *
 *  PARAMETERS
 *      appHandle:           Application handle
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
                                                 CsrUint16 *publicKey)
{
    CsrBtCmCryptoGenerateSharedSecretKeyReq *prim = CsrPmemZalloc(sizeof(CsrBtCmCryptoGenerateSharedSecretKeyReq));

    prim->type      = CSR_BT_CM_CRYPTO_GENERATE_SHARED_SECRET_KEY_REQ;
    prim->appHandle = appHandle;
    prim->keyType   = keyType;

    if (privateKey && publicKey)
    {
        SynMemMoveS(prim->privateKey, sizeof(prim->privateKey), privateKey, sizeof(prim->privateKey));
        SynMemMoveS(prim->publicKey, sizeof(prim->publicKey), publicKey, sizeof(prim->publicKey));
    }

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoEncryptReqSend
 *
 *  DESCRIPTION
 *      Request for the encryption of a 128-bitblock of data 
 *      using a 128-bit encryption key using AES
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      dataArray            Array containing the data to be encrypted
 *                           The array must be 8 words large (16 bytes)
 *      keyArray             Key array used to encrypt the given data
 *                           The array must be 8 words large (16 bytes)
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoEncryptReqSend(CsrSchedQid appHandle,
                                 CsrUint16 *dataArray,
                                 CsrUint16 *keyArray)
{
    CsrBtCmCryptoEncryptReq *prim = CsrPmemZalloc(sizeof(CsrBtCmCryptoEncryptReq));

    prim->type      = CSR_BT_CM_CRYPTO_ENCRYPT_REQ;
    prim->appHandle = appHandle;

    if (dataArray && keyArray)
    {
        SynMemMoveS(prim->dataArray, sizeof(prim->dataArray), dataArray, sizeof(prim->dataArray));
        SynMemMoveS(prim->keyArray, sizeof(prim->keyArray), keyArray, sizeof(prim->keyArray));
    }

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoHashReqSend
 *
 *  DESCRIPTION
 *      Request for the hashing of a data block 
 *      of arbitrary length using SHA256
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      dataArray            Array containing the data to be hashed
 *                           The array is of variable length
 *      arraySize            Size of the data array in octets
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoHashReqSend(CsrSchedQid appHandle,
                              CsrUint16 *dataArray,
                              CsrUint16 arraySize)
{
    CsrBtCmCryptoHashReq *prim = CsrPmemZalloc(sizeof(CsrBtCmCryptoHashReq));

    prim->type      = CSR_BT_CM_CRYPTO_HASH_REQ;
    prim->appHandle = appHandle;
    prim->arraySize = arraySize;

    if (dataArray && arraySize)
    {
        prim->dataArray = CsrPmemAlloc(arraySize);
        SynMemMoveS(prim->dataArray, arraySize, dataArray, arraySize);
    }
    else
    {
        prim->dataArray = NULL;
    }

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoHashContinueReqSend
 *
 *  DESCRIPTION
 *      Request the continuation of the SHA256 encryption operation 
 *      for the remaining data
 *
 *  PARAMETERS
 *      appHandle:           Application handle
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
                                      CsrUint16 currentIndex)
{
    CsrBtCmCryptoHashReq *prim = CsrPmemZalloc(sizeof(CsrBtCmCryptoHashReq));

    prim->type         = CSR_BT_CM_CRYPTO_HASH_REQ;
    prim->appHandle    = appHandle;
    prim->arraySize    = arraySize;
    prim->currentIndex = currentIndex;

    if (dataArray && arraySize)
    {
        prim->dataArray = CsrPmemAlloc(arraySize);
        SynMemMoveS(prim->dataArray, arraySize, dataArray, arraySize);
    }
    else
    {
        prim->dataArray = NULL;
    }

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoDecryptReqSend
 *
 *  DESCRIPTION
 *      Request for the decryption of a 128-bitblock of data 
 *      using a 128-bit decryption key using AES
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *      dataArray            Array containing the data to be encrypted
 *                           The array must be 8 words large (16 bytes)
 *      keyArray             Key array used to encrypt the given data
 *                           The array must be 8 words large (16 bytes)
 *----------------------------------------------------------------------------*/
void CsrBtCmCryptoDecryptReqSend(CsrSchedQid appHandle,
                                 CsrUint16 *dataArray,
                                 CsrUint16 *keyArray)
{
    CsrBtCmCryptoDecryptReq *prim = CsrPmemZalloc(sizeof(CsrBtCmCryptoDecryptReq));

    prim->type      = CSR_BT_CM_CRYPTO_DECRYPT_REQ;
    prim->appHandle = appHandle;

    if (dataArray && keyArray)
    {
        SynMemMoveS(prim->dataArray, sizeof(prim->dataArray), dataArray, sizeof(prim->dataArray));
        SynMemMoveS(prim->keyArray, sizeof(prim->keyArray), keyArray, sizeof(prim->keyArray));
    }

    CsrBtCmMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCryptoAesCtrReqSend
 *
 *  DESCRIPTION
 *      Encrypt or decrypt data using the AES128-CTR algorithm
 *
 *  PARAMETERS
 *      appHandle:           Application handle
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
                                CsrUint16 *data)
{
    CsrBtCmCryptoAesCtrReq *prim = CsrPmemZalloc(sizeof(CsrBtCmCryptoAesCtrReq));

    prim->type      = CSR_BT_CM_CRYPTO_AES_CTR_REQ;
    prim->appHandle = appHandle;
    prim->counter   = counter;
    prim->flags     = flags;
    prim->dataLen   = dataLen;

    if (secretKey && nonce && data && dataLen)
    {
        SynMemMoveS(prim->secretKey, sizeof(prim->secretKey), secretKey, sizeof(prim->secretKey));
        SynMemMoveS(prim->nonce, sizeof(prim->nonce), nonce, sizeof(prim->nonce));
        prim->data = CsrPmemAlloc(dataLen * sizeof(CsrUint16));
        SynMemMoveS(prim->data, dataLen * sizeof(CsrUint16), data, dataLen * sizeof(CsrUint16));
    }
    else
    {
        prim->data = NULL;
    }

    CsrBtCmMsgTransport(prim);
}
#endif /* End of CSR_BT_INSTALL_CRYPTO_SUPPORT */

CsrBtCmDmPowerSettingsReq *CsrBtCmDmPowerSettingsReq_struct(CsrBtDeviceAddr addr,
                                                            CsrUint8 powerTableSize,
                                                            CsrBtCmPowerSetting *powerTable)
{
    CsrBtCmDmPowerSettingsReq *prim = (CsrBtCmDmPowerSettingsReq*) CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_BT_CM_DM_POWER_SETTINGS_REQ;
    prim->addr = addr;
    prim->powerTableSize = powerTableSize;
    prim->powerTable = powerTable;

    return (prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caTpConnectReq_struct
 *
 *  DESCRIPTION
 *      Application uses this API to establish LE Connection Oriented 
 *      Channel (COC) over L2CAP.
 *
 *      Note: The support for BR/EDR transport is yet to be added.
 *            Currently, it rejects the BR/EDR transport request.
 *            Use it for LE connection establishment for now.
 *
 *  PARAMETERS
 *        appHandle:          Protocol handle
 *        tpdAddrT:           BT address with transport type of device to bonded with
 *        localPsm:           The local PSM channel
 *        remotePsm:          Remote PSM channel to connect to
 *        localMtu:           Requested Local MTU size (bytes) (Incoming PDU)
 *        transmitMtu         Maximum payload size (bytes) (Outgoing MTU)
 *                            In the case that transmitMtu < than the Mtu
 *                            received from the remote device in a L2CA_CONFIG_IND
 *        dataPriority        Setting Data Priority
 *        secLevel:           Level of security to be applied
 *        minEncKeySize       Minimum encryption key size
 *        credits             Initial Credit value in the range 0 to 0xFFFF
 *        connFlags           Connection flags
 *        context             Context value (passed back in the confirm)
 *----------------------------------------------------------------------------*/
CmL2caTpConnectReq *CmL2caTpConnectReq_struct(CsrSchedQid       appHandle,
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
                                                    CsrUint16           context)
{
    CmL2caTpConnectReq *prim;
    CsrUint16 *conftab;
    CsrUint16 idx;

    prim            = (CmL2caTpConnectReq *) CsrPmemAlloc(sizeof(CmL2caTpConnectReq));
    prim->type      = CM_L2CA_TP_CONNECT_REQ;
    prim->tpdAddrT  = tpdAddrT;
    prim->phandle   = appHandle;
    prim->localPsm  = localPsm;
    prim->remotePsm = remotePsm;
    prim->secLevel  = secLevel;
    prim->context   = context;;
    prim->minEncKeySize = minEncKeySize;
    conftab = NULL;
    idx = 0;
    CsrBtCmL2caConftabInit(&conftab, &idx);
    CsrBtCmL2caConftabMtu(conftab, &idx, TRUE, localMtu); /* incoming */
    CsrBtCmL2caConftabMtu(conftab, &idx, FALSE, transmitMtu);  /* outgoing */
    CmL2caConftabCredits(conftab, &idx, credits);  /* credits */
    CmL2caConftabConnFlags(conftab, &idx, connFlags);  /* conn flags */
    CsrBtCmL2caConftabHighDataPriority(conftab, &idx, dataPriority); /* data priority */
    CsrBtCmL2caConftabFinalize(conftab, &idx);
    CsrBtCmL2caConftabCull(&conftab, &idx);

    prim->conftab = conftab;
    prim->conftabCount = idx;

    return prim;
}


/*----------------------------------------------------------------------------*
 *  NAME
 *     CsrBtCml2caAddCreditReq_struct
 *
 *  DESCRIPTION
 *      Application uses this API to increase the credits of LE L2CAP Connection 
 *      Oriented Channel (COC) established with the remote device.
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
                                                    CsrUint16           credits)
{
    CmL2caAddCreditReq        *prim;

    prim                    = (CmL2caAddCreditReq *) CsrPmemAlloc(sizeof(CmL2caAddCreditReq));
    prim->type              = CM_L2CA_ADD_CREDIT_REQ;
    prim->phandle           = appHandle;
    prim->btConnId          = btConnId;
    prim->context           = context;
    prim->credits           = credits;
    return(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmL2caTpConnectAcceptRsp_struct
 *
 *  DESCRIPTION
 *      Application uses this API to accept or reject the L2CAP Connection Oriented
 *      Channel (COC) establishment indicated by CM_L2CA_TP_CONNECT_ACCEPT_IND event.
 *
 *  PARAMETERS
 *        appHandle:          Protocol handle
 *        accept:             Accept or reject the incoming L2CAP connect request
 *        btConnId            Connection ID
 *        localPsm:           The local PSM channel
 *        tpdAddrT:           BT address with transport type of device to bonded with
 *        identfier:          Remote PSM channel to connect to
 *        localMtu:           Requested Local MTU size (bytes) (Incoming PDU)
 *        transmitMtu         Maximum payload size (bytes) (Outgoing PDU)
 *                            In the case that transmitMtu < than the Mtu
 *                            received from the remote device in a L2CA_CONFIG_IND
 *        minEncKeySize       Minimum encryption key size
 *        credits             Initial Credit value in the range 0 to 0xFFFF
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
                                                        CsrUint16           credits)

{
    CmL2caTpConnectAcceptRsp *prim;
    CsrUint16 *conftab;
    CsrUint16 idx;

    prim                 = (CmL2caTpConnectAcceptRsp *) CsrPmemAlloc(sizeof(CmL2caTpConnectAcceptRsp));
    prim->type           = CM_L2CA_TP_CONNECT_ACCEPT_RSP;
    prim->phandle        = appHandle;
    prim->accept         = accept;
    prim->btConnId       = btConnId;
    prim->localPsm       = localPsm;
    prim->tpdAddrT       = tpdAddrT;
    prim->identifier     = identifier;
    prim->minEncKeySize  = minEncKeySize;
    conftab = NULL;
    idx = 0;
    CsrBtCmL2caConftabInit(&conftab, &idx);
    CsrBtCmL2caConftabMtu(conftab, &idx, TRUE, localMtu); /* incoming */
    CsrBtCmL2caConftabMtu(conftab, &idx, FALSE, transmitMtu);  /* outgoing */
    CmL2caConftabCredits(conftab, &idx, credits);  /* credits */
    CmL2caConftabConnFlags(conftab, &idx, 0);  /* conn flags */
    CsrBtCmL2caConftabFinalize(conftab, &idx);
    CsrBtCmL2caConftabCull(&conftab, &idx);

    prim->conftab = conftab;
    prim->conftabCount = idx;

    return prim;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      Cml2caRegisterFixedCidReqSend
 *
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
                                                      L2CA_CONFIG_T config)
{
    CmL2caRegisterFixedCidReq    *prim;

    prim                        = (CmL2caRegisterFixedCidReq *) CsrPmemAlloc(sizeof(CmL2caRegisterFixedCidReq));
    prim->type                  = CM_L2CA_REGISTER_FIXED_CID_REQ;
    prim->phandle               = appHandle;
    prim->fixedCid              = fixedCid;
    prim->context               = context;
    prim->config                = config;
    return(prim);
}

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
                                                        CsrUint8 *data)
{
    CmL2caPingReq *prim;
    prim = (CmL2caPingReq*)CsrPmemAlloc(sizeof(CmL2caPingReq));
    prim->type = CM_L2CA_PING_REQ;
    prim->address = address;
    prim->appHandle = appHandle;
    prim->context = context;
    prim->flags = flags;
    prim->lengthData = lengthData;
    prim->data = data;
    return prim;
}

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
 *        flags                Unused at present
 *        lengthData           Length of data
 *        data                 Pointer to data
 *----------------------------------------------------------------------------*/
CmL2caPingRsp *CmL2caPingRsp_struct(CsrBdAddr address,
                                                        CsrUint8 identifier,
                                                        CsrUint16 lengthData,
                                                        CsrUint8 *data)
{
    CmL2caPingRsp *prim;
    prim = (CmL2caPingRsp*)CsrPmemAlloc(sizeof(CmL2caPingRsp));
    prim->type = CM_L2CA_PING_RSP;
    prim->address = address;
    prim->identifier = identifier;
    prim->lengthData = lengthData;
    prim->data = data;
    return prim;
}
#endif

#ifdef CSR_BT_INSTALL_CM_QHS_PHY_SUPPORT
void CsrBtCmQhsPhySet(CsrBtDeviceAddr *deviceAddr, CsrBool qhsPhyConnected)
{
    aclTable *aclData = returnAclTable(&csrBtCmData, deviceAddr);

    if(aclData)
    {
        aclData->qhsPhyConnected = qhsPhyConnected;
    }
}
#endif /* CSR_BT_INSTALL_CM_QHS_PHY_SUPPORT */

#ifdef CSR_BT_INSTALL_CM_SWB_DISABLE_STATE
void CmDisableSWBSupport(CsrBtDeviceAddr *deviceAddr)
{
    aclTable *aclData = returnAclTable(&csrBtCmData, deviceAddr);

    if(aclData)
    {
        aclData->swbDisabled = TRUE;
    }
}
#endif /* CSR_BT_INSTALL_CM_SWB_DISABLE_STATE */

#ifdef INSTALL_CM_REGISTER_APP_HANDLE
void CmRegisterApplicationHandle(CsrSchedQid appHandle, CsrBtConnId btConnId, CmProtocolType protocol)
{
    cmInstanceData_t *cmData = &csrBtCmData;

    if (protocol == CM_PROTOCOL_L2CAP)
    {
        cmL2caConnElement *l2caConnElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId,
                                                                  &btConnId);
        if (l2caConnElement && l2caConnElement->cmL2caConnInst)
        {
            l2caConnElement->cmL2caConnInst->appHandle = appHandle;
        }
    }
    else if (protocol == CM_PROTOCOL_RFCOMM)
    {
         cmRfcConnElement *rfcConnElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &btConnId);

         if (rfcConnElement && rfcConnElement->cmRfcConnInst)
         {
            rfcConnElement->cmRfcConnInst->appHandle = appHandle;
         }
    }
}
#endif /* INSTALL_CM_REGISTER_APP_HANDLE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caConnectReqSend
 *      CsrBtCmJsr82l2caConnectReqSend
 *      CsrBtCml2caConnectHighDataPriorityReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:          Protocol handle
 *        deviceAddr:         BT address of device to bonded with
 *        localPsm:           The local PSM channel
 *        remotePsm:          Remote PSM channel to connect to
 *        framesize:          Maximum frame size (bytes) (outgoing MTU)
 *        secLevel:           Level of security to be applied
 *        minEncKeySize       Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBtCmL2caConnectReq *CsrBtCml2caConnectReq_struct(CsrSchedQid         appHandle,
                                                    CsrBtDeviceAddr     deviceAddr,
                                                    psm_t               theLocalPsm,
                                                    psm_t               theRemotePsm,
                                                    l2ca_mtu_t          theFramesize,
                                                    dm_security_level_t theSecLevel,
                                                    l2ca_mtu_t          transmitMtu,
                                                    CsrUint16           dataPriority,
                                                    CsrUint8            minEncKeySize)
{
    CsrBtCmL2caConnectReq *prim;
    CsrUint16 *conftab;
    CsrUint16 idx;

    prim            = (CsrBtCmL2caConnectReq *) CsrPmemAlloc(sizeof(CsrBtCmL2caConnectReq));
    prim->type      = CSR_BT_CM_L2CA_CONNECT_REQ;
    prim->addr      = deviceAddr;
    prim->phandle   = appHandle;
    prim->localPsm  = theLocalPsm;
    prim->remotePsm = theRemotePsm;
    prim->secLevel  = theSecLevel;
    prim->context   = CSR_BT_CM_CONTEXT_UNUSED; 
    prim->minEncKeySize = minEncKeySize;
    conftab = NULL;
    idx = 0;
    CsrBtCmL2caConftabInit(&conftab, &idx);
    CsrBtCmL2caConftabFlushToAllowAnyPeer(conftab, &idx);
    CsrBtCmL2caConftabMtu(conftab, &idx, TRUE, theFramesize); /* incoming */
    CsrBtCmL2caConftabMtu(conftab, &idx, FALSE, transmitMtu);  /* outgoing */
    CsrBtCmL2caConftabHighDataPriority(conftab, &idx, (CsrUint8) dataPriority);
    CsrBtCmL2caConftabNoReconf(conftab, &idx);
    CsrBtCmL2caConftabFinalize(conftab, &idx);
    CsrBtCmL2caConftabCull(&conftab, &idx);

    prim->conftab = conftab;
    prim->conftabCount = idx;

    return prim;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmJsr82l2caConnectAcceptReqSend
 *      CsrBtCml2caConnectAcceptSecondaryReqSend
 *      CsrBtCml2caConnectAcceptHighDataPriorityReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      appHandle:          Protocol handle
 *      localPsm:           The local PSM channel
 *      classOfDevice:      The profile COD
 *      secLevel:           Level of security to be applied
 *      framesize:          Maximum frame size (bytes)
 *      profileUuid:        The local profile Uuid
 *      primaryAcceptor     If TRUE it makes the host controller connectable,
 *                          e.g write the cod value and set it in page scan.
 *                          If FALSE the host controller is not set connecable
 *                          and the SM queue is not used. E.g this message is
 *                          handle at once by the CM.
 *      minEncKeySize       Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBtCmL2caConnectAcceptReq *CsrBtCml2caConnectAcceptReq_struct(CsrSchedQid          appHandle,
                                                                psm_t               theLocalPsm,
                                                                CsrUint24           theClassOfDevice,
                                                                dm_security_level_t theSecLevel,
                                                                l2ca_mtu_t          theFramesize,
                                                                uuid16_t            theProfileUuid,
                                                                l2ca_mtu_t          transmitMtu,
                                                                CsrBool             primaryAcceptor,
                                                                CsrUint16           dataPriority,
                                                                CsrUint8            minEncKeySize)
{
    CsrBtCmL2caConnectAcceptReq *prim;
    CsrUint16 *conftab;
    CsrUint16 idx;

    prim                    = (CsrBtCmL2caConnectAcceptReq *) CsrPmemZalloc(sizeof(CsrBtCmL2caConnectAcceptReq));
    prim->type              = CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ;
    prim->phandle           = appHandle;
    prim->localPsm          = theLocalPsm;
    prim->classOfDevice     = theClassOfDevice;
    prim->secLevel          = theSecLevel;
    prim->profileUuid       = theProfileUuid;
    prim->primaryAcceptor   = primaryAcceptor;
    prim->context           = CSR_BT_CM_CONTEXT_UNUSED;
    prim->minEncKeySize     = minEncKeySize;

    conftab = NULL;
    idx = 0;
    CsrBtCmL2caConftabInit(&conftab, &idx);
    CsrBtCmL2caConftabFlushToAllowAnyPeer(conftab, &idx);
    CsrBtCmL2caConftabMtu(conftab, &idx, TRUE, theFramesize); /* incoming */
    CsrBtCmL2caConftabMtu(conftab, &idx, FALSE, transmitMtu);  /* outgoing */
    CsrBtCmL2caConftabHighDataPriority(conftab, &idx, (CsrUint8) dataPriority);
    CsrBtCmL2caConftabNoReconf(conftab, &idx);
    CsrBtCmL2caConftabFinalize(conftab, &idx);
    CsrBtCmL2caConftabCull(&conftab, &idx);

    prim->conftab = conftab;
    prim->conftabCount = idx;

    return prim;
}
