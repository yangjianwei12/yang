/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_log_text_2.h"
#include "csr_bt_cm_le.h"

#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING

#define AD_STRUCT_FLAGS_PRESENT (1 << 7)

#ifdef CSR_STREAMS_ENABLE
extern Source StreamExtScanSource(void);
extern void CmStreamFlushSource(Source src);
#endif /* CSR_STREAMS_ENABLE */

void CsrBtCmDmExtScanGetGlobalParamsCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SCAN_GET_GLOBAL_PARAMS_CFM_T *dmPrim = msg;
    CmExtScanGetGlobalParamsCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->flags = dmPrim->flags;
    prim->own_address_type = dmPrim->own_address_type;
    prim->scanning_filter_policy = dmPrim->scanning_filter_policy;
    prim->filter_duplicates = dmPrim->filter_duplicates;
    prim->scanning_phys = dmPrim->scanning_phys;
    SynMemCpyS(prim->phys, sizeof(CmScanningPhy) * CM_EXT_SCAN_MAX_SCANNING_PHYS, 
               dmPrim->phys, sizeof(CmScanningPhy) * CM_EXT_SCAN_MAX_SCANNING_PHYS);

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}


void CsrBtCmDmExtScanSetGlobalParamsCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SCAN_SET_GLOBAL_PARAMS_CFM_T *dmPrim = msg;
    CmExtScanSetGlobalParamsCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}


void CsrBtCmDmExtScanRegisterScannerCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SCAN_REGISTER_SCANNER_CFM_T *dmPrim = msg;
    CmExtScanRegisterScannerCfm *prim;
    CsrUint8 i;

    for(i = 0; i < MAX_EXT_SCAN_APP; i++)
    {
        if(cmData->extScanHandles[i].pending == TRUE)
        {
            cmData->extScanHandles[i].scanHandle = dmPrim->scan_handle;
            cmData->extScanHandles[i].pending = FALSE;

            prim = CsrPmemAlloc(sizeof(*prim));
            prim->type = CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_CFM;
            prim->resultCode = (CsrBtResultCode) dmPrim->status;
            prim->scan_handle = dmPrim->scan_handle;

            CsrSchedMessagePut(cmData->extScanHandles[i].pHandle, CSR_BT_CM_PRIM, (prim));
            break;
        }
    }

    CsrBtCmDmLocalQueueHandler();
}


void CsrBtCmDmExtScanUnregisterScannerCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SCAN_UNREGISTER_SCANNER_CFM_T *dmPrim = msg;
    CmExtScanUnregisterScannerCfm *prim;
    CsrUint8 i;

    for (i = 0; i < MAX_EXT_SCAN_APP; i++)
    {
       if (cmData->dmVar.appHandle == cmData->extScanHandles[i].pHandle)
       {
           cmData->extScanHandles[i].pHandle = CSR_SCHED_QID_INVALID;
           cmData->extScanHandles[i].scanHandle = CSR_BT_EXT_SCAN_HANDLE_INVALID;
       }
    }

    if(cmData->dmVar.appHandle != CSR_SCHED_QID_INVALID)
    {
        prim = CsrPmemAlloc(sizeof(*prim));
        prim->type = CSR_BT_CM_EXT_SCAN_UNREGISTER_SCANNER_CFM;
        prim->resultCode = (CsrBtResultCode) dmPrim->status;

        CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    }

    CsrBtCmDmLocalQueueHandler();
}


void CsrBtCmDmExtScanConfigureScannerCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SCAN_CONFIGURE_SCANNER_CFM_T *dmPrim = msg;
    CmExtScanConfigureScannerCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}


void CsrBtCmDmExtScanEnableScannersCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SCAN_ENABLE_SCANNERS_CFM_T *dmPrim = msg;
    CmExtScanEnableScannersCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}


void CsrBtCmDmExtScanGetCtrlScanInfoCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM_T *dmPrim = msg;
    CmExtScanGetCtrlScanInfoCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->num_of_enabled_scanners = dmPrim->num_of_enabled_scanners;
    prim->duration = dmPrim->duration;
    prim->scanning_phys = dmPrim->scanning_phys;
    SynMemCpyS(prim->phys, sizeof(CmScanningPhy) * CM_EXT_SCAN_MAX_SCANNING_PHYS,
               dmPrim->phys, sizeof(CmScanningPhy) * CM_EXT_SCAN_MAX_SCANNING_PHYS);

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}


void CsrBtCmDmExtScanCtrlScanInfoIndHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SCAN_CTRL_SCAN_INFO_IND_T *dmPrim = msg;
    CmExtScanCtrlScanInfoInd *prim;
    CsrSchedQid appHandle = CSR_SCHED_QID_INVALID;
    uint8_t i;

