/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_write.h"
#include "gatt_vcs_client_debug.h"
#include "gatt_vcs_client.h"
#include "gatt_vcs_client_common.h"


/***************************************************************************/
void vcsClientHandleInternalWrite(GVCSC *const vcs_client,
                                  uint16 handle,
                                  uint16 size_value,
                                  uint8 * value)
{
    CsrBtGattWriteReqSend(vcs_client->srvcElem->gattId,
                          vcs_client->srvcElem->cid,
                          handle,
                          0,
                          size_value,
                          value);
}

/***************************************************************************/
static void vcsClientHandleVolumeControlPointOperation(const GVCSC *client,
                                                       vcs_client_control_point_opcodes_t opcode,
                                                       uint16 change_counter,
                                                       uint8 volume_setting,
                                                       uint8 vcs_cntrl_pnt_len)
{
    /* Check parameters */
    if (client == NULL)
    {
        GATT_VCS_CLIENT_PANIC("GVCSC: Invalid parameters\n");
    }
    else
    {
        MAKE_VCS_CLIENT_INTERNAL_MESSAGE(VCS_CLIENT_INTERNAL_MSG_WRITE);

        message->srvc_hndl = client->srvcElem->service_handle;
        message->handle = client->handles.volumeControlPointHandle;
        message->size_value = vcs_cntrl_pnt_len;

        message->value = (uint8*)(CsrPmemZalloc(VCS_CLIENT_VOL_CTRL_POINT_CHARACTERISTIC_SIZE_2));
        message->value[0] = opcode;
        message->value[1] = (uint8)change_counter;

        if (vcs_cntrl_pnt_len == VCS_CLIENT_VOL_CTRL_POINT_CHARACTERISTIC_SIZE_2)
        {
            message->value[2] = volume_setting;
        }

        VcsMessageSendConditionally(client->lib_task,
                                    VCS_CLIENT_INTERNAL_MSG_WRITE,
                                    message,
                                    &client->pending_cmd);
    }
}

/****************************************************************************/
static uint8 vcsClientGetCntrlPntLenFromOpcode(vcs_client_control_point_opcodes_t opcode)
{
    if (opcode == vcs_client_set_absolute_volume_op)
    {
        return VCS_CLIENT_VOL_CTRL_POINT_CHARACTERISTIC_SIZE_2;
    }
    else if (opcode < vcs_client_last_op)
    {
        return VCS_CLIENT_VOL_CTRL_POINT_CHARACTERISTIC_SIZE_1;
    }
    else
    {
        return 0;
    }
}

/****************************************************************************/
static void vcsClientControlPointOp(ServiceHandle clnt_hndl,
                                    vcs_client_control_point_opcodes_t opcode,
                                    uint8 change_counter,
                                    uint8 vol_setting_operand)
{
    GVCSC *client = ServiceHandleGetInstanceData(clnt_hndl);

    if (client)
    {
        vcsClientHandleVolumeControlPointOperation(client,
                                                   opcode,
                                                   change_counter,
                                                   vol_setting_operand,
                                                   vcsClientGetCntrlPntLenFromOpcode(opcode));
    }
    else
    {
        GATT_VCS_CLIENT_PANIC("Invalid VCS Client instance!\n");
    }
}

/****************************************************************************/
void GattVcsClientRelativeVolDownRequest(ServiceHandle clntHndl, uint8 changeCounter)
{
    vcsClientControlPointOp(clntHndl,
                            vcs_client_relative_volume_down_op,
                            changeCounter,
                            0);
}

/****************************************************************************/
void GattVcsClientRelativeVolUpRequest(ServiceHandle clntHndl, uint8 changeCounter)
{
    vcsClientControlPointOp(clntHndl,
                            vcs_client_relative_volume_up_op,
                            changeCounter,
                            0);
}

/****************************************************************************/
void GattVcsClientUnmuteRelativeVolDownRequest(ServiceHandle clntHndl, uint8 changeCounter)
{
    vcsClientControlPointOp(clntHndl,
                            vcs_client_unmute_relative_volume_down_op,
                            changeCounter,
                            0);
}

/****************************************************************************/
void GattVcsClientUnmuteRelativeVolUpRequest(ServiceHandle clntHndl, uint8 changeCounter)
{
    vcsClientControlPointOp(clntHndl,
                            vcs_client_unmute_relative_volume_up_op,
                            changeCounter,
                            0);
}

/****************************************************************************/
void GattVcsClientAbsoluteVolRequest(ServiceHandle clntHndl, uint8 changeCounter, uint8 volumeSetting)
{
    vcsClientControlPointOp(clntHndl,
                            vcs_client_set_absolute_volume_op,
                            changeCounter,
                            volumeSetting);
}

/****************************************************************************/
void GattVcsClientUnmuteRequest(ServiceHandle clntHndl, uint8 changeCounter)
{
    vcsClientControlPointOp(clntHndl,
                            vcs_client_unmute_op,
                            changeCounter,
                            0);
}

/****************************************************************************/
void GattVcsClientMuteRequest(ServiceHandle clntHndl, uint8 changeCounter)
{
    vcsClientControlPointOp(clntHndl,
                            vcs_client_mute_op,
                            changeCounter,
                            0);
}
