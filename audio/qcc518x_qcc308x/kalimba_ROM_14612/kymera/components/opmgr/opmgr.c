/****************************************************************************
 * Copyright (c) 2011 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr.c
 * \ingroup  opmgr
 *
 * Operator Manager main file. <br>
 * This file contains the operator manager functionality <br>
 */

/****************************************************************************
Include Files
*/

#include "opmgr_private.h"

/****************************************************************************
Private Type Declarations
*/

#ifdef PROFILER_ON
struct get_mips_usage_req_info
{
    CONNECTION_LINK con_id;
    /* op_list is the list of operator IDs in the request. */
    uint16 *op_list;
    /* mips_usage is the list of MIPS usage for the respective operator in op_list.*/
    uint16 *mips_usage;
    /* The callback that sends the ACCMD response after the request was dealt with.*/
    get_mips_usage_cback mips_usage_callback;
    /* The number of operator IDs in the ACCMD GET_MIPS_USAGE_REQ request. */
    unsigned list_length;
};

typedef struct get_mips_usage_req_info MIPS_USAGE_REQ_INFO;
#endif /* PROFILER_ON */
/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Macro Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

/** Pointer to head of "local" operators list. If multicore, then all OpMgrs
 *  running on P0 and secondary Pn processors will see this in their own private DRAMs.
 */
OPERATOR_DATA* oplist_head = NULL;

/* Pointer to head of 'remote' operators list - only used by P0 OpMgr.
 * If separate images are built, then this is compiled out in P1, otherwise in
 * the case of common image, it will be present in P1 but not used.
 */
#if defined(SUPPORTS_MULTI_CORE)
/** P0 book keeping of remote ops on P1, this is not the same as P1 local op list!
 *  Unlike local op lists, this list will only point to 'core' op data objects,
 *  as operator extra data is not allocated on P0 for remote ops.
 */
DM_P0_RW_ZI OPERATOR_DATA* remote_oplist_head = NULL;

static OPERATOR_DATA* find_op_data_in_list(INT_OP_ID id,
                                           OPERATOR_DATA *list_head);
#endif /* SUPPORTS_MULTI_CORE */

#ifdef PROFILER_ON
DM_SHARED MIPS_USAGE_REQ_INFO *get_mips_req_info = NULL;
#endif /* PROFILER_ON */

/****************************************************************************
Private Function Declarations
*/

/****************************************************************************
Private Function Definitions
*/

#ifdef PROFILER_ON

/* Frees the structures used for the GET_MIPS_USAGE_REQ */
static void opmgr_free_get_mips_usage_helper_vars(void)
{
    pfree(get_mips_req_info->op_list);
    pfree(get_mips_req_info->mips_usage);

    pfree(get_mips_req_info);

    get_mips_req_info = NULL;

}

#if defined(SUPPORTS_MULTI_CORE)
static bool opmgr_send_get_mips_req(CONNECTION_LINK con_id)
{
    return opmgr_kip_send_get_mips_req(con_id);
}
#else
static bool opmgr_send_get_mips_req(CONNECTION_LINK con_id)
{
    return TRUE;
}
#endif /* SUPPORTS_MULTI_CORE */
#endif /* PROFILER_ON */

#if (defined(SUPPORTS_MULTI_CORE) && defined(INSTALL_CAP_DOWNLOAD_MGR))
static bool check_op_availability(CAP_DOWNLOAD_PROCESSOR existing_cap,
                                  CAP_DOWNLOAD_PROCESSOR new_cap)
{
    patch_fn_shared(opmgr);
    if ((existing_cap == P0_ONLY && new_cap == P1_ONLY) ||
        (existing_cap == P1_ONLY && new_cap == P0_ONLY))
    {
        return TRUE;
    }

    return FALSE;
}

static bool check_if_correct_downloaded_cap(DOWNLOAD_CAP_DATA_DB *download_cap,
                                            PROC_ID_NUM processor_id)
{
    patch_fn_shared(opmgr);

    if ((download_cap->available_on == P0_ONLY && processor_id == PROC_PROCESSOR_0) ||
        (download_cap->available_on == P1_ONLY && processor_id == PROC_PROCESSOR_1) ||
        (download_cap->available_on == P0_AND_P1))
    {
        return TRUE;
    }

    return FALSE;
}
#endif /* SUPPORTS_MULTI_CORE && INSTALL_CAP_DOWNLOAD_MGR*/
/**
 * \brief Finds all operators connected to the source ep of operator A and request that
 * they no longer kick A.
 *
 * \param this_op The operator to that doesn't want to recieve kicks from
 * other operators any more.
 */
static void opmgr_stop_sources_being_kicked(OPERATOR_DATA *this_op)
{
    unsigned i;
    /* Go through all the connections and remove the connection from the
     * kick propagation table of the other operator */
    for (i = 0; i < this_op->cap_data->max_sources; i++)
    {
        ENDPOINT *ep = stream_get_connected_ep_from_id(opmgr_create_endpoint_id(INT_TO_EXT_OPID(this_op->id), i, SOURCE));

        if (ep != NULL)
        {
            unsigned ep_id = stream_ep_id_from_ep(ep);

            if ((ep_id & STREAM_EP_OP_BIT) != 0)
            {
                opmgr_kick_prop_table_remove(ep_id);
            }
            else
            {
                stream_disable_kicks_from_endpoint(ep);
            }

            /* The operator could be kicked by the tail sink endpoint (operator) of the
             * in place chain. */
            in_place_cancel_tail_kick(ep);
        }
    }
}


