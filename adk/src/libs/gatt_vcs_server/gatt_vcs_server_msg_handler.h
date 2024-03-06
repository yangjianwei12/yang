/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_VCS_SERVER_MSG_HANDLER_H_
#define GATT_VCS_SERVER_MSG_HANDLER_H_

/***************************************************************************
NAME
    vcsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void vcsServerMsgHandler(Task task, MessageId id, Message msg);


#endif /* GATT_VCS_SERVER_MSG_HANDLER_H_ */

