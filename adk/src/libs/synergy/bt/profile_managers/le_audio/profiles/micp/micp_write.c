/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/


#include "gatt_mics_client.h"

#include "micp.h"
#include "micp_write.h"
#include "micp_debug.h"
#include "micp_private.h"
#include "micp_common.h"
#include "micp_init.h"

/***************************************************************************/
static void micpSendMuteControlPointOpCfm(MICP *micp_inst,
                                           status_t status,
                                           MicpMessageId id)
{
    if (id != MICP_MESSAGE_TOP)
    {
        MAKE_MICP_MESSAGE(MicpSetMuteValueCfm);

        message->id = id;
        message->prflHndl = micp_inst->micp_srvc_hdl;
        message->status = status;

        MicpMessageSend(micp_inst->app_task, message);
    }
}

/***************************************************************************/
static MicpMessageId micpGetMicsMessageIdFromOpcode(MicpMicsControlPointOpcodes opcode)
{
    MicpMessageId id = MICP_MESSAGE_TOP;

    switch(opcode)
    {
        case MICP_SET_MUTE_VALUE_OP:
        {
            id = MICP_SET_MUTE_VALUE_CFM;
        }
        break;

        default:
        break;
    }

    return id;
}

/****************************************************************************/
void micpMicsControlPointOp(ServiceHandle profile_handle,
                          MicpMicsControlPointOpcodes opcode,
                          uint8 mute_value_operand)
{
    MICP *micp_inst = ServiceHandleGetInstanceData((ServiceHandle) profile_handle);

    if (micp_inst)
    {
        switch(opcode)
        {
            case MICP_SET_MUTE_VALUE_OP:
            {
                micp_inst->mute_setting_pending = mute_value_operand;
                GattMicsClientSetMuteValueReq(micp_inst->mics_srvc_hdl,
                                              mute_value_operand);
            }
            break;

            default:
            {
                MICP_ERROR("Invalid MICS control point opcode\n");
                micpSendMuteControlPointOpCfm(micp_inst,
                                               CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                               micpGetMicsMessageIdFromOpcode(opcode));
        }
        }
    }
    else
    {
        MICP_DEBUG("Invalid MICP Profile instance\n");
    }
}

/****************************************************************************/
static MicpInternalMsg micpGetInternalMessageIdFromOpcode(MicpMicsControlPointOpcodes opcode)
{
    MicpInternalMsg id = MICP_INTERNAL_MSG_BASE;

    switch(opcode)
    {
        case MICP_SET_MUTE_VALUE_OP:
        {
            id = MICP_INTERNAL_SET_MUTE_VALUE;
        }
        break;

        default:
        break;
    }

    return id;
}

/****************************************************************************/
static void micpMicsSendInternalMsg(MICP *micp_inst,
                                  MicpMicsControlPointOpcodes opcode,
                                  uint8 mute_value)
{
    MAKE_MICP_INTERNAL_MESSAGE(MICP_INTERNAL_SET_MUTE_VALUE);

    message->id = micpGetInternalMessageIdFromOpcode(opcode);
    message->prfl_hndl = micp_inst->micp_srvc_hdl;
    message->mute_value = mute_value;

    MicpMessageSendConditionally(micp_inst->lib_task,
                                micpGetInternalMessageIdFromOpcode(opcode),
                                message,
                                &micp_inst->pending_op);
}

/****************************************************************************/
void MicpSetMuteValueReq(MicpProfileHandle profileHandle, uint8 muteValue)
{
    MICP *micp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (micp_inst)
    {
        micpMicsSendInternalMsg(micp_inst, MICP_SET_MUTE_VALUE_OP, muteValue);
    }
    else
    {
       MICP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
static void micpHandleMicsControlPointOpCfm(MICP *micp_inst,
                                                 status_t status,
                                                 MicpMicsControlPointOpcodes opcode)
{
    micpSendMuteControlPointOpCfm(micp_inst,
                                   status,
                                   micpGetMicsMessageIdFromOpcode(opcode));
    micp_inst->mute_setting_pending = 0;
    micp_inst->pending_op = MICP_PENDING_OP_NONE;
}

/****************************************************************************/
void micpHandleMicsSetMuteValueOp(MICP *micp_inst,
                                      const GattMicsClientSetMuteValueCfm *msg)
{
    micpHandleMicsControlPointOpCfm(micp_inst,
                                  msg->status,
                                  MICP_SET_MUTE_VALUE_OP);
}