/**
 * \brief Finds all operators connected to the sink ep of operator A and request that
 * they no longer kick A.
 *
 * \param this_op The operator to that doesn't want to recieve kicks from
 * other operators any more.
 */
static void opmgr_stop_sinks_being_kicked(OPERATOR_DATA *this_op)
{
    unsigned i;
    /* Go through all the connections and remove the connection from the
     * kick propagation table of the other operator */
    for (i=0; i < this_op->cap_data->max_sinks; i++)
    {
        ENDPOINT *ep = stream_get_connected_ep_from_id(opmgr_create_endpoint_id(INT_TO_EXT_OPID(this_op->id), i, SINK));

        if (ep != NULL)
        {
            unsigned ep_id = stream_ep_id_from_ep(ep);

            if ((ep_id & STREAM_EP_OP_BIT) != 0)
            {
                opmgr_kick_prop_table_remove(ep_id);
            }
            else
            {
                stream_disable_kicks_from_endpoint(ep);
            }
        }
    }
}

/* \brief Count the number of operators with matching capability in a given list of operators.
 *        If capability ID is zero, count all operators.
 */
static unsigned opmgr_get_ops_count_in_oplist(CAP_ID capid,
                                              OPERATOR_DATA* oplist)
{
    OPERATOR_DATA* cur_op;
    unsigned int op_count = 0;

    /* Patch point */
    patch_fn_shared(opmgr);

    /* Several callers will need a count of operators for a certain cap ID or count of all */
    /* operators. So pulling this down to this level, so all callers don't duplicate code. */
    /* For a zero capid, it will return number of all operators that are instantiated. */
    for( cur_op = oplist; cur_op != NULL; cur_op = cur_op->next )
    {
        if ( (capid == 0) || ((capid != 0) && (capid == cur_op->cap_data->id)) )
        {
            op_count++;
        }
    }
    return op_count;
}

#ifdef INSTALL_CAP_DOWNLOAD_MGR

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief Get the entry in the downloadable capability database that
 *        matches the specified capability id.
 *
 * \param cap_id The identifier of the capability to look for.
 * \param processor_id The core the capability is on
 *
 * \return A pointer to the entry if found, NULL otherwise.
 */
static DOWNLOAD_CAP_DATA_DB *opmgr_get_cap_db_entry(CAP_ID cap_id, PROC_ID_NUM processor_id)
{
    DOWNLOAD_CAP_DATA_DB *cap_download_data_ptr;

    patch_fn_shared(opmgr);

    if (cap_download_data_list == NULL)
    {
        return NULL;
    }

    cap_download_data_ptr = *cap_download_data_list;
    while (cap_download_data_ptr != NULL)
    {
        /* Find the capability with id cap_id */
        if (cap_download_data_ptr->cap->id == cap_id)
        {
            if (check_if_correct_downloaded_cap(cap_download_data_ptr,
                                                processor_id))
            {
                return cap_download_data_ptr;
            }
        }
        cap_download_data_ptr = cap_download_data_ptr->next;
    }

    /* Capability not found */
    return NULL;
}
#else
/**
 * \brief Get the entry in the downloadable capability database that
 *        matches the specified capability id.
 *
 * \param cap_id The identifier of the capability to look for.
 *
 * \return A pointer to the entry if found, NULL otherwise.
 */
static DOWNLOAD_CAP_DATA_DB *opmgr_get_cap_db_entry(CAP_ID cap_id)
{
    DOWNLOAD_CAP_DATA_DB *cap_download_data_ptr;

    patch_fn_shared(opmgr);

    if (cap_download_data_list == NULL)
    {
        return NULL;
    }

    cap_download_data_ptr = *cap_download_data_list;
    while (cap_download_data_ptr != NULL)
    {
        /* Find the capability with id cap_id */
        if (cap_download_data_ptr->cap->id == cap_id)
        {
            return cap_download_data_ptr;
        }
        cap_download_data_ptr = cap_download_data_ptr->next;
    }

    /* Capability not found */
    return NULL;
}
#endif /* SUPPORTS_MULTI_CORE */

#endif /* INSTALL_CAP_DOWNLOAD_MGR */

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Dummy init function needed by all static tasks.
 */
void opmgr_task_init(void **data)
{
}

