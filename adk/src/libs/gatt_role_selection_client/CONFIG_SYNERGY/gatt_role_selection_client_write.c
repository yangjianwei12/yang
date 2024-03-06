/* Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_role_selection_client.h"
#include "gatt_role_selection_client_write.h"
#include "gatt_role_selection_client_notification.h"
#include "gatt_role_selection_client_init.h"
#include "gatt_role_selection_client_state.h"
#include "gatt_role_selection_client_debug.h"


/****************************************************************************/
void roleSelectionWriteStateClientConfigValue(GATT_ROLE_SELECTION_CLIENT *instance)
{
    /* Allocate memory */
    uint16 config_value_len = 2;
    uint8 *config_value = (uint8*)CsrPmemAlloc(config_value_len);
    
    config_value[0] = ROLE_SELECTION_SERVICE_NOTIFICATION_VALUE;
    config_value[1] = 0;

    GattWriteReqSend(instance->gattId, instance->cid,
                     instance->handles.handle_state_config, 0, 
                     config_value_len, config_value);
}


/****************************************************************************/
void roleSelectionWriteFigureOfMeritClientConfigValue(GATT_ROLE_SELECTION_CLIENT *instance)
{
    /* Allocate memory */
    uint16 config_value_len = 2;
    uint8 *config_value = (uint8*)CsrPmemAlloc(config_value_len);
    
    config_value[0] = ROLE_SELECTION_SERVICE_NOTIFICATION_VALUE;
    config_value[1] = 0;

    GattWriteReqSend(instance->gattId, instance->cid,
                     instance->handles.handle_figure_of_merit_config, 0, 
                     config_value_len, config_value);
}


/****************************************************************************/
void handleRoleSelectionWriteValueResp(GATT_ROLE_SELECTION_CLIENT *instance, 
                                       const CsrBtGattWriteCfm *write_cfm)
{
    if (   role_selection_client_waiting_write == gattRoleSelectionClientGetState(instance)
        && instance->handles.handle_role_control == write_cfm->handle)
    {
        gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
        if (CSR_BT_GATT_RESULT_SUCCESS != write_cfm->resultCode)
        {
            GATT_ROLE_SELECTION_CLIENT_DEBUG("handleRoleSelectionWriteValueResp: role_sel handle:%d bad status:0x%04x supplier:0x%04x",
                                             write_cfm->handle, write_cfm->resultCode, write_cfm->resultSupplier);
        }
        gattRoleSelectionSendClientCommandCfmMsg(instance, write_cfm->resultCode);
    }
    else if (   role_selection_client_setting_notification == gattRoleSelectionClientGetState(instance)
             && instance->handles.handle_state_config == write_cfm->handle)
    {
        gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
        if (CSR_BT_GATT_RESULT_SUCCESS != write_cfm->resultCode)
        {
            GATT_ROLE_SELECTION_CLIENT_DEBUG("handleRoleSelectionWriteValueResp: state_cfg handle:%d bad status:0x%04x supplier:0x%04x",
                                             write_cfm->handle, write_cfm->resultCode, write_cfm->resultSupplier);
        }
    }
    else if (   role_selection_client_setting_notification_fom == gattRoleSelectionClientGetState(instance)
             && instance->handles.handle_figure_of_merit_config == write_cfm->handle)
    {
        gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
        if (CSR_BT_GATT_RESULT_SUCCESS != write_cfm->resultCode)
        {
            GATT_ROLE_SELECTION_CLIENT_DEBUG("handleRoleSelectionWriteValueResp: merit_cfg handle:%d bad status:0x%04x supplier:0x%04x",
                                             write_cfm->handle, write_cfm->resultCode, write_cfm->resultSupplier);
        }
    }
}


void gattRoleSelectionSendClientCommandCfmMsg(GATT_ROLE_SELECTION_CLIENT *instance, 
                                              uint16 status)
{
    if (instance->control_response_needed)
    {
        MAKE_ROLE_SELECTION_MESSAGE(GATT_ROLE_SELECTION_CLIENT_COMMAND_CFM);

        message->instance = instance;
        message->result = status;

        MessageSend(instance->app_task, GATT_ROLE_SELECTION_CLIENT_COMMAND_CFM, message);
        
        instance->control_response_needed = FALSE;
    }
}

/*
    We check the state here as we could have a de-qeueued message
    after an error occurs. In that case we still send a 
    confirmation message to the application.
 */
void GattRoleSelectionClientChangePeerRoleImpl(GATT_ROLE_SELECTION_CLIENT *instance,
                                               GattRoleSelectionServiceControlOpCode role,
                                               bool queued)
{
    role_selection_client_state_t state = gattRoleSelectionClientGetState(instance);
    uint8 *state_to_write = (uint8*)CsrPmemAlloc(1);
    state_to_write[0] = (uint8)role;

    switch (state)
    {
        case role_selection_client_uninitialised:
        case role_selection_client_error:
        case role_selection_client_finding_handles:
            DEBUG_LOG("GattRoleSelectionClientChangePeerRoleImpl. Client probably ended. State:%d", state);

            if (queued)
            {
                instance->control_response_needed = TRUE;
                gattRoleSelectionSendClientCommandCfmMsg(instance, CSR_BT_GATT_RESULT_INTERNAL_ERROR);
            }
            break;

        case role_selection_client_initialised:
            gattRoleSelectionClientSetState(instance, role_selection_client_waiting_write);

            GattWriteReqSend(instance->gattId, instance->cid, instance->handles.handle_role_control, 
                             0, 1, state_to_write);
            
            instance->control_response_needed = TRUE;
            break;

        default:
            DEBUG_LOG("GattRoleSelectionClientChangePeerRoleImpl. Unexpected state %d", state);
            Panic();
            break;
    }
}

bool GattRoleSelectionClientChangePeerRole(GATT_ROLE_SELECTION_CLIENT *instance,
                                           GattRoleSelectionServiceControlOpCode role)
{
    role_selection_client_state_t state;

    if (!instance)
    {
        DEBUG_LOG("GattRoleSelectionClientChangePeerRole NO INSTANCE");

        return FALSE;
    }

    state = gattRoleSelectionClientGetState(instance);
    switch (state)
    {
        case role_selection_client_uninitialised:
        case role_selection_client_error:
            /* Dropthrough here. These states represent errors and are 
               handled in the impl function */

        case role_selection_client_initialised:
            GattRoleSelectionClientChangePeerRoleImpl(instance, role, FALSE);
            break;

            /* Although an unlikely scenario if in finding_handles state
               the command will go through. */
        case role_selection_client_finding_handles:
        default:
            /* The client is busy completing something else. Send a message to 
               do as soon as possible. Could send a message in all cases, but 
               delays the process unnecessarily */
            DEBUG_LOG("GattRoleSelectionClientChangePeerRole not initialised. state:%d",state);

            MAKE_ROLE_SELECTION_MESSAGE(ROLE_SELECTION_CLIENT_INTERNAL_CHANGE_ROLE);
            message->role = role;
            MessageSendConditionally(&instance->lib_task, ROLE_SELECTION_CLIENT_INTERNAL_CHANGE_ROLE,
                                     message, &instance->active_procedure);
            break;
    }

    return TRUE;
}

