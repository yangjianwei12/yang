/****************************************************************************
 * Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
 ****************************************************************************/
/**
 * \defrocks ksp
 * \ingroup capabilities
 * \file  ksp_capability.h
 * \ingroup ksp
 *
 * ksp operator private header file. <br>
 *
 */
#ifndef _KSP_CAPABILITY_H_
#define _KSP_CAPABILITY_H_
/*****************************************e************************************
Include Files
*/
#include "capabilities.h"
#include "ksp_packet.h"
#ifdef KSP_DUAL_CORE_SUPPORT
#include "proc/proc.h"
#endif /* KSP_DUAL_CORE_SUPPORT */
#include "ksp_gen_c.h"
#ifdef INSTALL_TIMING_TRACE
#include "timing_trace_irt/timing_trace.h"
#include "timing_trace_recorder.h"
#endif /* INSTALL_TIMING_TRACE */
/*****************************************************************************
Constants
*/
#define KSP_KSP_VERSION_MINOR 3
#define STREAM_DEFAULT_SAMPLES           96     /* default samples/channel/packet */
#define KSP_MAX_NR_SAMPLES        256    /* max samples/channel/packet */
#define KSP_MIN_NR_SAMPLES        32     /* min samples/channel/packet */
#define KSP_FAST_READ_PERIOD      1000   /* fast reader task period */
#define KSP_MAX_CHANNELS          4      /* max number of channels per stream,
                                                   not more than 8 */
#define KSP_MAX_COPY              2048    /* maximum number of words to read from
                                                 * transform buffers in one go */
#ifdef INSTALL_TIMING_TRACE
#define KTTR_MAX_COPY             1024    /* maximum number of words to read from kttr buffers
                                           * in one go */
#endif /* INSTALL_TIMING_TRACE */
#define KSP_MAX_STORED_PACKETS   5       /* Maximum number of packets that can be
                                                 * stored internally, have direct impact on the
                                                 * memory usage by the operator.
                                                 */
#define KSP_PACKET_MIN_SAMPLES_UNIT  8   /* Min unit for samples per packet that is configured by user.
                                                 * Samples in packet will be rounded up to a multiple of this value
                                                 */
#define KSP_OUTPUT_PACKET_TERMINAL_INDEX 0
#define KSP_OUTPUT_PACKET_TERMINAL_MASK (1<<KSP_OUTPUT_PACKET_TERMINAL_INDEX)
#define KSP_MAX_OUTPUT_TERMINALS 1
#ifdef KSP_DUAL_CORE_SUPPORT
#define KSP_INPUT_FORWARDING_TERMINAL_INDEX 0
#define KSP_INPUT_FORWARDING_TERMINAL_MASK (1<<KSP_INPUT_FORWARDING_TERMINAL_INDEX)
#define KSP_MAX_INPUT_TERMINALS 1
#else /* KSP_DUAL_CORE_SUPPORT */
#define KSP_MAX_INPUT_TERMINALS 0
#endif /* KSP_DUAL_CORE_SUPPORT */

#define KSP_OUTPUT_BUFFER_SIZE_IN_PACKETS       (2)
#ifdef KSP_DUAL_CORE_SUPPORT
/* Increase the size by one packet for double buffering when crossing processor boundary. */
#define KSP_OUTPUT_BUFFER_SIZE_IN_PACKETS_P1    (KSP_OUTPUT_BUFFER_SIZE_IN_PACKETS+1)
#endif /* KSP_DUAL_CORE_SUPPORT */

/****************************************************************************
Public Type Declarations
*/

/* streaming methods */
typedef enum ksp_streaming_mode
{
    /* KSP has a connected output, in this
     * method KSP just packetises the sniffed
     * data and pushes them to output. It will be the
     * responsibility of someone else to route them
     * out to a host. This method is mainly to send
     * the data via audio data service to apps and from
     * there route it out through test tunnel facility.
     */
    KSP_STREAMING_OUTPUT =  0x0,

    /* KSP has no connected output, in this
     * mode the operator will send packetised data
     * directly to transaction bridge.
     */
    KSP_STREAMING_TRB =     0x1,

    KSP_STREAMING_INVALID = MAXINT
}KSP_STREAMING_MODE;

