/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "mcp.h"
#include "mcp_write.h"
#include "mcp_debug.h"
#include "mcp_private.h"
#include "mcp_common.h"
#include "mcp_init.h"

/***************************************************************************/
static void mcpSendWriteCharacCfm(MCP *mcpInst,
                                 ServiceHandle srvc_hndl,
                                 MediaPlayerAttribute charac,
                                 status_t status)
{
    McpSetMediaPlayerAttributeCfm *message = CsrPmemAlloc(sizeof(*message));

    message->id = MCP_SET_MEDIA_PLAYER_ATTRIBUTE_CFM;
    message->prflHndl = mcpInst->mcpSrvcHndl;
    message->srvcHndl = srvc_hndl;
    message->charac = charac;
    message->status = status;

    McpMessageSend(mcpInst->appTask, message);
}

/*******************************************************************************/
void mcpHandleWriteCharacCfm(MCP *mcpInst,
                             const GattMcsClientSetMediaPlayerAttributeCfm *msg)
{
    mcpSendWriteCharacCfm(mcpInst,
                         msg->srvcHndl,
                         msg->charac,
                         msg->status);
}

/***************************************************************************/
static void mcpSendMediaControlPointCfm(MCP *mcpInst,
                                        ServiceHandle srvc_hndl,
                                        GattMcsOpResult status,
                                        GattMcsOpcode op)
{
    McpSetMediaControlPointCfm *message = CsrPmemAlloc(sizeof(*message));

    message->id = MCP_SET_MEDIA_CONTROL_POINT_CFM;
    message->prflHndl = mcpInst->mcpSrvcHndl;
    message->srvcHndl = srvc_hndl;
    message->status = status;
    message->op = op;

    McpMessageSend(mcpInst->appTask, message);
}

/*******************************************************************************/
void mcpHandleMediaControlPointCfm(MCP *mcpInst,
                                   const GattMcsClientSetMediaControlPointCfm *msg)
{
    mcpSendMediaControlPointCfm(mcpInst,
                                msg->srvcHndl,
                                msg->status,
                                msg->op);
}

/****************************************************************************/
void McpSetMediaPlayerAttribute(McpProfileHandle profileHandle,
                               ServiceHandle mcsHandle,
                               MediaPlayerAttribute charac,
                               uint16 len,
                               uint8 *val)

{
    MCP *mcpInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (mcpInst)
    {
        if (mcsHandle)
        {
            /* Check if the client handle is a valid one */
            if (mcpIsValidMcsInst(mcpInst, mcsHandle))
            {
                GattMcsClientSetMediaPlayerAttribute(mcsHandle,
                                                    charac,
                                                    len,
                                                    val);
            }
            else
            {
                mcpSendWriteCharacCfm(mcpInst,
                                      mcsHandle,
                                      charac,
                                      CSR_BT_GATT_RESULT_INTERNAL_ERROR);
            }
        }
        else
        {
            McpMcsSrvcHndl *ptr = mcpInst->firstMcsSrvcHndl;

            while(ptr)
            {
                GattMcsClientSetMediaPlayerAttribute(ptr->srvcHndl,
                                                     charac,
                                                     len,
                                                     val);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        MCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void McpSetMediaControlPoint(McpProfileHandle profileHandle,
                             ServiceHandle mcsHandle,
                             GattMcsOpcode op,
                             int32 val)

{
    MCP *mcpInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (mcpInst)
    {
        if (mcsHandle)
        {
            /* Check if the client handle is a valid one */
            if (mcpIsValidMcsInst(mcpInst, mcsHandle))
            {
                GattMcsClientSetMediaControlPoint(mcsHandle,
                                                  op,
                                                  val);
            }
            else
            {
                mcpSendMediaControlPointCfm(mcpInst,
                                            mcsHandle,
                                            CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                            op);
            }
        }
        else
        {
            McpMcsSrvcHndl *ptr = mcpInst->firstMcsSrvcHndl;

            while(ptr)
            {
                GattMcsClientSetMediaControlPoint(ptr->srvcHndl, op, val);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        MCP_DEBUG("Invalid profile_handle\n");
    }
}

