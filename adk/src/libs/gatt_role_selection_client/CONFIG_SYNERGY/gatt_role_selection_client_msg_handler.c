/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_role_selection_client_private.h"

#include "gatt_role_selection_client_msg_handler.h"
#include "gatt_role_selection_client_read.h"
#include "gatt_role_selection_client_write.h"
#include "gatt_role_selection_client_discover.h"
#include "gatt_role_selection_client_notification.h"
#include "gatt_role_selection_client_debug.h"
#include "gatt_role_selection_client_init.h"

static void handleRoleSelectionClientInternalChangeRole(Task task,
                                    const ROLE_SELECTION_CLIENT_INTERNAL_CHANGE_ROLE_T *role)
{
    GATT_ROLE_SELECTION_CLIENT *instance = (GATT_ROLE_SELECTION_CLIENT *)task;

    GATT_ROLE_SELECTION_CLIENT_DEBUG("handleRoleSelectionClientInternalChangeRole");

    GattRoleSelectionClientChangePeerRoleImpl(instance, role->role, TRUE);
}


/****************************************************************************/
static void handleInternalRoleSelectionMsg(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case ROLE_SELECTION_CLIENT_INTERNAL_CHANGE_ROLE:
            handleRoleSelectionClientInternalChangeRole(task, 
                                    (const ROLE_SELECTION_CLIENT_INTERNAL_CHANGE_ROLE_T*)message);
            break;

        default:
            GATT_ROLE_SELECTION_CLIENT_DEBUG("handleInternalRoleSelectionMsg: Unhandled [0x%x]\n", id);
            break;
    }
}

/****************************************************************************/
void roleSelectionClientMsgHandler(Task task, MessageId id, Message payload)
{
    GATT_ROLE_SELECTION_CLIENT *instance = (GATT_ROLE_SELECTION_CLIENT *)task;

    GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionClientMsgHandler: %d [0x%x]\n", id, id);
    
    if (id != GATT_PRIM)
    {
        handleInternalRoleSelectionMsg(task, id, payload);
        return;
    }

    CsrBtGattPrim *primType = (CsrBtGattPrim *)payload;
    
    switch (*primType)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            handleRoleSelectionClientRegisterCfm(instance, (CsrBtGattRegisterCfm*)payload);
            break;
            
        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
            handleRoleSelectionNotification(instance, 
                                            (const CsrBtGattClientNotificationInd *)payload);
            break;

        case CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM:
            roleSelectionHandleRegisterServiceCfm(instance, (const CsrBtGattClientRegisterServiceCfm *)payload);
            break;

        case CSR_BT_GATT_DISCOVER_SERVICES_IND:
            roleSelectionHandleDiscoverPrimaryServiceInd(instance, (const CsrBtGattDiscoverServicesInd *)payload);
            break;

        case CSR_BT_GATT_DISCOVER_SERVICES_CFM:
            roleSelectionHandleDiscoverPrimaryServiceCfm(instance, (const CsrBtGattDiscoverServicesCfm *)payload);
            break;

        case CSR_BT_GATT_DISCOVER_CHARAC_IND:
            roleSelectionHandleDiscoverAllCharacteristicsInd(instance, (const CsrBtGattDiscoverCharacInd *)payload);
            break;

        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
            roleSelectionHandleDiscoverAllCharacteristicsCfm(instance, (const CsrBtGattDiscoverCharacCfm *)payload);
            break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND:
            roleSelectionHandleDiscoverAllCharacteristicDescriptorsInd(instance, (const CsrBtGattDiscoverCharacDescriptorsInd *)payload);
            break;
            
        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
            roleSelectionHandleDiscoverAllCharacteristicDescriptorsCfm(instance, (const CsrBtGattDiscoverCharacDescriptorsCfm *)payload);
            break;

        case CSR_BT_GATT_READ_CFM:
            handleRoleSelectionReadValueResp(instance, 
                                             (const CsrBtGattReadCfm *)payload);
            break;

        case CSR_BT_GATT_WRITE_CFM:
            handleRoleSelectionWriteValueResp(instance, 
                                              (const CsrBtGattWriteCfm *)payload);
            break;

        default:
            GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionClientMsgHandler: Unhandled message [0x%x]\n", (*primType));
            break;
    }

    GattFreeUpstreamMessageContents((void *)payload);
}

