/* Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_CSIS_SERVER_MSG_HANDLER_H_
#define GATT_CSIS_SERVER_MSG_HANDLER_H_

#include "gatt_csis_server_private.h"

/***************************************************************************
NAME
    csisServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void csisServerMsgHandler(Task task, MessageId id, Message msg);
#endif

