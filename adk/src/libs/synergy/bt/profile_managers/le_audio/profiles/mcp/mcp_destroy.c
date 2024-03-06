/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "mcp_private.h"
#include "mcp_destroy.h"
#include "mcp_common.h"
#include "mcp_debug.h"

void mcpSendDestroyCfm(MCP * mcpInst, McpStatus status)
{
    McpDestroyCfm *message = CsrPmemAlloc(sizeof(*message));

    message->id = MCP_DESTROY_CFM;
    message->status = status;
    message->prflHndl = mcpInst->mcpSrvcHndl;

    McpMessageSend(mcpInst->appTask, message);
}

/******************************************************************************/
void McpDestroyReq(McpProfileHandle profileHandle)
{
    MCP * mcpInst = ServiceHandleGetInstanceData(profileHandle);

    if (mcpInst)
    {
        /* Send confirmation message with status in progress */
        mcpSendDestroyCfm(mcpInst, MCP_STATUS_IN_PROGRESS);

        if (mcpInst->mcsNum)
        {
            /* If there are MCS Client instances initialised, we have to terminate them */
            McpMcsSrvcHndl *ptr = mcpInst->firstMcsSrvcHndl;
            mcpInst->mcsCounter = mcpInst->mcsNum;

            while(ptr)
            {
                GattMcsClientTerminateReq(ptr->srvcHndl);
                ptr = ptr->next;
            }
        }
        else
        {
            mcpSendDestroyCfm(mcpInst, MCP_STATUS_SUCCESS);
        }
    }
    else
    {
        MCP_ERROR("Invalid profile handle\n");
    }
}

/******************************************************************************/
void mcpSendMcsTerminateCfm(MCP *mcpInst,
                            McpStatus status,
                            GattMcsClientDeviceData handles,
                            bool moreToCome)
{
    McpMcsTerminateCfm *message = CsrPmemAlloc(sizeof(*message));

    message->id = MCP_MCS_TERMINATE_CFM;
    message->status = status;
    message->prflHndl = mcpInst->mcpSrvcHndl;
    message->moreToCome = moreToCome;

    memcpy(&(message->mcsHandle), &handles, sizeof(GattMcsClientDeviceData));

    McpMessageSend(mcpInst->appTask, message);
}


/****************************************************************************/
void mcpHandleMcsClientTerminateResp(MCP *mcpInst,
                                     const GattMcsClientTerminateCfm * message)
{
    if (message->status == GATT_MCS_CLIENT_STATUS_SUCCESS)
    {
        bool moreToCome = TRUE;
        /* A MCS Client instance has been terminated successfully. */
        mcpInst->mcsCounter -= 1;

        if(!mcpInst->mcsCounter)
            moreToCome = FALSE;

        /* Send the MCS characteristic handles to the application */
        mcpSendMcsTerminateCfm(mcpInst,
                               MCP_STATUS_SUCCESS,
                               message->deviceData,
                               moreToCome);

        if (!mcpInst->mcsCounter)
        {
            /* There are no instances of MCS to terminate */
            mcpDestroyProfileInst(mcpInst);
        }
    }
    else
    {
        mcpSendDestroyCfm(mcpInst, MCP_STATUS_FAILED);
    }
}


/****************************************************************************/
void mcpDestroyProfileInst(MCP *mcpInst)
{
    bool res = FALSE;
    AppTask appTask = mcpInst->appTask;

    /* Send the confirmation message */
    McpDestroyCfm *message = CsrPmemAlloc(sizeof(*message));
    McpMainInst *mainInst = mcpGetMainInstance();

    message->id = MCP_DESTROY_CFM;
    message->prflHndl = mcpInst->mcpSrvcHndl;

    /* We can destroy all the list we have in the profile context */
    mcpDestroyReqAllSrvcHndlList(mcpInst);

    /* Remove the profile element from main list */
    if (mainInst)
        MCP_REMOVE_SERVICE_HANDLE(mainInst->profileHandleList, mcpInst->mcpSrvcHndl);

    /* Free the profile instance memory */
    res = FREE_MCP_CLIENT_INST(mcpInst->mcpSrvcHndl);

    if (res)
    {
        message->status = MCP_STATUS_SUCCESS;
    }
    else
    {
        message->status = MCP_STATUS_FAILED;
    }

    McpMessageSend(appTask, message);
}
