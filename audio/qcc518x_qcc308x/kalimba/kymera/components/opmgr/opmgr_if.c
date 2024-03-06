/****************************************************************************
 * Copyright (c) 2013 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr_if.c
 * \ingroup  opmgr
 *
 * Operator Manager interface file. <br>
 * This file contains the operator manager API that is exposed to the client <br>
 */

/****************************************************************************
Include Files
*/

#include "opmgr_private.h"
#include "audio_log/audio_log.h"

#ifdef THIRD_PARTY_SECURITY_SUPPORT
#include "third_party_security.h"
#endif

#if defined(SUPPORTS_MULTI_CORE)
#include "kip_msg_prim.h"
#endif

#ifdef INSTALL_AOV
#include "aov_common.h"
#endif

/****************************************************************************
Private type definitions
*/

/* Structure used to save parameters for a create request so that it
   can cross task boundaries i.e. in the case of a downloadable capability. */
typedef struct
{
    CONNECTION_LINK con_id;
    CAP_ID cap_id;
    EXT_OP_ID op_id;
    PRIORITY priority;
    PROC_ID_NUM processor_id;
    bool use_thread_offload;
    OP_CREATE_CBACK callback;
} CREATE_REQ_PARAMS;

typedef struct op_destroy_info
{
    unsigned count;
    EXT_OP_ID op_id_list[];
} op_destroy_info;

/** structure used to pass parameters to the callback.*/
typedef struct {
    /**
     * Operators to destroy.
     */
    op_destroy_info *destroy_op_list;
    /**
     * Final destroy callback.
     */
    OP_CON_ID_CBACK callback;

    /**
     * Data pointer for the callback function.
     */
    void *data_pointer;
}destroy_by_con_id_callback_parameters;

#if defined(SUPPORTS_MULTI_CORE)
typedef struct _cmd_aggregate
{
    unsigned int           agid;

    unsigned int          *op_list;
    unsigned int           num_ops;
    unsigned int           cur_idx;
    unsigned int           prv_idx;
    unsigned int           count;
    unsigned int          *chunk_list;
    unsigned int           num_chunks;
    OPCMD_ID               msg_id;
    uint16                 kip_msg_id;
    CONNECTION_LINK        con_id;
    preproc_function       preproc_func;
    OP_STD_LIST_CBACK      callback;

} CMD_AGGREGATE;

typedef struct _all_aggregates
{
    CMD_AGGREGATE*  entry[NUM_AGGREGATES];
} ALL_AGGREGATES;

DM_P0_RW_ZI ALL_AGGREGATES all_aggregates;

#endif /* defined(SUPPORTS_MULTI_CORE) */

/****************************************************************************
Private Constant Definitions
*/
/** A value that is used to represent that the effective client is internal to Kymera */
#define INTERNAL_CLIENT_ID 0xFF

/* Number of parallel start/stop/reset/destroy accmds that we support */
/* Maximum is 32, so that it fits in 5 bits in the connection id      */
#define NUM_AGGREGATES      1

/* use 13 bits for opid by retaining the top 3 bits of receiver processor id */
#define GET_CONID_PACKED_OPID(conid, opid)  \
            ((conid & CONID_PACKED_RECV_PROC_ID_MASK) | opid);

/* The highest opid value allowed to be generated. It wraps to 1 after this.
 * Currently this is 0x1fc0 >> 6 = 0x007F = 127.
 */
#define OPMGR_MAX_OPID_VALUE (STREAM_EP_OPID_MASK >> STREAM_EP_OPID_POSN)

/* Values used for 2nd parameter in 'opmgr_issue_list_cmd' (uint16 kip_msg_id).
 * When using dual-core build, use KIP_MSG_ID_xxx. When not using dual-core
 * build, the 2nd parameter is not actually used, yet define some reasonable
 * values anyway.
 */
#if defined(SUPPORTS_MULTI_CORE)
#define MP_MSG_ID_START_OPERATOR_REQ                  KIP_MSG_ID_START_OPERATOR_REQ
#define MP_MSG_ID_STOP_OPERATOR_REQ                   KIP_MSG_ID_STOP_OPERATOR_REQ
#define MP_MSG_ID_RESET_OPERATOR_REQ                  KIP_MSG_ID_RESET_OPERATOR_REQ
#define MP_MSG_ID_DESTROY_OPERATOR_REQ                KIP_MSG_ID_DESTROY_OPERATOR_REQ
#define MP_MSG_ID_P1_RUN_UNTIL_PREPROC_OPERATOR_REQ   KIP_MSG_ID_P1_RUN_UNTIL_PREPROC_OPERATOR_REQ
#else
#define MP_MSG_ID_START_OPERATOR_REQ                  OPCMD_START
#define MP_MSG_ID_STOP_OPERATOR_REQ                   OPCMD_STOP
#define MP_MSG_ID_RESET_OPERATOR_REQ                  OPCMD_RESET
#define MP_MSG_ID_DESTROY_OPERATOR_REQ                OPCMD_DESTROY
#define MP_MSG_ID_P1_RUN_UNTIL_PREPROC_OPERATOR_REQ   OPCMD_TEST
#endif

#ifdef UNIT_TEST_BUILD
#define opmgr_tag_dm_memory(ptr, id)
#else
#define opmgr_tag_dm_memory(ptr, id)         tag_dm_memory(ptr, id)
#endif

/****************************************************************************
Private Variable Declarations
*/
/** Next opid to be allocated */
static INT_OP_ID opid_next_id = 0;

/****************************************************************************
Private Function Definitions
*/

/**
 * \brief core function for sending messages to operators where the incoming
 * command has a list of operators to service.
 *
 * \param  con_id The connection ID of the source of the request
 * \param  num_ops The length of op_list
 * \param  op_list Pointer to an array of exeternal operator ids
 * \param  callback The callback function to use when the request has been handled.
 * \param  msg_id The id of the operator message to send to the operator(s)
 * \param  preproc_func The pointer to a function that can perform some operations prior to operator receiving its message.
 */
static void send_command_to_operator_list(CONNECTION_LINK con_id,
                                          unsigned int num_ops,
                                          unsigned int *op_list,
                                          OP_STD_LIST_CBACK callback,
                                          OPCMD_ID msg_id,
                                          preproc_function preproc_func)
{
    OPERATOR_DATA *cur_op;
    CONNECTION_LINK reversed_con_id;

    patch_fn_shared(opmgr);

    reversed_con_id = REVERSE_CONNECTION_ID(con_id);
    if (num_ops == 0)
    {
        /* Number of operator in the list is 0. */
        callback(reversed_con_id, STATUS_INVALID_CMD_PARAMS, 0, 0);
        return;
    }

    /* Lookup the first operator - this should only look in local oplist, as in single core case
     * (if notes on homogeneous lists of opIDs are in effect).
     */
    cur_op = get_anycore_op_data_from_id(EXT_TO_INT_OPID(op_list[0]));

    if (cur_op == NULL)
    {
        PL_PRINT_P0(TR_OPMGR, "Can't find operator\n");

        /* TODO - An error code needs setting here (See B-112315) */
        /* *error_code = OPERATOR NOT FOUND type of error code; */
        callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0);
        return;
    }
    else
    {
        /* Only the client that created the operator can ask it to do things so
         * we check the requestor is the creator before continuing.
         * The only exception is OBPM that may wish to issue commands to the
         * operator, e.g. reset bringing in default parameters to read or tweak.
         * Therefore OBPM is granted "special" access here and in case of opmsgs.
         */
        CONNECTION_PEER send_id;

        send_id = GET_CON_ID_SEND_ID(con_id);

        if ((GET_CON_ID_SEND_ID(cur_op->con_id) != send_id) &&
            (GET_SEND_RECV_ID_CLIENT_ID(send_id) != GET_SEND_RECV_ID_CLIENT_ID(ADAPTOR_OBPM)))
        {
            PL_PRINT_P0(TR_OPMGR, "Client does not own operator and it is not OBPM either.\n");

            callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0);
            return;
        }

        tRoutingInfo new_rinfo;
        /* This can be a multi operator operation so we need to save state
         * so that we know when all operators have been started. */
        MULTI_OP_REQ_DATA *data = xpnew(MULTI_OP_REQ_DATA);
        if (NULL == data)
        {
            callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0);
            return;
        }
        data->callback = (void *)callback;
        data->num_ops = num_ops;
        if (num_ops > 1)
        {
            /* We need to copy the op_list as it will get freed when this function
             * returns. We assume that only ops for the processor where we run are in the list.
             * Otherwise mega-messy, see wiki notes.
             */
            data->id_info.op_list = xpnewn(num_ops, unsigned int);
            if (NULL == data->id_info.op_list)
            {
                pfree(data);
                callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0);
                return;
            }
            /* we assume that only ops for the processor where we run are in the list. */
            memcpy(data->id_info.op_list, op_list, num_ops * sizeof(unsigned int));
        }
        else
        {
            /* Just store the single ID */
            data->id_info.id = op_list[0];
        }
        data->cur_index = 0;
#if defined(SUPPORTS_MULTI_CORE)
        /* Aggregate id encoded in recv part of conid, use whole con_id */
        if (!opmgr_store_in_progress_task(con_id, cur_op->id, (void *)data))
