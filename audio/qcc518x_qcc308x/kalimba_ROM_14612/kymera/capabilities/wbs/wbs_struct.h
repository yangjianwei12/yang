/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup wbs
 * \file  wbs_struct.h
 * \ingroup capabilities
 *
 * Wide Band Speech type header. <br>
 *
 */

#ifndef WBS_TYPES_H
#define WBS_TYPES_H
/*****************************************************************************
Include Files
*/
#include "sco_common_struct.h"
#include "sco_drv/sco_src_drv.h"
#include "sbc_c.h"
#ifdef INSTALL_PLC100
#include "plc100_c.h"
#endif /* INSTALL_PLC100 */


/** capability-specific extra operator data for WBS ENC */
typedef struct WBS_ENC_OP_DATA
{
    /** Common part of SCO SEND operators' extra data - it packages the buffer size,
    *  fadeout parameters and SCO parameters so that common code can safely reference same fields.
    */
    SCO_COMMON_SEND_OP_DATA sco_send_op_data;

    /* The codec deta structure - in the old days, it is what value at $caps.sco.CODEC_DATA_STRUC_PTR_FIELD used to point to */
    sbc_codec* codec_data;
} WBS_ENC_OP_DATA;


/** capability-specific extra operator data for WBS DEC */
typedef struct WBS_DEC_OP_DATA
{
    /** Common part of SCO Rcv operators' extra data - it packages the buffer size,
     *  fadeout parameters and SCO parameters so that common code can safely reference same fields.
     */
    SCO_COMMON_RCV_OP_DATA sco_rcv_op_data;

#ifdef INSTALL_PLC100
    /** Force PLC off - NOTE this flag is currently the only capability-specific
     *  parameter that is controllable from OBPM. If any other such parameters are
     *  added, they must be contiguous after this field - unless then all of them
     *  are moved into a dedicated structure, which is another option.
     */
    unsigned force_plc_off;

    /** PLC structure pointer if PLC installed */
    PLC100_STRUC* plc100_struc;
#endif /* INSTALL_PLC100 */

    /** The codec data structure - in the old days, it is what value at $caps.sco.CODEC_DATA_STRUC_PTR_FIELD used to point to */
    sbc_codec* codec_data;

#ifdef SCO_DEBUG
    /** Unin to hold wbs validate returned values.*/
    union
    {
        struct
        {
            /** Means no space in the output buffer.*/
            unsigned wbs_dec_validate_ret_0;

            /** Not enough data to process.*/
            unsigned wbs_dec_validate_ret_1;

            /** No sync word was found in the frame.*/
            unsigned wbs_dec_validate_ret_2;
        }named;

        /** Array for easy access.*/
        unsigned error_case[3];
    }wbs_dec_validate_ret;

    /** num of times the sync word was not at the start of the "frame".*/
    unsigned wbs_dec_validate_advanced;

    /** The number of times the decoder lost sync and the operator tried to
     * align the incoming data to the WBS frame to avoid the decoder running 
     * in unaligned mode.*/
    unsigned align_to_sync_count;

    /** The last amount which was discarded during the alignment.*/
    unsigned discarded_until_sync;
#endif /* SCO_DEBUG */

    /**  WBS validate returned an invalid result */
    unsigned wbs_dec_invalid_validate_result;

    /** number of samples resulting from WBS validate - this will match, if PLC is installed, the PLC packet length eventually */
    unsigned wbs_dec_output_samples;

} WBS_DEC_OP_DATA;

#endif /* WBS_TYPES_H */
