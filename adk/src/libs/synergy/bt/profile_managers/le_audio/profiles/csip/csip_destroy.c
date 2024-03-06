/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_csis_client.h"

#include "csip.h"
#include "csip_private.h"
#include "csip_destroy.h"
#include "csip_debug.h"

void csipSendDestroyCfm(Csip * csipInst, CsipStatus status, CsipHandles *handles)
{
    CsipDestroyCfm *message =
            (CsipDestroyCfm *) CsrPmemZalloc(sizeof(CsipDestroyCfm));

    message->id = CSIP_DESTROY_CFM;
    message->status = status;
    message->prflHndl = csipInst->csipSrvcHdl;
    message->handles = handles;

    CsipMessageSend(csipInst->app_task, message);

    /* Only one instance of CSIS per CSIP, hence can be destroyed*/
}

/******************************************************************************/
void CsipDestroyReq(CsipProfileHandle profileHandle)
{
    Csip * csipInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (csipInst)
    {
        /* Send confirmation message with status in progress */
        csipSendDestroyCfm(csipInst, CSIP_STATUS_IN_PROGRESS, NULL);

        /* Terminate CSIS Client */
        GattCsisClientTerminate(csipInst->csisSrvcHdl);
    }
    else
    {
        CSIP_ERROR("Invalid profile handle\n");
    }
}

/****************************************************************************/
void csipHandleCsisClientTerminateResp(Csip *csipInst,
                                     const GattCsisClientTerminateCfm * message)
{

    if (message->status == GATT_CSIS_CLIENT_STATUS_SUCCESS)
    {
        /* CSIS Client has been terminated successfully. */

        /* Send the csis characteristic handles to the application */
        CsipHandles *handles = CsrPmemZalloc(sizeof(CsipHandles));
        handles->csisHandle.csisLockCcdHandle = message->deviceData.csisLockCcdHandle;
        handles->csisHandle.csisLockHandle = message->deviceData.csisLockHandle;
        handles->csisHandle.csisRankHandle = message->deviceData.csisRankHandle;
        handles->csisHandle.csisSirkCcdHandle = message->deviceData.csisSirkCcdHandle;
        handles->csisHandle.csisSirkHandle = message->deviceData.csisSirkHandle;
        handles->csisHandle.csisSizeCcdHandle =  message->deviceData.csisSizeCcdHandle;
        handles->csisHandle.csisSizeHandle = message->deviceData.csisSizeHandle;
        handles->csisHandle.endHandle = message->deviceData.endHandle;
        handles->csisHandle.startHandle = message->deviceData.startHandle;

        csipDestroyProfileInst(csipInst, handles);
    }
    else
    {
        csipSendDestroyCfm(csipInst, CSIP_STATUS_FAILED, NULL);
    }
}

/****************************************************************************/
void csipDestroyProfileInst(Csip *csipInst, CsipHandles* handles)
{
    bool res = FALSE;
    AppTask appTask = csipInst->app_task;

    /* Clear pending messages
    MessageFlushTask((Task)&csipInst->lib_task); */

    /* Send the confirmation message */
    CsipDestroyCfm *message =
            (CsipDestroyCfm *) CsrPmemZalloc(sizeof(CsipDestroyCfm));

    message->id = CSIP_DESTROY_CFM;
    message->prflHndl = csipInst->csipSrvcHdl;
    message->handles = NULL;

    /* Free the profile instance memory */
    res = ServiceHandleFreeInstanceData((ServiceHandle) (csipInst->csipSrvcHdl));

    if (res)
    {
        if (handles)
        {
            message->handles = handles;
            handles = NULL;
        }
        message->status = CSIP_STATUS_SUCCESS;
    }
    else
    {
        message->status = CSIP_STATUS_INST_PERSISTED;
    }

    CsipMessageSend(appTask, message);
}