typedef enum ksp_stream_id
{
    KSP_STREAM0 = 0,      /* stream 0 */
    KSP_STREAM1 = 1,      /* stream 1 */
    KSP_MAX_STREAMS = 2,  /* max streams (shall not exceed 4) */
#ifdef KSP_DUAL_CORE_SUPPORT
    KSP_STREAM_FORWARD = KSP_MAX_STREAMS, /* Source to check is the forwarding input */
    KSP_MAX_SOURCES = KSP_MAX_STREAMS + 1,
#else /* KSP_DUAL_CORE_SUPPORT */
    KSP_MAX_SOURCES = KSP_MAX_STREAMS,
#endif /* KSP_DUAL_CORE_SUPPORT  */
    KSP_INVALID_STREAM_ID = MAXINT
}KSP_STREAM_ID;

#ifdef INSTALL_TIMING_TRACE
/** This describes the enable_mask field of the SET_STREAM_TIMING_TRACE operator message */
enum {
    /** When this bit is set, buffer level recording is enabled. */
    TIMING_TRACE_ENABLE_TRANSFORMS_SHIFT        = 0,
    TIMING_TRACE_ENABLE_TRANSFORMS_MASK         = (1 << TIMING_TRACE_ENABLE_TRANSFORMS_SHIFT),

    /** When this bit is set, buffer levels are recorded for all transforms
     * and the transform_enable_mask bitmap is ignored.
     */
    TIMING_TRACE_ENABLE_ALL_TRANSFORMS_SHIFT    = 1,
    TIMING_TRACE_ENABLE_ALL_TRANSFORMS_MASK     = (1 << TIMING_TRACE_ENABLE_ALL_TRANSFORMS_SHIFT),

    TIMING_TRACE_ENABLE_TRANSFORMS_OPTIONS_MASK = TIMING_TRACE_ENABLE_TRANSFORMS_MASK |
                                                  TIMING_TRACE_ENABLE_ALL_TRANSFORMS_MASK,
    TIMING_TRACE_ENABLE_TRANSFORMS_SELECTIVE    = TIMING_TRACE_ENABLE_TRANSFORMS_MASK,

    /** When this bit is set, metadata info is recorded for buffers whose
     * levels are recorded and which have metadata
     */
    TIMING_TRACE_ENABLE_METADATA_INFO_SHIFT     = 2,
    TIMING_TRACE_ENABLE_METADATA_INFO_MASK      = (1 << TIMING_TRACE_ENABLE_METADATA_INFO_SHIFT),

    /** When this bit is set, written data is recorded for one transform
     * buffer
     */
    TIMING_TRACE_ENABLE_TIMED_DATA_SHIFT        = 3,
    TIMING_TRACE_ENABLE_TIMED_DATA_MASK         = (1<< TIMING_TRACE_ENABLE_TIMED_DATA_SHIFT)
};

/** This numbers the channel in a TTR stream */
enum {
    /** Index of channel for TTR's data */
    KSP_TTR_CHANNEL      = 0,
    /** Number of channels for KSP stream purposes */
    KSP_TTR_NUM_CHANNELS
};
#endif /* INSTALL_TIMING_TRACE */

typedef enum {
    /** Output is by TRB, or is by connection and has space */
    KSP_OUTPUT_STATUS_DEFAULT,

    /** Output is by connection and is full */
    KSP_OUTPUT_STATUS_FULL

} KSP_OUTPUT_STATUS;

/* KSP Stream object */
typedef struct ksp_stream
{
    /* stream id, this will be KSP_STREAMING_INVALID
     * until the stream has been successfully configured by
     * the user. The nr_active_stream field in KSP_OP_DATA
     * will be number of all streams with this field is valid.
     */
    KSP_STREAM_ID stream_id;

    /* number of channels in this stream, cannot exceed
     * KSP_MAX_CHANNELS, trying to configure a stream
     * with more channels will fail at the configuration point.
     */
    unsigned nr_channels;

    /* number of samples/channel/packet for this stream,
     * limited to KSP_MIN/MAX_NR_SAMPLES range */
    unsigned nr_samples;

    /* sequence counter for this stream's packets,
     * it's a 128-mode counter updated once for every
     * sent packet for this stream.
     */
    unsigned sequence_counter;

    /* frame length before packetising */
    unsigned frame_len;

    /* length of packets in 32-bit words */
    unsigned packet_len;

    /* data type for all channels in this stream */
    KSP_DATA_TYPE data_type;

    /* channel info */
    unsigned channel_info;

    /* reader buffer size in words */
    unsigned reader_buffer_size_cfg;

    /* ---------- channel fields ---------------------
     * We could define just one array of channel structs,
     * but for efficiency of assembly routines it's better
     * to have individual arrays for each field.
     * ---------------------------------------------*/

    /* array of transform buffers, NULL for unused channels */
    tCbuffer *transform_bufs[KSP_MAX_CHANNELS];

    /* array of buffers cloning transform buffers, NULL for unused channels */
    tCbuffer *clone_transform_bufs[KSP_MAX_CHANNELS];

    /* internal buffers for fast reading of transform buffers */
    tCbuffer *reader_bufs[KSP_MAX_CHANNELS];

    /* transform IDs, stored only for debugging purposes */
    TRANSFORM_ID transform_ids[KSP_MAX_CHANNELS];

} KSP_STREAM;

