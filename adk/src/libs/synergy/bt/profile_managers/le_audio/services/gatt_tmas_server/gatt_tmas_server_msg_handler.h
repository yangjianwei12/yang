/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_TMAS_SERVER_MSG_HANDLER_H_
#define GATT_TMAS_SERVER_MSG_HANDLER_H_

#include "csr_bt_tasks.h"

/***************************************************************************
NAME
    gattTmasServerHandleGattMsg

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void gattTmasServerHandleGattMsg(void* task, MsgId id, Msg msg);

#endif /* GATT_TMAS_SERVER_MSG_HANDLER_H_ */