#ifdef CSR_STREAMS_ENABLE
    uint8 extendedScanners = dmPrim->num_of_enabled_scanners;
    if (dmPrim->legacy_scanner_enabled && extendedScanners)
    {
        extendedScanners--;
    }
    /* Extended scan report comes via stream
     * Check if extended scan is enabled then connect to the new stream 
     * or dispose the old one */
    if (extendedScanners) 
    {
        Source src = StreamExtScanSource();
        if (src && src != cmData->extScanSource)
        {
            cmData->extScanSource = src;
            SynergyStreamsSourceRegister(CSR_BT_CM_IFACEQUEUE, cmData->extScanSource);
        }
    }
    else
    {
        /* If any report is being processed then mark for termination and defer invalidating extScanSource */
        if(cmData->extScanState & CSR_BT_EA_PA_REPORT_PROCESSING)
        {
            /* Mark for termination */
            cmData->extScanState |= CSR_BT_EA_PA_TERMINATING;
            CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "CsrBtCmDmExtScanCtrlScanInfoIndHandler extScanSource=%p terminating", cmData->extScanSource));
        }
        else
        {
            /* Flush the data.*/
            CmStreamFlushSource(cmData->extScanSource);
            cmData->extScanSource = 0;
        }
    }
#endif /* CSR_STREAMS_ENABLE */

    for (i = 0; i < MAX_EXT_SCAN_APP; i++)
    {
        appHandle = cmData->extScanHandles[i].pHandle;

        if(appHandle != CSR_SCHED_QID_INVALID)
        {
            prim = CsrPmemAlloc(sizeof(*prim));
            prim->type = CSR_BT_CM_EXT_SCAN_CTRL_SCAN_INFO_IND;
            prim->reason = dmPrim->reason;
            prim->controller_updated = dmPrim->controller_updated;
            prim->num_of_enabled_scanners = dmPrim->num_of_enabled_scanners;
            prim->legacy_scanner_enabled = dmPrim->legacy_scanner_enabled;
            prim->duration = dmPrim->duration;
            prim->scanning_phys = dmPrim->scanning_phys;
            SynMemCpyS(prim->phys, sizeof(CmScanningPhy) * CM_EXT_SCAN_MAX_SCANNING_PHYS, 
                       dmPrim->phys, sizeof(CmScanningPhy) * CM_EXT_SCAN_MAX_SCANNING_PHYS);

            CsrSchedMessagePut(appHandle, CSR_BT_CM_PRIM, (prim));
        }
    }
}

void CsrBtCmDmExtScanFilteredAdvReportIndHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SCAN_FILTERED_ADV_REPORT_IND_T *dmPrim = msg;
    CmExtScanFilteredAdvReportInd *prim;
#ifdef CSR_STREAMS_ENABLE
    CmExtScanFilteredAdvReportDoneInd *donePrim;
    const CsrUint8 *sourcePtr = NULL;
    CsrUint16 sourceSize = 0;
#endif /* CSR_STREAMS_ENABLE */
    CsrSchedQid appHandle = CSR_SCHED_QID_INVALID;
    CsrUint8 i, j;

#ifdef CSR_STREAMS_ENABLE
    if (!cmData->extScanSource)
    {
#ifdef CSR_TARGET_PRODUCT_VM
        CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "CsrBtCmDmExtScanFilteredAdvReportIndHandler cmData->extScanSource=%p report NOT processing", cmData->extScanSource));
#endif
        return;
    }
    /* Do not process Advert Report (MESSAGE_MORE_DATA event) if processing of Advert
     * Report is not completed */
    if (cmData->extScanState & CSR_BT_EA_PA_REPORT_PROCESSING)
    {
#ifdef CSR_TARGET_PRODUCT_VM
        CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "CsrBtCmDmExtScanFilteredAdvReportIndHandler cmData->extScanSource=%p cmData->processingAdvReport=TRUE NOT processing", cmData->extScanSource));
#endif
        return;
    }
    /* Mark the state to indicate Advert Report is getting processed */
    cmData->extScanState |= CSR_BT_EA_PA_REPORT_PROCESSING;

    sourcePtr = SourceMap(cmData->extScanSource);
    sourceSize = SourceBoundary(cmData->extScanSource);
