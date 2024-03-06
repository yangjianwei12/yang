/*****************************************************************************
*
* Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
*
*****************************************************************************/
/** \file
 *
 *  This is the main public project header for the \c ipc library.
 *
 */
/****************************************************************************
Include Files
*/

#ifndef IPC_H
#define IPC_H

#include "types.h"
#include "buffer/buffer_ipc.h"
#ifdef INSTALL_IPC_INTERFACE_TEST
#include "test/if_test/ipc_if_test.h"
#endif

#include "ipc_kip.h"

/****************************************************************************
Public Type Declarations
*/

typedef void (*IPC_MESSAGE_NOTIFICATION_HANDLER)(uint16 channel_id, ipc_msg *msg);

/*
 * Optional parameters for IPC_INTERFACE_CALLBACK
 */
typedef struct
{
    IPC_STATUS             ipc_status;

} ipc_event_params_default;

typedef struct
{
    IPC_STATUS             ipc_status;
    uint16                 signal_channel_id;
    uint16                 signal_id;

} ipc_event_params_signal;

typedef struct
{
    IPC_STATUS             ipc_status;
    uint16                 signal_channel_id;
    uint16                 signal_id;
    PROC_ID_NUM            remote_proc_id;

} ipc_event_params_signal_ind;

typedef struct
{
    IPC_STATUS             ipc_status;
    IPC_STATUS             remote_status;
    uint16                 data_channel_id;

} ipc_event_params_deactivated;

typedef struct
{
    IPC_STATUS             ipc_status;
    uint16                 data_channel_id;
    PROC_ID_NUM            remote_proc_id;

} ipc_event_params_activated;

typedef struct
{
    IPC_STATUS             ipc_status;
    uint16                 msg_channel_id;

} ipc_event_params_msgchan;

typedef struct
{
    IPC_STATUS             ipc_status;
} ipc_event_params_watchdog;

typedef struct
{
    uint16                 error_type;
    uint16                 error_level;
    uint16                 error_code;
    PROC_ID_NUM            remote_proc_id;

} ipc_event_params_error_ind;


/*
 * In case of a need to add additional optional event parameters and types,
 * note that every event enum IPC_EVENT has its own optional parameters type.
 */
typedef ipc_event_params_default            ipc_event_setup_ready_params_t;                            /* IPC_SETUP_READY */
typedef ipc_event_params_default            ipc_event_setup_complete_params_t;                         /* IPC_SETUP_COMPLETE */
typedef ipc_event_params_signal             ipc_event_sig_configured_params_t;                         /* IPC_SIG_CONFIGURED */
typedef ipc_event_params_signal_ind         ipc_event_sig_configure_ind_params_t;                      /* IPC_SIG_CONFIGURE_IND */
typedef ipc_event_params_activated          ipc_event_data_channel_activated_params_t;                 /* IPC_DATA_CHANNEL_ACTIVATED */
typedef ipc_event_params_deactivated        ipc_event_data_channel_deactivated_params_t;               /* IPC_DATA_CHANNEL_DEACTIVATED */
typedef ipc_event_params_default            ipc_event_msghandler_failed_params_t;                      /* IPC_MSGHANDLER_FAILED */
typedef ipc_event_params_msgchan            ipc_event_teardown_completed_params_t;                     /* IPC_TEARDOWN_COMPLETED */
typedef ipc_event_params_msgchan            ipc_event_teardown_ind_params_t;                           /* IPC_TEARDOWN_IND */
typedef ipc_event_params_watchdog           ipc_event_watchdog_ind_params_t;                           /* IPC_WATCHDOG_IND*/
typedef ipc_event_params_watchdog           ipc_event_watchdog_completed_params_t;                     /* IPC_WATCHDOG_COMPLETE*/
typedef ipc_event_params_msgchan            ipc_event_reset_request_params_t;                          /* IPC_RESET_REQUEST/COMPLETED */
typedef ipc_event_params_error_ind          ipc_event_params_error_ind_t;                              /* IPC_ERROR_IND */

/*
 * Dedicated signal handler installed by function 'ipc_setup_static_signal'.
 */
typedef IPC_STATUS (*IPC_DEDICATED_SIGNAL_HANDLER)(PROC_ID_NUM remote_proc_id, uint16 signal_id, uintptr_t data_channel_id);

/****************************************************************************
Public Constant and macros
*/

/****************************************************************************
Public Variable Declarations
*/

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief Setup IPC communications
 *
 * \param[in]  msg_callback   - Caller provided callback function to process message events
 * \param[in]  num_signals    - Number of signal channels/events to support
 * \param[out] channel_id     - The channel id of the messaging channel that was setup
 *
 * \return  0 if successfully setup IPC communications else error code
 */
extern IPC_STATUS ipc_setup_comms(IPC_MESSAGE_NOTIFICATION_HANDLER msg_callback, uint16 num_signals, uint16 *channel_id);

