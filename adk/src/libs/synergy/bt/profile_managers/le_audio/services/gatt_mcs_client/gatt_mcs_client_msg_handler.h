/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_MCS_CLIENT_MSG_HANDLER_H_
#define GATT_MCS_CLIENT_MSG_HANDLER_H_

#include <message.h>

/***************************************************************************
NAME
    gattMcsClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void gattMcsClientMsgHandler(Task task, MsgId id, Msg msg);

#endif
