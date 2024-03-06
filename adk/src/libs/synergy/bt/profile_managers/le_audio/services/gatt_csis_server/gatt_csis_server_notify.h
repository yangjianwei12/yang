/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#ifndef GATT_CSIS_SERVER_NOTIFY_H_
#define GATT_CSIS_SERVER_NOTIFY_H_

#include "gatt_csis_server_private.h"


void csisServerSendCharacteristicChangedNotification(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 size_value,
        uint8  *const value
        );

void csisServerNotifySirkChange(GCSISS_T* csis_server, connection_id_t cid);

void csisServerNotifySizeChange(GCSISS_T* csis_server, connection_id_t cid);

void csisServerNotifyLockChange(GCSISS_T* csis_server, connection_id_t cid);

void csisServerNotifyLockChangeOtherClients(GCSISS_T* csis_server, connection_id_t cid);



#endif