/* capability-specific extra operator data */
typedef struct ksp_extra_op
{
    CPS_PARAM_DEF params_def;
    unsigned ReInitFlag;
    unsigned config;

    /* KSP Stream objects, invalid for unused streams */
    KSP_STREAM streams[KSP_MAX_STREAMS];

    /* number of active streams, an active stream is
     * a stream with valid config from user. This will
     * never exceed KSP_MAX_STREAMS as configs
     * for higher stream IDs will be rejected. The field
     * only shows the total active streams, for individual
     * streams the stream_id field needs to be checked,
     * see KSP_STREAM_IS_VALID macro.
     */
    unsigned  nr_active_streams;

    /* streaming mode, to connected output or directly to trb*/
    KSP_STREAMING_MODE streaming_mode;

    /* output buffer for packetised data, depending on streaming_mode
     * the content of this buffer will be routed to a connected output
     * or directly to TRB
     */
    tCbuffer *output_buffer;

    /* this is just a clone output_buffer, used to make it
     * easier to do packetising in multiple stage while output_buffer
     * might be read by an external entity
     */
    tCbuffer *clone_output_buffer;

    /* timer task period for fast reader thread
     * in microseconds
     */
    unsigned fast_read_period;

    /* Kick ID for fast reader thread */
    tTimerId fast_read_kick_id;

    /* maximum frame length needed among all streams stream_len fields,
     * this will be 0 if no valid stream has been configured(nr_active_streams==0)
     * and updated every time a new stream is configured successfully, or a stream
     * config gets removed (becomes invalid).
     */
    unsigned max_frame_len_in_words;

    /* total data sent to host, in octets */
    unsigned total_data_sent;

    /* next stream to check for sending data */
    KSP_STREAM_ID next_source_to_check;

#ifdef INSTALL_EVENT_BASED_SNIFF
    /* total number of transforms using event-based sniffing,
     * For info only.
     */
    unsigned total_configured_in_place_transforms;
#endif

#ifdef KSP_DUAL_CORE_SUPPORT
    /* Input connection buffer, forwarding data from a KSP on processor 1 */
    tCbuffer *forwarding_input_buffer;
#endif

#ifdef INSTALL_TIMING_TRACE
    /* Timing trace setup buffer size in words */
    uint16 ttr_setup_buffer_size;
    /* Timing trace event buffer size in words */
    uint16 ttr_event_buffer_size;
    /* Max copy per fast reader run for TTR */
    uint16 ttr_max_copy;
    /* Timing trace options */
    uint16 ttr_enable_mask;
#ifdef KSP_KTTR_TIMED_DATA_SUPPORT
    /* Timed data endpoint ID */
    uint16 ttr_timed_data_ep_id;
#endif /* KSP_KTTR_TIMED_DATA_SUPPORT */
    /* Timing trace transform bitmask */
    uint32 ttr_transform_enable_mask[TIMING_TRACE_NUM_TRANSFORM_IDS/32];
#endif /* INSTALL_TIMING_TRACE */
} KSP_OP_DATA;

/****************************************************************************
Private Function Definitions
*/
extern void ksp_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);

/* Message handlers */
extern bool ksp_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool ksp_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool ksp_stop_reset(OPERATOR_DATA *op_data,void *message_data, unsigned *response_id, void **response_data);
extern bool ksp_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool ksp_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool ksp_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool ksp_op_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern void ksp_stop_fast_reader(OPERATOR_DATA *op_data);
extern void ksp_cleanup(OPERATOR_DATA *op_data);

