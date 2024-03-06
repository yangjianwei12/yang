/****************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
#ifndef SOURCE_SYNC_DEFS_H
#define SOURCE_SYNC_DEFS_H

/*****************************************************************************
Include Files
*/
/* Enable printf format checking in _gcc_warnings builds */
#define AUDIO_LOG_CHECK_FORMATS

#include "capabilities.h"
#include "common_conversions.h"
#include "cbops_mgr/cbops_mgr.h"
#include "cbops_mgr/cbops_flags.h"
#include "rate/rate.h"
#include "source_sync/source_sync_gen_c.h"


/****************************************************************************
Module Private Macros
*/

#define SRC_SYNC_VERSION_MINOR 1

/* Define this to enable most non-warning trace printouts */
/* #define SOSY_VERBOSE */

/* Define this to number log messages, in order to detect gaps */
#define SOSY_NUMBERED_LOG_MESSAGES

/* Define this in order to only see the first n messages */
#define SOSY_LOG_MESSAGE_LIMIT (MAXINT)

/* Define to check alignment of output metadata and cbuffer pointers
 * before/after transfer */
/* #define SOSY_CHECK_METADATA_TRANSPORT_POINTERS */
/* #define SOSY_ASSERT_METADATA_TRANSPORT_POINTERS */

/* Define to check for metadata tag leaks */
/* #define SOSY_CHECK_METADATA_TAG_COUNTS */

#ifdef SOSY_VERBOSE

#ifdef SOSY_NUMBERED_LOG_MESSAGES
#define SOSY_PFX_FMT "src_sync #%d "
#define SOSY_PFX_ARG src_sync_trace_serial++
#else /* SOSY_NUMBERED_LOG_MESSAGES */
#define SOSY_PFX_FMT "src_sync 0x%04x "
#define SOSY_PFX_ARG op_extra_data->id
#endif /* SOSY_NUMBERED_LOG_MESSAGES */

#if defined(SOSY_NUMBERED_LOG_MESSAGES)&&defined(SOSY_LOG_MESSAGE_LIMIT)
#define SOSY_MSG_PRE(E) do { if(((((const SRC_SYNC_OP_DATA*)op_extra_data)->trace_enable & (E)) == (E)) && (src_sync_trace_serial <= src_sync_trace_limit))
#define SOSY_MSG_POST   ; } while(0)
#else
#define SOSY_MSG_PRE(E) do { if((((const SRC_SYNC_OP_DATA*)op_extra_data)->trace_enable & (E)) == (E))
#define SOSY_MSG_POST   ; } while(0)
#endif

#define SOSY_MSG(E,F)                 SOSY_MSG_PRE(E) L2_DBG_MSG1(SOSY_PFX_FMT F, SOSY_PFX_ARG) SOSY_MSG_POST
#define SOSY_MSG1(E,F,P1)             SOSY_MSG_PRE(E) L2_DBG_MSG2(SOSY_PFX_FMT F, SOSY_PFX_ARG, (P1)) SOSY_MSG_POST
#define SOSY_MSG2(E,F,P1,P2)          SOSY_MSG_PRE(E) L2_DBG_MSG3(SOSY_PFX_FMT F, SOSY_PFX_ARG, (P1),(P2)) SOSY_MSG_POST
#define SOSY_MSG3(E,F,P1,P2,P3)       SOSY_MSG_PRE(E) L2_DBG_MSG4(SOSY_PFX_FMT F, SOSY_PFX_ARG, (P1),(P2),(P3)) SOSY_MSG_POST
#define SOSY_MSG4(E,F,P1,P2,P3,P4)    SOSY_MSG_PRE(E) L2_DBG_MSG5(SOSY_PFX_FMT F, SOSY_PFX_ARG, (P1),(P2),(P3),(P4)) SOSY_MSG_POST

/** When SOSY_VERBOSE is defined, SOSY_IF_VERBOSE passes through its argument. */
#define SOSY_IF_VERBOSE(X) X

#else /* SOSY_VERBOSE */

/** SOSY_MENTION causes the compiler to check the argument's validity without
 * emitting any code. The cast inside sizeof is needed in case X is a bitfield.
 */
#define SOSY_MENTION(X) (void)sizeof((uintptr_t)(X))

/** SOSY_IF_VERBOSE counteracts SOSY_MENTION, i.e. prevents the compiler from
 * looking at its argument, if SOSY_VERBOSE is undefined. Use it only for
 * arguments to SOSY_MSGx which are declared within #ifdef SOSY_VERBOSE, i.e.
 * variables only calculated for logging purposes.
 */
#define SOSY_IF_VERBOSE(X) 0

#define SOSY_MSG(E,F)                 do{SOSY_MENTION(E), SOSY_MENTION(F); }while(0)
#define SOSY_MSG1(E,F,P1)             do{SOSY_MENTION(E), SOSY_MENTION(F), SOSY_MENTION(P1); }while(0)
#define SOSY_MSG2(E,F,P1,P2)          do{SOSY_MENTION(E), SOSY_MENTION(F), SOSY_MENTION(P1), SOSY_MENTION(P2); }while(0)
#define SOSY_MSG3(E,F,P1,P2,P3)       do{SOSY_MENTION(E), SOSY_MENTION(F), SOSY_MENTION(P1), SOSY_MENTION(P2), SOSY_MENTION(P3); }while(0)
#define SOSY_MSG4(E,F,P1,P2,P3,P4)    do{SOSY_MENTION(E), SOSY_MENTION(F), SOSY_MENTION(P1), SOSY_MENTION(P2), SOSY_MENTION(P3), SOSY_MENTION(P4);}while(0)
#endif /* SOSY_VERBOSE */


/** Keep this in sync with src_sync_sink_state */
#define SRC_SYNC_FOR_EACH_SINK_STATE(X) \
    X(NOT_CONNECTED) \
    X(FLOWING) \
    X(PENDING) \
    X(STALLED) \
    X(RECOVERING_RESTARTING) \
    X(RECOVERING_FILLING) \
    X(RECOVERING_DISCARDING) \
    X(RECOVERING_WAITING_FOR_TAG)

/** Asserts which check internal consistency (due to refactoring,
 * caching conditions, etc.)
 */
