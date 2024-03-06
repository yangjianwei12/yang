/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>
#include <logging.h>

#include "gatt_role_selection_server.h"
#include "gatt_role_selection_server_debug.h"
#include "gatt_role_selection_server_msg_handler.h"
#include "gatt_role_selection_server_db.h"
#include "gatt_role_selection_server_private.h"
#include "gatt_role_selection_server_init.h"

#include <panic.h>

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(gatt_role_selection_server_message_id_t)
LOGGING_PRESERVE_MESSAGE_TYPE(role_selection_server_internal_msg_t)

/***************************************************************************
NAME
    sendRoleSelectionInitRsp

DESCRIPTION
    Sends response to the application
*/

static void sendRoleSelectionInitRsp(const GATT_ROLE_SELECTION_SERVER *instance, 
                                     gatt_role_selection_server_init_status_t status_code)
{
    MAKE_ROLE_SELECTION_MESSAGE(GATT_ROLE_SELECTION_SERVER_INIT_IND);

    message->instance = instance;
    message->status = status_code;

    MessageSend(instance->app_task, GATT_ROLE_SELECTION_SERVER_INIT_IND, message);

}


/****************************************************************************/
bool GattRoleSelectionServerInit(GATT_ROLE_SELECTION_SERVER *instance, 
                                 Task app_task,
                                 uint16 start_handle,
                                 uint16 end_handle,
                                 bool enable_fom_notification)
{
    if ((app_task == NULL) || (instance == NULL))
    {
        GATT_ROLE_SELECTION_SERVER_DEBUG("GattRoleSelectionServerInit: Invalid Initialisation parameters");
        Panic();
        return FALSE;
    }

    memset(instance, 0, sizeof(*instance));

    /* Set up library handler for external messages */
    instance->lib_task.handler = roleSelectionServerMsgHandler;

    /* Store the Task function parameter.
       All library messages need to be sent here */
    instance->app_task = app_task;

    instance->initialised = TRUE;

    instance->end_handle  = end_handle;
    instance->start_handle = start_handle;

    if(enable_fom_notification)
    { /* Setting this to 0x01 turns on notifications for the figure of merit */
        instance->merit_client_config = 0x01;
    }

    GattRegisterReqSend(&(instance->lib_task), 1234);
    
    return TRUE;
}

void handleRoleSelectionServiceRegisterCfm(GATT_ROLE_SELECTION_SERVER *instance,
                                           const CsrBtGattRegisterCfm *cfm)
{
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        instance->gattId = cfm->gattId;
        GattFlatDbRegisterHandleRangeReqSend(instance->gattId, instance->start_handle, instance->end_handle);
    }
    else
    {
        sendRoleSelectionInitRsp(instance, gatt_role_selection_server_init_failed);
    }
}

void handleRoleSelectionServiceRegisterHandleRangeCfm(GATT_ROLE_SELECTION_SERVER *instance,
                                                      const CsrBtGattFlatDbRegisterHandleRangeCfm *cfm)
{
    gatt_role_selection_server_init_status_t status = gatt_role_selection_server_init_failed;
    
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        status = gatt_role_selection_server_init_success;
    }

    sendRoleSelectionInitRsp(instance, status);
}
