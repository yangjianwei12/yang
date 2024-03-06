/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include "gatt_root_key_client_indication.h"

#include "gatt_root_key_client_state.h"
#include "gatt_root_key_client_write.h"
#include "gatt_root_key_client_discover.h"


/****************************************************************************
Internal functions
****************************************************************************/

/***************************************************************************/
void handleRootKeyIndication(GATT_ROOT_KEY_CLIENT *instance, 
                             const CsrBtGattClientIndicationInd *ind)
{
    /* Indications are only expected on the challenge control point */
    if (    ind->valueLength != (GRKC_KEY_SIZE_128BIT_OCTETS + 1)
        || (ind->valueHandle != instance->handle_challenge_control))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("handleRootKeyIndication. Unexpected handle:%d or size:%d",
                                    ind->valueHandle, ind->valueLength);
        gattRootKeyClientSetState(instance, root_key_client_error);
    }
    else
    {
        MAKE_ROOT_KEY_MESSAGE(ROOT_KEY_CLIENT_INTERNAL_INDICATION);

        message->instance = instance;
        message->opcode = ind->value[0];
        memcpy(&message->value.key, &ind->value[1], sizeof(message->value.key));
        MessageSend(&instance->lib_task, ROOT_KEY_CLIENT_INTERNAL_INDICATION, message);
    }

    GattClientIndicationRspSend(instance->gattId, instance->cid);
}

