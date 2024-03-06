/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_BASS_SERVER_MSG_HANDLER_H_
#define GATT_BASS_SERVER_MSG_HANDLER_H_


/***************************************************************************
NAME
    bassServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void bassServerMsgHandler(Task task, MessageId id, Message payload);


#endif
