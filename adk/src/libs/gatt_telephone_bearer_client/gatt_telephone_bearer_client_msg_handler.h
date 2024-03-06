/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_TBS_CLIENT_MSG_HANDLER_H_
#define GATT_TBS_CLIENT_MSG_HANDLER_H_

/***************************************************************************
NAME
    tbsClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void tbsClientMsgHandler(Task task, MessageId id, Message payload);

#endif
