/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#include "chp_seeker_debug.h"
#include "chp_seeker_private.h"
#include "chp_seeker_common.h"
#include "chp_seeker_read.h"

#include "gatt_tds_client.h"
/***************************************************************************/
static void chpSeekerSendReadBrEdrHandoverDataCfm(CHP *chpSeekerInst,
                                                  ChpSeekerStatus status,
                                                  uint16 sizeValue,
                                                  uint8* value)
{
    void* msg = NULL;
    ChpSeekerReadBredrHandoverDataCfm* message = CsrPmemAlloc(sizeof(*message));

    message->id = CHP_SEEKER_READ_BREDR_HANDOVER_DATA_CFM;
    message->profileHandle = chpSeekerInst->chpSeekerSrvcHndl;
    message->status   = status;

    if (sizeValue && value)
    {
        message->bredrFeatures = value[0];
        message->bdAddr.lap =  (value[3] << 16) | (value[2] << 8) | value[1];
        message->bdAddr.uap = value[4];
        message->bdAddr.nap = (value[6] << 8) | value[5];
        message->classOfDevice = (value[9] << 16) | (value[8] << 8) | value[7];
    }
    else
    {
        message->bredrFeatures = 0;
        message->bdAddr.lap = 0;
        message->bdAddr.uap = 0;
        message->bdAddr.nap = 0;
        message->classOfDevice = 0;
    }
    msg = (void*)message;

    ChpSeekerMessageSend(chpSeekerInst->appTask, msg);
}


/***************************************************************************/
static void chpSeekerSendReadBrEdrTransportBlockDataCfm(CHP *chpSeekerInst,
                                                        ChpSeekerStatus status,
                                                        uint16 sizeValue,
                                                        uint8* value)
{
    void* msg = NULL;
    ChpSeekerReadBredrTransportBlockDataCfm* message = CsrPmemAlloc(sizeof(*message));

    message->id = CHP_SEEKER_READ_BREDR_TRANSPORT_BLOCK_DATA_CFM;
    message->profileHandle = chpSeekerInst->chpSeekerSrvcHndl;
    message->status   = status;

    if(sizeValue && value)
    {
        message->tdsDataAdType = value[0];
        message->orgId = value[1];
        message->tdsFlags = value[2];
        message->transportDataLength = value[3];
        message->transportData = &value[4];
    }
    else
    {
        message->tdsDataAdType = 0;
        message->orgId = 0;
        message->tdsFlags = 0;
        message->transportDataLength = 0;
        message->transportData = NULL;
    }

    msg = (void*)message;

    ChpSeekerMessageSend(chpSeekerInst->appTask, msg);
}

/*******************************************************************************/
void chpSeekerHandleTdsClientGetTdsAttributeResp(CHP *chpSeekerInst,
                                                 const GattTdsClientGetTdsAttributeCfm *msg)
{
    switch (msg->charac)
    {
        case BREDR_HANDOVER_DATA:
            chpSeekerSendReadBrEdrHandoverDataCfm(chpSeekerInst,
                                                  msg->status,
                                                  msg->sizeValue,
                                                  msg->value);
        break;

        case COMPLETET_TRANSPORT_BLOCK:
            chpSeekerSendReadBrEdrTransportBlockDataCfm(chpSeekerInst,
                                                        msg->status,
                                                        msg->sizeValue,
                                                        msg->value);
        break;
    }
}
/***************************************************************************/
void ChpSeekerReadBredrHandoverDataReq(ChpSeekerProfileHandle profileHandle)
{
    CHP *chpSeekerInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (chpSeekerInst)
    {
        GattTdsClientGetTdsAttribute(chpSeekerInst->tdsSrvcHndl,BREDR_HANDOVER_DATA);
    }
    else
    {
        CHP_ERROR("Invalid profile_handle\n");
    }
}
/***************************************************************************/
void ChpSeekerReadBredrTransportBlockDataReq(ChpSeekerProfileHandle profileHandle)
{
    CHP *chpSeekerInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (chpSeekerInst)
    {
        GattTdsClientGetTdsAttribute(chpSeekerInst->tdsSrvcHndl,COMPLETET_TRANSPORT_BLOCK);
    }
    else
    {
        CHP_ERROR("Invalid profile_handle\n");
    }
}
