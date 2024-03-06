/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/
#include "gatt_mcs_client_handover.h"

#include "mcp_common.h"
#include "mcp_debug.h"
#include "mcp_handover.h"


bool McpHandoverVeto(ServiceHandle mcpProfileHandle)
{
    MCP *mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(mcpProfileHandle);

    if (mcpInst)
    {
        if (gattMcsClientHandoverVeto(mcpInst->mcsSrvcHndl))
        {
            MCP_DEBUG("McpHandoverVeto");
            return TRUE;
        }
    }

    return FALSE;
}