#if defined(SUPPORTS_MULTI_CORE)
const CAPABILITY_DATA *opmgr_lookup_cap_data_for_cap_id(CAP_ID cap_id, PROC_ID_NUM processor_id)
{
    unsigned i;
    unsigned num_caps;

    patch_fn_shared(opmgr);

    /* TODO: for the downloadable capabilities, this mechanism to move to a more
       dynamic one, where the static database PLUS any downloaded stuff gets
       counted nicely. */
    for (num_caps = 0; capability_data_table[num_caps] != NULL; num_caps++);

    PL_PRINT_P1(TR_OPMGR, "Number of caps in cap data table: %u \n", num_caps);

    for (i=0; i < num_caps; i++)
    {
        if (capability_data_table[i]->id == cap_id)
        {
            return capability_data_table[i];
        }
    }

#ifdef INSTALL_CAP_DOWNLOAD_MGR
    DOWNLOAD_CAP_DATA_DB *cap_download_data_ptr;

    /* opmgr_get_cap_db_entry() assumes that it's called on the same core the
     * operator is to be created on. */
    cap_download_data_ptr = opmgr_get_cap_db_entry(cap_id, processor_id);
    if (cap_download_data_ptr != NULL)
    {
        return cap_download_data_ptr->cap;
    }
#endif

    return NULL;
}
#else
const CAPABILITY_DATA *opmgr_lookup_cap_data_for_cap_id(CAP_ID cap_id)
{
    unsigned i;
    unsigned num_caps;

    patch_fn_shared(opmgr);

    /* TODO: for the downloadable capabilities, this mechanism to move to a more
       dynamic one, where the static database PLUS any downloaded stuff gets
       counted nicely. */
    for (num_caps = 0; capability_data_table[num_caps] != NULL; num_caps++);

    PL_PRINT_P1(TR_OPMGR, "Number of caps in cap data table: %u \n", num_caps);

    for (i=0; i < num_caps; i++)
    {
        if (capability_data_table[i]->id == cap_id)
        {
            return capability_data_table[i];
        }
    }

#ifdef INSTALL_CAP_DOWNLOAD_MGR
    DOWNLOAD_CAP_DATA_DB *cap_download_data_ptr;

    /* opmgr_get_cap_db_entry() assumes that it's called on the same core the
     * operator is to be created on. */
    cap_download_data_ptr = opmgr_get_cap_db_entry(cap_id);
    if (cap_download_data_ptr != NULL)
    {
        return cap_download_data_ptr->cap;
    }
#endif

    return NULL;
}
#endif /* SUPPORTS_MULTI_CORE */
#ifdef INSTALL_CAP_DOWNLOAD_MGR
bool opmgr_add_cap_download_data(CAPABILITY_DATA *cap_data, bool is_on_both_cores)
{
    DOWNLOAD_CAP_DATA_DB *cap_download_data_ptr;
    DOWNLOAD_CAP_DATA_DB **cap_download_data_end;

    patch_fn_shared(opmgr);

#if defined(SUPPORTS_MULTI_CORE)
    CAP_DOWNLOAD_PROCESSOR available_on;
    if (is_on_both_cores == TRUE)
    {
        available_on = P0_AND_P1;
    }
    else
    {
        available_on = (CAP_DOWNLOAD_PROCESSOR)(proc_get_processor_id());
    }
#endif /* SUPPORTS_MULTI_CORE */
    /* Find the end of the list if it is not empty. */
    cap_download_data_end = cap_download_data_list;
    if (cap_download_data_list != NULL)
    {
        cap_download_data_ptr = *cap_download_data_list;
        while (cap_download_data_ptr != NULL)
        {
            /* Save address where to store new entry */
            cap_download_data_end = &(cap_download_data_ptr->next);
            /* Check it doesn't exist already */
            if (cap_download_data_ptr->cap->id == cap_data->id)
            {
#if defined(SUPPORTS_MULTI_CORE)
                if (!check_op_availability(cap_download_data_ptr->available_on,
                                           available_on))
                {
                    return FALSE;
                }
#else
                return FALSE;
#endif /* SUPPORTS_MULTI_CORE */
            }
            cap_download_data_ptr = cap_download_data_ptr->next;
        }
    }

    /* Create a new entry */
    cap_download_data_ptr = xppmalloc(sizeof(DOWNLOAD_CAP_DATA_DB),
                                      MALLOC_PREFERENCE_SHARED);
    if (cap_download_data_ptr == NULL)
    {
        return FALSE;
    }
    cap_download_data_ptr->cap = cap_data;
    cap_download_data_ptr->status = CAP_INSTALLED;
    cap_download_data_ptr->next = NULL;
#if defined(SUPPORTS_MULTI_CORE)
    cap_download_data_ptr->available_on = available_on;
#endif /* SUPPORTS_MULTI_CORE */

    /* And add it at the end of the list. */
    *cap_download_data_end = cap_download_data_ptr;
    return TRUE;
}

bool opmgr_remove_cap_download_data(CAP_ID cap_id)
{
    DOWNLOAD_CAP_DATA_DB **cap_download_data_tmp;
    DOWNLOAD_CAP_DATA_DB *cap_download_data_ptr;
#if defined (SUPPORTS_MULTI_CORE)
    PROC_ID_NUM processor_id;
#endif /* SUPPORTS_MULTI_CORE */
    patch_fn_shared(opmgr);

    if (cap_download_data_list == NULL)
    {
        return FALSE;
    }

#ifdef SUPPORTS_MULTI_CORE
    /* The remove functions is always called on the core the capability
     * is downloaded on. */
    processor_id = proc_get_processor_id();
#endif /* SUPPORTS_MULTI_CORE */
    cap_download_data_ptr = *cap_download_data_list;
    cap_download_data_tmp = cap_download_data_list;
    while (cap_download_data_ptr != NULL)
    {
        /* Find the capability with id cap_id */
        if (cap_download_data_ptr->cap->id == cap_id)
        {
#if defined (SUPPORTS_MULTI_CORE)
            if (check_if_correct_downloaded_cap(cap_download_data_ptr,
                                                processor_id))
#endif /* SUPPORTS_MULTI_CORE */
            {
                *cap_download_data_tmp = cap_download_data_ptr->next;
                pfree(cap_download_data_ptr);
                return TRUE;
            }
        }
        else
        {
            cap_download_data_tmp = &(cap_download_data_ptr->next);
            cap_download_data_ptr = cap_download_data_ptr->next;
        }
    }
    /* Capability not found */
    return FALSE;
}

