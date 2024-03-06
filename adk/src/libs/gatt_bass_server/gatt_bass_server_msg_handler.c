/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

# include <gatt_manager.h>

#include "gatt_bass_server_msg_handler.h"
#include "gatt_bass_server_access.h"
#include "gatt_bass_server_debug.h"
#include "gatt_bass_server_private.h"

/****************************************************************************/
void bassServerMsgHandler(Task task, MessageId id, Message payload)
{
    GBASSSS *bass_server = (GBASSSS *)task;
    
    switch (id)
    {
        case GATT_MANAGER_SERVER_ACCESS_IND:
        {
            /* Read/write access to characteristic */
            handleBassServerAccess(bass_server, (GATT_MANAGER_SERVER_ACCESS_IND_T *)payload);
        }
        break;

        case GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM:
        {
            /* Library just absorbs confirmation messages */
        }
        break;
        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_BASS_SERVER_DEBUG_PANIC(("GATT Manager Server Msg not handled\n"));
        }
        break;
    }
}

