/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_CSIS_SERVER_LOCK_MANAGEMENT_H_
#define GATT_CSIS_SERVER_LOCK_MANAGEMENT_H_

#include "gatt_csis_server_private.h"

void csisServerUpdateLock(
                    GattCsisServerLocktype lock,
                    uint16 lock_timeout,
                    ServiceHandle handle,
                    connection_id_t  cid,
                    tp_bdaddr *tp_addr);

GattCsisServerLocktype csisServerGetLockState(void);

uint16 csisServerGetLockTimeout(void);

connection_id_t csisServerGetClientLockConnId(void);

void csisServerHandleLockTimerExpiry(GCSISS_T *csis_server, connection_id_t cid);
tp_bdaddr csisServerGetTpAddrFromCid(connection_id_t cid);

uint8 csisServerEvaluateLockValue(connection_id_t cid,
            uint16 size,
            GattCsisServerLocktype value,
            uint8 *update_timer,
            tp_bdaddr *tpAddr);

void csisServerUpdateLockCid(connection_id_t cid);

#endif

