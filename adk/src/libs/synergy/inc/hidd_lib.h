/******************************************************************************
 Copyright (c) 2021-22 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 DESCRIPTION
     Header file providing mapping for Synergy HIDD profile's public interfaces.
     Refer to csr_bt_hidd_lib.h for APIs descriptions.
******************************************************************************/
#ifndef HIDD_LIB_H
#define HIDD_LIB_H

#include "synergy.h"
#include "csr_bt_hidd_lib.h"
#include "csr_bt_profiles.h"
#include "csr_bt_tasks.h"
#define HIDD_PRIM             (SYNERGY_EVENT_BASE + CSR_BT_HIDD_PRIM)

#define HiddActivateReqSend(_appHandle, _qosCtrl, _qosIntr, _flushTimeout, _deviceAddr, _hidSdpLen, _hidSdp) \
    CsrBtHiddActivateReqSend(CSR_BT_HIDD_IFACEQUEUE, TrapToOxygenTask(_appHandle), _qosCtrl, _qosIntr, _flushTimeout, _deviceAddr, 0, NULL, _hidSdpLen, _hidSdp)

#define HiddDeactivateReqSend() \
    CsrBtHiddDeactivateReqSend(CSR_BT_HIDD_IFACEQUEUE)

#define HiddDataReqSend(_reportLen, _report) \
    CsrBtHiddDataReqSend(CSR_BT_HIDD_IFACEQUEUE, _reportLen, _report)

#define HiddControlResSend(_transactionType, _parameter, _dataLen, _data) \
    CsrBtHiddControlResSend(CSR_BT_HIDD_IFACEQUEUE, _transactionType, _parameter, _dataLen, _data)

#define HiddUnplugReqSend(_deviceAddr) \
    CsrBtHiddUnplugReqSend(CSR_BT_HIDD_IFACEQUEUE, _deviceAddr)

#define HiddModeChangeReqSend(_mode) \
    CsrBtHiddModeChangeReqSend(CSR_BT_HIDD_IFACEQUEUE, _mode)

#define HiddFreeUpstreamMessageContents(_msg) \
    CsrBtHiddFreeUpstreamMessageContents(CSR_BT_HIDD_PRIM, _msg)

#endif // HIDD_LIB_H
