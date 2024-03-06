/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_bt_result.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SD_MODULE
#include "csr_bt_sd_private_lib.h"
#endif
#include "csr_bt_pac_handler.h"
#include "csr_log_text_2.h"

#ifdef CSR_LOG_ENABLE
/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtPacLto);
#endif
#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_obex_streams.h"
#include "csr_bt_obex_util.h"
#include "csr_streams.h"
#endif

#ifdef CSR_TARGET_PRODUCT_VM
#include <panic.h>
#endif /* CSR_TARGET_PRODUCT_VM */

#define CSR_BT_PAC_INST_ID              0x01

static const PacStateHandlerType pacStateHandlers[CSR_BT_PAC_PRIM_DOWNSTREAM_COUNT] =
{
  CsrBtPacConnectReqHandler,        /* CSR_BT_PAC_CONNECT_REQ          */
  CsrBtPacAuthenticateResHandler,   /* CSR_BT_PAC_AUTHENTICATE_RES     */
  CsrBtPacPullPbReqHandler,         /* CSR_BT_PAC_PULL_PB_REQ          */
  CsrBtPacPullPbResHandler,         /* CSR_BT_PAC_PULL_PB_RES          */
  CsrBtPacSetFolderReqHandler,      /* CSR_BT_PAC_SET_FOLDER_REQ       */
  CsrBtPacSetBackFolderReqHandler,  /* CSR_BT_PAC_SET_BACK_FOLDER_REQ  */
  CsrBtPacSetRootFolderReqHandler,  /* CSR_BT_PAC_SET_ROOT_FOLDER_REQ  */
  CsrBtPacPullvCardListReqHandler,  /* CSR_BT_PAC_PULL_VCARD_LIST_REQ  */
  CsrBtPacPullvCardListResHandler,  /* CSR_BT_PAC_PULL_VCARD_LIST_RES  */
  CsrBtPacPullvCardEntryReqHandler, /* CSR_BT_PAC_PULL_VCARD_ENTRY_REQ */
  CsrBtPacPullvCardEntryResHandler, /* CSR_BT_PAC_PULL_VCARD_ENTRY_RES */
  CsrBtPacAbortReqHandler,          /* CSR_BT_PAC_ABORT_REQ            */
  CsrBtPacDisconnectReqHandler,     /* CSR_BT_PAC_DISCONNECT_REQ       */
  CsrBtPacCancelConnectReqHandler,  /* CSR_BT_PAC_CANCEL_CONNECT_REQ   */
  CsrBtPacSecurityOutReqHandler,    /* CSR_BT_PAC_SECURITY_OUT_REQ     */
  PacRegisterQIDReqHandler,         /* PAC_REGISTER_QID_REQ            */
  PacGetInstanceIdsReqHandler,      /* PAC_GET_INSTANCE_IDS_REQ        */
};

#ifdef CSR_TARGET_PRODUCT_VM
PacInst csrBtPacInstData[PAC_MAX_NUM_INSTANCES];
static CsrUint8 pacNumInstRegistered;

CsrBool PacSetAppHandle(CsrSchedQid pacInstanceId, CsrSchedQid appHandle)
{
    CsrUint8 instIdx;
    for (instIdx = 0; instIdx < PAC_MAX_NUM_INSTANCES; instIdx++)
    {
        PacInst *pInst = &(csrBtPacInstData[instIdx]);

        if (pInst->pacInstanceId == pacInstanceId)
        {
            pInst->appHandle = appHandle;
            return TRUE;
        }
    }
    return FALSE;
}
#endif /* CSR_TARGET_PRODUCT_VM */

void CsrBtPacResetLocalAppHeaderPar(PacInst *pInst)
{
    pInst->propertySel = 0;
    pInst->vcardSel = 0;
    pInst->vcardSelOp = CSR_BT_PB_VCARD_SELECTOR_OPERATOR_OR;
    pInst->order = CSR_BT_PB_ORDER_INDEXED;
    pInst->searchVal = 0;
    pInst->searchAtt = CSR_BT_PB_SEARCH_ATT_NAME;
    pInst->maxLstCnt = CSR_BT_PB_DEFAULT_MAX_LIST_COUNT;
    pInst->lstStartOff = 0;
    pInst->format = CSR_BT_PB_FORMAT_VCARD2_1;
    pInst->rstMissedCall = 0;
}

