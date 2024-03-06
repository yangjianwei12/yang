/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include <vm.h>

#include "gatt_vcs_server_msg_handler.h"
#include "gatt_vcs_server_access.h"
#include "gatt_vcs_server_debug.h"

/******************************************************************************/
void vcsServerMsgHandler(Task task, MessageId id, Message msg)
{
    GVCS *volume_control_server = (GVCS *)task;

    switch (id)
    {
        case GATT_MANAGER_SERVER_ACCESS_IND:
        {
            vcsServerHandleAccessIndication(
                    volume_control_server,
                    (GATT_MANAGER_SERVER_ACCESS_IND_T *)msg
                    );
        }
        break;

        case GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM:
        {
            /* Nothing to do */
        }
        break;

        default:
        {
            GATT_VCS_SERVER_DEBUG_PANIC((
                        "GVCS: GATT Manager message 0x%04x not handled\n",
                        id
                        ));
            break;
        }
    } /* switch */
}


