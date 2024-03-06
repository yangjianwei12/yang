/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef MCP_MSG_HANDLER_H_
#define MCP_MSG_HANDLER_H_


/***************************************************************************
NAME
    mcpMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void mcpMsgHandler(Task task, MessageId id, Message msg);

#endif
