/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <stdlib.h>
#include "mcp_debug.h"
#include "mcp_private.h"
#include "mcp_notification.h"
#include "mcp_common.h"

/***************************************************************************/
static void mcpSendRegisterForNtfCfm(MCP *mcpInst,
                                     ServiceHandle srvc_hdnl,
                                     McpStatus status,
                                     McpMessageId id)
{
    McpNtfCfm *message = CsrPmemAlloc(sizeof(*message));

    message->id = id;
    message->prflHndl = mcpInst->mcpSrvcHndl;
    message->status = status;
    message->srvcHndl = srvc_hdnl;

    McpMessageSend(mcpInst->appTask, message);
}

static void mcpSendRegisterForNtfInd(MCP *mcpInst,
                                     ServiceHandle srvc_hdnl,
                                     MediaPlayerAttribute charac,
                                     status_t status,
                                     McpMessageId id)
{
    McpNtfInd *message = CsrPmemAlloc(sizeof(*message));

    message->id = id;
    message->prflHndl = mcpInst->mcpSrvcHndl;
    message->charac = charac;
    message->status = status;
    message->srvcHndl = srvc_hdnl;

    McpMessageSend(mcpInst->appTask, message);
}

/****************************************************************************/
void McpRegisterForNotificationReq(McpProfileHandle profileHandle,
                                   ServiceHandle mcsHandle,
                                   MediaPlayerAttributeMask characType,
                                   uint32 notifValue)
{
    MCP *mcpInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (mcpInst)
    {
        if (mcsHandle)
        {
            /* We have to apply the operation to a specific MCS instance */
            /* Check if the client handle is a valid one */
            if (mcpIsValidMcsInst(mcpInst, mcsHandle))
            {
                GattMcsClientRegisterForNotificationReq(mcsHandle, characType, notifValue);
            }
            else
            {
                mcpSendRegisterForNtfCfm(mcpInst,
                                         mcsHandle,
                                         MCP_STATUS_FAILED,
                                         MCP_NTF_CFM);
            }
        }
        else
        {
            /* We have to apply the operation to all the MCS instance */
            McpMcsSrvcHndl *ptr = mcpInst->firstMcsSrvcHndl;

            while(ptr)
            {
                GattMcsClientRegisterForNotificationReq(ptr->srvcHndl, characType, notifValue);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        MCP_ERROR("Invalid profile_handle\n");
    }
}

/***************************************************************************/
void mcpHandleMcsNtfInd(MCP *mcpInst,
                        const GattMcsClientNtfInd *msg)
{
    mcpSendRegisterForNtfInd(mcpInst,
                             msg->srvcHndl,
                             msg->charac,
                             msg->status,
                             MCP_NTF_IND);
}

/***************************************************************************/
void mcpHandleMcsNtfCfm(MCP *mcpInst,
                        const GattMcsClientNtfCfm *msg)
{
    mcpSendRegisterForNtfCfm(mcpInst,
                             msg->srvcHndl,
                             MCP_STATUS_SUCCESS,
                             MCP_NTF_CFM);
}
