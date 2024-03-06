/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_BATTERY_SERVER_MSG_HANDLER_H_
#define GATT_BATTERY_SERVER_MSG_HANDLER_H_

#include "gatt_battery_server.h"

/***************************************************************************
NAME
    batteryServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void batteryServerMsgHandler(void* task, MsgId id, Msg message);


#endif
