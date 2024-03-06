/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#include "pbp_debug.h"
#include "pbp_private.h"

/****************************************************************************/
static void pbpDestroyProfileInst(PBP* pbpInst)
{
    bool res = FALSE;
    AppTask appTask = pbpInst->appTask;

    /* Send the confirmation message */
    PbpDestroyCfm* message = CsrPmemAlloc(sizeof(*message));
    PbpMainInst* mainInst = pbpGetMainInstance();

    message->prflHndl = pbpInst->pbpSrvcHndl;

    /* Free the profile instance memory */
    res = FREE_PBP_INST(pbpInst->pbpSrvcHndl);

    /* Remove the profile element from main list */
    if (mainInst)
        REMOVE_PBP_SERVICE_HANDLE(mainInst->profileHandleList, pbpInst->pbpSrvcHndl);

    if (res)
    {
        message->status = PBP_STATUS_SUCCESS;
    }
    else
    {
        message->status = PBP_STATUS_FAILED;
    }

    PbpMessageSend(appTask, PBP_DESTROY_CFM, message);
}

/******************************************************************************/
void PbpDestroyReq(PbpProfileHandle profileHandle)
{
    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        pbpDestroyProfileInst(pbpInst);
    }
    else
    {
        PBP_ERROR("Invalid profile handle\n");
    }
}
