/****************************************************************************
 * Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
 ****************************************************************************/
/**
 * \defrocks ksp
 * \ingroup capabilities
 * \file  ksp_packet.h
 * \ingroup ksp
 *
 * ksp packet header
 *
 * A packet has a header and payload. It also can optionally have
 * channel-specific info. All parts are multiple of 4 octets.
 *
 * +---------------+---------------------------------------+------------------------------------+
 * | Header        | Channel specific info (optional)      | Payload                            |
 * | (64 bit)      | 32-bit x Number of channels           | (always multiple of 32-bit)        |
 * +---------------+---------------------------------------+------------------------------------+
 *
 * 1. Header
 *
 * Header has fixed size of 4 octets (32-bits).
 *
 * +------------+----------------+-------------------------------------------+
 * | Field      | Bits           | Comments                                  |
 * +============+================+===========================================+
 * | Sync Word  | 0:7 (8 bits)   | 0xA6                                      |
 * +------------+----------------+-------------------------------------------+
 * | Sequence   | 8:15 (8 bits)  | 128-mode counter, this can be used to     |
 * | Counter    |                | detect occasional lost packets in host    |
 * |            |                | side. Bit 7 is always 0                   |
 * +------------+----------------+-------------------------------------------+
 * | Channel-   | 16:18 (3 bit)  | if set the header is extended to have     |
 * | info       |                | one extra 32-bit info per channel.        |
 * +------------+----------------+-------------------------------------------+
 * | Number of  | 19:23 (5 bits) | Number of samples/words (1/8 of samples   |
 * | samples    |                | minus one), always same number of         |
 * |            |                | samples/words are read from each channel, |
 * |            |                | up to 256 samples/words per packet.       |
 * +------------+----------------+-------------------------------------------+
 * | Number of  | 24:26 (3 bits) | Number of channels in the stream (minus   |
 * | channels   |                | one), up to 8 channels is allowed.        |
 * +------------+----------------+-------------------------------------------+
 * | Data type  | 27:29 (3 bits) | Data type. All channels will have same    |
 * |            |                | data type.                                |
 * +------------+----------------+-------------------------------------------+
 * | Stream     | 30:31 (2 bits) | up to 4 independent streams are supported.|
 * | number     |                |                                           |
 * +------------+----------------+-------------------------------------------+
 * | Timestamp  | 32:63 (32 bits)| Chip timer time at the time of            |
 * |            |                | packetising.                              |
 * +------------+----------------+-------------------------------------------+
 *
 * Data types:
 *
 * type       value     transmits/size              suitable for
 * DATA16     0         Lower 16 bits (2 octets)    16-bit encoded channels
 * PCM16      1         Upper 16 bits (2 octets)    linear PCM audio channels
 * PCM24      2         Upper 24 bits (3 octets)    linear PCM audio channels
 * PCM32      3         Upper 32 bits (4 octets)    linear PCM audio channels
 * DATA32     4         Lower 32 bits (4 octets)    32-bit encoded channels(*)
 * TTR        5         32 bits (4 octets)          timing trace data
 * Reserved   6,7       -                           -
 *
 * (*) in Crescendo-based chips DATA32 will practically be the same as PCM32,
 * but will be useful when extracting data in host)
 *
 * 2. Channel-Specific info (Optional)
 *
 * This will be present only if the channel-info bit in header has been set.
 * There will be 32-bit info per channel. I haven't decided yet what
 * information should be sent, perhaps some metadata/ttp info about channels.
 * This will not be enabled in first delivery.
 *
 * Example with 3 channels in the stream.
 *
 * +---------------------+----------------------+----------------------+
 * | Chan0-specific-info | Chan1-specific-info  | Chan2-specific-info  |
 * | (32 bit)            | (32 bit)             |  (32 bit)            |
 * +---------------------+----------------------+----------------------+
 *
 * 3. Payload
 *
 * Data for channels are interleaved in the payload. The total size of the
 * payload will always be a multiple of four octets (32 bits), this is ensured
 * by enforcing the number of samples/words to always be a multiple of 8, so
 * for any given data type the total size will be multiple of 4 octets.
 *
 * Example for 2 channels, and number of samples in packet = 96
 * +--------------+--------------+--------------+--------------+-----                     ---+--------------+--------------+
 * | Chan0-data0  | Chan1-data0  | Chan0-data1  | Chan1-data1  |  ............               | Chan0-data95  | Chan1-data95|
 * |len(data type)|len(data type)|len(data type)|len(data type)|                             |len(data type)|len(data type)|
 * +--------------+------------- +--------------+------------- +------                    ---+--------------+------------- +
 *
 */
#ifndef _KSP_PACKET_H_
#define _KSP_PACKET_H_

#include <types.h>
#include "cbuffer_c.h" /* OCTETS_PER_SAMPLE */


typedef enum ksp_data_type
{                             /* transmits         suitable for              */
    KSP_DATATYPE_DATA16 =  0, /* Lower 16 bits     16-bit encoded streams    */
    KSP_DATATYPE_AUDIO16 = 1, /* Upper 16 bits     linear PCM audio channels */
    KSP_DATATYPE_AUDIO24 = 2, /* Upper 24 bits     linear PCM audio channels */
    KSP_DATATYPE_AUDIO32 = 3, /* Upper 32 bits     linear PCM audio channels */
    KSP_DATATYPE_DATA32 =  4, /* Lower 32 bits     32-bit encoded streams    */
    KSP_DATATYPE_TTR    =  5, /* Combined stream of 32-bit words */
    KSP_NUM_DATA_TYPES  =  6
}KSP_DATA_TYPE;

#define KSP_SYNC_BYTE (0xa6u)
#define STREAM_HEADER_LENGTH_WORDS  2
#define STREAM_HEADER_LENGTH_OCTETS (STREAM_HEADER_LENGTH_WORDS*OCTETS_PER_SAMPLE)
#define STREAM_CHANNEL_INFO_LEGTH_WORDS  0  /* channel specific meta data, not supported yet*/
#define STREAM_CHANNEL_INFO_LEGTH_OCTETS                \
    (STREAM_CHANNEL_INFO_LEGTH_WORDS*OCTETS_PER_SAMPLE)


typedef union ksp_packet_header_word_1
{
    struct {
        /** Always 0xA6 */
        uint32 sync                 :  8;

        /**
         * 128-mode counter, this can be used to detect occasional lost
         * packets in host side. Bit 7 is always 0
         */
        uint32 count                :  8;

        /**
         * if set the header is extended to have one extra 32-bit info
         * per channel.
         */
        uint32 channel_info         :  3;

        /**
         * Number of samples/words (1/8 of samples minus one), always
         * same number of samples/words are read from each channel,
         * up to 256 samples/words per packet
         */
        uint32 num_8samples_m1      :  5;

        /**
         * Number of channels in the stream (minus one), up to 8 channels
         * is allowed.
         */
        uint32 num_channels_m1      :  3;

        /** Data type. All channels will have same data type. */
        KSP_DATA_TYPE data_format   :  3;

        /** up to 4 independent streams are supported */
        uint32 stream_id            :  2;

    } fields;

    uint32 w;

} KSP_HEADER_WORD_1;

#endif /* _KSP_PACKET_H_ */
