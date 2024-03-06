/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <stdio.h>

#include <util.h>

#include "gatt_role_selection_client_init.h"
#include "gatt_role_selection_client_discover.h"
#include "gatt_role_selection_client_state.h"
#include "gatt_role_selection_client_debug.h"

#include <logging.h>

#include "gatt_role_selection_client_msg_handler.h"

LOGGING_PRESERVE_MESSAGE_TYPE(role_selection_internal_msg_t)
LOGGING_PRESERVE_MESSAGE_TYPE(gatt_role_selection_client_message_id_t)

/******************************************************************************/
void gattRoleSelectionInitSendInitCfm(GATT_ROLE_SELECTION_CLIENT *instance,
                                      gatt_role_selection_client_status_t status)
{
    if (instance->init_response_needed)
    {
        MAKE_ROLE_SELECTION_MESSAGE(GATT_ROLE_SELECTION_CLIENT_INIT_CFM);
        message->instance = instance;
        message->status = status;

        MessageSend(instance->app_task, GATT_ROLE_SELECTION_CLIENT_INIT_CFM, message);

        instance->init_response_needed = FALSE;
    }
}

/****************************************************************************/
bool GattRoleSelectionClientDestroy(GATT_ROLE_SELECTION_CLIENT *instance)
{
    PanicNull(instance);

    instance->active_procedure = FALSE;
    instance->state = role_selection_client_uninitialised;

    /* UnRegister with the GATT  */
    GattUnregisterReqSend(instance->gattId);

    return TRUE;
}

/****************************************************************************/
bool GattRoleSelectionClientInit(GATT_ROLE_SELECTION_CLIENT *instance, 
                                 Task app_task,
                                 gatt_role_selection_handles_t *cached_handles,
                                 uint32 cid,
                                 typed_bdaddr *peer_addr)
{
    /* Check parameters */
    if ((app_task == NULL) || (instance == NULL))
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("GattRoleSelectionClientInit: Invalid parameters");
        Panic();
        return FALSE;
    }

    /* Set memory contents to all zeros */
    memset(instance, 0, sizeof(GATT_ROLE_SELECTION_CLIENT));

    instance->lib_task.handler = roleSelectionClientMsgHandler;
    instance->app_task = app_task;

    instance->peer_state = GrssMirrorStateUnknown;

    instance->cid = cid;

    instance->peer_addr = *peer_addr;

    if(cached_handles)
    {
        instance->handles = *cached_handles;
        instance->handles_cached = TRUE;
    }

    GattRegisterReqSend(&(instance->lib_task), 1234);

    return TRUE;
}

void handleRoleSelectionClientRegisterCfm(GATT_ROLE_SELECTION_CLIENT *instance,
                                          const CsrBtGattRegisterCfm *cfm)
{
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        CsrBtTypedAddr address;
        instance->gattId = cfm->gattId;

        GATT_ROLE_SELECTION_CLIENT_DEBUG("GattRoleSelectionClientInit Initialised");

        instance->init_response_needed = TRUE;

        if(instance->handles_cached)
        {
            GattClientUtilFindAddrByConnId(instance->cid, &address);

            DEBUG_LOG("handleRoleSelectionClientRegisterCfm  lap 0x%04x uap 0x%01x nap 0x%02x start handle:%d end_handle:%d" , address.addr.lap, address.addr.uap, address.addr.nap, instance->handles.handle_service_start, instance->handles.handle_service_end);
            GattClientRegisterServiceReqSend(instance->gattId, instance->handles.handle_service_start, instance->handles.handle_service_end, address);
        }
        else
        {
            /* Discover primary service, all characteristics and descriptors after successful registration */
            roleSelectionDiscoverPrimaryService(instance);
            gattRoleSelectionClientSetState(instance, role_selection_client_finding_service_handle);
            
        }
    }
    else
    {
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
    }
}