#if defined(SOURCE_SYNC_ENABLE_CHECKS) || defined(RUNNING_ON_KALSIM)
#define SRC_SYNC_DEV_ASSERTS
#endif

#ifdef SRC_SYNC_DEV_ASSERTS
#define SOSY_DEV_ASSERT(COND) PL_ASSERT(COND)
#else
#define SOSY_DEV_ASSERT(COND) do{(void)sizeof((bool)(COND));}while(0)
#endif

/****************************************************************************
Module Private Constant Definitions
*/

enum {
    /** default buffer size configuration for this operator;
     * the value 0 causes some automatic calculations to take place instead */
    SRC_SYNC_DEFAULT_BUFFER_SIZE_CONFIG     = 0,

    /** Default buffer size is this factor in Q6.N x system kick period x sample rate */
    SRC_SYNC_AUTO_BUFFER_SS_PERIOD_MULT     = (2 * (1<<(DAWTH-6))),

    /** default sample rate for buffer size calculations */
    SRC_SYNC_DEFAULT_SAMPLE_RATE            = 48000,

    /** default block size for this operator's terminals */
    SRC_SYNC_DEFAULT_BLOCK_SIZE             = 1,

    /** Max number of terminals on either side */
    SRC_SYNC_CAP_MAX_CHANNELS               = 24,

    /** Bit field position of clock index */
    SRC_SYNC_CLOCK_INDEX_LSB_POSN           = 7
};

/** Bitmask of all channel bits; #define because of range */
#define SRC_SYNC_CAP_CHANNELS_MASK          0xFFFFFFU

/**
 * Keep this in sync with SRC_SYNC_FOR_EACH_SINK_STATE.
 * The enum is not defined using that macro in order
 * to allow doxygen comments here.
 */
typedef enum src_sync_sink_state_enum
{
    /** Input is not connected. */
    SRC_SYNC_SINK_NOT_CONNECTED,

    /** Input is flowing at a sustained rate */
    SRC_SYNC_SINK_FLOWING,

    /** Input is limiting transfers but not stalled yet */
    SRC_SYNC_SINK_PENDING,

    /** Waiting longer for input would have caused outputs
     * to underrun. Silence is being inserted.
     */
    SRC_SYNC_SINK_STALLED,

    /** Following a stall, and data being available again, and having
     * determined that the new data constitutes a new start of stream,
     * fill downstream with silence until full, before starting
     * to forward data.
     */
    SRC_SYNC_SINK_RECOVERING_RESTARTING,

    /** Following a stall, and data being available again, and having
     * determined that more silence needs to be still inserted, wait
     * for the downstream to consume the additional silence.
     */
    SRC_SYNC_SINK_RECOVERING_FILLING,

    /** Following a stall, and data being available again, and having
     * determined that data needs to be discarded, more upstream
     * data is needed before resuming copy.
     */
    SRC_SYNC_SINK_RECOVERING_DISCARDING,

    /** Following a stall of a stream which contained
     * TTP or ToA tags previously, data has arrived again
     * but no tag yet. Discard up to the first tag.
     */
    SRC_SYNC_SINK_RECOVERING_WAITING_FOR_TAG,

    SRC_SYNC_NUM_SINK_STATES
} src_sync_sink_state;

typedef enum src_sync_stall_recovery_type_enum
{
    /** The size of a gap, if any, is not known (usually because
     * the input stream has no TTP or ToA tags.)
     */
    SRC_SYNC_STALL_RECOVERY_UNKNOWN,

    /** The audio before and after the stall had timestamps,
     * so the size of the gap could be determined and looked sane.
     */
    SRC_SYNC_STALL_RECOVERY_GAP,

    /** The metadata after the stall contained a stream start flag,
     * or timestamps were available but did not look like a
     * continuation.
     */
    SRC_SYNC_STALL_RECOVERY_RESTART,

    /** There was metadata with time before the stall but not yet
     * after the stall. Keep waiting for tags.
     */
    SRC_SYNC_STALL_RECOVERY_WAITING_FOR_TAG
} src_sync_stall_recovery_type;


/* Bits for the argument to the debug opmsg 0x10 */
typedef enum src_sync_trace_enable_enum
{
    SRC_SYNC_TRACE_ALWAYS           = 0,
    SRC_SYNC_TRACE_KICK             = (1<< 0),
    SRC_SYNC_TRACE_REFRESH          = (1<< 1),
    SRC_SYNC_TRACE_TRANSITION       = (1<< 2),
    SRC_SYNC_TRACE_SINK_STATE       = (1<< 3),
    SRC_SYNC_TRACE_COMPUTE_TRANSFER = (1<< 4),
    SRC_SYNC_TRACE_SRC_SPACE        = (1<< 5),
    SRC_SYNC_TRACE_SRC_TERM_SPACE   = (1<< 6),
    SRC_SYNC_TRACE_SRC_SPACE_FILLED = (1<< 7),
    SRC_SYNC_TRACE_SINK_AVAIL       = (1<< 8),
    SRC_SYNC_TRACE_FLOWING          = (1<< 9),
    SRC_SYNC_TRACE_PENDING          = (1<<10),
    SRC_SYNC_TRACE_STALLED          = (1<<11),
    SRC_SYNC_TRACE_RECOVERED        = (1<<12),
    SRC_SYNC_TRACE_RECOVER_DISCARD  = (1<<13),
    SRC_SYNC_TRACE_PERFORM_TRANSFER = (1<<14),
    SRC_SYNC_TRACE_PEEK_RESUME      = (1<<15),
    SRC_SYNC_TRACE_RCV_GAP          = (1<<16),
    SRC_SYNC_TRACE_FWD_SPLICE       = (1<<17),
    SRC_SYNC_TRACE_METADATA         = (1<<18),
    SRC_SYNC_TRACE_RCV_METADATA     = (1<<19),
    SRC_SYNC_TRACE_SEND_METADATA    = (1<<20),
    SRC_SYNC_TRACE_EOF              = (1<<21),
    SRC_SYNC_TRACE_RATE_MATCH       = (1<<22),
    SRC_SYNC_TRACE_RM_TRANSFER      = (1<<23)
} src_sync_trace_enable;

