#ifndef CSR_BT_CM_COMMON_AMP_H__
#define CSR_BT_CM_COMMON_AMP_H__
/******************************************************************************
 Copyright (c) 2010-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef CSR_AMP_ENABLE

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Common upstream senders */
void CsrBtCmAmpMoveChannelCfmSend(cmInstanceData_t *inst,
                                  CsrBtConnId btConnId,
                                  CsrBtAmpController localControl,
                                  CsrBtDeviceAddr addr,
                                  CsrBtResultCode resultCode,
                                  CsrBtSupplier resultSupplier);
void CsrBtCmAmpMoveChannelCmpIndSend(cmInstanceData_t *inst,
                                     CsrBtConnId btConnId,
                                     CsrBtAmpController localControl,
                                     CsrBtDeviceAddr addr,
                                     CsrBtResultCode resultCode,
                                     CsrBtSupplier resultSupplier);
void CsrBtCmAmpMoveChannelIndSend(cmInstanceData_t *inst,
                                  CsrBtConnId btConnId,
                                  CsrBtAmpController localControl,
                                  CsrBtDeviceAddr addr);

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
/* Common downstream handlers */
void CsrBtCmAmpMoveChannelReqHandler(cmInstanceData_t *cmData);
void CsrBtCmAmpMoveChannelResHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmAmpMoveChannelReqHandler NULL
#define CsrBtCmAmpMoveChannelResHandler NULL
#endif

/* Utility functions */
void CsrBtCmAmpMoveIndProfiles(cmInstanceData_t *cmData,
                               CsrBtConnId btConnId);

#ifdef __cplusplus
}
#endif

#else
#define CsrBtCmAmpMoveChannelReqHandler NULL
#define CsrBtCmAmpMoveChannelResHandler NULL
#endif /* CSR_AMP_ENABLE */
#endif /* CSR_BT_CM_COMMON_AMP_H__ */
