/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_TBS_CLIENT_NOTIFICATION_H_
#define GATT_TBS_CLIENT_NOTIFICATION_H_


#include "gatt_telephone_bearer_client_private.h"


#define TELEPHONE_BEARER_NOTIFICATION_VALUE 0x0001



/***************************************************************************
NAME
    tbsHandleInternalRegisterForNotification

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_SET_NOTIFICATION message.
*/
void tbsHandleInternalRegisterForNotification(GTBSC *const tbs_client,
                                                    bool enable,
                                                    uint16 handle);

/***************************************************************************
NAME
    handleTbsNotification

DESCRIPTION
    Handles the internal GATT_MANAGER_NOTIFICATION_IND message.
*/
void handleTbsNotification(const ServiceHandle tbsHandle, const GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND_T *ind);


/***************************************************************************
NAME
    tbsSendNotificationRequest

DESCRIPTION
    Deals with sending a notification request to the server. This may be getting or setting the notification configuration.
*/
void tbsSendNotificationRequest(GTBSC *tbs_client);


#endif