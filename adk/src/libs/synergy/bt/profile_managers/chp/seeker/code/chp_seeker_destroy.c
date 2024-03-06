/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/

#include "chp_seeker_private.h"
#include "chp_seeker_destroy.h"
#include "chp_seeker_common.h"
#include "chp_seeker_debug.h"

void chpSeekerSendDestroyCfm(CHP *chpSeekerInst,
                             ChpSeekerStatus status)
{
    ChpSeekerDestroyCfm *message = CsrPmemAlloc(sizeof(*message));

    message->id = CHP_SEEKER_DESTROY_CFM;
    message->status = status;
    message->profileHandle = chpSeekerInst->chpSeekerSrvcHndl;

    ChpSeekerMessageSend(chpSeekerInst->appTask, message);
}

/******************************************************************************/
void ChpSeekerDestroyReq(ChpSeekerProfileHandle profileHandle)
{
    CHP *chpSeekerInst = ServiceHandleGetInstanceData(profileHandle);

    if (chpSeekerInst)
    {
        /* Send confirmation message with status in progress */
        chpSeekerSendDestroyCfm(chpSeekerInst, CHP_SEEKER_STATUS_IN_PROGRESS);

        /* If control point timer is running, stop it */
        stopControlPointTimer();

        GattTdsClientTerminateReq(chpSeekerInst->tdsSrvcHndl);
    }
    else
    {
        CHP_ERROR("Invalid profile handle\n");
    }
}

/****************************************************************************/
void chpSeekerHandleTdsClientTerminateResp(CHP *chpSeekerInst,
                                           const GattTdsClientTerminateCfm * message)
{
    if (message->status == GATT_TDS_CLIENT_STATUS_SUCCESS)
    {
        /* Destroy CHP instance */
        chpSeekerDestroyProfileInst(chpSeekerInst);
    }
    else
    {
        chpSeekerSendDestroyCfm(chpSeekerInst, CHP_SEEKER_STATUS_OPERATION_FAILED);
    }
}


/****************************************************************************/
void chpSeekerDestroyProfileInst(CHP *chpSeekerInst)
{
    bool res = FALSE;
    AppTask appTask = chpSeekerInst->appTask;
    ChpSeekerDestroyCfm *message = CsrPmemAlloc(sizeof(*message));
    ChpSeekerMainInst *mainInst = chpSeekerGetMainInstance();

    message->id = CHP_SEEKER_DESTROY_CFM;
    message->profileHandle = chpSeekerInst->chpSeekerSrvcHndl;

    /* Free the profile instance memory */
    res = FREE_CHP_INST(chpSeekerInst->chpSeekerSrvcHndl);

    /* Remove the profile element from main list */
    if (mainInst)
        REMOVE_CHP_SERVICE_HANDLE(mainInst->profileHandleList, chpSeekerInst->chpSeekerSrvcHndl);

    if (res)
    {
        message->status = CHP_SEEKER_STATUS_SUCCESS;
    }
    else
    {
        message->status = CHP_SEEKER_STATUS_OPERATION_FAILED;
    }

    ChpSeekerMessageSend(appTask, message);
}
