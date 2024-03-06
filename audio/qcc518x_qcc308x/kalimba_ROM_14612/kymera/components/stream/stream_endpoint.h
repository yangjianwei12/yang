/****************************************************************************
 * Copyright (c) 2011 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup endpoints Endpoints
 * \ingroup stream
 */
/**
 * \file stream_endpoint.h
 * \ingroup endpoints
 */

#ifndef _STREAM_ENDPOINT_H_
#define _STREAM_ENDPOINT_H_

#include "stream_common.h"

/****************************************************************************
Include Files
*/
#if defined(INSTALL_HYDRA)
#include "audio.h"
#endif /* INSTALL_HYDRA */
#include "stream/stream_for_opmgr.h"
#include "stream/stream.h"
#include "stream_kick_obj.h"
#include "sched_oxygen/sched_oxygen.h"
#ifdef INSTALL_SCO
#include "sco_data_mgr.h"
#endif /* INSTALL_SCO */
#include "cbops_mgr/cbops_mgr.h"
#include "util.h"

#include "adaptor/connection_id.h"
#include "pl_timers/pl_timers.h"

#include "ttp/timed_playback.h"
#include "rate/rate.h"
#include "stream_type_alias.h"
#include "stream/stream_for_override.h"

#include "sco_drv/sco_src_drv.h"
#include "sco_drv/sco_sink_drv.h"

#ifdef INSTALL_USB_AUDIO
#include "ttp/ttp_pid.h"
#endif

#ifdef INSTALL_SCOISO_EP_PROFILING
#include "profiler_c.h"
#endif

/****************************************************************************
Private Constant Declarations
*/

/** Number of consecutive underruns that make us decide it is a gap / pause in the data
 *  stream.
 */
#define MAX_CONSECUTIVE_UNDERRUNS       3
#define MAX_CONSECUTIVE_UNDERRUNS_BITS  2

/** Number of consecutive updates that yield average perceived data block at sink input */
#define NR_DATA_BLOCK_UPDATES           8
#define NR_DATA_BLOCK_UPDATES_BITS      4

/** Bit mask to identify internal endpoint configure keys
 * (EP_DATA_FORMAT etc.)
 */
#define ENDPOINT_INT_CONFIGURE_KEYS_MASK    (0x10000)

/****************************************************************************
Private Type Declarations
*/
typedef unsigned STREAM_INSTANCE;
typedef unsigned STREAM_CHANNEL;

/**
 * Internal endpoint configure keys
 */
/* These ids have to be different from other keys used in different
 * endpoints, to avoid problems set the 17th bit
 * (all other keys are 16 bits). */
typedef enum ENDPOINT_INT_CONFIGURE_KEYS
{
    /** Get the data format the endpoint is currently configured for */
    EP_DATA_FORMAT = 0x010000,

    /** Endpoint kick period */
    EP_KICK_PERIOD,

    /** Endpoint block size */
    EP_BLOCK_SIZE,

     /** Processing time of upstream operator */
    EP_PROC_TIME,

    /** Flag to tell an operator endpoint that it's kicked by a hard deadline
     * and hence shouldn't run when kicked from upstream
     */
    EP_KICKED_FROM_ENDPOINT,

    /** Configure the override flag for the endpoint and the size of buffer
     * which will be supplied by the overridden endpoint.  Override will be
     * automatically disabled during disconnect. Because of this reason,
     * "clear" key is not necessary  for this feature.
     * Note: Under Bluecore the size  is ignored.*/
    EP_OVERRIDE_ENDPOINT,

    /** This key is used to get and configure the cbops parameters
      * of an endpoint.
      */
    EP_CBOPS_PARAMETERS,

    /** Request the type of ratematching support the endpoint possesses. */
    EP_RATEMATCH_ABILITY,

    /** Ratematching enactment setting enable(d)/disable(d) */
    EP_RATEMATCH_ENACTING,

    /** Request the measured rate used for ratematching */
    EP_RATEMATCH_RATE,

    /** Apply a rate adjustment value */
    EP_RATEMATCH_ADJUSTMENT,

    /** Used by the operator endpoint only.*/
    EP_DETAILS,

    /* spdif endpoint sample can change. */
    EP_SAMPLE_RATE,

    /* spdif can have different channel types */
    EP_CHANNEL_ORDER,


    /** Set input hardware gain in platform-independent units
     * (1/60dB steps as a signed number in 32-bit 2's complement,
     * relative to some possibly platform-dependent level). */
    EP_SET_INPUT_GAIN,

    /** Set output hardware gain in platform-independent units
     * (1/60dB steps as a signed number in 32-bit 2's complement,
     * relative to some possibly platform-dependent level). */
    EP_SET_OUTPUT_GAIN,

    /* Set the KIP shadow endpoint data channel */
    EP_SET_DATA_CHANNEL,

    /* Block KIP endpoint to supply remote buffer */
    EP_CLONE_REMOTE_BUFFER,

    /* Shadow endpoint buffer size */
    EP_SET_SHADOW_BUFFER_SIZE,

    /* EP_SET_SHADOW_TINFO_BLOCKSIZE and EP_SET_SHADOW_TINFO_PERIOD removed */

    /* Shadow endpoint metadata flag. */
    EP_METADATA_SUPPORT = 0x010015,

    /* Shadow endpoint metadata data channel ID */
    EP_METADATA_CHANNEL_ID,

    /** Ratematch non-enacting EP measurement */
    EP_RATEMATCH_MEASUREMENT,

    /** Ratematch enacting reference */
    EP_RATEMATCH_REFERENCE,

    /* standalone rate adjust operator available
     * for performing rate adjustment.
     */
    EP_RATE_ADJUST_OP,

    /* depending on how hw warp values are calculated it could be
     * accumulative (diff is supplied) or direct apply
     */
    EP_HW_WARP_APPLY_MODE,

    /* current actual HW warp value */
    EP_CURRENT_HW_WARP,

    /* number of words left after processing above threshold for
     * silence insertion
     */
    EP_LATENCY_AMOUNT,

    /* Shadow endpoint buffer */
    EP_SET_SHADOW_STATE_BUFFER,

} ENDPOINT_INT_CONFIGURE_KEYS;

/**
 * Structure for extended EP_RATEMATCH_REFERENCE parameters
 */
typedef struct
{
    /** Approximate (averaged) sample period deviation */
    int                 sp_deviation;

    /** Reference rate in rate library format */
    RATE_RELATIVE_RATE  ref;

    /** Handle to get information about the non-enacting endpoint */
    unsigned int        ref_endpoint_id;

} ENDPOINT_RATEMATCH_REFERENCE_PARAMS;

/**
 * Structure for extended EP_RATEMATCH_MEASUREMENT results
 */
typedef struct
{
    /** Approximate (averaged) sample period deviation */
    int                 sp_deviation;

    /** Rate measurement in rate library format */
    RATE_RELATIVE_RATE  measurement;

} ENDPOINT_RATEMATCH_MEASUREMENT_RESULT;

/**
 * Structure to contain get_config results
 */
struct ENDPOINT_GET_CONFIG_RESULT
{
    union {
        /** The default case */
        uint32                          value;

        /** EP_RATEMATCH_MEASUREMENT */
        ENDPOINT_RATEMATCH_MEASUREMENT_RESULT  rm_meas;
    } u;
};

/**
 * Timing information structure
 * Used to retrieve timing information from an endpoint
 * or operator terminal.
 */
typedef struct
{
    /** The size of a block/frame that the endpoint / terminal
        consumes/produces. Currently only used by operator terminals. */
    unsigned int block_size;
    /** Flag indicating that the endpoint might have independent lifetime,
        might disappear and the operators connected must cope with it. */
    bool is_volatile:8;
    /** Flag indicating whether the endpoint is clocked locally or remotely.
     * This is only valid for real-endpoints. */
    bool locally_clocked:8;
    /** Flag indicating whether the endpoint wants to receive kicks from the thing
     * it is connected to or not.
     */
    bool wants_kicks:8;
} ENDPOINT_TIMING_INFORMATION;


/**
 * Enumeration of endpoint types. Used to decide how endpoints are connected.
 */
typedef enum
{
    /** Endpoint is an audio interface */
    endpoint_audio,
    /** Endpoint is a sco link */
    endpoint_sco,
    /** Endpoint is a iso link */
    endpoint_iso,
    /** Endpoint is an operator */
    endpoint_operator,
    /** Endpoint is a shunt interface */
    endpoint_shunt,
    /** Endpoint is a A2DP interface */
    endpoint_a2dp,
    /** Endpoint is a File interface */
    endpoint_file,
    /** Endpoint is a raw buffer interface */
    endpoint_raw_buffer,
    /** Endpoint is a tester interface */
    endpoint_tester,
    /* Endpoint is a spdif interface */
    endpoint_spdif,
#if defined(SUPPORTS_MULTI_CORE)
    /* Shadow endpoint */
    endpoint_shadow,
#endif
    /* Endpoint is timestamped data  */
    endpoint_timestamped,
    /* Endpoint is a usb audio interface */
    endpoint_usb_audio

} ENDPOINT_TYPE;

