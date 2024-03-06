/****************************************************************************
 * Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco_fw Sco
 * \file  sco_drv_fw.h
 *
 * SCO drv header
 *
 */

#ifndef SCO_FW_C_H
#define SCO_FW_C_H

/****************************************************************************
Include Files
*/
#include "sco_drv_struct.h"
#include "sco_src_drv.h"
#ifdef SCO_DEBUG
#include "audio_log/audio_log.h"
#endif /* SCO_DEBUG */

/****************************************************************************
Public Type Declarations
*/
/**************************************************************************************
 * Stream SCO metadata related types.
 *
 */

/**
 * During the process of a sco packet first the sco metadata is read.
 * After reading the sco metadata it is analysed and the packet is processed. These
 * stages are represented by the sco packet status.
 */
typedef enum
{
    /**
     * Initial state. Packet is not read yet.
     */
    SCO_METADATA_NOT_READ,
    /**
     * Sco metadata is read and is ready for further analyses.
     */
    SCO_METADATA_READY,
    /**
     * Sco metadata was analysed and the SCO packet is late.
     */
    SCO_METADATA_LATE,
    /**
     * Sco metadata was analysed and the SCO packet is on time.
     */
    SCO_METADATA_ON_TIME,
    /**
     * Sco metadata was analysed and there is a jump in the
     * SCO metadata timestmp.
     */
    SCO_METADATA_TS_JUMP,
    /**
     * The sco metadata was read and is invalid.
     */
    SCO_METADATA_INVALID

} stream_sco_metadata_status;

/****************************************************************************
Public Constant Declarations
*/

/****************************************************************************
Public Variable Declarations
*/

/****************************************************************************
Public Constant Declarations
*/
/**
 * Constant string used for displaying a packet
 */
#define PACKET_STRING \
" ==> Packet    time_stamp: %5d; length: %5d; status: %5d;"

/**
 * Constant string used for displaying the sco state
 */
#define SCO_STATE_STRING \
"SCO_rcv_state  exp_time_stamp: %5d; sco_pkt_size: %5d; ts_step: %5d;"

/****************************************************************************
Public Macro Declarations
*/

/*
These values apply transformations to the sco metadata. The format of the data
should be the same on all platforms if the endpoints are doing there job
correctly it may be useful during development of a new platform to modify
these values.
*/

/* Byte swap */
#define SCO_NB_METADATA_BYTE_SWAP(x)    (((x)<<8) & 0xFF00) |(((x)>>8) & 0x00FF)

/* Right shift 8 */
#define SCO_NB_METADATA_SHIFT(x)        ((x)>> (DAWTH- 16))

/* Value used for masking the first 16 bit.*/
#define MASK_16_BIT_VAL                 (0xFFFF)

/* Mask the last two octets */
#define MASK_16_BIT(x)                  ((x) & MASK_16_BIT_VAL)

/**
 * Transform macro used in case of a sco nb.
 */
#ifdef CVSD_CODEC_SOFTWARE
#define SCO_NB_METADATA_TRANSFORM(x) MASK_16_BIT(x)
#else
#define SCO_NB_METADATA_TRANSFORM(x) MASK_16_BIT(SCO_NB_METADATA_SHIFT(x))
#endif

/**
 * Transform macro used for wbs.
 */
#define WBS_METADATA_TRANSFORM(x)    SCO_NB_METADATA_BYTE_SWAP(x)

/****************************************************************************
Public Function Declarations
*/

/* Metadata related functions. */
extern void init_packet_record_header(stream_sco_metadata *record_header);
extern stream_sco_metadata_status read_packet_metadata(SCO_SRC_DRV_DATA* sco_data, stream_sco_metadata *in_packet);
extern stream_sco_metadata_status analyse_sco_metadata(SCO_SRC_DRV_DATA* sco_data, stream_sco_metadata *in_packet);
#ifdef INSTALL_ISO_CHANNELS
extern stream_sco_metadata_status analyse_framed_metadata(SCO_SRC_DRV_DATA* sco_data, stream_sco_metadata *in_packet, unsigned sdu_interval);
#endif

extern void discard_packet(SCO_SRC_DRV_DATA *sco_data, unsigned packet_length);

/* Sco state controlling functions. */
extern void sco_fw_update_expected_timestamp(SCO_SRC_DRV_DATA* sco_data);
extern void sco_fw_check_bad_kick_threshold(SCO_SRC_DRV_DATA* sco_data);

