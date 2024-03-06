/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_vocs_client.h"
#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client_write.h"
#include "gatt_vocs_client_common.h"

#define VOCS_CLIENT_VOL_OFFSET_CNTRL_POINT_SIZE  (4)

/***************************************************************************/
static void vocsClientHandleVolOffsetControlPointOp(const GVOCS *gatt_vocs_client,
                                                    vocs_client_control_point_opcodes_t opcode,
                                                    uint16 change_counter,
                                                    int16 volume_offset)
{
    MAKE_VOCS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(VOCS_CLIENT_INTERNAL_MSG_WRITE, VOCS_CLIENT_VOL_OFFSET_CNTRL_POINT_SIZE);

    message->handle = gatt_vocs_client->handles.volumeOffsetControlPointHandle;
    message->size_value = VOCS_CLIENT_VOL_OFFSET_CNTRL_POINT_SIZE;

    message->value[0] = opcode;
    message->value[1] = change_counter;
    message->value[2] = (uint8) volume_offset;
    message->value[3] = (uint8)(volume_offset >> 8);


    MessageSendConditionally((Task)&gatt_vocs_client->lib_task,
                             VOCS_CLIENT_INTERNAL_MSG_WRITE,
                             message,
                             &gatt_vocs_client->pending_cmd);
}

/***************************************************************************/
void GattVocsClientSetVolumeOffsetReq(ServiceHandle clntHndl,
                                      uint8 changeCounter,
                                      int16 volumeOffset)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        vocsClientHandleVolOffsetControlPointOp(gatt_vocs_client,
                                                vocs_client_relative_set_vol_offset_op,
                                                changeCounter,
                                                volumeOffset);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}

/***************************************************************************/
void vocsClientHandleInternalWrite(GVOCS *const vocs_client,
                                   uint16 handle,
                                   uint16 size_value,
                                   uint8 *value)
{
    GattManagerWriteCharacteristicValue((Task)&vocs_client->lib_task,
                                        handle,
                                        size_value,
                                        value);
}

/***************************************************************************/
void vocsClientHandleInternalWriteWithoutResponse(GVOCS *const vocs_client,
                                                  uint16 handle,
                                                  uint16 size_value,
                                                  uint8 *value)
{
    GattManagerWriteWithoutResponse((Task)&vocs_client->lib_task,
                                     handle,
                                     size_value,
                                     value);
}

/***************************************************************************/
void GattVocsClientSetAudioLocReq(ServiceHandle       clntHndl,
                                  GattVocsClientAudioLoc audioLocationVal)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        if (gatt_vocs_client->handles.audioLocationProperties & VOCS_CLIENT_WRITE_CMD_PROP)
        {
            MAKE_VOCS_CLIENT_INTERNAL_MESSAGE(VOCS_CLIENT_INTERNAL_MSG_AUDIO_LOC_WRITE);

            message->handle = gatt_vocs_client->handles.audioLocationHandle;
            message->value = audioLocationVal;

            MessageSendConditionally((Task)&gatt_vocs_client->lib_task,
                                     VOCS_CLIENT_INTERNAL_MSG_AUDIO_LOC_WRITE,
                                     message,
                                     &gatt_vocs_client->pending_cmd);
        }
        else
        {
            GATT_VOCS_CLIENT_DEBUG_PANIC(("Write not supported!\n"));
            vocsClientSendVocsClientWriteCfm(gatt_vocs_client,
                                             gatt_status_request_not_supported,
                                             GATT_VOCS_CLIENT_SET_AUDIO_LOC_CFM);
        }
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}

/***************************************************************************/
void GattVocsClientSetAudioOutputDescReq(ServiceHandle clntHndl,
                                         uint16 sizeValue,
                                         uint8 *audioOutputDescVal)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        if (gatt_vocs_client->handles.audioOutputDescProperties & VOCS_CLIENT_WRITE_CMD_PROP)
        {
            MAKE_VOCS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(VOCS_CLIENT_INTERNAL_MSG_WRITE, sizeValue);

            message->handle = gatt_vocs_client->handles.audioOutputDescriptionHandle;
            message->size_value = sizeValue;

            memcpy(message->value, audioOutputDescVal, sizeValue);

            MessageSendConditionally((Task)&gatt_vocs_client->lib_task,
                                     VOCS_CLIENT_INTERNAL_MSG_WRITE,
                                     message,
                                     &gatt_vocs_client->pending_cmd);
        }
        else
        {
            GATT_VOCS_CLIENT_DEBUG_PANIC(("Write not supported!\n"));
            vocsClientSendVocsClientWriteCfm(gatt_vocs_client,
                                             gatt_status_request_not_supported,
                                             GATT_VOCS_CLIENT_SET_AUDIO_OUTPUT_DESC_CFM);
        }
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}