#else
        if (!opmgr_store_in_progress_task(GET_CON_ID_CLIENT_INFO(con_id), cur_op->id,
                    (void *)data))
#endif
        {
            if (data->num_ops > 1)
            {
                pfree(data->id_info.op_list);
            }
            pfree(data);
            callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0);
            return;
        }

#if defined(SUPPORTS_MULTI_CORE)
        /* The OPCMD_TEST is meant to run up to the preproc function on Px (x=1,2,3). */
        /* P0, upon successful return, can then do the preproc on P0, then issue the  */
        /* actual command than has preproc functionality.                             */
        if (msg_id == OPCMD_TEST)
        {
            opmgr_retrieve_in_progress_task(con_id, cur_op->id);
            if (data->num_ops > 1)
            {
                pfree(data->id_info.op_list);
            }
            pfree(data);
            /* The errcode (KIP_MSG_ID_P1_RUN_UNTIL_PREPROC_OPERATOR_REQ) indicates to  */
            /* P0 that it can do the preproc on P0 then issue the Px (x=1,2,3) destroy. */
            callback(reversed_con_id, STATUS_OK, 0, KIP_MSG_ID_P1_RUN_UNTIL_PREPROC_OPERATOR_REQ);
            return;
        }
#endif /* defined(SUPPORTS_MULTI_CORE) */

        /* Perform pre-processing (if any needed) prior to operator being issued the command. */
        /* Currently the pre-processing function receives operator data only. This may change in future if needed. */
        if(preproc_func)
        {
            if(!preproc_func(cur_op))
            {
                /* Take the task back that we've just decided to cancel before
                 * freeing it's memory. We do it early as it mallocs and may
                 * fail. */
#if defined(SUPPORTS_MULTI_CORE)
                /* Aggregate id encoded in recv part of conid, use whole con_id */
                opmgr_retrieve_in_progress_task(con_id, cur_op->id);
#else
                opmgr_retrieve_in_progress_task(GET_CON_ID_CLIENT_INFO(con_id), cur_op->id);
#endif
                if (data->num_ops > 1)
                {
                    pfree(data->id_info.op_list);
                }
                pfree(data);
                callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0);
                return;
            }
        }

        /* Send a message to the operator telling it to do something.
         * In multicore case, routing info from con_id contains also processor ID in the sender part.
         */
#if defined(SUPPORTS_MULTI_CORE)
        /* Aggregate id encoded in recv part of conid, use whole con_id */
        new_rinfo.src_id = con_id;
#else
        new_rinfo.src_id = GET_CON_ID_CLIENT_INFO(con_id);
#endif

        /* with homogeneous opID lists, the processor ID would be the same as
         * the one where we are running here & now. Otherwise much bigger mess - see wiki notes.
         */
        new_rinfo.dest_id = cur_op->id;

        /* see above notes on homogeneous vs. heterogeneous op lists. For now we assume
         * homogeneous, so op list only contains opIDs for the processor where we are running this.
         * Otherwise much bigger mess - see wiki notes.
         */
        put_message_with_routing(cur_op->task_id, msg_id, NULL, &new_rinfo);
    }
}

static INT_OP_ID get_next_opid(void)
{
    /* Increment and wrap id if needed */
    INT_OP_ID id = opid_next_id;

    do
    {
        ++id;
        /* Check for the wrap value of the id */
        if (id > OPMGR_MAX_OPID_VALUE)
        {
            id = 1;
        }
    }
    while (get_anycore_op_data_from_id(id));

    opid_next_id = id;

    return id;
}


#if defined(SUPPORTS_MULTI_CORE)

/**
 * \brief Enable/disable the endpoints connected to operators on P1 and the
 *        shadow endpoints on P0. This happens before starting/stopping the
 *        operator on P1 to mimic the single core behaviour.
 *
 * \param  cmd_aggregate The context
 *
 */
static void opmgr_kip_en_dis_endpoints(CMD_AGGREGATE *cmd_aggregate)
{
    unsigned i;
    void (*handler)(unsigned int) = NULL;

    patch_fn_shared(opmgr);

    switch(cmd_aggregate->msg_id )
    {
        case OPCMD_START:
            {
                handler = stream_enable_shadow_endpoint;
                break;
            }
        case OPCMD_STOP:
        case OPCMD_RESET:
            {
                handler = stream_disable_shadow_endpoint;
                break;
            }
        default:
            /* not expected */
            break;
    }

    if (handler != NULL)
    {
        for( i = cmd_aggregate->prv_idx; i<cmd_aggregate->cur_idx; i++)
        {
            unsigned idx, opidep, opid;
            OPERATOR_DATA *cur_op;

            opid = cmd_aggregate->op_list[i];

            cur_op = get_anycore_op_data_from_id(EXT_TO_INT_OPID(opid));

            for(idx = 0; idx < cur_op->cap_data->max_sinks; idx++)
            {
                opidep = opmgr_create_endpoint_id (opid, idx, SINK);
                handler(opidep);
            }

            for(idx = 0; idx < cur_op->cap_data->max_sources; idx++)
            {
                opidep = opmgr_create_endpoint_id (opid, idx, SOURCE);
                handler(opidep);
            }
        }
    }
}

/**
 * \brief Allocate an aggregate object.

 * This object is to split heterogeneous oplists into 1 or more homogeneous
 * oplist, which are then processed one-by-one.
 *
 * \param num_ops Number of operators in the oplist.
 * \param agid    Aggregate id.
 *
 * \return NULL upon error, valid pointer to an aggregate object if successful.
 */
static CMD_AGGREGATE *alloc_aggregate(unsigned num_ops, unsigned agid)
{
    CMD_AGGREGATE *aggregate;

    aggregate = xzpnew(CMD_AGGREGATE);
    if (aggregate == NULL)
    {
        return NULL;
    }
    aggregate->agid = agid;
    aggregate->op_list = xzpnewn(num_ops, unsigned int);
    if (aggregate->op_list == NULL)
    {
        pdelete(aggregate);
        return NULL;
    }
    return aggregate;
}

/**
 * \brief  Find an unused entry in the aggregate pointer array and allocate an
 *         aggregate object.
 *
 * \param  num_ops - Number of operators in the oplist.
 *
 * \return NULL upon error, valid pointer to an aggregate object if successful.
 */
static CMD_AGGREGATE *get_aggregate(unsigned num_ops)
{
    unsigned int i;

    for (i=0; i < NUM_AGGREGATES; i++)
    {
        if (all_aggregates.entry[i] == NULL)
        {
            all_aggregates.entry[i] = alloc_aggregate(num_ops, i);
            return all_aggregates.entry[i];
        }
    }

    return NULL;
}

/**
 * \brief  Free and remove an aggregate object from the aggregate pointer array.
 *
 * \param  agid - Index into the aggregate pointer array.
 *
 * \return NULL upon error, valid pointer to an aggregate object if successful.
 */
static bool rem_aggregate(unsigned int agid)
{
    if((agid < NUM_AGGREGATES) && (all_aggregates.entry[agid] != NULL))
    {
        pfree(all_aggregates.entry[agid]->op_list);
        all_aggregates.entry[agid]->op_list = NULL;
        pfree(all_aggregates.entry[agid]);
        all_aggregates.entry[agid] = NULL;
        return TRUE;
    }

    return FALSE;
}

/**
 * \brief Find an aggregate object for a given id.
 *
 * \param agid Index in the array of an aggregate object.
 *
 * \return NULL if object not found, else valid pointer to aggregate object
 */
static inline CMD_AGGREGATE *find_aggregate(unsigned int agid)
{
    return (agid < NUM_AGGREGATES)? all_aggregates.entry[agid]:NULL;
}

/**
 * \brief Allocate an aggregate object. This object is to split heterogeneous oplists into 1 or
 *        more homogeneous oplist, which are then processed one-by-one. Then initialise the
 *        newly allocated aggegate object.
 *
 * \param  msg_id       - Message ID
 * \param  kip_msg_id   - KIP message ID
 * \param  con_id       - Connection ID
 * \param  num_ops      - Number of operators in oplist
 * \param  op_list      - The array/list of operators
 * \param  callback     - The callback of the destroy/start/stop/reset operator request
 * \param  preproc_func - The preprocessing function
 *
 * \return NULL if object not found, else valid pointer to aggregate object with given id
 */
static CMD_AGGREGATE *aggregate_set_context(OPCMD_ID msg_id,
                                            uint16 kip_msg_id,
                                            CONNECTION_LINK con_id,
                                            unsigned num_ops,
                                            unsigned int *op_list,
                                            OP_STD_LIST_CBACK callback,
                                            preproc_function preproc_func)
{
    CMD_AGGREGATE *aggregate;
    unsigned int   i;

    /* Allocate a new object */
    aggregate = get_aggregate(num_ops);
    if (aggregate == NULL)
    {
        return NULL;
    }

    /* Initialise the aggregate object */
    for (i=0; i<num_ops; i++)
    {
        aggregate->op_list[i] = op_list[i];
    }
    aggregate->num_ops      = num_ops;
    aggregate->cur_idx      = 0;
    aggregate->prv_idx      = 0;
    aggregate->count        = 0;
    aggregate->chunk_list   = NULL;
    aggregate->num_chunks   = 0;
    aggregate->msg_id       = msg_id;
    aggregate->kip_msg_id   = kip_msg_id;
    aggregate->con_id       = con_id;
    aggregate->preproc_func = preproc_func;
    aggregate->callback     = callback;

    return aggregate;
}

