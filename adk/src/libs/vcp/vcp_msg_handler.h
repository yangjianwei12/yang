/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef VCP_MSG_HANDLER_H_
#define VCP_MSG_HANDLER_H_

#include <csrtypes.h>
#include <message.h>

/***************************************************************************
NAME
    vcpMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void vcpMsgHandler(Task task, MessageId id, Message msg);

#endif
