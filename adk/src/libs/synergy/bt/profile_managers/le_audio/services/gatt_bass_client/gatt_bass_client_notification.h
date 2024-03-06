/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_BASS_CLIENT_NOTIFICATION_H_
#define GATT_BASS_CLIENT_NOTIFICATION_H_


#include "gatt_bass_client_private.h"

/* BASS notification value */
#define BASS_NOTIFICATION_VALUE   (0x01)

/***************************************************************************
NAME
    bassClientHandleInternalRegisterForNotification

DESCRIPTION
    Handle a BASS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ message.
*/
void bassClientHandleInternalRegisterForNotification(GBASSC *const bass_client,
                                                     bool enable,
                                                     uint16 handle);


/***************************************************************************
NAME
    bassClientHandleClientNotification

DESCRIPTION
    Handle CsrBtGattClientNotificationInd message.
*/
void bassClientHandleClientNotification(GBASSC *bass_client,
                                        const CsrBtGattClientNotificationInd *ind);
#endif
