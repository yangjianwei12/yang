/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "csr_bt_gatt_lib.h"

#include "gatt_aics_client_private.h"
#include "gatt_aics_client_write.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client.h"
#include "gatt_aics_client_common.h"

#define AICS_CLIENT_AUDIO_INPUT_CTRL_POINT_SIZE_1  (2)
#define AICS_CLIENT_AUDIO_INPUT_CTRL_POINT_SIZE_2  (3)

/***************************************************************************/
static void aisClientHandleAudioInputControlPointOperation(const GAICS *client,
                                                           aics_client_control_point_opcodes_t opcode,
                                                           uint8 change_counter,
                                                           int8 gain_setting,
                                                           uint8 aics_cntrl_pnt_len)
{
    MAKE_AICS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(AICS_CLIENT_INTERNAL_MSG_WRITE, aics_cntrl_pnt_len);
    message->value = (uint8 *) CsrPmemZalloc(aics_cntrl_pnt_len);

    message->handle = client->handles.audioInputControlPointHandle;
    message->size_value = aics_cntrl_pnt_len;

    message->value[0] = (uint8) opcode;
    message->value[1] = (uint8) change_counter;

    if (aics_cntrl_pnt_len == AICS_CLIENT_AUDIO_INPUT_CTRL_POINT_SIZE_2)
    {
        message->value[2] = gain_setting;
    }

    AicsMessageSendConditionally(client->lib_task,
                                 AICS_CLIENT_INTERNAL_MSG_WRITE,
                                 message,
                                 &client->pending_cmd);
}

/****************************************************************************/
static uint8 aicsClientGetCntrlPntLenFromOpcode(aics_client_control_point_opcodes_t opcode)
{
    if (opcode == aics_client_set_gain_setting_op)
    {
        return AICS_CLIENT_AUDIO_INPUT_CTRL_POINT_SIZE_2;
    }
    else if (opcode < aics_client_last_op)
    {
        return AICS_CLIENT_AUDIO_INPUT_CTRL_POINT_SIZE_1;
    }
    else
    {
        return 0;
    }
}

/****************************************************************************/
static void aicsClientControlPointOp(ServiceHandle clnt_hndl,
                                     aics_client_control_point_opcodes_t opcode,
                                     uint8 change_counter,
                                     int8 gain_setting_operand)
{
    GAICS *client = ServiceHandleGetInstanceData(clnt_hndl);

    if (client)
    {
        aisClientHandleAudioInputControlPointOperation(client,
                                                       opcode,
                                                       change_counter,
                                                       gain_setting_operand,
                                                       aicsClientGetCntrlPntLenFromOpcode(opcode));
    }
    else
    {
        gattAicsClientError();
    }
}

/***************************************************************************/
void GattAicsClientSetGainSettingReq(ServiceHandle clntHndl, uint8 changeCounter, int8 gainSetting)
{
    aicsClientControlPointOp(clntHndl,
                             aics_client_set_gain_setting_op,
                             changeCounter,
                             gainSetting);
}

/***************************************************************************/
void aicsClientHandleInternalWrite(GAICS *const aics_client,
                                   uint16 handle,
                                   uint16 size_value,
                                   uint8 * value)
{
    CsrBtGattWriteReqSend(aics_client->srvcElem->gattId,
                          aics_client->srvcElem->cid,
                          handle,
                          0,
                          size_value,
                          value);
}

/***************************************************************************/
void GattAicsClientUnmuteReq(ServiceHandle clntHndl, uint8 changeCounter)
{
    aicsClientControlPointOp(clntHndl,
                             aics_client_unmute_op,
                             changeCounter,
                             0);
}

/***************************************************************************/
void GattAicsClientMuteReq(ServiceHandle clntHndl, uint8 changeCounter)
{
    aicsClientControlPointOp(clntHndl,
                             aics_client_mute_op,
                             changeCounter,
                             0);
}

/***************************************************************************/
void GattAicsClientSetManualGainModeReq(ServiceHandle clntHndl, uint8 changeCounter)
{
    aicsClientControlPointOp(clntHndl,
                             aics_client_set_manual_gain_mode_op,
                             changeCounter,
                             0);
}

/***************************************************************************/
void GattAicsClientSetAutomaticGainModeReq(ServiceHandle clntHndl, uint8 changeCounter)
{
    aicsClientControlPointOp(clntHndl,
                             aics_client_set_automatic_gain_mode_op,
                             changeCounter,
                             0);
}

/****************************************************************************/
void GattAicsClientSetAudioInputDescReq(ServiceHandle clntHndl,
                                        uint16 sizeValue,
                                        uint8 *audioInputDescVal)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        if (client->handles.audioInputDescProperties & AICS_CLIENT_WRITE_CMD_PROP)
        {
            MAKE_AICS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(AICS_CLIENT_INTERNAL_MSG_WRITE, sizeValue);
            memset(message, 0, sizeof(AICS_CLIENT_INTERNAL_MSG_WRITE_T) + ((sizeValue - 1) * sizeof(uint8)));
            message->value = (uint8 *) CsrPmemZalloc(sizeValue);


            message->handle = client->handles.audioInputDescriptionHandle;
            message->size_value = sizeValue;

            memcpy(message->value, audioInputDescVal, sizeValue);

            AicsMessageSendConditionally(client->lib_task,
                                         AICS_CLIENT_INTERNAL_MSG_WRITE,
                                         message,
                                         &client->pending_cmd);
        }
        else
        {
            GATT_AICS_CLIENT_ERROR("Write not supported!\n");
            aicsClientSendAicsClientWriteCfm(client,
                                             CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED,
                                             GATT_AICS_CLIENT_SET_AUDIO_INPUT_DESC_CFM);
        }
    }
    else
    {
        gattAicsClientError();
    }
}

/***************************************************************************/
void aicsClientHandleInternalWriteWithoutResponse(GAICS *const aics_client,
                                                  uint16 handle,
                                                  uint16 size_value,
                                                  uint8 *value)
{
    CsrBtGattWriteReqSend(aics_client->srvcElem->gattId,
                          aics_client->srvcElem->cid,
                          handle,
                          0,
                          size_value,
                          value);
}
