/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>
#include <logging.h>

#include <util.h>

#include "gatt_root_key_client_init.h"

#include "gatt_root_key_client_state.h"
#include "gatt_root_key_client_discover.h"
#include "gatt_root_key_client_msg_handler.h"

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(gatt_root_key_client_message_id_t)
LOGGING_PRESERVE_MESSAGE_TYPE(root_key_internal_msg_t)

/******************************************************************************/
void gattRootKeyInitSendInitCfm(GATT_ROOT_KEY_CLIENT *instance, gatt_root_key_client_status_t status)
{
    MAKE_ROOT_KEY_MESSAGE(GATT_ROOT_KEY_CLIENT_INIT_CFM);
    message->instance = instance;
    message->status = status;

    MessageSend(instance->app_task, GATT_ROOT_KEY_CLIENT_INIT_CFM, message);

    instance->init_response_needed = FALSE;
}


/****************************************************************************/
bool GattRootKeyClientInit(GATT_ROOT_KEY_CLIENT *instance, 
                           Task app_task,
                           uint32 cid,
                           typed_bdaddr *peer_addr)
{
    /* Check parameters */
    if ((app_task == NULL) || (instance == NULL))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientInit: Invalid parameters");
        Panic();
        return FALSE;
    }

    /* Set memory contents to all zeros */
    memset(instance, 0, sizeof(GATT_ROOT_KEY_CLIENT));

    /* Set up library handler for external messages */
    instance->lib_task.handler = rootKeyClientMsgHandler;

    /* Store the Task function parameter.
       All library messages need to be sent here */
    instance->app_task = app_task;

    instance->cid = cid;
    instance->peer_addr = *peer_addr;
    GattRegisterReqSend(&(instance->lib_task), 1234);

    return TRUE;
}

void handleRootKeyClientRegisterCfm(GATT_ROOT_KEY_CLIENT *instance,
                                    const CsrBtGattRegisterCfm *cfm)
{
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        instance->gattId = cfm->gattId;

        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientInit Initialised");

        /* Discover primary service, all characteristics and descriptors after successful registration */
        rootKeyDiscoverPrimaryService(instance);
        gattRootKeyClientSetState(instance, root_key_client_finding_service_handle);
        instance->init_response_needed = TRUE;
    }
    else
    {
        gattRootKeyClientSetState(instance, root_key_client_error);
    }
}