/**
 * \brief Remove entries from P0 remote_oplist as per a given oplist.
 *
 * \param  count         - Number of opids successfully destroyed
 * \param  cmd_aggregate - Pointer to object that holds all info
 */
static void remove_remote_oplist(CMD_AGGREGATE *cmd_aggregate, unsigned count)
{
    OPERATOR_DATA *cur_op;
    unsigned i;

    /* Only do this upon destroy! */
    if (cmd_aggregate->msg_id != OPCMD_DESTROY)
    {
        return;
    }

    /* Remove 'count' entries from P0's remote op list; don't necessarily
       delete all entries, only the ones in oplist for P1; upon error do delete
       all that succeeded, this is reflected in 'count' (this is number of
       successful destroy operations). */
    for (i = 0; i < count; i++)
    {
        INT_OP_ID id;

        id = EXT_TO_INT_OPID(cmd_aggregate->op_list[i]);
        cur_op = get_remote_op_data_from_id(id);
        if ((cur_op != NULL) && (cur_op->processor_id != 0))
        {
            remove_op_data_from_list(id, &remote_oplist_head);
        }
    }
}

static bool callback_call(CMD_AGGREGATE *cmd_aggregate,
                          unsigned int agid,
                          STATUS_KYMERA status,
                          unsigned err_code)
{
    OP_STD_LIST_CBACK callback;
    CONNECTION_LINK reversed_con_id;
    unsigned count;

    /* Upon error report back how far we got: 'count' is how many operators
       in the list were handled ok. */
    remove_remote_oplist(cmd_aggregate, cmd_aggregate->count);
    reversed_con_id = REVERSE_CONNECTION_ID(cmd_aggregate->con_id);
    callback = cmd_aggregate->callback;
    count = cmd_aggregate->count;

    rem_aggregate(agid);

    return callback(reversed_con_id,
                    status,
                    count,
                    err_code);
}

/**
 * \brief Split a heterogeneous oplist into a series of homogeneous oplists and
 *        handle each homogeneous oplist one-by-one. This callback is called 1 or
 *        more times from P0 and/or Px (x=1,2,3) until all operators are handled
 *        or upon error.
 *
 * \param  con_id   - Connection ID
 * \param  status   - Return status
 * \param  count    - Number of operators handled successfully until error or success
 * \param  err_code - Error code
 *
 * \return TRUE upon success, FALSE upon error
 */
static bool callback_aggregate(CONNECTION_LINK con_id,
                               STATUS_KYMERA status,
                               unsigned count,
                               unsigned err_code)
{
    CMD_AGGREGATE *cmd_aggregate;
    OPERATOR_DATA *cur_op, *next_op;
    INT_OP_ID int_op_id;
    unsigned int agid;
    uint16 kip_msg_id;

    patch_fn_shared(opmgr);

    /* Context id is encoded in the conid receiver when issuing instruction
       so it's now in the sender part since this function is called with the
       inverted con-id. */
    agid = GET_SEND_RECV_ID_CLIENT_ID(GET_CON_ID_SEND_ID(con_id));

    L3_DBG_MSG2("LIST Command agid = %d packed con_id = 0x%X", agid, con_id);
    cmd_aggregate = find_aggregate(agid);
    if (cmd_aggregate == NULL)
    {
        panic(PANIC_AUDIO_OPMGR_OPLIST_LOST_CONTEXT);
    }

    /* Update sum total count with completed chunk. */
    cmd_aggregate->count += count;

    /* Hm, let's see what they did and whether there is more work to do. */
    if ((status != STATUS_OK) ||
        (cmd_aggregate->cur_idx >= cmd_aggregate->num_ops))
    {
        return callback_call(cmd_aggregate, agid, status, err_code);
    }

    /* Yes. Keep track of where we are now. */
    cmd_aggregate->prv_idx = cmd_aggregate->cur_idx;

    /* Find processor ID of current operand. */
    int_op_id = EXT_TO_INT_OPID(cmd_aggregate->op_list[cmd_aggregate->cur_idx]);
    cur_op = get_anycore_op_data_from_id(int_op_id);
    if (cur_op == NULL)
    {
        return callback_call(cmd_aggregate, agid, STATUS_CMD_FAILED, 2);
    }

    /* Create new oplist with consecutive same processor IDs. */
    cmd_aggregate->chunk_list = &cmd_aggregate->op_list[cmd_aggregate->cur_idx];
    cmd_aggregate->cur_idx++;
    cmd_aggregate->num_chunks = 1;
    next_op = cur_op;
    /* Handle only sub-lists of size 1 on Px (x=1,2,3) */
    if (cur_op->processor_id == PROC_PROCESSOR_0)
    {
        while ((next_op != NULL) &&
               (cmd_aggregate->cur_idx < cmd_aggregate->num_ops) &&
               (cur_op->processor_id == next_op->processor_id))
        {
            int_op_id = EXT_TO_INT_OPID(cmd_aggregate->op_list[cmd_aggregate->cur_idx]);
            next_op = get_anycore_op_data_from_id(int_op_id);
            if ((next_op != NULL) &&
                (cur_op->processor_id == next_op->processor_id))
            {
                cmd_aggregate->num_chunks++;
                cmd_aggregate->cur_idx++;
            }
        }
    }

    /*
     * The con-id so-far is the inverse con-id; issue a new (partial)
     * start/stop/reset/destroy oplist command with a proper con-id,
     * and encode the aggregate id in the receiver id. The original
     * receiver id is kept in the aggregate->conid and used when
     * reporting back to accmd.
     */
    con_id = PACK_CON_ID(GET_CON_ID_SEND_ID(cmd_aggregate->con_id),
                         (CONNECTION_PEER) agid);

    /* Send chunk oplist. */
    if (cur_op->processor_id == 0)
    {
        /* We are on same processor where the operators reside
           so do the usual tedium. */
        send_command_to_operator_list(con_id,
                                      cmd_aggregate->num_chunks,
                                      cmd_aggregate->chunk_list,
                                      callback_aggregate,
                                      cmd_aggregate->msg_id,
                                      cmd_aggregate->preproc_func);
        return FALSE;
    }

    kip_msg_id = cmd_aggregate->kip_msg_id;

    /* Perform pre-processing (if any needed) prior to operator being issued
       the command. Currently the pre-processing function receives operator
       data only. This may change in future if needed. */
    if (cmd_aggregate->preproc_func != NULL)
    {
        if (err_code == KIP_MSG_ID_P1_RUN_UNTIL_PREPROC_OPERATOR_REQ)
        {
            if (!cmd_aggregate->preproc_func(cur_op))
            {
                /* Take the task back that we've just decided to cancel before
                   freeing it's memory. We do it early as it mallocs and may
                   fail. */
                return callback_call(cmd_aggregate, agid, STATUS_CMD_FAILED, 0);
            }
        }
        else
        {
            cmd_aggregate->cur_idx--;
            kip_msg_id = KIP_MSG_ID_P1_RUN_UNTIL_PREPROC_OPERATOR_REQ;
        }
    }

    con_id = PACK_CONID_PROCID(con_id, cur_op->processor_id);

    /* Before we send the start request to P1, we need to start the connected
     * endpoint on P0. */
    opmgr_kip_en_dis_endpoints(cmd_aggregate);

    if (!opmgr_kip_build_send_list_cmd(con_id,
                                       cmd_aggregate->num_chunks,
                                       cmd_aggregate->chunk_list,
                                       kip_msg_id))
    {
        return callback_call(cmd_aggregate, agid, STATUS_CMD_FAILED, 0);
    }

    return FALSE;
}


/**
 * \brief handle the list response from kip
 *
 * \param  con_id   Connection ID
 * \param  status   Return status
 * \param  count    Number of operators handled successfully
 *                  until error or success
 * \param  err_code Error code
 *
 * \return TRUE upon success, FALSE upon error
 */
bool opmgr_kip_list_resp_handler(CONNECTION_LINK con_id,
                                 STATUS_KYMERA status,
                                 unsigned count,
                                 unsigned err_code)
{
    CMD_AGGREGATE *cmd_aggregate;
    OP_STATE state;
    unsigned int agid;

    patch_fn_shared(opmgr);

    /*
     * Context id is encoded in the conid receiver when issuing instruction
     * so it's now in the sender part since this function is called with the
     * inverted con-id.
     */
    agid = GET_SEND_RECV_ID_CLIENT_ID(GET_CON_ID_SEND_ID(con_id));

    L3_DBG_MSG2("LIST Command agid = %d packed con_id = 0x%X",agid, con_id);
    cmd_aggregate = find_aggregate(agid);
    if (cmd_aggregate == NULL)
    {
        panic(PANIC_AUDIO_OPMGR_OPLIST_LOST_CONTEXT);
    }

    state = OP_NOT_RUNNING;
    switch( cmd_aggregate->msg_id )
    {
        case OPCMD_START:
            {
                state = OP_RUNNING;
                break;
            }
        case OPCMD_STOP:
        case OPCMD_RESET:
            {
                break;
            }
        default:
            /* not expected */
            break;
    }

    if (status == STATUS_OK)
    {
        unsigned i;

        for( i = cmd_aggregate->prv_idx; i<cmd_aggregate->cur_idx; i++)
        {
            unsigned opid;
            OPERATOR_DATA *cur_op;

            opid = cmd_aggregate->op_list[i];

            cur_op = get_anycore_op_data_from_id(EXT_TO_INT_OPID(opid));

            /* set the op state based on the remote event */
            cur_op->state = state;
        }
    }

    return callback_aggregate(con_id, status, count, err_code);
}

