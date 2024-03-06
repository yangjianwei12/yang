/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_AICS_CLIENT_NOTIFICATION_H_
#define GATT_AICS_CLIENT_NOTIFICATION_H_

#include "gatt_aics_client_private.h"

/* AICS notification value */
#define AICS_NOTIFICATION_VALUE   (0x01)

/***************************************************************************
NAME
    aicsClientHandleInternalRegisterForNotification

DESCRIPTION
    Handle a AICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ message.
*/
void aicsClientHandleInternalRegisterForNotification(GAICS *const aics_client,
                                                     bool enable,
                                                     uint16 handle);

#endif