void CsrBtPacResetRemoteAppHeaderPar(PacInst *pInst)
{
    pInst->pbSize = 0;
    pInst->newMissedCall = 0;
    CsrMemSet(&pInst->versionInfo, 0, sizeof(CsrBtPbVersionInfo));
}

#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
static void pacRegisterNewInstanceReqSend(PacInst *pInst)
{
    PacRegisterQidReq *prim = CsrPmemAlloc(sizeof(PacRegisterQidReq));

    prim->type          = PAC_REGISTER_QID_REQ;
    prim->pacInstanceId = pInst->pacInstanceId;

    CsrBtPacMessagePut(CSR_BT_PAC_IFACEQUEUE, prim);
}
#endif

void CsrBtPacInit(void **gash)
{
    PacInst *pInst;

    CSR_LOG_TEXT_REGISTER(&CsrBtPacLto, "BT_PAC", 0, NULL);

#ifdef CSR_TARGET_PRODUCT_VM
    PanicFalse(pacNumInstRegistered < PAC_MAX_NUM_INSTANCES);
    *gash = &csrBtPacInstData[pacNumInstRegistered++];
    CSR_LOG_TEXT_INFO((CsrBtPacLto, 0, "CsrBtPacInit: pacNumInstRegistered(%d)", pacNumInstRegistered));
#else
    /* allocate instance data */
    *gash = CsrPmemZalloc(sizeof(PacInst));
#endif /* CSR_TARGET_PRODUCT_VM */

    pInst                = *gash;
    pInst->pacInstanceId = CsrSchedTaskQueueGet();

    /* Do not free this, it shall be used between Init and Deinit.
     * It shall be passed to Applications.
     * Note: Applications shall not free it.
     */
    pInst->obexInst = ObexUtilInit(pInst->pacInstanceId,
                                   pInst,
                                   CSR_BT_PAC_INST_ID);

#ifdef INSTALL_PAC_CUSTOM_SECURITY_SETTINGS
    CsrBtScSetSecOutLevel(&pInst->secOutgoing,
                          CSR_BT_SEC_DEFAULT,
                          CSR_BT_PBAP_MANDATORY_SECURITY_OUTGOING,
                          CSR_BT_PBAP_DEFAULT_SECURITY_OUTGOING,
                          CSR_BT_RESULT_CODE_OBEX_SUCCESS,
                          CSR_BT_RESULT_CODE_OBEX_UNACCEPTABLE_PARAMETER);
#endif

    if (pInst->pacInstanceId == CSR_BT_PAC_IFACEQUEUE)
    {/* This is the main PAC instance (i.e. PAC manager); contains the list
      * of all instance IDs application instantiates */

        /* Register the PAC own serviceRecord */
        CsrBtPacCmSdsRegisterReqHandler(pInst);

#ifndef EXCLUDE_CSR_BT_SD_MODULE
        /* Tell the SD that it must look for the CSR_BT_OBEX_PBA_SERVER_PROFILE_UUID
         service, when it perform a SD_READ_AVAILABLE_SERVICE_REQ                 */
        CsrBtSdRegisterAvailableServiceReqSend(CSR_BT_OBEX_PBA_SERVER_PROFILE_UUID);
#endif
    }

    PAC_INSTANCE_STATE_CHANGE(pInst->state, PAC_INSTANCE_STATE_IDLE);

#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
    /* Register this instance to PAC manager instance */
    pacRegisterNewInstanceReqSend(pInst);
#endif
}

static void csrBtPacCmMessageHandler(PacInst *pInst, void **msg)
{
    CsrPrim *type = *msg;
#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_INFO((CsrBtPacLto, 0, "csrBtPacCmMessageHandler MESSAGE:CsrBtCmPrim:%d", *type));
#endif

    if (*type == CSR_BT_CM_SDS_REGISTER_CFM)
    {
        CsrBtPacCmSdsRegistertCfmHandler(pInst, *msg);
    }
    else if (ObexUtilCmMsgHandler(pInst->obexInst,
                                  msg) != CSR_BT_OBEX_UTIL_STATUS_EXCEPTION)
    { /* This message is handled by the common OBEX library */
        ;
    }
    else
    {
        /* State/Event ERROR! */
        CsrGeneralException(CsrBtPacLto,
                            0,
                            CSR_BT_CM_PRIM,
                            *type,
                            0,
                            "Unhandled CSR_BT_CM_PRIM");
        CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, *msg);
    }
}

