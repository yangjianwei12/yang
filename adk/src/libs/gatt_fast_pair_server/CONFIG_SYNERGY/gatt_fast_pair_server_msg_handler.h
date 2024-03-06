/* Copyright (c) 2018-22 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_FAST_PAIR_SERVER_MSG_HANDLER_H_
#define GATT_FAST_PAIR_SERVER_MSG_HANDLER_H_

#define CLIENT_CONFIG_VALUE_SIZE    (2)
#define MODEL_ID_VALUE_SIZE         (3)


/***************************************************************************
NAME
    fpsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void fpsServerMsgHandler(Task task, MessageId id, Message msg);


#endif /* GATT_FAST_PAIR_SERVER_MSG_HANDLER_H_ */

