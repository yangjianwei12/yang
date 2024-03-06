/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef MCP_DESTROY_H_
#define MCP_DESTROY_H_

#include "mcp_private.h"

/***************************************************************************
NAME
    mcpSendDestroyCfm
    
DESCRIPTION
    Send the MCP_DESTROY_CFM message.
*/
void mcpSendDestroyCfm(MCP * mcpInst, McpStatus status);

/***************************************************************************
NAME
    mcpSendMcsTerminateCfm

DESCRIPTION
    Send the MCP_MCS_TERMINATE_CFM message.
*/
void mcpSendMcsTerminateCfm(MCP *mcpInst,
                            McpStatus status,
                            GattMcsClientDeviceData handles,
                            bool moreToCome);


/***************************************************************************
NAME
    mcpHandleMcsClientTerminateResp

DESCRIPTION
    Handle the GATT_MCS_CLIENT_TERMINATE_CFM message.
*/
void mcpHandleMcsClientTerminateResp(MCP *mcpInst,
                                     const GattMcsClientTerminateCfm * message);

/***************************************************************************
NAME
    mcpDestroyProfileInst

DESCRIPTION
    Destroy the profile memory instance.
*/
void mcpDestroyProfileInst(MCP *mcpInst);

#endif