#endif /* CSR_STREAMS_ENABLE */

    for (i = 0; i < dmPrim->num_of_scan_handles; i++)
    {
        for (j = 0; j < MAX_EXT_SCAN_APP; j++)
        {
            if (dmPrim->scan_handles[i] == cmData->extScanHandles[j].scanHandle)
            {
                appHandle = cmData->extScanHandles[j].pHandle;
                break;
            }
        }

        if(appHandle != CSR_SCHED_QID_INVALID)
        {
            prim = CsrPmemAlloc(sizeof(*prim));
            CsrMemSet(prim,0,sizeof(*prim));
            prim->type = CSR_BT_CM_EXT_SCAN_FILTERED_ADV_REPORT_IND;
            prim->eventType = dmPrim->event_type;
            CsrBtAddrCopyWithType(&(prim->currentAddrt),
                    dmPrim->current_addr_type, &(dmPrim->current_addr));
            CsrBtAddrCopyWithType(&(prim->permanentAddrt),
                    dmPrim->permanent_addr_type, &(dmPrim->permanent_addr));
            CsrBtAddrCopyWithType(&(prim->directAddrt),
                    dmPrim->direct_addr_type, &(dmPrim->direct_addr));
            prim->primaryPhy = dmPrim->primary_phy;
            prim->secondaryPhy = dmPrim->secondary_phy;
            prim->advSid = dmPrim->adv_sid;
            prim->txPower = dmPrim->tx_power;
            prim->rssi = dmPrim->rssi;
            prim->periodicAdvInterval = dmPrim->periodic_adv_interval;
            prim->advDataInfo = dmPrim->adv_data_info;
            prim->adFlags = (dmPrim->adv_data_info & AD_STRUCT_FLAGS_PRESENT ? dmPrim->ad_flags : 0);

#ifdef CSR_STREAMS_ENABLE
            if (sourceSize)
            {
                prim->data = (CsrUint8*) sourcePtr;
                prim->dataLength = sourceSize;
            }
#else
            if (dmPrim->adv_data)
            {
                /* Get the MBLK data length */
                prim->dataLength = mblk_get_length(dmPrim->adv_data);
                prim->data = (CsrUint8 *) CsrPmemAlloc(prim->dataLength);
                /* Read data from MBLK */
                mblk_read_head(&dmPrim->adv_data, prim->data, prim->dataLength);
            }
#endif /* CSR_STREAMS_ENABLE */

            CsrSchedMessagePut(appHandle, CSR_BT_CM_PRIM, (prim));
            appHandle = CSR_SCHED_QID_INVALID;
        }
    }

#ifdef CSR_STREAMS_ENABLE
    /* Finally send the Advert Report done indication to CM to free the Advert Report data
     * once processing of Advert Report is completed. */
    donePrim = CsrPmemAlloc(sizeof(*donePrim));
    CsrMemSet(donePrim,0,sizeof(*donePrim));
    donePrim->type = CM_EXT_SCAN_FILTERED_ADV_REPORT_DONE_IND;

    donePrim->data = &cmData->extScanSource;
    donePrim->dataLength = sourceSize;
    CsrSchedMessagePut(CSR_BT_CM_IFACEQUEUE, CSR_BT_CM_PRIM, donePrim);
#endif /* CSR_STREAMS_ENABLE */

}


void CsrBtCmDmExtScanDurationExpiredIndHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SCAN_DURATION_EXPIRED_IND_T *dmPrim = msg;
    CmExtScanDurationExpiredInd *prim;
    CsrUint8 i;
    CsrSchedQid appHandle = CSR_SCHED_QID_INVALID;

    for(i = 0; i < MAX_EXT_SCAN_APP; i++)
    {
        if(cmData->extScanHandles[i].scanHandle == dmPrim->scan_handle)
        {
            appHandle = cmData->extScanHandles[i].pHandle;

            if(dmPrim->scan_handle_unregistered)
            {
                cmData->extScanHandles[i].pHandle = CSR_SCHED_QID_INVALID;
                cmData->extScanHandles[i].scanHandle = CSR_BT_EXT_SCAN_HANDLE_INVALID;
                cmData->extScanHandles[i].pending = FALSE;
            }
            break;
        }
    }

    if(appHandle != CSR_SCHED_QID_INVALID)
    {
        prim = CsrPmemAlloc(sizeof(*prim));
        prim->type = CSR_BT_CM_EXT_SCAN_DURATION_EXPIRED_IND;
        prim->scan_handle = dmPrim->scan_handle;
        prim->scan_handle_unregistered = dmPrim->scan_handle_unregistered;

        CsrSchedMessagePut(appHandle, CSR_BT_CM_PRIM, (prim));
    }
}

#ifdef INSTALL_CM_EXT_SET_CONN_PARAM
void CmDmExtSetConnParamsCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_SET_CONNECTION_PARAMS_CFM_T *dmPrim = msg;
    CmDmExtSetConnParamsCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CM_DM_EXT_SET_CONN_PARAMS_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}
#endif /* End of INSTALL_CM_EXT_SET_CONN_PARAM */
#endif /* End of CSR_BT_INSTALL_EXTENDED_SCANNING */
