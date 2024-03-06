/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_TELEPHONE_BEARER_SERVER_MSG_HANDLER_H_
#define GATT_TELEPHONE_BEARER_SERVER_MSG_HANDLER_H_

#include "gatt_telephone_bearer_server_private.h"

/***************************************************************************
NAME
    gattTbsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void gattTbsServerMsgHandler(void* task, MsgId id, Msg msg);


#endif /* GATT_TELEPHONE_BEARER_SERVER_MSG_HANDLER_H_ */