#ifdef SOSY_VERBOSE
enum {
    SRC_SYNC_DEFAULT_TRACE_ENABLE =
            /* Comment out as desired for debugging
             * (TODO: perhaps a MIB key would be more convenient.) */
            /* SRC_SYNC_TRACE_KICK | */
            /* SRC_SYNC_TRACE_REFRESH | */
            /* SRC_SYNC_TRACE_TRANSITION | */
            /* SRC_SYNC_TRACE_SINK_STATE | */
            /* SRC_SYNC_TRACE_COMPUTE_TRANSFER | */
            /* SRC_SYNC_TRACE_SRC_SPACE | */
            /* SRC_SYNC_TRACE_SRC_TERM_SPACE | */
            /* SRC_SYNC_TRACE_SRC_SPACE_FILLED | */
            /* SRC_SYNC_TRACE_SINK_AVAIL | */
            /* SRC_SYNC_TRACE_FLOWING | */
            /* SRC_SYNC_TRACE_PENDING | */
            /* SRC_SYNC_TRACE_STALLED | */
            /* SRC_SYNC_TRACE_RECOVERED | */
            /* SRC_SYNC_TRACE_RECOVER_DISCARD | */
            /* SRC_SYNC_TRACE_PERFORM_TRANSFER | */
            /* SRC_SYNC_TRACE_PEEK_RESUME | */
            /* SRC_SYNC_TRACE_RCV_GAP | */
            /* SRC_SYNC_TRACE_FWD_SPLICE | */
            /* SRC_SYNC_TRACE_METADATA | */
            /* SRC_SYNC_TRACE_RCV_METADATA | */
            /* SRC_SYNC_TRACE_SEND_METADATA | */
            /* SRC_SYNC_TRACE_EOF | */
            /* SRC_SYNC_TRACE_RATE_MATCH | */
            /* SRC_SYNC_TRACE_RM_TRANSFER | */
            0
};
#endif /* SOSY_VERBOSE */

#ifdef INSTALL_TIMING_TRACE
enum {
    SRC_SYNC_TIMING_EVENT_SOURCE_STATE = TIMING_TRACE_OP_EVENT_OP_BASE,
    SRC_SYNC_TIMING_EVENT_SINK_STATE
};
#endif /* INSTALL_TIMING_TRACE */

enum {
    /** The maximum value is extreme but so far legal:
     * it is the case of each terminal being used and in a group
     * by itself (i.e. the upper bound on the number of source + sink groups)
     */
    SRC_SYNC_BUFFER_LEVEL_HISTORY_MAX = 2 * SRC_SYNC_CAP_MAX_CHANNELS,

    /* The period, in microseconds, over which to accumulate buffer levels,
     * is the system kick period plus a margin for scheduling.
     * This constant is that margin.
     */
    SRC_SYNC_BUFFER_LEVEL_HISTORY_PERIOD_MARGIN = 100,

    /**
     * Extra precision of sample period
     */
    SRC_SYNC_INV_RATE_RESOLUTION = 10,

    /** From stream_endpoint.h: "The number of samples headroom to allow in
     * the buffer to compensate for any rate missmatch variation in buffer
     * levels before the RM system compensates"
     */
    SRC_SYNC_RM_HEADROOM_AMOUNT = 2,

    /** Scale difference between sp_adjust in metadata tags (Q1.N)
     * and rate measurement (QM.22)
     */
    SRC_SYNC_SCALE_SP_ADJUST_TO_RATE_MEASURE =
            (DAWTH-1-STREAM_RATEMATCHING_FIX_POINT_SHIFT)
};


/****************************************************************************
Module Private Type Definitions
*/

/* Forward declare struct types for mutual references */
typedef struct src_sync_terminal_group  SRC_SYNC_TERMINAL_GROUP;
typedef struct src_sync_sink_group      SRC_SYNC_SINK_GROUP;
typedef struct src_sync_source_group    SRC_SYNC_SOURCE_GROUP;

typedef struct src_sync_terminal_entry  SRC_SYNC_TERMINAL_ENTRY;
typedef struct src_sync_sink_entry      SRC_SYNC_SINK_ENTRY;
typedef struct src_sync_source_entry    SRC_SYNC_SOURCE_ENTRY;

typedef struct src_sync_rm_state_struct SRC_SYNC_RM_STATE;

/**
 * Buffer level history
 */
typedef struct src_sync_buffer_level_record
{
    TIME                            timestamp;
    unsigned                        amount;
}
SRC_SYNC_BUFFER_LEVEL_RECORD;

typedef struct src_sync_buffer_level_history
{
    unsigned                        num_entries;
    unsigned                        next_wr_pos;
    SRC_SYNC_BUFFER_LEVEL_RECORD    *entries;
}
SRC_SYNC_BUFFER_LEVEL_HISTORY;

typedef struct src_sync_sink_metadata_str
{
    /** Tag list removed from current buffer */
    metadata_tag*                   received;

    /** The two index distances returned by buff_metadata_remove */
    unsigned                        rcv_beforeidx;
    unsigned                        rcv_afteridx;

    /** Number of samples into the next buffer that are covered by last tag */
    unsigned                        remaining_octets;

    /** Type of timestamp seen on this stream */
    RATE_TIMESTAMP_TYPE             timestamp_type;

    /** Coarse timestamp at start of next buffer */
    TIME                            ts_start_of_next_buffer;

    /** Sp_adjust from last tag */
    int                             sp_adjust;
}
SRC_SYNC_SINK_METADATA;

typedef struct src_sync_source_metadata_str {
    /** Octets ahead covered by tags */
    unsigned                        remaining_octets;

    /** Reuse tag containing EOF rather than delete */
    metadata_tag*                   eof_tag;

    /** Enable TTP on output */
    bool                            provide_ttp;

} SRC_SYNC_METADATA_DEST;


/* Common to source and sink groups */
struct src_sync_terminal_group
{
    SRC_SYNC_TERMINAL_GROUP         *next;
    SRC_SYNC_TERMINAL_ENTRY         *terminals;
    unsigned                        channel_mask;

    /* Number of group (in the order defined in set_..._groups)
     * \note Byte access */
    unsigned                        idx                 : 8;

    /** Set if all terminals in this group are connected.
     * (Maintained by connect/disconnect.)
     * \note Byte access
     */
    bool                            connected           : 8;

    /** Metadata is enabled by set_sink_groups/set_source_groups
     * \note Byte access */
    bool                            metadata_enabled    : 8;

