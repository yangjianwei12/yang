/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_root_key_server_indicate.h"
#include "gatt_root_key_server_private.h"

#include "gatt_root_key_server_state.h"

#include "gatt_root_key_server_db.h"
#ifdef DISABLE_ROOT_KEY_EXCHANGE
#include "gatt_root_key_server_challenge.h"
#endif


/****************************************************************************/
bool gattRootKeyServerSendChallengeIndication(const GATT_ROOT_KEY_SERVER *instance,
                                              uint32 conn_id, uint8 opcode, const GRKS_KEY_T *key)
{
    bool result = FALSE;

    /* Allocate memory */
    uint8 *indication = (uint8*)CsrPmemAlloc(GRKS_SIZE_CONTROL_WITH_KEY_OCTETS);

    if (instance != NULL)
    {
        result = TRUE;

        indication[0] = opcode;
        memcpy(&indication[1], key, GRKS_KEY_SIZE_128BIT_OCTETS);

        GattIndicationEventReqSend(instance->gattId, conn_id, 
                                   HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT,
                                   GRKS_SIZE_CONTROL_WITH_KEY_OCTETS, indication);
    }

    return result;
}


static void handleRootKeyChallengeIndicationDelivered(GATT_ROOT_KEY_SERVER *instance)
{
    root_key_server_state_t state = gattRootKeyServerGetState(instance);

    switch (state)
    {
        case root_key_server_responded_random:
            gattRootKeyServerSetState(instance, root_key_server_awaiting_hash);
            break;

        case root_key_server_responded_hash:
#ifdef DISABLE_ROOT_KEY_EXCHANGE
            /* in case Root Key exchange is not required, then we can go back to initialized 
               and inform the clients that we are done with the service */
            gattRootKeyServerSetState(instance, root_key_server_initialised);
            gattRootKeySendChallengeInd(instance, gatt_root_key_challenge_status_success);
#else
            gattRootKeyServerSetState(instance, root_key_server_authenticated);
#endif
            break;

        default:
            gattRootKeyServerSetState(instance, root_key_server_error);
            break;
    }

}

#if 0
static void handleRootKeyIndicationDelivered(GATT_ROOT_KEY_SERVER *instance,
                                             uint16 handle)
{
    switch (handle)
    {
        case HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT:
            handleRootKeyChallengeIndicationDelivered(instance);
            break;

        default:
            gattRootKeyServerSetState(instance, root_key_server_error);
            break;
    }
}
#endif

void handleRootKeyIndicationCfm(GATT_ROOT_KEY_SERVER *instance,
                                const CsrBtGattEventSendCfm *payload)
{
   // uint16 handle = payload->attrHandle - instance->dbStartHandle;
    
    switch (payload->resultCode)
    {
        case ATT_RESULT_SUCCESS_SENT:
            /* Indicates that has been sent successfully. Ignore */
            /*GATT_ROOT_KEY_SERVER_DEBUG("handleRootKeyIndicationCfm. Indication handle:%d sent",
                                       payload->handle);*/
            GATT_ROOT_KEY_SERVER_DEBUG("handleRootKeyIndicationCfm.");
            break;

        case CSR_BT_GATT_RESULT_SUCCESS:
            //handleRootKeyIndicationDelivered(instance, payload->handle);
            handleRootKeyChallengeIndicationDelivered(instance);
            break;

        default:
            GATT_ROOT_KEY_SERVER_DEBUG_PANIC();
            break;
    }
}

