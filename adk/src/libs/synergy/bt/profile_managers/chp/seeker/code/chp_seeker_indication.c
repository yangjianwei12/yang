/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/


#include "chp_seeker.h"
#include "chp_seeker_common.h"
#include "chp_seeker_debug.h"
#include "chp_seeker_indication.h"

/****************************************************************************/
void chpSeekerHandleTdsClientIndResp(CHP* chpSeekerInst,
                                     const GattTdsClientIndicationCfm *cfm)
{
    void* msg = NULL;
    ChpSeekerRegForIndicationCfm* message = CsrPmemAlloc(sizeof(*message));

    CSR_UNUSED(cfm);

    message->id = CHP_SEEKER_REGISTER_FOR_TDS_CONTROL_POINT_INDICATION_CFM;
    message->profileHandle = chpSeekerInst->chpSeekerSrvcHndl;
    message->status   = CHP_SEEKER_STATUS_SUCCESS;

    msg = (void*)message;

    ChpSeekerMessageSend(chpSeekerInst->appTask, msg);
}

/****************************************************************************/
void chpSeekerHandleTdsClientControlPointInd(CHP* chpSeekerInst,
                                             const GattTdsClientTdsCPAttributeInd *ind)
{
    void* msg = NULL;
    ChpSeekerActivateTransportRspInd* message = CsrPmemAlloc(sizeof(*message));

    message->id = CHP_SEEKER_ACTIVATE_TRANSPORT_RSP_IND;
    message->profileHandle = chpSeekerInst->chpSeekerSrvcHndl;
    message->status   = ind->value[1] + 1; /* Status begins from 1 in CHP, hence adding by one*/
    message->responseDataLen = (ind->sizeValue - 2);
    message->responseData = (ind->value + (sizeof(ind->value[0]) * 2));

    msg = (void*)message;
    ChpSeekerMessageSend(chpSeekerInst->appTask, msg);

    /* Stop Control Point Timer */
    stopControlPointTimer();
}

void ChpSeekerRegForIndicationReq (ChpSeekerProfileHandle profileHandle,
                                   bool indicationsEnable)
{
    CHP *chpSeekerInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);
    uint32 enable = indicationsEnable ? 1 : 0;

    if (chpSeekerInst)
    {
        GattTdsClientRegisterForIndicationReq(chpSeekerInst->tdsSrvcHndl, enable);
    }
    else
    {
        CHP_ERROR("Invalid profile_handle\n");
    }    
}