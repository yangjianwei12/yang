/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef MCP_INIT_H_
#define MCP_INIT_H_

#include "mcp_private.h"

/***************************************************************************
NAME
    mcpSendInitCfm
    
DESCRIPTION
    Send a MCP_INIT_CFM message to the application.
*/
void mcpSendInitCfm(MCP * mcpInst, McpStatus status);


/***************************************************************************
NAME
    mcpHandleMcsClientInitResp

DESCRIPTION
    Handle the GATT_MCS_CLIENT_INIT_CFM message.
*/
void mcpHandleMcsClientInitResp(MCP *mcpInst,
                                const GattMcsClientInitCfm * message);

#endif
