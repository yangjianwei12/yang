/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco_nb
 * \file  nbs_struct.h
 * \ingroup capabilities
 *
 * Narrow Band Speech type header. <br>
 *
 */

#ifndef NBS_TYPES_H
#define NBS_TYPES_H
/*****************************************************************************
Include Files
*/
#include "sco_common_struct.h"
#ifdef INSTALL_PLC100
#include "plc100_c.h"
#endif /* INSTALL_PLC100 */



/** capability-specific extra operator data for SCO SEND */
typedef struct
{

    /** Common part of SCO SEND operators' extra data - it packages the buffer size,
    *  fadeout parameters and SCO parameters so that common code can safely reference same fields.
    */
    SCO_COMMON_SEND_OP_DATA sco_send_op_data;

#ifdef CVSD_CODEC_SOFTWARE
    /** CVSD data struct*/
    sCvsdState_t cvsd_struct;
    /** Pointer to scratch memory */
    int* ptScratch;
#endif

} SCO_SEND_OP_DATA;


/** capability-specific extra operator data for WBS DEC */
typedef struct SCO_NB_DEC_OP_DATA
{
    /** Common part of SCO Rcv operators' extra data - it packages the buffer
     * size, fadeout parameters and SCO parameters so that common code can
     * safely reference same fields.
     */
    SCO_COMMON_RCV_OP_DATA sco_rcv_op_data;

#ifdef CVSD_CODEC_SOFTWARE
    /** CVSD data struct*/
    sCvsdState_t cvsd_struct;
    /** Pointer to scratch memory */
    int* ptScratch;
#endif

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

    unsigned sco_rcv_output_samples;
} SCO_NB_DEC_OP_DATA;

#endif /* NBS_TYPES_H */
