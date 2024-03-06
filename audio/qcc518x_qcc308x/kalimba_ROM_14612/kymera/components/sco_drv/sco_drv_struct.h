/****************************************************************************
 * Copyright (c) 2013 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco
 * \file  sco_drv_struct.h
 * \ingroup capabilities
 *
 * SCO operators public header file.
 *
 */

#ifndef SCO_DRV_STRUCT_H
#define SCO_DRV_STRUCT_H

#include "audio_fadeout.h"
#include "op_msg_utilities.h"
#ifdef CVSD_CODEC_SOFTWARE
#include "cvsd.h"
#endif
#include "sco_global.h"
#include "ttp/ttp_pid.h"

/* Record header size in 16 bit words. Because the record header in the
 * input buffer is in 16 bit unpacked words we can consider them as words.
 */
#define METADATA_HEADER_SIZE_SCO_OR_ISO_UNFRAMED 5
#define METADATA_HEADER_SIZE_ISO_FRAMED 7

#define METADATA_HEADER_SIZE_MAX \
    MAX((METADATA_HEADER_SIZE_SCO_OR_ISO_UNFRAMED), \
        (METADATA_HEADER_SIZE_ISO_FRAMED))

#define RECORD_HEADER_BYTES(words) \
    (CONVERT_SCO_WORDS_TO_OCTETS(words))

/* Sync word for used in the record metadata (both to and from air).
 */
#define METADATA_HEADER_SYNC_WORD (0x5c5c)
#define SYNC_WORD_BYTES (CONVERT_SCO_WORDS_TO_OCTETS(1))

/* The size of the record header without the SYNC_WORD, in bytes.
 */
#define RECORD_HEADER_WITHOUT_SYNC_WORD_BYTES(words) (RECORD_HEADER_BYTES(words) - \
                                                      SYNC_WORD_BYTES)

/** Default buffer size, minimum and default block size for sco receive  TODO in words. */
#define SCO_RCV_INPUT_BUFFER_SIZE                  (SCO_DEFAULT_SCO_BUFFER_SIZE)
#define SCO_RCV_OUTPUT_BUFFER_SIZE                 (SCO_DEFAULT_SCO_BUFFER_SIZE)


/**
 * The sco stream metadata consist of generic header and a sco specific header.
 */
typedef struct sco_metadata
{

    /*** Generic sco stream metadata header.***/
    /*
     * Resynchronisation word: 0x5C5C. This word is not presented
     * in this structure.
     */

    /**
     * The length of the metadata, in words. This includes the generic header, so the
     * minimum value is 4. For SCO nb and WBS the metadata length is 5.
     */
    unsigned metadata_length;
    /**
     *  The length of the associated data packet, in octets. If this is odd, then an
     *  octet of padding will be added to the end of the packet so that the transferred
     *  data is an integral number of words.
     */
    unsigned packet_length;

    /*** Sco specific metadata header. ***/
    /**
     * Status represents the state of the sco packet (see enumeration before).
     */
    metadata_status status;
    /**
     * The least 16 bits of the Bluetooth clock for the master's reserved slot.
     */
    unsigned time_stamp;
    /**
     * 32-bit microsecond timestamp (framed ISO only).
     */
    TIME framed_ts;
} stream_sco_metadata;

#ifdef INSTALL_ISO_CHANNELS
/* Parameters specific to framed from-air ISO data */
typedef struct ISO_FRAMED_RCV_PARAMS
{
    /**
     * Expected SDU timestamp
     */
    TIME expected_framed_ts;

    /*** Framed SDU tracking values ***/

    /**
     * The total number of SDUs we should have received
     */
    unsigned expected_sdu_count;
    /**
     * Amount of "SDU time" left in the current ISO interval.
     */
    unsigned expected_sdu_time;
    /**
     * The total number of SDUs we have either received or faked.
     */
    unsigned actual_sdu_count;
    /**
     * Count of consecutive kicks where the actual "count" is greater
     * than the "expected" count.
     */
    unsigned ahead_count;
    /**
     * Reference time for the previous kick.
     */
    TIME prev_kick_time;
    /**
     * ToA value used in calculations for the previous kick.
     */
    TIME prev_toa;
} framed_params;
#endif

/* Common parameters for SCO from-air operators.
 * Any calls to SCO FW need to work with same indexes to get to these
 * parameters located inside various operator data structures.
 */
