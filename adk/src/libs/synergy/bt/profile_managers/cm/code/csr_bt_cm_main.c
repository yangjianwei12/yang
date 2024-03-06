/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_sched.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_callback_q.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_sdc.h"
#include "csr_bt_cm_util.h"
#include "td_db_main.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_cm_streams_handler.h"
#endif

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#include "csr_bt_cm_le.h"
#include "dmlib.h"

#ifndef EXCLUDE_CSR_BCCMD_MODULE
#include "csr_bccmd_lib.h"
#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#include "csr_bt_cm_bccmd.h"
#endif
#endif

#ifndef CSR_TARGET_PRODUCT_VM
#include "csr_tm_bluecore_lib.h"
#include "csr_tm_bluecore_prim.h"
#endif

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
#include "csr_bt_cm_cme.h"
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

#ifdef CSR_BT_GLOBAL_INSTANCE
cmInstanceData_t  csrBtCmData;
#endif /* CSR_BT_GLOBAL_INSTANCE */

#ifdef CSR_LOG_ENABLE
#ifdef CSR_TARGET_PRODUCT_VM
#include "csr_bt_cm_prim_enum_dbg.h"
CSR_PRESERVE_GENERATED_ENUM(dm_prim_tag)
CSR_PRESERVE_GENERATED_ENUM(CsrBtCmPrim)
#endif /*CSR_TARGET_PRODUCT_VM*/

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtCmLto);
#ifdef CSR_LOG_ENABLE_REGISTRATION
static const CsrCharString *subOrigins[] =
{
    "STATE",
    "PLAYER",
    "DM Queue",
    "SM Queue"
};
#endif /* CSR_LOG_ENABLE_REGISTRATION */
#endif /* CSR_LOG_ENABLE */


void CsrBtCmInit(void **gash);
void CsrBtCmHandler(void **gash);

#ifdef CSR_TARGET_PRODUCT_VM
extern const char* synergy_si_version;
static void printSIVersion(const char *s)
{
    CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "Synergy SI Version: %s",synergy_si_version));
    CSR_UNUSED(s);
}
#endif

/*************************************************************************************
  CsrBtCmInit:
************************************************************************************/
void CsrBtCmInit(void **gash)
{ /* Allocate and initialise CM instance data space */
    cmInstanceData_t  *cmData;

    CSR_LOG_TEXT_REGISTER(&(CsrBtCmLto),
                          "BT_CM",
                          CSR_ARRAY_SIZE(subOrigins),
                          subOrigins);

#ifdef CSR_BT_GLOBAL_INSTANCE
    *gash = &csrBtCmData;
#else
    *gash = (void *) CsrPmemZalloc(sizeof(cmInstanceData_t));
#endif
    cmData = (cmInstanceData_t *) *gash;

    CsrBtCmInitInstData(cmData);
#ifndef CSR_TARGET_PRODUCT_VM
    CsrTmBlueCoreActivateTransportReqSend(CSR_BT_CM_IFACEQUEUE);
#else
    dm_am_register_req(CSR_BT_CM_IFACEQUEUE);
#endif

#ifdef EXCLUDE_CSR_BT_SC_MODULE
#ifndef CSR_TARGET_PRODUCT_VM

#ifdef INCLUDE_BT_WEARABLE_TD_DB_PS
    CsrBtTdDbInit(CSR_BT_TD_DB_LIST_MAX);
#else
    CsrBtTdDbInit();
#endif

#endif
#endif

#ifdef CSR_BT_INSTALL_CM_CACHE_PARAMS
    CsrMemSet((void *)&cmData->dmVar.dmCacheParamTable, 0, sizeof(cmData->dmVar.dmCacheParamTable));
#endif /* CSR_BT_INSTALL_CM_CACHE_PARAMS */
    CsrMemSet((void *)&cmData->sdcVar.sdcSearchList, 0, sizeof(cmData->sdcVar.sdcSearchList));
    CsrMemSet((void *)&cmData->subscriptions, 0, sizeof(cmData->subscriptions));
#ifdef CSR_BT_INSTALL_CM_AFH
    CsrMemSet((void *)&cmData->afhMaps, 0, sizeof(cmData->afhMaps));
#endif /* CSR_BT_INSTALL_CM_AFH */

    /* Initialize device utility module if supported.*/
    CmDuInit(cmData);
#ifdef CSR_TARGET_PRODUCT_VM
    printSIVersion(synergy_si_version);
#endif
}

