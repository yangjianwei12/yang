/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef MCP_WRITE_H_
#define MCP_WRITE_H_

#include "mcp_private.h"

void mcpHandleWriteCharacCfm(MCP *mcpInst,
                             const GattMcsClientSetMediaPlayerAttributeCfm *msg);

void mcpHandleMediaControlPointCfm(MCP *mcpInst,
                                   const GattMcsClientSetMediaControlPointCfm *msg);

#endif