/**
 * Structure of common endpoint function pointers.
 */
typedef struct ENDPOINT_FUNCTIONS
{
    /**
     * Close the endpoint
     */
    bool (*close)(ENDPOINT *);

    /**
     * Connect to the endpoint
     */
    bool (*connect)(ENDPOINT *, tCbuffer *, ENDPOINT *, bool *);

    /**
     * Disconnect from the endpoint
     */
    bool (*disconnect)(ENDPOINT *);

    /**
     * Retrieve the buffer details from the endpoint
     */
    bool (*buffer_details)(ENDPOINT *, BUFFER_DETAILS *);

    /**
     * Make the endpoint produce or consume some data
     */
    void (*kick)(ENDPOINT *, ENDPOINT_KICK_DIRECTION);

    /**
     * When the endpoint is responsible for scheduling chain kicks this function
     * is called to perform any real-time scheduling that needs to occur per kick
     */
    void (*sched_kick)(ENDPOINT *, KICK_OBJECT *);

    /**
     * When the endpoint is responsible for scheduling chain kicks this function
     * initiates a kick interrupt source to start producing kicks.
     */
    bool (*start)(ENDPOINT *, KICK_OBJECT *);

    /**
     * When the endpoint is responsible for scheduling chain kicks this function
     * cancels the associated kick interrupt source.
     */
    bool (*stop)(ENDPOINT *);

    /**
     * Configure the endpoint
     */
    bool (*configure)(ENDPOINT *, unsigned, uint32);

    /**
     * Get endpoint configuration
     */
    bool (*get_config)(ENDPOINT *, unsigned, ENDPOINT_GET_CONFIG_RESULT *);

    /**
     * Obtain the timing information from the endpoint
     */
    void (*get_timing_info)(ENDPOINT *, ENDPOINT_TIMING_INFORMATION *);

    /**
     * Synchronise two endpoints
     */
    bool (*sync)(ENDPOINT *, ENDPOINT *);

    /**
     * Report whether two endpoints have the same clock source
     */
    bool (*have_same_clock)(ENDPOINT *, ENDPOINT *, bool);

} ENDPOINT_FUNCTIONS;

/**
 * Information for real endpoint latency control. This only exists for real
 * endpoints, operator endpoints are assumed not needing this now nor in future.
 */
typedef struct ENDPOINT_LATENCY_CTRL_INFO
{
    /** Number of inserted sink silence samples that are not yet compensated for */
    unsigned silence_samples;

    /** Perceived input data block */
    unsigned data_block;

} ENDPOINT_LATENCY_CTRL_INFO;

/** The data specifically required for audio endpoints. The shape of this varies
 * dependent upon the platform one builds for. */

/**
 * Only allow MAX_CONSECUTIVE_UNDERRUNS to reach 2^MAX_CONSECUTIVE_UNDERRUNS_BITS - 1
 * This should prevent arithmetic overflow in underrun_sequence provided it is checked
 * after every increment.
 *
 */
STATIC_ASSERT( ( MAX_CONSECUTIVE_UNDERRUNS < (1 << MAX_CONSECUTIVE_UNDERRUNS_BITS)),
                not_enough_bits_for_underrun_sequence);

/**
 * Only allow NR_DATA_BLOCK_UPDATES to reach 2^NR_DATA_BLOCK_UPDATES_BITS - 1
 * This should prevent arithmetic overflow in data_block_updates provided it is checked
 * after every increment.
 *
 */
STATIC_ASSERT(  (NR_DATA_BLOCK_UPDATES < (1 << NR_DATA_BLOCK_UPDATES_BITS)),
                not_enough_bits_for_data_block_updates);

/**
 * Information needed to manage a deferred kick handler
 */
typedef struct endpoint_deferred_kick
{
    /**
     * Enable deferred kick handling. If false, the
     * endpoint kick function is called directly in the
     * interrupt context.
     * \note Packing :8 causes byte instructions
     * to be emitted i.e. is both space and time efficient.
     */
    bool kick_is_deferred : 8;

    /**
     * Configure deferred kick handling
     */
    bool config_deferred_kick : 8;

    /**
     * Direction of deferred kick
     */
    ENDPOINT_KICK_DIRECTION kick_dir : 8;

    /**
     * Task ID of a task containing just a bg_int handler
     */
    taskid bg_task;

    /**
     * Time taken by the first interrupt handler
     */
    TIME interrupt_handled_time;
} endpoint_deferred_kick;

#ifdef INSTALL_UNINTERRUPTABLE_ANC
/** Structure used to hold ANC configuration data */
typedef struct endpoint_anc_state
{
    /* Primary instance of the ANC H/W associated with the Endpoint */
    STREAM_ANC_INSTANCE instance_id;

    /* Secondary instance of the ANC H/W associated with the Endpoint. 
     * This field is used when two ANC instances associated with the same endpoint */
    STREAM_ANC_INSTANCE secondary_instance_id;

    /* Input path associated with the endpoint */
    STREAM_ANC_PATH input_path_id;

    /* Secondary Input path associated with the endpoint.
     * This field is used when two ANC paths are associated with the same endpoint */
    STREAM_ANC_PATH secondary_input_path_id;

#ifdef INSTALL_ANC_STICKY_ENDPOINTS
    /* Flag to indicate that an attempt to close the endpoint has been made while
     * it is in-use by ANC
     */
    bool close_pending;
#endif /* INSTALL_ANC_STICKY_ENDPOINTS */

}endpoint_anc_state;
#endif /* INSTALL_UNINTERRUPTABLE_ANC */

