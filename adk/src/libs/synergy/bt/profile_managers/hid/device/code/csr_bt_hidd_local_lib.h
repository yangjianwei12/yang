#ifndef CSR_BT_HIDD_LOCAL_LIB_H__
#define CSR_BT_HIDD_LOCAL_LIB_H__
/******************************************************************************
 Copyright (c) 2009-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_bt_result.h"
#include "csr_bt_profiles.h"
#include "csr_bt_hidd_prim.h"

#ifdef __cplusplus
extern "C" {
#endif

void CsrBtHiddMessagePut(CsrSchedQid phandle, void *msg);

void CsrBtHiddActivateCfmSend(HiddInstanceDataType *instData,
                                    CsrBtResultCode resultCode,
                                    CsrBtSupplier resultSupplier);
void CsrBtHiddDeactivateCfmSend(HiddInstanceDataType *instData,
                                       CsrBtResultCode resultCode,
                                       CsrBtSupplier resultSupplier);
void CsrBtHiddStatusIndSend(HiddInstanceDataType *instData,
                            CsrBtHiddStatusType status,
                            CsrBtConnId btConnId);
void CsrBtHiddControlIndSend(HiddInstanceDataType *instData, CsrBtHiddTransactionType transactionType, CsrBtHiddParameterType parameter, CsrUint16 dataLen, CsrUint8* data);
void CsrBtHiddDataCfmSend(HiddInstanceDataType *instData,
                                CsrBtResultCode resultCode,
                                CsrBtSupplier resultSupplier);
void CsrBtHiddDataIndSend(HiddInstanceDataType *instData, CsrBtHiddReportType reportType, CsrUint16 reportLen, CsrUint8 *report);
void CsrBtHiddUnplugCfmSend(HiddInstanceDataType *instData,
                                   CsrBtResultCode resultCode,
                                   CsrBtSupplier resultSupplier);
void CsrBtHiddUnplugIndSend(HiddInstanceDataType *instData,
                                  CsrBtDeviceAddr deviceAddr,
                                  CsrBtResultCode resultCode,
                                  CsrBtSupplier resultSupplier);
void CsrBtHiddModeChangeIndSend(HiddInstanceDataType *instData,
                                         CsrBtHiddPowerModeType mode,
                                         CsrBtResultCode resultCode,
                                         CsrBtSupplier resultSupplier);
extern void CsrBtHiddReactivateIndSend(HiddInstanceDataType *instData);

#ifdef INSTALL_HIDD_CUSTOM_SECURITY_SETTINGS
void CsrBtHiddSecurityInCfmSend(CsrSchedQid appHandle,
                                      CsrBtResultCode resultCode,
                                      CsrBtSupplier resultSupplier);
void CsrBtHiddSecurityOutCfmSend(CsrSchedQid appHandle,
                                        CsrBtResultCode resultCode,
                                        CsrBtSupplier resultSupplier);
#endif

#ifdef __cplusplus
}
#endif
#endif /* CSR_BT_HIDD_LOCAL_LIB_H__ */

