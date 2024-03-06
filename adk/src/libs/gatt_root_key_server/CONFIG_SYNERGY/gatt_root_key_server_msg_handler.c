/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_root_key_server_private.h"

#include "gatt_root_key_server_msg_handler.h"
#include "gatt_root_key_server_challenge.h"
#include "gatt_root_key_server_indicate.h"

#include "gatt_root_key_server_access.h"
#include "gatt_root_key_server_init.h"


/****************************************************************************/
static void handleInternalRootKeyServerMsg(Task task, MessageId id, Message message)
{
    UNUSED(task);
    switch (id)
    {
        case ROOT_KEY_SERVER_INTERNAL_CHALLENGE_WRITE:
            handleInternalChallengeWrite((const ROOT_KEY_SERVER_INTERNAL_CHALLENGE_WRITE_T *)message);
            break;

        case ROOT_KEY_SERVER_INTERNAL_KEYS_WRITE:
            handleInternalKeysWrite((const ROOT_KEY_SERVER_INTERNAL_KEYS_WRITE_T *)message);
            break;

        case ROOT_KEY_SERVER_INTERNAL_KEYS_COMMIT:
            handleInternalKeysCommit((const ROOT_KEY_SERVER_INTERNAL_KEYS_COMMIT_T *)message);
            break;

        default:
            GATT_ROOT_KEY_SERVER_DEBUG("handleInternalRootKeyMsg: Unhandled [0x%x]\n", id);
            break;
    }
}


/****************************************************************************/
void rootKeyServerMsgHandler(Task task, MessageId id, Message message)
{
    GATT_ROOT_KEY_SERVER *instance= (GATT_ROOT_KEY_SERVER*)task;

    GATT_ROOT_KEY_SERVER_DEBUG("rootKeyServerMsgHandler: %d (x%x)",id,id);

    if (id != GATT_PRIM)
    {
        handleInternalRootKeyServerMsg(task, id, message);
        return;
    }
    
    CsrBtGattPrim *primType = (CsrBtGattPrim *)message;

    switch(*primType)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            handleRootKeyRegisterCfm(instance, (CsrBtGattRegisterCfm*)message);
            break;
        
        case CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM:
            handleRootKeyRegisterHandleRangeCfm(instance, (CsrBtGattFlatDbRegisterHandleRangeCfm*)message);
            break;
        
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
            /* Write access to characteristic */
            handleRootKeyWriteAccess(instance, (CsrBtGattDbAccessWriteInd *)message);
            break;
            
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
            /* Read access to characteristic */
            handleRootKeyReadAccess(instance, (CsrBtGattDbAccessReadInd *)message);
            break;

        case CSR_BT_GATT_EVENT_SEND_CFM:
            handleRootKeyIndicationCfm(instance, 
                                       (const CsrBtGattEventSendCfm *)message);
            break;

        default:
            /* Unrecognised message */
            GATT_ROOT_KEY_SERVER_DEBUG("rootKeyServerMsgHandler: Msg 0x%x not handled", *primType);
            /*! \todo This panic may be a step too far */
            GATT_ROOT_KEY_SERVER_DEBUG_PANIC();
            break;
    }

    GattFreeUpstreamMessageContents((void *)message);
}

