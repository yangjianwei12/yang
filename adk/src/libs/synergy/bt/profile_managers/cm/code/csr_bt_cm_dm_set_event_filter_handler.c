/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "hci_prim.h"
#include "csr_bt_cm_lib.h"

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#include "csr_bt_cm_bccmd.h"
#endif

#include "csr_bt_cm_util.h"

static void sendSetEventFilterCfm(CsrSchedQid                   appHandle,
                                  CsrBtResultCode       resultCode,
                                  CsrBtSupplier   resultSupplier,
                                  CsrBtCmPrim           pendingFilterRequest)
{
    void *resultPrim = NULL;

    switch(pendingFilterRequest)
    {
#ifdef CSR_BT_INSTALL_CM_PRI_SET_EVENT_FILTER_BDADDR
        case CSR_BT_CM_SET_EVENT_FILTER_BDADDR_REQ:
        {
            CsrBtCmSetEventFilterBdaddrCfm *cmPrim;
            cmPrim = (CsrBtCmSetEventFilterBdaddrCfm*)CsrPmemAlloc(sizeof(CsrBtCmSetEventFilterBdaddrCfm));

            cmPrim->type            = CSR_BT_CM_SET_EVENT_FILTER_BDADDR_CFM;
            cmPrim->resultCode      = resultCode;
            cmPrim->resultSupplier  = resultSupplier;
            resultPrim              = cmPrim;
            break;
        }
#endif
        case CSR_BT_CM_SET_EVENT_FILTER_COD_REQ:
        {
            CsrBtCmSetEventFilterCodCfm *cmPrim;
            cmPrim = (CsrBtCmSetEventFilterCodCfm*)CsrPmemAlloc(sizeof(CsrBtCmSetEventFilterCodCfm));

            cmPrim->type            = CSR_BT_CM_SET_EVENT_FILTER_COD_CFM;
            cmPrim->resultCode      = resultCode;
            cmPrim->resultSupplier  = resultSupplier;
            resultPrim              = cmPrim;
            break;
        }

        case CSR_BT_CM_CLEAR_EVENT_FILTER_REQ:
        {
            CsrBtCmClearEventFilterCfm *cmPrim;
            cmPrim = (CsrBtCmClearEventFilterCfm*)CsrPmemAlloc(sizeof(CsrBtCmClearEventFilterCfm));

            cmPrim->type            = CSR_BT_CM_CLEAR_EVENT_FILTER_CFM;
            cmPrim->resultCode      = resultCode;
            cmPrim->resultSupplier  = resultSupplier;
            resultPrim              = cmPrim;
            break;
        }
        default:
        {
            /* This should not happen! */
        }
    }
    if(resultPrim != NULL)
    {
        CsrBtCmPutMessage(appHandle, resultPrim);
    }
}

#if !defined(EXCLUDE_CSR_BT_CM_BCCMD_FEATURE) || defined(CSR_BT_INSTALL_CM_PRI_SET_EVENT_FILTER_BDADDR) || \
    defined(INSTALL_CM_SET_EVENT_FILTER_COD) || defined(INSTALL_CM_CLEAR_EVENT_FILTER)
