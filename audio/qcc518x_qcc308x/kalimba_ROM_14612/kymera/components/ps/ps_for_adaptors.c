/****************************************************************************
 * Copyright (c) 2014 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file ps_for_adaptors.c
 * \ingroup ps
 *
 */

/****************************************************************************
Include Files
*/
#include <string.h>
#include "ps_for_adaptors.h"
#include "ps/ps_msg.h"
#include "sched_oxygen/sched_oxygen.h"
#include "pmalloc/pl_malloc.h"
#include "kip_msg_prim.h"
#include "ps_router/ps_router.h"
#include "fault/fault.h"
#include "sys_events.h"

static void ps_encapsulate_and_send(KIP_MSG_ID id, unsigned length, void *msg)
{
    PS_ROUTER_MSG *emsg;

    /* Allocated memory is freed in ps_router_handler. */
    emsg = xpnew(PS_ROUTER_MSG);
    if (emsg == NULL)
    {
        pdelete(msg);
        return;
    }

    /* Encapsulate message. */
    emsg->msg_id = id;
    emsg->msg_length = (uint16) length;
    emsg->msg_data = (uint16 *) msg;

    put_message(PS_ROUTER_TASK_QUEUE_ID, id, emsg);
}

/* TODO: Is it possible to get rid of argument cback? */
void ps_signal_shutdown(CONNECTION_LINK conn_id, PS_SHUTDOWN_CALLBACK cback)
{
    KIP_MSG_PS_SHUTDOWN_REQ *msg;

    /* Allocated memory is freed in ps_router_shutdown_req. */
    msg = xpnew(KIP_MSG_PS_SHUTDOWN_REQ);
    if (msg == NULL)
    {
        return;
    }

    /* Build message. */
    KIP_MSG_PS_SHUTDOWN_REQ_CON_ID_SET(msg, 0);

    ps_encapsulate_and_send(KIP_MSG_ID_PS_SHUTDOWN_REQ,
                            KIP_MSG_PS_SHUTDOWN_REQ_WORD_SIZE,
                            msg);
}

void ps_read_resp(bool success, unsigned total_size, unsigned payload_size, uint16 *payload)
{
    unsigned length;
    KIP_MSG_PS_READ_RES *msg;

    /* Allocated memory is freed in ps_router_read_resp. */
    length = KIP_MSG_PS_READ_RES_DATA_WORD_OFFSET;
    length += payload_size;
    msg = (KIP_MSG_PS_READ_RES*) xpnewn(length, uint16);
    if (msg == NULL)
    {
        return;
    }

    /* Build message. */
    KIP_MSG_PS_READ_RES_STATUS_SET(msg, success);
    KIP_MSG_PS_READ_RES_CON_ID_SET(msg, 0);
    KIP_MSG_PS_READ_RES_TOTAL_LEN_SET(msg, (uint16)total_size);
    memcpy(&msg->_data[KIP_MSG_PS_READ_RES_DATA_WORD_OFFSET], payload, payload_size * sizeof(uint16));

    ps_encapsulate_and_send(KIP_MSG_ID_PS_READ_RES,
                            length,
                            msg);
}

void ps_write_resp(unsigned success)
{
    KIP_MSG_PS_WRITE_RES *msg;

    /* Allocated memory is freed in ps_router_write_resp. */
    msg = xpnew(KIP_MSG_PS_WRITE_RES);
    if (msg == NULL)
    {
        return;
    }

    /* Build message. */
    KIP_MSG_PS_WRITE_RES_STATUS_SET(msg, success ? 0 : 1);
    KIP_MSG_PS_WRITE_RES_CON_ID_SET(msg, 0);

    ps_encapsulate_and_send(KIP_MSG_ID_PS_WRITE_RES,
                            KIP_MSG_PS_WRITE_RES_WORD_SIZE,
                            msg);
}

void ps_register(CONNECTION_LINK conn_id)
{
    KIP_MSG_PS_REGISTER_REQ *msg;

    /* Allocated memory is freed in ps_router_register_req. */
    msg = xpnew(KIP_MSG_PS_REGISTER_REQ);
    if (msg == NULL)
    {
        return;
    }

    /* Build message. */
    KIP_MSG_PS_REGISTER_REQ_CON_ID_SET(msg, conn_id);

    ps_encapsulate_and_send(KIP_MSG_ID_PS_REGISTER_REQ,
                            KIP_MSG_PS_REGISTER_REQ_WORD_SIZE,
                            msg);

    set_system_event(SYS_EVENT_PS_READY);
}