    /** A group specific buffer size has been set
     * \note Byte access */
    bool                            has_buffer_size     : 8;

    tCbuffer*                       metadata_buffer;

    /** Buffer size for terminals in the group. Replaces the operator-
     * wide configuration if has_buffer_size is set.
     * For values, see the buffer_size field in struct src_sync_exop. */
    int                             buffer_size;
};

struct src_sync_sink_group
{
    SRC_SYNC_TERMINAL_GROUP         common;

    unsigned                        transfer_w;

    SRC_SYNC_SOURCE_GROUP*          route_dest;
    bool                            have_route_dest     : 8;

    /** This flag controls whether to consume data if all terminals in
     * the group are connected and none are routed.
     * \note Byte access
     */
    bool                            purge               : 8;

    /** Configure whether rate matching is supported
     * \note Byte access */
    bool                            rate_adjust_enable  : 8;

    /** Set for the first connected non-rate-adjusting group with metadata
     * \note Byte access */
    bool                            ts_rate_master      : 8;

    /** States through the flowing-pending-stalled-recovering cycle
     * \note Byte access */
    src_sync_sink_state             stall_state;

    /** Number of zero samples written during stall */
    unsigned                        inserted_silence_words;

    /** Number of zero samples remaining to write during recovery
     * in state SRC_SYNC_SINK_RECOVERING_FILLING
     */
    unsigned                        stall_recovery_silence_words;

    /** Number of zero samples remaining to discard during recovery
     * in state SRC_SYNC_SINK_RECOVERING_DISCARDING
     */
    unsigned                        stall_recovery_discard_words;

    /** Limit on the SRC_SYNC_SINK_RECOVERING_DISCARDING state
     * in terms of silence words
     */
    unsigned                        stall_recovery_discard_remaining;

    /** Direction of transitions between copying and silence
     * \note Byte access */
    bool                            copy_before_silence     : 8;

    /** In state RECOVERING_FILLING, stop filling as soon as downstream full.
     * Otherwise, stall_recovery_silence_words samples of silence will
     * always be generated.
     * \note Byte access
     */
    bool                            filling_until_full      : 8;

    /** The back kick mode is
     * 0 for "edge": kick back when the input buffer level was above
     *   the threshold before processing and is below the threshold
     *   after processing;
     * 1 for "level": kick back when the input buffer level is below
     *   the threshold after processing.
     */
    OPMSG_COMMON_BACK_KICK_MODE     back_kick_mode          : 8;

    /** This is set at the start of a processing run and indicates
     * that the conditions are fulfilled for checking the input buffer
     * level against the back kick threshold at the end of the run.
     */
    bool                            recheck_back_kick       : 8;

    /** A positive value for the back kick threshold is compared
     * with amount of data in words at the input. For a negative value,
     * the magnitude is compared with the amount of space in words
     * at the input. Zero reverts to the previous method.
     */
    int                             back_kick_threshold;

    /** Source group where to route the incoming metadata */
    SRC_SYNC_SOURCE_GROUP*          metadata_dest;

    /** Information about the last timestamp seen */
    SRC_SYNC_SINK_METADATA          timestamp_state;

    /** Metadata input buffer. Either the metadata connection
     * buffer (common->metadata_buffer) or the output of the
     * rate adjustment stage.
     */
    tCbuffer*                       metadata_input_buffer;

    /** Saved setting from the ratematch manager.
     * It may be set before connect.
     */
    bool                            rm_enact;

    /** Optional rate adjustment fields */
    SRC_SYNC_RM_STATE*              rm_state;
};

struct src_sync_source_group
{
    SRC_SYNC_TERMINAL_GROUP         common;

    SRC_SYNC_METADATA_DEST          metadata_dest;

    /** Sink group which provides metadata */
    SRC_SYNC_SINK_GROUP*            metadata_in;

#ifdef SOSY_CHECK_METADATA_TRANSPORT_POINTERS
    /** Copy of metadata write index before last transfer */
    unsigned                        last_md_write_idx;
#endif
};

/* Common to source and sink entries */
struct src_sync_terminal_entry
{
    /* Efficient iteration over connected and routed terminals */
    SRC_SYNC_TERMINAL_ENTRY         *next;

    /** Index of the terminal in the operator */
    unsigned                        idx;

    /** Index of the channel in a stream */
    unsigned                        idx_in_group;

    /* Connection buffer */
    tCbuffer                        *buffer;

    /* Transfer, relative rate and metadata */
    SRC_SYNC_TERMINAL_GROUP*        group;
};

/* Linked list of Sinks */
struct src_sync_sink_entry
{
    SRC_SYNC_TERMINAL_ENTRY         common;

    /** Route destination */
    SRC_SYNC_SOURCE_ENTRY*          source;

    /** Input buffer. If rate adjustment is enabled,
     * this is the output buffer of the SRA cbops chain,
     * otherwise this is the connection buffer.
     */
    tCbuffer*                       input_buffer;
};

typedef struct src_sync_route_entry
{
    /* Designed gain in dB*60 */
    int                             gain_dB;

    /* Gain applied to route (Q5.XX) */
    unsigned                        gain_lin;

    /* Valid if sample rate field in SET_ROUTE opmsg was non-zero */
    bool                            is_valid;

    /* Pointer to Sink Structure */
    SRC_SYNC_SINK_ENTRY             *sink;

} SRC_SYNC_ROUTE_ENTRY;

/* Linked list of Sources */
struct src_sync_source_entry
{
    SRC_SYNC_TERMINAL_ENTRY         common;

    /** Inverse Transition period for switching Sinks.
        inv_transition is zero for immediate switch
        (inv_transition)  applied to switch out old sink
        (-inv_transition) applied to switch in new sink
        inv_transition = zero when switch complete*/
    int                             inv_transition;

    /** Progress of transition */
    unsigned                        transition_pt;

    /** Current route, containing sink, rate and gain. */
    SRC_SYNC_ROUTE_ENTRY            current_route;

    /** Route to be set after out-transition completes */
    SRC_SYNC_ROUTE_ENTRY            switch_route;

    /** Next source whose route has the same sink, if any */
    SRC_SYNC_SOURCE_ENTRY           *next_split_source;

#ifdef SOSY_CHECK_METADATA_TRANSPORT_POINTERS
    /** Copy of cbuffer write index before last transfer */
    unsigned                        last_cb_write_idx;
#endif
};