#endif /* defined(SUPPORTS_MULTI_CORE) */

static void opmgr_issue_list_cmd(OPCMD_ID msg_id,
                                 uint16 kip_msg_id,
                                 CONNECTION_LINK con_id,
                                 unsigned num_ops,
                                 unsigned int *op_list,
                                 OP_STD_LIST_CBACK callback,
                                 preproc_function preproc_func)
{
#if defined(SUPPORTS_MULTI_CORE)
    if (PROC_PRIMARY_CONTEXT())
    {
        CONNECTION_LINK reversed_con_id;
        CMD_AGGREGATE *opcmd_context;

        opcmd_context = aggregate_set_context(msg_id,
                                              kip_msg_id,
                                              con_id,
                                              num_ops,
                                              op_list,
                                              callback,
                                              preproc_func);
        if (opcmd_context != NULL)
        {
            /*
            * Now the receiver is the context. Store the orginal conid in the conext.
            * Retrieve that to get the orginal conid before sending the orginal
            * response.
            * The context is needed to keep track
            * progress of a heterogeneous oplist (both P0 and P1 operators in list),
            * and the list is processed using partial oplists that have the same
            * contiguous processor owner. E.g an oplist P0 P0 P1 P1 P0 is handled
            * as 3 partial oplists (P0, P0), (P1 P1), (P0), which are dealt with
            * consecutively.
            */
            reversed_con_id = PACK_CON_ID((CONNECTION_PEER) opcmd_context->agid,
                                          GET_CON_ID_SEND_ID(con_id));
            callback_aggregate(reversed_con_id, STATUS_OK, 0, 0);
        }
        else
        {
            reversed_con_id = REVERSE_CONNECTION_ID(con_id);
            callback(reversed_con_id, STATUS_CMD_FAILED, 0, 1);
        }
    }
    else
#endif /* defined(SUPPORTS_MULTI_CORE) */
    {
        /* We are on same processor where the operators reside - so do the usual tedium */
        send_command_to_operator_list(con_id, num_ops, op_list, callback, msg_id, preproc_func);
    }
}

/**
 * \brief Count the number of entries in the table of downloada
 *          capabilities linked list.
 *
 * \return total number of downloadable capabilities.
 */
#ifdef INSTALL_CAP_DOWNLOAD_MGR
static inline unsigned get_num_download_caps(void)
{
    unsigned download_num_caps = 0;
    DOWNLOAD_CAP_DATA_DB* cap_download_data_ptr = *cap_download_data_list;
    /* No static capability found, try searching in the download table */
    while (cap_download_data_ptr != NULL)
    {
        download_num_caps++;
        cap_download_data_ptr = cap_download_data_ptr->next;
    }
    return download_num_caps;
}
#else
static inline unsigned get_num_download_caps(void)
{
    return 0;
}
#endif /* INSTALL_CAP_DOWNLOAD_MGR */

/**
 * \brief Add download capabilities ids to capid_list
 *
 * \param  *capid_list          - pointer to the list of capabilities id's to be updated.
 * \param  missing_num_capids   - Number of missing capabilities to be added to the list.
 * \param  num_capids_skip      - Number of capids to be skip from downloadable linked list.
 *
 * \return none
 */
#ifdef INSTALL_CAP_DOWNLOAD_MGR
static inline void add_download_cap_to_list(unsigned *capid_list,
                                            unsigned missing_num_capids,
                                            unsigned num_capids_skip)
{
    unsigned i;
    DOWNLOAD_CAP_DATA_DB* cap_download_data_ptr = *cap_download_data_list;

    for (i = 0; i< num_capids_skip; i++)
    {
        cap_download_data_ptr = cap_download_data_ptr->next;
    }

    /* Copy the id of the relevant downloaded capabilities. */
    for (i = 0; i < missing_num_capids; i++)
    {
        capid_list[i] = cap_download_data_ptr->cap->id;
        cap_download_data_ptr = cap_download_data_ptr->next;
    }
}
#else
static inline void add_download_cap_to_list(unsigned *capid_list,
                                            unsigned missing_num_capids,
                                            unsigned num_capids_skip)
{}
#endif

/****************************************************************************
Public Function Definitions
*/
/****************************************************************************
 *
 * opmgr_start_operator
 *
 */
void opmgr_start_operator(CONNECTION_LINK con_id,
                          unsigned num_ops,
                          EXT_OP_ID *op_list,
                          OP_STD_LIST_CBACK callback)
{
    PL_PRINT_P0(TR_OPMGR, "Opmgr: Start Operator(s).\n");

    opmgr_issue_list_cmd(OPCMD_START, MP_MSG_ID_START_OPERATOR_REQ, con_id, num_ops, op_list, callback, NULL);
}

/****************************************************************************
 *
 * opmgr_stop_operator
 *
 */
void opmgr_stop_operator(CONNECTION_LINK con_id,
                         unsigned int num_ops,
                         EXT_OP_ID *op_list,
                         OP_STD_LIST_CBACK callback)
{
    PL_PRINT_P0(TR_OPMGR, "Opmgr: Stop Operator(s).\n");

    opmgr_issue_list_cmd(OPCMD_STOP, MP_MSG_ID_STOP_OPERATOR_REQ, con_id, num_ops, op_list, callback, NULL);
}

/****************************************************************************
 *
 * opmgr_reset_operator
 *
 */
void opmgr_reset_operator(CONNECTION_LINK con_id,
                          unsigned int num_ops,
                          EXT_OP_ID *op_list,
                          OP_STD_LIST_CBACK callback)
{
    PL_PRINT_P0(TR_OPMGR, "Opmgr: Reset Operator(s).\n");

    opmgr_issue_list_cmd(OPCMD_RESET, MP_MSG_ID_RESET_OPERATOR_REQ, con_id, num_ops, op_list, callback, NULL);
}

