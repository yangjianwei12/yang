/****************************************************************************
 * Copyright (c) 2011 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_kip.c
 * \ingroup stream
 *
 * Private Stream KIP elements. <br>
 *
 */

#ifndef STREAM_KIP_H
#define STREAM_KIP_H

/****************************************************************************
Include Files
*/

#include "status_prim.h"
#include "stream/stream.h"
#include "stream/stream_transform.h"
#include "stream/stream_for_adaptors.h"
#include "proc/proc.h"

/****************************************************************************
Type Declarations
*/

/* Do not change these value since it is used for bit operation
 * Location of the endpoint based on the processor context. From P0
 * STREAM_EP_REMOTE_ALL means source and sink endpoints are not located
 * with P0 (on dual core, those are with P1)
 */
typedef enum
{
    STREAM_EP_REMOTE_NONE   = 0x0,
    STREAM_EP_REMOTE_SOURCE = 0x1,
    STREAM_EP_REMOTE_SINK   = 0x2,
    STREAM_EP_REMOTE_ALL    = 0x3
} STREAM_EP_LOCATION;

/* Structure for keeping record of connect stages when shadow EPs are involved */
typedef struct STREAM_KIP_CONNECT_INFO STREAM_KIP_CONNECT_INFO;

typedef union
{
    STREAM_COUNT_CBACK tr_disc_cb;
    STREAM_TRANSFORM_PAIR_CBACK disc_cb;
} STREAM_KIP_TRANSFORM_DISCONNECT_CB;

typedef struct STREAM_KIP_TRANSFORM_DISCONNECT_INFO STREAM_KIP_TRANSFORM_DISCONNECT_INFO;

/**
 * \brief kick the kip endpoints on receiving kip signals
 *
 * \param data_chan_id Data channel id
 * \param kick_dir     The kick direction
 */
extern void stream_kip_kick_eps(unsigned data_chan_id,
                                ENDPOINT_KICK_DIRECTION kick_dir);

typedef struct STREAM_KIP_TRANSFORM_INFO STREAM_KIP_TRANSFORM_INFO;


/****************************************************************************
Constant Declarations
*/

/****************************************************************************
Macro Declarations
*/

#ifdef KIP_DEBUG
#define STREAM_KIP_ASSERT(x) PL_ASSERT(x)
#else
#define STREAM_KIP_ASSERT(x) (void)(x)
#endif

#ifndef INSTALL_DELEGATE_AUDIO_HW

#define STREAM_KIP_VALIDATE_EPS(ep1, ep2) \
            (!(STREAM_EP_IS_REALEP_ID(ep1) || STREAM_EP_IS_REALEP_ID(ep2)))
#else

#define STREAM_KIP_VALIDATE_EPS(ep1, ep2) \
            (!(STREAM_EP_IS_REALEP_ID(ep1) && STREAM_EP_IS_REALEP_ID(ep2)))
#endif

/****************************************************************************
Variable Definitions
*/
/**
 * This keeps information about the remote transforms. On P0, this list contains
 * both transform associated with KIP endpoints as well as P1 transforms.
 * On P1 list, it contains only the KIP transforms. P0 and P1 local transforms
 * are maintained in its transform_list.
 **/
extern STREAM_KIP_TRANSFORM_INFO *kip_transform_list;

/****************************************************************************
Function Declarations
*/

/* IPC event callback handlers.*/
/**
 * \brief Indication from IPC when the data channel activated
 *
 * \param status     - STATUS_OK on success
 * \param proc_id    - The remote processor id connected to the data channel
 * \param channel_id - The data channel id
 *
 * \return STATUS_KYMERA
 */
STATUS_KYMERA stream_kip_data_channel_activated(STATUS_KYMERA status,
                                                PROC_ID_NUM proc_id,
                                                uint16 channel_id);
/**
 * \brief Indication from IPC when the data channel deactivated
 *
 * \param status  STATUS_OK on success
 * \param channel The data channel id
 *
 * \return STATUS_KYMERA
 */
STATUS_KYMERA stream_kip_data_channel_deactivated(STATUS_KYMERA status,
                                                  uint16 channel);

/* TODO: This #ifndef is temporary, to not stop crescendo tests  */
/* Should properly resolve unit test build errors without ifndef */
#ifndef UNIT_TEST_BUILD
/**
 * \brief Create a connect info record during connection state and partially
 *        initialise it.
 *
 * \param con_id                - The packed connection id
 * \param source_id             - The source id at the local side
 * \param sink_id               - The sink id at the  local side
 * \param ep_location           - Location of endpoints
 * \param callback              - The callback to be called after handling the
 *                                response
 * \param data_format           - The data format of the local endpoint.
 * \param sync_shadow_eps       - Parameter to tell the other side if the
 *                                shadow endpoints should be synced
 *
 * \return  A connect information record or NULL
 */
