/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_VCS_CLIENT_NOTIFICATION_H_
#define GATT_VCS_CLIENT_NOTIFICATION_H_

#include "gatt_vcs_client_private.h"

/* VCS notification value */
#define VCS_NOTIFICATION_VALUE   (0x01)

/***************************************************************************
NAME
    vcsClientHandleInternalRegisterForNotification

DESCRIPTION
    Handle a VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ message.
*/
void vcsClientHandleInternalRegisterForNotification(GVCSC *const vcs_client,
                                                    bool enable,
                                                    uint16 handle);

#endif