#ifdef CSR_STREAMS_ENABLE
static void csrBtPacStreamDataCfmHandler(void *inst, CsrBtCmDataCfm **cfm)
{
    PacInst *pInst = inst;
    csrBtPacCmMessageHandler(pInst, (void**)cfm);
}

static void csrBtPacStreamDataIndHandler(void *inst, CsrBtCmDataInd **ind)
{
    PacInst *pInst = inst;
    csrBtPacCmMessageHandler(pInst, (void**)ind);
}
#endif

void CsrBtPacHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    PacInst *pInst = *gash;

    CsrSchedMessageGet(&eventClass, &msg);

    switch (eventClass)
    {
        case CSR_BT_PAC_PRIM:
        {
            CsrBtPacPrim *type = msg;
#ifdef CSR_TARGET_PRODUCT_VM
            CSR_LOG_TEXT_INFO((CsrBtPacLto, 0, "CsrBtPacHandler MESSAGE:CsrBtPacPrim:0x%x", *type));
#endif

            if (*type < CSR_BT_PAC_PRIM_DOWNSTREAM_COUNT  &&
                (pacStateHandlers[*type] != NULL) &&
                (pacStateHandlers[*type](pInst, msg)
                 != CSR_BT_OBEX_UTIL_STATUS_EXCEPTION))
            {
                ;
            }
            else
            {/* State/Event ERROR! */
                CsrGeneralException(CsrBtPacLto,
                                    0,
                                    CSR_BT_PAC_PRIM,
                                    *type,
                                    0,
                                    "Unhandled CSR_BT_PAC_PRIM");
                CsrBtPacFreeDownstreamMessageContents(eventClass, msg);
            }
            break;
        }

#ifdef CSR_STREAMS_ENABLE
        case MESSAGE_MORE_SPACE:
        {
            CsrUint8 protocol = CSR_BT_CONN_ID_IS_L2CA(ObexUtilGetConnId(pInst->obexInst))?L2CAP_ID:RFCOMM_ID;
            CsrBtObexMessageMoreSpaceHandler(pInst, csrBtPacStreamDataCfmHandler, (MessageMoreSpace *)msg, protocol);            
            msg = NULL; /*Message already freed*/
            break;
        }
        case MESSAGE_MORE_DATA:
        {
            CsrUint8 protocol = CSR_BT_CONN_ID_IS_L2CA(ObexUtilGetConnId(pInst->obexInst))?L2CAP_ID:RFCOMM_ID;
            CsrBtObexMessageMoreDataHandler(pInst, csrBtPacStreamDataIndHandler, (MessageMoreData *)msg, protocol);
            msg = NULL; /*Message already freed*/
            break;
        }
#endif

        case CSR_BT_CM_PRIM:
        {
            csrBtPacCmMessageHandler(pInst, &msg);
            break;
        }
        case CSR_SCHED_PRIM:
        {
            break;
        }
        default:
        {
            CsrGeneralException(CsrBtPacLto,
                                0,
                                eventClass,
                                0,
                                0,
                                "PAC Unknown event type");
            break;
        }
    }

    SynergyMessageFree(eventClass, msg);
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************
 This function is called by the scheduler to perform a graceful shutdown
 of a scheduler task.
 This function must:
 1)  empty the input pInstmessage queue and free any allocated pInstmemory in the
 pInstmessages.
 2)  free any instance data that may be allocated.
 ****************************************************************************/
void CsrBtPacDeinit(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    CsrBool lastMsg = FALSE;
    PacInst *pInst = *gash;

    while (!lastMsg)
    {
        lastMsg = (CsrBool) (!CsrSchedMessageGet(&eventClass, &msg));

        if (!lastMsg)
        {
            switch (eventClass)
            {
                case CSR_BT_PAC_PRIM:
                {
                    CsrBtPacFreeDownstreamMessageContents(eventClass, msg);
                    break;
                }
                case CSR_BT_CM_PRIM:
                {
                    CsrBtCmFreeUpstreamMessageContents(eventClass, msg);
                    break;
                }
                default:
                {
                    CsrGeneralException(CsrBtPacLto, 0, eventClass, 0, 0, NULL);
                    break;
                }
            }
            CsrPmemFree(msg);
        }
    }

#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
    if (pInst->pacInstances != NULL)
    {
        CsrPmemFree(pInst->pacInstances);
        pInst->pacInstances = NULL;
    }
#endif

    ObexUtilDeinit(&pInst->obexInst);
    CsrPmemFree(pInst);
}
#endif /* ENABLE_SHUTDOWN       */

