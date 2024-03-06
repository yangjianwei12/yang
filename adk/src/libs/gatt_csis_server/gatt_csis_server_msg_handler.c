/* Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include <vm.h>

#include "gatt_csis_server_private.h"
#include "gatt_csis_server_access.h"
#include "gatt_csis_server_msg_handler.h"
#include "gatt_csis_server_lock_management.h"



static void csisServerInternalMsgHandler(Task task, MessageId id, Message message)
{
    GCSISS_T *csis_server = (GCSISS_T*)task;

    switch (id)
    {
        case CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER:
        {
            GATT_CSIS_SERVER_DEBUG_INFO(("CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER\n"));

            csisServerHandleLockTimerExpiry(csis_server,
                 ((const CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER_T*)message)->cid);
        }
        break;

        default:
        {
            GATT_CSIS_SERVER_DEBUG_INFO(("Unhandled CSIS server internal msg [0x%x]\n", id));
        }
        break;
    }
}

/****************************************************************************/
void csisServerMsgHandler(Task task, MessageId id, Message msg)
{
    GCSISS_T *csis_server = (GCSISS_T*)task;

    /* Check message is internal Message */
     if((id > CSIS_SERVER_INTERNAL_MSG_BASE) && (id < CSIS_SERVER_INTERNAL_MSG_TOP))
     {
         csisServerInternalMsgHandler(task,id,msg);
         return;
     }
     else if(id >= CL_MESSAGE_BASE && id < CL_MESSAGE_TOP)
     {
         switch (id)
         {
             case CL_SM_SIRK_OPERATION_CFM:
                GATT_CSIS_SERVER_DEBUG_INFO(("CSIS: Sirk Encrpytion/Decryption 0x%x \n",id));

                csisServerhandleSirkOperation(csis_server, (const CL_SM_SIRK_OPERATION_CFM_T *) msg);
             break;
         }
         return;
     }

    GATT_CSIS_SERVER_DEBUG_INFO(("CSIS: GATT Manager Server Msg 0x%x \n",id));

    switch (id)
    {
        case GATT_MANAGER_SERVER_ACCESS_IND:
        {
            /* Read/write access to characteristic */
            handleCsisServerAccessInd(csis_server, (GATT_MANAGER_SERVER_ACCESS_IND_T *)msg);
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
            GATT_CSIS_SERVER_DEBUG_INFO(("CSIS: GATT Manager Server Msg 0x%x not handled\n",id));
        }
        break;
    }
}