STREAM_KIP_CONNECT_INFO *stream_kip_create_connect_info_record_ex(CONNECTION_LINK con_id,
                                                                  unsigned source_id,
                                                                  unsigned sink_id,
                                                                  STREAM_EP_LOCATION ep_location,
                                                                  STREAM_TRANSFORM_CBACK callback,
                                                                  AUDIO_DATA_FORMAT data_format,
                                                                  bool sync_shadow_eps);
#endif

/**
 * \brief Create a connect info record during connection state and partially
 *        initialise it. This is different from the above in that parameters
 *        'block_size' and 'period' are replaced by default values for a 
 *        shadow endpoint. This is useful at the start of the shadow 
 *        endpoint connection setup sequence.
 *
 * \param con_id                - The packed connection id
 * \param source_id             - The source id at the local side
 * \param sink_id               - The sink id at the  local side
 * \param ep_location           - Location of endpoints
 * \param callback              - The callback to be called after handling the
 *                                response
 * \param sync_shadow_eps       - Parameter to tell the other side if the
 *                                shadow endpoints should be synced
 *
 * \return  A connection information record or NULL
 */
STREAM_KIP_CONNECT_INFO *stream_kip_create_connect_info_record(CONNECTION_LINK con_id,
                                                               unsigned source_id,
                                                               unsigned sink_id,
                                                               STREAM_EP_LOCATION ep_location,
                                                               STREAM_TRANSFORM_CBACK callback,
                                                               bool sync_shadow_eps);

/**
 * \brief Generate a transform id and send a KIP stream connect request
 *        Sends a KIP connect REQ to secondary core(s) - only used on P0
 *
 * \param packed_con_id     The packed connection id
 * \param remote_source_id  The source endpoint id
 * \param remote_sink_id    The sink endpoint id
 * \param state             The connect state info
 *
 * \return TRUE on success
 */
bool stream_kip_connect_endpoints(CONNECTION_LINK packed_con_id,
                                  unsigned source_id,
                                  unsigned sink_id,
                                  STREAM_KIP_CONNECT_INFO *state);

/**
 * \brief Create local endpoints and send a remote request to create
 *        endpoints at secondary core.
 *
 * \param packed_con_id    The packed connection id
 * \param remote_source_id The source endpoint id
 * \param remote_sink_id   The sink endpoint id
 * \param state            The connect state info
 *
 * \return TRUE on success
 */
bool stream_kip_create_endpoints(CONNECTION_LINK packed_con_id,
                                 unsigned source_id,
                                 unsigned sink_id,
                                 STREAM_KIP_CONNECT_INFO *state);

/**
 * \brief Handle the connect resp from the secondary core
 *
 * \param con_id       - The connection id
 * \param status       - Status of the request
 * \param transform_id - Transform id returned
 */
void stream_kip_connect_response_handler(CONNECTION_LINK con_id,
                                         STATUS_KYMERA status,
                                         TRANSFORM_ID transform_id);

/**
 * \brief Handle the transform disconnect response from the remote
 *
 * \param con_id     - The connection id
 * \param status     - Status of the request
 * \param count      - The number of disconnected transforms
 * \param state      - The disconnect state
 */
void stream_kip_transform_disconnect_response_handler(CONNECTION_LINK con_id,
                                                      STATUS_KYMERA status,
                                                      unsigned count);

/**
 * \brief Handling the incoming create endpoint response.
 *
 * \param con_id      The connection id
 * \param status      Status
 * \param channel_id  The data channel id. This must not be 0.
 * \param buffer_size Negotiated buffer size for the connection
 * \param flags       The buffer related flags
 */
void stream_kip_create_endpoints_response_handler(CONNECTION_LINK con_id,
                                                  STATUS_KYMERA status,
                                                  unsigned channel_id,
                                                  unsigned buffer_size,
                                                  unsigned flags,
                                                  unsigned data_format);

/**
 * \brief Handling the incoming destroy endpoint response.
 *
 * \param con_id The connection id
 * \param status Status
 * \param state  The connection state information.
 */
void stream_kip_destroy_endpoints_response_handler(CONNECTION_LINK con_id,
                                                   STATUS_KYMERA status);

/**
 * \brief Create disconnect info record
 *
 * \param con_id      - The connection id
 * \param count       - Number of transforms in the list
 * \param ep_disc_cb  - Flag as to which callback to call
 *                      (state->callback.disc_cb or .tr_disc_cb)
 * \param transforms  - The transform list
 * \param callback    - The callback
 *
 * \return            Pointer to an allocated disconnect info object
 */