/* Operator message handlers */
extern bool ksp_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ksp_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ksp_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ksp_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ksp_opmsg_set_ucid(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ksp_opmsg_get_ps_id(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ksp_opmsg_set_stream_transforms(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
#ifdef INSTALL_TIMING_TRACE
extern bool ksp_opmsg_set_stream_timing_trace(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
#endif /* INSTALL_TIMING_TRACE */
extern bool ksp_set_internal_buffer_size(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

extern void ksp_clear_stream(KSP_OP_DATA *p_ext_data, unsigned stream_id);
extern void ksp_clear_streams(KSP_OP_DATA *p_ext_data);
extern void ksp_init_reader_buf(tCbuffer *stream_buf, tCbuffer *reader_buf);
extern void ksp_init_stream_transform_read(KSP_STREAM *ksp_stream);

extern void ksp_fast_reader_task(void *kick_object);
extern void ksp_copy_from_transform_buffers(KSP_STREAM *stream, unsigned max_copy);
extern unsigned ksp_get_max_frame_length(KSP_OP_DATA *p_ext_data);
extern void ksp_update_stream_length(KSP_STREAM *ksp_stream);
extern bool ksp_write_a_packet(KSP_STREAM *ksp_stream, tCbuffer *out_buf, tCbuffer *clone_out_buf);
extern unsigned ksp_send_frame_via_trb(unsigned words_to_send, uint32 *buf);

/**
 * \brief Get the operator specific data
 */
static inline KSP_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (KSP_OP_DATA *) base_op_get_instance_data(op_data);
}

/**
 * KSP_STREAM_IS_VALID
 * \brief checks whether a stream is valid
 * \param ksp_stream pointer to stream structure
 * \return True if stream has been configured else False
 */
inline static bool KSP_STREAM_IS_VALID(const KSP_STREAM *ksp_stream)
{
    return (ksp_stream->stream_id != KSP_INVALID_STREAM_ID);
}

/**
 * KSP_STREAMS_CONFIGURED
 * \brief checks whether at least one stream has been configured
 * \param ls pointer to the ksp operator specific structure
 * \return True if operator has at least one valid stream.
 */
inline static bool KSP_STREAMS_CONFIGURED(const KSP_OP_DATA *ksp)
{
    return (ksp->nr_active_streams != 0);
}

/**
 * \brief Checks if there is a forwarding input connection
 * \param p_ext_data Operator structure
 * \return TRUE if operator has a forwarding input connection.
 */
#ifdef KSP_DUAL_CORE_SUPPORT
inline static bool KSP_FORWARDING_CONFIGURED(const KSP_OP_DATA *p_ext_data)
{
    return p_ext_data->forwarding_input_buffer != NULL;
}
#else /* KSP_DUAL_CORE_SUPPORT */
#define KSP_FORWARDING_CONFIGURED(p_ext_data) FALSE
#endif /* KSP_DUAL_CORE_SUPPORT */

/**
 * \brief Checks whether the operator has a valid configuration.
 * \param p_ext_data Operator structure
 * \return TRUE if operator has a valid configuration.
 */
#ifdef KSP_DUAL_CORE_SUPPORT
#define KSP_CONFIGURED(p_ext_data) ksp_configured(p_ext_data)
extern bool ksp_configured(const KSP_OP_DATA *p_ext_data);
#else /* KSP_DUAL_CORE_SUPPORT */
#define KSP_CONFIGURED(p_ext_data) KSP_STREAMS_CONFIGURED(p_ext_data)
#endif /* KSP_DUAL_CORE_SUPPORT */


#ifdef INSTALL_TIMING_TRACE
/**
 * \brief Retrieve a stream with data type TTR, if there is one
 * \param p_ext_data Pointer to the operator specific structure
 * \param except_stream_id Stream ID to ignore
 * \return TRUE if there is a KSP_DATATYPE_TTR stream
 */
extern KSP_STREAM* ksp_ttr_find_stream(KSP_OP_DATA *p_ext_data,
                                       KSP_STREAM_ID except_stream_id);

void ksp_timing_trace_start(KSP_OP_DATA *p_ext_data);
void ksp_timing_trace_stop(void);
void ksp_copy_from_timing_trace(KSP_STREAM *stream);
#endif /* INSTALL_TIMING_TRACE */
/**
 * ksp_enable_event_based_sniff_for_in_place_transforms
 * \brief enanble invoking fast reader when data written to in-place
 *  transform buffers *
 * \param op_data Pointer to the ksp operator data
 * \param enable whether to enable or disable the event based sniff
 */
void ksp_event_based_sniff_enable_for_in_place_transforms(OPERATOR_DATA *op_data, bool enable);
#ifdef INSTALL_EVENT_BASED_SNIFF

#endif /* INSTALL_EVENT_BASED_SNIFF */
#endif /* _KSP_CAPABILITY_H_ */
