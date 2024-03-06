/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#ifndef GATT_CSIS_SERVER_LOCK_MANAGEMENT_H_
#define GATT_CSIS_SERVER_LOCK_MANAGEMENT_H_

#include "gatt_csis_server_private.h"

void csisServerUpdateLock(
                    GattCsisServerLocktype lock,
                    uint16 lock_timeout,
                    CsisServerServiceHandleType handle,
                    connection_id_t  cid,
                    CsrBtTpdAddrT* tp_addr);

GattCsisServerLocktype csisServerGetLockState(void);

uint16 csisServerGetLockTimeout(void);

connection_id_t csisServerGetClientLockConnId(void);

void csisServerHandleLockTimerExpiry(CsrUint16 cid, void *data);

uint8 csisServerEvaluateLockValue(connection_id_t cid,
            uint16 size,
            GattCsisServerLocktype value,
            uint8 *update_timer,
            CsrBtTpdAddrT* tpAdd);

void csisServerUpdateLockCid(connection_id_t cid);

void CsisMessageSendLater(AppTask task, void* msg);

CsrBtTpdAddrT csisServerGetTpAddrFromCid(connection_id_t cid);

#endif

