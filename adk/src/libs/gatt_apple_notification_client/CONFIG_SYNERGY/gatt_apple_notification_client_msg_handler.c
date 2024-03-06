/* Copyright (c) 2014 - 2022 Qualcomm Technologies International, Ltd. */

#include "gatt_apple_notification_client_msg_handler.h"
#include "gatt_apple_notification_client_private.h"
#include "gatt_apple_notification_client_discover.h"
#include "gatt_apple_notification_client_notification.h"
#include "gatt_apple_notification_client_external_msg_send.h"
#include "gatt_apple_notification_client_write.h"

static void handleSetNsNotificationMsg(GANCS *ancs, const ANCS_INTERNAL_MSG_SET_NS_NOTIFICATION_T *notif)
{
    ancs->task_pending_cfm = notif->task_pending_cfm;
    ancs->ns_notification_mask = notif->notifications_mask;
    ancsSetNotificationRequest(ancs, notif->notifications_enable, GATT_APPLE_NOTIFICATION_NS);
}

static void handleSetDsNotificationMsg(GANCS *ancs, const ANCS_INTERNAL_MSG_SET_DS_NOTIFICATION_T *notif)
{
    ancs->task_pending_cfm = notif->task_pending_cfm;
    ancsSetNotificationRequest(ancs, notif->notifications_enable, GATT_APPLE_NOTIFICATION_DS);
}

static void handleWriteControlPointCharacteristicMsg(GANCS *ancs, const ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC_T *charact)
{
    ancs->task_pending_cfm = charact->task_pending_cfm;
    ancs->pending_cmd = charact->pending_cmd;

    if(CHECK_VALID_HANDLE(ancs->control_point))
        ancsWriteCharValue(ancs, charact, ancs->control_point);
    else
        gattAncsSendWriteControlPointResponse(ancs, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
}

static void handleGattMsg(GANCS *ancs, MessageId id, Message payload)
{
    UNUSED(id);
    CsrBtGattPrim *primType = (CsrBtGattPrim *)payload;
    switch (*primType)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            handleAncsClientRegisterCfm(ancs, (CsrBtGattRegisterCfm*)payload);
            break;

        case CSR_BT_GATT_UNREGISTER_CFM:
            handleAncsClientUnRegisterCfm(ancs, (CsrBtGattUnregisterCfm*)payload);
            break;
            
        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
            handleAncsNotification(ancs, (const CsrBtGattClientNotificationInd *)payload);
            break;

        case CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM:
            handleAncsClientRegisterServiceCfm(ancs, (const CsrBtGattClientRegisterServiceCfm *)payload);
            break;

        case CSR_BT_GATT_DISCOVER_CHARAC_IND:
            handleAncsDiscoverAllCharacteristicsInd(ancs, (const CsrBtGattDiscoverCharacInd *)payload);
            break;

        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
            handleAncsDiscoverAllCharacteristicsCfm(ancs, (const CsrBtGattDiscoverCharacCfm *)payload);
            break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND:
            handleAncsDiscoverAllCharacteristicDescriptorsInd(ancs, (const CsrBtGattDiscoverCharacDescriptorsInd *)payload);
            break;
            
        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
            handleAncsDiscoverAllCharacteristicDescriptorsCfm(ancs, (const CsrBtGattDiscoverCharacDescriptorsCfm *)payload);
            break;

        case CSR_BT_GATT_WRITE_CFM:
            handleAncsWriteValueResp(ancs, (const CsrBtGattWriteCfm *)payload);
            break;

        default:
            DEBUG_PANIC("handleGattMsg: unhandled 0x%X", *primType);
        break;
    }

    GattFreeUpstreamMessageContents((void *)payload);
}

static void handleInternalAncsMsg(GANCS *ancs, MessageId id, Message payload)
{
    switch (id)
    {
        case ANCS_INTERNAL_MSG_SET_NS_NOTIFICATION:
            handleSetNsNotificationMsg(ancs, (ANCS_INTERNAL_MSG_SET_NS_NOTIFICATION_T *) payload);
        break;

        case ANCS_INTERNAL_MSG_SET_DS_NOTIFICATION:
            handleSetDsNotificationMsg(ancs, (ANCS_INTERNAL_MSG_SET_DS_NOTIFICATION_T *) payload);
        break;

        case ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC:
            handleWriteControlPointCharacteristicMsg(ancs, (ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC_T *) payload);
        break;

        default:
            DEBUG_PANIC("handleInternalAncsMsg: unhandled 0x%X", id);
        break;
    }
}

void appleNotificationClientMsgHandler(Task task, MessageId id, Message payload)
{
    GANCS *ancs = (GANCS *) task;

    if (id == GATT_PRIM)
        handleGattMsg(ancs, id, payload);
    else
        handleInternalAncsMsg(ancs, id, payload);
}
