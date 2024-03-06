/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #59 $
******************************************************************************/

#include "chp_seeker_debug.h"
#include "chp_seeker_init.h"
#include "chp_seeker_common.h"
#include "gatt_service_discovery_lib.h"

ChpSeekerMainInst *chpSeekerMain;
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtChpsLto);

/******************************************************************************/
void chpSeekerSendInitCfm(CHP *chpSeekerInst, ChpSeekerStatus status)
{
    ChpSeekerInitCfm *message = CsrPmemAlloc(sizeof(*message));

    message->id = CHP_SEEKER_INIT_CFM;
    message->status = status;
    message->profileHandle = chpSeekerInst->chpSeekerSrvcHndl;
    message->tdsSrvcHandle = chpSeekerInst->tdsSrvcHndl;

    ChpSeekerMessageSend(chpSeekerInst->appTask, message);
}

/***************************************************************************/
void ChpSeekerInitReq(AppTask appTask,
                      ChpSeekerInitData *clientInitParams,
                      ChpSeekerHandles *deviceData)
{
    CHP *chpSeekerInst = NULL;
    ChpSeekerProfileHandle profileHndl = 0;
    ProfileHandleListElm_t *elem = NULL;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        CHP_PANIC("Application Task NULL\n");
    }

    elem = ADD_CHP_SERVICE_HANDLE(chpSeekerMain->profileHandleList);
    profileHndl = ADD_CHP_INST(chpSeekerInst);
    elem->profile_handle = profileHndl;

    if (profileHndl)
    {
        /* Reset all the service library memory */
        memset(chpSeekerInst, 0, sizeof(CHP));

        /* Set up library handler for external messages */
        chpSeekerInst->libTask = CSR_BT_CHP_SEEKER_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        chpSeekerInst->appTask = appTask;

        chpSeekerInst->cid = clientInitParams->cid;

        chpSeekerInst->chpSeekerSrvcHndl = profileHndl;

        chpSeekerSendInitCfm(chpSeekerInst, CHP_SEEKER_STATUS_IN_PROGRESS);

        if(deviceData)
        {
            GattTdsClientInitData initData;

            initData.cid = chpSeekerInst->cid;
            initData.startHandle = deviceData->tdsHandle[0].startHandle;
            initData.endHandle = deviceData->tdsHandle[0].endHandle;

            GattTdsClientInitReq(chpSeekerInst->libTask,
                                 &initData,
                                 &(deviceData->tdsHandle[0]));
        }
        else
        {
            GattSdSrvcId srvcIds = GATT_SD_TDS_SRVC;

            /* Find handle value range for the TDS from GATT SD */
            GattServiceDiscoveryFindServiceRange(CSR_BT_CHP_SEEKER_IFACEQUEUE, chpSeekerInst->cid, srvcIds);
        }
    }
    else
    {
        CHP_PANIC("Memory allocation of CHP Seeker Profile instance failed!\n");
    }
}


/****************************************************************************/
void chpSeekerHandleTdsClientInitResp(CHP *chpSeekerInst,
                                      const GattTdsClientInitCfm * message)
{
    if(message->status == GATT_TDS_CLIENT_STATUS_SUCCESS)
    {
        /* Store TDS handle instance */
        chpSeekerInst->tdsSrvcHndl = message->srvcHndl;

        chpSeekerSendInitCfm(chpSeekerInst, CHP_SEEKER_STATUS_SUCCESS);

    }
    else
    {
        chpSeekerSendInitCfm(chpSeekerInst, CHP_SEEKER_STATUS_OPERATION_FAILED);
    }
}

static void initProfileHandleList(CsrCmnListElm_t *elem)
{
    ProfileHandleListElm_t *cElem = (ProfileHandleListElm_t *) elem;

    cElem->profile_handle = 0;
}

void chpSeekerInit(void **gash)
{
    chpSeekerMain = CsrPmemAlloc(sizeof(*chpSeekerMain));
    *gash = chpSeekerMain;


    /* Register logging */
    CSR_LOG_TEXT_REGISTER(&CsrBtChpsLto, "CHP Seeker", 0, NULL);

    if (chpSeekerMain)
        CsrCmnListInit(&chpSeekerMain->profileHandleList, 0, initProfileHandleList, NULL);

    CsrBtGattRegisterReqSend(CSR_BT_CHP_SEEKER_IFACEQUEUE,
                             0);
}

ChpSeekerMainInst *chpSeekerGetMainInstance(void)
{
    return chpSeekerMain;
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************/
void chpSeekerDeInit(void **gash)
{
    CsrCmnListDeinit(&chpSeekerMain->profileHandleList);
    CsrPmemFree(chpSeekerMain);
}
#endif