bool opmgr_get_download_cap_status(CAP_ID cap_id, CAP_DOWNLOAD_STATUS *status)
{
    DOWNLOAD_CAP_DATA_DB *cap_download_data_ptr;
#if defined(SUPPORTS_MULTI_CORE)
    PROC_ID_NUM processor_id;

    /* This function is called during the download process, therefore the
     * processor_id is always the local one. */
    processor_id = proc_get_processor_id();
    cap_download_data_ptr = opmgr_get_cap_db_entry(cap_id, processor_id);
#else
    cap_download_data_ptr = opmgr_get_cap_db_entry(cap_id);
#endif /* SUPPORTS_MULTI_CORE */
    if (cap_download_data_ptr != NULL)
    {
        *status = cap_download_data_ptr->status;
        return TRUE;
    }

    return FALSE;
}

bool opmgr_set_download_cap_status(CAP_ID cap_id, CAP_DOWNLOAD_STATUS status)
{
    DOWNLOAD_CAP_DATA_DB *cap_download_data_ptr;

#if defined(SUPPORTS_MULTI_CORE)
    PROC_ID_NUM processor_id;

    /* This function is called during the download process, therefore the
     * processor_id is always the local one. */
    processor_id = proc_get_processor_id();
    cap_download_data_ptr = opmgr_get_cap_db_entry(cap_id, processor_id);
#else
    cap_download_data_ptr = opmgr_get_cap_db_entry(cap_id);
#endif /* SUPPORTS_MULTI_CORE */
    if (cap_download_data_ptr != NULL)
    {
        cap_download_data_ptr->status = status;
        return TRUE;
    }

    return FALSE;
}

bool opmgr_cap_is_instantiated(CAP_ID cap_id)
{
    if (opmgr_get_ops_count(cap_id) + opmgr_get_remote_ops_count(cap_id) > 0)
    {
        return TRUE;
    }
    return FALSE;
}
#endif /* INSTALL_CAP_DOWNLOAD_MGR */

/****************************************************************************
 *
 * opmgr_get_op_capid
 *
 */
unsigned int opmgr_get_op_capid(unsigned int ep_id)
{
    /* Look in local oplist. It actually is not used
     * by Streams or anyone currently.
     */
    OPERATOR_DATA *cur_op = get_op_data_from_id(get_opid_from_opidep(ep_id));
    if (cur_op == NULL)
    {
        /* This request only ever comes from Streams, so we should panic if we
         *  can't find the relevant operator. */
        PL_PRINT_P0(TR_OPMGR, "opmgr_get_op_capid: can't find operator\n");
        panic_diatribe(PANIC_AUDIO_OPERATOR_NOT_FOUND, ep_id);
    }

    return cur_op->cap_data->id;
}

/****************************************************************************
 *
 * opmgr_get_capid_from_opid
 *
 */
bool opmgr_get_capid_from_opid(EXT_OP_ID opid, CAP_ID *capid)
{
    OPERATOR_DATA *cur_op;
    INT_OP_ID int_op_id;

    /* Look in local oplist. */
    int_op_id = EXT_TO_INT_OPID(opid);
    cur_op = get_op_data_from_id(int_op_id);
    if (cur_op == NULL)
    {
        /* no operator with this opid found */
        return FALSE;
    }

    /* operator found, get the cap id from operator data */
    *capid =  cur_op->cap_data->id;
    return TRUE;
}

/****************************************************************************
 *
 * opmgr_get_ops_count
 *
 */
unsigned int opmgr_get_ops_count(CAP_ID capid)
{
    return opmgr_get_ops_count_in_oplist(capid, oplist_head);
}

#if defined(SUPPORTS_MULTI_CORE)
unsigned int opmgr_get_remote_ops_count(CAP_ID capid)
{
    return (PROC_PRIMARY_CONTEXT() ?
             opmgr_get_ops_count_in_oplist(capid, remote_oplist_head) : 0);
}

unsigned int opmgr_get_list_remote_ops_count(unsigned int num_ops,
                                             EXT_OP_ID *op_list,
                                             PROC_ID_NUM proc_id)
{
    unsigned int i;
    INT_OP_ID id;
    unsigned int n = 0;
    OPERATOR_DATA* entry;

    /* Patch point */
    patch_fn_shared(opmgr);

    for (i=0; i<num_ops; i++)
    {
        id = EXT_TO_INT_OPID(op_list[i]);
        entry = find_op_data_in_list(id, remote_oplist_head);
        n += ((entry != NULL)&&(entry->processor_id==proc_id));
    }

    return n;
}

PROC_ID_NUM opmgr_get_processor_id_from_opid(EXT_OP_ID ext_opid)
{
    OPERATOR_DATA* op_data;

    op_data = get_anycore_op_data_from_id(EXT_TO_INT_OPID(ext_opid));
    if (op_data == NULL)
    {
        return PROC_PROCESSOR_INVALID;
    }

    return op_data->processor_id;
}

PROC_ID_NUM opmgr_get_processor_id(unsigned ep_id)
{
    OPERATOR_DATA* op_data;

    op_data = get_anycore_op_data_from_id(get_opid_from_opidep(ep_id));
    if (op_data == NULL)
    {
        return PROC_PROCESSOR_INVALID;
    }

    return op_data->processor_id;
}

