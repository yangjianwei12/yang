/* Copyright (c) 2014 - 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_DEVICE_INFO_SERVER_MSG_HANDLER_H_
#define GATT_DEVICE_INFO_SERVER_MSG_HANDLER_H_

#include "gatt_device_info_server.h"

/***************************************************************************
NAME
    deviceInfoServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void deviceInfoServerMsgHandler(void* task, MsgId id, Msg msg);


#endif

