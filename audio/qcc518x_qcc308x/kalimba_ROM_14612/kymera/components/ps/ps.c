/****************************************************************************
 * Copyright (c) 2014 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file ps.c
 * \ingroup ps
 *
 */

/****************************************************************************
Include Files
*/
#include "ps.h"
#include "ps_msg.h"
#include "sched_oxygen/sched_oxygen.h"
#include "pmalloc/pl_malloc.h"
#include "string.h"

#define TASK_ID PS_SAR_TASK_QUEUE_ID

/****************************************************************************
Private Function Definitions
*/
static void ps_put_message(void *instance_data,
                           PS_MSG_TYPE messageInt, 
                           PS_MSG_DATA *pMessageBody)
{
    tRoutingInfo routing;

    /* Build the routing info, destination is not used in this case. */
    routing.src_id = (unsigned)(uintptr_t) instance_data;
    routing.dest_id = 0;

    /* No point of checking the result as the function will panic. */
    (void) put_message_with_routing(TASK_ID,
                                    messageInt,
                                    pMessageBody,
                                    &routing);
}


/****************************************************************************
Public Function Definitions
*/

void ps_entry_read(void* instance_data,
                   PS_KEY_TYPE key,
                   PERSISTENCE_RANK rank,
                   PS_READ_CALLBACK cback)
{
    PS_MSG_DATA* ps_msg;

    if (cback == NULL)
    {
        return;
    }

    /* A key value of 0 is reserved for exclusive use
       by the application framework. */
    if (key == 0)
    {
        cback(instance_data,
              key,
              rank,
              0,
              NULL,
              STATUS_INVALID_CMD_PARAMS,
              0);
        return;
    }

    /* Construct message to PS handler. */
    ps_msg = xpnew(PS_MSG_DATA);
    if (ps_msg == NULL)
    {
        cback(instance_data, key, rank, 0, NULL, STATUS_CMD_FAILED, 0);
        return;
    }

    /* There is no PS data in this message. */
    ps_msg->data_length = 0;
    ps_msg->callback.read = cback;
    ps_msg->key = key;
    ps_msg->rank = rank;

    /* Drop message on the platform-specific PS handler queue. */
    ps_put_message(instance_data, PS_MSG_READ_REQ, ps_msg);
}

void ps_entry_write(void* instance_data,
                    PS_KEY_TYPE key,
                    PERSISTENCE_RANK rank,
                    uint16 length,
                    uint16* data,
                    PS_WRITE_CALLBACK cback)
{
    PS_MSG_DATA* ps_msg;

    if (cback == NULL)
    {
        return;
    }

    /* A key value of 0 is reserved for exclusive use
       by the application framework. */
    if (key == 0)
    {
        cback(instance_data, key, rank, STATUS_INVALID_CMD_PARAMS, 0);
        return;
    }

    /* Construct message to PS handler, decoupling data from the caller */
    ps_msg = xpmalloc(sizeof(PS_MSG_DATA) + length*sizeof(uint16));
    if (ps_msg == NULL)
    {
        cback(instance_data, key, rank, STATUS_CMD_FAILED, 0);
        return;
    }

    ps_msg->data_length = length;
    ps_msg->callback.write = cback;
    ps_msg->key = key;
    ps_msg->rank = rank;
    memcpy(ps_msg->data, data, length*sizeof(uint16));

    /* Drop message on the platform-specific PS handler queue. */
    ps_put_message(instance_data, PS_MSG_WRITE_REQ, ps_msg);
}

void ps_entry_delete(void* instance_data,
                     PS_KEY_TYPE key,
                     PS_ENTRY_DELETE_CALLBACK cback)
{
    PS_MSG_DATA* ps_msg;

    if (cback == NULL)
    {
        return;
    }

    /* A key value of 0 is reserved for exclusive use
       by the application framework. */
    if (key == 0)
    {
        cback(instance_data, key, STATUS_INVALID_CMD_PARAMS, 0);
        return;
    }

    /* Construct message to PS handler. */
    ps_msg = xpnew(PS_MSG_DATA);
    if (ps_msg == NULL)
    {
        cback(instance_data, key, STATUS_CMD_FAILED, 0);
        return;
    }

    /* There is no PS data in this message. */
    ps_msg->data_length = 0;
    ps_msg->callback.entry_del = cback;
    ps_msg->key = key;
    ps_msg->rank = PERSIST_ANY;

    /* Drop message on the platform-specific PS handler queue. */
    ps_put_message(instance_data, PS_MSG_ENTRY_DELETE_REQ, ps_msg);
}

void ps_delete(void* instance_data,
               PERSISTENCE_RANK rank,
               PS_DELETE_CALLBACK cback)
{
    PS_MSG_DATA* ps_msg;

    if (cback == NULL)
    {
        return;
    }

    /* Construct message to PS handler. */
    ps_msg = xpnew(PS_MSG_DATA);
    if (ps_msg == NULL)
    {
        cback(instance_data, rank, STATUS_CMD_FAILED, 0);
        return;
    }

    /* There is no PS data in this message. */
    ps_msg->data_length = 0;
    ps_msg->callback.del = cback;
    ps_msg->key = 0;
    ps_msg->rank = rank;

    /* Drop message on the platform-specific PS handler queue. */
    ps_put_message(instance_data, PS_MSG_DELETE_REQ, ps_msg);
}
