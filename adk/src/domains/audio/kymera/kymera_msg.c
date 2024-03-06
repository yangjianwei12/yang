/*!
\copyright  Copyright (c) 2022  Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_msg.c
\brief      Kymera message helper
*/

#include "kymera_msg.h"
#include "kymera.h"
#include "kymera_data.h"

void appKymera_MsgRegisteredClients(MessageId id, uint16 info)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    if(theKymera->client_tasks)
    {
        MESSAGE_MAKE(ind, kymera_aanc_event_msg_t);
        ind->info = info;

        TaskList_MessageSendWithSize(theKymera->client_tasks, id, ind, sizeof(ind));
    }
}
