/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#include "gatt_mics_client.h"
#include "gatt_aics_client.h"

#include "micp.h"
#include "micp_debug.h"
#include "micp_private.h"
#include "micp_common.h"
#include "micp_read.h"
#include "micp_write.h"
#include "micp_init.h"

/***************************************************************************/
static void micpMicsSendReadCccCfm(MICP *micp_inst,
                                 status_t status,
                                 uint16 size_value,
                                 const uint8 *value,
                                 GattMicsClientMessageId id)
{
    /* We will use MICP_READ_MUTE_VALUE_CCC_CFM to create the message
     * because the structure of all the CCC read confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_MICP_MESSAGE_WITH_LEN(MicpReadMuteValueCccCfm, size_value);

    message->id = id;
    message->prflHndl = micp_inst->micp_srvc_hdl;
    message->status = status;
    message->sizeValue = size_value;

    CsrMemMove(message->value, value, size_value);

    MicpMessageSend(micp_inst->app_task, message);
}

/***************************************************************************/
static void micpSendReadMuteValueCfm(MICP *micp_inst,
                                      status_t status,
                                      uint8 muteValue)
{
    MAKE_MICP_MESSAGE(MicpReadMuteValueCfm);

    message->id = MICP_READ_MUTE_VALUE_CFM;
    message->prflHndl = micp_inst->micp_srvc_hdl;
    message->status = status;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->muteValue = muteValue;
    }
    else
    {
        message->muteValue = 0;
    }

    MicpMessageSend(micp_inst->app_task, message);
}

/****************************************************************************/
void MicpReadMuteValueCccReq(MicpProfileHandle profileHandle)
{
    MICP *micp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (micp_inst)
    {
        GattMicsClientReadMuteValueCccReq(micp_inst->mics_srvc_hdl);
    }
    else
    {
        MICP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void micpHandleMicsReadMuteValueCccCfm(MICP *micp_inst,
                                       const GattMicsClientReadMuteValueCccCfm *msg)
{
    micpMicsSendReadCccCfm(micp_inst,
                         msg->status,
                         msg->sizeValue,
                         msg->value,
                         MICP_READ_MUTE_VALUE_CCC_CFM);
}

/****************************************************************************/
void micpHandleMicsReadMuteValueCfm(MICP *micp_inst,
                                    const GattMicsClientReadMuteValueCfm *msg)
{
    micpSendReadMuteValueCfm(micp_inst,
                              msg->status,
                              msg->muteValue);
}

/*******************************************************************************/
void MicpReadMuteValueReq(MicpProfileHandle profileHandle)
{
    MICP *micp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (micp_inst)
    {
        GattMicsClientReadMuteValueReq(micp_inst->mics_srvc_hdl);
    }
    else
    {
        MICP_DEBUG("Invalid profile_handle\n");
    }
}

