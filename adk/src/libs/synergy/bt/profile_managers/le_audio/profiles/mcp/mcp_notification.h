/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef MCP_NOTIFICATION_H_
#define MCP_NOTIFICATION_H_

#include "mcp_private.h"

void mcpHandleMcsNtfCfm(MCP *mcpInst,
                        const GattMcsClientNtfCfm *msg);

void mcpHandleMcsNtfInd(MCP *mcpInst,
                        const GattMcsClientNtfInd *msg);

#endif