STREAM_KIP_TRANSFORM_DISCONNECT_INFO *stream_kip_create_disconnect_info_record(CONNECTION_LINK con_id,
                                                                               unsigned count,
                                                                               bool ep_disc_cb,
                                                                               TRANSFORM_ID *transforms,
                                                                               STREAM_KIP_TRANSFORM_DISCONNECT_CB callback);

/**
 * \brief  Send a KIP stream disconnect
 *
 * \param state        Disconnect state.
 * \param px_tr_offset Offset to start processing the transform list
 *                     of the state for the secondary core.
 *
 * \return  TRUE on success
 */
bool stream_kip_transform_disconnect(STREAM_KIP_TRANSFORM_DISCONNECT_INFO *state,
                                     unsigned px_tr_offset);

/**
 * \brief Disconnect the transform associated with a KIP endpoint
 *
 * \param endpoint    - The KIP endpoint
 * \param proc_id     - The processor_id
 *
 * \return            TRUE if success, FALSE otherwise
 */
bool stream_kip_disconnect_endpoint(ENDPOINT *endpoint, PROC_ID_NUM proc_id);

/**
 * \brief  Destroy the data channel from ipc
 *
 * \param  channel - The data channel to be destroyed.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_destroy_ipc(uint16 channel);

/**
 * \brief  Shadow endpoint calls this to destroy the data channel
 *
 * \param  channel - The data channel to be destroyed.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_destroy(uint16 channel);

/**
 * \brief  Deactivate the data channel from ipc
 *         Shadow endpoint calls this directly to deactivate meta data channel
 *
 * \param channel - The data channel to be deactivated.
 *
 * \return FALSE if deactivation failed, TRUE if success
 */
bool stream_kip_data_channel_deactivate_ipc(uint16 channel);

/**
 * \brief  Shadow endpoint calls this to deactivate data channel
 *
 * \param  channel - The data channel to be deactivated.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_deactivate(uint16 channel);

/**
 * \brief Get KIP transform info from KIP transform information list
 *
 * \param id          - KIP transform info id
 *
 * \return            Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_id(TRANSFORM_INT_ID id);

/**
 * \brief Get KIP transform info from KIP transform information list
 *
 * \param epid        - The ID of the endpoint that is known
 *
 * \return            Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_epid(unsigned epid);

/**
 * \brief Helper function to find the ID of a remote endpoint
 *        connected to a known endpoint.
 *
 * \param epid        - The ID of the endpoint that is known
 *
 * \return The ID of the endpoint connected to endpoint with ID epid. 
 *         0 if not found.
 */
unsigned stream_kip_connected_to_epid(unsigned epid);

/**
 * \brief Helper function to create new remote transform info entry
 *        and add it to the respective list.
 *
 * \param id           Internal transform id
 * \param processor_id Remote processor id
 * \param source_id    Remote source id
 * \param sink_id      Remote sink id
 * \param data_chan_id Data channel id
 *
 * \return Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_add_transform_info(TRANSFORM_INT_ID id,
                                                         PROC_ID_NUM processor_id,
                                                         unsigned source_id,
                                                         unsigned sink_id,
                                                         uint16 data_chan_id);

/**
 * \brief Helper function to remove remote transform info
 *        entry from respective list
 *
 * \param transform - Transform to be removed
 */
void stream_kip_remove_transform_info(STREAM_KIP_TRANSFORM_INFO *transform);

/**
 * \brief Helper function to remove remote transform info
 *        entry from respective list
 *
 * \param tr_id Internal transform id of transform to be removed
 */
void stream_kip_remove_transform_info_by_id(TRANSFORM_INT_ID tr_id);

/**
 * \brief Helper function to retrieve entry in remote transform
 *        info list based on data channel ID.
 *
 * \param data_chan_id - data channel id
 *
 * \return             Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_chanid(uint16 data_chan_id);

/**
 * \brief Handling the incoming stream disconnect request from P0
 *
 * \param con_id     - The connection id
 * \param count      - Number of transforms to disconnect
 * \param tr_list    - The list of transforms
 */
void stream_kip_transform_disconnect_request_handler(CONNECTION_LINK con_id,
                                                     unsigned count,
                                                     TRANSFORM_ID *tr_list);

/**
 * \brief Handling the incoming kip_transform_list entry remove
 *        request from secondary core
 *
 * \param con_id     - The connection id
 * \param count      - Number of transforms to cleanup/remove
 * \param tr_list    - The list of transforms
 */
void stream_kip_transform_list_remove_entry_request_handler(CONNECTION_LINK con_id,
                                                            unsigned count,
                                                            TRANSFORM_ID *tr_list);

/**
 * \brief Handling the incoming stream connect request from P0
 *
 * \param con_id       - The connection id
 * \param source_id    - The source endpoint id
 * \param sink_id      - The sink endpoint id
 * \param transform_id - The transform id
 * \param channel_id   - The data channel id
 */