typedef struct endpoint_audio_state
{
    /**
     * The running state of the interrupt source.
     */
    bool running;

    /**
     * Next audio end point that is synchronised with this one. Set to NULL
     * if this end point is not synchronised with any other end point
     */
    struct ENDPOINT *nep_in_sync;

    /**
     * Master end point that all other endpoint in the synchronised chain follow
     * If this end point is not synchronised with any other end point, this end
     * point is the master.
     */
    struct ENDPOINT *head_of_sync;

    /** This is a signed value representing the difference between the rate data
     * is being produced and consumed. This value is normalised.
     */
    unsigned int rm_adjust_amount;

    /** Amount of compensation to perform in the sink endpoint cbops copy to
     * maintain the output buffer level on account of any rate mismatch. */
    int rm_diff;

    /** The cbuffer that audio data arrives from. If the endpoint is a source
     * this represents the port. */
    tCbuffer *source_buf;

    /** The cbuffer that audio data leaves through. If the endpoint is a sink
     * this represents the port. */
    tCbuffer *sink_buf;

    /** Internal kick timer ID used to run an operator kick at the same priority level
     *  as a hardware interrupt
     */
    tTimerId internal_kick_timer_id;

    /**
     * The running state of the interrupt source.
     */
    bool is_overridden:8;

#if defined(CHIP_BASE_HYDRA)
    /**
     * Is the hardware allocated?
     */
    bool hw_allocated:8;

    /**
     * Is the monitor running?
     */
    bool monitor_enabled:8;

#if defined(AOV_EOD_ENABLED)
    /**
     * Is the AoV Empty on Demand running?
     */
    bool aov_eod_enabled:8;
#endif /* AOV_EOD_ENABLED */

    /**
     * Bit shift to configure on buffer
     */
    unsigned int shift:5;

    /**
     * How often should the audio endpoint notify us of new data?
     * (measured in samples)
     */
    int monitor_threshold;

    /** The last time the hardware produced an interrupt. Used for calculating the
     * actual data rate when the hardware isn't locally clocked.
     */
    RATE_TIME rm_period_start_time;

    /**
     * The time between kicks integrated over recent history to allow for
     * expected run time jitter.
     */
    RATE_TIME rm_int_time;

    /**
     * Expected time between kicks, scaled same as rm_int_time.
     */
    RATE_TIME rm_expected_time;

    /**
     * Rate matching support
     */
    unsigned rm_support:8; /* Encoded as RATEMATCHING_SUPPORT. */

    /** Allow endpoint to advertise SW rate adjustment ability */
    bool rm_enable_sw_rate_adjust:8;

    /** Allow endpoint to advertise HW rate adjustment ability */
    bool rm_enable_hw_rate_adjust:8;

    /** Allow endpoint to provide CLRM rate measurements */
    bool rm_enable_clrm_measure:8;

    /**
     * Current rate adjustment value
     */
    int rm_adjust_prev;

    /**
     * Current sp_deviation of HW rate warp capable device (fractional)
     */
    int rm_hw_sp_deviation;

    /**
     * Current reported sp_deviation (fractional)
     */
    int rm_report_sp_deviation;

    /**
     *  The sample rate of the audio hardware, used among others
     *  to compute ToA efficiently.
     */
    unsigned sample_rate;

    /** Sample count/time accumulator */
    RATE_MEASURE rm_measure;

#if defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM)
    /**
     * HAL warp update descriptor
     */
    WARP_UPDATE_DESC rm_update_desc;
#endif /* defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM) */
#endif /* CHIP_BASE_HYDRA */

#ifdef INSTALL_MCLK_SUPPORT
    /* showing whether MCLK is ready to use by this endpoint */
    bool mclk_claimed;
#endif

#ifdef INSTALL_UNINTERRUPTABLE_ANC
    struct endpoint_anc_state anc;
#endif /* INSTALL_UNINTERRUPTABLE_ANC */

    /** Delta samples - internal, calculated by RM mechanism. Some mechanisms
     *  may not make use of this... so common future proofing.
     */
    unsigned delta_samples;

    /* Channel number when synchronised in a group */
    unsigned channel;

    /** Data started flag (across all channels) */
    bool sync_started;

    /* depending on how hw warp values are calculated it could be
     * accumulative (diff is supplied) or direct apply. default is
     * accumulative.
     */
    bool direct_hw_warp_apply:8;

    /**
     * Bitfield used to detect the start of the data flow.
     */
    bool data_flow_started:8;

    /**
     * True, if the timed playback takes care of playing the audio.
     */
    bool use_timed_playback:8;

#ifdef SUPPORTS_CONTINUOUS_BUFFERING
    /**
     * Flag to indicate that previous_toa should be initialised.
     */
    bool initialise_toa:8;

    /**
     * Last ToA recorded
     */
    TIME previous_toa;
#endif /* SUPPORTS_CONTINUOUS_BUFFERING */
    /**
     * Delay added by the endpoint (microseconds).
     */
    unsigned endpoint_delay_us;

    /**
     * Timed playback module.
     */
    TIMED_PLAYBACK* timed_playback;

#ifdef INSTALL_DELEGATE_RATE_ADJUST_SUPPORT
    /* standalone rate adjust operator that
     * is available to this endpoint
     */
    unsigned external_rate_adjust_opid;
    /* Previous adjustment value
     * Cached so we only update if it changes
     */
    int external_last_rate;
#endif /* INSTALL_DELEGATE_RATE_ADJUST_SUPPORT */
    /**
     * Flag to enable metadata generation by source endpoint
     * This is configured by user.
     */
    bool generate_metadata;

    /**
     * minimum tag length in words, this is to make
     * sure endpoint doesn't create a lot of small-length
     * tags
     */
    unsigned min_tag_len;

    /** number of words left to complete last written tags */
    unsigned last_tag_left_words;

    /**
     * Information for latency control purposes.
     * Should be the last member of endpoint_audio_state
     * so as to allow variable size endpoint_audio_state.
     * ie: latency_ctrl_info is allocated only for audio sinks
     */
    struct ENDPOINT_LATENCY_CTRL_INFO latency_ctrl_info;
    /*
     * Don't add anything here - see above !
     */
}endpoint_audio_state;

#ifdef INSTALL_SPDIF

/* number of channel status words, each word is
 * 16 bit regardless of architecture
 */
#define SPDIF_NOROF_CHSTS_WORDS 12

/* S/PDIF channel order */
typedef enum
{

/* The channel order numbering are the same for all
 * chip types, however for hydra we get that directly
 * from ACCMD-based definitions in hal.
 */
#ifdef CHIP_BASE_HYDRA
    /* A channel (LEFT) */
    SPCO_CHANNEL_A = SPDIF_CHANNEL_A,

    /* B channel (RIGHT) */
    SPCO_CHANNEL_B = SPDIF_CHANNEL_B,

    /* interleaved channel (LEFT+RIGHT) */
    SPCO_CHANNEL_AB = SPDIF_CHANNEL_A_B_INTERLEAVED

#else
    /* A channel (LEFT) */
    SPCO_CHANNEL_A = 0,

    /* B channel (RIGHT) */
    SPCO_CHANNEL_B = 1,

    /* interleaved channel (LEFT+RIGHT) */
    SPCO_CHANNEL_AB = 3

#endif
} SPDIF_CHANNEL_ORDER;

/** The data specifically required for spdif endpoints. The shape of this varies
 * dependent upon the platform one builds for. */
typedef struct endpoint_spdif_state
{
    /**
     * The running state of the interrupt source.
     */
    bool running;

    /** The cbuffer that audio data arrives from. If the endpoint is a source
     * this represents the port. */
    tCbuffer *source_buf;

    /** The cbuffer that audio data leaves through. If the endpoint is a sink
     * this represents the port. */
    tCbuffer *sink_buf;

    /**
     * The timer id of the timer generating the next kick. It may be
     * possible to union this with the monitor_enabled field.
     */
    tTimerId kick_id;

    /** Whether this chip is the clock source for the audio hardware. */
    bool locally_clocked:8;

#ifdef CHIP_BASE_HYDRA
    /**
     * Is the hardware allocated?
     */
    bool hw_allocated:8;
    /**
     * Is the monitor running?
     */
    bool monitor_enabled:8;

    /* flag showing the endpoint is overridden */
    bool is_overridden:8;

    /**
     * Bit shift to configure on buffer
     */
    unsigned int shift:5;

    /**
     * How often should the audio endpoint notify us of new data?
     * (measured in samples)
     */
    int monitor_threshold;
#endif

    /** channel order */
    SPDIF_CHANNEL_ORDER channel_order;

    /* HW instance */
    unsigned instance;

    /* the output format of spdif endpoint is always
     * SPDIF_INPUT_DATA_FORMAT (which means it can be either
     * audio, or data and in both cases it can be interleaved
     * as well), so it can only connect to a spdif_decode
     * operator. However to be able to test the endpoint
     * separately we allow to explicitly change the endpoint
     * output format, it shouldn't be used in a real application.
     */
    unsigned output_format;

    /**
     * The period of the kick timer.
     */
    unsigned int kick_period;

    /* current sample rate of the input */
    unsigned int sample_rate;

    /* an A type must be paired with a B type
     */
    struct ENDPOINT *twin_endpoint;

    /* Note: this structure is also used
     * in assembly function
     * */
    struct spdif_extra_states
    {

      /* The sample rate that is detected
       * by FW (external) */
       unsigned fw_sample_rate;

      /* a flag showing fw changed its
       * decision */
       unsigned fw_sample_rate_changed;

       /* when stream stalled, the endpoint will generate silence
        * for limited or unlimited amount of time
        */
       int silence_duration;

       /* number of auxiliary bits*/
       unsigned num_aux_bits;

       /* a simple counter that incremented
        * every time channel status updated
        * (no immediate use)
        */
       unsigned channel_status_counter;

       /* save 384 bits of channel status,
        * (atomic update), first half bit will be
        * for channel A, second half for channel B
        */
       unsigned channel_status[2*SPDIF_NOROF_CHSTS_WORDS];

       /* last time the input was read
        */
       TIME last_read_time;

       /* final decision about whether
        * the stream shall be regarded as valid
        */
       unsigned stream_valid;

       /* all supported sample rates */
        unsigned nrof_supported_rates;
        const unsigned *supported_rates;

       /* bitmask for supported sample rates */
        unsigned supported_rates_mask;

       /* This is what DSP thinks about sample rate
        */
       unsigned dsp_sample_rate;

       /* last time the input was read
        */
       int read_interval;

       /* some state variables used
        * for pause handling
        */
       unsigned time_in_pause;
       unsigned pause_state;
       unsigned measured_sample_rate;
       unsigned silence_res;

       /* for rate  mismatch report */
       int norm_rate_ratio;

       /* internally stores double precision mismatch rate */
       int norm_rate_ratio_dp[2];

       /* rate detect in DSP */
       unsigned rate_detect_hist_index;

       /* some history buffer used for rate checking
        * in assembly function. The rate is measured based
        * on a history of amount of data received in last
        * 40 kicks, and final rate is decided based on last
        * 15 measurements.
        */
       unsigned rate_detect_hist[40+15];
    }*extra;
}endpoint_spdif_state;
#endif /* #ifdef INSTALL_SPDIF */

#ifdef INSTALL_USB_AUDIO
/* forward declaration for usb rate measure structure */
typedef struct usb_audio_rate_measure usb_audio_rate_measure;

