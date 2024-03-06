/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_MICS_SERVER_MSG_HANDLER_H_
#define GATT_MICS_SERVER_MSG_HANDLER_H_

#include "gatt_mics_server_private.h"

/***************************************************************************
NAME
    tbsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void micsServerMsgHandler(void* task, MsgId id, Msg msg);


#endif /* GATT_MICS_SERVER_MSG_HANDLER_H_ */

