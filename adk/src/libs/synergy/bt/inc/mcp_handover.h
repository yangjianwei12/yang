/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef MCP_HANDOVER_H_
#define MCP_HANDOVER_H_

#include "service_handle.h"

/***************************************************************************
NAME
    McpHandoverVeto

DESCRIPTION
    Veto the handover of MCP Profile data.

    @return TRUE if the module wishes to veto the handover attempt.
*/

bool McpHandoverVeto(ServiceHandle mcpProfileHandle);

#endif
