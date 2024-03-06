/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include "gatt_root_key_client_private.h"

#include "gatt_root_key_client_msg_handler.h"


/****************************************************************************/
bool GattRootKeyClientDestroy(GATT_ROOT_KEY_CLIENT *instance)
{
    bool result = TRUE;
    MessageId id;

    /* Check parameters */
    if (instance == NULL)
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientDestroy: Invalid parameters");
        Panic();
        return FALSE;
    }

    /* UnRegister with the GATT  */
    GattUnregisterReqSend(instance->gattId);
    
    /* Clear pending messages */
    MessageFlushTask(&instance->lib_task);
    for (id = GATT_ROOT_KEY_CLIENT_MESSAGE_BASE; id < GATT_ROOT_KEY_CLIENT_MESSAGE_TOP; id++)
    {
        MessageCancelAll(instance->app_task, id);
    }

    return result;
}
