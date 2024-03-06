/******************************************************************************
 Copyright (c) 2012-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#include "csr_synergy.h"
#include "csr_msgconv.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_bt_cm_free_handcoded.h"
#ifndef EXCLUDE_CSR_BT_CM_MODULE
#include "csr_bt_cm_prim.h"
#ifdef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_cm_private_prim.h"
#endif

void CsrBtCmFreeHandcoded(void *message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;
    switch(*prim)
    {
#ifndef EXCLUDE_CSR_BT_CM_BNEP_CONNECT_ACCEPT_REQ
        case CSR_BT_CM_BNEP_CONNECT_ACCEPT_REQ:
        {
            CsrBtCmBnepConnectAcceptReqPrimFree(prim);
            break;
        }
#endif /* EXCLUDE_CSR_BT_CM_BNEP_CONNECT_ACCEPT_REQ */
#ifndef EXCLUDE_CSR_BT_CM_BNEP_CONNECT_REQ
        case CSR_BT_CM_BNEP_CONNECT_REQ:
        {
            CsrBtCmBnepConnectReqPrimFree(prim);
            break;
        }
#endif /* EXCLUDE_CSR_BT_CM_BNEP_CONNECT_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_RES
        case CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_RES:
        {
            CsrBtCmSmIoCapabilityRequestResPrimFree(prim);
            break;
        }
#endif /* EXCLUDE_CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_RES */
#ifdef EXCLUDE_CSR_BT_SC_MODULE
#ifndef EXCLUDE_CSR_BT_CM_DATABASE_REQ
        case CSR_BT_CM_DATABASE_REQ:
        {
            CsrBtCmDatabaseReqPrimFree(prim);
            break;
        }
#endif /* EXCLUDE_CSR_BT_CM_DATABASE_REQ */
#ifndef EXCLUDE_CSR_BT_CM_DATABASE_CFM
        case CSR_BT_CM_DATABASE_CFM:
        {
            CsrBtCmDatabaseCfmPrimFree(prim);
            break;
        }
#endif /* EXCLUDE_CSR_BT_CM_DATABASE_CFM */
#endif /* EXCLUDE_CSR_BT_SC_MODULE */
        default:
        {
            break;
        }
    }
}

#endif

