/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#include "gatt_mics_client.h"

#include "micp.h"
#include "micp_private.h"
#include "micp_destroy.h"
#include "micp_common.h"
#include "micp_debug.h"

void micpSendDestroyCfm(MICP * micp_inst, MicpStatus status)
{
    MAKE_MICP_MESSAGE(MicpDestroyCfm);

    message->id = MICP_DESTROY_CFM;
    message->status = status;
    message->prflHndl = micp_inst->micp_srvc_hdl;

    MicpMessageSend(micp_inst->app_task, message);
}

/******************************************************************************/
void MicpDestroyReq(MicpProfileHandle profileHandle)
{
    MICP * micp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (micp_inst)
    {
        /* Send confirmation message with status in progress */
        micpSendDestroyCfm(micp_inst, MICP_STATUS_IN_PROGRESS);

        /* Terminate MICS Client */
        GattMicsClientTerminateReq(micp_inst->mics_srvc_hdl);
    }
    else
    {
        MICP_DEBUG("Invalid profile handle\n");
    }
}

/******************************************************************************/
void micpSendMicsTerminateCfm(MICP *micp_inst,
                            MicpStatus status,
                            GattMicsClientDeviceData handles)
{
    MAKE_MICP_MESSAGE(MicpMicsTerminateCfm);

    message->id = MICP_MICS_TERMINATE_CFM;
    message->status = status;
    message->prflHndl = micp_inst->micp_srvc_hdl;

    CsrMemCpy(&(message->micsHandle), &handles, sizeof(GattMicsClientDeviceData));

    MicpMessageSend(micp_inst->app_task, message);
}

/****************************************************************************/
void micpHandleAicsClientTerminateResp(MICP *micp_inst,
                                      const GattAicsClientTerminateCfm *message)
{
    if (message->status == GATT_AICS_CLIENT_STATUS_SUCCESS)
    {
        /* A AICS Client instance has been terminated successfully. */
        micp_inst->aics_counter -= 1;

        /* Save the aics characteristic handles to send to the application
        micpAddAicsInst(micp_inst,
                       &(message->device_data),
                       message->start_handle,
                       message->end_handle);
*/
        if (!micp_inst->aics_counter)
        {
            /* We can destroy all the list we have in the profile context */
            micpDestroyProfileInst(micp_inst);
        }
    }
    else
    {
        micpSendDestroyCfm(micp_inst, MICP_STATUS_FAILED);
    }
}

/****************************************************************************/
void micpHandleMicsClientTerminateResp(MICP *micp_inst,
                                     const GattMicsClientTerminateCfm *message)
{
    if (message->status == GATT_MICS_CLIENT_STATUS_SUCCESS)
    {
        /* MICS Client has been terminated successfully. */

        /* Send the mics characteristic handles to the application */
        micpSendMicsTerminateCfm(micp_inst,
                               MICP_STATUS_SUCCESS,
                               message->deviceData);

        if (micp_inst->aics_num)
        {
            /* If there are AICS instances, we need to terminate them too. */

            micp_inst->aics_counter = micp_inst->aics_num;

            /*while(ptr)
            {
                GattAicsClientTerminateReq(ptr->srvc_hdnl);
                ptr = ptr->next;
            }*/
        }
        else
        {
            /* There are no instances of AICS */
            micpDestroyProfileInst(micp_inst);
        }
    }
    else
    {
        micpSendDestroyCfm(micp_inst, MICP_STATUS_FAILED);
    }
}

/****************************************************************************/
void micpSendIncludedServicesTerminateCfm(MICP *micp_inst, MicpStatus status)
{
    micp_aics_inst_t *aics_ptr = micp_inst->first_aics_inst;

    if (micp_inst->aics_num)
    {
        while(aics_ptr)
        {
            MAKE_MICP_MESSAGE(MicpAicsTerminateCfm);

            message->id = MICP_AICS_TERMINATE_CFM;
            message->status = status;
            message->prflHndl = micp_inst->micp_srvc_hdl;
            message->aicsSizeValue = micp_inst->aics_num;
            message->startHandle = aics_ptr->start_handle;
            message->endHandle = aics_ptr->end_handle;

            CsrMemCpy(&message->aicsValue, &aics_ptr->device_data, sizeof(GattAicsClientDeviceData));

            aics_ptr = aics_ptr->next;

            if(aics_ptr)
            {
                message->moreToCome = TRUE;
            }
            else
            {
                message->moreToCome =FALSE;
            }

            MicpMessageSend(micp_inst->app_task, message);
        }
    }
    else
    {
        MAKE_MICP_MESSAGE(MicpAicsTerminateCfm);

        message->id = MICP_AICS_TERMINATE_CFM;
        message->status = status;
        message->prflHndl = micp_inst->micp_srvc_hdl;
        message->aicsSizeValue = 0;
        message->startHandle = 0;
        message->endHandle = 0;

        CsrMemSet(&message->aicsValue, 0, sizeof(GattAicsClientDeviceData));

        message->moreToCome = FALSE;

        MicpMessageSend(micp_inst->app_task, message);
    }
}

/****************************************************************************/
void micpDestroyProfileInst(MICP *micp_inst)
{
    bool res = FALSE;
    AppTask app_task = micp_inst->app_task;
    MAKE_MICP_MESSAGE(MicpDestroyCfm);

    /* Send the AICS characteristic handles to the application */
    micpSendIncludedServicesTerminateCfm(micp_inst, MICP_STATUS_SUCCESS);

    /* We can destroy all the list we have in the profile context */
    micpDestroyReqAllSrvcHndlList(micp_inst);
    MicpDestroyReqAllInstList(micp_inst);

    /* Clear pending messages
    MessageFlushTask((Task)&micp_inst->lib_task); */

    /* Send the confirmation message */

    message->id = MICP_DESTROY_CFM;
    message->prflHndl = micp_inst->micp_srvc_hdl;

    /* Free the profile instance memory */
    res = ServiceHandleFreeInstanceData((ServiceHandle) (micp_inst->micp_srvc_hdl));

    if (res)
    {
        message->status = MICP_STATUS_SUCCESS;
    }
    else
    {
        message->status = MICP_STATUS_FAILED;
    }

    MicpMessageSend(app_task, message);
}
