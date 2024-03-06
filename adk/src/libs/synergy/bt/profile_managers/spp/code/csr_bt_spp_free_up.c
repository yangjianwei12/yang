/******************************************************************************
 Copyright (c) 2012-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_SPP_MODULE
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_bt_autogen.h"
#include "csr_bt_spp_lib.h"
#include "csr_bt_spp_prim.h"

void CsrBtSppFreeUpstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_SPP_PRIM)
    {
        CsrBtSppPrim *prim = (CsrBtSppPrim *) message;
        switch (*prim)
        {
#if !defined(EXCLUDE_CSR_BT_SPP_SERVICE_NAME_IND) && defined(INSTALL_SPP_OUTGOING_CONNECTION)
            case CSR_BT_SPP_SERVICE_NAME_IND:
            {
                CsrBtSppServiceNameInd *p = message;
                CsrPmemFree(p->serviceNameList);
                p->serviceNameList = NULL;
                break;
            }
#endif /* !EXCLUDE_CSR_BT_SPP_SERVICE_NAME_IND && INSTALL_SPP_OUTGOING_CONNECTION */
#if !defined(EXCLUDE_CSR_BT_SPP_DATA_IND) && !defined(CSR_STREAMS_ENABLE)
            case CSR_BT_SPP_DATA_IND:
            {
                CsrBtSppDataInd *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* !EXCLUDE_CSR_BT_SPP_DATA_IND && !CSR_STREAMS_ENABLE */
#ifndef EXCLUDE_CSR_BT_SPP_GET_INSTANCES_QID_CFM
            case CSR_BT_SPP_GET_INSTANCES_QID_CFM:
            {
                CsrBtSppGetInstancesQidCfm *p = message;
                CsrPmemFree(p->phandlesList);
                p->phandlesList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_SPP_GET_INSTANCES_QID_CFM */
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
#endif /* EXCLUDE_CSR_BT_SPP_MODULE */
