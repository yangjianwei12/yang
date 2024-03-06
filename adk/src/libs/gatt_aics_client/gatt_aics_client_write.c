/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_aics_client_private.h"
#include "gatt_aics_client_write.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client.h"
#include "gatt_aics_client_common.h"

#define AICS_CLIENT_AUDIO_INPUT_CTRL_POINT_SIZE_1  (2)
#define AICS_CLIENT_AUDIO_INPUT_CTRL_POINT_SIZE_2  (3)

/***************************************************************************/
static void aisClientHandleAudioInputControlPointOperation(const GAICS *gatt_aics_client,
                                                           aics_client_control_point_opcodes_t opcode,
                                                           uint8 change_counter,
                                                           int8 gain_setting,
                                                           uint8 aics_cntrl_pnt_len)
{
    MAKE_AICS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(AICS_CLIENT_INTERNAL_MSG_WRITE,
                                               aics_cntrl_pnt_len);

    message->handle = gatt_aics_client->handles.audioInputControlPointHandle;
    message->size_value = aics_cntrl_pnt_len;

    message->value[0] = (uint8) opcode;
    message->value[1] = (uint8) change_counter;

    if (aics_cntrl_pnt_len == AICS_CLIENT_AUDIO_INPUT_CTRL_POINT_SIZE_2)
    {
        message->value[2] = gain_setting;
    }

    MessageSendConditionally((Task)&gatt_aics_client->lib_task,
                             AICS_CLIENT_INTERNAL_MSG_WRITE,
                             message,
                             &gatt_aics_client->pending_cmd);
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
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clnt_hndl);

    if (gatt_aics_client)
    {
        aisClientHandleAudioInputControlPointOperation(gatt_aics_client,
                                                       opcode,
                                                       change_counter,
                                                       gain_setting_operand,
                                                       aicsClientGetCntrlPntLenFromOpcode(opcode));
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/***************************************************************************/
void GattAicsClientSetGainSettingReq(ServiceHandle clntHndl,
                                     uint8 changeCounter,
                                     int8 gainSetting)
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
    GattManagerWriteCharacteristicValue((Task)&aics_client->lib_task,
                                        handle,
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
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        if (gatt_aics_client->handles.audioInputDescProperties & AICS_CLIENT_WRITE_CMD_PROP)
        {
            MAKE_AICS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(AICS_CLIENT_INTERNAL_MSG_WRITE, sizeValue);
            memset(message, 0, sizeof(AICS_CLIENT_INTERNAL_MSG_WRITE_T) + ((sizeValue - 1) * sizeof(uint8)));


            message->handle = gatt_aics_client->handles.audioInputDescriptionHandle;
            message->size_value = sizeValue;

            memcpy(message->value, audioInputDescVal, sizeValue);

            MessageSendConditionally((Task)&gatt_aics_client->lib_task,
                                     AICS_CLIENT_INTERNAL_MSG_WRITE,
                                     message,
                                     &gatt_aics_client->pending_cmd);
        }
        else
        {
            GATT_AICS_CLIENT_DEBUG_PANIC(("Write not supported!\n"));
            aicsClientSendAicsClientWriteCfm(gatt_aics_client,
                                             gatt_status_request_not_supported,
                                             GATT_AICS_CLIENT_SET_AUDIO_INPUT_DESC_CFM);
        }
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/***************************************************************************/
void aicsClientHandleInternalWriteWithoutResponse(GAICS *const aics_client,
                                                  uint16 handle,
                                                  uint16 size_value,
                                                  uint8 *value)
{
    GattManagerWriteWithoutResponse((Task)&aics_client->lib_task,
                                     handle,
                                     size_value,
                                     value);
}
