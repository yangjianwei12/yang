/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    vcp_profile_client
    \brief      VCP Profile Client
*/

#include "vcp_profile_client.h"

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

#include "synergy.h"
#include "vcp.h"
#include <logging.h>

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

/*! \brief VCP profile task data */
typedef struct
{
    /*! VCP Client profile task */
    TaskData   task_data;
    connection_id_t  cid;
    VcpProfileHandle vcp_handle;
} vcp_profile_client_task_data_t;

/*! \brief VCP client task data. */
static vcp_profile_client_task_data_t *vcp_profile_client_taskdata = NULL;

static void vcpProfileClient_HandleVcpInitCfm(VcpInitCfm *msg)
{
    VcpInitCfm *cfm = (VcpInitCfm*)msg;

    DEBUG_LOG("vcpProfileClient_HandleVcpInitCfm: VcpInitCfm: \n");
    DEBUG_LOG("Status : 0x%02x, Vcp Handle: 0x%04x, CID: 0x%04x \n", cfm->status, cfm->prflHndl, cfm->cid);

    vcp_profile_client_taskdata->vcp_handle = cfm->prflHndl;
    vcp_profile_client_taskdata->cid = cfm->cid;
}
static void vcpProfileClient_HandleVcpMessage(Message message)
{
    CsrBtGattPrim *vcp_id = (CsrBtGattPrim *)message;

    DEBUG_LOG("\n(VCP) : vcpProfileClient_HandleVcpMessage : %x\n", *vcp_id);

    switch(*vcp_id)
    {
        case VCP_INIT_CFM:
            vcpProfileClient_HandleVcpInitCfm((VcpInitCfm *) message);
        break;

        case VCP_DESTROY_CFM:
        {
            VcpDestroyCfm *cfm = (VcpDestroyCfm*)message;

            DEBUG_LOG("\n(VCP) : VCP_DESTROY_CFM:\n");

            if(cfm->status == VCP_STATUS_SUCCESS)
            {
                /* free instance data */
                CsrPmemFree(vcp_profile_client_taskdata);
                vcp_profile_client_taskdata = NULL;
            }
        }
        break;

        default:
            DEBUG_LOG("vcpProfileClient_HandleMessage  Message Id : 0x%x", vcp_id);
        break;
    }
}

/*! \brief Handler that receives notification from BAP Profile library */
static void vcpProfileClient_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case VCP_CLIENT_PRIM:
            vcpProfileClient_HandleVcpMessage(message);
        break;

        default:
            DEBUG_LOG("vcpProfileClient_HandleMessage Unhandled Message Id : 0x%x", id);
        break;
    }
}

static void vcpClient_Init(void)
{
    DEBUG_LOG("VcpClient_Init");

    if (vcp_profile_client_taskdata == NULL)
    {
        vcp_profile_client_taskdata = (vcp_profile_client_task_data_t *) CsrPmemAlloc(sizeof(vcp_profile_client_task_data_t));

        memset(vcp_profile_client_taskdata, 0, sizeof(vcp_profile_client_task_data_t));
        vcp_profile_client_taskdata->task_data.handler = vcpProfileClient_HandleMessage;
    }
    else
    {
        DEBUG_LOG("VcpClient_Init : Already Initialized");
        Panic();
    }
}

bool VcpProfileClient_CreateInstance(gatt_cid_t cid)
{
    VcpInitData vcp_init_data;

    vcp_init_data.cid = cid;

    vcpClient_Init();
    VcpInitReq(TrapToOxygenTask((Task)&vcp_profile_client_taskdata->task_data), &vcp_init_data, NULL, FALSE);
    vcp_profile_client_taskdata->cid = cid;

    return TRUE;
}

void VcpProfileClient_ReadStateCharacteristic(void)
{
    VcpVolumeStateRequest(vcp_profile_client_taskdata->vcp_handle);
}

void VcpProfileClient_ReadFlagCharacteristic(void)
{
    VcpReadVolumeFlagRequest(vcp_profile_client_taskdata->vcp_handle);
}

void VcpProfileClient_SetAbsoluteVolume(void)
{
    VcpAbsoluteVolumeRequest(vcp_profile_client_taskdata->vcp_handle, 10);
}

void VcpProfileClient_UnmuteVolume(void)
{
    VcpUnmuteRequest(vcp_profile_client_taskdata->vcp_handle);
}

void VcpProfileClient_RegisterNotification(void)
{
    VcpVolumeStateRegisterForNotificationReq(vcp_profile_client_taskdata->vcp_handle, TRUE);
    VcpVolumeFlagRegisterForNotificationReq(vcp_profile_client_taskdata->vcp_handle, TRUE);
}

bool VcpProfileClient_DestroyInstance(gatt_cid_t cid)
{
    bool status = FALSE;

    DEBUG_LOG("VcpProfileClient_DestroyInstance cid 0x%x", cid);

    if (vcp_profile_client_taskdata != NULL && vcp_profile_client_taskdata->cid == cid)
    {
        VcpDestroyReq(vcp_profile_client_taskdata->vcp_handle);
        status = TRUE;
    }

    return status;
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

