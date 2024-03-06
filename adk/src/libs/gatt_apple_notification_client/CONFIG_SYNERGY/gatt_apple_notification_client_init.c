/* Copyright (c) 2014 - 2022 Qualcomm Technologies International, Ltd. */

#include "gatt_apple_notification_client.h"
#include "gatt_apple_notification_client_msg_handler.h"
#include "gatt_apple_notification_client_private.h"
#include <gatt_lib.h>
#include <string.h>

static void initAncsClient(GANCS *ancs, Task app_task, uint32 cid, uint16 start_handle, uint16 end_handle)
{
    memset(ancs, 0, sizeof(GANCS));
    ancs->lib_task.handler = appleNotificationClientMsgHandler;
    ancs->app_task = app_task;
    ancs->cid = cid;
    ancs->service_start_handle = start_handle;
    ancs->service_end_handle = end_handle;
    ancs->notification_source = GATT_ANCS_INVALID_HANDLE;
    ancs->data_source = GATT_ANCS_INVALID_HANDLE;
    ancs->control_point = GATT_ANCS_INVALID_HANDLE;
    ancs->ns_ccd = GATT_ANCS_INVALID_HANDLE;
    ancs->ds_ccd = GATT_ANCS_INVALID_HANDLE;
}

bool GattAncsInit(Task app_task, uint32 cid, uint16 start_handle, uint16 end_handle, void *ancs_dynamic, void* ancs_constant)
{
    UNUSED(ancs_constant);
    GANCS* ancs = (GANCS*)ancs_dynamic;

    if ((app_task == NULL) || (ancs == NULL))
        return FALSE;

    initAncsClient(ancs, app_task, cid, start_handle, end_handle);

    GattRegisterReqSend(&(ancs->lib_task), 1234);

    ancs->pending_cmd = ancs_pending_discover_all_characteristics;

    DEBUG_LOG_INFO("GattAncsInit: initialised instance %p with cid 0x%08X", (void *) ancs, ancs->cid);

    return TRUE;
}
