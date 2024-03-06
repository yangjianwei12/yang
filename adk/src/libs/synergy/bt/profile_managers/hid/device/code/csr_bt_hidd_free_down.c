/******************************************************************************
 Copyright (c) 2012-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef EXCLUDE_CSR_BT_HIDD_MODULE
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_bt_autogen.h"
#include "csr_bt_hidd_lib.h"
#include "csr_bt_hidd_prim.h"
#include "csr_bt_hidd_sef.h"

void CsrBtHiddFreeDownstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_HIDD_PRIM)
    {
        CsrBtHiddPrim *prim = (CsrBtHiddPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_BT_HIDD_DATA_REQ
            case CSR_BT_HIDD_DATA_REQ:
            {
                CsrBtHiddDataReq *p = message;
                CsrPmemFree(p->report);
                p->report = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HIDD_DATA_REQ */
#ifndef EXCLUDE_CSR_BT_HIDD_ACTIVATE_REQ
            case CSR_BT_HIDD_ACTIVATE_REQ:
            {
                CsrBtHiddActivateReq *p = message;
                CsrPmemFree(p->qosCtrl);
                p->qosCtrl = NULL;
                CsrPmemFree(p->qosIntr);
                p->qosIntr = NULL;
                CsrPmemFree(p->deviceIdSdp);
                p->deviceIdSdp = NULL;
                CsrPmemFree(p->hidSdp);
                p->hidSdp = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HIDD_ACTIVATE_REQ */
#ifndef EXCLUDE_CSR_BT_HIDD_CONTROL_RES
            case CSR_BT_HIDD_CONTROL_RES:
            {
                CsrBtHiddControlRes *p = message;
                CsrPmemFree(p->data);
                p->data = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HIDD_CONTROL_RES */
            default:
            {
                break;
            }
        } /* End switch */
    } /* End if */
    else
    {
        /* Unknown primitive type, exception handling */
    }
}
#endif /* EXCLUDE_CSR_BT_HIDD_MODULE */