void stream_kip_connect_request_handler(CONNECTION_LINK con_id,
                                        unsigned source_id,
                                        unsigned sink_id,
                                        TRANSFORM_ID transform_id,
                                        unsigned channel_id);

#if !defined(COMMON_SHARED_HEAP)
/**
 * \brief Handle the incoming stream connect confirm request
 *        from the primary core
 *
 * \param con_id       - The connection id
 * \param conn_to      - The shadow endpoint ID connected to
 */
void stream_kip_connect_confirm_handler(CONNECTION_LINK con_id,
                                        unsigned conn_to);
#endif /* !COMMON_SHARED_HEAP */


/**
 * \brief Handling the incoming create endpoints request from P0
 *
 * \param con_id             - The connection id
 * \param source_id          - The source endpoint id
 * \param sink_id            - The sink endpoint id
 * \param channel_id         - The data channel id
 * \param buffer_size        - The buffer size for negotiation
 * \param flags              - The buffer related flags
 * \param sync_shadow_eps    - This parameter informs the secondary core if
 *                             it should sync the shadow endpoints on the same
 *                             port
 */
void stream_kip_create_endpoints_request_handler(CONNECTION_LINK con_id,
                                                 unsigned source_id,
                                                 unsigned sink_id,
                                                 unsigned channel_id,
                                                 unsigned buffer_size,
                                                 unsigned flags,
                                                 AUDIO_DATA_FORMAT data_format,
                                                 bool sync_shadow_eps);

/**
 * \brief Handling the incoming destroy endpoints request from P0
 *
 * \param con_id     - The connection id
 * \param source_id  - The source endpoint id
 * \param sink_id    - The sink endpoint id
 */
void stream_kip_destroy_endpoints_request_handler(CONNECTION_LINK con_id,
                                                  unsigned source_id,
                                                  unsigned sink_id);

/**
 * \brief   Get metadata_buffer from the same endpoint base
 *
 * \param   endpoint   - Pointer to endpoint object
 *
 * \return  metadata_buffer if it found any, otherwise, NULL
 */
extern tCbuffer *stream_kip_return_metadata_buf(ENDPOINT *ep);

#if !defined(COMMON_SHARED_HEAP)
/**
 * \brief   Check if this endpoint is in the last metadata data connection
 *
 * \param   endpoint   - Pointer to endpoint object
 *
 * \return  TRUE if it is, otherwise, FALSE;
 */
extern bool stream_kip_is_last_meta_connection(ENDPOINT *endpoint);

/**
 * \brief   Request remote to set the activated flag in the kip state with
 *          an existing metadata data channel. Then, send a response back
 *
 * \param   packed_con_id   - Packed send/receive connection ID
 * \param   meta_channel_id - The existing metadata data channel id
 */
extern void stream_kip_metadata_channel_activated_req_handler(CONNECTION_LINK packed_con_id,
                                                              uint16 meta_channel_id);
/**
 * \brief   Response to local to set the activated flag in the kip state
 *          with an existing metadata data channel
 *
 * \param   packed_con_id   - Packed send/receive connection ID
 * \param   status          - STATUS_KYMERA
 * \param   meta_channel_id - The existing metadata data channel id
 */
extern void stream_kip_metadata_channel_activated_resp_handler(CONNECTION_LINK packed_con_id,
                                                               STATUS_KYMERA status,
                                                               uint16 meta_channel_id);
#endif /* COMMON_SHARED_HEAP */

extern unsigned stream_kip_get_transform_source_id(STREAM_KIP_TRANSFORM_INFO *tfm);
extern unsigned stream_kip_get_transform_sink_id(STREAM_KIP_TRANSFORM_INFO *tfm);
extern STREAM_KIP_TRANSFORM_INFO *stream_kip_get_next_transform(STREAM_KIP_TRANSFORM_INFO *tfm);

/**
 * \brief Count how many connections with the specified external source ID.
 *
 * \param source_id  internal source ID if filtering list, or 0 for unfiltered list
 * \param sink_id internal sink ID if filtering list, or 0 for unfiltered list
 *
 * \return Number of P0 transforms
 */
unsigned num_p1_transforms(unsigned source_id, unsigned sink_id);

/**
 * \brief Get a "list" of P1 transform IDs, source & sink terminal ID triads,
 *        and add it to the end of an exiting list (the P0 list of the same).
 *
 * \param source_id  internal source ID if filtering list, or 0 for unfiltered list
 * \param sink_id internal sink ID if filtering list, or 0 for unfiltered list
 * \param length  pointer to location where the constructed list is
 * \param conn_list pointer to a pointer to vector where the required information was assembled
 */
void stream_get_p1_connection_list(unsigned source_id, unsigned sink_id, unsigned p0_count, unsigned length, unsigned** conn_list);

#endif /* STREAM_KIP_H */
