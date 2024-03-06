/* Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_aics_client.h"
#include "gatt_aics_client_private.h"
#include "gatt_aics_client_common.h"
#include "gatt_aics_client_debug.h"

#include "csr_bt_gatt_lib.h"

/*******************************************************************************/
void aicsClientSendAicsClientWriteCfm(GAICS *const aics_client,
                                      const status_t status,
                                      GattAicsClientMessageId id)
{
    /* We will use GATT_AICS_CLIENT_INPUT_STATE_SET_NTF_CFM to create the message
     * because the structure of all the write confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientInputStateSetNtfCfm);

    /* Fill in client reference */
    message->srvcHndl = aics_client->srvcElem->service_handle;

    /* Fill in the status */
    message->status = status;

    /* Send the confirmation message to app task  */
    AicsMessageSend(aics_client->app_task, id, message);
}

/*******************************************************************************/
void aicsSendReadCccCfm(GAICS *vcs_client,
                        status_t status,
                        uint16 size_value,
                        const uint8 *value,
                        GattAicsClientMessageId id)
{
    /* We will use GATT_AICS_CLIENT_READ_INPUT_STATE_CCC_CFM to create the message
     * because the structure of all the CCC read confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_AICS_CLIENT_MESSAGE_WITH_LEN(GattAicsClientReadInputStateCccCfm, size_value);
    memset(message, 0, sizeof(GattAicsClientReadInputStateCccCfm) + (size_value * sizeof(uint8)));

    if(value)
    {
        message->value = (uint8 *) CsrPmemZalloc(size_value);
        memmove(message->value, value, size_value);
    }
    else
    {
        message->value = NULL;
    }

    message->srvcHndl = vcs_client->srvcElem->service_handle;
    message->status = status;
    message->sizeValue = size_value;

    AicsMessageSend(vcs_client->app_task, id, message);
}

/*******************************************************************************/
void gattAicsClientError(void)
{
    GATT_AICS_CLIENT_ERROR("Invalid AICS Client instance!\n");
}

