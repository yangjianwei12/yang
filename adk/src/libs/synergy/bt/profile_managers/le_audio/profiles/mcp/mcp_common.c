/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "mcp_common.h"
#include "mcp_debug.h"

/****************************************************************************/
bool mcpIsValidMcsInst(MCP *mcpInst, ServiceHandle srvc_hndl)
{
    McpMcsSrvcHndl *ptr = mcpInst->firstMcsSrvcHndl;

    while(ptr)
    {
        if (ptr->srvcHndl == srvc_hndl)
        {
            return TRUE;
        }

        ptr = ptr->next;
    }

    return FALSE;
}

/****************************************************************************/
static void mcpDestroyReqMcsSrvcHndlList(MCP *mcpInst)
{
    McpMcsSrvcHndl *ptr = mcpInst->firstMcsSrvcHndl;
    McpMcsSrvcHndl *tmp = NULL;

    while(ptr)
    {
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }

    mcpInst->firstMcsSrvcHndl = NULL;
    mcpInst->lastMcsSrvcHndl = NULL;
}

/****************************************************************************/
void mcpDestroyReqAllSrvcHndlList(MCP *mcpInst)
{
    if (mcpInst->firstMcsSrvcHndl && mcpInst->lastMcsSrvcHndl)
    {
        /* There is a list of MCS service handles: we need to destroy it */
        mcpDestroyReqMcsSrvcHndlList(mcpInst);
    }
}

/****************************************************************************/
void mcpAddElementMcsSrvcHndlList(MCP *mcpInst,
                                  ServiceHandle element)
{
    if (!mcpInst->firstMcsSrvcHndl)
    {
        /* it's the first time a MCS service is initialised */
        mcpInst->firstMcsSrvcHndl = CsrPmemAlloc(sizeof(McpMcsSrvcHndl));

        mcpInst->firstMcsSrvcHndl->srvcHndl = element;
        mcpInst->firstMcsSrvcHndl->next = NULL;

        mcpInst->lastMcsSrvcHndl = mcpInst->firstMcsSrvcHndl;
    }
    else
    {
        /* There are already other initialised MCS instances */
        mcpInst->lastMcsSrvcHndl->next = CsrPmemAlloc(sizeof(McpMcsSrvcHndl));

        mcpInst->lastMcsSrvcHndl->next->srvcHndl = element;
        mcpInst->lastMcsSrvcHndl->next->next = NULL;

        mcpInst->lastMcsSrvcHndl = mcpInst->lastMcsSrvcHndl->next;
    }

}

GattMcsClientDeviceData *McpGetMediaPlayerAttributeHandles(McpProfileHandle profileHandle,
                                                           ServiceHandle mcsHandle)
{
    MCP *mcpInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (mcpInst)
    {
        if (mcsHandle)
        {
            /* Check if the client handle is a valid one */
            if (mcpIsValidMcsInst(mcpInst, mcsHandle))
            {
                return GattMcsClientGetHandlesReq(mcsHandle);
            }
            return NULL;
        }
        return NULL;
    }
    else
    { /* In case debug lib is not defined, the following macro won't cause panic so a return would be
         required for the function */
        MCP_ERROR("Invalid profile_handle\n");
        return NULL;
    }
}
