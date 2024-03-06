/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_PACS_SERVER_MSG_HANDLER_H_
#define GATT_PACS_SERVER_MSG_HANDLER_H_

/***************************************************************************
NAME
    pacsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void pacsServerMsgHandler(Task task, MessageId id, Message msg);

#endif

