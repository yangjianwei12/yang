/******************************************************************************
 Copyright (c) 2012-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef EXCLUDE_CSR_BT_HF_MODULE
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_bt_autogen.h"
#include "csr_bt_hf_lib.h"
#include "csr_bt_hf_prim.h"
#include "csr_bt_hf_main_sef.h"

void CsrBtHfFreeDownstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_HF_PRIM)
    {
        CsrBtHfPrim *prim = (CsrBtHfPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_BT_HF_AT_CMD_REQ
            case CSR_BT_HF_AT_CMD_REQ:
            {
                CsrBtHfAtCmdReq *p = message;
                CsrPmemFree(p->atCmdString);
                p->atCmdString = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HF_AT_CMD_REQ */
#ifndef EXCLUDE_CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES
            case CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES:
            {
                CsrBtHfAudioAcceptConnectRes *p = message;
                CsrPmemFree(p->acceptParameters);
                p->acceptParameters = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES */
#ifndef EXCLUDE_CSR_BT_HF_DIAL_REQ
            case CSR_BT_HF_DIAL_REQ:
            {
                CsrBtHfDialReq *p = message;
                CsrPmemFree(p->number);
                p->number = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HF_DIAL_REQ */
#ifndef EXCLUDE_CSR_BT_HF_CONFIG_AUDIO_REQ
            case CSR_BT_HF_CONFIG_AUDIO_REQ:
            {
                CsrBtHfConfigAudioReq *p = message;
                CsrPmemFree(p->audioSetting);
                p->audioSetting = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HF_CONFIG_AUDIO_REQ */
#ifndef EXCLUDE_CSR_BT_HF_ACTIVATE_REQ
            case CSR_BT_HF_ACTIVATE_REQ:
            {
                CsrBtHfActivateReq *p = message;
                CsrPmemFree(p->hfSupportedHfIndicators);
                p->hfSupportedHfIndicators = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HF_ACTIVATE_REQ */
#ifndef EXCLUDE_CSR_BT_HF_AUDIO_CONNECT_REQ
            case CSR_BT_HF_AUDIO_CONNECT_REQ:
            {
                CsrBtHfAudioConnectReq *p = message;
                CsrPmemFree(p->audioParameters);
                p->audioParameters = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HF_AUDIO_CONNECT_REQ */
#ifdef HF_ENABLE_OPTIONAL_CODEC_SUPPORT
            case HF_UPDATE_OPTIONAL_CODEC_REQ:
            {
                HfUpdateOptionalCodecReq *p = message;
                CsrPmemFree(p->codecIdList);
                p->codecIdList = NULL;
                break;
            }
#endif /* HF_ENABLE_OPTIONAL_CODEC_SUPPORT */

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
#endif /* EXCLUDE_CSR_BT_HF_MODULE */
