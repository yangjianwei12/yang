/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CSIP_MSG_HANDLER_H_
#define CSIP_MSG_HANDLER_H_


/***************************************************************************
NAME
    csipMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void csipMsgHandler(Task task, MessageId id, Message msg);

#endif