/* capability-specific extra operator data */
typedef struct src_sync_exop
{
    unsigned                forward_kicks;       /* Sources connected with routes */
    unsigned                sinks_connected;     /* Sink terminals connected */
    unsigned                sources_connected;   /* Source terminals connected */

    SRC_SYNC_SINK_ENTRY     *sinks[SRC_SYNC_CAP_MAX_CHANNELS];
    SRC_SYNC_SOURCE_ENTRY   *sources[SRC_SYNC_CAP_MAX_CHANNELS];
    
    SRC_SYNC_SINK_GROUP     *sink_groups;
    SRC_SYNC_SOURCE_GROUP   *source_groups;

    /** Bitmap of sink terminals covered by sink groups */
    unsigned                sink_group_mask;

    /** Bitmap of source terminals covered by source groups */
    unsigned                source_group_mask;

    /** Current CPS parameter values */
    SOURCE_SYNC_PARAMETERS  cur_params;

    CPS_PARAM_DEF           parms_def;

    /** Update internal lists at start of next data processing,
     * based on changes to connection/configuration.
     * \note Use byte access */
    bool                    bRinit : 8;

    /** Cached condition equal to: all routed sinks are unconnected.
     * Only in this state, limit the rate at which silence is produced,
     * based on the system clock and nominal rate.
     * \note Use byte access */
    bool                    all_routed_sinks_unconnected : 8;

    /**
     * True if there is only one source group and it is connected
     * via a single buffer to an audio device sink or real operator sink.
     * \note Use byte access.
     */
    bool                    single_downstream_hop : 8;

    /* CPS statistics fields */
    unsigned                Dirty_flag;
    /** Status bitmap for stalled sink terminals */
    unsigned                stat_sink_stalled;
    /** Status bitmap for stalled sink terminals (cleared by reading) */
    unsigned                stat_sink_stall_occurred;

    /** Minimum of the output buffer sizes (i.e. upper bound
     * on available space) */
    unsigned                max_space_w;

    /** System time at the last time processing started */
    TIME                    time_stamp;

    /** An estimate of the remaining amount buffered downstream
     * at the end of processing */
    RATE_SECOND_INTERVAL    est_latency;

    /** Bitmap of source terminals with pending route switch
     * (i.e. always equal to an aggregate of
     * src_ptr->switch_route.sink != NULL for all sources)
     */
    unsigned                src_route_switch_pending_mask;

    /** Bitmap of sink terminals with rate adjustment enabled
     * (from SET_SINK_GROUPS)
     */
    unsigned                sink_rm_enabled_mask;

    /** Bitmap of sink terminals with rate adjustment initialised */
    unsigned                sink_rm_initialised_mask;

    /** Sample period deviation for the primary clock
     * i.e. all sources, and non-rate-adjusted sinks.
     * Scaled and represented like sp_adjust in metadata tags,
     * i.e. Q1.N not scaled,
     * actual_sample_period = (1 + sp_adjust) nominal_sample_period
     */
    int                     primary_sp_adjust;

    /** True if receiving rate monitor measurements
     * from a downstream sink
     * \note Use byte access */
    bool                    have_primary_monitored_rate : 8;

    /** Enable unsolicited operator message when sink state changes
     * to and from stalled
     * \note Use byte access */
    bool                    enable_sink_state_notification : 8;

    /**
     * Buffer size for the next get_buffer_details.
     * If positive, it is a number of samples; if negative,
     * its magnitude is a time in fractional seconds, to be
     * multiplied with the default sample rate (not a route
     * sample rate).
     */
    int                     buffer_size;

    /** Global sample rate
     */
    unsigned                sample_rate;

    /**
     * Nominal sample period
     */
    RATE_SAMPLE_PERIOD      sample_period;

    /**
     * The system kick period in second interval
     */
    RATE_SECOND_INTERVAL    system_kick_period;

    /**
     * SS_PERIOD in samples
     */
    unsigned                ss_period_w;

    /** Minimum amount to transfer to avoid underrun */
    unsigned                min_transfer_w;

    /** Source transfer size in samples */
    unsigned                src_transfer_w;

    /** Keep track of maximum buffer space over a kick period */
    SRC_SYNC_BUFFER_LEVEL_HISTORY   source_buffer_history;

    /** Timer Task ID  */
    tTimerId                kick_id;

    /** External operator ID for traces */
    uint16                  id;

#ifdef SOSY_VERBOSE
    /** Bit mask to enable trace categories */
    uint32                  trace_enable;
#endif /* SOSY_VERBOSE */
    /** TTP-based rate measurement for reference (primary rate) */
    RATE_MEASURE_METADATA   rm_measure_primary;

    /** Cached stream_short_downstream_probe state */
    STREAM_SHORT_DOWNSTREAM_PROBE_RESULT downstream_probe;

    /** General operator data, needed to send unsolicited messages */
    OPERATOR_DATA           *op_data;

#ifdef SOSY_CHECK_METADATA_TAG_COUNTS
    /** The following counters are cleared at the
     * start of each process_data.
     * At the end: num_tags_received + num_tags_allocated
     * == num_tags_deleted + num_tags_sent
     */
    /** Counter for received tags (excluding peeked) */
    unsigned                num_tags_received;
    /** Counter for allocated tags */
    unsigned                num_tags_allocated;
    /** Counter for deleted tags (excluding peeked) */
    unsigned                num_tags_deleted;
    /** Counter for sent tags */
    unsigned                num_tags_sent;
    /** Number of cached tags */
    unsigned                num_tags_cached;
#endif /* SOSY_CHECK_METADATA_TAG_COUNTS */

    /* Fixups */
    unsigned                reserved;

} SRC_SYNC_OP_DATA;

/** To break up src_sync_compute_transfer, bundle the
 * state which needs to be kept in this small structure.
 */
typedef struct src_sync_compute_context
{
    /** The working est_latency is signed because some intermediate
     * results may be negative */
    RATE_SECOND_INTERVAL est_latency_t;

    /** Amount of data (in samples) which needs to be written to outputs
     * to avoid possibility of underrun before the next opportunity
     * to write data */
    unsigned min_transfer_w;

    /** Upper limit on transfers based on space, data available */
    unsigned max_transfer_w;

    /* Event flag */
    bool downstream_filled;

} SRC_SYNC_COMP_CONTEXT;

