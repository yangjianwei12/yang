/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VCS_CLIENT_MSG_HANDLER_H_
#define GATT_VCS_CLIENT_MSG_HANDLER_H_

#include <csrtypes.h>
#include <message.h>

/***************************************************************************
NAME
    gattVcsClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void gattVcsClientMsgHandler(AppTask task, MsgId id, Msg msg);

#endif
