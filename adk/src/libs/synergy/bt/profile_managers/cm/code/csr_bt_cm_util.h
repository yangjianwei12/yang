#ifndef CSR_BT_CM_UTIL_H__
#define CSR_BT_CM_UTIL_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_bt_cm_main.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************************
 These function are found under csr_bt_cm_profile_provider.c
************************************************************************************/
void CsrBtCmProfileProvider(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under csr_bt_cm_maintenance_handler.c
************************************************************************************/
void CsrBtCmPutMessage(CsrSchedQid phandle, void *msg);

CsrUint8 returnServerRegistrationIndex(cmInstanceData_t *cmData, CsrUint8 theServerChannel);
CsrUint8 CsrBtCmReturnNumOfConnectionsToPeerDevice(cmInstanceData_t *cmData, CsrBtDeviceAddr theAddr);
CsrUint8 CmReturnNumOfL2CRFCToPeerDevice(cmInstanceData_t *cmData, CsrBtDeviceAddr theAddr);
CsrBool CsrBtCmElementCounterIncrement(cmInstanceData_t *cmData);
void CsrBtCmInitInstData(cmInstanceData_t *cmData);
CsrUint24 CsrBtCmReturnClassOfdevice(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
CsrBool updateLogicalChannelTypeMaskAndNumber(cmInstanceData_t *cmData, CsrBtDeviceAddr *theAddr);
#endif

#ifdef CSR_BT_INSTALL_CM_HANDLE_REGISTER
void CsrBtCmRegisterHandlerReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmRegisterHandlerReqHandler                        NULL
#endif

#ifdef CSR_STREAMS_ENABLE
#define CsrBtCmDataBufferEmptyReqHandler            csrBtCmCommonDataPrimHandler
#else /* CSR_STREAMS_ENABLE */
void CsrBtCmDataBufferEmptyReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDataBufferEmptyCfmSend(CsrSchedQid appHandle, CsrUint16 context);
#endif /* !CSR_STREAMS_ENABLE */

#ifdef CSR_AMP_ENABLE
CsrUint8 CsrBtCmBtControllerActive(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr);
#endif

void CmHandlePendingPrim(cmInstanceData_t *cmData, 
                         CsrUint16 eventClass, 
                         CsrBtCmPrim event,
                         void* pContext,
                         CsrBool (*cmPendingMsgHandler)(cmInstanceData_t *cmData, void* msg, void* pContext),
                         void (*cmPendingMsgFree)(void* msg));
/*************************************************************************************
 These function are found under csr_bt_cm_service_manager_provider.c
************************************************************************************/
void CsrBtCmRestoreServiceManagerQueue(cmInstanceData_t *cmData);
void CsrBtCmServiceManagerLocalQueueHandler(cmInstanceData_t *cmData);
void CsrBtCmServiceManagerProvider(cmInstanceData_t *cmData);
void CsrBtCmServiceManagerCancelAcceptConnectProvider(cmInstanceData_t *cmData);
void CsrBtCmServiceManagerL2caConnectAcceptProvider(cmInstanceData_t *cmData);
void CsrBtCmServiceManagerL2caCancelConnectAcceptProvider(cmInstanceData_t *cmData);
CsrBool cancelServiceManagerMsg(cmInstanceData_t *cmData,
                               CsrBtCmPrim type,
                               CsrSchedQid phandle,
                               CsrBtDeviceAddr bd_addr,
                               CsrUint8 serverCh,
                               psm_t localPsm,
                               CsrUint8 *sdcServer,
                               CsrUint16     *context);
CsrBool CsrBtCmRemoveSavedOutgoingConnectMessage(cmInstanceData_t *cmData, CsrPrim type, CsrUint32 matchValue, CsrUint16 context);
CsrBool CsrBtCmCheckSavedIncomingConnectMessages(cmInstanceData_t *cmData, CsrBtDeviceAddr bd_addr);
void CsrBtCmRemoveSavedIncomingConnectMessages(cmInstanceData_t *cmData, CsrBtDeviceAddr bd_addr);

/*************************************************************************************
 These function are found under cm_sdc_provider.c
************************************************************************************/
void CmSdcManagerProvider(cmInstanceData_t *cmData);
void CmSdcManagerLocalQueueHandler(cmInstanceData_t *cmData);
void CmSdcRestoreSdcQueueHandler(cmInstanceData_t *cmData);
CsrBool CmSdcCancelSdcManagerMsg(cmInstanceData_t *cmData,
                                 CsrBtCmSdcCancelSearchReq *prim,
                                 CsrUint8 *sdcServer);
#ifdef CSR_TARGET_PRODUCT_VM
void CsrBtCmSetScoHandle(hci_connection_handle_t scoHandle);
#endif /* CSR_TARGET_PRODUCT_VM */

void CsrBtCmInitCompleted(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under csr_bt_cm_l2cap_connect_handler.c
************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

