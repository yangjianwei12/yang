/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_VOCS_CLIENT_NOTIFICATION_H_
#define GATT_VOCS_CLIENT_NOTIFICATION_H_

#include "gatt_vocs_client_private.h"

/* VOCS notification value */
#define VOCS_NOTIFICATION_VALUE   (0x01)

/***************************************************************************
NAME
    vocsClientHandleInternalRegisterForNotification

DESCRIPTION
    Handle a VOCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ message.
*/
void vocsClientHandleInternalRegisterForNotification(GVOCS *const vocs_client,
                                                     bool enable,
                                                     uint16 handle);

#endif
