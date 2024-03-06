/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include "vsc_main.h"
#include "csr_gsched.h"

static void VscListInit(CsrCmnListElm_t *elem)
{
    VscApplicationInstanceElement *inst = (VscApplicationInstanceElement *) elem;

    inst->handle = VSC_SCHED_QID_INVALID;
}

void VscInitInstData(vscInstanceData_t *vscData)
{
    vscData->vscVar.saveQueue = NULL;
    vscData->vscVar.lockMsg = VSC_QUEUE_LOCK;
    vscData->vscVar.appHandle = VSC_SCHED_QID_INVALID;
    vscData->len = 0;
    vscData->recvMsgP = NULL;
    CsrCmnListInit(&vscData->InstanceList,
                   0,
                   VscListInit,
                   NULL);
}

void VscLockQueue(vscInstanceData_t *vscData)
{
    VscPrim *primPtr = (VscPrim*) vscData->recvMsgP;
    vscData->vscVar.lockMsg = *primPtr;
}

/* Function called to start clearing the queue */
void VscLocalQueueHandler(void)
{
    VscHouseCleaning *prim;
    prim = CsrPmemAlloc(sizeof(VscHouseCleaning));
    prim->type = VSC_HOUSE_CLEANING;
    VSC_PUT_MESSAGE(CSR_BT_VSDM_IFACEQUEUE, prim);
}

/* Unlocks the queue then starts sending pending primitives. */
void VscRestoreQueueHandler(vscInstanceData_t *vscData)
{
    CsrUint16 eventClass;
    void *msg;

    VSC_UNLOCK_QUEUE(vscData)

    if(CsrMessageQueuePop(&vscData->vscVar.saveQueue, &eventClass, &msg))
    {
        SynergyMessageFree(BT_VSDM_PRIM, vscData->recvMsgP);
        vscData->recvMsgP = msg;

        VscProfileProvider(vscData);
    }
}
