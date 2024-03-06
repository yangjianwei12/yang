/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#ifndef GATT_MICS_CLIENT_NOTIFICATION_H_
#define GATT_MICS_CLIENT_NOTIFICATION_H_

#include "gatt_mics_client_private.h"

/* MICS notification value */
#define MICS_NOTIFICATION_VALUE   (0x01)

/***************************************************************************
NAME
    micsClientHandleInternalRegisterForNotification

DESCRIPTION
    Handle a MICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ message.
*/
void micsClientHandleInternalRegisterForNotification(GMICSC *const mics_client,
                                                    bool enable,
                                                    uint16 handle);

#endif