/****************************************************************************
Module Private Data Declarations
*/

extern const opmsg_handler_lookup_table_entry src_sync_opmsg_handler_table[];

/****************************************************************************
Module Private Inline Function Definitions
*/

/* Make casts safer by checking the input pointer type */
static inline SRC_SYNC_SINK_GROUP* cast_sink_group(SRC_SYNC_TERMINAL_GROUP* p)
{
    return (SRC_SYNC_SINK_GROUP*)p;
}
static inline SRC_SYNC_SOURCE_GROUP* cast_source_group(SRC_SYNC_TERMINAL_GROUP* p)
{
    return (SRC_SYNC_SOURCE_GROUP*)p;
}
static inline SRC_SYNC_SINK_ENTRY* cast_sink_entry(SRC_SYNC_TERMINAL_ENTRY* p)
{
    return (SRC_SYNC_SINK_ENTRY*)p;
}
static inline SRC_SYNC_SOURCE_ENTRY* cast_source_entry(SRC_SYNC_TERMINAL_ENTRY* p)
{
    return (SRC_SYNC_SOURCE_ENTRY*)p;
}
/* Wrap pointer member accesses with casts to avoid accidentally
 * cross-casting e.g. from a source entry to a sink group.
 */
static inline SRC_SYNC_SOURCE_GROUP* next_source_group(SRC_SYNC_SOURCE_GROUP* p)
{
    return cast_source_group(p->common.next);
}
static inline SRC_SYNC_SINK_GROUP* next_sink_group(SRC_SYNC_SINK_GROUP* p)
{
    return cast_sink_group(p->common.next);
}
static inline SRC_SYNC_SOURCE_ENTRY* next_source_entry(SRC_SYNC_SOURCE_ENTRY* p)
{
    return cast_source_entry(p->common.next);
}
static inline SRC_SYNC_SINK_ENTRY* next_sink_entry(SRC_SYNC_SINK_ENTRY* p)
{
    return cast_sink_entry(p->common.next);
}
static inline SRC_SYNC_SOURCE_GROUP* source_group_from_entry(SRC_SYNC_SOURCE_ENTRY* p)
{
    return cast_source_group(p->common.group);
}
static inline SRC_SYNC_SOURCE_ENTRY* source_entries_from_group(SRC_SYNC_SOURCE_GROUP* p)
{
    return cast_source_entry(p->common.terminals);
}
static inline SRC_SYNC_SINK_GROUP* sink_group_from_entry(SRC_SYNC_SINK_ENTRY* p)
{
    return cast_sink_group(p->common.group);
}
static inline SRC_SYNC_SINK_ENTRY* sink_entries_from_group(SRC_SYNC_SINK_GROUP* p)
{
    return cast_sink_entry(p->common.terminals);
}

/* Time unit conversions */
/**
 * \param usec Non-negative time in microseconds,
 *             expected (though not checked) to be <1e6
 * \return Time in seconds represented as Q1.N
 */
static inline unsigned src_sync_usec_to_sec_frac(unsigned usec)
{
    /* usec << (DAWTH-1-20) "normalizes" usec <= 1e6 */
    return 2 * frac_mult(usec << (DAWTH-1-20), FRACTIONAL(0.5 / (1.0e6 / (1<<20))));
}


/****************************************************************************
Module Private Function Declarations
*/

extern void src_sync_transfer_route(SRC_SYNC_SOURCE_ENTRY* src_ptr,
    tCbuffer* sink_buffer, unsigned transfer);

extern SRC_SYNC_TERMINAL_GROUP* src_sync_find_group(
        SRC_SYNC_TERMINAL_GROUP* groups, unsigned channel_num);

extern SRC_SYNC_SINK_ENTRY *src_sync_alloc_sink(
        SRC_SYNC_OP_DATA *op_extra_data,unsigned term_idx);
extern SRC_SYNC_SOURCE_ENTRY *src_sync_alloc_source(
        SRC_SYNC_OP_DATA *op_extra_data,unsigned term_idx);
extern bool src_sync_alloc_buffer_histories(
        OPERATOR_DATA *op_data, SRC_SYNC_SINK_GROUP* sink_groups,
        SRC_SYNC_SOURCE_GROUP* source_groups);

extern void src_sync_set_sink_state(
        SRC_SYNC_OP_DATA *op_extra_data, SRC_SYNC_SINK_GROUP* sink_grp,
        src_sync_sink_state new_state );

/* source_sync_utils.c */
extern bool src_sync_alloc_buffer_history(
        SRC_SYNC_BUFFER_LEVEL_HISTORY* hist, unsigned num_entries);
extern void src_sync_free_buffer_history(
        SRC_SYNC_BUFFER_LEVEL_HISTORY* hist);
extern void src_sync_put_buffer_history(
        SRC_SYNC_BUFFER_LEVEL_HISTORY* hist, TIME t, unsigned amount);
extern unsigned src_sync_get_buffer_history_min(
        SRC_SYNC_BUFFER_LEVEL_HISTORY* hist, TIME now, TIME max_age);
extern unsigned src_sync_get_buffer_history_max(
        SRC_SYNC_BUFFER_LEVEL_HISTORY* hist, TIME now, TIME max_age);
extern char src_sync_sign_and_magnitude(int value, unsigned* magnitude);
extern SRC_SYNC_TERMINAL_GROUP* src_sync_alloc_groups(unsigned num_groups, unsigned struct_size);
extern RATE_SECOND_INTERVAL src_sync_get_period(const SRC_SYNC_OP_DATA *op_extra_data);
extern RATE_SECOND_INTERVAL src_sync_get_max_period(const SRC_SYNC_OP_DATA *op_extra_data);
extern RATE_SECOND_INTERVAL src_sync_get_max_latency(const SRC_SYNC_OP_DATA *op_extra_data);
extern RATE_SECOND_INTERVAL src_sync_get_stall_recovery_default_fill(const SRC_SYNC_OP_DATA *op_extra_data);

/* source_sync_opmsg.c */
#ifdef SOSY_VERBOSE
extern void src_sync_trace_params(const SRC_SYNC_OP_DATA* op_extra_data);
#else
static inline void src_sync_trace_params(const SRC_SYNC_OP_DATA* op_extra_data)
{
}
#endif

