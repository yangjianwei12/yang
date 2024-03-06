/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include <vm.h>

#include "gatt_pacs_server_msg_handler.h"
#include "gatt_pacs_server_access.h"
#include "gatt_pacs_server_utils.h"


/****************************************************************************/
void pacsServerMsgHandler(Task task, MessageId id, Message msg)
{
    GPACSS_T *pacs = (GPACSS_T*)task;

    GATT_PACS_SERVER_DEBUG_INFO(("PACS: GATT Manager Server Msg 0x%x \n",id));

    switch (id)
    {
        case GATT_MANAGER_SERVER_ACCESS_IND:
        {
            /* Read/write access to characteristic */
            handlePacsServerAccessInd(pacs, (GATT_MANAGER_SERVER_ACCESS_IND_T *)msg);
        }
        break;
        case GATT_MANAGER_REMOTE_CLIENT_INDICATION_CFM:
        case GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM:
        {
            /* Library just absorbs confirmation messages */
        }
        break;
        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_PACS_SERVER_DEBUG_INFO(("PACS: GATT Manager Server Msg 0x%x not handled\n",id));
        }
        break;
    }
}

