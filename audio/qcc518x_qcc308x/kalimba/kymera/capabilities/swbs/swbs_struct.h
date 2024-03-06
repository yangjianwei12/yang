/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup swbs
 * \file  swbs_struct.h
 * \ingroup capabilities
 *
 * Super Wide Band Speech type header. <br>
 *
 */

#ifndef SWBS_TYPES_H
#define SWBS_TYPES_H
/*****************************************************************************
Include Files
*/
#include "sco_common_struct.h"
#ifdef ADAPTIVE_R2_BUILD
#include "axBuf_t_r2.h"
#else
#include "axBuf_t.h"
#endif

#define APTX_ADAPTIVE_IN_BUF_SIZE           0xF78  // = 3300 = 825 words
#define APTX_ADAPTIVE_OUT_BUF_SIZE          0x2D00 // = 11520 SPACE FOR 2 OUTPUT PACKETS
                                             // 4608 (1152 samples) + 64

#define SWBS_BIT_ERROR_BUF_SIZE 0x7C // 122 (rounded up to 124) bytes

/** capability-specific extra operator data for SWBS ENC */
typedef struct SWBS_ENC_OP_DATA
{
    /** Common part of SCO SEND operators' extra data - it packages the buffer size,
    *  fadeout parameters and SCO parameters so that common code can safely reference same fields.
    */
    SCO_COMMON_SEND_OP_DATA sco_send_op_data;

    /* This is where additional SWBS-specific information can start */
    /* The codec deta structure - in the old days, it is what value at $caps.sco.CODEC_DATA_STRUC_PTR_FIELD used to point to */
    void* codec_data[64];            // aptX codec data struct
    unsigned codecMode;

    /** aptX adaptive uses axBuf types for data input/output */
    axBuf_t  axInBuf;
    axBuf_t  axOutBuf;

} SWBS_ENC_OP_DATA;


/** capability-specific extra operator data for SWBS DEC */
typedef struct SWBS_DEC_OP_DATA
{
    /** Common part of SCO Rcv operators' extra data - it packages the buffer size,
     *  fadeout parameters and SCO parameters so that common code can safely reference same fields.
     */
    SCO_COMMON_RCV_OP_DATA sco_rcv_op_data;

    /** Force PLC off - NOTE this flag is currently the only capability-specific
     *  parameter that is controllable from OBPM. If any other such parameters are
     *  added, they must be contiguous after this field - unless then all of them
     *  are moved into a dedicated structure, which is another option.
     */
    unsigned force_plc_off;

    /** The codec data structure - in the old days, it is what value at $caps.sco.CODEC_DATA_STRUC_PTR_FIELD used to point to */
    void* codec_data[64];             // aptX codec data struct

    /** aptX adaptive uses axBuf types for data input/output */
    axBuf_t  axInBuf;
    axBuf_t  axOutBuf;

    unsigned codecMode;

    unsigned storedWP;
    bool     overlap_finished;
    unsigned num_out_channels;
    unsigned packet_size;

#ifdef SCO_DEBUG
    /** Num of kicks in period */
    unsigned swbs_dec_num_kicks;
    /** num of times validate returned non zero values: zero means no space in op buffer */
    unsigned swbs_dec_validate_ret_nz;
    /** num of times validate returned values >= 240. */
    unsigned swbs_dec_validate_ret_good;
    /** num of times validate_wbm returned values >= 240. */
    unsigned swbs_dec_validate_ret_wbm;
#endif /* SCO_DEBUG */

    /** number of samples resulting from SWBS validate - this will match, if PLC is installed, the PLC packet length eventually */
    unsigned swbs_dec_output_samples;
} SWBS_DEC_OP_DATA;

#endif /* SWBS_TYPES_H */