/* source_sync_metadata.c */
extern unsigned src_sync_metadata_count_tags(const metadata_tag* tags);
extern metadata_tag* src_sync_metadata_find_last_tag(metadata_tag* tag);
extern bool src_sync_metadata_move_eof(
        metadata_tag *dest_tag, metadata_tag *src_tag);
extern unsigned src_sync_get_next_timestamp(
        TIME timestamp, unsigned nr_of_samples,
        RATE_SAMPLE_PERIOD sample_period, int sp_adjust);
extern unsigned src_sync_get_prev_timestamp(
        const metadata_tag* tag, unsigned nr_of_samples,
        RATE_SAMPLE_PERIOD sample_period );
#ifdef SOSY_CHECK_METADATA_TRANSPORT_POINTERS
extern void src_sync_check_md_transport_pre(SRC_SYNC_OP_DATA* op_extra_data);
extern void src_sync_check_md_transport_post(SRC_SYNC_OP_DATA* op_extra_data);
#endif /* SOSY_CHECK_METADATA_TRANSPORT_POINTERS */
#ifdef SOSY_CHECK_METADATA_TAG_COUNTS
extern void src_sync_clear_tag_counts(SRC_SYNC_OP_DATA* op_extra_data);
extern void src_sync_check_tag_counts(SRC_SYNC_OP_DATA* op_extra_data);
#endif /* SOSY_CHECK_METADATA_TAG_COUNTS */


/* source_sync_metadata_rcv.c */
extern bool src_sync_get_input_metadata(
        SRC_SYNC_OP_DATA* op_extra_data, SRC_SYNC_SINK_GROUP* sink_grp,
        SRC_SYNC_SINK_METADATA* md, tCbuffer* input_metadata_buffer,
        unsigned* remove_octets );
extern unsigned src_sync_get_sink_metadata(
        SRC_SYNC_OP_DATA* op_extra_data, SRC_SYNC_SINK_GROUP* sink_grp,
        unsigned words );

extern src_sync_stall_recovery_type src_sync_peek_resume(
        SRC_SYNC_OP_DATA* op_extra_data, metadata_tag* tag,
        SRC_SYNC_SINK_METADATA* ts_state, unsigned rcv_beforeidx_octets,
        unsigned available_samples,
        unsigned* p_gap_samples, unsigned sink_grp_idx );

/* source_sync_metadata_send.c */
extern void src_sync_metadata_drop_tags(
        SRC_SYNC_OP_DATA* op_extra_data,
        metadata_tag* tags,
        SRC_SYNC_METADATA_DEST* dest_state );
extern void src_sync_metadata_silence(
        SRC_SYNC_OP_DATA* op_extra_data, SRC_SYNC_TERMINAL_GROUP* dest_common,
        SRC_SYNC_METADATA_DEST* dest_state, unsigned silence_octets );
extern void src_sync_metadata_forward(
        SRC_SYNC_OP_DATA* op_extra_data, SRC_SYNC_SINK_METADATA* sink,
        unsigned octets_length, SRC_SYNC_TERMINAL_GROUP* dest_common,
        SRC_SYNC_METADATA_DEST* dest_state );


/* source_sync.c */
/* Message handlers */
extern bool src_sync_create(OPERATOR_DATA *op_data, void *message_data,
                            unsigned *response_id, void **response_data);
extern bool src_sync_destroy(OPERATOR_DATA *op_data, void *message_data,
                             unsigned *response_id, void **response_data);
extern bool src_sync_start(OPERATOR_DATA *op_data, void *message_data,
                           unsigned *response_id, void **response_data);
extern bool src_sync_reset(OPERATOR_DATA *op_data, void *message_data,
                           unsigned *response_id, void **response_data);
extern bool src_sync_stop(OPERATOR_DATA *op_data, void *message_data,
                          unsigned *response_id, void **response_data);
extern bool src_sync_connect(OPERATOR_DATA *op_data, void *message_data,
                             unsigned *response_id, void **response_data);
extern bool src_sync_disconnect(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data);
extern bool src_sync_buffer_details(
        OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data);
extern void src_sync_cleanup(SRC_SYNC_OP_DATA *op_extra_data);
extern bool src_sync_stop_reset(OPERATOR_DATA *op_data,void **response_data);

/* Synchronisation */
#define src_sync_suspend_processing(op_data) opmgr_op_suspend_processing(op_data)
extern void src_sync_resume_processing(OPERATOR_DATA* op_data);

/* Data processing function */
extern void src_sync_update_derived(SRC_SYNC_OP_DATA *op_extra_data);
extern void src_sync_set_sink_recovering_restarting(
        SRC_SYNC_OP_DATA *op_extra_data, SRC_SYNC_SINK_GROUP *sink_grp);
extern void src_sync_refresh_sink_list(SRC_SYNC_OP_DATA *op_extra_data);
extern void src_sync_refresh_source_list(SRC_SYNC_OP_DATA *op_extra_data);
extern void src_sync_refresh_connections(SRC_SYNC_OP_DATA* op_extra_data);
extern void src_sync_refresh_forward_routes(SRC_SYNC_OP_DATA* op_extra_data);
extern void src_sync_refresh_metadata_routes(SRC_SYNC_OP_DATA* op_extra_data);
extern void src_sync_refresh_downstream_probe(SRC_SYNC_OP_DATA* op_extra_data);
extern bool src_sync_perform_transitions(SRC_SYNC_OP_DATA* op_extra_data);
extern void src_sync_pre_check_back_kick(SRC_SYNC_OP_DATA* op_extra_data);
extern unsigned src_sync_post_check_back_kick(SRC_SYNC_OP_DATA* op_extra_data);
extern unsigned src_sync_calc_sink_group_available_data(SRC_SYNC_SINK_GROUP* sink_grp);
extern unsigned src_sync_any_group_space(
#ifdef SOSY_VERBOSE
        SRC_SYNC_OP_DATA* op_extra_data,
#endif
        SRC_SYNC_TERMINAL_GROUP* any_grp);
extern unsigned src_sync_compute_space(SRC_SYNC_OP_DATA* op_extra_data);
extern RATE_SECOND_INTERVAL src_sync_compute_transfer_space(
        SRC_SYNC_OP_DATA* op_extra_data, SRC_SYNC_COMP_CONTEXT* comp);
