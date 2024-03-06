/* Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef MCP_INDICATION_H_
#define MCP_INDICATION_H_

#include "mcp_private.h"

void mcpHandleCharacIndCfm(MCP *mcpInst,
                           const GattMcsClientMediaPlayerAttributeInd *msg);

#endif
