/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_common.h"

/*******************************************************************************/
void vocsClientSendVocsClientWriteCfm(GVOCS *const vocs_client,
                                      const status_t status,
                                      GattVocsClientMessageId id)
{
    if (id != GATT_VOCS_CLIENT_MESSAGE_TOP)
    {
        /* We will use GATT_VOCS_CLIENT_OFFSET_STATE_SET_NTF_CFM to create the message
         * because the structure of all the write confirmations is the same,
         * but we will send the right message using the id parameter */
        MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientOffsetStateSetNtfCfm);

        /* Fill in client reference */
        message->srvcHndl = vocs_client->srvcElem->service_handle;

        /* Fill in the status */
        message->status = status;

        /* Send the confirmation message to app task  */
        VocsMessageSend(vocs_client->app_task, id, message);
    }
}

/*******************************************************************************/
void vocsSendReadCccCfm(GVOCS *vcs_client,
                        status_t status,
                        uint16 size_value,
                        const uint8 *value,
                        GattVocsClientMessageId id)
{
    /* We will use GATT_VOCS_CLIENT_READ_OFFSET_STATE_CCC_CFM to create the message
     * because the structure of all the CCC read confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_VOCS_CLIENT_MESSAGE_WITH_LEN(GattVocsClientReadOffsetStateCccCfm, size_value);

    if(value)
    {
        message->value = (uint8 *) CsrPmemZalloc(size_value);
        memmove(message->value, value, size_value);
    }
    else
    {
        message->value = NULL;
    }

    message->sizeValue = size_value;
    message->srvcHdnl = vcs_client->srvcElem->service_handle;
    message->status = status;

    VocsMessageSend(vcs_client->app_task, id, message);
}