/**
 * \brief Teardown IPC communications
 *
 * Undoes all the allocations and configurations done by 'ipc_setup_comms'.
 * It may only be called after all other processors have been through teardown themselves and are
 * all powered off. Can only be called on P0.
 *
 * \return  0 if successful teardown of IPC communications else error code
 */
extern IPC_STATUS ipc_unregister_comms(void);

/**
 * \brief Start a processor through keyhole
 *
 * \param[in] remote_proc_id - Remote processor ID. Allowed numbers are 1 - 3. Primary processor is 0.
 *
 * \return  0 if successfully started the processor else error code
 */
extern IPC_STATUS ipc_start_processor(PROC_ID_NUM remote_proc_id);

/*
 * \brief Forcefully stop a processor through keyhole
 *
 * \param[in] remote_proc_id - Remote processor ID. Allowed numbers are 1 - 3. Primary processor is 0.
 */
extern IPC_STATUS ipc_stop_processor(PROC_ID_NUM remote_proc_id);

/*
 * \brief Request a processor to shutdown. This will tear off ipc link as well.
 *
 * \param[in] remote_proc_id - Remote processir ID. Allowed numbers are 1 - 3. Primary processor is 0.
 */
extern IPC_STATUS ipc_poweroff_processor(PROC_ID_NUM remote_proc_id);

/**
 * \brief Enable watchdog pings
 *        Note that this function assumes the specified remote processor has
 *        already started. The caller must ensure that this is the case.
 *
 * \param[in] remote_proc_id   - Remote processor ID. Allowed numbers are 1 - 3.
 * \param[in] watchdog_timeout - The watchdog timeout in ms
 * \param[in] callback         - Call upon timeout
 *
 * \return  0 if successfully enabled watchdog pings else error code
 */
extern IPC_STATUS ipc_watchdog_ping(PROC_ID_NUM remote_proc_id);

/**
 * \brief Sets up a signalling channel dedicated for the signal ID.
 *
 * \param[in]  signal id      - Caller supplied signal id (see CS-336170-DD section 10.1.4.2)
 * \param[in]  callback       - Signal handler (callback)
 * \param[out] signal_channel - Allocated signal channel (if successful)
 *  Signal channel numbers are 0 - 'num_signals-1' (NB. we only support two dedicated signals)
 *
 * \return IPC_SUCCESS if successfully sent external message else error code
 */
extern IPC_STATUS ipc_setup_static_signal(uint16 signal_id, IPC_DEDICATED_SIGNAL_HANDLER callback, uint16 *signal_channel);

/**
 * \brief Raise a signal to a partner processor.
 *
 * \param[in] remote_proc_id - Remote processor ID (0 - 3).
 * \param[in] signal id      - Caller supplied signal id (see CS-336170-DD section 10.1.4.2)
 * \param[in] data_channel_id - ID of the data channel
 *
 * \return IPC_SUCCESS if successfully raised signal else error code
 */
extern IPC_STATUS ipc_raise_signal(PROC_ID_NUM remote_proc_id, uint16 signal_id, uintptr_t data_channel_id);

/**
 * \brief Creates a data channel. Create the port of it, the request is for the first channel.
 *        Maximum of 16 channels (8 in either direction).
 *
 * \param[in]  port_id            - Port ID.
 * \param[in]  channel_number     - Data channel ID (0..127).
 * \param[in]  IPC_DATA_DIRECTION - Direction: channel read or write.
 * \param[in]  cbuffer            - If provided use that cbuffer. If NULL, function must allocate cbuffer.
 * \param[out] data_channel_id    - Channel id of the data channel created
 *
 * \return  0 if successfully created data channel else error code
 */
extern IPC_STATUS ipc_create_data_channel(uint16 port_id, uint16 channel_number, IPC_DATA_DIRECTION direction, uint16 *data_channel_id);
/**
 * \brief Destroy a channel. For the last channel, destroy the port too.
 *
 * \param[in] data_channel_id - Data channel ID (0..127).
 *
 * \return  0 if successfully destroyed data channel else error code
 */
extern IPC_STATUS ipc_destroy_data_channel(uint16 data_channel_id);

/**
 * \brief  Activate a new data channel for use with partner processor 'remote_proc_id'.
 *         Callers of this function can specify an opaque data block to be transferred
 *         to the secondary processor
 *
 * \param  data_channel_id [IN] - data channel ID (from ipc_create_data_channel)
 * \param  remote_proc_id  [IN] - secondary processor to communicate activation to
 * \param  cbuffer         [IN] - pointer to caller supplied circular buffer
 * \param  create_cbuffer   [IN] - if true, IPC allocates a new cbuffer in the shared
 *                                memory and syncs it with the passed cbuffer.
 *                                Otherwise the passes buffer is used directly in
 *                                the channel.
 *
 * \return Return IPC_SUCCESS if channel activated successfully, error if not.
 *         The interface callback will be called upon receiving reply from
 *         the target secondary processor handling the activation event.
 */