/*************************************************************************************
  CsrBtCmHandler:
************************************************************************************/
void CsrBtCmHandler(void **gash)
{
    cmInstanceData_t    *cmData;
    CsrUint16            eventClass=0;
    void *                msg=NULL;

    cmData = (cmInstanceData_t *) (*gash);
    CsrSchedMessageGet(&eventClass , &msg);

    if (eventClass == CSR_SCHED_PRIM)
    {
        SynergyMessageFree(eventClass, msg);
        return;
    }
    cmData->recvMsgP = msg;

    switch(eventClass)
    {
        case CSR_BT_CM_PRIM:
        { /* Received a event from the profiles */
            CsrBtCmProfileProvider(cmData);
            break;
        }
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        case RFCOMM_PRIM :
        { /* Received a event from the RFCOMM layer */
            CsrBtCmRfcArrivalHandler(cmData);
            break;
        }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
        case L2CAP_PRIM :
        { /* Received a event from the L2CAP layer */
            CsrBtCmL2CaArrivalHandler(cmData);
            break;
        }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
        case CSR_BT_BNEP_PRIM:
        { /* Received a event from the BNEP layer */
            CsrBtCmBnepArrivalHandler(cmData);
            break;
        }
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */
        case DM_PRIM:
        { /* Received a event from the DM layer */
            CsrBtCmDmArrivalHandler(cmData);
            break;
        }
        case SDP_PRIM:
        { /* Received a event from the SDP layer */
            CsrBtCmSdcArrivalHandler(cmData);
            break;
        }
#ifndef EXCLUDE_CSR_BCCMD_MODULE
        case CSR_BCCMD_PRIM:
        {
#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
            CsrBtCmBccmdArrivalHandler(cmData);
#else
            CsrBccmdFreeUpstreamMessageContents(CSR_BCCMD_PRIM, cmData->recvMsgP);
#endif
            break;
        }
#endif /* !EXCLUDE_CSR_BCCMD_MODULE */
        case CSR_TM_BLUECORE_PRIM:
        {
#ifndef CSR_TARGET_PRODUCT_VM
            CsrPrim *primType = (CsrPrim *)cmData->recvMsgP;

            if (*primType == CSR_TM_BLUECORE_ACTIVATE_TRANSPORT_CFM)
            {
                CsrTmBluecoreActivateTransportCfm *prim = (CsrTmBluecoreActivateTransportCfm *) cmData->recvMsgP;
                prim = (CsrTmBluecoreActivateTransportCfm *) cmData->recvMsgP;

                if (prim->result == CSR_RESULT_SUCCESS)
                {
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                    CsrBtCmCmeInit();
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
                    CmInitSequenceHandler(cmData,
                                          CM_INIT_SEQ_BLUECORE_ACTIVATE_TRANSPORT_CFM,
                                          CSR_BT_RESULT_CODE_CM_SUCCESS,
                                          CSR_BT_SUPPLIER_CM);
                }
                else
                {
                    CsrBtCmGeneralException(CSR_TM_BLUECORE_PRIM,
                                            *primType,
                                            0,
                                            "CsrTmBluecoreActivateTransportCfm with a failure");
                }
            }
            else
            {
                CsrBtCmGeneralException(CSR_TM_BLUECORE_PRIM,
                                        *primType,
                                        0,
                                        "Unhandle CsrTmBlueCore message");
            }
#endif
            break;
        }
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
        case CSR_HCI_PRIM:
        {
            CsrHciPrim *primType;
            primType = (CsrHciPrim *) msg;

            if (*primType == CSR_HCI_VENDOR_SPECIFIC_EVENT_IND)
            {
                CsrBtCmCmeHciVendorSpecificEventIndHandler(cmData);
            }
            else if (*primType == CSR_HCI_REGISTER_VENDOR_SPECIFIC_EVENT_HANDLER_CFM)
            {
                /* No actions */
                /* Register confirmation for HCI Extension */
            }
            else
            {
                CsrBtCmGeneralException(eventClass,
                                        *primType,
                                        cmData->globalState,
                                        "");
            }
            break;
        }
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

#ifdef EXCLUDE_CSR_BT_SC_MODULE
#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
        case CSR_BT_GATT_PRIM:
            CsrBtCmGattArrivalHandler(cmData);
            break;
#endif
#endif

#if (defined (CSR_BT_LE_ENABLE) && defined (CSR_STREAMS_ENABLE))
        case MESSAGE_MORE_DATA:
        {
            CsrBtCmMessageMoreDataHandler(cmData);
            break;
        }
#endif /* CSR_BT_LE_ENABLE and CSR_STREAMS_ENABLE */

        default:
        {
            CsrBtCmGeneralException(eventClass, 0, cmData->globalState, "");
            break;
        }
    }
    SynergyMessageFree(eventClass, cmData->recvMsgP);
    cmData->recvMsgP = NULL;
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************
    This function is called by the scheduler to perform a graceful shutdown
    of a scheduler task.
    This function must:
    1)    empty the input message queue and free any allocated memory in the
        messages.
    2)    free any instance data that may be allocated.
