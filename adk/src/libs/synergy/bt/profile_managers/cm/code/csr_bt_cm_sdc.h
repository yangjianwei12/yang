#ifndef CSR_BT_CM_SDC_H__
#define CSR_BT_CM_SDC_H__
/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "sdc_prim.h"
#include "sds_prim.h"
#include "sdclib.h"
#include "sdslib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************************
 These function are found under cmSdpArrivalHandler.c
************************************************************************************/
void CsrBtCmSdcArrivalHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under cmSdcHandler.c
************************************************************************************/
CsrBool CsrBtCmSdcSearchListAddrCompare(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtCmSdcSearchListIndexItemCompare(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtCmSdcSearchListItemCompare(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtCmSdcSearchListElemRemove(CsrCmnListElm_t *elem, void *data);
void CsrBtCmSdcSearchListDeinit(CsrCmnList_t *searchList);

void CsrBtCmSdcSearchReqHandler(cmInstanceData_t *cmData);
void CmSdcServiceSearchAttrReqHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_CM_PRI_SDC
void CsrBtCmSdcServiceSearchReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSdcServiceSearchReqHandler NULL
#endif

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmSdcRfcSearchReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSdcRfcExtendedSearchReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSdcReleaseResourcesReqHandler(cmInstanceData_t *cmData);

#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
void CsrBtCmSdcUuid128RfcSearchReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSdcUuid128RfcSearchReqHandler NULL
#endif /* CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH */

#else /* EXCLUDE_CSR_BT_RFC_MODULE */
#define CsrBtCmSdcRfcSearchReqHandler           NULL
#define CsrBtCmSdcRfcExtendedSearchReqHandler   NULL
#define CsrBtCmSdcReleaseResourcesReqHandler    NULL
#define CsrBtCmSdcUuid128RfcSearchReqHandler    NULL
#endif /* !EXCLUDE_CSR_BT_RFC_MODULE */

void CsrBtCmSdcCancelSearchReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSdcAttrReqHandle(cmInstanceData_t *cmData);
void CsrBtCmSdcCloseReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSdsRegisterReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSdsUnRegisterReqHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
void cmSdsRegisterUuid128ReqHandler(cmInstanceData_t *cmData);
void cmSdsUnRegisterUuid128ReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSdcUuid128SearchReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSdcUuid128SearchReqHandler NULL
#endif

#ifdef CSR_BT_INSTALL_CM_PRI_SDC
void CsrBtCmSdcOpenReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSdcOpenReqHandler NULL
#endif

void CsrBtCmSdcOpenSearchCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmSdcServiceSearchCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmSdcServiceAttributeCfmHandler(cmInstanceData_t * cmData);
void CmSdcServiceSearchAttributeCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmSdcCloseSearchIndHandler(cmInstanceData_t * cmData);
void CsrBtCmSdsRegisterCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmSdsUnRegisterCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmSdcStartHandler(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr);

void cmSdcServiceSearchAttrFailureHandler(cmInstanceData_t *cmData);
#ifdef __cplusplus
}
#endif

#endif /* ndef _CM_SDP_H */
