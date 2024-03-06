/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "csr_bt_gatt_lib.h"

#include "gatt_vocs_client.h"
#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client_write.h"
#include "gatt_vocs_client_common.h"

#define VOCS_CLIENT_VOL_OFFSET_CNTRL_POINT_SIZE  (4)

/***************************************************************************/
static void vocsClientHandleVolOffsetControlPointOp(const GVOCS *client,
                                                    vocs_client_control_point_opcodes_t opcode,
                                                    uint16 change_counter,
                                                    int16 volume_offset)
{
    MAKE_VOCS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(VOCS_CLIENT_INTERNAL_MSG_WRITE, VOCS_CLIENT_VOL_OFFSET_CNTRL_POINT_SIZE);
    message->value = (uint8 *) CsrPmemZalloc(VOCS_CLIENT_VOL_OFFSET_CNTRL_POINT_SIZE);

    message->handle = client->handles.volumeOffsetControlPointHandle;
    message->size_value = VOCS_CLIENT_VOL_OFFSET_CNTRL_POINT_SIZE;

    message->value[0] = opcode;
    message->value[1] = (uint8)change_counter;
    message->value[2] = (uint8) volume_offset;
    message->value[3] = (uint8)(volume_offset >> 8);


    VocsMessageSendConditionally(client->lib_task,
                                 VOCS_CLIENT_INTERNAL_MSG_WRITE,
                                 message,
                                 &client->pending_cmd);
}

/***************************************************************************/
void GattVocsClientSetVolumeOffsetReq(ServiceHandle clntHndl,
                                      uint8 changeCounter,
                                      int16 volumeOffset)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vocsClientHandleVolOffsetControlPointOp(client,
                                                vocs_client_relative_set_vol_offset_op,
                                                changeCounter,
                                                volumeOffset);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}

/***************************************************************************/
void vocsClientHandleInternalWrite(GVOCS *const vocs_client,
                                   uint16 handle,
                                   uint16 size_value,
                                   uint8 *value)
{
    CsrBtGattWriteReqSend(vocs_client->srvcElem->gattId,
                          vocs_client->srvcElem->cid,
                          handle,
                          0,
                          size_value,
                          value);
}

/***************************************************************************/
void vocsClientHandleInternalWriteWithoutResponse(GVOCS *const vocs_client,
                                                  uint16 handle,
                                                  uint16 size_value,
                                                  uint8 *value)
{
    CsrBtGattWriteReqSend(vocs_client->srvcElem->gattId,
                          vocs_client->srvcElem->cid,
                          handle,
                          0,
                          size_value,
                          value);
}

/***************************************************************************/
void GattVocsClientSetAudioLocReq(ServiceHandle clntHndl,
                                  GattVocsClientAudioLoc audioLocationVal)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        if (client->handles.audioLocationProperties & VOCS_CLIENT_WRITE_CMD_PROP)
        {
            MAKE_VOCS_CLIENT_INTERNAL_MESSAGE(VOCS_CLIENT_INTERNAL_MSG_AUDIO_LOC_WRITE);

            message->handle = client->handles.audioLocationHandle;
            message->value = audioLocationVal;

            VocsMessageSendConditionally(client->lib_task,
                                     VOCS_CLIENT_INTERNAL_MSG_AUDIO_LOC_WRITE,
                                     message,
                                     &client->pending_cmd);
        }
        else
        {
            GATT_VOCS_CLIENT_ERROR("Write not supported!\n");
            vocsClientSendVocsClientWriteCfm(client,
                                             CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED,
                                             GATT_VOCS_CLIENT_SET_AUDIO_LOC_CFM);
        }
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}

/***************************************************************************/
void GattVocsClientSetAudioOutputDescReq(ServiceHandle clntHndl,
                                         uint16 sizeValue,
                                         uint8 *audioOutputDescVal)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        if (client->handles.audioOutputDescProperties & VOCS_CLIENT_WRITE_CMD_PROP)
        {
            MAKE_VOCS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(VOCS_CLIENT_INTERNAL_MSG_WRITE, sizeValue);
            message->value = (uint8 *) CsrPmemZalloc(sizeValue);

            message->handle = client->handles.audioOutputDescriptionHandle;
            message->size_value = sizeValue;

            memcpy(message->value, audioOutputDescVal, sizeValue);

            VocsMessageSendConditionally(client->lib_task,
                                         VOCS_CLIENT_INTERNAL_MSG_WRITE,
                                         message,
                                         &client->pending_cmd);
        }
        else
        {
            GATT_VOCS_CLIENT_ERROR("Write not supported!\n");
            vocsClientSendVocsClientWriteCfm(client,
                                             CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED,
                                             GATT_VOCS_CLIENT_SET_AUDIO_OUTPUT_DESC_CFM);
        }
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}