/** The data specifically required for usb audio endpoints. The shape of this varies
 * dependent upon the platform one builds for. */
typedef struct endpoint_usb_audio_state
{

    /** The running state of the interrupt source. */
    bool running:1;

    /**
     * The timer id of the timer generating the next kick (Tx-only)
     */
    tTimerId kick_id;

    /**
     * The period of the kick timer. (Tx-only)
     */
    unsigned int kick_period;

    /** private audio data service handle */
    void *service_priv;

    /** The cbuffer that usb audio data arrives from. If the
     * endpoint is a source this represents the port in bluecore
     * and an mmu buffer in hydra */
    tCbuffer *source_buf;

    /** The cbuffer that usb audio data leaves through. If the
     * endpoint is a sink this represents the port in bluecore
     * and an mmu buffer in hydra */
    tCbuffer *sink_buf;

    /* nominal sample rate of the stream */
    unsigned sample_rate;

    /* number of channels */
    unsigned n_channels;

    /* subframe size in bits */
    unsigned subframe_size;

    /* data format of the endpoint, expected to be
     * USB_AUDIO_DATA_FORMAT to be connectable
     * to usb_audio operator. However it also
     * can be AUDIO_DATA_FORMAT_FIXP for testing
     * purposes
     */
    AUDIO_DATA_FORMAT data_format;

    /* number of frames in a TX packet - integer part
     * Note: the packet rate is 1000Hz, for most common sample
     * rates this will be translated into an integer number of
     * samples, but for 44.khz family of sample rates the
     * packet length is adjusted to keep the average sample
     * rate accurate.
     */
    unsigned frames_in_packet_int;

    /* number of frames in a TX packet - fractional part
    */
    unsigned frames_in_packet_rem;

    /* accumulator for remainder */
    unsigned packet_rem_acc;

    /* the length of (next) packet in frames */
    unsigned packet_len_in_frames;

    /* subframe_size/8 */
    unsigned subframe_in_octets;

    /* subframe_in_octets*n_channels */
    unsigned frame_in_octets;

    /* rate mismatch that will be reported to rate match manager
     * This isn't for TTP purpose.
     */
    unsigned norm_rate_ratio;

    /* whether this ep is performing rate-matching - TX only */
    bool ep_ratematch_enacting;

    /* Whether the rate adjustment is in TTP mode - TX only */
    bool timed_playback;

    /* the sra operator that performs rate adjustment - Tx only */
    cbops_op *rate_adjust_op;

    /* target rate adjust value, used when the endpoint is
     * enacting in non-TTP mode - TX only
     */
    unsigned target_rate_adjust_val;

    /* time stamp for last read packet from input Tx-only */
    TIME last_read_timestamp;

    /* whether last_read_timestamp is valid - TX only */
    bool last_read_timestamp_valid;

    /* current error threshold - Tx only */
    int error_threshold;

    /* few fields for adjusting timer period - Tx only */
    /* for detecting stall in packet consumption */
    unsigned timer_period_adjust_stall_counter;

    /* adjustment starts after seeing movement for some time */
    unsigned timer_period_adjust_normal_counter;

    /* whether we are adjusting the timer period */
    bool timer_period_adjust_normal_mode;

    /* difference between actual and expected packets sent */
    int timer_period_adjust_packet_offset;

    /* few fields for ttp error control */
    TIME_INTERVAL ttp_control_prev_error;

    /* accumulator for averaging error */
    int ttp_control_error_acc;
    unsigned ttp_control_error_acc_counter;

    /* maximum number of packets in sink buffer - Tx only */
    unsigned max_packets_in_tx_output_buffer;

    /** The PID controller settings and internal state - Tx only */
    ttp_pid_controller *pid;

    /* rate measure structure */
    usb_audio_rate_measure *rate_measure;

    /* counter for host pause detection */
    unsigned no_copy_counter;

    /* Flag showing host is in pause now */
    bool tx_host_in_pause;

} endpoint_usb_audio_state;

#endif /* #ifdef INSTALL_USB_AUDIO */

#ifdef INSTALL_SCO
/** The data specifically required for sco endpoints. The shape of this varies
 * dependent upon the platform one builds for. */
typedef struct endpoint_sco_state
{
    /**
     * The timer id of the timer generating the next kick. It may be
     * possible to union this with the monitor_enabled field.
     */
    tTimerId kick_id;

    /**
     * The period of the kick timer, relative to the BT master clock.
     */
    unsigned int kick_period_us;

    /**
     * Local time of the current BT slot (SCO slot or ISO anchor point).
     * Used to compute ToA of data received in the current slot.
     */
    TIME current_slot_time;

    /**
     * The BT clock value at which data is expected to be available. Either
     * for transmission over the air or to have been received over the air.
     */
    uint32 data_avail_bt_ticks;

    /**
     * Microsecond offset from time derived from BT clock for cases where
     * the kick period is not a whole number of slots
     */
    TIME_INTERVAL data_avail_us_remainder;

    /**
     * The processing time of the to-air endpoint.
     */
    unsigned int proc_time;

    /**
     * The BT clock count when the rate was last reported.
     */
    int32 rm_bt_clock;

    /**
     * The microsecond offset when the rate was last reported.
     */
    TIME_INTERVAL rm_us_remainder;

    /**
     * Pointer to the cbuffer structure that encapsulates the mmu buffer
     * associated with the sco endpoint
     */
    tCbuffer *cbuffer;

    /* For SCO rate measurement in sco_sched_kick */
    int  rate_measurement;
    TIME rm_start_time;

    /* difference between actual and expected received packets*/
    int packet_offset;
    unsigned packet_offset_counter;
    /* shows whether we are confident about packet offset */
    bool packet_offset_stable;

    /* counter showing number of times we turned up at expected
     * arrival time but new sco packet hadn't arrived yet.     *
     */
    unsigned late_arrival_counter;

    /* The last time we observed a late arrival */
    TIME last_late_arrival_time;

    /* TRUE if the endpoint start has been requested, but is waiting for
     * something (typically SCO_PARAMS) before starting the kick timers
     */
    bool start_pending;

    /* standalone rate adjust operator that
     * is available to this endpoint
     */
    EXT_OP_ID external_rate_adjust_opid;

#if defined(PROFILER_ON) && defined(INSTALL_SCOISO_EP_PROFILING)
    profiler *profiler;
#endif

    /* Pointer to sco drv data instace.
     * Depending on the EP direction this will be one of the two:
     * - sco sink drv instance assocoiated to a SCO sink ep
     * - sco source driver associated to a SCO source ep
     */
    union SCO_DRV
    {
        SCO_SRC_DRV_DATA *sco_src_drv;
        SCO_SINK_DRV_DATA *sco_sink_drv;
    } sco_drv;

} endpoint_sco_state;
#endif /* INSTALL_SCO */

#ifdef INSTALL_ISO_CHANNELS
#define endpoint_iso_state endpoint_sco_state
#endif

#ifdef INSTALL_FILE
/** The data specifically required for file endpoints. The shape of this varies
 * dependent upon the platform one builds for. */
typedef struct endpoint_file_state
{
    /**
     * TRUE if the Hydra File endpoint has started, FALSE if stopped or not started.
     */
    bool running;
    /** cbuffer data is read from */
    tCbuffer *source_buf;
    /** cbuffer data is written to */
    tCbuffer *sink_buf;
    /** handle private to the data service */
    void * service_priv;
    /** Internal kick timer ID used to run a backwards kick at the same priority level
     *  as a bus interrupt
     */
    tTimerId internal_kick_timer_id;
    /** BAC handle byte swap configuration of the underlying source mmu data buffer */
    bool byte_swap:8;
    /** Number of usable octets per word in the transform buffer (3 usable bits) */
    unsigned usable_octets:8;
    /** BAC handle shift of the underlying source mmu data buffer */
    unsigned shift:5;
    /* The endpoint's data format. */
    AUDIO_DATA_FORMAT data_format;
    /* Channel number when synchronised in a group */
    unsigned channel;
} endpoint_file_state;
#endif /* INSTALL_FILE */

#ifdef INSTALL_TIMESTAMPED_ENDPOINT
/** The data specifically required for timestamped endpoints. The shape of this varies
 * dependent upon the platform one builds for */
