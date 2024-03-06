/******************************************************************************
 Copyright (c) 2012-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef EXCLUDE_CSR_BT_PAC_MODULE
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_bt_autogen.h"
#include "csr_bt_pac_lib.h"
#include "csr_bt_pac_prim.h"

void CsrBtPacFreeUpstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_PAC_PRIM)
    {
        CsrBtPacPrim *prim = (CsrBtPacPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_BT_PAC_AUTHENTICATE_IND
            case CSR_BT_PAC_AUTHENTICATE_IND:
            {
                CsrBtPacAuthenticateInd *p = message;
                CsrPmemFree(p->realm);
                p->realm = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_PAC_AUTHENTICATE_IND */
#ifndef EXCLUDE_CSR_BT_PAC_PULL_VCARD_LIST_IND
            case CSR_BT_PAC_PULL_VCARD_LIST_IND:
            {
                CsrBtPacPullVcardListInd *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_PAC_PULL_VCARD_LIST_IND */
#ifndef EXCLUDE_CSR_BT_PAC_PULL_PB_IND
            case CSR_BT_PAC_PULL_PB_IND:
            {
                CsrBtPacPullPbInd *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_PAC_PULL_PB_IND */
#ifndef EXCLUDE_CSR_BT_PAC_PULL_VCARD_ENTRY_IND
            case CSR_BT_PAC_PULL_VCARD_ENTRY_IND:
            {
                CsrBtPacPullVcardEntryInd *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_PAC_PULL_VCARD_ENTRY_IND */
            case PAC_GET_INSTANCE_IDS_CFM:
            {
                PacGetInstanceIdsCfm *p = message;
                CsrPmemFree(p->instanceIdsList);
                p->instanceIdsList = NULL;
                break;
            }
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
#endif /* EXCLUDE_CSR_BT_PAC_MODULE */
