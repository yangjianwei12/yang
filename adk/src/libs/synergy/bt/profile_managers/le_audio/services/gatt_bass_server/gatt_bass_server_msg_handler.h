/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_BASS_SERVER_MSG_HANDLER_H_
#define GATT_BASS_SERVER_MSG_HANDLER_H_

#include "csr_bt_tasks.h"
/***************************************************************************
NAME
    bassServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void bassServerMsgHandler(void* task, MsgId id, Msg payload);


#endif
