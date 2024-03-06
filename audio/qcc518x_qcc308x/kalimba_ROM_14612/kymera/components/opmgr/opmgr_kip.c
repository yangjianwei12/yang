/****************************************************************************
 * Copyright (c) 2013 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr_kip.c
 * \ingroup  opmgr
 *
 * Operator Manager KIP-related parts. <br>
 */

/****************************************************************************
Include Files
*/

#include "opmgr_private.h"
#include "kip_msg_prim.h"
#include "patch.h"

/****************************************************************************
Type definitions
*/

/* Structure to keep extra create operator keys from create command */
typedef struct
{
    unsigned num_keys;
    OPERATOR_CREATE_EX_INFO *ex_info;
} OPMGR_CREATE_REQ_KEYS;

/****************************************************************************
Function Definitions
*/

/* Send 'list' type command via KIP: stop, reset, start, destroy */
bool opmgr_kip_build_send_list_cmd(CONNECTION_LINK con_id,
                                   unsigned num_ops,
                                   unsigned *op_list,
                                   KIP_MSG_ID kip_msg_id)
{
    bool success;
    unsigned length;
    ADAPTOR_MSGID id;
    KIP_MSG_OPLIST_CMD_REQ *req;
    uint16 *msg_data;

    length = KIP_MSG_OPLIST_CMD_REQ_OP_LIST_WORD_OFFSET + num_ops;
    req = (KIP_MSG_OPLIST_CMD_REQ *) xpnewn(length, uint16);
    if (req == NULL)
    {
        return FALSE;
    }

    KIP_MSG_OPLIST_CMD_REQ_COUNT_SET(req, num_ops);
    msg_data = &KIP_MSG_OPLIST_CMD_REQ_OP_LIST_GET(req);
    (void) adaptor_pack_list_to_uint16(msg_data, op_list, num_ops);

    id = KIP_MSG_ID_TO_ADAPTOR_MSGID(kip_msg_id);
    success = adaptor_send_message(con_id,
                                   id,
                                   length,
                                   (ADAPTOR_DATA) req);
    pfree(req);

    return success;
}

/**
 * \brief Build and send a request to create an operator on a secondary core.
 *
 * \param con_id       The identifier of the connection used.
 * \param cap_id       The type of capability of the operator.
 * \param op_id        The external identifier of the operator.
 * \param priority     The priority of the task used by the operator.
 * \param callback     The function to call back once finished.
 *
 * \return TRUE if the message was sent successfully.
 */
bool opmgr_kip_build_send_create_op_req(CONNECTION_LINK con_id,
                                        CAP_ID cap_id,
                                        EXT_OP_ID op_id,
                                        PRIORITY priority)
{
    bool success;
    unsigned length;
    KIP_MSG_CREATE_OPERATOR_REQ *msg_data;
    ADAPTOR_MSGID id;

    patch_fn_shared(kip);

    length = KIP_MSG_CREATE_OPERATOR_REQ_WORD_SIZE;
    msg_data = (KIP_MSG_CREATE_OPERATOR_REQ *) xzpnewn(length, uint16);
    if (msg_data == NULL)
    {
        return FALSE;
    }

    KIP_MSG_CREATE_OPERATOR_REQ_CAPABILITY_ID_SET(msg_data, cap_id);
    KIP_MSG_CREATE_OPERATOR_REQ_OP_ID_SET(msg_data, op_id);
    KIP_MSG_CREATE_OPERATOR_REQ_PRIORITY_SET(msg_data, priority);

    id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_CREATE_OPERATOR_REQ);
    success = adaptor_send_message(con_id,
                                   id,
                                   length,
                                   (ADAPTOR_DATA) msg_data);

    pfree(msg_data);

    return success;
}

/**
 * \brief Send operator message to KIP.
 *
 * \param con_id     Connection id
 * \param op_id      The created operator id
 * \param num_params Length of parameters in message
 * \param params     Pointer to the Parameters
 *
 * \return TRUE if the message was sent successfully.
 */
