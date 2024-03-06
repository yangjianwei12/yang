/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include <vm.h>

#include "gatt_telephone_bearer_server_msg_handler.h"
#include "gatt_telephone_bearer_server_access.h"
#include "gatt_telephone_bearer_server_debug.h"
#include "gatt_telephone_bearer_server_private.h"

/******************************************************************************/
void tbsServerMsgHandler(Task task, MessageId id, Message msg)
{
    GTBS_T *telephone_bearer_server = (GTBS_T *)task;

    switch (id)
    {
        case GATT_MANAGER_SERVER_ACCESS_IND:
        {
            tbsHandleAccessIndication(
                    telephone_bearer_server,
                    (GATT_MANAGER_SERVER_ACCESS_IND_T *)msg
                    );
        }
        break;

        case GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM:
        {
            /* Nothing to do */

        }
        break;

        case GATT_TBS_INTERNAL_SIGNAL_STRENGTH_TIMER:
        {
            /* Notify Clients if the latest value has not been notified */
            if(telephone_bearer_server->data.signalStrengthNotified != telephone_bearer_server->data.signalStrength)
            {
                gattTelephoneBearerServerNotifySignalStrength(telephone_bearer_server, NULL);
            }
            else
            {   /* Finished notifying then the reporting flag can be cleared */
                telephone_bearer_server->data.signal_strength_timer_flag = FALSE;
            }
        }
        break;

        default:
        {
            GATT_TELEPHONE_BEARER_SERVER_DEBUG_PANIC((
                        "GTBS: GATT Manager message 0x%04x not handled\n",
                        id
                        ));
            break;
        }
    } /* switch */
}


