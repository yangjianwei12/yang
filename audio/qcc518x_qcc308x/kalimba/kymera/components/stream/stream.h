/****************************************************************************
 * Copyright (c) 2011 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup stream Stream Subsystem
 */
/**
 * \file stream.h
 * \ingroup stream
 *
 * This file contains public stream functions and types.
 */

#ifndef STREAM_H
#define STREAM_H

/****************************************************************************
Include Files
*/

#include "types.h"
#include "buffer.h"
#include "status_prim.h"
#include "stream_prim.h"
#include "stream_audio_data_format.h"
#include "stream_common.h"
#include "stream_for_ops.h"
#include "adaptor/connection_id.h"

/****************************************************************************
Public Type Declarations
*/

typedef struct TRANSFORM TRANSFORM;
typedef struct STREAM_CONNECT_INFO STREAM_CONNECT_INFO;

typedef bool (*STREAM_ENDPOINT_CBACK)(CONNECTION_LINK con_id,
                                      STATUS_KYMERA status,
                                      unsigned source_id);

typedef bool (*STREAM_TRANSFORM_CBACK)(CONNECTION_LINK con_id,
                                       STATUS_KYMERA status,
                                       TRANSFORM_ID transform_id);

typedef bool (*STREAM_TRANSFORM_PAIR_CBACK)(CONNECTION_LINK con_id,
                                            STATUS_KYMERA status,
                                            TRANSFORM_ID transform_id_source,
                                            TRANSFORM_ID transform_id_sink);


/** Structure containing properties of a requested buffer's details */
struct BUFFER_DETAILS
{
    /** Indicates whether the requestee already has a buffer to supply */
    bool supplies_buffer:8;

    /** Indicates that the endpoint can be overriden. */
    bool can_override:8;

    /** Indicates that the endpoint will override
     * the connected endpoint */
    bool wants_override:8;

    /** Indicates that the requestee is going to run in place. For an operator
     * sink this indicates that the input buffer will be re-used at one or more
     * of the output terminals. For an operator source this indicates that the
     * buffer should be the same underlying memory as that of one of it's input
     * terminals. The terminal is indicated by passing the Cbuffer pointer of
     * the input terminal.
     */
    bool runs_in_place:8;

    /** Indicates that the endpoint is metadata aware or not. If the endpoint is
     * a source then it indicates that it will produce metadata. If it's a sink
     * then it indicates that it can consume/transport metadata. */
    bool supports_metadata:8;

    /** When the supports_metadata flag is set this buffer if present shares
     * metadata with the connection being made. */
    tCbuffer *metadata_buffer;

    /** Union discriminated by supplies_buffer. When supplies_buffer is TRUE
      * buffer should be populated. */
    union buffer_info_union{
        /** The buffer parameters being requested when
          * supplies_buffer and runs_in_place is FALSE */
        struct buffer_requirements{
            /** The minimum size of buffer required */
            unsigned int size;
            /** Any configuration flags requested for the buffer */
            unsigned int flags;
        }buff_params;

        /** A pointer to the buffer that the endpoint wants to supply.
          * supplies_buffer is TRUE */
        tCbuffer *buffer;


        /** The buffer parameters being requested when runs_in_place is True */
        struct {
            /**
             * The in place terminal shows where the operator would run in pace
             * on for the asked endpoint. */
            unsigned int in_place_terminal;
            /** The operator minimum buffer size in words. */
            unsigned int size;
            /** A pointer to the buffer that the in_place_terminal is connected
              * to. NULL if the terminal is not yet connected. */
            tCbuffer *buffer;
        }in_place_buff_params;
    }b;
};

/****************************************************************************
Public Constant Declarations
*/
/* macro to convert STREAM_RATEMATCHING rate to a fractional number */
#define STREAM_RATEMATCHING_RATE_TO_FRAC(x) \
    (((int)(1 << STREAM_RATEMATCHING_FIX_POINT_SHIFT) - (int)(x)) << \
        (DAWTH - STREAM_RATEMATCHING_FIX_POINT_SHIFT - 1))

/* Sampling rate for the streams in the system */
#define STREAM_AUDIO_SAMPLE_RATE_8K    8000
#define STREAM_AUDIO_SAMPLE_RATE_16K  16000
#define STREAM_AUDIO_SAMPLE_RATE_32K  32000
#define STREAM_AUDIO_SAMPLE_RATE_44K1 44100
#define STREAM_AUDIO_SAMPLE_RATE_48K  48000
#define STREAM_AUDIO_SAMPLE_RATE_96K  96000

/****************************************************************************
Public Macro Declarations
*/


/****************************************************************************
Public Variable Declarations
*/

/****************************************************************************
Public Function Declarations
*/

/****************************************************************************
Functions from stream.c
*/

/**
 * \brief gets the endpoint from the externally visible id
 *
 * \param id Id as seen by the host
 *
 */
extern ENDPOINT *stream_endpoint_from_extern_id(unsigned id);

/**
 * \brief Find out whether endpoint exists in the chain(s).
 *
 * \param ep The ENDPOINT structure pointer for the endpoint to be found.
 *
 * \return TRUE/FALSE depending on endpoint found or not.
 */
extern bool stream_does_ep_exist(ENDPOINT* ep);

/**
 * \brief gets the external id from the endpoint
 *
 * \param *endpoint pointer to the endpoint that we want the id
 *        from
 *
 */
extern unsigned stream_external_id_from_endpoint(ENDPOINT *endpoint);

#endif /* STREAM_H */