bool opmgr_kip_build_send_opmsg(CONNECTION_LINK con_id,
                                EXT_OP_ID op_id,
                                unsigned num_params,
                                unsigned *params)
{
    KIP_MSG_OPERATOR_MESSAGE_REQ *req;
    ADAPTOR_MSGID id;
    uint16 *payload;
    unsigned length;
    bool success;

    patch_fn_shared(kip);

    length = KIP_MSG_OPERATOR_MESSAGE_REQ_OP_MESSAGE_WORD_OFFSET + num_params;
    req = (KIP_MSG_OPERATOR_MESSAGE_REQ *) xpnewn(length, uint16);
    if (req == NULL)
    {
        return FALSE;
    }

    KIP_MSG_OPERATOR_MESSAGE_REQ_OPID_SET(req, op_id);
    payload = &KIP_MSG_OPERATOR_MESSAGE_REQ_OP_MESSAGE_GET(req);
    adaptor_pack_list_to_uint16(payload, params, num_params);

    id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_OPERATOR_MSG_REQ);
    success = adaptor_send_message(con_id,
                                   id,
                                   length,
                                   (ADAPTOR_DATA) req);

    pfree(req);
 
    return success;
}

/***************** Request handler functions *********************/

/**
 * \brief Handle the create_operator_ex request from kip adaptor.
 *
 * \param con_id   Connection id.
 * \param cap_id   The id of the capability the operator is an instance of.
 * \param op_id    The created operator id.
 * \param priority The priority of the task associated with the operator.
 * \param callback The callback function of create operator for KIP.
 */
void opmgr_kip_create_operator_ex_req_handler(CONNECTION_LINK con_id,
                                              CAP_ID cap_id,
                                              EXT_OP_ID op_id,
                                              PRIORITY priority,
                                              OP_CREATE_CBACK callback)
{
    OPERATOR_CREATE_EX_INFO info[2];
    info[0].key = OPERATOR_CREATE_OP_PRIORITY;
    info[0].value = (int32) priority;
    info[1].key = OPERATOR_CREATE_PROCESSOR_ID;
    info[1].value = (int32) proc_get_processor_id();
    opmgr_create_operator_ex(con_id, cap_id, op_id, 2, info, callback);
}

/**
 * \brief Handle the operator message request from kip adaptor
 *
 * \param con_id    Connection id
 * \param status    Generic Kymera status/error code
 * \param op_id     The created operator id
 * \param num_param Length of parameters in operator message
 * \param params    Parameters in the operator message
 * \param callback  The callback function of operator message
 */
void opmgr_kip_operator_message_req_handler(CONNECTION_LINK con_id,
                                            STATUS_KYMERA status,
                                            EXT_OP_ID op_id,
                                            unsigned num_param,
                                            unsigned *params,
                                            OP_MSG_CBACK callback)
{
    patch_fn_shared(kip);

    if (status == STATUS_OK)
    {
        opmgr_operator_message(con_id, op_id, num_param, params, callback);
    }
    else
    {
        callback(con_id, status, 0, 0, NULL);
    }
}

/***************** Response handler functions *********************/

/**
 * \brief Handle the create operator response from kip
 *
 * \param con_id  Connection id
 * \param status  Status of the request
 * \param op_id  The created operator id
 */
void opmgr_kip_create_resp_handler(CONNECTION_LINK con_id,
                                   STATUS_KYMERA status,
                                   EXT_OP_ID op_id)
{
    OP_CREATE_CBACK callback;
    CONNECTION_PEER client_id;
    INT_OP_ID int_op_id;

    patch_fn_shared(kip);

    int_op_id = EXT_TO_INT_OPID(op_id);
    if (status != STATUS_OK)
    {
        /* Remove the given operator's data structure
           from the remote operator list. */
        remove_op_data_from_list(int_op_id, &remote_oplist_head);
    }

    client_id = GET_CON_ID_SEND_ID(con_id);
    callback = (OP_CREATE_CBACK) opmgr_retrieve_in_progress_task(client_id,
                                                                 int_op_id);

    callback(con_id, status, op_id);
}

