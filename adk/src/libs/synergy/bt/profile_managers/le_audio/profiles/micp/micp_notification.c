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
#include "micp_notification.h"
#include "micp_common.h"

/***************************************************************************/
static void micpSendMicsSetNtfCfm(MICP *micp_inst,
                                status_t status,
                                MicpMessageId id)
{
    MAKE_MICP_MESSAGE(MicpNtfCfm);

    message->id = id;
    message->prflHndl = micp_inst->micp_srvc_hdl;
    message->status = status;

    MicpMessageSend(micp_inst->app_task, message);
}

/***************************************************************************/
void micpHandleMicsNtfCfm(MICP *micp_inst,
                                      const GattMicsClientNtfCfm *msg)
{
    micp_inst->pending_op = MICP_PENDING_OP_NONE;
    micpSendMicsSetNtfCfm(micp_inst,
                        msg->status,
                        MICP_NTF_CFM);
}

/****************************************************************************/
void MicpRegisterForNotificationReq(MicpProfileHandle profileHandle,
                                              bool notificationsEnable)
{
    MICP *micp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (micp_inst)
    {
        GattMicsClientRegisterForNotificationReq(micp_inst->mics_srvc_hdl,
                                                        notificationsEnable);
    }
    else
    {
        MICP_DEBUG("Invalid profile_handle\n");
    }
}

