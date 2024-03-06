/******************************************************************************
 Copyright (c) 2010-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_CM_MODULE

#include "csr_pmem.h"
#include "csr_bt_cm_prim.h"
#ifdef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_cm_private_prim.h"
#endif
#include "csr_bt_cm_free_handcoded.h"


void CsrBtCmBnepConnectAcceptReqPrimFree(void *prim)
{
    CSR_UNUSED(prim);
}

void CsrBtCmBnepConnectReqPrimFree(void *prim)
{
    CSR_UNUSED(prim);
}

void CsrBtCmSmIoCapabilityRequestResPrimFree(void *message)
{
    CsrBtCmSmIoCapabilityRequestRes *p = message;
    CsrPmemFree(p->oobHashC);
    CsrPmemFree(p->oobRandR);
    p->oobHashC = NULL;
    p->oobRandR = NULL;
}

#ifdef EXCLUDE_CSR_BT_SC_MODULE
void CsrBtCmDatabaseReqPrimFree(void *message)
{
    CsrBtCmDatabaseReq *p = message;
    CsrPmemFree(p->key);
    p->key = NULL;
}

void CsrBtCmDatabaseCfmPrimFree(void *message)
{
    CsrBtCmDatabaseCfm *p = message;
    CsrPmemFree(p->key);
    p->key = NULL;
}
#endif /* EXCLUDE_CSR_BT_SC_MODULE */

#endif