/**
 * \brief    Handle the std list operator (start/stop/reset/destroy) response from kip
 *
 * \param    con_id    Connection id
 * \param    status    Status of the request
 * \param    count     Number of operators handled successfully
 * \param    err_code  Error code upon operator error
 * \param    callback  The callback function of create operator
 */
void opmgr_kip_stdlist_resp_handler(CONNECTION_LINK con_id,
                                    STATUS_KYMERA status,
                                    unsigned count,
                                    unsigned err_code)
{
    patch_fn_shared(kip);

    opmgr_kip_list_resp_handler(con_id, status, count, err_code);
}

/**
 * \brief Handle the operator message response from kip.
 *
 * This is used in the KIP message handler on P0, where it
 * receives operator messages responses from P1 that must
 * be forwarded to the originator (apps0 typically).
 *
 * \param con_id          Connection id
 * \param status          Status of the request
 * \param op_id           The created operator id
 * \param num_resp_params Length of parameters in response message
 * \param resp_params     Parameters in response message
 */
void opmgr_kip_operator_msg_resp_handler(CONNECTION_LINK con_id,
                                         STATUS_KYMERA status,
                                         EXT_OP_ID op_id,
                                         unsigned num_resp_params,
                                         unsigned *resp_params)
{
    OP_MSG_CBACK callback;
    CONNECTION_PEER client_id;
    INT_OP_ID int_op_id;

    patch_fn_shared(kip);

    int_op_id = EXT_TO_INT_OPID(op_id);
    client_id = GET_CON_ID_SEND_ID(con_id);
    callback = (OP_MSG_CBACK) opmgr_retrieve_in_progress_task(client_id,
                                                              int_op_id);

    if (callback != NULL)
    {
        /* Just forward the whole response, unalterated */
        callback(con_id, status, op_id, num_resp_params, resp_params);
    }
    else
    {
        PL_PRINT_P0(TR_DUALCORE, "No context for KIP_MSG_ID_OPERATOR_MSG_RES.\n");
    }
}

/**
 * \brief Send an unsolicited message to KIP
 *
 * \param con_id Connection id
 * \param msg    Unsolicited messsage from the operator 
 *
 * \return True if successful.
 */
bool opmgr_kip_unsolicited_message(CONNECTION_LINK con_id,
                                   OP_UNSOLICITED_MSG *msg)
{
    bool success;
    unsigned length;
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ *req;
    uint16 *msg_data;
    ADAPTOR_MSGID id;

    patch_fn_shared(kip);

    /* Allocate the KIP message. */
    length = KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_MESSAGE_WORD_OFFSET;
    length += msg->length;
    req = (KIP_MSG_MESSAGE_FROM_OPERATOR_REQ*) xpnewn(length, uint16);
    if (req == NULL)
    {
        return FALSE;
    }

    /* Serialize the message. */
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_EXT_OP_ID_SET(req, msg->op_id);
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_CLIENT_ID_SET(req, msg->client_id);
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_MSG_ID_SET(req, msg->msg_id);
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_LENGTH_SET(req, msg->length);
    msg_data = &KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_MESSAGE_GET(req);
    adaptor_pack_list_to_uint16(msg_data, &msg->payload[0], msg->length);

    /* And send it to the primary processor. */
    id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_UNSOLICITED_FROM_OP_REQ);
    success = adaptor_send_message(con_id,
                                   id,
                                   length,
                                   (ADAPTOR_DATA) req);
    pfree(req);

    return success;
}

#ifdef PROFILER_ON
/**
 * \brief Send a KIP request to P1 to get the MIPS usage for the op IDs on P1.
 *
 * \param con_id            Connection id

 * \return True if successful.
 */
bool opmgr_kip_send_get_mips_req(CONNECTION_LINK con_id)
{
    bool success;
    ADAPTOR_MSGID id;

    patch_fn_shared(kip);

    /* And send it to the secondary processor. */
    id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_GET_MIPS_USAGE_REQ);
    success = adaptor_send_message(con_id,
                                   id,
                                   0,
                                   ADAPTOR_NULL);

    return success;
}
#endif /* PROFILER_ON */