extern RATE_SECOND_INTERVAL src_sync_compute_transfer_sinks(
        SRC_SYNC_OP_DATA* op_extra_data, SRC_SYNC_COMP_CONTEXT* comp);
extern unsigned src_sync_perform_transfer(SRC_SYNC_OP_DATA *op_extra_data);
extern void src_sync_timer_task(void *kick_object);
extern void src_sync_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);
extern void src_sync_notify_sink_state_change(
        SRC_SYNC_OP_DATA* op_extra_data, SRC_SYNC_SINK_GROUP* sink_grp,
        src_sync_sink_state new_state);


extern void src_sync_free_buffer_histories(SRC_SYNC_OP_DATA *op_extra_data);

extern SRC_SYNC_SINK_GROUP* src_sync_find_sink_group(
        SRC_SYNC_OP_DATA *op_extra_data, unsigned channel_num);
extern SRC_SYNC_SOURCE_GROUP* src_sync_find_source_group(
        SRC_SYNC_OP_DATA *op_extra_data, unsigned channel_num);

extern bool src_sync_valid_route(const SRC_SYNC_ROUTE_ENTRY* route);

extern bool src_sync_connect_metadata_buffer( SRC_SYNC_TERMINAL_ENTRY* entry,
                                              SRC_SYNC_TERMINAL_GROUP* group);
extern void src_sync_find_alternate_metadata_buffer(
        unsigned connected,
        SRC_SYNC_TERMINAL_ENTRY* entry,
        SRC_SYNC_TERMINAL_ENTRY** all_entries);

extern unsigned src_sync_route_write_silence( SRC_SYNC_OP_DATA *op_extra_data,
                                              SRC_SYNC_SINK_GROUP *sink_grp,
                                              unsigned words );
extern unsigned src_sync_route_copy( SRC_SYNC_OP_DATA *op_extra_data,
                                     SRC_SYNC_SINK_GROUP *sink_grp,
                                     unsigned words );
extern unsigned src_sync_route_discard_input( SRC_SYNC_OP_DATA *op_extra_data,
                                              SRC_SYNC_SINK_GROUP *sink_grp,
                                              unsigned words );

/* Op msg handlers */
extern bool src_sync_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_set_ucid(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_get_ps_id(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_set_buffer_size(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_set_terminal_buffer_size(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_set_back_kick_threshold(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_set_sample_rate(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_set_stall_notification_enable(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool src_sync_opmsg_ep_get_config(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_ep_configure(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_opmsg_ep_clock_id(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool src_sync_set_route(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_get_route(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_set_sink_groups(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_set_source_groups(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool src_sync_set_trace_enable(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool src_sync_ups_params(void* instance_data,PS_KEY_TYPE key,PERSISTENCE_RANK rank,
                 uint16 length, unsigned* data, STATUS_KYMERA status,uint16 extra_status_info);

/* Helpers for set_sink_groups, set_source_groups, set_route */
extern bool src_sync_disjoint_union_group_masks(
        const unsigned* data_ptr, unsigned num_groups,
        unsigned* p_union_mask );
extern bool src_sync_parse_sink_flags(
        SRC_SYNC_TERMINAL_GROUP* grp_ptr, unsigned x_msw);
extern bool src_sync_parse_source_flags(
        SRC_SYNC_TERMINAL_GROUP* grp_ptr, unsigned x_msw);
extern bool src_sync_populate_groups(
        SRC_SYNC_TERMINAL_GROUP* groups, const unsigned* data_ptr,
        unsigned num_groups,
        bool (*parse_flags)(SRC_SYNC_TERMINAL_GROUP*,unsigned) );
extern unsigned src_sync_get_num_groups(const SRC_SYNC_TERMINAL_GROUP*);
extern void src_sync_clear_all_routes(SRC_SYNC_OP_DATA *op_extra_data);

/* From source_sync_rate_adjust.c */
extern void src_sync_ra_set_rate(
        SRC_SYNC_OP_DATA *op_extra_data, SRC_SYNC_SINK_GROUP* sink_grp,
        unsigned rate);
extern void src_sync_ra_set_primary_rate(
        SRC_SYNC_OP_DATA *op_extra_data, unsigned rate);
extern void src_sync_ra_update_delay(
        SRC_SYNC_OP_DATA *op_extra_data, SRC_SYNC_SINK_GROUP* sink_grp);
extern bool src_sync_is_sink_rm_enabled(
        const SRC_SYNC_OP_DATA *op_extra_data, unsigned channel_num);
extern void src_sync_check_primary_clock_connected(
        SRC_SYNC_OP_DATA *op_extra_data);
extern bool src_sync_rm_enact(
        OPERATOR_DATA *op_data, bool is_sink, unsigned terminal_num,
        bool enable);
extern bool src_sync_rm_adjust(
        SRC_SYNC_OP_DATA *op_extra_data, bool is_sink, unsigned terminal_num,
        uint32 value);
extern tCbuffer* src_sync_get_input_buffer(
        SRC_SYNC_SINK_GROUP* sink_grp, SRC_SYNC_SINK_ENTRY* sink_ptr);
extern tCbuffer* src_sync_get_input_metadata_buffer(
        SRC_SYNC_SINK_GROUP* sink_grp);
extern bool src_sync_rm_init(
        SRC_SYNC_OP_DATA *op_extra_data, SRC_SYNC_SINK_GROUP* sink_grp);
extern bool src_sync_rm_fini(
        SRC_SYNC_OP_DATA *op_extra_data, SRC_SYNC_SINK_GROUP* sink_grp);
extern void src_sync_rm_process( SRC_SYNC_OP_DATA *op_extra_data, unsigned *back_kick);

/****************************************************************************
Module Private Data Declarations
*/
#if defined(SOSY_VERBOSE) && defined(SOSY_NUMBERED_LOG_MESSAGES)
extern unsigned src_sync_trace_serial;
#ifdef SOSY_LOG_MESSAGE_LIMIT
extern unsigned src_sync_trace_limit;
#endif
#endif /* DEBUG_NUMBERED_LOG_MESSAGES */

#endif /* SOURCE_SYNC_DEFS_H */