static bool opmgr_create_operator_post_dnld(const CREATE_REQ_PARAMS *params)
{
    OPERATOR_DATA *new_op;
    CONNECTION_PEER client_id;
    const CAPABILITY_DATA *cap_data_ptr;

    patch_fn_shared(opmgr);

    new_op = NULL;
    cap_data_ptr = NULL;
    PL_PRINT_P1(TR_OPMGR, "Opmgr: Create Operator with capid (%d).\n", params->cap_id);

    /* get pointer to the capability data */
#if defined(SUPPORTS_MULTI_CORE)
    cap_data_ptr = opmgr_lookup_cap_data_for_cap_id(params->cap_id,
                                                    params->processor_id);
#else
    cap_data_ptr = opmgr_lookup_cap_data_for_cap_id(params->cap_id);
#endif /* SUPPORTS_MULTI_CORE */
    if (cap_data_ptr == NULL)
    {
        L2_DBG_MSG1("CREATE_OPERATOR failed, unknown capability ID 0x%04X", params->cap_id);
        return FALSE;
    }

    /* Allocate storage for the new operator - standard and cap-specific part */
    /* Efficient to get this done in one block, if this fails we the operation
     * isn't possible at this time. */

#if defined(SUPPORTS_MULTI_CORE)
    /* Operator is created remotely */

    if (!PROC_ON_SAME_CORE(params->processor_id))
    {
       /* we should not end up here on a
        * secondary OpMgr with processor_id set to 0 (primary)!
        */
        if (PROC_PRIMARY_CONTEXT())
        {
            new_op = xzpmalloc(sizeof(OPERATOR_DATA));
        }

        if (new_op == NULL)
        {
            L2_DBG_MSG1("CREATE_OPERATOR failed to allocate op data for capability ID 0x%04X", params->cap_id);
            return FALSE;
        }

    }
    else
#endif /* defined(SUPPORTS_MULTI_CORE) */
    /* Only create extra op data if we are on same processor where op is being actually created */
    {
        new_op = xzpmalloc(sizeof(OPERATOR_DATA) +
                           cap_data_ptr->instance_data_size);

        if (new_op == NULL)
        {
            L2_DBG_MSG1("CREATE_OPERATOR failed to allocate op data for capability ID 0x%04X", params->cap_id);
            return FALSE;
        }

        /* hook pointer to specific part into the standard operator data
         * - always next address after the extra op data pointer */
        new_op->extra_op_data = (void*)((uintptr_t)&new_op->extra_op_data + sizeof(void*));
    }

    /* Hook cap data pointer into the operator data standard part. */
    new_op->cap_data = cap_data_ptr;
    new_op->local_process_data = cap_data_ptr->process_data;
    new_op->state = OP_NOT_RUNNING;

    /* Setup operator ID. We only accept unspecified op ID if we are on
       primary processor. We already checked and rejected case of running
       on secondary processor and receiving zero op ID in the command. */
    if (params->op_id == 0)
    {
        /* If we got to here, we are on primary (or one and only) processor.
           Get a new op ID. */
        new_op->id = get_next_opid();
    }
    else
    {
        INT_OP_ID int_op_id;

        int_op_id = EXT_TO_INT_OPID(params->op_id);
#if defined(SUPPORTS_MULTI_CORE)
        if (new_op->id == 0)
        {
            /* assign the same operator ID created on P0 to the new op data structure on P1 */
            new_op->id = int_op_id;
        }
#else
        /* Rather than blindly accepting an operator ID make sure it has a valid
         * format. That is STREAM_EP_OP_BIT set and no other bits set outside
         * the STREAM_EP_OPID_MASK. Also we shouldn't already have an operator
         * with this ID. */
        if ((params->op_id & STREAM_EP_OP_BIT) &&
            (0 == (params->op_id & ~(STREAM_EP_OP_BIT | STREAM_EP_OPID_MASK))))
        {
            if (get_anycore_op_data_from_id(int_op_id) != NULL)
            {
                L2_DBG_MSG1("CREATE_OPERATOR failed, operator ID 0x%04X already exists", params->op_id);
                pfree(new_op);
                return FALSE;
            }
            else
            {
                new_op->id = int_op_id;
            }
        }
        else
        {
            L2_DBG_MSG1("CREATE_OPERATOR failed, invalid operator ID 0x%04X", params->op_id);
            pfree(new_op);
            return FALSE;
        }
#endif /* defined(SUPPORTS_MULTI_CORE) */
    }

    /* Store connection ID, used when operators send unsolicited messages */
    new_op->con_id = params->con_id;

    /* operator to store the creator client ID - may seem a bit redundant, but con_id field could change later */
    new_op->creator_client_id = GET_CON_ID_SEND_ID(params->con_id);

    /* Store the processor id of which the operator is supposed to be created */
    new_op->processor_id = params->processor_id;

#ifdef INSTALL_THREAD_OFFLOAD
    new_op->thread_offload_enabled = params->use_thread_offload;
#endif

    /* set the state of the operator */
    new_op->state = OP_NOT_RUNNING;

    /* All is well so far - add operator into list (if multicore, local or remote op list depending on where it is created) */
    client_id = GET_CON_ID_SEND_ID(params->con_id);
#if defined(SUPPORTS_MULTI_CORE)
    if (!PROC_ON_SAME_CORE(params->processor_id))
    /* We are not creating the operator locally, so this op will not have task created for
     * it on this processor. We must not end up here if we are on P1 and
     * creating on P0 that is invalid and should be caught by sanity checking
     * prior to this point.
     */
    {
        new_op->next = remote_oplist_head;
        remote_oplist_head = new_op;

        if (!opmgr_store_in_progress_task(client_id,
                                          new_op->id,
                                          (void *) params->callback))
        {
            L2_DBG_MSG("CREATE_OPERATOR failed, unable to save context");
            remote_oplist_head = new_op->next;
            pfree(new_op);
            return FALSE;
        }

        /* Send KIP message to create it on remote processor. The KIP response
         * will lead to the API callback being called, so use some housekeeping
         * there to call the callback when response comes back. */
        if (!opmgr_kip_build_send_create_op_req(params->con_id,
                                                params->cap_id,
                                                INT_TO_EXT_OPID(new_op->id),
                                                params->priority))
        {
            /* All these operations happen in the background so new_op is still
               top of the list, no need to check. */
            L2_DBG_MSG("CREATE_OPERATOR failed to send remote request");
            remote_oplist_head = new_op->next;
            (void) opmgr_retrieve_in_progress_task(client_id,
                                                   new_op->id);
            pfree(new_op);
            return FALSE;
        }
#ifdef INSTALL_TIMING_TRACE
        if (timing_trace_setup_is_enabled())
        {
            timing_trace_setup_op_create(new_op, PROC_PROCESSOR_1);
        }
#endif /* INSTALL_TIMING_TRACE */
    }
    else
#endif /* defined(SUPPORTS_MULTI_CORE) */
    {
        INT_OP_ID *msg_op_id;
        tRoutingInfo new_rinfo;

        new_op->next = oplist_head;
        oplist_head = new_op;

        /* Create a task for the operator (BUT only if we are on the processor where the op is created).
         * All "local" operator tasks have one queue for control messages.
         * They are kicked via background interrupts.
         *
         * Every "local" operator task has the same background interrupt handler
         * (opmgr_operator_bgint_handler). The task_data pointer we pass in here
         * is the pointer to the OPERATOR_DATA we are in the process of creating, so
         * that in the handler we can identify which operator to poke.
         */
        new_op->task_id = create_task(params->priority,
                                      new_op,
                                      opmgr_operator_task_handler,
                                      opmgr_operator_bgint_handler);
        if (new_op->task_id == NO_TASK)
        {
            /* All these operations happen in the background so new_op is still top
             * of the list, no need to check. */
            L2_DBG_MSG("CREATE_OPERATOR failed to create new task");
            oplist_head = oplist_head->next;
            pfree(new_op);
            return FALSE;
        }

        /* Set owner of operator allocated memory to its task id */
        opmgr_tag_dm_memory(new_op, new_op->task_id);

        /* Create a message for the operator task containing the internal
           operator id. In practice the operators all ignore the content of
           the message as they can get the relevant information through the
           functions "base_op_get_int_op_id" and "base_op_get_ext_op_id". */
        msg_op_id = xpnew(INT_OP_ID);
        /* If there isn't enough RAM to send the message fail creation */
        if (msg_op_id == NULL)
        {
            L2_DBG_MSG("CREATE_OPERATOR failed to send message to operator");
            /* Delete the task that there is no use for */
            delete_task(new_op->task_id);

            /* All these operations happen in the background so new_op is still top
             * of the list, no need to check. */
            oplist_head = oplist_head->next;
            pfree(new_op);
            return FALSE;
        }

        *msg_op_id = new_op->id;
        new_rinfo.src_id  = client_id;
        new_rinfo.dest_id = GET_CONID_PACKED_OPID(params->con_id, new_op->id);

        /* Save the callback so we can use it when we get the response. If we can't, fail */
        if (!opmgr_store_in_progress_task(client_id,
                                          new_op->id,
                                          (void *) params->callback))
        {
            L2_DBG_MSG("CREATE_OPERATOR failed, unable to save context");

            pfree(msg_op_id);
            /* Delete the task that there is no use for */
            delete_task(new_op->task_id);

            /* All these operations happen in the background so new_op is still top
             * of the list, no need to check. */
            oplist_head = oplist_head->next;
            pfree(new_op);
            return FALSE;
        }

#ifdef INSTALL_TIMING_TRACE
        if (timing_trace_setup_is_enabled())
        {
            timing_trace_setup_op_create(new_op, PROC_PROCESSOR_0);
        }
#endif /* INSTALL_TIMING_TRACE */

        put_message_with_routing(new_op->task_id,
                                 OPCMD_CREATE,
                                 msg_op_id,
                                 &new_rinfo);
    }
    return TRUE;
}

static bool opmgr_create_operator_cback(void *context, STATUS_KYMERA status)
{
    CREATE_REQ_PARAMS *req = (CREATE_REQ_PARAMS *) context;
    bool success;

    /* If download failed, send back an error before trying to create.
       If we are on primary processor, it does callback, otherwise
       sends KIP response. */
    if (status != STATUS_OK)
    {
        success = FALSE;
    }
    else
    {
        /* Recover create request parameters and
           call normal operator create routine. */
        success = opmgr_create_operator_post_dnld(req);
    }

    if (!success)
    {
        CONNECTION_LINK reversed_con_id;
        reversed_con_id = REVERSE_CONNECTION_ID(req->con_id);
        req->callback(reversed_con_id, STATUS_CMD_FAILED, req->op_id);
    }

    pdelete(req);

    return TRUE;
}

/* NOTE: In multi-processor case, it is assumed this is called on primary
   processor as in single proc case (from command adaptor), and on secondary
   processor as a result of receiving a KIP message. */
