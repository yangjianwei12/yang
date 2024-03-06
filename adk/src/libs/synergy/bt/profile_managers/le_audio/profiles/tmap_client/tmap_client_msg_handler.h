/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef TMAP_CLIENT_MSG_HANDLER_H_
#define TMAP_CLIENT_MSG_HANDLER_H_


/***************************************************************************
NAME
    tmapClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void tmapClientMsgHandler(Task task, MessageId id, Message msg);

#endif
