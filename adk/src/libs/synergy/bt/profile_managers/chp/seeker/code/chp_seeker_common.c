/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#include "chp_seeker_common.h"
#include "chp_seeker_debug.h"

static uint32 controlPointTimerId = 0;
/****************************************************************************/
static void handleControlPointTimer(uint16 m, void *data)
{
    void* msg = NULL;
    CHP * chpSeekerInst = (CHP*)data;
    ChpSeekerActivateTransportRspInd* message = CsrPmemAlloc(sizeof(*message));

    CSR_UNUSED(m);

    CHP_INFO("(CHP) :Control Point timer expired\n");
    controlPointTimerId = 0;

    message->id = CHP_SEEKER_ACTIVATE_TRANSPORT_RSP_IND;
    message->profileHandle = chpSeekerInst->chpSeekerSrvcHndl;
    message->status   = CHP_SEEKER_STATUS_OPERATION_FAILED;
    message->responseDataLen = 0;
    message->responseData = NULL;

    msg = (void*)message;
    ChpSeekerMessageSend(chpSeekerInst->appTask, msg);
}

/****************************************************************************/
void startControlPointTimer(CHP *chpSeekerInst)
{
    if(!controlPointTimerId)
    {
        controlPointTimerId = CsrSchedTimerSet(CSR_SCHED_SECOND * CONTROL_POINT_TIMER_TIMEOUT_SECONDS, handleControlPointTimer, 0, (void*)chpSeekerInst);
        CHP_INFO("(CHP) : Started Control Point timer of 10 seconds\n");
    }
}

/****************************************************************************/
void stopControlPointTimer(void)
{
    CsrSchedTimerCancel(controlPointTimerId, NULL, NULL);
    controlPointTimerId = 0;
    CHP_INFO("(CHP) : Stopped Control Point timer\n");
}