****************************************************************************/
void CsrBtCmDeinit(void **gash)
{
    CsrUint16 msg_type=0;
    void *msg_data=NULL;
    CsrBool lastMsg;
    CsrUintFast16 i = 0;
    cmInstanceData_t *cmData;

    cmData = (cmInstanceData_t *) (*gash);
    lastMsg = FALSE;

    /* Remove message off both save queues and the scheduler queue */
    while (!lastMsg)
    {
        if (!CsrMessageQueuePop(&cmData->dmVar.saveQueue, &msg_type, &msg_data))
        {
            if (!CsrMessageQueuePop(&cmData->smVar.saveQueue, &msg_type, &msg_data))
            {
                lastMsg = (CsrBool)(!CsrSchedMessageGet(&msg_type, &msg_data));
            }
        }
        if (!lastMsg)
        {
            switch (msg_type)
            {
#ifndef EXCLUDE_CSR_BCCMD_MODULE
                case CSR_BCCMD_PRIM:
                {
                    CsrBccmdFreeUpstreamMessageContents(CSR_BCCMD_PRIM, msg_data);
                    break;
                }
#endif /* !EXCLUDE_CSR_BCCMD_MODULE */
                case CSR_BT_CM_PRIM:
                {
                    CsrPrim *primType = (CsrPrim *) msg_data;

                    if(*primType >= CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)
                    {
                        CsrBtCmFreeUpstreamMessageContents(msg_type, msg_data);
                    }
                    else
                    {
                        CsrBtCmFreeDownstreamMessageContents(msg_type, msg_data);
                    }
                    break;
                }
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
                case RFCOMM_PRIM :
                {
                    rfc_free_primitive(msg_data);
                    msg_data = NULL;
                    break;
                }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

                case DM_PRIM:
                {
                    dm_free_upstream_primitive(msg_data);
                    msg_data = NULL;
                    break;
                }

                case SDP_PRIM:
                {
                    sdp_free_upstream_primitive(msg_data);
                    msg_data = NULL;
                    break;
                }

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
                case L2CAP_PRIM:
                {
                    L2CA_FreePrimitive((L2CA_UPRIM_T *) msg_data);
                    msg_data = NULL;
                    break;
                }

#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
                case CSR_BT_BNEP_PRIM :
                {
                    CsrPrim    *primType;

                    primType = (CsrPrim *) msg_data;
                    switch (*primType)
                    {
                        case BNEP_EXTENDED_DATA_IND :
                        {
                            BNEP_EXTENDED_DATA_IND_T *prim;

                            prim = (BNEP_EXTENDED_DATA_IND_T *) msg_data;
                            CsrMblkDestroy(prim->mblk);
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */
                default:
                {
                    break;
                }
            }
            SynergyMessageFree (msg_type, msg_data);
        }

    }

    /* Now free the remaining instance data */

    CsrBtCmCallbackDeinit(cmData);

    CsrBtCmRemoveCacheParamTable(cmData);

    CsrBtCmSdcSearchListDeinit((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList);

    CsrBtCmFlushCmCacheStopTimer(cmData);
    CsrPmemFree(cmData->dmVar.eventFilters);
    cmData->dmVar.eventFilters = NULL;

    CsrPmemFree(cmData->dmVar.localName);
    cmData->dmVar.localName = NULL;

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    CsrCmnListDeinit(&(cmData->rfcVar.connList));
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifdef CSR_BT_INSTALL_CM_AUTO_EIR
    if (cmData->dmVar.localEirData)
    {
        /* Free all Extended Inquiry Response related instance data */
        CsrPmemFree(cmData->dmVar.localEirData->currentName);
        cmData->dmVar.localEirData->currentName = NULL;

        CsrPmemFree(cmData->dmVar.localEirData->services);
        cmData->dmVar.localEirData->services = NULL;

        CsrPmemFree(cmData->dmVar.localEirData->manufacturerData);
        cmData->dmVar.localEirData->manufacturerData = NULL;

        CsrPmemFree(cmData->dmVar.localEirData->requestedName);
        cmData->dmVar.localEirData->requestedName = NULL;

        CsrPmemFree(cmData->dmVar.localEirData->requestedServices);
        cmData->dmVar.localEirData->requestedServices = NULL;

        CsrPmemFree(cmData->dmVar.localEirData);
        cmData->dmVar.localEirData = NULL;
    }
#endif /* CSR_BT_INSTALL_CM_AUTO_EIR */

#ifdef CSR_BT_LE_ENABLE
    CsrBtCmLeDeinit(cmData);
#endif

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    CsrCmnListDeinit(&(cmData->l2caVar.connList));
#ifdef CSR_BT_INSTALL_L2CAP_CONNLESS_SUPPORT
    CsrCmnListDeinit(&(cmData->l2caVar.connlessList));
#endif
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

    CsrPCmnListDeinit(cmData->pendingMsgs);

#ifdef CSR_BT_INSTALL_CM_AFH
    CsrCmnListDeinit((CsrCmnList_t *) &cmData->afhMaps);
#endif /* CSR_BT_INSTALL_CM_AFH */

    /* Reset the data */
    CsrBtCmInitInstData(cmData);
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_DEINITIALIZED
    {
        CsrBtResultCode resultCode = CSR_BT_SUPPLIER_CM;
        CsrBtSupplier   resultSupplier = CSR_BT_RESULT_CODE_CM_SUCCESS;

        CsrBtCmPropgateEvent(cmData,
                                 CsrBtCmPropgateBlueCoreDeInitializedEvents,
                                 CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_DEINITIALIZED,
                                 HCI_SUCCESS,
                                 &resultCode,
                                 &resultSupplier);
    }
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_DEINITIALIZED */

    CsrCmnListDeinit((CsrCmnList_t *) &cmData->subscriptions);

#ifdef INCLUDE_BT_WEARABLE_TD_DB_MEM
    CsrBtTdDbDeInit();
#endif /* INCLUDE_BT_WEARABLE_TD_DB_MEM */

#ifndef CSR_BT_GLOBAL_INSTANCE
    CsrPmemFree(cmData);
    cmData = NULL;    
#else
    CsrMemSet(cmData, 0, sizeof(cmInstanceData_t));
#endif
    *gash = NULL;
}
#endif /* ENABLE_SHUTDOWN */

