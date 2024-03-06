/******************************************************************************
 Copyright (c) 2012-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
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
#include "csr_bt_spp_sef.h"

void CsrBtSppFreeDownstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_SPP_PRIM)
    {
        CsrBtSppPrim *prim = (CsrBtSppPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_BT_SPP_AUDIO_CONNECT_REQ
            case CSR_BT_SPP_AUDIO_CONNECT_REQ:
            {
                CsrBtSppAudioConnectReq *p = message;
                CsrPmemFree(p->audioParameters);
                p->audioParameters = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_SPP_AUDIO_CONNECT_REQ */
#ifndef EXCLUDE_CSR_BT_SPP_ACTIVATE_REQ
            case CSR_BT_SPP_ACTIVATE_REQ:
            {
                CsrBtSppActivateReq *p = message;
                CsrPmemFree(p->serviceName);
                p->serviceName = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_SPP_ACTIVATE_REQ */
#ifndef EXCLUDE_CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES
            case CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES:
            {
                CsrBtSppAudioAcceptConnectRes *p = message;
                CsrPmemFree(p->acceptParameters);
                p->acceptParameters = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES */
#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
            case CSR_BT_SPP_EXTENDED_ACTIVATE_REQ:
            {
                CsrBtSppExtendedActivateReq *p = message;
                CsrPmemFree(p->serviceRecord);
                p->serviceRecord = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
#if !defined(EXCLUDE_CSR_BT_SPP_DATA_REQ) && !defined(CSR_STREAMS_ENABLE)
            case CSR_BT_SPP_DATA_REQ:
            {
                CsrBtSppDataReq *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* !EXCLUDE_CSR_BT_SPP_DATA_REQ and !CSR_STREAMS_ENABLE */
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
