#ifndef CSR_BT_CM_CME_H__
#define CSR_BT_CM_CME_H__
/******************************************************************************
 Copyright (c) 2013-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/


#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
#include "csr_bt_cm_main.h"

#ifdef __cplusplus
extern "C" {
#endif

void CsrBtCmCmeInit(void);

void CsrBtCmCmeHciVendorSpecificEventIndHandler(cmInstanceData_t* cmData);

void CsrBtCmCmeProfileA2dpStartIndSend(cmInstanceData_t* cmData, CsrUint8 idx);

void CsrBtCmCmeProfileA2dpStopIndSend(cmInstanceData_t* cmData, CsrUint8 idx);

#ifdef __cplusplus
}
#endif

#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

#endif /* CSR_BT_CM_CME_H__ */
