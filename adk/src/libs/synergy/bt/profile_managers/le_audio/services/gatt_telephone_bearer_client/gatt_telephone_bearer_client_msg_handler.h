/******************************************************************************
 Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef GATT_TBS_CLIENT_MSG_HANDLER_H_
#define GATT_TBS_CLIENT_MSG_HANDLER_H_

#include <message.h>
/***************************************************************************
NAME
    tbsClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void tbsClientMsgHandler(Task task, MessageId id, Message payload);

#endif
