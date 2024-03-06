/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_TDS_CLIENT_MSG_HANDLER_H_
#define GATT_TDS_CLIENT_MSG_HANDLER_H_

#include <message.h>

/***************************************************************************
NAME
    gattTdsClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void gattTdsClientMsgHandler(Task task, MsgId id, Msg msg);

#endif
