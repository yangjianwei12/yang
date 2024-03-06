/****************************************************************************
 * Copyright (c) 2011 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file opmgr_private.h
 * \ingroup opmgr
 *
 */

#ifndef OPMGR_PRIVATE_H
#define OPMGR_PRIVATE_H

/****************************************************************************
Include Files
*/

#include "pmalloc/pl_malloc.h"
#include <string.h>
#include "platform/pl_trace.h"
#include "platform/pl_assert.h"
#include "platform/pl_intrinsics.h"
#include "platform/pl_error.h"
#include "sched_oxygen/opmgr_sched.h"
#include "pl_timers/pl_timers.h"
#include "panic/panic.h"
#include "patch/patch.h"
#include "fault/fault.h"
#include "stream/stream.h"
#include "opmgr/opmgr.h"
#include "opmgr/opmgr_operator_data.h"
#include "opmgr/opmgr_for_adaptors.h"
#include "opmgr/opmgr_for_ops.h"
#include "opmgr/opmgr_for_stream.h"
#include "stream/stream_for_opmgr.h"
#include "adaptor/adaptor.h"
#include "audio_log/audio_log.h"
#ifdef INSTALL_CAP_DOWNLOAD_MGR
#include "opmgr/opmgr_for_cap_download.h"
#include "cap_download_mgr/cap_download_mgr.h"
#endif
#ifdef INSTALL_PIO_SUPPORT
#include "pio.h"
#endif
#include "sys_events/sys_events.h"
#include "hal/hal_dm_sections.h"

#include "operator_prim.h"
#include "opmsg_prim.h"
#include "cap_id_prim.h"

#if defined(SUPPORTS_MULTI_CORE)
#include "kip_mgr/kip_mgr.h"
#include "opmgr/opmgr_kip.h"
#endif

#ifdef INSTALL_THREAD_OFFLOAD
#include "thread_offload/thread_offload.h"
#endif

#include "timing_trace_irt/timing_trace.h"
#include "timing_trace_irt/timing_trace_instrumentation.h"

/****************************************************************************
Private Type Declarations
*/

/**
 * Store a single operator ID, or a pointer to a list of them
 */
typedef union
{
    unsigned *op_list;
    unsigned id;
} OPERATOR_ID_INFO;

/**
 * data object for keeping track of requests that implicate multiple operators
 */
typedef struct
{
    void *callback;
    unsigned num_ops;
    unsigned cur_index;
    OPERATOR_ID_INFO id_info;
} MULTI_OP_REQ_DATA;

/** Function pointer prototype of pre-processing functions. */
typedef bool (*preproc_function)(struct OPERATOR_DATA *op_data);



/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Macro Declarations
*/

/** Convert an internal opid into an external sink endpoint */
#define INT_TO_EXT_SINK(id) (STREAM_EP_OP_SINK | (id << STREAM_EP_OPID_POSN))

/* Number of parallel start/stop/reset/destroy accmds that we support */
/* Maximum is 32, so that it fits in 5 bits in the connection id      */
#define NUM_AGGREGATES      1

/****************************************************************************
Private Variable Declarations
*/

/** Pointer to head of operators list */
extern OPERATOR_DATA* oplist_head;

/**
 * Pointer to head of 'remote' operators list.
 * This is only for P0 in dual core case.
 */
#if defined(SUPPORTS_MULTI_CORE)
extern OPERATOR_DATA* remote_oplist_head;
#else
#define remote_oplist_head NULL
#endif

/** Table of capability data pointers */
extern const CAPABILITY_DATA* const capability_data_table[];

#ifdef DESKTOP_TEST_BUILD
extern bool test_failed;
#endif /* DESKTOP_TEST_BUILD */

/* Download capability database */
#ifdef INSTALL_CAP_DOWNLOAD_MGR
extern DOWNLOAD_CAP_DATA_DB** cap_download_data_list;
#endif

/****************************************************************************
Private Function Declarations
*/

#if defined(SUPPORTS_MULTI_CORE)
extern OPERATOR_DATA* get_remote_op_data_from_id(INT_OP_ID id);
#else
#define get_remote_op_data_from_id(x) NULL
#endif

/**
 * \brief Tell streams to kill the operator endpoints before
 *        the operator is destroyed.
 *
 * \param op_data Pointer to the operator data for the operator.
 *
 * \note It is possible that return value and/or more arguments would
 *       be needed in future.
 */
extern bool opmgr_destroy_op_endpoints(OPERATOR_DATA* op_data);

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief Returns pointer to capability data for a certain cap ID.
 *
 * \param cap_id The id of the capability
 * \param processor_id The core where the capability is on
 *
 * \return A pointer to a constant cap data structure,
 *         if not found NULL is returned.
 */
extern const CAPABILITY_DATA* opmgr_lookup_cap_data_for_cap_id(CAP_ID cap_id,
                                                               PROC_ID_NUM processor_id);
#else
/**
 * \brief Returns pointer to capability data for a certain cap ID.
 *
 * \param cap_id The id of the capability
 *
 * \return A pointer to a constant cap data structure,
 *         if not found NULL is returned.
 */
extern const CAPABILITY_DATA* opmgr_lookup_cap_data_for_cap_id(CAP_ID cap_id);
#endif /* SUPPORTS_MULTI_CORE */

/**
 * \brief Adds a new capability to the downloadable capabilities list.
 *
 * \param  cap_data The capability to store
 * \param  is_on_both_cores Boolean to let Opmgr know if the capability is
 *         available on both cores
 *
 * \return  True if successfully added, or False otherwise
 */
