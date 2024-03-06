/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */


#include "gatt_ascs_server_private.h"
#include "gatt_ascs_server_msg_handler.h"
#include "gatt_ascs_server_access.h"

#include <stdio.h>
#include <stdlib.h>

/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_CLIENT_CONFIG_NUM_OCTETS   2


/***************************************************************************
NAME
    sendAscsServerAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void sendAscsServerAccessRsp(Task task,
                                    uint16 cid,
                                    uint16 handle,
                                    uint16 result,
                                    uint16 size_value,
                                    const uint8 *value)
{
    if (!GattManagerServerAccessResponse(task, cid, handle, result, size_value, value))
    {
        /* The GATT Manager should always know how to send this response */
        GATT_ASCS_SERVER_DEBUG_PANIC(("Couldn't send GATT access response\n"));
    }
}

/***************************************************************************
NAME
    sendAscsServerAccessErrorRsp

DESCRIPTION
    Send an error access response to the GATT Manager library.
*
static void sendAscsServerAccessErrorRsp(const GASCS_T *ascs, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind, uint16 error)
{
    sendAscsServerAccessRsp((Task)&ascs->lib_task, access_ind->cid, access_ind->handle, error, 0, NULL);
}*/

/****************************************************************************/
void ascsServerMsgHandler(Task task, MessageId id, Message msg)
{
    GattAscsServer *ascs = (GattAscsServer*)task;

    GATT_ASCS_SERVER_DEBUG_INFO(("ASCS: GATT Manager Server Msg 0x%x \n",id));

    switch (id)
    {
        case GATT_MANAGER_SERVER_ACCESS_IND:
        {
            /* Read/write access to characteristic */
            handleAscsAccess(ascs, (const GATT_MANAGER_SERVER_ACCESS_IND_T *)msg);
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
            GATT_ASCS_SERVER_DEBUG_INFO(("ASCS: GATT Manager Server Msg 0x%x not handled\n",id));
        }
        break;
    }
}