static CsrBool filterFitsInList(cmInstanceData_t *cmData)
{
    CsrUintFast8 i;

    for (i = 0; i < cmData->dmVar.maxEventFilters; i++)
    {
        if (cmData->dmVar.eventFilters[i].type == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static CsrBool filterIsInList(cmInstanceData_t *cmData)
{
    CsrUintFast8 i;

    for (i = 0; i < cmData->dmVar.maxEventFilters; i++)
    {
        if (CsrMemCmp((CsrUint8 *)&(cmData->dmVar.eventFilters[i]), (CsrUint8 *)&cmData->dmVar.filterInProgress, sizeof(eventFilterStruct)) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void writeFilterToChip(cmInstanceData_t *cmData)
{
#ifdef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
    if (cmData->dmVar.eventFilters == NULL)
    {
        /* For QC chips the max event filters are fixed to 5 and it cannot be 
            read or modified */
        cmData->dmVar.maxEventFilters = CM_QCA_MAX_EVENT_FILTERS;
        cmData->dmVar.eventFilters = (eventFilterStruct *)CsrPmemZalloc(cmData->dmVar.maxEventFilters * sizeof(eventFilterStruct));
    }
#else /* EXCLUDE_CSR_BT_CM_BCCMD_FEATURE */
    if (cmData->dmVar.eventFilters == NULL)
    {
        CsrBccmdReadPsValueReqSend(CSR_BT_CM_IFACEQUEUE, 0x0001, PSKEY_LM_MAX_EVENT_FILTERS, CSR_BCCMD_STORES_DEFAULT, 2);
    }
    else
#endif /* !EXCLUDE_CSR_BT_CM_BCCMD_FEATURE */
    {

        if (filterIsInList(cmData) && (cmData->dmVar.pendingFilterRequest != CSR_BT_CM_CLEAR_EVENT_FILTER_REQ))
        {
            /* Do not write existing filters to chip */
            sendSetEventFilterCfm(cmData->dmVar.appHandle, CSR_BT_RESULT_CODE_CM_SUCCESS,
                                CSR_BT_SUPPLIER_CM, cmData->dmVar.pendingFilterRequest);

            CsrBtCmDmLocalQueueHandler();
            CsrMemSet(&cmData->dmVar.filterInProgress, 0, sizeof(eventFilterStruct));
        }
        else if (filterFitsInList(cmData))
        {
            dm_hci_set_event_filter(cmData->dmVar.filterInProgress.type,
                                    cmData->dmVar.filterInProgress.conditionType,
                                    &cmData->dmVar.filterInProgress.condition,
                                    NULL);
        }
        else
        {
            sendSetEventFilterCfm(cmData->dmVar.appHandle,
                        CSR_BT_RESULT_CODE_CM_NUMBER_OF_LM_EVENT_FILTERS_EXCEEDED,
                        CSR_BT_SUPPLIER_CM, cmData->dmVar.pendingFilterRequest);
            CsrBtCmDmLocalQueueHandler();
            CsrMemSet(&cmData->dmVar.filterInProgress, 0, sizeof(eventFilterStruct));
        }
    }
}
#endif /* !EXCLUDE_CSR_BT_CM_BCCMD_FEATURE OR CSR_BT_INSTALL_CM_PRI_SET_EVENT_FILTER_BDADDR OR INSTALL_CM_SET_EVENT_FILTER_COD OR INSTALL_CM_CLEAR_EVENT_FILTER */

static void insertFilterIntoList(cmInstanceData_t *cmData)
{
    CsrUintFast8 i;

    for (i = 0; i < cmData->dmVar.maxEventFilters; i++)
    {
        eventFilterStruct filter;
        filter = cmData->dmVar.eventFilters[i];
        if (filter.type == 0)
        {
            SynMemCpyS(&cmData->dmVar.eventFilters[i], sizeof(eventFilterStruct), &cmData->dmVar.filterInProgress, sizeof(eventFilterStruct));
            return;
        }
    }
}

void CsrBtCmSetEventFilterCommonCfmHandler(cmInstanceData_t *cmData)
{
    DM_HCI_SET_EVENT_FILTER_CFM_T *dmPrim = (DM_HCI_SET_EVENT_FILTER_CFM_T*) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_DUT_MODE
    if (cmData->dmVar.deviceUnderTest)
    {
        if (dmPrim->status == HCI_SUCCESS)
        {
            cmData->dmVar.pendingChipScanMode = HCI_SCAN_ENABLE_INQ_AND_PAGE;
            dm_hci_write_scan_enable(HCI_SCAN_ENABLE_INQ_AND_PAGE, NULL);
        }
        else
        {
            CsrBtCmSendDeviceUnderTestComplete(cmData->dmVar.appHandle,
                                               dmPrim->status,
                                               1);
        }
    }
    else
#endif
    {
        if (dmPrim->status == HCI_SUCCESS)
        {
            if (cmData->dmVar.pendingFilterRequest != CSR_BT_CM_CLEAR_EVENT_FILTER_REQ)
            {
                insertFilterIntoList(cmData);
            }
            sendSetEventFilterCfm(cmData->dmVar.appHandle,
                                  CSR_BT_RESULT_CODE_CM_SUCCESS,
                                  CSR_BT_SUPPLIER_CM,
                                  cmData->dmVar.pendingFilterRequest);
        }
        else
        {
            sendSetEventFilterCfm(cmData->dmVar.appHandle,
                                  (CsrBtResultCode) dmPrim->status,
                                  CSR_BT_SUPPLIER_HCI,
                                  cmData->dmVar.pendingFilterRequest);
        }

        CsrBtCmDmLocalQueueHandler();
        CsrMemSet(&cmData->dmVar.filterInProgress,
                  0,
                  sizeof(eventFilterStruct));
    }
}

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
void CsrBtCmBccmdMaxEventFilterHandler(cmInstanceData_t *cmData)
{
    CsrBccmdCfm * prim =  (CsrBccmdCfm *) cmData->recvMsgP;

    if (prim->status == CSR_RESULT_SUCCESS)
    {
        cmData->dmVar.maxEventFilters = prim->payload[6]; /* Upper 8 bits are not used */

        if (cmData->dmVar.maxEventFilters > 0)
        {
            cmData->dmVar.eventFilters = (eventFilterStruct *)CsrPmemAlloc(cmData->dmVar.maxEventFilters * sizeof(eventFilterStruct));
            CsrMemSet(cmData->dmVar.eventFilters, 0, cmData->dmVar.maxEventFilters * sizeof(eventFilterStruct));
            writeFilterToChip(cmData);
        }
        else
        {
            sendSetEventFilterCfm(cmData->dmVar.appHandle, CSR_BT_RESULT_CODE_CM_UNSPECIFIED_ERROR,
                                                CSR_BT_SUPPLIER_CM, cmData->dmVar.pendingFilterRequest);
            CsrBtCmDmLocalQueueHandler();
            CsrMemSet(&cmData->dmVar.filterInProgress, 0, sizeof(eventFilterStruct));
        }
    }
    else
    {
        sendSetEventFilterCfm(cmData->dmVar.appHandle, (CsrBtResultCode)prim->status,
                                            CSR_BT_SUPPLIER_BCCMD, cmData->dmVar.pendingFilterRequest);
        CsrBtCmDmLocalQueueHandler();
        CsrMemSet(&cmData->dmVar.filterInProgress, 0, sizeof(eventFilterStruct));
    }
    CsrPmemFree(prim->payload);
    prim->payload = NULL;
}
#endif /* !EXCLUDE_CSR_BT_CM_BCCMD_FEATURE */

/* Add BDADDR type filter */
#ifdef CSR_BT_INSTALL_CM_PRI_SET_EVENT_FILTER_BDADDR
void CsrBtCmSetEventFilterBdaddrReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSetEventFilterBdaddrReq  *cmPrim;

    cmPrim = (CsrBtCmSetEventFilterBdaddrReq*)cmData->recvMsgP;
    cmData->dmVar.pendingFilterRequest = cmPrim->type;
    cmData->dmVar.appHandle            = cmPrim->appHandle;

    /* Setup filter */
    CsrMemSet(&cmData->dmVar.filterInProgress.condition, 0, sizeof(CONDITION_T));
    if(cmPrim->selectInquiryFilter)
    {
        cmData->dmVar.filterInProgress.type = INQUIRY_RESULT_FILTER;
        cmData->dmVar.filterInProgress.condition.bd_addr = cmPrim->address;
    }
    else
    {
        cmData->dmVar.filterInProgress.type = CONNECTION_FILTER;
        cmData->dmVar.filterInProgress.condition.bd_addr     = cmPrim->address;
        cmData->dmVar.filterInProgress.condition.auto_accept = cmPrim->autoAccept;
    }

    cmData->dmVar.filterInProgress.conditionType = ADDRESSED_DEVICE_RESPONDED;

    writeFilterToChip(cmData);
}
#endif

#ifdef INSTALL_CM_SET_EVENT_FILTER_COD
/* Add COD type filter */
void CsrBtCmSetEventFilterCodReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSetEventFilterCodReq  *cmPrim;
    cmPrim = (CsrBtCmSetEventFilterCodReq*)cmData->recvMsgP;
    cmData->dmVar.pendingFilterRequest = cmPrim->type;
    cmData->dmVar.appHandle            = cmPrim->appHandle;

    /* Setup filter */
    CsrMemSet(&cmData->dmVar.filterInProgress.condition, 0, sizeof(CONDITION_T));
    if(cmPrim->selectInquiryFilter)
    {
        cmData->dmVar.filterInProgress.type                                     = INQUIRY_RESULT_FILTER;
        cmData->dmVar.filterInProgress.condition.class_mask.class_of_device     = cmPrim->cod;
        cmData->dmVar.filterInProgress.condition.class_mask.mask                = cmPrim->codMask;
    }
    else
    {
        cmData->dmVar.filterInProgress.type                                     = CONNECTION_FILTER;
        cmData->dmVar.filterInProgress.condition.cma.auto_accept                = cmPrim->autoAccept;
        cmData->dmVar.filterInProgress.condition.cma.class_mask.class_of_device = cmPrim->cod;
        cmData->dmVar.filterInProgress.condition.cma.class_mask.mask            = cmPrim->codMask;
    }

    cmData->dmVar.filterInProgress.conditionType = CLASS_OF_DEVICE_RESPONDED;

    writeFilterToChip(cmData);
}
#endif /* INSTALL_CM_SET_EVENT_FILTER_COD */

#ifdef INSTALL_CM_CLEAR_EVENT_FILTER
/* Clear filter(s) */
void CsrBtCmClearEventFilterReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmClearEventFilterReq  *cmPrim;

    cmPrim = (CsrBtCmClearEventFilterReq*)cmData->recvMsgP;
    cmData->dmVar.pendingFilterRequest = cmPrim->type;
    cmData->dmVar.appHandle            = cmPrim->appHandle;

    CsrMemSet(&cmData->dmVar.filterInProgress, 0, sizeof(eventFilterStruct));

    switch(cmPrim->filter)
    {
    case CSR_BT_EVENT_FILTER_CONNECTIONS:
        {
            CsrUintFast8 i;
            for (i = 0; i < cmData->dmVar.maxEventFilters; i++)
            {
                if (cmData->dmVar.eventFilters[i].type == CONNECTION_FILTER)
                {
                    CsrMemSet(cmData->dmVar.eventFilters + i, 0, sizeof(eventFilterStruct));
                }
            }
            cmData->dmVar.filterInProgress.type = CONNECTION_FILTER;
            break;
        }
    case CSR_BT_EVENT_FILTER_INQUIRY:
        {
            CsrUintFast8 i;
            for (i = 0; i < cmData->dmVar.maxEventFilters; i++)
            {
                if (cmData->dmVar.eventFilters[i].type == INQUIRY_RESULT_FILTER)
                {
                    CsrMemSet(cmData->dmVar.eventFilters + i, 0, sizeof(eventFilterStruct));
                }
            }
            cmData->dmVar.filterInProgress.type = INQUIRY_RESULT_FILTER;
            break;
        }
    case CSR_BT_EVENT_FILTER_ALL:
        {
            cmData->dmVar.filterInProgress.type = CLEAR_ALL_FILTERS;
            CsrMemSet(cmData->dmVar.eventFilters, 0, cmData->dmVar.maxEventFilters * sizeof(eventFilterStruct));
            break;
        }
        default:
        {
            sendSetEventFilterCfm(cmData->dmVar.appHandle, CSR_BT_RESULT_CODE_CM_UNACCEPTABLE_PARAMETER,
                                                CSR_BT_SUPPLIER_CM, cmData->dmVar.pendingFilterRequest);
            CsrBtCmDmLocalQueueHandler();
            break;
        }
    }
    writeFilterToChip(cmData);
}
#endif /* INSTALL_CM_CLEAR_EVENT_FILTER */