extern IPC_STATUS ipc_activate_data_channel(uint16 data_channel_id ,
                                            PROC_ID_NUM remote_proc_id,
                                            tCbuffer *cbuffer,
                                            bool create_cbuffer);

/**
 * \brief Deactivate the data channel. The data port as well while destroying the last channel
 * of a port.
 *
 * \param[in] data_channel_id - Data channel ID (0..127).
 *
 * \return  0 if successfully deactivated data channel else error code
 */
extern IPC_STATUS ipc_deactivate_data_channel(uint16 data_channel_id);

/**
 * \brief Return the cbuffer associated with a data channel.
 *
 * \param[in] data_channel_id - Data channel ID (0..127).
 *
 * \return  NULL if data_channel_id not found or if stored cbuffer of data channel is NULL.
 *          Non-NULL if data_channel_id valid and stored cbuffer of data channel is non-NULL.
 */
extern tCbuffer *ipc_data_channel_get_cbuffer(uint16 data_channel_id);

#if !defined(COMMON_SHARED_HEAP)
/**

 * \brief  Synchronise a data channel with a cbuffer.
 *         If the data channel and cbuffer share the same data buffer, then
 *         update the read and write pointers, so that the structs are now
 *         synchronised. If the data buffers are different, copy all data
 *         from the cbuffer to the data channel.
 *
 *         Purpose is to sync two cbuffers using the same data buffer.
 *         Some operators will have buffers created for all channels even though
 *         it has connected only one channel, such as mixer,
 *         splitter, resampler. Real endpoints (e.g. A2DP source endpoint) will also have
 *         its buffers. This buffer must be used through the dual-core data channel, rather
 *         than creating a new cbuffer. But for cbuffers accessed by two cpus the cbuffer
 *         structure must be in shared memory, accessible by both cpus.
 *         In these cases, we keep a duplicate cbuffer structure: the original one in
 *         private memory, and a second one in shared memory (created in ipc_activate_
 *         data_channel), using the same data buffer. The two cbuffer structures must be
 *         synced from time to time; ipc_data_channel_write_sync syncs the shared cbuffer
 *         struct of the data channel to a private cbuffer; ipc_data_channel_read_sync
 *         syncs a private cbuffer to the shared cbuffer struct of the data channel.
 *
 * \param  src_cbuffer      [IN] - cbuffer to read from
 * \param  dst_data_channel [IN] - data channel ID (from ipc_create_data_channel)
 *                                 whose cbuffer to write to
 * \param  lim_wr_idx       [IN] - limiting write index, used to force the write_ptr
 *                                 according to the last tag index when not all
 *                                 metadata tags get transported (space limitation);
 *                                 negative means "no limitation"
 */
extern void ipc_data_channel_write_sync(tCbuffer *src_cbuffer, uint16 dst_data_channel, int lim_wr_idx);

/**
 * \brief  Synchronise a cbuffer with a data channel.
 *         If the cbuffer and data channel share the same data buffer, then
 *         update the read and write pointers, so that the structs are now
 *         synchronised. If the data buffers are different, copy all data
 *         from the data channel to the cbuffer.
 *
 *         See also comments under ipc_data_channel_write_sync above.
 *
 * \param  src_data_channel [IN] - data channel ID (from ipc_create_data_channel)
 *                                 whose cbuffer to read from
 * \param  dst_cbuffer      [IN] - cbuffer to write to
 */
extern void ipc_data_channel_read_sync(uint16 src_data_channel, tCbuffer *dst_cbuffer);
#endif /* !COMMON_SHARED_HEAP */

/**
 * \brief  Returns whether the secondary processor has started
 *         That is, it verifies whether the processor is in a state
 *         ready to communicate with other processors. If not, it is
 *         either off, or still establishing communications, or in
 *         teardown.
 *
 * \param[in] proc_id           - Remote processor ID (1 - 3).
 *
 * \return  TRUE if secondary processor has started, FALSE if not.
 */
extern bool ipc_aux_proc_has_started(PROC_ID_NUM proc_id);

/**
 * \brief  Returns whether the debugger support should be used.
 *         The ADK debugger sets a bit in a register to indicate
 *         whether the firmware should enable debugger support.
 *
 * \return  TRUE if debugger mode is required, FALSE if not.
 */
extern bool ipc_support_debugger(PROC_ID_NUM remote_proc_id);

/**
 * \brief   Reports the panic details to P0.
 *          Px is about to panic: it tries to send any details to P0
 */
extern void ipc_panic_report(uint16 data_length, void * panic_data);

/**
 * \brief  Halt secondary processor.
 *
 * \param[in] remote_proc_id           - Remote processor ID (1 - 3).
 *
 * \return  IPC_SUCCESS if secondary processor was halted, error if not.
 */
extern IPC_STATUS ipc_halt_processor(PROC_ID_NUM remote_proc_id);

#endif /* IPC_H */