typedef struct endpoint_timestamped_state
{
    tCbuffer *source_buf;
    tCbuffer *sink_buf;
    /** handle private to the data service */
    void * service_priv;
    /** Internal kick timer ID used to run a kick from a connected operator
     *  at the same priority level as a bus interrupt
     */
    tTimerId internal_kick_timer_id;
    /** Count of octets uncopied from previously-tagged data */
    unsigned tag_octets_remaining;
    /** TRUE if the endpoint has started, FALSE if stopped or not started */
    bool running;
    /** TRUE if the initial data alignment has been completed */
    bool aligned;
    /** BAC handle shift of the underlying source mmu data buffer */
    unsigned shift:5;
    /* The endpoint data format */
    AUDIO_DATA_FORMAT data_format;
    /* The endpoint's usable octets. */
    unsigned usable_octets:3;
} endpoint_timestamped_state;
#endif /* INSTALL_TIMESTAMPED_ENDPOINT */

#ifdef INSTALL_RAW_BUFFER
typedef struct endpoint_raw_buffer_state
{
    tCbuffer *source_buf;
} endpoint_raw_buffer_state;
#endif /* INSTALL_RAW_BUFFER */

#ifdef INSTALL_AUDIO_DATA_SERVICE_TESTER
typedef struct endpoint_tester_state
{
    tCbuffer *source_data_buf;
    tCbuffer *sink_data_buf;
    void * service_priv;
    uint16 priv_hdr_len;
    /** TRUE if the endpoint has started, FALSE if stopped or not started */
    bool running;
    /** Internal kick timer ID used to run an operator kick at the same priority level
     *  as a bus interrupt
     */
    tTimerId internal_kick_timer_id;
} endpoint_tester_state;
#endif

#if defined(INSTALL_A2DP) ||\
    defined(INSTALL_SHUNT)

/** The data specifically required for a2dp endpoints. The shape of this varies
 * dependent upon the platform one builds for. */
typedef struct endpoint_a2dp_state
{
    /** The cbuffer that a2dp data arrives from. If the
     * endpoint is a source this represents the port in bluecore. */
    tCbuffer *source_buf;

    /** The cbuffer that a2dp data leaves through. If the
     * endpoint is a sink this represents the port in bluecore.*/
    tCbuffer *sink_buf;

    /** The running state of the interrupt source. */
    bool running:8;

    /** private audio data service handle */
    void *service_priv;

    /** The timer id of the timer used to retry pushing data forward */
    tTimerId self_kick_timer_id;

#ifdef INSTALL_SHUNT
    /** The L2CAP channel ID used by the endpoint */
     unsigned int cid;
#endif /* INSTALL_SHUNT */

} endpoint_a2dp_state, endpoint_shunt_state;

#endif /* INSTALL_A2DP || INSTALL_SHUNT */


#if defined(SUPPORTS_MULTI_CORE)
/** The data specifically required for shadow endpoints. */
typedef struct endpoint_shadow_state
{
    /** Data channel ID for KIP EP
     * This is preserved here to search the KIP transform connected
     * to this endpoint using the data channel id
     */
    uint16 channel_id;

    /** Flag to indicate if this shadow EP represents an endpoint that
     * supports metadata */
    bool supports_metadata:1;
#if !defined(COMMON_SHARED_HEAP)
    /** IPC data channel ID for transporting metadata tags */
    uint16 meta_channel_id;

    /** buffer associated with the metadata channel */
    tCbuffer *metadata_shared_buf;
#endif /* COMMON_SHARED_HEAP */
    /** If the endpoint is a source, this is what data channel connects
     *  across IPC and the Cbuffer is owned by the remote processor
     *  If the endpoint is a sink, this is what data channel extends to
     * the remote processor through IPC. This is same as the local cbuffer
     * connected to kip.
     */
    tCbuffer *buffer;

    /* The buffer size requirement for the endpoint. This is valid
     * only when buffer is NULL.
     */
    uint16 buffer_size;

    /* data format */
    AUDIO_DATA_FORMAT data_format;

    /** Flag indicating if a kick is currently in progress. This is a whole word
     * to avoid read-modify-write races. This is used to avoid the kick handler
     * thread safety issues. */
    bool kick_in_progress;

    /** remote kick is pending */
    bool remote_kick;

    /** The direction that a kick was received from when another kick was already
     * in progress. */
    ENDPOINT_KICK_DIRECTION kick_blocked;

    /**
     * Next shadow endpoint that is synchronised with this one. Set to NULL
     * if this endpoint is not synchronised with any other endpoint.
     */
    struct ENDPOINT *nep_in_sync;

    /**
     * Master endpoint that all other endpoints in the synchronised chain follow.
     * If this endpoint is not synchronised with any other endpoint, this
     * endpoint is the master.
     */
    struct ENDPOINT *head_of_sync;
} endpoint_shadow_state;
#endif /* defined(SUPPORTS_MULTI_CORE) */

/**
 * Structure that describes an endpoint
 */
struct ENDPOINT
{
    /**
     * Table of common endpoint function pointers
     */
    const struct ENDPOINT_FUNCTIONS *functions;

    /**
     * The key of the endpoint. Each endpoint within a specific endpoint type
     * and direction will have a unique key. Keys should not be treated as
     * unique across different endpoint types or endpoint directions.
     */
    unsigned int key;

    /**
     * The id of the endpoint. This is unique across all endpoint of the same
     * direction, i.e. it is possible for a source and a sink endpoint to have
     * the same id.
     */
    unsigned int id:16;

    /**
     * Connection id of the owner of this endpoint
     */
    unsigned int con_id:16;

    /**
     * Flag to say if the endpoint can be closed
     */
    bool can_be_closed:8;

    /**
     * Flag to say if the endpoint can be destroyed
     */
    bool can_be_destroyed:8;

    /**
     * Flag to say if an endpoint is created at connection time
     * and hence needs to be destroyed when disconnected.
     * Currently this is only true for operator endpoints.
     */
    bool destroy_on_disconnect:8;

    /**
     * Flag to say if an endpoint is a source or a sink
     */
    ENDPOINT_DIRECTION direction:8;

    /**
     * Flag to say if an endpoint is a 'real' endpoint
     * (where real means "at the end of a chain)
     */
    bool is_real:8;

    /**
     * Flag to say if an endpoint has been started
     * (is processing data).
     */
    bool is_enabled:8;

    /**
     * Flag to say if an endpoint uses the rate match interface
     * even though it is not a 'real' endpoint
     */
    bool is_rate_match_aware:8;

    /**
     * Enum to say how the endpoint should be connected
     */
    ENDPOINT_TYPE stream_endpoint_type:4;

    /**
     * Pointer to cbops_manager that encapsulates the cbops
     * information associated with the endpoint
     */
    struct cbops_mgr *cbops;

    /**
     * Pointer to the endpoint that this endpoint has a connection to.
     */
    struct ENDPOINT *connected_to;

    /**
     * Endpoint to kick. Can be null if the endpoint doesn't need to kick anything.
     */
    struct ENDPOINT *ep_to_kick;

    /**
     * Pointer to next endpoint in the list.
     */
    struct ENDPOINT *next;

    /**
     * Fields for running the kick function as a bg_int
     */
    struct endpoint_deferred_kick deferred;

    /**
     * Endpoint specific state information
     *
     * As this is variable length this MUST be at the end of the
     * structure
     */
    union ENDPOINT_STATE
    {
        struct endpoint_audio_state audio;
#ifdef INSTALL_SCO
        struct endpoint_sco_state sco;
#endif /* INSTALL_SCO */
#ifdef INSTALL_ISO_CHANNELS
        struct endpoint_iso_state iso;
#endif /* INSTALL_ISO_CHANNELS */
        struct endpoint_operator_state
        {
            /**
             * This is currently used as an on/off switch for DC remove.
             * It can be easily repurposed to indicate to an operator endpoint the 'per-chain'
             * data processing required like cbops (with details on which cbops operators are required),
             * rate matching decision.
             *
             */
            unsigned int cbops_flags;

            /** The bgint task of the underlying operator, this is provided to accelerate kicking from
             * endpoints. */
            BGINT_TASK op_bg_task;

            /** Linked list of operator's sources or sinks */
            struct ENDPOINT* next_terminal;

#ifdef INSTALL_TIMING_TRACE
            /** Transform, if connected */
            struct TRANSFORM* transform;
#endif /* INSTALL_TIMING_TRACE */
        }operator;
#ifdef INSTALL_FILE
        struct endpoint_file_state file;
#endif
#ifdef INSTALL_RAW_BUFFER
        struct endpoint_raw_buffer_state raw_buffer;
#endif

#ifdef INSTALL_SHUNT
        struct endpoint_a2dp_state shunt;
#endif /* INSTALL_SHUNT */

#ifdef INSTALL_A2DP
        struct endpoint_a2dp_state a2dp;
#endif /* INSTALL_A2DP */
#ifdef INSTALL_SPDIF
       struct endpoint_spdif_state spdif;
#endif /* #ifdef INSTALL_SPDIF */

#ifdef INSTALL_USB_AUDIO
        struct endpoint_usb_audio_state usb_audio;
#endif /* #ifdef INSTALL_USB_AUDIO */

#ifdef INSTALL_AUDIO_DATA_SERVICE_TESTER
        struct endpoint_tester_state tester;
#endif /* INSTALL_AUDIO_DATA_SERVICE_TESTER */
#if defined(SUPPORTS_MULTI_CORE)
        struct endpoint_shadow_state shadow;
#endif /* defined(SUPPORTS_MULTI_CORE) */
#ifdef INSTALL_TIMESTAMPED_ENDPOINT
        struct endpoint_timestamped_state timestamped;
#endif
    } state;
};


