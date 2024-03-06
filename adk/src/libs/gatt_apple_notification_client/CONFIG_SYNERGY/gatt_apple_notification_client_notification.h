/* Copyright (c) 2014 - 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_APPLE_NOTIFICATION_CLIENT_NOTIFICATION_H_
#define GATT_APPLE_NOTIFICATION_CLIENT_NOTIFICATION_H_

#include "gatt_apple_notification_client.h"
#include <gatt_lib.h>

/***************************************************************************
NAME
    handleAncsNotification

DESCRIPTION
    Handles the internal CSR_BT_GATT_CLIENT_NOTIFICATION_IND message.
*/
void handleAncsNotification(GANCS *ancs, const CsrBtGattClientNotificationInd *ind);

/***************************************************************************
NAME
    ancsSetNSNotificationRequest

DESCRIPTION
    Handles the internal ANCS_INTERNAL_MSG_SET_xx_NOTIFICATION message.
*/
void ancsSetNotificationRequest(GANCS *ancs, bool notifications_enable, uint8 characteristic);

#endif /* GATT_APPLE_NOTIFICATION_CLIENT_NOTIFICATION_H_ */