typedef struct SCO_RCV_PARAMS
{
    /** SCO packet and timing-related parameters */
    unsigned sco_pkt_size;
    unsigned ts_step;
    unsigned out_of_time_pkt_cnt;
    unsigned expected_time_stamp;

    /* Length (in 16-bit words) of the in-band header record */
    unsigned md_header_len;

    /* Record header, of the current packet */
    stream_sco_metadata record_header;

    /* Whether the current record's header has been stashed */
    bool record_header_stashed;

#ifdef INSTALL_ISO_CHANNELS
    framed_params framed;
#endif

    /** if true, allow varying SDU size */
    unsigned variable_length_sdu;

    /** Some counters and variables used for debug mainly, not conditioned for only
     *  debug builds.
     */
    struct
    {
        /** Number of kicks received since the last restart */
        unsigned md_num_kicks;

        /** Saves the number of packet size changes */
        unsigned md_pkt_size_changed;

        /** The number of late packets.*/
        unsigned md_late_pkts;

        /** The number of timestamp jumps in the incoming packets.*/
        unsigned md_ts_jumps;

        /** Out of time packets can cause resets.
         * This is the counter for the number of resets*/
        unsigned md_out_of_time_reset;

        /** Counter for the number of bad kicks. */
        unsigned num_bad_kicks;

        /** Counter for the number of kicks without data.*/
        unsigned num_kicks_no_data;

#ifdef INSTALL_ISO_CHANNELS
        /** Counter for the number of framed SDUs inserted */
        unsigned num_framed_sdus_inserted;
#endif

#ifdef SCO_DEBUG_STATUS_COUNTERS
        /** Counter for in-band sco metadata with status ok.*/
        unsigned md_status_ok;

        /** Counter for in-band sco metadata with status crc error.*/
        unsigned md_status_crc;

        /** Counter for in-band sco metadata with status nothing received.*/
        unsigned md_status_nothing;

        /** Counter for in-band sco metadata with status not scheduled.*/
        unsigned md_status_not_sched;

        /** Counter for in-band sco metadata with status ok with weak bit mask.*/
        unsigned md_status_ok_wbm;
#endif

#ifdef SCO_DEBUG
        /** Counter for reading the sco metadata.*/
        unsigned num_pkts_searched;

        /** Counter for finding the sco metadata.*/
        unsigned metadata_found;
#endif /* SCO_DEBUG */
    } debug;
} SCO_RCV_PARAMS;


/* Common parameters for SCO to-air operators.
 * Any calls to SCO FW need to work with same indexes to get to these
 * parameters located inside various operator data structures.
 */
typedef struct SCO_SEND_PARAMS
{
    /* SCO packet and timing-related parameters */
    unsigned sco_pkt_size;

    /* Length (in 16-bit words) of the in-band header record */
    unsigned md_header_len;

    unsigned kicks;
    unsigned kicks_no_space;
    unsigned kicks_no_data;
} SCO_SEND_PARAMS;

typedef enum SCO_DATA_FORMAT
{
    SCO_DATA_16_BIT = 0,
    SCO_DATA_16_BIT_FIXP,
    SCO_DATA_16_BIT_BYTE_SWAP,
}SCO_DATA_FORMAT;

typedef struct SCO_ENDPOINT_TIMING
{
    bool valid;
    TIME toa_ep;
    int sp_adjust;
}SCO_ENDPOINT_TIMING;

/* Data structure used by the SCO source driver */
struct SCO_SRC_DRV_DATA
{
    /* Terminal buffers */
    SCO_TERMINAL_BUFFERS buffers;

    /* SCO parameters that are common between different
     * SCO from-air capabilities
     */
    SCO_RCV_PARAMS sco_rcv_parameters;

    metadata_tag *cur_tag;

    SCO_DATA_FORMAT data_format;

    unsigned output_buffer_size;

    /* Timimg info passed from the endpoint to the sco src drv,
     * used to generate metadata
     */
    SCO_ENDPOINT_TIMING ep_timing;

    /* Signals the acumulated discarded data amount. */
    unsigned discarded_data;
};

/**
 * Structure for holding to air record framing related data.
 */
typedef struct SCO_RECORD_FRAMING
{
    /* Flag showing if to air framing is enabled.*/
    bool enabled:8;

    /* Sequence number of the packet when framing is enabled. */
    unsigned seq_num:16;
}SCO_RECORD_FRAMING;

/* sink driver flag for To-air-TTP:
 * To signal status of packets with TTP timestamps against
 * current driver reference time.
 * "IN_TIME" means within +/-1 TESCO relative
 * to current driver reference
*/
typedef enum TTP_IN_DATA_STATUS
{
    TTP_IN_DATA_EARLY  = -1,
    TTP_IN_DATA_IN_TIME = 0,
    TTP_IN_DATA_LATE    = 1
}TTP_IN_DATA_STATUS;

typedef struct SINK_DRV_TTP_INFO
{
    /* current reference time in [us]
     * Used for TTP input packet
     * early/late/in-time decision
     */
    TIME ref_time;

    /* Maximum deviation of input buf timestamp
     * from ref_time in [us] */
    unsigned max_ts_deviation_from_ref_time;

    /* current deviation of input pkt timestamp
     * from ref_time in [us]
     */
    TIME_INTERVAL ts_deviation_from_ref_time;

    /* timing state of input buffer */
    TTP_IN_DATA_STATUS input_data_status;

    /* TTP PID controller struct */
    ttp_pid_controller *pid;

    /* Flag to check if ttp is enabled in driver */
    bool ttp_enable;

}SINK_DRV_TTP_INFO;

/* Data structure used by the SCO sink driver */
struct SCO_SINK_DRV_DATA
{
    /* Terminal buffers */
    SCO_TERMINAL_BUFFERS buffers;

    /* SCO parameters that are common between different
     * sco to-air capabilities
     */
    SCO_SEND_PARAMS sco_send_parameters;

    SCO_DATA_FORMAT data_format;

    /* Record framing related data. */
    SCO_RECORD_FRAMING framing;

    /* SCO SINK timing related data*/
    SINK_DRV_TTP_INFO ttp_info;

    /**
     * Internal clone of the output buffer. This buffer is used to hold the data until the
     * whole to air record is ready. In this way the driver avoids presenting the record in
     * two segment which can result to incomplete record reads by the BT subsystem.
     */
    tCbuffer clone_op_buffer;
};

/**
 * To air record status status field:
 */
typedef enum SCO_RECORD_STATUS
{
    /** Valid data received. */
    SCO_RECORD_OK = 0,
    /** Status No data - sco driver got nothing useful */
    SCO_RECORD_NO_DATA,
}SCO_RECORD_STATUS;
#endif /* SCO_DRV_STRUCT_H */