/****************************************************************************
Private Macro Declarations
*/

#define DEFINE_ENDPOINT_FUNCTIONS(name, \
    close,connect,disconnect,buffer_details,kick,sched_kick,start,stop, \
    configure,get_config,timing_info,sync,have_same_clock) \
  const ENDPOINT_FUNCTIONS endpoint_##name = \
  { close,connect,disconnect,buffer_details,kick,sched_kick,start,stop, \
    configure,get_config,timing_info,sync,have_same_clock }

#define IS_ENDPOINT_AUDIO_SOURCE(tag, dir) ((endpoint_##tag == endpoint_audio) && (dir == SOURCE))

/*
 * Do not include latency ctrl for Audio Source endpoints
 */
#define ENDPOINT_STATE_SIZE(tag, direction) (sizeof(struct endpoint_##tag##_state) - (IS_ENDPOINT_AUDIO_SOURCE(tag, direction) * sizeof(ENDPOINT_LATENCY_CTRL_INFO)))

#define STREAM_NEW_ENDPOINT(tag, key, direction, owner) \
          (stream_new_endpoint(&endpoint_##tag##_functions, key, ENDPOINT_STATE_SIZE(tag, direction), direction, endpoint_##tag, owner))

#define STREAM_NEW_ENDPOINT_NO_STATE(tag, key, direction, owner) \
          (stream_new_endpoint(&endpoint_##tag##_functions, key, 0, direction, endpoint_##tag, owner))

/*
 *  These are the cookies that convert between an internal id and an external
 *  id as seen by an off-chip application. (Note that these are not applied
 *  to operator endpoint ids).
 */
#define SOURCE_EP_COOKIE 0x0EA1
#define SINK_EP_COOKIE   0x0A5E

/**
 * Convert an internal endpoint id to an external endpoint id or vice-versa.
 * For operator endpoints the internal and external ID's are the same therefore
 * the ID won't be toggled.
 */
#define TOGGLE_EP_ID_BETWEEN_INT_AND_EXT(id) \
    do { \
        id = stream_toggle_ep_id_between_int_and_ext(id); \
    } while(0)

#define STREAM_EP_IS_OPEP_ID(id) (((id) & STREAM_EP_OP_BIT) == STREAM_EP_OP_BIT)
#define STREAM_EP_IS_REALEP_ID(id) (((id) & STREAM_EP_SHADOW_TYPE_MASK) == STREAM_EP_EP_BIT)
#define STREAM_EP_IS_SINK_EP(id) (((id) & STREAM_EP_TYPE_MASK) == STREAM_EP_EXT_SINK)

#define STREAM_EP_IS_OPERATOR(ep) ((ep)->stream_endpoint_type == endpoint_operator)
#define STREAM_EP_IS_REAL(ep) ((ep)->is_real)

#if defined(SUPPORTS_MULTI_CORE)
#define STREAM_GET_SHADOW_EP_ID(id)  ((id) & STREAM_EP_SHADOW_ID_MASK)
#define STREAM_EP_ID_FROM_SHADOW_ID(id) ((id) | STREAM_EP_SHADOW_TYPE_MASK )
#define STREAM_EP_IS_SHADOW_ID(id) (((id) & STREAM_EP_SHADOW_TYPE_MASK) == 0)

#define STREAM_EP_IS_SHADOW(ep) ((ep)->stream_endpoint_type == endpoint_shadow)
#define STREAM_EP_GET_SHADOW_EP(source_ep, sink_ep) ( (STREAM_EP_IS_SHADOW(source_ep)? source_ep : sink_ep) );

#define EP_TO_KICK_FOR_SHADOW_SOURCE(source, sink) ((STREAM_EP_IS_SHADOW(source) && STREAM_EP_IS_OPERATOR(sink))? sink: NULL)
#define EP_TO_KICK_FOR_SHADOW_SINK(source, sink) ((STREAM_EP_IS_SHADOW(sink) && STREAM_EP_IS_OPERATOR(source)) ? source: NULL)

#else
#define STREAM_EP_IS_SHADOW_ID(id) FALSE
#define STREAM_EP_IS_SHADOW(ep) FALSE
#define STREAM_EP_GET_SHADOW_EP(source_ep, sink_ep) NULL
#define EP_TO_KICK_FOR_SHADOW_SINK(source, sink) NULL
#define EP_TO_KICK_FOR_SHADOW_SOURCE(source, sink) NULL
#endif /* defined(SUPPORTS_MULTI_CORE) */

/*
 * Opmgr has equivalent API function. Using macro internally will be more
 * efficient than calling that function.
 */
#define GET_TERMINAL_FROM_OPIDEP(opidep) \
         (((opidep) & STREAM_EP_CHAN_MASK) >> STREAM_EP_CHAN_POSN)

#define GET_BASE_EPID_FROM_EPID(epid) ((epid) & ~(STREAM_EP_CHAN_MASK |  STREAM_EP_SHADOW_MASK))

#define STREAM_EP_IS_REAL_SHADOW_ID(id) (PROC_SECONDARY_CONTEXT() && \
                                         STREAM_EP_IS_REALEP_ID(id))

/**
 * Propagates a kick for a given endpoint.
 *
 * \param *endpoint - pointer to the endpoint that will kick.
 * \param direction - the direction of the kick.
 */
static inline void propagate_kick(ENDPOINT* endpoint, ENDPOINT_KICK_DIRECTION direction)
{
    ENDPOINT* endpoint_to_kick = endpoint->ep_to_kick;
    if (endpoint_to_kick)
    {
        endpoint_to_kick->functions->kick(endpoint_to_kick, direction);
    }
}

#ifdef INSTALL_SCO
static inline bool endpoint_is_sco_type(ENDPOINT *endpoint)
{
    return ( endpoint->stream_endpoint_type == endpoint_sco
#ifdef INSTALL_ISO_CHANNELS
            || endpoint->stream_endpoint_type == endpoint_iso
#endif
            );
}
#endif
/****************************************************************************
Functions from stream.c
*/

/**
 * \brief gets the first endpoint in the source or sink endpoint list
 *
 * \param dir direction of the endpoint, i.e. source or sink
 *
 */
ENDPOINT *stream_first_endpoint(ENDPOINT_DIRECTION dir);

/**
 * \brief create a new endpoint structure
 *
 * \param *functions pointer to the function table for the
 *        endpoint <br>
 * \param key unique key for the type of endpoint <br> Note that
 *        keys are not unique across different endpoint
 *        types <br>
 * \param state_size is the amount of data that the endpoints
 *        state requires <br>
 * \param dir direction of the endpoint, i.e. is it a source or
 *        a sink endpoint <br>
 * \param ep_type type of endpoint used for connection purposes
 *        <br>
 * \param con_id connection ID of the originator of this
 *        endpoint <br>
 *
 */
ENDPOINT *stream_new_endpoint(const ENDPOINT_FUNCTIONS *functions,
                              unsigned int key,
                              unsigned int state_size,
                              ENDPOINT_DIRECTION dir,
                              ENDPOINT_TYPE ep_type,
                              CONNECTION_LINK con_id);

/**
 * \brief returns a pointer to a endpoint based on the endpoints
 *        key and functions <br> It also requires whether the
 *        endpoint is a source or a sink
 *
 * \param key unique key for the type of endpoint <br> Note that
 *        keys are not unique across different endpoint
 *        types <br>
 * \param dir direction of the endpoint, i.e. is it a source or
 *        a sink endpoint <br>
 * \param *functions pointer to the function table for the
 *        endpoint <br>
 *
 */
ENDPOINT *stream_get_endpoint_from_key_and_functions(unsigned int key,
                                                     ENDPOINT_DIRECTION dir,
													 const ENDPOINT_FUNCTIONS *functions);

/**
 * \brief Causes the endpoint to stop functioning by first
 *        tearing down any active connections, and then calling
 *        stream_destroy_endpoint
 *
 * \param *endpoint pointer to endpoint to be closed
 *
 */
bool stream_close_endpoint(ENDPOINT *endpoint);

/**
 * \brief remove endpoint from endpoint list and free the memory
 *        used by the endpoint.
 *
 * \param *endpoint pointer to endpoint to be destroyed
 *
 */
void stream_destroy_endpoint (ENDPOINT *endpoint);

/**
 * \brief Gets endpoint head of synchronisation endpoint endpoint list.
 *
 * \param ep Pointer to endpoint
 *
 * \return Endpoint head of synchronisation endpoint endpoint list.
 */
ENDPOINT *stream_get_head_of_sync(ENDPOINT* ep);

/**
 * \brief Gets next endpoint in sync from endpoint list.
 *
 * \param ep Pointer to endpoint
 *
 * \return Next endpoint in sync from endpoint list.
 */
ENDPOINT *stream_get_nep_in_sync(ENDPOINT* ep);

/**
 * \brief Get endpoint configuration with a single uint32 result
 * \param ep Pointer to endpoint
 * \param key
 * \param value Pointer to result variable
 */
bool stream_get_endpoint_config(ENDPOINT* ep, unsigned key, uint32* value);

/****************************************************************************
Functions from stream_audio.c
*/

/**
 * \brief Get a pointer to an audio endpoint specified by
 *        hardware, instance and channel.
 *
 * If the endpoint doesn't exist then streams will attempt to create the
 * endpoint.
 *
 * \param con_id       Connection ID of the originator of this request
 * \param dir          Direction of the endpoint
 *                     (i.e. a source or a sink)
 * \param hardware     The audio hardware requested
 *                     (e.g. PCM/FM...)
 * \param num_params   Number of parameters pointed by "params".
 * \param params       Further device specific information.
 * \param[out] pending Set to TRUE by the endpoint if it needs to wait for
 *                     further information before it can be used.
 *
 * \return A pointer to the created endpoint, NULL otherwise.
 */
extern ENDPOINT *stream_audio_get_endpoint(CONNECTION_LINK con_id,
                                           ENDPOINT_DIRECTION dir,
                                           unsigned int hardware,
                                           unsigned num_params,
                                           unsigned *params,
                                           bool *pending);
#ifdef INSTALL_MCLK_SUPPORT
/**
 * \brief Activate mclk output for an audio interface
 *
 * \param ep              Pointer to endpoint.
 * \param activate_output If not 0, user wants to activate mclk OUTPUT for this
 *                        endpoint, otherwise it will de-activate the output.
 *                        Activation/De-activation request will only be done if:
 *                        - the endpoint can have mclk output (e.g i2s master)
 *                        - interface wants to route the MCLK output via GPIO,
 *                          Note that the MCLK output can be generated from
 *                          internal clock too.
 * \param enable_mclk     Makes the mclk available to use by the endpoint
 *                        (instead of root clock). For an interface to use MCLK
 *                        we need to make sure that the MCLK is available and
 *                        stable this should be able to be done automatically
 *                        before an interface gets activated (normally at
 *                        connection point), so we might deprecate this flag in
 *                        the future.
 * \param[out] pending Set to TRUE by the endpoint if it needs to wait for
 *                     further information before it can be used.
 *
 * \return True if successful.
 */
extern bool stream_audio_activate_mclk(ENDPOINT *ep,
                                       unsigned activate_output,
                                       unsigned enable_mclk,
                                       bool *pending);
#endif /* #ifdef INSTALL_MCLK_SUPPORT */

/****************************************************************************
Functions from stream_operator.c
*/

/**
 * \brief Get a pointer to an operator endpoint specified by key
 *
 * \param key the key of the endpoint
 *
 * \return pointer to the endpoint, or NULL if not found
 */
extern ENDPOINT *stream_operator_get_endpoint_from_key(unsigned key);

#ifdef INSTALL_ISO_CHANNELS
/****************************************************************************
Functions from stream_iso_interface.c
*/

/**
 * \brief Get a pointer to the iso endpoint specified by the hci handle
 *        and direction
 *
 * \param con_id     Connection ID of the originator of this request
 * \param dir        Whether a source or sink is requested
 * \param num_params The number of parameters provided
 * \param params     A pointer to the creation parameters
 *
 * \return Pointer to the iso endpoint or NULL if not found
 *
 */
extern ENDPOINT *stream_iso_get_endpoint(CONNECTION_LINK con_id,
                                         ENDPOINT_DIRECTION dir,
                                         unsigned num_params,
                                         unsigned *params);
#endif /* INSTALL_ISO_CHANNELS */

/****************************************************************************
Functions from stream_schedule_timers.c
*/

/**
 * \brief Schedule a real endpoint. Create the kick object and get the
 *        block size from a connected operator. For audio endpoints,
 *        checks if the endpoint is head of sync and does nothing if not.
 *
 * \param ep pointer to the endpoint
 *
 */
extern void stream_schedule_real_endpoint(ENDPOINT *ep);

/**
 * \brief Enables the endpoint so that it can be kicked and scheduled periodically.
 *
 * \param *ep Pointer to the endpoint structure which needs to be enabled
 */
extern void stream_enable_endpoint(ENDPOINT *ep);

/**
 * \brief Disables the endpoint so that the kicks and scheduling can stop.
 *
 * \param *ep Pointer to the endpoint structure which needs to be disabled
 */
extern void stream_disable_endpoint(ENDPOINT *ep);

/**
 * \brief Create an endpoint using external id
 *
 * \param ep_id  The external endpoint id
 * \param con_id The connection id
 *
 * \return Endpoint if successful else NULL.
 */
ENDPOINT* stream_create_endpoint(unsigned ep_id, CONNECTION_LINK con_id);

/**
 * \brief Check the type and destroy the endpoint
 *        It destroys only operator endpoint and KIP endpoint
 *
 * \param ep_id  The endpoint id
 *
 * \return Endpoint if successfully else NULL.
 */
bool stream_destroy_endpoint_id(unsigned ep_id);

/****************************************************************************
Functions from stream_connect.c
*/
/**
 * \brief Creates a transform structure and places it in the transform list.
 *
 * \param source_ep    Pointer to the source endpoint in the connection.
 * \param sink_ep      Pointer to the sink endpoint in the connection.
 * \param transform_id Unique identifier numeric identifier for the transform.
 *                     If 0, the function will select an identifier.
 *
 * \return A pointer to the newly created transform, NULL otherwise.
 */
extern TRANSFORM *stream_new_transform(ENDPOINT *source_ep,
                                       ENDPOINT *sink_ep,
                                       TRANSFORM_INT_ID transform_id);

/**
 * \brief Called during the creation of a transform to resolve differing
 *        source and sink data formats.
 *
 * \param source_ep pointer to the source endpoint in the connection
 * \param sink_ep pointer to the sink endpoint in the connection
 * \return \c TRUE if successful, \c FALSE otherwise
 */
bool stream_resolve_endpoint_data_formats(ENDPOINT *source_ep, ENDPOINT *sink_ep);

/**
 * \brief If the chain is complete, this function should
 *        set everything up for scheduling, priming and
 *        rate matching, and cbops if required
 *
 * \param src_ep source endpoint in the chain to work on.
 *
 * \param sink_ep source endpoint in the chain to work on.
 */
bool stream_chain_update(ENDPOINT *src_ep, ENDPOINT *sink_ep);

/**
 * \brief Returns the value that is most suitable for zeroing buffer content.
 *  It relies on endpoint data format so this has to be set up already.
 *
 * \param ep  endpoint in the chain to look at.
 *
 * \return The "zero" value for the endpoint buffer.
 */
unsigned int get_ep_buffer_zero_value(ENDPOINT* ep);

/****************************************************************************
Functions from stream.c
*/

/**
 * \brief Connect a source endpoint to a sink endpoint.
 *
 * \param source_ep    Pointer to the source endpoint.
 * \param sink_ep      Pointer to the sink endpoint.
 * \param state_info   Pointer that holds state information like buffer details
 *                     and timing information etc.
 * \param transform_id Unique identifier numeric identifier for the transform.
 *                     If 0, the function will select an identifier.
 *
 * \return A pointer to the transform connecting the 2 endpoints.
 *
 * \note The transform_id should be non zero only on secondary cores. In that
 *       case, the primary core selected the identifier that the secondary core
 *       must use.
 */
TRANSFORM *stream_connect_endpoints(ENDPOINT *source_ep,
                                    ENDPOINT *sink_ep,
                                    STREAM_CONNECT_INFO *state_info,
                                    TRANSFORM_INT_ID transform_id);

/**
 * \brief Get the endpoint buffer for the stream connection
 *
 * \param *source_ep pointer to the source endpoint
 * \param *sink_ep pointer to the sink endpoint
 * \param state_info - Connects state information will have the endpoint
 *                     buffer information.
 *
 * \return TRUE on getting the buffer successfully
 */
bool stream_connect_get_buffer( ENDPOINT *source_ep,
                                ENDPOINT *sink_ep,
                                STREAM_CONNECT_INFO* state_info);

#if defined(SUPPORTS_MULTI_CORE)
ENDPOINT* stream_shadow_ep_from_data_channel(unsigned data_chan_id);
ENDPOINT *stream_shadow_get_endpoint(unsigned int epid);
#endif /* defined(SUPPORTS_MULTI_CORE) */

/**
 * \brief Default check whether two endpoints of the same type share a clock source.
 *
 * \param ep1 Endpoint to compare with ep2.
 * \param ep2 Endpoint to compare with ep1.
 * \param both_local boolean indicating if both endpoints are locally clocked.
 *
 * \return TRUE if ep1 and ep2 share a clock source, otherwise FALSE.
 */
bool stream_have_same_clock_common(ENDPOINT *ep1, ENDPOINT *ep2, bool both_local);

/****************************************************************************
Functions from stream_monitor_interrupt.c
*/

#if defined(CHIP_BASE_HYDRA)

/**
 * \brief Enables read-monitor interrupts of specified type for the given endpoint.
 *
 * \param ep pointer to the audio endpoint that should be monitored
 * \param handle mmu_handle associated with the local buffer attached to the endpoint.
 * \param ko pointer to the KICK_OBJECT that will be kicked
 *
 * \note  We expect ep->state.audio.monitor_threshold to contain the monitor
 *        threshold. This specifies the number of samples between interrupts.
 *        In all scenarios we care about, one sample (be it 8-bit or 16-bit)
 *        takes up one word in buffer memory.
 *
 * \return Returns False if no dedicated read monitor interrupt is available
 */
extern bool stream_monitor_int_rd_enable(ENDPOINT* ep, mmu_handle handle, KICK_OBJECT *ko);

/**
 * \brief Enables write-monitor interrupts of specified type for the given endpoint.
 *
 * \param ep pointer to the endpoint that should be monitored
 * \param handle mmu_handle associated with the local buffer attached to the endpoint.
 * \param ko pointer to the KICK_OBJECT that will be kicked
 *
 * \note  We expect ep->state.audio.monitor_threshold to contain the monitor
 *        threshold. This specifies the number of samples between interrupts.
 *        In all scenarios we care about, one sample (be it 8-bit or 16-bit)
 *        takes up one word in buffer memory.
 *
 * \return Returns False if no dedicated write monitor interrupt is available
 */
extern bool stream_monitor_int_wr_enable(ENDPOINT* ep, mmu_handle handle, KICK_OBJECT *ko);

#if defined(SUPPORTS_CONTINUOUS_BUFFERING) && defined(AOV_EOD_ENABLED)
/****************************************************************************
Functions from stream_aov_eod_event.c
*/

/**
 * \brief Enables AoV Empty on Demand mechanism for the given endpoint.
 *
 * \param ep pointer to the audio endpoint that will have Empty on Demand enabled
 * \param ko pointer to the KICK_OBJECT that will be kicked
 *
 * \note  Always On Voice uses a buffer in the Audio harware which can the
 *        emptied by the Audio SS on demand.
 *
 * \return Returns False if no AoV instance is available
 */
extern bool stream_aov_eod_event_enable(ENDPOINT* ep, KICK_OBJECT *ko);

/**
 * \brief Disables AoV Empty on Demand mechanism for the given endpoint.
 *
 * \param ep pointer to the audio endpoint that will have Empty on Demand disabled
 *
 * \return Returns False when the enpoint's associated AoV instance is invalid
 */
extern bool stream_aov_eod_event_disable(ENDPOINT* ep);
#endif /* SUPPORTS_CONTINUOUS_BUFFERING && AOV_EOD_ENABLED */

#ifdef INSTALL_SPDIF
extern bool stream_spdif_monitor_int_wr_enable(ENDPOINT* ep, mmu_handle handle, KICK_OBJECT *ko);

#endif /* INSTALL_SPDIF */
#endif /* CHIP_BASE_HYDRA */

/****************************************************************************
Dummy Functions for function tables
*/

/* These are not going in to doxygen */
void stream_kick_dummy(ENDPOINT *ep, bool aod_valid, ENDPOINT_KICK_DIRECTION kick_dir);
void stream_sched_kick_dummy(ENDPOINT *ep, KICK_OBJECT *ko);
bool stream_close_dummy(ENDPOINT *ep);
bool stream_sync_sids_dummy(ENDPOINT *ep1, ENDPOINT *ep2);
ENDPOINT *stream_head_of_sync_dummy(ENDPOINT *ep);

/******************************************************************************
 * Functions declarations related to deferred kicks (i.e. delegated
 * from interrupt context to high priority bg_int)
 */
bool stream_set_deferred_kick(ENDPOINT* ep, bool deferred);
void stream_destroy_deferred_kick(ENDPOINT* ep);

/******************************************************************************
 * Protected Functions Declarations
 *
 * Functions only available to implementations of the base audio endpoint
 */

/*
 * \brief Generates a stream key based on the hardware, instance and channel
 *
 * \param hardware audio hardware to be used <br>
 * \param instance physical instance of hardware to be used <br>
 * \param channel channel, slot or port on hardware interface to be used <br>
 *
 */
extern unsigned create_stream_key(unsigned int hardware, unsigned int instance,
                                unsigned int channel);

/**
 * \brief Retrieves the audio hardware instance of an audio endpoint
 *
 * \param ep The endpoint to get the hardware instance of
 *
 * \return The hardware instance of the endpoint.
 */
extern STREAM_INSTANCE get_hardware_instance(ENDPOINT *ep);

/**
 * \brief Retrieves the audio hardware channel of an audio endpoint
 *
 * \param ep The endpoint to get the hardware channel of
 *
 * \return The hardware channel of the endpoint.
 */
extern STREAM_CHANNEL get_hardware_channel(ENDPOINT *ep);

/*
 * \brief Add an endpoint to the synchronisation list.
 *
 * \param ep1 endpoint
 * \param ep2 endpoint
 */
bool add_to_sync_list(ENDPOINT *ep1, ENDPOINT *ep2);

/*
 * \brief Remove an endpoint to the synchronisation list.
 *
 * \param ep endpoint
 *
 */
bool remove_from_sync_list(ENDPOINT *ep);

/**
 * \brief tell audio to sync two endpoints
 *
 * Audio endpoints are synchronised by hardware control, i.e., audio hardware reads both
 * endpoints at same time. This is useful for stereo processing. This function can be
 * used to sync two endpoints (or 2 separate group of endpoints) or to unsync an
 * endpoint from its sync group.
 * The synchronisation group is also maintained as a linked list in the endpoint state.
 * Each endpoint stores the head of the list and the next in the list. Storing the head
 * of list is easier to search through the list. However, when an endpoint is added to/
 * removed from the head, all the endpoints in the list have to be updated with the
 * change.
 *
 * \param *ep1 pointer to the first endpoint to sync
 * \param *ep2 pointer to the second endpoint to sync
 *
 * \return Whether the request succeeded.
 */
bool sync_endpoints (ENDPOINT *ep1, ENDPOINT *ep2);

/**
 * \brief Gets the ratematching capability an audio endpoint has in it's current
 * configuration.
 *
 * \param endpoint The endpoint that is being queried for its ratematching ability
 * \param value pointer where the ratematching capability is returned
 *
 * \return TRUE if value was populated.
 */
bool audio_get_config_rm_ability(ENDPOINT *endpoint, uint32 *value);

/**
 * \brief Sets up an audio Endpoint to perform or stop performing rateadjustment.
 *
 * \param endpoint  The endpoint that is being asked to perform rateadjustment.
 * \param value  The configure value containing a bool indicating enable or disable
 *
 * \return TRUE if the request was satisfied.
 */
bool audio_configure_rm_enacting(ENDPOINT *endpoint, uint32 value);

/**
 * \brief get the audio data format of the underlying hardware associated with
 * the endpoint. This function MUST be implemented by all implementations of an
 * audio endpoint.
 *
 * \param endpoint pointer to the endpoint to get the data format of.
 *
 * \return the data format of the underlying hardware
 */
AUDIO_DATA_FORMAT audio_get_data_format (ENDPOINT *endpoint);

/**
 * \brief Convert an internal endpoint id to an external endpoint id or vice-versa.
 * For operator endpoints the internal and external ID's are the same therefore
 * the ID won't be toggled.
 *
 * \param The external or internal endpoint ID
 *
 * \return The external endpoint ID if the argument was an internal endpoint ID,
 * or vice versa.
 */
unsigned stream_toggle_ep_id_between_int_and_ext(unsigned ep_id);

#endif /*_STREAM_ENDPOINT_H_*/