void opmgr_create_operator_ex(CONNECTION_LINK con_id,
                              CAP_ID cap_id,
                              EXT_OP_ID op_id,
                              unsigned int num_keys,
                              OPERATOR_CREATE_EX_INFO *info,
                              OP_CREATE_CBACK callback)
{
    CREATE_REQ_PARAMS *params;
    CONNECTION_LINK reversed_con_id;
    PRIORITY priority;
    PROC_ID_NUM processor_id;
    bool offload_enabled;
    unsigned int i;

    patch_fn_shared(opmgr);

    reversed_con_id = REVERSE_CONNECTION_ID(con_id);
    priority = LOWEST_PRIORITY;
    processor_id = PROC_PROCESSOR_0;
    offload_enabled = FALSE;

    for (i=0; i<num_keys; i++)
    {
        unsigned int key = info[i].key;
        int32 value = info[i].value;

        switch (key)
        {
            case OPERATOR_CREATE_OP_PRIORITY:
            {
                /* Do a 32 bits compare in case someone tries to be clever
                   with the key. */
                if ((value < LOWEST_PRIORITY) ||
                    (value > HIGHEST_PRIORITY))
                {
                    callback(reversed_con_id, STATUS_CMD_FAILED, op_id);
                    return;
                }
                priority = (PRIORITY) value;
                break;
            }

            case OPERATOR_CREATE_PROCESSOR_ID:
            {
                /* Do a 32bit compare in case someone tries to be clever
                   with the key. */
                if (!proc_is_valid((unsigned) value))
                {
                    callback(reversed_con_id, STATUS_CMD_FAILED, op_id);
                    return;
                }

                processor_id = (PROC_ID_NUM) value;

#if defined(SUPPORTS_MULTI_CORE)
                /* The primary processor is always available while
                   auxiliary processors might not be. */
                if ((processor_id != PROC_PROCESSOR_0) &&
                    !kip_aux_processor_has_started(processor_id))
                {
#ifdef INSTALL_THREAD_OFFLOAD
                    if (thread_offload_is_enabled())
                    {
                        /* Offloaded operator is created on P0 */
                        processor_id = PROC_PROCESSOR_0;
                        offload_enabled = TRUE;
                        L2_DBG_MSG1("opmgr_create_operator_ex cap_id = %04X offloaded", cap_id);
                    }
                    else
#endif /* INSTALL_THREAD_OFFLOAD */
                    {
                        callback(reversed_con_id, STATUS_CMD_FAILED, op_id);
                        return;
                    }
                }
#endif /* defined(SUPPORTS_MULTI_CORE) */

                /* Pack the connection id and processor id
                   only the first time. */
#if defined(SUPPORTS_MULTI_CORE)
                if (PROC_PRIMARY_CONTEXT())
                {
                    con_id = PACK_CONID_PROCID(con_id, processor_id);
                    reversed_con_id = REVERSE_CONNECTION_ID(con_id);
                }
#endif
                break;
            }

            default:
            {
                /* key is unknown */
                callback(reversed_con_id, STATUS_CMD_FAILED, op_id);
                return;
            }
        }
    }

    /* Save the parameters to use through the callbacks.
       Memory is freed in opmgr_create_operator_cback. */
    params = xpnew(CREATE_REQ_PARAMS);
    if (params == NULL)
    {
        callback(reversed_con_id, STATUS_CMD_FAILED, op_id);
        return;
    }
    params->con_id = con_id;
    params->cap_id = cap_id;
    params->op_id = op_id;
    params->priority = priority;
    params->processor_id = processor_id;
    params->callback = callback;
    params->use_thread_offload = offload_enabled;

#if defined(INSTALL_CAP_DOWNLOAD_MGR)
    /* Downloadable remote capability is only handled on remote processor. */
    if (PROC_ON_SAME_CORE(processor_id))
    {
        CAP_DOWNLOAD_STATUS status;
        bool is_downloadable;

        is_downloadable = opmgr_get_download_cap_status(cap_id, &status);

        if (is_downloadable)
        {
            /* If it is a downloadable capability and it's in installed state,
               let capability download manager do the download and deal with the
               request, otherwise, operate as a normal capability. */
            if (status == CAP_INSTALLED)
            {
                /* Download mgr will call callback that will handle response
                   depending on single-processor vs. multi-processor case. */
                cap_download_mgr_make_cap_ready(con_id,
                                                cap_id,
                                                params,
                                                opmgr_create_operator_cback);
                return;
            }
        }
    }
#endif /* INSTALL_CAP_DOWNLOAD_MGR */

    opmgr_create_operator_cback(params, STATUS_OK);
}

/****************************************************************************
 *
 * opmgr_p1_run_until_preproc_operator
 *
 */
void opmgr_p1_run_until_preproc_operator(CONNECTION_LINK con_id,
                                         unsigned int num_ops,
                                         EXT_OP_ID *op_list,
                                         OP_STD_LIST_CBACK callback)
{
    PL_PRINT_P0(TR_OPMGR, "Opmgr: Test Operator(s).\n");

    opmgr_issue_list_cmd(OPCMD_TEST, MP_MSG_ID_P1_RUN_UNTIL_PREPROC_OPERATOR_REQ, con_id, num_ops, op_list, callback, NULL);
}

/****************************************************************************
 *
 * opmgr_destroy_operator
 *
 */
void opmgr_destroy_operator(CONNECTION_LINK con_id,
                            unsigned int num_ops,
                            EXT_OP_ID *op_list,
                            OP_STD_LIST_CBACK callback)
{
    PL_PRINT_P0(TR_OPMGR, "Opmgr: Destroy Operator(s).\n");

    opmgr_issue_list_cmd(OPCMD_DESTROY, MP_MSG_ID_DESTROY_OPERATOR_REQ, con_id, num_ops, op_list, callback, opmgr_destroy_op_endpoints);
}


static bool opmgr_destroy_by_con_id_destroy_callback(CONNECTION_LINK con_id,
                                                     STATUS_KYMERA status,
                                                     unsigned count,
                                                     unsigned err_code)
{
    /* We've finished processing the destroys now we can free the memory
     * containing the operator list. */

    destroy_by_con_id_callback_parameters *callback_parameters = opmgr_retrieve_in_progress_task(INTERNAL_CLIENT_ID, INTERNAL_CLIENT_ID);
    op_destroy_info *destroy_list = callback_parameters->destroy_op_list;


    if(status == STATUS_OK)
    {
        L2_DBG_MSG("Destroy operators finished, destroy the endpoints.");
    }
    else
    {
        L2_DBG_MSG2("Destroy operators failed with count %d and error %d",count,err_code);
    }

    callback_parameters->callback(callback_parameters->data_pointer);

    pfree(destroy_list);
    pdelete(callback_parameters);
    return TRUE;
}

static bool opmgr_destroy_by_con_id_stop_callback(CONNECTION_LINK reversed_con_id,
                                                  STATUS_KYMERA status,
                                                  unsigned count,
                                                  unsigned err_code)
{
    /* We've finished processing the stops now we can destroy the operators.
     * There isn't much point paying attention to the status, as if the stop
     * failed somewhere we still want to destroy as many operators as possible. */
    destroy_by_con_id_callback_parameters *callback_parameters = opmgr_retrieve_in_progress_task(INTERNAL_CLIENT_ID, INTERNAL_CLIENT_ID);
    op_destroy_info *destroy_list = callback_parameters->destroy_op_list;


    if(status == STATUS_OK)
    {
        L2_DBG_MSG("Stop operators finished, now destroy them");
    }
    else
    {
        L2_DBG_MSG2("Stop operators failed with count %d and error %d, Try destroying anyways",count,err_code);
    }

    /* Save the destroy list so the callback can free it. If we haven't the RAM
     * to do this then panic. */
    if (!opmgr_store_in_progress_task(INTERNAL_CLIENT_ID, INTERNAL_CLIENT_ID,
                                    (void *)callback_parameters))
    {
        panic(PANIC_AUDIO_OPMGR_DESTROY_BY_CONID_NO_RESOURCES);
    }

    /* Connection ID here has been reversed on its way back,
     * so client id will be in the recv field.
     */
    opmgr_destroy_operator(GET_CON_ID_RECV_ID(reversed_con_id), destroy_list->count, destroy_list->op_id_list,
            opmgr_destroy_by_con_id_destroy_callback);

    return TRUE;

}

static unsigned int opmgr_count_ops(CONNECTION_LINK con_id,
                                    OPERATOR_DATA* cur_op)
{
    unsigned int cnt = 0;

    while (cur_op != NULL)
    {
        if (cur_op->con_id == con_id)
        {
            cnt++;
        }
        cur_op = cur_op->next;
    }
    return cnt;
}

static unsigned int opmgr_list_ops(CONNECTION_LINK con_id,
                                   OPERATOR_DATA* cur_op,
                                   unsigned int cnt,
                                   unsigned int *destroy_list)
{
    while (cur_op != NULL)
    {
        if (cur_op->con_id == con_id)
        {
            destroy_list[cnt++] = INT_TO_EXT_OPID(cur_op->id);
        }
        cur_op = cur_op->next;
    }
    return cnt;
}

/****************************************************************************
 *
 * opmgr_destroy_ops_by_con_id
 *
 * N.B. This function possibly needs a different behaviour. At the moment it
 * calls stop on a list of operators if any one rejects the stop it doesn't try
 * and stop any more. The same behaviour happens with the destroy call. This
 * could be changed to call stop and destroy on each operator at a time, ensuring
 * we definitely try to stop and destroy every operator.
 *
 * MULTICORE: handle both P0 and P1 operators. Since it is called with a certain
 * con_id, it means it would only act on operators for that processor specified in
 * the con_id. This then also means it acts on ops local to the OpMgr on that processor!
 * The top 3 bits of con_id receiver and sender are used to denote processor id.
 * Amend the con_id to find operators on processors other than P0.
 */
