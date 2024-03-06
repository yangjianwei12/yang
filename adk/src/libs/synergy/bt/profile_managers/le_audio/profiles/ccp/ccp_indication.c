/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "ccp.h"
#include "ccp_indication.h"

/****************************************************************************/
void ccpHandleTbsProviderNameInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientProviderNameInd *ind)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpProviderNameInd,
                              ind->providerNameSize);

    message->id = CCP_PROVIDER_NAME_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->providerNameSize = ind->providerNameSize;

    memcpy(message->providerName, ind->providerName, ind->providerNameSize);

    CcpMessageSend(ccp_inst->appTask, message);
}


/****************************************************************************/
void ccpHandleTbsBearerTechnologyInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientBearerTechnologyInd *ind)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpBearerTechnologyInd,
                              ind->bearerTechSize);

    message->id = CCP_BEARER_TECHNOLOGY_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->bearerTechSize = ind->bearerTechSize;

    memcpy(message->bearerTech, ind->bearerTech, ind->bearerTechSize);

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsSignalStrengthInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientSignalStrengthInd *ind)
{
    MAKE_CCP_MESSAGE(CcpSignalStrengthInd);

    message->id = CCP_SIGNAL_STRENGTH_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->signalStrength = ind->signalStrength;

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsCurrentCallsInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientCurrentCallsInd *ind)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpCurrentCallsListInd,
                              ind->currentCallsListSize);

    message->id = CCP_CURRENT_CALLS_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->currentCallsListSize = ind->currentCallsListSize;

    memcpy(message->currentCallsList, ind->currentCallsList, ind->currentCallsListSize);

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsFlagsInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientFlagsInd *ind)
{
    MAKE_CCP_MESSAGE(CcpFlagsInd);

    message->id = CCP_FLAGS_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->flags = ind->flags;

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsIncomingCallTargetBearerUriInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientIncomingCallTargetBearerUriInd *ind)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpIncomingCallTargetBearerUriInd,
                              ind->uriSize);

    message->id = CCP_INCOMING_CALL_TARGET_BEARER_URI_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->uriSize = ind->uriSize;
    message->callId = ind->callId;

    memcpy(message->uri, ind->uri, ind->uriSize);

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsCallStateInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientCallStateInd *ind)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpCallStateInd,
                              ind->callStateListSize * sizeof(TbsCallState));

    message->id = CCP_CALL_STATE_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->callStateListSize = ind->callStateListSize;

    memcpy(message->callStateList, ind->callStateList, ind->callStateListSize * sizeof(TbsCallState));

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsCallControlPointInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientCallControlPointInd *ind)
{
    MAKE_CCP_MESSAGE(CcpCallControlPointInd);

    message->id = CCP_CALL_CONTROL_POINT_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->callId = ind->callId;
    message->resultCode = ind->resultCode;
    message->opcode = ind->opcode;
    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsTerminationReasonInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientTerminationReasonInd *ind)
{
    MAKE_CCP_MESSAGE(CcpTerminationReasonInd);

    message->id = CCP_TERMINATION_REASON_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->reasonCode = ind->reasonCode;
    message->callId = ind->callId;

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsIncomingCallInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientIncomingCallInd *ind)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpIncomingCallInd,
                              ind->uriSize);

    message->id = CCP_INCOMING_CALL_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->uriSize = ind->uriSize;
    message->callId = ind->callId;

    memcpy(message->uri, ind->uri, ind->uriSize);

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsCallFriendlyNameInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientCallFriendlyNameInd *ind)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpCallFriendlyNameInd,
                              ind->friendlyNameSize);

    message->id = CCP_CALL_FRIENDLY_NAME_IND;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->friendlyNameSize = ind->friendlyNameSize;
    message->callId = ind->callId;

    memcpy(message->friendlyName, ind->friendlyName, ind->friendlyNameSize);

    CcpMessageSend(ccp_inst->appTask, message);
}