/*
 * As it stands, it only gets op data from remote list of ops
 * (it only exists on P0 when separate images are built).
 * If we are on secondary core, and this gets called, it returns NULL.
 */
OPERATOR_DATA* get_remote_op_data_from_id(INT_OP_ID id)
{
    return (PROC_PRIMARY_CONTEXT() ? find_op_data_in_list(id, remote_oplist_head) : NULL);
}
#endif /* defined(SUPPORTS_MULTI_CORE) */

/****************************************************************************
 *
 * opmgr_does_op_exist
 *
 */
bool opmgr_does_op_exist(void* op_data)
{
    OPERATOR_DATA* temp;

    patch_fn_shared(opmgr);

    if(op_data != NULL)
    {
        /**
         * Only look in local oplist. To look in oplists on multiple
         * cores, create a new function, but currently nothing needs
         * to use such a function.
         */
        for(temp = oplist_head; temp != NULL; temp = temp->next)
        {
            if (op_data == temp)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/****************************************************************************
 *
 * opmgr_get_op_task_from_epid
 *
 */
BGINT_TASK opmgr_get_op_task_from_epid(unsigned opidep)
{
    /* Only look at local list, as long as this is only called on OpMgr that
     * has tasks for local operators. Remote ones are kicked/poked differently,
     * not having tasks in OpMgr that is not local to those operators.
     */
    OPERATOR_DATA *cur_op = get_op_data_from_id(get_opid_from_opidep(opidep));

    if (cur_op)
    {
        BGINT_TASK bg;
        if (sched_find_bgint(cur_op->task_id, &bg))
        {
            return bg;
        }
    }

    return NULL;
}

/****************************************************************************
 *
 * opmgr_create_endpoint_id
 *
 */
unsigned int opmgr_create_endpoint_id(EXT_OP_ID opid,
                                      unsigned int idx,
                                      ENDPOINT_DIRECTION dir)
{
    unsigned int type;
    patch_fn_shared(opmgr);

    /* Validate opid */
    if ((opid & STREAM_EP_TYPE_MASK) != STREAM_EP_OP_BIT ||
        (opid & STREAM_EP_CHAN_MASK) != 0)
    {
        /* opid is invalid */
        PL_PRINT_P0(TR_STREAM, "opmgr_create_endpoint_id: opid is invalid\n");
        return 0;
    }

    /* Validate idx */
    if ((idx & ~STREAM_EP_CHAN_MASK) != 0)
    {
        /* idx is invalid */
        PL_PRINT_P0(TR_STREAM, "opmgr_create_endpoint_id: idx is invalid\n");
        return 0;
    }

    /* Derive operator type bits from direction */
    type = (dir == SOURCE) ? STREAM_EP_OP_SOURCE : STREAM_EP_OP_SINK;

    /* Generate the external operator endpoint id */
    return (type | opid | idx);
}

void opmgr_stop_kicks(INT_OP_ID op_id, STOP_KICK side)
{
    OPERATOR_DATA *cur_op;
    patch_fn_shared(opmgr);

    /* Look in local op lists: judging from its use, i.e. on graphs,
     * it should  only look in local list - Stream and OpMgr will act
     * on actual ops in their own (local) graph.
     */
    cur_op = get_op_data_from_id(op_id);

    if (cur_op == NULL)
    {
        /* This request only ever comes from Streams, so we should panic if we
         *  can't find the relevant operator. */
        PL_PRINT_P0(TR_OPMGR, "Can't find op_id!\n");
        panic_diatribe(PANIC_AUDIO_OPERATOR_NOT_FOUND, op_id);
    }
    else
    {
        cur_op->stop_chain_kicks = side;
        /* If there are already other operators connected to this operator then
         * we need to tell all of them to stop kicking this capability. */
        if (side == SOURCE_SIDE)
        {
            /* disable the sources being kicked. */
            opmgr_stop_sources_being_kicked(cur_op);
        }
        else if (side == SINK_SIDE)
        {
            /* disable the sinks being kicked. */
            opmgr_stop_sinks_being_kicked(cur_op);
        }
        else /* BOTH_SIDES */
        {
            opmgr_stop_sources_being_kicked(cur_op);
            opmgr_stop_sinks_being_kicked(cur_op);
        }
    }
}

/****************************************************************************
*
* opmgr_get_num_sink_terminals
*
*/
unsigned opmgr_get_num_sink_terminals(EXT_OP_ID opid)
{
    OPERATOR_DATA *cur_op;
    INT_OP_ID int_op_id;
    unsigned num_sinks = 0;

    /*
     * Stream and OpMgr will act on
     * actual ops in their own (local) graph.
     */
    int_op_id = EXT_TO_INT_OPID(opid);
    cur_op = get_anycore_op_data_from_id(int_op_id);
    if (cur_op != NULL)
    {
        num_sinks = cur_op->cap_data->max_sinks;
    }

    return num_sinks;
}

unsigned opmgr_get_num_source_terminals(EXT_OP_ID opid)
{
    OPERATOR_DATA *cur_op;
    INT_OP_ID int_op_id;
    unsigned num_sources = 0;

    /*
     * Stream and OpMgr will act on
     * actual ops in their own (local) graph.
     */
    int_op_id = EXT_TO_INT_OPID(opid);
    cur_op = get_anycore_op_data_from_id(int_op_id);
    if (cur_op != NULL)
    {
        num_sources = cur_op->cap_data->max_sources;
    }

    return num_sources;
}

#ifdef PROFILER_ON
/**
 * opmgr_handle_get_operator_mips_req
 */
bool opmgr_handle_get_operator_mips_req(CONNECTION_LINK con_id,
                                        uint16 *oplist,
                                        unsigned length,
                                        get_mips_usage_cback callback)
{
    bool op_on_p1;

    get_mips_req_info = xzppnew(MIPS_USAGE_REQ_INFO, MALLOC_PREFERENCE_SHARED);
    if (get_mips_req_info == NULL)
    {
        return FALSE;
    }
    /* Initialise local and global variables. */
    get_mips_req_info->con_id = con_id;
    get_mips_req_info->list_length = length;
    get_mips_req_info->mips_usage_callback = callback;
    
    /* Allocate the globals op_list and mips_usage (freed when the callback is
     * called).
     * op_list stores all the operator IDs in the request.
     * mips_usage stores the MIPS consumed by each operator in the request, in 
     * the order of op IDs in the request. If an operator (or more) is on P1,
     * a request is sent to P1 to get the MIPS for its operator(s).
     */
    get_mips_req_info->op_list = xzppnewn(length, uint16, MALLOC_PREFERENCE_SHARED);
    /* Return if allocation failed. */
    if (get_mips_req_info->op_list == NULL)
    {
        pfree(get_mips_req_info);
        return FALSE;
    }
    get_mips_req_info->mips_usage = xzppnewn(length, uint16, MALLOC_PREFERENCE_SHARED);
    /* Return if allocation failed. */
    if (get_mips_req_info->mips_usage == NULL)
    {
        pfree(get_mips_req_info->op_list);
        pfree(get_mips_req_info);
        return FALSE;
    }

    /* Copy the operator list. */
    memcpy(get_mips_req_info->op_list, oplist, sizeof(uint16) * length);

    op_on_p1 = opmgr_get_operator_mips();

    if (op_on_p1)
    {
        /* Send a KIP message to P1 to request for MIPS usage for the ops on P1. */
        if (!opmgr_send_get_mips_req(con_id))
        {
            opmgr_free_get_mips_usage_helper_vars();
            return FALSE;
        }
    }
    else
    {
        /* No operators on P1, therefore call the callback and free any memory. */
        opmgr_send_get_mips_usage_resp();
    }

    return TRUE;
}

/**
 * opmgr_get_operator_mips
 */
bool opmgr_get_operator_mips(void)
{
    OPERATOR_DATA *op;
    bool op_not_on_local_core;
    unsigned i;
    
    op_not_on_local_core = FALSE;

    for (i = 0; i < get_mips_req_info->list_length; i++)
    {
        op = get_anycore_op_data_from_id(EXT_TO_INT_OPID(get_mips_req_info->op_list[i]));
        if (op != NULL)
        {
            if (PROC_ON_SAME_CORE(op->processor_id))
            {
                if (op->profiler != NULL)
                {
                    get_mips_req_info->mips_usage[i] =
                                           (uint16) op->profiler->cpu_fraction;
                }
                else
                {
                    get_mips_req_info->mips_usage[i] = INVALID_MIPS_USAGE;
                }
            }
            else
            {
                op_not_on_local_core = TRUE;
            }
        }
        else
        {
            /* P1 does not know about operators on P0. P1 operators should not
             * be NULL if this is called on P1, since P0 clearly found (some of) 
             * them. */
            if (PROC_PRIMARY_CONTEXT())
            {
                get_mips_req_info->mips_usage[i] = INVALID_MIPS_USAGE;
            }
        }
    }

    return op_not_on_local_core;
}

/**
 * opmgr_send_get_mips_usage_resp
 */
void opmgr_send_get_mips_usage_resp(void)
{
    /* Send the response to the adaptor. */
    get_mips_req_info->mips_usage_callback(get_mips_req_info->con_id,
                                           get_mips_req_info->list_length,
                                           get_mips_req_info->op_list,
                                           get_mips_req_info->mips_usage);
    
    /* Free up any memory. */
    opmgr_free_get_mips_usage_helper_vars();
}
#endif /* PROFILER_ON */

/****************************************************************************
Private Function Definitions
*/

/**
 * \brief Tell streams to kill the operator endpoints before the operator is destroyed.
 *        It is possible that return value and/or more arguments would be needed in future.
 *
 * \param  op_data Pointer to the operator data for the operator.
 *
 *  \return True if the operation was successful, False otherwise.
 */
bool opmgr_destroy_op_endpoints(OPERATOR_DATA* op_data)
{
    /* An operator can only be destroyed if it is stopped. So check this and
     * fail the command before any state gets torn down.
     */
    /* Operator is created remotely - NOTE: we should not end up here on a
     * secondary OpMgr with processor_id set to 0 (primary)! */
    if (PROC_ON_SAME_CORE(op_data->processor_id))
    {
        if (OP_RUNNING == op_data->state)
        {
            return FALSE;
        }
    }
    else
    {
        /* Operator not on this core (which is executing this).
           That is, on P0 and operator is on Px. Nothing to do
           in this case. Or on Px and operator is on P0 - but
           this should never happen, guaranteed by the aggregate
           function on P0, which forwards Px-only reqs to Px.
           Nothing to do, so avoid going through stream_destroy_
           all_operators_endpoints, and return TRUE here.
        */
        return TRUE;
    }

    /* It is important to destroy the sinks first as we probably need to traverse
     * the graph to work out what the current topology is. If the sources went
     * first then we might end up with a chain flopping around we don't know about.
     */
    return stream_destroy_all_operators_endpoints(INT_TO_EXT_OPID(op_data->id),
                                                    op_data->cap_data->max_sinks,
                                                    op_data->cap_data->max_sources);
}


static OPERATOR_DATA* find_op_data_in_list(INT_OP_ID id, OPERATOR_DATA *list_head)
{
    OPERATOR_DATA* entry = list_head;

    while ((entry != NULL) && (entry->id != id))
    {
        entry = entry->next;
    }

    return entry;
}

/****************************************************************************
 *
 * get_op_data_from_id
 *
 * SINGLECORE: it searches local list and return the entry.
 * MULTICORE: it searches in local and remote op lists and return the entry.
 *            (It only exist on P0 when separate image is built).
 */
OPERATOR_DATA* get_op_data_from_id(INT_OP_ID id)
{
    OPERATOR_DATA* entry = find_op_data_in_list(id, oplist_head);
    return entry;
}

/****************************************************************************
 *
 * get_anycore_op_data_from_id
 *
 * SINGLECORE: it searches local list and return the entry.
 * MULTICORE: it searches in local and remote op lists and return the entry.
 *            (It only exist on P0 when separate image is built).
 */
OPERATOR_DATA* get_anycore_op_data_from_id(INT_OP_ID id)
{
    OPERATOR_DATA* entry = find_op_data_in_list(id, oplist_head);

    /* If we are on P0, and haven't found in local list then look among remote ops */
    if (PROC_PRIMARY_CONTEXT() && (entry == NULL))
    {
        entry = find_op_data_in_list(id, remote_oplist_head);
    }
    return entry;
}

/**
 * \brief Remove the operator data from the operator list
 *
 * \param id      Identifier of operator to be removed.
 * \param op_list Pointer to array of pointers to operator structure.
 */
void remove_op_data_from_list(INT_OP_ID id, OPERATOR_DATA** op_list)
{
    OPERATOR_DATA **p, *cur_op;
    patch_fn_shared(opmgr);

    /* Delete the entry from remote operator list */
    cur_op = find_op_data_in_list(id, *op_list);

    if(cur_op != NULL)
    {
        p = op_list;
        while(*p && *p != cur_op) p = &((*p)->next);
        if(*p)
        {
            *p = cur_op->next;
            pfree(cur_op);
        }
    }
    else
    {
        /* This indicates something has corrupted the list.*/
        panic_diatribe(PANIC_AUDIO_OPERATOR_NOT_FOUND, id);
    }

}

bool is_op_running(INT_OP_ID op_id)
{
    OPERATOR_DATA* operator = get_anycore_op_data_from_id(op_id);
    if (operator)
    {
        if ((operator->state) == OP_RUNNING)
        {
            return TRUE;
        }
    }
    /* The operator doesn't exist or it's not running*/
    return FALSE;
}

/****************************************************************************
 *
 * get_is_source_from_opidep
 *
 */
bool get_is_source_from_opidep(unsigned int opidep)
{
    return ((opidep & STREAM_EP_TYPE_MASK) == STREAM_EP_OP_SOURCE);
}

/****************************************************************************
 *
 * get_is_sink_from_opidep
 *
 */
bool get_is_sink_from_opidep(unsigned int opidep)
{
    return ((opidep & STREAM_EP_TYPE_MASK) == STREAM_EP_OP_SINK);
}

INT_OP_ID get_opid_from_opidep(unsigned int opidep)
{
    return ((opidep & STREAM_EP_OPID_MASK) >> STREAM_EP_OPID_POSN);
}

/****************************************************************************
 *
 * get_ext_opid_from_opidep
 *
 */
unsigned int get_ext_opid_from_opidep(unsigned int opidep)
{
    return INT_TO_EXT_OPID(get_opid_from_opidep(opidep));
}

/****************************************************************************
 *
 * get_terminal_from_opidep
 *
 */
unsigned int get_terminal_from_opidep(unsigned int opidep)
{
    return ((opidep & STREAM_EP_CHAN_MASK) >> STREAM_EP_CHAN_POSN);
}

/****************************************************************************
 *
 * get_opidep_from_opid_and_terminal
 *
 */
unsigned int get_opidep_from_opid_and_terminalid(unsigned int op_id, unsigned int terminal_id)
{
    unsigned int id = ((terminal_id & STREAM_EP_CHAN_MASK) | ((op_id<<STREAM_EP_OPID_POSN) & STREAM_EP_OPID_MASK));

    if(terminal_id&TERMINAL_SINK_MASK)
    {
        id |= STREAM_EP_OP_SINK;
    }
    else
    {
        id |= STREAM_EP_OP_SOURCE;
    }
    return id;
}

bool opmgr_is_opidep_valid(unsigned int opidep)
{
    INT_OP_ID opid;
    unsigned terminal_num;
    OPERATOR_DATA *op_data;

    terminal_num = get_terminal_from_opidep(opidep);
    opid = get_opid_from_opidep(opidep);

    /* If the operator exists and the terminal id is within range of the
       maximum number of sinks/sources then it is valid. */
    op_data = get_anycore_op_data_from_id(opid);
    if (op_data != NULL)
    {
        if ((get_is_sink_from_opidep(opidep)) != 0)
        {
            if (terminal_num < op_data->cap_data->max_sinks)
            {
                return TRUE;
            }
        }
        else
        {
            if (terminal_num < op_data->cap_data->max_sources)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

CONNECTION_LINK opmgr_con_id_from_opid(EXT_OP_ID opid)
{
    INT_OP_ID int_op_id;
    OPERATOR_DATA *op_data;

    int_op_id = EXT_TO_INT_OPID(opid);
    op_data = get_anycore_op_data_from_id(int_op_id);
    if (op_data != NULL)
    {
        return op_data->con_id;
    }
    return CONN_LINK_INVALID;
}

/**
 * \brief   Sets the creator client id for all operators in an array.
 *          Panics if an operator in the list is not found.
 * \param   client_id Client id of the owner and creator
 * \param   num_ops Number of required operators
 * \param   op_list Array of ids of the required operators.
 */
void opmgr_set_creator_id(CONNECTION_PEER client_id,
                          unsigned num_ops,
                          const EXT_OP_ID *op_list)
{
    CONNECTION_LINK con_id;
    unsigned i;
    OPERATOR_DATA * op_data;
    for (i = 0; i < num_ops; i++)
    {
        INT_OP_ID op_id = EXT_TO_INT_OPID(op_list[i]);
        op_data = get_anycore_op_data_from_id(op_id);
        if (op_data == NULL)
        {
            panic_diatribe(PANIC_AUDIO_OPERATOR_NOT_FOUND, op_list[i]);
        }
        LOCK_INTERRUPTS;
        op_data->creator_client_id = client_id;
        con_id = PACK_CON_ID((CONNECTION_PEER) client_id,
                             GET_CON_ID_RECV_ID(op_data->con_id));
        op_data->con_id = con_id;
        UNLOCK_INTERRUPTS;
    }
}

/*
 *  \brief construct a simple unsolicited message without a payload
 *  \param op_id operator id
 *  \param client_id the external owner's client id, 0xFF if invalid.
 *  \param msg_id message id
 *  \param msg_len message payload length in words
 *  \param msg_id message payload
 *  \param pointer to the length of the message constructed
 */
const OP_UNSOLICITED_MSG *opmgr_make_unsolicited_message(EXT_OP_ID op_id,
                                                         CONNECTION_PEER client_id,
                                                         unsigned msg_id,
                                                         unsigned msg_len,
                                                         unsigned *msg_body,
                                                         unsigned *length)
{
    OP_UNSOLICITED_MSG * msg;
    *length = sizeof(OP_UNSOLICITED_MSG)/sizeof(unsigned) +msg_len;
    msg = (OP_UNSOLICITED_MSG *) pnewn(*length, unsigned);
    msg->op_id = op_id;
    msg->client_id = (unsigned) client_id;
    msg->msg_id= msg_id;
    msg->length = msg_len;
    memcpy(msg->payload, msg_body, msg_len * sizeof(unsigned));
    return msg;
}

ADAPTOR_TARGET opmgr_get_adaptor(CONNECTION_LINK con_id)
{
    return adaptor_get_target(con_id);
}

bool opmgr_get_generic_value_from_operator(unsigned int endpoint_id,
                                           OPMSG_CONFIGURATION_KEYS key,
                                           uint32 *result)
{
    OPMSG_GET_CONFIG_RESULT get_config_result;
    bool success;

    get_config_result.value = 0;
    success = opmgr_get_config_msg_to_operator(endpoint_id,
                                               key,
                                               &get_config_result);
    *result = get_config_result.value;
    return success;
}

void opmgr_get_terminal_details_from_operator(unsigned int endpoint_id,
                                              uint32 *result)
{
    OPMSG_GET_CONFIG_RESULT get_config_result;

    get_config_result.value = OPMSG_GET_CONFIG_TERMINAL_DETAILS_NONE;
    opmgr_get_config_msg_to_operator(endpoint_id,
                                     OPMSG_OP_TERMINAL_DETAILS,
                                     &get_config_result);
    *result = get_config_result.value;
}

bool opmgr_get_ratematch_measure_from_operator(unsigned int endpoint_id,
                                               int *sp_deviation,
                                               RATE_RELATIVE_RATE *measurement)
{
    OPMSG_GET_CONFIG_RESULT msg_result;
    bool success;
    msg_result.value = 0;

    success = opmgr_get_config_msg_to_operator(endpoint_id,
                                               OPMSG_OP_TERMINAL_RATEMATCH_MEASUREMENT,
                                               &msg_result);

    *sp_deviation = msg_result.rm_measurement.sp_deviation;
    *measurement = msg_result.rm_measurement.measurement;
    return success;
}

bool opmgr_check_discardable_unsolicited_msg(OP_UNSOLICITED_MSG *opmsg)
{
    if ((opmsg->msg_id & OPMSG_REPLY_ID_DISCARDABLE_MESSAGE) != 0)
    {
        opmsg->msg_id &= ~OPMSG_REPLY_ID_DISCARDABLE_MESSAGE;
        return TRUE;
    }

    return FALSE;
}
