/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <gatt_telephone_bearer_client.h>

#include "ccp.h"
#include "ccp_debug.h"
#include "ccp_private.h"
#include "ccp_common.h"
#include "ccp_read.h"
#include "ccp_write.h"
#include "ccp_init.h"


/*******************************************************************************/
void CcpReadProviderNameRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadProviderNameRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadBearerUciRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadBearerUciRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadBearerTechnologyRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadBearerTechnologyRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadBearerUriRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadBearerUriRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadSignalStrengthRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadSignalStrengthRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadSignalStrengthIntervalRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadSignalStrengthIntervalRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadCurrentCallsRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadCurrentCallsRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadContentControlIdRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadContentControlIdRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadStatusAndFeatureFlagsRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadStatusAndFeatureFlagsRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadIncomingTargetBearerUriRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadIncomingTargetBearerUriRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadCallStateRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadCallStateRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadIncomingCallRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadIncomingCallRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadCallFriendlyNameRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadCallFriendlyNameRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/*******************************************************************************/
void CcpReadCallControlPointOptionalOpcodesRequest(const CcpProfileHandle profileHandle)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientReadCallControlPointOptionalOpcodesRequest(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/* Read cfm messages */
/****************************************************************************/
void ccpHandleTbsReadProviderNameCfm(CCP *ccp_inst,
                                    const GattTelephoneBearerClientReadProviderNameCfm *msg)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpReadProviderNameCfm, msg->providerNameSize);

    message->id = CCP_READ_PROVIDER_NAME_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->providerNameSize = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->providerNameSize = msg->providerNameSize;
        memmove(message->providerName, msg->providerName, msg->providerNameSize);
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadBearerUciCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadBearerUciCfm *msg)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpReadBearerUciCfm, msg->bearerUciSize);

    message->id = CCP_READ_BEARER_UCI_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->bearerUciSize = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->bearerUciSize = msg->bearerUciSize;
        memmove(message->bearerUci, msg->bearerUci, msg->bearerUciSize);
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadBearerTechnologyCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadBearerTechnologyCfm *msg)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpReadBearerTechnologyCfm, msg->bearerTechSize);

    message->id = CCP_READ_BEARER_TECHNOLOGY_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->bearerTechSize = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->bearerTechSize = msg->bearerTechSize;
        memmove(message->bearerTech, msg->bearerTech, msg->bearerTechSize);
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadBearerUriSchemesCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm *msg)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpReadBearerUriSchemesSupportedListCfm, msg->uriListSize);

    message->id = CCP_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->uriListSize = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->uriListSize = msg->uriListSize;
        memmove(message->uriList, msg->uriList, msg->uriListSize);
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadSignalStrengthCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadSignalStrengthCfm *msg)
{
    MAKE_CCP_MESSAGE(CcpReadSignalStrengthCfm);

    message->id = CCP_READ_SIGNAL_STRENGTH_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->signalStrength = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->signalStrength = msg->signalStrength;
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadSignalStrengthIntervalCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadSignalStrengthIntervalCfm *msg)
{
    MAKE_CCP_MESSAGE(CcpReadSignalStrengthIntervalCfm);

    message->id = CCP_READ_SIGNAL_STRENGTH_INTERVAL_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->interval = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->interval = msg->interval;
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadCurrentCallsListCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadCurrentCallsListCfm *msg)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpReadCurrentCallsListCfm, msg->currentCallsListSize);

    message->id = CCP_READ_CURRENT_CALLS_LIST_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->currentCallsListSize = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->currentCallsListSize = msg->currentCallsListSize;
        memmove(message->currentCallsList, msg->currentCallsList, msg->currentCallsListSize);
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadContentControlIdCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadContentControlIdCfm *msg)
{
    MAKE_CCP_MESSAGE(CcpReadContentControlIdCfm);

    message->id = CCP_READ_CONTENT_CONTROL_ID_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->contentControlId = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->contentControlId = msg->contentControlId;
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadFlagsCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadFeatureAndStatusFlagsCfm *msg)
{
    MAKE_CCP_MESSAGE(CcpReadFlagsCfm);

    message->id = CCP_READ_FEATURE_AND_STATUS_FLAGS_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->flags = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->flags = msg->flags;
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadIncomingCallTargetBearerUriCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm *msg)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpReadIncomingCallTargetBearerUriCfm, msg->uriSize);

    message->id = CCP_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;
    message->uriSize = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {           
        message->callId = msg->callId;
        message->uriSize = msg->uriSize;
        memmove(message->uri, msg->uri, msg->uriSize);
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadCallStateCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientMsgReadCallStateCfm *msg)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpReadCallStateCfm,
                              msg->callStateListSize * sizeof(TbsCallState));

    message->id = CCP_READ_CALL_STATE_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;
    message->callStateListSize = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->callStateListSize = msg->callStateListSize;
        memmove(message->callStateList, msg->callStateList, msg->callStateListSize * sizeof(TbsCallState));
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadIncomingCallCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientMsgReadIncomingCallCfm *msg)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpReadIncomingCallCfm, msg->callUriSize);

    message->id = CCP_READ_INCOMING_CALL_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->callUriSize = 0;
    message->callId = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->callId = msg->callId;
        message->callUriSize = msg->callUriSize;
        memmove(message->callUri, msg->callUri, msg->callUriSize);
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadCallFriendlyNameCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientMsgReadCallFriendlyNameCfm *msg)
{
    MAKE_CCP_MESSAGE_WITH_LEN(CcpReadCallFriendlyNameCfm, msg->friendlyNameSize);

    message->id = CCP_READ_CALL_FRIENDLY_NAME_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->friendlyNameSize = 0;
    message->callId = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->callId = msg->callId;
        message->friendlyNameSize = msg->friendlyNameSize;
        memmove(message->friendlyName, msg->friendlyName, msg->friendlyNameSize);
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/****************************************************************************/
void ccpHandleTbsReadOptionalOpcodesCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadOptionalOpcodesCfm *msg)
{
    MAKE_CCP_MESSAGE(CcpReadOptionalOpcodesCfm);

    message->id = CCP_READ_CCP_OPTIONAL_OPCODES_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = msg->status;

    message->opcodes = 0;

    if (message->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        message->opcodes = msg->opcodes;
    }

    CcpMessageSend(ccp_inst->appTask, message);
}

/* TODO read ccc cfm messages */
