/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_VCS_SERVER_MSG_HANDLER_H_
#define GATT_VCS_SERVER_MSG_HANDLER_H_

#include "csr_bt_tasks.h"

/***************************************************************************
NAME
    vcsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void vcsServerMsgHandler(void* task, MsgId id, Msg msg);

#endif /* GATT_VCS_SERVER_MSG_HANDLER_H_ */

