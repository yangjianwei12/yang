/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#include "gatt_mics_client_private.h"
#include "gatt_mics_client_write.h"
#include "gatt_mics_client_debug.h"
#include "gatt_mics_client.h"
#include "gatt_mics_client_common.h"


/***************************************************************************/
void micsClientHandleInternalWrite(GMICSC *const mics_client,
                                  uint16 handle,
                                  uint16 size_value,
                                  uint8 * value)
{
    CsrBtGattWriteReqSend(mics_client->srvcElem->gattId,
                          mics_client->srvcElem->cid,
                          handle,
                          0,
                          size_value,
                          value);
}

/***************************************************************************/
static void micsClientHandleSetMuteValueOperation(const GMICSC *client,
                                                       uint8 mute_value,
                                                       uint8 mics_cntrl_pnt_len)
{
    /* Check parameters */
    if (client == NULL)
    {
        GATT_MICS_CLIENT_PANIC("GMICSC: Invalid parameters\n");
    }
    else
    {
        MAKE_MICS_CLIENT_INTERNAL_MESSAGE(MICS_CLIENT_INTERNAL_MSG_WRITE);

        message->srvc_hndl = client->srvcElem->service_handle;
        message->handle = client->handles.muteHandle;
        message->size_value = mics_cntrl_pnt_len;

        message->value = (uint8*)(CsrPmemZalloc(MICS_CLIENT_MUTE_CHARACTERISTIC_SIZE));
        message->value[0] = mute_value;

        MicsMessageSendConditionally(client->lib_task,
                                    MICS_CLIENT_INTERNAL_MSG_WRITE,
                                    message,
                                    &client->pending_cmd);
    }
}

/****************************************************************************/
static uint8 micsClientGetCntrlPntLenFromOpcode(mics_client_control_point_opcodes_t opcode)
{
    if (opcode == mics_client_set_mute_value_op)
    {
        return MICS_CLIENT_MUTE_CHARACTERISTIC_SIZE;
    }
    else
    {
        return 0;
    }
}

/****************************************************************************/
static void micsClientControlPointOp(ServiceHandle clnt_hndl,
                                    mics_client_control_point_opcodes_t opcode,
                                    uint8 mute_value)
{
    GMICSC *client = ServiceHandleGetInstanceData(clnt_hndl);

    if (client)
    {
        if (mute_value >= MICS_CLIENT_MUTE_DISABLED)
        {
            micsClientSendMicsClientWriteCfm(client,
                                           ATT_RESULT_VALUE_NOT_ALLOWED,
                                           GATT_MICS_CLIENT_SET_MUTE_VALUE_CFM);
            return;
        }
        
        micsClientHandleSetMuteValueOperation(client,
                                                   mute_value,
                                                   micsClientGetCntrlPntLenFromOpcode(opcode));
    }
    else
    {
        GATT_MICS_CLIENT_PANIC("Invalid MICS Client instance!\n");
    }
}

/****************************************************************************/
void GattMicsClientSetMuteValueReq(ServiceHandle clntHndl, uint8 muteValue)
{
    micsClientControlPointOp(clntHndl,
                            mics_client_set_mute_value_op,
                            muteValue);
}

