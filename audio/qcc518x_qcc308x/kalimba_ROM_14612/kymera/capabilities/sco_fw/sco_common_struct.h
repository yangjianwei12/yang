/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco
 * \file  sco_common_struct.h
 * \ingroup capabilities
 *
 * SCO decoders common public header file.
 *
 */

#ifndef SCO_COMMON_STRUCT_H
#define SCO_COMMON_STRUCT_H

#include "audio_fadeout.h"
#include "op_msg_utilities.h"
#ifdef CVSD_CODEC_SOFTWARE
#include "cvsd.h"
#endif
#include "sco_drv/sco_global.h"

/** The terminal ID of the input terminal */
#define INPUT_TERMINAL_ID (0 | TERMINAL_SINK_MASK)
/** The terminal ID of the output terminal */
#define OUTPUT_TERMINAL_ID (0)

/** Default buffer size, minimum and default block size for sco receive  TODO in words. */
#define SCO_RCV_INPUT_BUFFER_SIZE                  (SCO_DEFAULT_SCO_BUFFER_SIZE)
#define SCO_RCV_OUTPUT_BUFFER_SIZE                 (SCO_DEFAULT_SCO_BUFFER_SIZE)

/* Used to mark variable length arrays in structures. */
#define SCO_ANY_SIZE 1


/** Structure for additional terminals. Referenced from SCO_TERMINAL_BUFFERS.
 * This will be used by ISO operators that require multiple inputs or outputs.*/
struct ISO_ADDITIONAL_TERMINALS
{
    /** Number of additional terminals */
    unsigned num_additional_terminals;

    /** Indicates whether the additional terminals are inputs or outputs */
    bool terminals_are_inputs;

    /** Array of additional inputs */
    tCbuffer * additional_terminals[SCO_ANY_SIZE];
};

typedef struct sco_enc_ttp
{
    /** Timestamp from the last metadata tag processed */
    TIME last_tag_timestamp;

    /** Sample period adjustment value from the last metadata tag */
    unsigned last_tag_spa;

    /** Samples read since the last metadata tag */
    unsigned last_tag_samples;

    /* Error offset ID from last tag */
    unsigned last_tag_err_offset_id;

    /** Encode / Decode algorithmic delay, in samples */
    unsigned delay_samples;
} SCO_ENC_TTP;


/** 
 * Frame information for SCO/ISO operators
 */
typedef struct sco_iso_frame_info
{
    /** If whole_frame is TRUE, only whole aligned frames are handled.
      * and the size and duration fields will be ignored.
      * Otherwise, size and duration should both be nonzero. 
      */
    bool whole_frame;
    /** Encoded frame size. Zero if variable length */
    unsigned frame_size_octets;
    /** Frame duration. Zero if unknown */
    unsigned frame_duration_us;
} SCO_ISO_FRAME_INFO;

/** Common part of SCO SEND operator data - WB SCO uses extra information.
 *  In order for common functionality across NB and WB SCO to work safely,
 *  care must be taken to use this common definition when making pointer casts to refer to below
 *  part of the WB SCO operator(s).
 */
typedef struct SCO_COMMON_SEND_OP_DATA
{
    /** Terminal buffers */
    SCO_TERMINAL_BUFFERS buffers;

    /** Fade-out parameters */
    FADEOUT_PARAMS fadeout_parameters;

    SCO_ENC_TTP enc_ttp;

    unsigned output_buffer_size;
    unsigned input_buffer_size;
} SCO_COMMON_SEND_OP_DATA;


/** Ouptut time metadata history for the error handling decoder.
 */
typedef struct SCO_COMMON_RCV_METADATA_HIST_OUT
{
    /** If this field is set to true, TTP information is forwarded. Otherwise
     * TOA is converted to TTP based on the latency. */
    bool forward_ttp;

    /** Nr of samples generated since the last output metadata tag */
    unsigned nrof_samples;

    /** Timestamp from the last output metadata tag/ */
    TIME timestamp;

    /** Sample period adjustment value from the output metadata tag */
    unsigned spa;

    /** Only tags that are aligned with frames are accepted as a timpstamped tag.
     * Otherwise, due to the encoding we are not sure where the timestamp ends
     * or begins.*/
    bool valid_timestamp;
} SCO_COMMON_RCV_METADATA_HIST_OUT;

/** Common part of SCO RCV operator data - WB SCO uses extra information.
 *  In order for common functionality across NB and WB SCO to work safely,
 *  care must be taken to use this common definition when making pointer casts to refer to below
 *  part of the WB SCO operator(s).
 */
typedef struct SCO_COMMON_RCV_OP_DATA
{
    /** Terminal buffers */
    SCO_TERMINAL_BUFFERS buffers;

    /** Fade-out parameters */
    FADEOUT_PARAMS fadeout_parameters;

    CPS_PARAM_DEF  parms_def;

    /** Debug data fields */
    unsigned frame_count;
    unsigned frame_error_count;

    /** if enabled TTP will be generated instead of ToA */
    bool generate_timestamp;

    /** Latency used when transforming TOA to TTP. */
    unsigned timestamp_latency;

    /** Input metadata tag which will be reused for the input. */
    metadata_tag *cur_tag;

    /** Information saved for generating the output metadata tag */
    SCO_COMMON_RCV_METADATA_HIST_OUT out_tag;

    /** When the frames are not aligned with the metadata tags, we need to
     * remember the status read from the private data of the latest tag
     * that we've seen. (that tag also describes part of the data from
     * the frame we want to process). */
    metadata_status old_sco_status;

    /** The sample rate of the operator.*/
    unsigned sample_rate;

    /** Output buffer size of the operator.*/
    unsigned output_buffer_size;

    /** Input buffer size of the operator.*/
    unsigned input_buffer_size;

    /** After a discarded packet frame boundaries have to be
     * aligned to packet boundary. This field is used to keep
     * the amount of data to the next frame boundary when
     * discarding didn't have enough data. */
    unsigned amount_to_frame_boundary;
} SCO_COMMON_RCV_OP_DATA;

#endif /* SCO_COMMON_STRUCT_H */
