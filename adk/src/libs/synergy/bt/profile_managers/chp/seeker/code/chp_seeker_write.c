/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#include "chp_seeker_debug.h"
#include "chp_seeker_private.h"
#include "chp_seeker_common.h"
#include "chp_seeker_write.h"

#include "gatt_tds_client.h"
/***************************************************************************/
static void chpSeekerSendActivateTransportCfm(CHP *chpSeekerInst,
                                              ChpSeekerStatus status)
{
    void* msg = NULL;
    ChpSeekerActivateTransportCfm* message = CsrPmemAlloc(sizeof(*message));

    message->id = CHP_SEEKER_ACTIVATE_TRANSPORT_CFM;
    message->profileHandle = chpSeekerInst->chpSeekerSrvcHndl;
    message->status   = status;

    msg = (void*)message;

    ChpSeekerMessageSend(chpSeekerInst->appTask, msg);
}

/*******************************************************************************/
void chpSeekerHandleTdsClientSetTdsCPResp(CHP *chpSeekerInst,
                                          const GattTdsClientSetTdsControlPointCfm *msg)
{
    chpSeekerSendActivateTransportCfm(chpSeekerInst,
                                      msg->status);

    /* Start the timer */
    startControlPointTimer(chpSeekerInst);
}

void ChpSeekerActivateTransportReq(ChpSeekerProfileHandle profileHandle, 
                                    uint8 opCode,
                                    uint8 orgId,
                                    uint16 profileDataLength,
                                    uint8* ltvData)
{
    CHP *chpSeekerInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);
    uint8 *val;

    if (chpSeekerInst)
    {
        val = CsrPmemAlloc(sizeof(orgId) + profileDataLength);
        val[0] = orgId;
        memcpy(&val[1], ltvData, sizeof(uint8)*profileDataLength);
        GattTdsClientSetTdsControlPoint(chpSeekerInst->tdsSrvcHndl, opCode,profileDataLength + 1, val);
    }
    else
    {
        CHP_ERROR("Invalid profile_handle\n");
    }
}
