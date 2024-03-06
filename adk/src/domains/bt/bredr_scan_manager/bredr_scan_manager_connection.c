/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version       
    \file
    \ingroup   bredr_scan_manager
    \brief	    BREDR scan manager interface to connection library.
*/

#include "bredr_scan_manager_private.h"
#include "connection_abstraction.h"

void bredrScanManager_ConnectionWriteScanEnable(void)
{
#ifndef USE_SYNERGY
    hci_scan_enable enable = hci_scan_enable_off;
#endif
    bsm_scan_context_t *page_scan = bredrScanManager_PageScanContext();
    bsm_scan_context_t *inquiry_scan = bredrScanManager_InquiryScanContext();
    bool page = FALSE;
    bool inquiry = FALSE;

    if (TEST_BSM_STATE(page_scan->state, BSM_SCAN_ENABLED) || TEST_BSM_STATE(page_scan->state, BSM_SCAN_ENABLING))
    {
        page = TRUE;
    }
    if (TEST_BSM_STATE(inquiry_scan->state, BSM_SCAN_ENABLED) || TEST_BSM_STATE(inquiry_scan->state, BSM_SCAN_ENABLING))
    {
        inquiry = TRUE;
    }

#ifdef USE_SYNERGY
    DEBUG_LOG("bredrScanManager_ConnectionWriteScanEnable inquiry %d page %d", inquiry, page);
    CmWriteScanEnableReqSend(bredrScanManager_GetTask(), !inquiry, !page);
#else
    if (page)
    {
        enable = inquiry ? hci_scan_enable_inq_and_page : hci_scan_enable_page;
    }
    else
    {
        enable = inquiry ? hci_scan_enable_inq : hci_scan_enable_off;
    }

    DEBUG_LOG("bredrScanManager_ConnectionWriteScanEnable enable %d", enable);

    ConnectionWriteScanEnable(enable);
#endif
}

void bredrScanManager_ConnectionWriteScanActivity(bsm_scan_context_t *context)
{
    bredr_scan_manager_scan_parameters_t *params = context->curr_scan_params;

    if (context == bredrScanManager_InquiryScanContext())
    {
        DEBUG_LOG("bredrScanManager_ConnectionWriteScanActivity: Updating inquiry scan settings i:%d, w:%d", params->interval, params->window);
#ifdef USE_SYNERGY
        CmWriteInquiryScanSettingsReqSend(bredrScanManager_GetTask(), params->interval, params->window);
#else
        ConnectionWriteInquiryscanActivity(params->interval, params->window);
#endif
    }
    else
    {
        DEBUG_LOG("bredrScanManager_ConnectionWriteScanActivity: Updating page scan / truncated page scan type and parameters i:%d w:%d t:%d", params->interval, params->window, params->scan_type);
#ifdef USE_SYNERGY
        CmWritePageScanTypeReqSend(bredrScanManager_GetTask(), params->scan_type);
        CmWritePageScanSettingsReqSend(bredrScanManager_GetTask(), params->interval, params->window);
#else
        ConnectionWritePageScanType(params->scan_type);
        ConnectionWritePagescanActivity(params->interval, params->window);
#endif
    }
}

void bredrScanManager_ConnectionHandleClDmWriteScanEnableCfm(void)
{
    /* try the PS context first */
    if (!bredrScanManager_InstanceCompleteTransition(bredrScanManager_PageScanContext()))
    {
        /* if we were not waiting for a confirmation in the PS context try the IS context
         * It is important to only process a single change per confirmation message
         * If confirmation to PS change request triggerst an IS change request we could
         * mistakenly believe it was done already even though that is not the case
         */
        bredrScanManager_InstanceCompleteTransition(bredrScanManager_InquiryScanContext());
        /* note: TPS notifications come through bredrScanManager_HandleQcomTruncatedScanEnableCfm() */
    }
}
