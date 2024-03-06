/******************************************************************************
 Copyright (c) 2012-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

/* Note: this is an auto-generated file. */
#include "csr_synergy.h"
#ifndef EXCLUDE_CSR_BT_MAPC_MODULE

#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_bt_autogen.h"
#include "csr_bt_mapc_lib.h"
#include "csr_bt_mapc_prim.h"
#include "csr_bt_mapc_sef.h"

void CsrBtMapcFreeDownstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_MAPC_PRIM)
    {
        CsrBtMapcPrim *prim = (CsrBtMapcPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_BT_MAPC_GET_MESSAGE_REQ
            case CSR_BT_MAPC_GET_MESSAGE_REQ:
            {
                CsrBtMapcGetMessageReq *p = message;
                CsrPmemFree(p->messageHandle);
                p->messageHandle = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_MAPC_GET_MESSAGE_REQ */
#ifndef EXCLUDE_CSR_BT_MAPC_GET_OWNER_STATUS_REQ
            case CSR_BT_MAPC_GET_OWNER_STATUS_REQ:
            {
                CsrBtMapcGetOwnerStatusReq *p = message;
                CsrPmemFree(p->conversationId);
                p->conversationId = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_MAPC_GET_OWNER_STATUS_REQ */
#ifndef EXCLUDE_CSR_BT_MAPC_GET_CONVERSATION_LISTING_REQ
            case CSR_BT_MAPC_GET_CONVERSATION_LISTING_REQ:
            {
                CsrBtMapcGetConversationListingReq *p = message;
                CsrPmemFree(p->filterLastActivityBegin);
                p->filterLastActivityBegin = NULL;
                CsrPmemFree(p->filterLastActivityEnd);
                p->filterLastActivityEnd = NULL;
                CsrPmemFree(p->filterRecipient);
                p->filterRecipient = NULL;
                CsrPmemFree(p->conversationId);
                p->conversationId = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_MAPC_GET_CONVERSATION_LISTING_REQ */
#ifndef EXCLUDE_CSR_BT_MAPC_GET_MESSAGE_LISTING_REQ
            case CSR_BT_MAPC_GET_MESSAGE_LISTING_REQ:
            {
                CsrBtMapcGetMessageListingReq *p = message;
                CsrPmemFree(p->folderName);
                p->folderName = NULL;
                CsrPmemFree(p->filterPeriodBegin);
                p->filterPeriodBegin = NULL;
                CsrPmemFree(p->filterPeriodEnd);
                p->filterPeriodEnd = NULL;
                CsrPmemFree(p->filterRecipient);
                p->filterRecipient = NULL;
                CsrPmemFree(p->filterOriginator);
                p->filterOriginator = NULL;
                CsrPmemFree(p->conversationId);
                p->conversationId = NULL;
                CsrPmemFree(p->filterMessageHandle);
                p->filterMessageHandle = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_MAPC_GET_MESSAGE_LISTING_REQ */
#ifndef EXCLUDE_CSR_BT_MAPC_SET_OWNER_STATUS_REQ
            case CSR_BT_MAPC_SET_OWNER_STATUS_REQ:
            {
                CsrBtMapcSetOwnerStatusReq *p = message;
                CsrPmemFree(p->presenceText);
                p->presenceText = NULL;
                CsrPmemFree(p->lastActivity);
                p->lastActivity = NULL;
                CsrPmemFree(p->conversationId);
                p->conversationId = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_MAPC_SET_OWNER_STATUS_REQ */
#ifndef EXCLUDE_CSR_BT_MAPC_PUSH_MESSAGE_RES
            case CSR_BT_MAPC_PUSH_MESSAGE_RES:
            {
                CsrBtMapcPushMessageRes *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_MAPC_PUSH_MESSAGE_RES */
#ifndef EXCLUDE_CSR_BT_MAPC_SET_MESSAGE_STATUS_REQ
            case CSR_BT_MAPC_SET_MESSAGE_STATUS_REQ:
            {
                CsrBtMapcSetMessageStatusReq *p = message;
                CsrPmemFree(p->messageHandle);
                p->messageHandle = NULL;
                CsrPmemFree(p->extendedData);
                p->extendedData = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_MAPC_SET_MESSAGE_STATUS_REQ */
#ifndef EXCLUDE_CSR_BT_MAPC_SET_FOLDER_REQ
            case CSR_BT_MAPC_SET_FOLDER_REQ:
            {
                CsrBtMapcSetFolderReq *p = message;
                CsrPmemFree(p->folderName);
                p->folderName = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_MAPC_SET_FOLDER_REQ */
#ifndef EXCLUDE_CSR_BT_MAPC_PUSH_MESSAGE_REQ
            case CSR_BT_MAPC_PUSH_MESSAGE_REQ:
            {
                CsrBtMapcPushMessageReq *p = message;
                CsrPmemFree(p->folderName);
                p->folderName = NULL;
                CsrPmemFree(p->conversationId);
                p->conversationId = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_MAPC_PUSH_MESSAGE_REQ */
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
#endif /* EXCLUDE_CSR_BT_MAPC_MODULE */