/**
 * \brief Eliminate kymera metadata tags associated
 *        to the processed input buffer data.
 *
 * \param sco_data         Pointer to the SCO drv data.
 * \param input_processed  Amount of input processed (in octets).
 */
extern void sco_fw_consume_input_metadata(SCO_SRC_DRV_DATA *sco_data,
                                          unsigned input_processed);

/**
 * \brief Generates Kymera metadata for each packet in the SCO src drv output
 *        buffer. The tag contains ToA, status and discarded data information.
 *        The ToA is obtained from the SCO src endpoint. The status is provided
 *        by the SCO src drv, indicating the decoder to run PLC on the input.
 *        The amount of discarded data (in bytes) is also further passed to the
 *        decoder.
 *        In case the SCO src drv input buffer has metadata, the tag is
 *        reused for the output buffer. This should happen only for ASYNC_WBS.
 *
 * \param sco_drv_data      Pointer to the SCO src drv data structure.
 * \param amount_generated  The packet length (in bytes).
 * \param status            Kymera metadata status.
 */
extern void sco_fw_write_output_metadata(SCO_SRC_DRV_DATA *sco_data,
                                         unsigned amount_generated,
                                         metadata_status status);
/**
 * sco_rcv_flush_input_buffer
 * \brief clearing sco input buffer with metadata
 * \param Pointer to the sco drv data.
 */
extern void sco_rcv_flush_input_buffer(SCO_SRC_DRV_DATA* sco_data);

/**
 * \brief Populate the status of the SCO packet data into
 *        kymera metadata tag.
 *
 * \param mtag            Metadata tag
 * \param status          Status information to be populated
 * \param discarded_data  Notify the decoder of the amount of discarded data
 *
 * \return TRUE if private metadata was added in the tag, FALSE otherwise.
 */
extern bool sco_drv_append_metadata_status(metadata_tag *mtag,
                                           metadata_status status,
                                           unsigned discarded_data);

/**
 * \brief Log the current values of from-air debug counters.
 *
 * \param reason String containing reason for emitting this message.
 */
extern void sco_rcv_log_counters(SCO_SRC_DRV_DATA *sco_data,
                                 const char *reason);

#ifdef SCO_DEBUG_STATUS_COUNTERS
/**
 * sco_rcv_update_status_counters
 * \brief update internal debug counters related to the possible values of the
 *        status field in the metadata.
 * \param sco_data Pointer to the SCO drv data
 * \param sco_metadata Pointer to the sco_metadata copy
 */
extern void sco_rcv_update_status_counters(SCO_SRC_DRV_DATA* sco_data,
                                           stream_sco_metadata* sco_metadata);
#endif
/****************************************************************************
Public Function Definition
*/

/**
 * \brief Returns the stream sco packet payload length in words.
 *
 * \param  sco_data  Pointer to the stream sco packet.
 */
static inline unsigned get_packet_payload_length(stream_sco_metadata* sco_metadata)
{
    return CONVERT_OCTETS_TO_SCO_WORDS(sco_metadata->packet_length);
}

/**
 * \brief If SCO_DEBUG is enabled this function prints out the sco rcv state.
 *
 * \param  sco_data  Pointer to the sco drv data.
 */
static inline void print_SCO_state(SCO_SRC_DRV_DATA* sco_data)
{
    SCO_DBG_MSG3(SCO_STATE_STRING, sco_data->sco_rcv_parameters.expected_time_stamp,
                 sco_data->sco_rcv_parameters.sco_pkt_size,
                 sco_data->sco_rcv_parameters.ts_step);

}

/**
 * \brief If SCO_DEBUG is enabled this function prints out the packet read from the
 *        input buffer.
 *
 * \param  sco_packet  Pointer to the stream sco packet (must be different than NULL).
 */
static inline void print_sco_metadata(stream_sco_metadata *sco_metadata)
{
    SCO_DBG_MSG3(PACKET_STRING, sco_metadata->time_stamp,
            sco_metadata->packet_length,
            sco_metadata->status);

}

/**
 * \brief Translates one word of metadata from SCO to AudioSS endiannes
 * \param type Data format specifying how the data should be read from the
 *             buffer.
 * \returns Translated, readable metadata word
 */
static inline int transform_metadata(int data, SCO_DATA_FORMAT type)
{
    if (type == SCO_DATA_16_BIT_BYTE_SWAP)
    {
        return WBS_METADATA_TRANSFORM(data);
    }
    else
    {
        return SCO_NB_METADATA_TRANSFORM(data);
    }
}

#endif /* SCO_FW_C_H */
