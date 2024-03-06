/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */


#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_common.h"

/*******************************************************************************/
void vocsClientSendVocsClientWriteCfm(GVOCS *const vocs_client,
                                      const gatt_status_t status,
                                      GattVocsClientMessageId id)
{
    if (id>= GATT_VOCS_CLIENT_INIT_CFM && id <= GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_IND)
    {
        /* We will use GATT_VOCS_CLIENT_OFFSET_STATE_SET_NTF_CFM to create the message
         * because the structure of all the write confirmations is the same,
         * but we will send the right message using the id parameter */
        MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientOffsetStateSetNtfCfm);

        /* Fill in client reference */
        message->srvcHndl = vocs_client->srvc_hndl;

        /* Fill in the status */
        message->status = status;

        /* Send the confirmation message to app task  */
        MessageSend(vocs_client->app_task, id, message);
    }
}

/*******************************************************************************/
void vocsSendReadCccCfm(GVOCS *vcs_client,
                        gatt_status_t status,
                        uint16 size_value,
                        const uint8 *value,
                        GattVocsClientMessageId id)
{
    /* We will use GATT_VOCS_CLIENT_READ_OFFSET_STATE_CCC_CFM to create the message
     * because the structure of all the CCC read confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_VOCS_CLIENT_MESSAGE_WITH_LEN(GattVocsClientReadOffsetStateCccCfm, size_value);

    message->srvcHdnl = vcs_client->srvc_hndl;
    message->status = status;
    message->sizeValue = size_value;

    memmove(message->value, value, size_value);

    MessageSend(vcs_client->app_task, id, message);
}
