/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>
#include <logging.h>

#include "gatt_root_key_server_private.h"

#include "gatt_root_key_server_state.h"
#include "gatt_root_key_server_msg_handler.h"
#include "gatt_root_key_server_db.h"
#include "gatt_root_key_server_init.h"

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(gatt_root_key_server_message_id_t)
LOGGING_PRESERVE_MESSAGE_TYPE(root_key_server_internal_msg_t)

/***************************************************************************
NAME
    sendRootKeyServerInitRsp

DESCRIPTION
    Sends response to the application
*/

static void sendRootKeyServerInitRsp(const GATT_ROOT_KEY_SERVER *instance, 
                                     gatt_root_key_server_init_status_t status_code)
{
    MAKE_ROOT_KEY_MESSAGE(GATT_ROOT_KEY_SERVER_INIT_IND);

    message->instance = instance;
    message->status = status_code;

    MessageSend(instance->app_task, GATT_ROOT_KEY_SERVER_INIT_IND, message);

}

/****************************************************************************/

bool GattRootKeyServerInit(GATT_ROOT_KEY_SERVER *instance, 
                           Task app_task,
                           const gatt_root_key_server_init_params_t *init_params,
                           uint16 start_handle,
                           uint16 end_handle)
{
    if ((app_task == NULL) || (instance == NULL) || (init_params == NULL))
    {
        GATT_ROOT_KEY_SERVER_DEBUG("GattRootKeyServerInit: Invalid Initialisation parameters");
        Panic();
        return FALSE;
    }

    memset(instance, 0, sizeof(*instance));

    /* Set up library handler for external messages */
    instance->lib_task.handler = rootKeyServerMsgHandler;

    /* Store the Task function parameter.
       All library messages need to be sent here */
    instance->app_task = app_task;

    instance->features = init_params->features;

    instance->status = 0;

    /*! \todo Can the initial mirror client config sensibly come in init_params
                And can it sensibly be stored (for this server use case) */
    instance->mirror_client_config = 0;
    
    instance->commit_cid = INVALID_COMMIT_CID;

    instance->end_handle  = end_handle;
    instance->start_handle = start_handle;

    GattRegisterReqSend(&(instance->lib_task), 1234);

    return TRUE;
}

void handleRootKeyRegisterCfm(GATT_ROOT_KEY_SERVER *instance,
                              const CsrBtGattRegisterCfm *cfm)
{
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        instance->gattId = cfm->gattId;
        GattFlatDbRegisterHandleRangeReqSend(instance->gattId, instance->start_handle, instance->end_handle);
    }
    else
    {
        sendRootKeyServerInitRsp(instance, gatt_root_key_server_init_failed);
    }
}

void handleRootKeyRegisterHandleRangeCfm(GATT_ROOT_KEY_SERVER *instance,
                                         const CsrBtGattFlatDbRegisterHandleRangeCfm *cfm)
{
    gatt_root_key_server_init_status_t status = gatt_root_key_server_init_failed;
    
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        status = gatt_root_key_server_init_success;
        gattRootKeyServerSetState(instance, root_key_server_idle);
    }

    sendRootKeyServerInitRsp(instance, status);
}

