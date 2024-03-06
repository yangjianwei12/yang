/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_lib.h>

#include "gatt_role_selection_server.h"
#include "gatt_role_selection_server_msg_handler.h"
#include "gatt_role_selection_server_debug.h"
#include "gatt_role_selection_server_access.h"
#include "gatt_role_selection_server_notify.h"
#include "gatt_role_selection_server_init.h"

#include <panic.h>

/****************************************************************************/
static void handleInternalRoleSelectionServerMsg(Task task, MessageId id, Message message)
{
    GATT_ROLE_SELECTION_SERVER *instance= (GATT_ROLE_SELECTION_SERVER*)task;
    switch (id)
    {
        case ROLE_SELECTION_SERVER_INTERNAL_STATE_UPDATED:
            handleRoleSelectionServerStateChanged(instance, (const ROLE_SELECTION_SERVER_INTERNAL_STATE_UPDATED_T *)message);
            break;

        case ROLE_SELECTION_SERVER_INTERNAL_FIGURE_UPDATED:
            handleRoleSelectionServerFigureChanged(instance, (const ROLE_SELECTION_SERVER_INTERNAL_FIGURE_UPDATED_T *)message);
            break;

        default:
            GATT_ROLE_SELECTION_SERVER_DEBUG("handleInternalRoleSelectionServerMsg: Unhandled [0x%x]\n", id);
            break;
    }
}


/****************************************************************************/
void roleSelectionServerMsgHandler(Task task, MessageId id, Message message)
{
    GATT_ROLE_SELECTION_SERVER *instance= (GATT_ROLE_SELECTION_SERVER*)task;

    GATT_ROLE_SELECTION_SERVER_DEBUG("roleSelectionServerMsgHandler: %d (x%x)",id,id);

    if (id != GATT_PRIM)
    {
        handleInternalRoleSelectionServerMsg(task, id, message);
        return;
    }

    CsrBtGattPrim *primType = (CsrBtGattPrim *)message;

    switch (*primType)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            handleRoleSelectionServiceRegisterCfm(instance, (CsrBtGattRegisterCfm*)message);
            break;
        
        case CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM:
            handleRoleSelectionServiceRegisterHandleRangeCfm(instance, (CsrBtGattFlatDbRegisterHandleRangeCfm*)message);
            break;
            
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
            handleRoleSelectionServiceWriteAccess(instance, (CsrBtGattDbAccessWriteInd *)message);
            break;

        case CSR_BT_GATT_DB_ACCESS_READ_IND:
            handleRoleSelectionServiceReadAccess(instance, (CsrBtGattDbAccessReadInd *)message);
        break;

        case CSR_BT_GATT_EVENT_SEND_CFM:
            handleRoleSelectionNotificationCfm(instance, (const CsrBtGattEventSendCfm *)message);
            break;

            /* UNKNOWN messages */
        default:
            GATT_ROLE_SELECTION_SERVER_DEBUG("roleSelectionServerMsgHandler: Msg 0x%x not handled", id);
            /*! \todo This panic may be a step too far */
            GATT_ROLE_SELECTION_SERVER_DEBUG_PANIC();
            break;
    }

    GattFreeUpstreamMessageContents((void *)message);
}

