/* Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef MCP_COMMON_H_
#define MCP_COMMON_H_

#include "mcp_private.h"

/***************************************************************************
NAME
    mcpIsValidMcsInst

DESCRIPTION
    Check if the service handle provided is a valid one for a MCS client instance.
*/
bool mcpIsValidMcsInst(MCP *mcpInst, ServiceHandle srvcHndl);


/***************************************************************************
NAME
    mcpDestroyReqAllSrvcHndlList

DESCRIPTION
    Destroy all the lists of service handles in the profile memory instance.
*/
void mcpDestroyReqAllSrvcHndlList(MCP *mcpInst);


/***************************************************************************
NAME
    mcpAddElementMcsSrvcHndlList

DESCRIPTION
    Add a new element to the list of MCS service handles.
*/
void mcpAddElementMcsSrvcHndlList(MCP *mcpInst, ServiceHandle element);

#endif
