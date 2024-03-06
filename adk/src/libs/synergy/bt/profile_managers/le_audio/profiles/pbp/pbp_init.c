/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#include "pbp.h"
#include "pbp_debug.h"
#include "pbp_init.h"
#include "pbp_prim.h"

PbpMainInst *pbpMain;

/******************************************************************************/
void pbpSendInitCfm(PBP *pbpInst, PbpStatus status)
{
    PbpInitCfm *message = CsrPmemZalloc(sizeof(*message));

    message->status = status;
    message->prflHndl = pbpInst->pbpSrvcHndl;

    PbpMessageSend(pbpInst->appTask, PBP_INIT_CFM, message);
} 

/***************************************************************************/
void PbpInitReq(AppTask appTask)
{
    PBP* pbpInst = NULL;
    PbpProfileHandle profileHndl = 0;
    PbpProfileHandleListElm *elem = NULL;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        PBP_PANIC("Application Task NULL\n");
    }

    elem = ADD_PBP_SERVICE_HANDLE(pbpMain->profileHandleList);
    profileHndl = ADD_PBP_INST(pbpInst);
    elem->profileHandle = profileHndl;

    if (profileHndl)
    {
        /* Reset all the profile library memory */
        memset(pbpInst, 0, sizeof(PBP));

        /* Set up library task for external messages */
        pbpInst->libTask = CSR_BT_PBP_IFACEQUEUE;

        /* Store the Application Task.
         * All library messages need to be sent here */
        pbpInst->appTask = appTask;

        pbpInst->pbpSrvcHndl = profileHndl;
        pbpInst->bcastSrcHandle = NO_PROFILE_HANDLE;

        pbpSendInitCfm(pbpInst, PBP_STATUS_SUCCESS);
    }
    else
    {
        PBP_PANIC("Memory allocation of PBP Profile instance failed!\n");
    }
} 

static void initProfileHandleList(CsrCmnListElm_t *elem)
{
    PbpProfileHandleListElm *cElem = (PbpProfileHandleListElm*) elem;

    cElem->profileHandle = 0;
}

void pbpInit(void **gash)
{
    pbpMain = CsrPmemZalloc(sizeof(PbpMainInst));
    *gash = pbpMain;

    CsrCmnListInit(&pbpMain->profileHandleList, 0, initProfileHandleList, NULL);
}

PbpMainInst *pbpGetMainInstance(void)
{
    return pbpMain;
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************/
void pbpDeInit(void **gash)
{
    CsrCmnListDeinit(&pbpMain->profileHandleList);
    CsrPmemFree(pbpMain);
}
#endif
