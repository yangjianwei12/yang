/* Copyright (c) 2014 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_apple_notification_client.h"
#include "gatt_apple_notification_client_private.h"
#include "gatt_apple_notification_client_ready_state.h"
#include <gatt_lib.h>

bool GattAncsDestroy(void *ancs_dynamic)
{
    GANCS* ancs = (GANCS*)ancs_dynamic;

    if (ancs == NULL)
        return FALSE;

    GattUnregisterReqSend(ancs->gattId);

    return TRUE;
}
