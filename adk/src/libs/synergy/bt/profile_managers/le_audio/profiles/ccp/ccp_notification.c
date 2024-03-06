/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <stdlib.h>

#include <gatt_telephone_bearer_client.h>

#include "ccp.h"
#include "ccp_debug.h"
#include "ccp_private.h"
#include "ccp_notification.h"
#include "ccp_common.h"



/****************************************************************************/
void CcpSetProviderNameNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetProviderNameNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                    notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void CcpSetTechnologyNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetTechnologyNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                    notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void CcpSetSignalStrengthNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetSignalStrengthNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                    notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void CcpSetListCurrentCallsNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetListCurrentCallsNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                    notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void CcpSetFlagsNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetFlagsNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                    notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void CcpSetIncomingCallTargetBearerUriNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetIncomingCallTargetBearerUriNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                    notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void CcpSetCallStateNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetCallStateNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                    notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void CcpSetCallControlPointNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetCallControlPointNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                    notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void CcpSetTerminationReasonNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetTerminationReasonNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                    notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void CcpSetIncomingCallNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetIncomingCallNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                    notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void CcpSetCallFriendlyNameNotificationRequest(const CcpProfileHandle profileHandle, bool notificationsEnable)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        GattTelephoneBearerClientSetCallFriendlyNameNotificationRequest(ccp_inst->tbs_srvc_hdl,
                                                                        notificationsEnable);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/***************************************************************************/
void ccpSendTbsSetNtfCfm(CCP *ccp_inst,
                                status_t status,
                                CcpMessageId id)
{
    /* We will use CcpSetCfm to create the message
     * because the structute of all the set notification confirmations is the same,
     * but we will send the right message using the id parameter.
     */
    MAKE_CCP_MESSAGE(CcpSetCfm);

    message->id = id;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;
    message->status = status;

    CcpMessageSend(ccp_inst->appTask, message);
}





