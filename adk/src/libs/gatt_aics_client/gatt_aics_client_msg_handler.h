/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_AICS_CLIENT_MSG_HANDLER_H_
#define GATT_AICS_CLIENT_MSG_HANDLER_H_

#include <csrtypes.h>
#include <message.h>

/***************************************************************************
NAME
    gattAicsClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void gattAicsClientMsgHandler(Task task, MessageId id, Message msg);

#endif
