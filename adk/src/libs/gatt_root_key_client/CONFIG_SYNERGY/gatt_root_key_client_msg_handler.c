/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_root_key_client_private.h"

#include "gatt_root_key_client_msg_handler.h"

#include "gatt_root_key_client_state.h"
#include "gatt_root_key_client_challenge.h"
#include "gatt_root_key_client_discover.h"
#include "gatt_root_key_client_indication.h"
#include "gatt_root_key_client_write.h"
#include "gatt_root_key_client_init.h"

static void rootKeyHandleIndication(GATT_ROOT_KEY_CLIENT *instance, const ROOT_KEY_CLIENT_INTERNAL_INDICATION_T *ind)
{
    root_key_client_state_t state = gattRootKeyClientGetState(instance);

    if (   root_key_client_starting_challenge == state
        && GattRootKeyScOpcodeOutgoingRequest == ind->opcode)
    {
        instance->remote_random = ind->value;

        gattRootKeyGenerateHashB(instance);

        gattRootKeyClientWriteChallenge(instance, GattRootKeyScOpcodeIncomingResponse,
                                        &instance->hashB_out);
        gattRootKeyClientSetState(instance, root_key_client_finishing_challenge);
        return; 
    }
    else if (   root_key_client_finishing_challenge == state
             && GattRootKeyScOpcodeOutgoingResponse == ind->opcode)
    {
        instance->hashA_in = ind->value;

        if (!gattRootKeyCheckHash(instance))
        {
            GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleIndication HASH match failure");
        }
        else
        {
            gattRootKeySendChallengePeerInd(instance, gatt_root_key_challenge_status_success);
            gattRootKeyClientSetState(instance, root_key_client_authenticated);
            return;
        }
        
    }
    else
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleIndication Bad opcode/state. Op:%d Pending:%d",
                                ind->opcode, state);
    }
    gattRootKeyClientSetState(instance, root_key_client_error);
}


/****************************************************************************/
static void handleInternalRootKeyMsg(Task task, MessageId id, Message message)
{
    GATT_ROOT_KEY_CLIENT *instance = (GATT_ROOT_KEY_CLIENT *)task;

    switch (id)
    {
        case ROOT_KEY_CLIENT_INTERNAL_INDICATION:
            rootKeyHandleIndication(instance, (const ROOT_KEY_CLIENT_INTERNAL_INDICATION_T *)message);
            break;

        default:
            GATT_ROOT_KEY_CLIENT_DEBUG("handleInternalRootKeyMsg: Unhandled [0x%x]\n", id);
            break;
    }
}

/****************************************************************************/
void rootKeyClientMsgHandler(Task task, MessageId id, Message message)
{
    GATT_ROOT_KEY_CLIENT *instance = (GATT_ROOT_KEY_CLIENT *)task;

    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyClientMsgHandler: %d [0x%x]\n", id, id);
    
    if (id != GATT_PRIM)
    {
        handleInternalRootKeyMsg(task, id, message);
        return;
    }

    CsrBtGattPrim *primType = (CsrBtGattPrim *)message;

    switch(*primType)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            handleRootKeyClientRegisterCfm(instance, (CsrBtGattRegisterCfm*)message);
            break;
            
        case CSR_BT_GATT_CLIENT_INDICATION_IND:
            handleRootKeyIndication(instance, (const CsrBtGattClientIndicationInd *)message);
            break;

        case CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM:
            rootKeyHandleRegisterServiceCfm(instance, (const CsrBtGattClientRegisterServiceCfm *)message);
            break;

        case CSR_BT_GATT_DISCOVER_SERVICES_IND:
            rootKeyHandleDiscoverPrimaryServiceInd(instance, (const CsrBtGattDiscoverServicesInd *)message);
            break;

        case CSR_BT_GATT_DISCOVER_SERVICES_CFM:
            rootKeyHandleDiscoverPrimaryServiceCfm(instance, (const CsrBtGattDiscoverServicesCfm *)message);
            break;

        case CSR_BT_GATT_DISCOVER_CHARAC_IND:
            rootKeyHandleDiscoverAllCharacteristicsInd(instance, (const CsrBtGattDiscoverCharacInd *)message);
            break;
            
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
            rootKeyHandleDiscoverAllCharacteristicsCfm(instance, (const CsrBtGattDiscoverCharacCfm *)message);
            break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND:
            rootKeyHandleDiscoverAllCharacteristicDescriptorsInd(instance, (const CsrBtGattDiscoverCharacDescriptorsInd *)message);
            break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
            rootKeyHandleDiscoverAllCharacteristicDescriptorsCfm(instance, (const CsrBtGattDiscoverCharacDescriptorsCfm *)message);
            break;

        case CSR_BT_GATT_WRITE_CFM:
            handleRootKeyWriteValueResp(instance, (const CsrBtGattWriteCfm *)message);
            break;

        default:
            GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyClientMsgHandler: Unhandled message [0x%x]\n", (*primType));
            break;
    }

    GattFreeUpstreamMessageContents((void *)message);
}

