/******************************************************************************
 Copyright (c) 2012-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef EXCLUDE_CSR_BT_HFG_MODULE
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_bt_autogen.h"
#include "csr_bt_hfg_lib.h"
#include "csr_bt_hfg_prim.h"
#include "csr_bt_hfg_main.h"
#include "csr_bt_hfg_proto.h"

void CsrBtHfgFreeDownstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_HFG_PRIM)
    {
        CsrBtHfgPrim *prim = (CsrBtHfgPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_BT_HFG_CALL_WAITING_REQ
            case CSR_BT_HFG_CALL_WAITING_REQ:
            {
                CsrBtHfgCallWaitingReq *p = message;
                CsrPmemFree(p->number);
                p->number = NULL;
                CsrPmemFree(p->name);
                p->name = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_CALL_WAITING_REQ */
#ifndef EXCLUDE_CSR_BT_HFG_OPERATOR_RES
            case CSR_BT_HFG_OPERATOR_RES:
            {
                CsrBtHfgOperatorRes *p = message;
                CsrPmemFree(p->operatorName);
                p->operatorName = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_OPERATOR_RES */
#ifndef EXCLUDE_CSR_BT_HFG_RING_REQ
            case CSR_BT_HFG_RING_REQ:
            {
                CsrBtHfgRingReq *p = message;
                CsrPmemFree(p->number);
                p->number = NULL;
                CsrPmemFree(p->name);
                p->name = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_RING_REQ */
#ifndef EXCLUDE_CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ
            case CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ:
            {
                CsrBtHfgConfigAtcmdHandlingReq *p = message;
                CsrPmemFree(p->bitwiseIndicators);
                p->bitwiseIndicators = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ */
#ifndef EXCLUDE_CSR_BT_HFG_CALL_LIST_RES
            case CSR_BT_HFG_CALL_LIST_RES:
            {
                CsrBtHfgCallListRes *p = message;
                CsrPmemFree(p->number);
                p->number = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_CALL_LIST_RES */
#ifndef EXCLUDE_CSR_BT_HFG_BT_INPUT_RES
            case CSR_BT_HFG_BT_INPUT_RES:
            {
                CsrBtHfgBtInputRes *p = message;
                CsrPmemFree(p->response);
                p->response = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_BT_INPUT_RES */
#ifndef EXCLUDE_CSR_BT_HFG_AT_CMD_REQ
            case CSR_BT_HFG_AT_CMD_REQ:
            {
                CsrBtHfgAtCmdReq *p = message;
                CsrPmemFree(p->command);
                p->command = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_AT_CMD_REQ */
#ifndef EXCLUDE_CSR_BT_HFG_CONFIG_AUDIO_REQ
            case CSR_BT_HFG_CONFIG_AUDIO_REQ:
            {
                CsrBtHfgConfigAudioReq *p = message;
                CsrPmemFree(p->audioSetting);
                p->audioSetting = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_CONFIG_AUDIO_REQ */
#ifndef EXCLUDE_CSR_BT_HFG_MANUAL_INDICATOR_RES
            case CSR_BT_HFG_MANUAL_INDICATOR_RES:
            {
                CsrBtHfgManualIndicatorRes *p = message;
                CsrPmemFree(p->indicators);
                p->indicators = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_MANUAL_INDICATOR_RES */
#ifndef EXCLUDE_CSR_BT_HFG_SUBSCRIBER_NUMBER_RES
            case CSR_BT_HFG_SUBSCRIBER_NUMBER_RES:
            {
                CsrBtHfgSubscriberNumberRes *p = message;
                CsrPmemFree(p->number);
                p->number = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_SUBSCRIBER_NUMBER_RES */
#ifndef EXCLUDE_CSR_BT_HFG_ACTIVATE_REQ
            case CSR_BT_HFG_ACTIVATE_REQ:
            {
                CsrBtHfgActivateReq *p = message;
                CsrPmemFree(p->serviceName);
                p->serviceName = NULL;
                CsrPmemFree(p->hfgSupportedHfIndicators);
                p->hfgSupportedHfIndicators = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_ACTIVATE_REQ */
#ifndef EXCLUDE_CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES
            case CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES:
            {
                CsrBtHfgAudioAcceptConnectRes *p = message;
                CsrPmemFree(p->acceptParameters);
                p->acceptParameters = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES */
#ifndef EXCLUDE_CSR_BT_HFG_VOICE_RECOG_REQ
            case CSR_BT_HFG_VOICE_RECOG_REQ:
            {
                CsrBtHfgVoiceRecogReq *p = message;
                CsrPmemFree(p->textId);
                CsrPmemFree(p->string);
                p->textId = NULL;
                p->string = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_HFG_VOICE_RECOG_REQ */
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
#endif /* EXCLUDE_CSR_BT_HFG_MODULE */