void opmgr_destroy_ops_by_con_id(CONNECTION_LINK con_id,
                                 OP_CON_ID_CBACK callback,
                                 void *data)
{
    unsigned int cnt = 0;
    op_destroy_info *destroy_list;
#if defined(SUPPORTS_MULTI_CORE)
    PROC_ID_NUM proc_id_index;
#endif

    patch_fn_shared(opmgr);

    PL_PRINT_P0(TR_OPMGR, "Opmgr: Destroy Operator(s) on a given ACCMD connection.\n");

    /* Before we can destroy the operators we need to stop them. So let's make
     * a list of all the operators with the con_id in question.
     *
     * This is pretty crude but lets loop through the list twice so we can define
     * an array of the correct size.
     */
    cnt = opmgr_count_ops(con_id, oplist_head);
#if defined(SUPPORTS_MULTI_CORE)
    /* Include the ops on Px (x=1,2,3) */
    for (proc_id_index=PROC_PROCESSOR_1; proc_id_index<=kip_get_max_processor_id(); proc_id_index++)
    {
        cnt += opmgr_count_ops(PACK_CONID_PROCID(con_id, proc_id_index), remote_oplist_head);
    }
#endif

    if (cnt == 0)
    {
        /* Call the callback directly. No operator needs destroying.*/
        callback(data);
        return;
    }

    /* This is called internally so if this malloc fails all bets are off so
     * permitting a system level malloc call here! Using xppmalloc so a useful
     * panic can be provided. */
    destroy_list = (op_destroy_info *)xppmalloc(sizeof(op_destroy_info) + cnt * sizeof(unsigned int), MALLOC_PREFERENCE_SYSTEM);
    if (NULL == destroy_list)
    {
        panic(PANIC_AUDIO_OPMGR_DESTROY_BY_CONID_NO_RESOURCES);
    }
    destroy_list->count = cnt;

    /* go through the list again and populate the destroy_list */
    cnt = opmgr_list_ops(con_id, oplist_head, 0, destroy_list->op_id_list);
#if defined(SUPPORTS_MULTI_CORE)
    /* Include the ops on Px (x=1,2,3) */
    for (proc_id_index=PROC_PROCESSOR_1; proc_id_index<=kip_get_max_processor_id(); proc_id_index++)
    {
        cnt += opmgr_list_ops(PACK_CONID_PROCID(con_id, proc_id_index), remote_oplist_head, cnt, destroy_list->op_id_list);
    }
#endif

    /* Allocate the callback parameters for the operator list destroy. */
    destroy_by_con_id_callback_parameters *callback_parameters;
    callback_parameters = pnew(destroy_by_con_id_callback_parameters);

    /* save the destroy list for further use. */
    callback_parameters->destroy_op_list = destroy_list;

    /* Save the parameters for the callback function. */
    callback_parameters->callback = callback;
    callback_parameters->data_pointer = data;

    /* Set cnt back to number of operators */
    cnt = destroy_list->count;
    L2_DBG_MSG4("Stop operator list (-1 is the last): 0x%x, 0x%x, 0x%x, 0x%x .../",
            (cnt>=1?destroy_list->op_id_list[0]:(-1)), (cnt>=2?destroy_list->op_id_list[1]:(-1)),
            (cnt>=3?destroy_list->op_id_list[2]:(-1)), (cnt>=4?destroy_list->op_id_list[3]:(-1)));

    /* Save the destroy list so the callback can call destroy on it */
    if (!opmgr_store_in_progress_task(INTERNAL_CLIENT_ID, INTERNAL_CLIENT_ID,
                                (void *)callback_parameters))
    {
        /* There isn't much we can do if this fails but panic */
        panic(PANIC_AUDIO_OPMGR_DESTROY_BY_CONID_NO_RESOURCES);
    }

    /* Initiate stop operator command  */
    opmgr_stop_operator(con_id, destroy_list->count, destroy_list->op_id_list,
            opmgr_destroy_by_con_id_stop_callback);
}

static bool opmgr_validate_message_source(CONNECTION_LINK sender_con_id,
                                          CONNECTION_LINK owner_con_id)
{
    CONNECTION_PEER send_id;

    COMPILE_TIME_ASSERT(OPMGR_ADAPTOR_OBPM == ADAPTOR_OBPM,
            Assumption_about_OPMGR_ADAPTOR_OBPM_in_opmgr_violated);

    send_id = GET_CON_ID_SEND_ID(sender_con_id);
    if ((sender_con_id == owner_con_id) ||
        (GET_SEND_RECV_ID_CLIENT_ID(send_id) == ADAPTOR_OBPM) ||
        (GET_SEND_RECV_ID_CLIENT_ID(send_id) == ADAPTOR_INTERNAL))
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Send message to specified operator.
 *
 * \param con_id     The connection that created the operator.
 * \param op_id      The id of the operator to send a message to.
 * \param num_params The length of the message pointed to by params.
 * \param params     A pointer to the memory containing the message.
 * \param callback   Function pointer to the function to call with the response.
 */
void opmgr_operator_message(CONNECTION_LINK con_id,
                            EXT_OP_ID op_id,
                            unsigned num_params,
                            unsigned *params,
                            OP_MSG_CBACK callback)
{
    OPERATOR_DATA *cur_op;
    CONNECTION_PEER client_id;
    INT_OP_ID int_op_id;
    CONNECTION_LINK reversed_con_id;
    PROC_ID_NUM processor_id;

    patch_fn_shared(opmgr);

    reversed_con_id = REVERSE_CONNECTION_ID(con_id);

    PL_PRINT_P0(TR_OPMGR, "Opmgr: Operator Message.\n");

#if defined(THIRD_PARTY_SECURITY_SUPPORT) && !defined(UNIT_TEST_BUILD)
    if (OPMSG_FRAMEWORK_SET_BDADDR == params[0])
    {
        if (handle_generic_set_bdaddr(con_id, op_id, num_params, params, callback))
        {
            /* Exit 'opmgr_operator_message', callback already called */
            return;
        }
        /* else we fall through to standard opmsg handling */
    }
    /* Not for us, not handled, continue */
#endif

    /* Lookup the operator */
    int_op_id = EXT_TO_INT_OPID(op_id);
    cur_op = get_anycore_op_data_from_id(int_op_id);
    if (cur_op == NULL)
    {
        PL_PRINT_P0(TR_OPMGR, "Can't find operator\n");

        /* Indicate a fail, all other parameters are invalid. */
        callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0, NULL);
        return;
    }

#if defined(SUPPORTS_MULTI_CORE)
    processor_id = cur_op->processor_id;
    if (PROC_PRIMARY_CONTEXT())
    {
        /* Pack the connection id with processor ID */
        con_id = PACK_CONID_PROCID(con_id, processor_id);
        reversed_con_id = REVERSE_CONNECTION_ID(con_id);
    }
#else
    processor_id = PROC_PROCESSOR_0;
#endif

    /* Only the client that created the operator can ask it to do things so
       we check the requestor is the creator before continuing. Exception is
       the special client OBPM that needs to talk "side-ways" to the system. */
    if (!opmgr_validate_message_source(con_id, cur_op->con_id))
    {
        PL_PRINT_P0(TR_OPMGR, "Client does not own operator and it is not OBPM/AOV either.\n");

        callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0, NULL);
        return;
    }

    if (num_params == 0)
    {
        PL_PRINT_P0(TR_OPMGR, "Message is below minimum length\n");

        callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0, NULL);
        return;
    }

    /* Save the callback so we can use it when we get the response.
       Make sure it is saved with the conid we received here, not
       the one stored at creation. If anyone other than creator or special
       super-user like OBPM is talking to us, we don't reach this point. */
    client_id = GET_CON_ID_SEND_ID(con_id);
    if (!opmgr_store_in_progress_task(client_id,
                                      int_op_id,
                                      (void *) callback))
    {
        callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0, NULL);
        return;
    }

    /* If the message is not to local processor, forward the message to
     * remote processor's operators via KIP. We must not end up here if we
     * are on P1 and poking P0 - invalid case, already caught above.*/
    if (!PROC_ON_SAME_CORE(processor_id))
    {
        if (!opmgr_kip_build_send_opmsg(con_id,
                                        op_id,
                                        num_params,
                                        params))
        {
            (void) opmgr_retrieve_in_progress_task(client_id, op_id);
            callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0, NULL);
        }
    }
    else
    {
        /* Otherwise we are on processor local to the operator,
           send the message to the operator. */
        tRoutingInfo new_rinfo;
        unsigned *msg_data = NULL;

        /* Pack the message in a form that the operator expects. Don't send it
         * yet as we need to store the in progress task data first but it's
         * easier to unwind if we do this first. */
        msg_data = xpnewn(num_params + 2, unsigned);
        /* If there isn't enough RAM to send the message fail */
        if (msg_data == NULL)
        {
            (void) opmgr_retrieve_in_progress_task(client_id, op_id);
            callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0, NULL);
            return;
        }
        msg_data[0] = client_id;
        msg_data[1] = num_params;

        /* N.B. the message id in params[0] has already being copied above */
        memcpy(&msg_data[2], params, num_params * sizeof(unsigned));

        /* We use the current connection ID to find sender, it can only be the
        * creator or the admin tool / OBPM. */
        new_rinfo.src_id = client_id;

        /* The receiver id is the opid + remote processor id packed */
        new_rinfo.dest_id = GET_CONID_PACKED_OPID(con_id, int_op_id);

        put_message_with_routing(cur_op->task_id,
                                 OPCMD_MESSAGE,
                                 msg_data,
                                 &new_rinfo);
    }
}

