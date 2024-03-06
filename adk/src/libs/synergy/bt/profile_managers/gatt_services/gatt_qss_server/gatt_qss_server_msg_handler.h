/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_QSS_SERVER_MSG_HANDLER_H_
#define GATT_QSS_SERVER_MSG_HANDLER_H_

/***************************************************************************
NAME
    gattQssMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void gattQssMsgHandler(void* task, MsgId id, Msg msg);

#endif /* GATT_QSS_SERVER_MSG_HANDLER_H_ */