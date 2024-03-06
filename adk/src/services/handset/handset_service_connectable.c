/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    handset_service
    \brief      Handset service connectable
*/
#include "handset_service_connectable.h"
#include "handset_service_protected.h"

#include <bredr_scan_manager.h>
#include <bt_device.h>


static bool allow_bredr;
static bool observing_connections;

static handset_service_connectable_t bredr_state;

static void handsetService_CancelFastPageScanTimeout(void)
{
    MessageCancelFirst(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_FAST_PAGE_SCAN_TIMEOUT_IND);
}

static void handsetService_SendFastPageScanTimeout(void)
{
    handsetService_CancelFastPageScanTimeout();
    MessageSendLater(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_FAST_PAGE_SCAN_TIMEOUT_IND, NULL, HANDSET_SERVICE_FAST_PAGE_SCAN_TIMEOUT_MS);
}

void handsetService_ConnectableInit(void)
{
    allow_bredr = FALSE;
    bredr_state = handset_service_connectable_disable;
    observing_connections = FALSE;

    /* Register for connection notifications if peer setup is complete,
       or is not needed. */
    if (BtDevice_IsPeerSetupComplete())
    {
        handsetService_ObserveConnections();
    }
}

void handsetService_ObserveConnections(void)
{
    if(!observing_connections)
    {
        ConManagerRegisterTpConnectionsObserver(cm_transport_all, HandsetService_GetTask());
        observing_connections = TRUE;
    }
}

void handsetService_DontObserveConnections(void)
{
    if(observing_connections)
    {
        ConManagerUnregisterTpConnectionsObserver(cm_transport_all, HandsetService_GetTask());
        observing_connections = FALSE;
    }
}

void handsetService_ConnectableEnableBredr(handset_service_connectable_t setting)
{
    if(bredr_state == setting)
    {
        return;
    }
    
    if(!allow_bredr && setting != handset_service_connectable_disable)
    {
        return;
    }
    
    switch(setting)
    {
        case handset_service_connectable_enable_fast:
            BredrScanManager_PageScanRequest(HandsetService_GetTask(), SCAN_MAN_PARAMS_TYPE_FAST);
            handsetService_SendFastPageScanTimeout();
        break;
        
        case handset_service_connectable_enable_slow:
            BredrScanManager_PageScanRequest(HandsetService_GetTask(), SCAN_MAN_PARAMS_TYPE_SLOW);
        break;
        
        case handset_service_connectable_disable:
            BredrScanManager_PageScanRelease(HandsetService_GetTask());
            handsetService_CancelFastPageScanTimeout();
        break;
    }
    
    bredr_state = setting;
}

void handsetService_ConnectableAllowBredr(bool allow)
{
    allow_bredr = allow;
}

void HandsetService_HandleEarbudCreated(void)
{
    handsetService_ObserveConnections();
}

void handsetService_HandleFastPageScanTimeout(void)
{
    if(allow_bredr && bredr_state != handset_service_connectable_disable)
    {
        handsetService_ConnectableEnableBredr(handset_service_connectable_enable_slow);
    }
}

void HandsetService_EnableTruncatedPageScan(void)
{
    if (allow_bredr)
    {
        HS_LOG("HandsetService_EnableTruncatedPageScan - request truncated page scan");
        BredrScanManager_TruncatedPageScanRequest(HandsetService_GetTask(), SCAN_MAN_PARAMS_TYPE_SLOW);
    }
    else
    {
        HS_LOG("HandsetService_EnableTruncatedPageScan - BR/EDR disabled");
    }
}

void HandsetService_DisableTruncatedPageScan(void)
{
    HS_LOG("HandsetService_DisableTruncatedPageScan");
    MessageCancelAll(HandsetService_GetTask(), HANDSET_SERVICE_INTERNAL_TRUNC_PAGE_SCAN_ENABLE);
    BredrScanManager_TruncatedPageScanRelease(HandsetService_GetTask());
}

void HandsetService_HandleInternalTruncatedPageScanEnable(void)
{
    if (HandsetServiceSm_MaxBredrAclConnectionsReached())
    {
        HandsetService_EnableTruncatedPageScan();
    }
}
