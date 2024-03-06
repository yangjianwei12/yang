/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_MCS_CLIENT_NOTIFICATION_H_
#define GATT_MCS_CLIENT_NOTIFICATION_H_

#include "gatt_mcs_client_private.h"

/* MCS notification value */
#define MCS_NOTIFICATION_VALUE   (0x01)

/***************************************************************************
NAME
    mcsClientHandleInternalRegisterForNotification

DESCRIPTION
    Handle a MCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ message.
*/
void mcsClientHandleInternalRegisterForNotification(GMCSC *gattMcsClient,
                                                    MediaPlayerAttributeMask characList,
                                                    uint32 notifValue);

void handleMcsClientNotification(GMCSC *mcsClient, uint16 handle, uint16 valueLength, uint8 *value);

void mcsClientNotifCfm(GMCSC *const mcsClient,
                       GattMcsClientStatus status,
                       GattMcsClientMessageId id);

void mcsClientNotifInd(GMCSC *const mcsClient,
                       MediaPlayerAttribute charac,
                       status_t status,
                       GattMcsClientMessageId id);

#endif
