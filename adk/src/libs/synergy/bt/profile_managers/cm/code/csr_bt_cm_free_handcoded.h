#ifndef CSR_BT_CM_FREE_HANDCODED_H__
#define CSR_BT_CM_FREE_HANDCODED_H__
/******************************************************************************
 Copyright (c) 2012-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXCLUDE_CSR_BT_CM_MODULE

void CsrBtCmFreeHandcoded(void *message);

/* The following functions must be handcoded */
void CsrBtCmBnepConnectAcceptReqPrimFree(void *message);
void CsrBtCmBnepConnectReqPrimFree(void *message);
void CsrBtCmSmIoCapabilityRequestResPrimFree(void *message);

#ifdef EXCLUDE_CSR_BT_SC_MODULE
void CsrBtCmDatabaseReqPrimFree(void *message);
void CsrBtCmDatabaseCfmPrimFree(void *message);
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_CM_FREE_HANDCODED_H__ */

