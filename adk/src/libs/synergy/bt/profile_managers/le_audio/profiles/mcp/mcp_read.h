/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef MCP_READ_H_
#define MCP_READ_H_

#include "mcp_private.h"

void mcpHandleReadCharacCfm(MCP *mcpInst,
                            const GattMcsClientGetMediaPlayerAttributeCfm *msg);

#endif
