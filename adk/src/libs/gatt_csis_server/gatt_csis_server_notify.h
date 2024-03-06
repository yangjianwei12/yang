/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_CSIS_SERVER_NOTIFY_H_
#define GATT_CSIS_SERVER_NOTIFY_H_

#include "gatt_csis_server_private.h"

void csisServerSendCharacteristicChangedNotification(
        Task task,
        uint16 cid,
        uint16 handle,
        uint16 size_value,
        const uint8 *value
        );

void csisServerNotifySirkChange(GCSISS_T *csis_server, connection_id_t cid);

void csisServerNotifySizeChange(GCSISS_T *csis_server, connection_id_t cid);

void csisServerNotifyLockChange(GCSISS_T *csis_server, connection_id_t cid);

void csisServerNotifyLockChangeOtherClients(GCSISS_T *csis_server, connection_id_t cid);


#endif