/****************************************************************************
 *
 * opmgr_get_capid_list
 * A case of a purely OpMgr command that doesn't go to operators.
  *
 * TODO MULTICORE: If secondary proc may have different supported capabilities, this has to be
 * processor-specific - in which case con_id has to be accurately passing processor ID, too.
 * So command i/f needs to carry proc ID in that case, and adaptor will put this into con_id when calls this.
 */
void opmgr_get_capid_list(CONNECTION_LINK con_id,
                          CAP_INFO_LIST_CBACK callback,
                          unsigned start_index,
                          unsigned max_count)
{
    CONNECTION_LINK reversed_con_id;
    unsigned *capid_list;
    unsigned num_capids, static_num_caps, total_num_caps;
    unsigned num_capids_skip = 0, num_missing_capids = 0;
    unsigned i;

    patch_fn_shared(opmgr);

    reversed_con_id = REVERSE_CONNECTION_ID(con_id);
    for (static_num_caps = 0; capability_data_table[static_num_caps] != NULL; static_num_caps++);
    total_num_caps = static_num_caps;

    total_num_caps += get_num_download_caps();

    /* No capabilities to report. */
    if ((total_num_caps == 0) || (start_index >= total_num_caps))
    {
        callback(reversed_con_id, STATUS_OK, total_num_caps, 0, NULL);
        return;
    }

    /* Limit number of capabilities to report. */
    if (start_index + max_count >= total_num_caps)
    {
        num_capids = total_num_caps - start_index;
    }
    else
    {
        num_capids = max_count;
    }

    capid_list = xpnewn(num_capids, unsigned int);
    if (NULL == capid_list)
    {
        callback(reversed_con_id, STATUS_CMD_FAILED, total_num_caps, 0, NULL);
        return;
    }

    /* Copy the id of the relevant built-in capabilities. */
    for (i = 0; (i < num_capids) && ((start_index + i) < static_num_caps); i++)
    {
        capid_list[i] = capability_data_table[start_index + i]->id;
    }
    num_missing_capids = num_capids - i;

    if (start_index > static_num_caps)
    {
        num_capids_skip = start_index - static_num_caps;
    }

    if (num_missing_capids > 0)
    {
        add_download_cap_to_list(&capid_list[i], num_missing_capids, num_capids_skip);
    }

    /* Pass info to callback with reversed connection ID */
    callback(reversed_con_id, STATUS_OK, total_num_caps, num_capids, capid_list);

    pfree(capid_list);
}


/****************************************************************************
 *
 * opmgr_get_opid_list
 * A case of a purely OpMgr command that doesn't go to operators.
 *
 * TODO MULTICORE: If secondary proc may have different supported capabilities, this has to be
 * processor-specific - in which case con_id has to be accurately passing processor ID, too.
 * Also, then look up only in local oplist or also in remote_oplist, depending on what convention
 * we keep for this. Cleanest would be this to only act for a specific processor's ops? But then
 * command i/f needs to pass in the processor ID to be put into conn_id by adaptor.
 */
void opmgr_get_opid_list(CONNECTION_LINK con_id,
                         CAP_ID capid,
                         OPID_LIST_CBACK callback,
                         unsigned num_skipped_ops,
                         unsigned max_count)
{
    CONNECTION_LINK reversed_con_id;
    unsigned *cap_op_list = NULL;
    unsigned num_ops_in_results; /* Total number of filtered operators minus number of skipped operators and capped to max_count. */
    unsigned count = 0, i = 0, j;
    unsigned local_search_ops =0;

    OPERATOR_DATA* op_data = NULL;
    bool found = FALSE;

    patch_fn_shared(opmgr);

    unsigned local_ops = opmgr_get_ops_count(capid);
    unsigned remote_ops = opmgr_get_remote_ops_count(capid); /* count how many operators are created with this capid or all (if capid == 0) */
    unsigned total_filtered_ops = local_ops + remote_ops; /* Total number of operator minus the operators that do not match the search criteria. */

    reversed_con_id = REVERSE_CONNECTION_ID(con_id);

    /* No operators to report. */
    if ((total_filtered_ops == 0) || (num_skipped_ops >= total_filtered_ops))
    {
        callback(reversed_con_id, STATUS_OK, total_filtered_ops, 0, NULL);
        return;
    }

    /* Limit number of operators to report. */
    if (num_skipped_ops + max_count >= total_filtered_ops)
    {
        num_ops_in_results = total_filtered_ops - num_skipped_ops;
    }
    else
    {
        num_ops_in_results = max_count;
    }

    /* Allocate array of cap ID and op ID pairs */
    cap_op_list = xpmalloc(num_ops_in_results*sizeof(unsigned)*2);
    if (NULL == cap_op_list)
    {
        callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0, NULL);
        return;
    }

    /* build the joint list - not checking for now the fields it needs, if those are not correct, */
    /* something truly nasty happened - possibly later put some checks and panic or error code */

    /* The function relies on the fact that no operators will be destroyed while
       it is running. The outer loop counts at most the number of filtered results
       which is always inferior or equal to the total number of operators. */

    /* Deal with local list first */
    if (local_ops > 0)
    {
        op_data = oplist_head;

        local_search_ops = ((num_ops_in_results + num_skipped_ops) >= local_ops) ? \
                           local_ops : (num_ops_in_results + num_skipped_ops);

        for (j=0; j<local_search_ops; j++)
        {
            found = FALSE;
            do
            {
                PL_ASSERT(op_data!=NULL);
                /* Once all results to skip have been discarded, the block in the
                   inner if clause copy the operators that match the filter. */
                if ((capid == 0) || (op_data->cap_data->id == capid))
                {
                    if (count >= num_skipped_ops)
                    {
                        cap_op_list[i++] = INT_TO_EXT_OPID(op_data->id);
                        cap_op_list[i++] = op_data->cap_data->id;
                    }
                    count++;
                    found = TRUE;
                }
                op_data = op_data->next;
            } while(!found);
         }
    }

    /* We also need to search the remote list */
    if (remote_ops > 0)
    {
        op_data = remote_oplist_head;

        for (j = 0; j< num_ops_in_results+num_skipped_ops-local_search_ops; j++)
        {
            found = FALSE;
            do
            {
                PL_ASSERT(op_data!=NULL);
                /* Once all results to skip have been discarded, the block in the
                   inner if clause copy the operators that match the filter. */
                if ((capid == 0) || (op_data->cap_data->id == capid))
                {
                    if (count >= num_skipped_ops)
                    {
                        cap_op_list[i++] = INT_TO_EXT_OPID(op_data->id);
                        cap_op_list[i++] = op_data->cap_data->id;
                    }
                    count++;
                    found = TRUE;
                }
                op_data = op_data->next;
            } while (!found);
        }
    }

    /* No operators was found, though we suppose to have some operators returned. */
    PL_ASSERT(count!=0);

    /* Pass info to callback with reversed connection ID */
    callback(reversed_con_id, STATUS_OK, total_filtered_ops, num_ops_in_results, cap_op_list);
    pfree(cap_op_list);

}

/**
 * \brief Handle the unsolicited message from the operator
 *
 * \param con_id       Connection id
 * \param processor_id Processor id where the operator is created
 * \param msg_from_op  Unsolicited messsage from the operator
 */
bool opmgr_unsolicited_message(CONNECTION_LINK con_id,
                               PROC_ID_NUM processor_id,
                               OP_UNSOLICITED_MSG *msg_from_op)
{
    tRoutingInfo rinfo;

    /* Something has gone wrong if we are not running on the same core as
     * the operator is being created */
    PL_ASSERT(PROC_ON_SAME_CORE(processor_id));

    if (processor_id != PROC_PROCESSOR_0)
    {
        /* Forward the message to the primary processor. */
        return opmgr_kip_unsolicited_message(REVERSE_CONNECTION_ID(con_id),
                                             msg_from_op);
    }

    rinfo.src_id = msg_from_op->op_id;
    rinfo.dest_id = msg_from_op->client_id;

    /* put message on OpMgr queue - needs the static OpMgr queue ID */
    put_message_with_routing(OPMGR_TASK_QUEUE_ID,
                             OPCMD_FROM_OPERATOR,
                             (OP_UNSOLICITED_MSG *) msg_from_op,
                             &rinfo);

    return TRUE;
}

#ifdef INSTALL_CAP_DOWNLOAD_MGR
/****************************************************************************
 *
 * opmgr_install_capability
 * This function tells OpMgr that a new capability has to be added to the list of available capabilities. If
 * the capability already exists, it will return an error. Once a capability has been installed, it can be
 * operated with as usual (create, start, stop, reset, etc.)
 */
bool opmgr_install_capability(CAPABILITY_DATA* cap_info, bool is_on_both_cores)
{
    return opmgr_add_cap_download_data(cap_info, is_on_both_cores);
}

/****************************************************************************
 *
 * opmgr_uninstall_capability
 * This function tells OpMgr that a capability previously installed has to be removed from the list of
 * available capabilities. The capability must be in installed state, otherwise it would send an error
 * If the capability does not exist, it will return an error as well. Once a capability has been uninstalled, it
 * cannot be operated with anymore.
 */
bool opmgr_uninstall_capability(CAP_ID cap_id)
{
    return opmgr_remove_cap_download_data(cap_id);
}
#endif /* INSTALL_CAP_DOWNLOAD_MGR */