#ifdef INSTALL_CAP_DOWNLOAD_MGR
extern bool opmgr_add_cap_download_data(CAPABILITY_DATA* cap_data,
                                        bool is_on_both_cores);
#endif

/**
 * \brief Remove an existing capability from the downloadable capabilities list.
 *
 * \param  cap_id The capability id to remove
 *
 * \return  True if successfully added, or False otherwise
 */
#ifdef INSTALL_CAP_DOWNLOAD_MGR
extern bool opmgr_remove_cap_download_data(CAP_ID cap_id);
#endif

/**
 * \brief Stores information about an operator request whilst
 *        it is being serviced.
 *
 * \param client_id The ID of the client making the request.
 * \param op_id     The ID of the operator an action is requested from.
 * \param data      Void pointer to whatever data structure the user needs
 *                  to store to handle the response message from the operator
 *                  when it is received.
 *
 * \return TRUE if the task was stored, FALSE if it couldn't be.
 *
 * \note This function only fails if malloc fails.
 */
extern bool opmgr_store_in_progress_task(unsigned client_id,
                                         INT_OP_ID op_id,
                                         void *data);

/**
 * \brief Retrieves the callback associated with a request that was sent to an
 *        operator.
 *
 * \param client_id The id of the client that made the request.
 * \param op_id     The ID of the operator the action was requested from.
 *
 * \return A void pointer to the callback function that was stored,
 *         NULL if not found.
 */
extern void *opmgr_retrieve_in_progress_task(unsigned client_id,
                                             INT_OP_ID op_id);

/**
 * \brief Gets the operator structure for the operator with id
 *        on any core in a multi-core configuration.
 * 
 * \param  id the internal id of the operator.
 * 
 * \return pointer to the operator structure, NULL if not found.
 */
extern OPERATOR_DATA* get_anycore_op_data_from_id(INT_OP_ID id);

/**
 * \brief Remove the operator data from the operator list
 *
 * \param id      Identifier of operator to be removed.
 * \param op_list Pointer to array of pointers to operator structure.
 */
extern void remove_op_data_from_list(INT_OP_ID id, OPERATOR_DATA **op_list);

/**
 * \brief Extracts the terminal id of the operator that the endpoint refers to.
 *        Any information about direction is lost.
 *
 * \param opidep the endpoint id of an operator endpoint.
 *
 * \return the terminal number on the operator the endpoint refers to.
 */
extern unsigned int get_terminal_from_opidep(unsigned int opidep);

/**
 * \brief Called when a message is sent to an operator.
 *
 * This looks up the correct function to handle the message that was received
 * from the capabilities handler table, calls it and sends a response message
 * to OpMgr.
 *
 * \param msg_data Pointer to a pointer to the OPERATOR_DATA of the operator.
 *                 The type is void ** as this is a generic scheduler message
 *                 handler.
 */
extern void opmgr_operator_task_handler(void **msg_data);

/**
 * \brief Called when a background interrupt is sent to an operator.
 *
 * This is a kick and sets off the data processing function of the operator.
 *
 * \param bg_data Pointer to a pointer to the OPERATOR_DATA of the operator.
 *                The type is void ** as this is a generic background
 *                interrupt handler.
 */
extern void opmgr_operator_bgint_handler(void **bg_data);

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief Count the number of operators with matching capability,
 *        on remote cores.
 *
 * \param capid The capability ID to be searched for.
 *              If CAP_ID_NONE, all operators get counted.
 *
 * \return Number of instances of operator with matching capability or
 *         count of all operators, on all cores.
 *
 * \note This function should be called only from the primary processor.
 */
extern unsigned opmgr_get_remote_ops_count(CAP_ID capid);

/**
 * \brief Build and send a request to create an operator on a secondary core.
 *
 * \param con_id       The identifier of the connection used.
 * \param cap_id       The type of capability of the operator.
 * \param op_id        The external identifier of the operator.
 * \param priority     The priority of the task used by the operator.
 *
 * \return TRUE if the message was sent successfully.
 */
extern bool opmgr_kip_build_send_create_op_req(CONNECTION_LINK con_id,
                                               CAP_ID cap_id,
                                               EXT_OP_ID op_id,
                                               PRIORITY priority);

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
extern bool opmgr_kip_build_send_opmsg(CONNECTION_LINK con_id,
                                       EXT_OP_ID op_id,
                                       unsigned num_params,
                                       unsigned *params);

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
extern bool opmgr_kip_list_resp_handler(CONNECTION_LINK con_id,
                                        STATUS_KYMERA status,
                                        unsigned count,
                                        unsigned err_code);

/**
 * \brief Send an unsolicited message over KIP.
 *
 * \param con_id      Connection id
 * \param msg_from_op Unsolicited messsage from the operator 
 *
 * \return True if successful.
 */
extern bool opmgr_kip_unsolicited_message(CONNECTION_LINK con_id,
                                          OP_UNSOLICITED_MSG *msg_from_op);

#else
#define opmgr_get_remote_ops_count(x) 0
#define opmgr_kip_build_send_create_op_req(p1, p2, p3, p4) FALSE
#define opmgr_kip_build_send_opmsg(p1, p2, p3, p4) FALSE
#define opmgr_kip_unsolicited_message(p1, p2) FALSE
#endif

#endif /* OPMGR_PRIVATE_H */
