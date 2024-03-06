/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 ****************************************************************************/
/**
 *
 * \file  anc_hw_manager_filter.c
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) filter functions.
 *
 */



#include "anc_hw_manager_filter.h"


bool check_filters_equal(uint32 *prev, uint32 *cur, uint32 length)
{
    for (int i = 0; i < length; i++)
    {
        if (prev[i] != cur[i])
        {
            return FALSE;
        }
    }
    return TRUE;
}

void ahm_update_filter_coeffs(AHM_IIR_FILTER_BANK *p_prev_iir, AHM_IIR_FILTER_BANK *p_iir, bool *cfg,
                              OPMSG_AHM_FILTER_COEFFS_MSG *p_msg,
                              uint16 mode,
                              EXT_OP_ID ext_op_id)
{
    unsigned int denom_taps;
    unsigned int numer_taps;
    unsigned int instance;
    unsigned int path;
    unsigned int denom_coeffs_start_offset;
    unsigned int num_coeffs_start_offset;
    unsigned msg_id;
    unsigned int i;
    unsigned int j;
    uint32 coeffs[TOTAL_NUM_COEFFS_IIR];
    uint32 coeffs_prev[TOTAL_NUM_COEFFS_IIR] = { 0 };
    unsigned int no_of_taps;
    no_of_taps = TOTAL_NUM_COEFFS_IIR;
    msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*) p_msg);
    instance = OPMSG_FIELD_GET(p_msg, OPMSG_SET_IIR_FILTER_COEFFS,ANC_INSTANCE);
    path = OPMSG_FIELD_GET(p_msg, OPMSG_SET_IIR_FILTER_COEFFS, FILTER);
    denom_taps = OPMSG_FIELD_GET(p_msg, OPMSG_SET_IIR_FILTER_COEFFS,
                                 QUANTITY_DENOMINATOR_COEFFS);
    numer_taps = OPMSG_FIELD_GET(p_msg, OPMSG_SET_IIR_FILTER_COEFFS,
                                 QUANTITY_NUMERATOR_COEFFS);

    L2_DBG_MSG3("OPID: %x, AHM IIR filter coeff payload: msg_id =%d, instance =%d",
                 ext_op_id, msg_id, instance);
    L2_DBG_MSG4("OPID: %x, AHM IIR filter coeff payload: path = %d, denom_taps =%d,"
                "num_taps=%d", ext_op_id, path, denom_taps, numer_taps);

    /* payload of data is [instance, path, no_of_denom_taps, no_of_numer_taps,
       denom_coeff_start, denom_coeff_start+1, ...... denom_coeff_start+end,
       num_coeff_start, num_coeff_start+....]
    */
    denom_coeffs_start_offset = DENOM_COEFFS_START_OFFSET;
    /* each of the payload data is 16 bit (MSW,LSW) which is fit into 32 bit */
    /* 2 *denom_taps (2 is because each word is 16 bits) */
    /* The ANC HW assumes that coefficients are in the order [LSW, MSW] */
    num_coeffs_start_offset = denom_coeffs_start_offset + 2 * denom_taps;

    if(mode == AHM_HYBRID_ENABLE)
    {
        switch (path)
        {
            case AHM_ANC_PATH_FFB_ID:
                for (i = 0, j = 0; i < 2 * denom_taps; i += 2, j += 1)
                {
                    p_iir->ff.p_den[j] = \
                    OPMSG_AHM_SET_COEFFS32(p_msg,denom_coeffs_start_offset + i);
                    coeffs[j] = (p_iir->ff).p_den[j];
                    coeffs_prev[j] = (p_prev_iir->ff).p_den[j];
                    L5_DBG_MSG3("OPID: %x, ANC FFA DENOM IIR COEFF NUM [%d] = 0x%08X",
                                ext_op_id, j, (p_iir->ff).p_den[j]);
                }
                for (i = 0, j = 0; i < 2 * numer_taps; i += 2, j += 1)
                {
                    p_iir->ff.p_num[j] = \
                    OPMSG_AHM_SET_COEFFS32(p_msg,num_coeffs_start_offset + i);
                    coeffs[j + denom_taps] = (p_iir->ff).p_num[j];
                    coeffs_prev[j + denom_taps] = (p_prev_iir->ff).p_num[j];
                    L5_DBG_MSG3("OPID: %x, ANC FFA NUMER IIR COEFF NUM [%d] = 0x%08X",
                                ext_op_id, j, (p_iir->ff).p_num[j]);
                }
        
                if (!check_filters_equal(coeffs_prev, coeffs, no_of_taps))
                {
                    cfg[AHM_ANC_FILTER_FF_ID] = TRUE;
                    L5_DBG_MSG1("OPID: %x, Mismatch between cur and prev filters in FF path",
                                ext_op_id);
                    #ifndef RUNNING_ON_KALSIM
                        stream_anc_set_anc_iir_foreground_coeffs(
                        (STREAM_ANC_INSTANCE) instance, (STREAM_ANC_PATH) path,
                        denom_taps + numer_taps, coeffs);
                    #endif

                    /* copy previous iir ff filters */
                    for (i = 0, j = 0; i < 2 * denom_taps; i += 2, j += 1)
                    {
                        (p_prev_iir->ff).p_den[j] = (p_iir->ff).p_den[j];
                    }
                    for (i = 0, j = 0; i < 2 * numer_taps; i += 2, j += 1)
                    {
                        (p_prev_iir->ff).p_num[j] = (p_iir->ff).p_num[j];
                    }

                }
                else
                {
                    L5_DBG_MSG1("OPID: %x, Current and previous filters in FF path are same",
                                ext_op_id);
                }
            break;
            case AHM_ANC_PATH_FFA_ID:
                for (i = 0, j = 0; i < 2 * denom_taps; i += 2, j += 1)
                {
                    p_iir->fb.p_den[j] = \
                    OPMSG_AHM_SET_COEFFS32(p_msg,denom_coeffs_start_offset + i);
                    coeffs[j] = (p_iir->fb).p_den[j];
                    coeffs_prev[j] = (p_prev_iir->fb).p_den[j];
                    L5_DBG_MSG3("OPID: %x, ANC FFB DENOM IIR COEFF NUM [%d] = 0x%08X",
                                ext_op_id, j, (p_iir->fb).p_den[j]);
                }
                for (i = 0, j = 0; i < 2 * numer_taps; i += 2, j += 1)
                {
                    p_iir->fb.p_num[j] = \
                    OPMSG_AHM_SET_COEFFS32(p_msg,num_coeffs_start_offset + i);
                    coeffs[j + denom_taps] = (p_iir->fb).p_num[j];
                    coeffs_prev[j + denom_taps] = (p_prev_iir->fb).p_num[j];
                    L5_DBG_MSG3("OPID: %x, ANC FFB NUMER IIR COEFF NUM [%d] = 0x%08X",
                                ext_op_id, j, (p_iir->fb).p_num[j]);
                }
                if (!check_filters_equal(coeffs_prev, coeffs, no_of_taps))
                {
                    cfg[AHM_ANC_FILTER_FB_ID] = TRUE;
                    L5_DBG_MSG1("OPID: %x, Mismatch between cur and prev filters in FB path",
                                ext_op_id);
                    #ifndef RUNNING_ON_KALSIM
                        stream_anc_set_anc_iir_foreground_coeffs(
                        (STREAM_ANC_INSTANCE) instance, (STREAM_ANC_PATH) path,
                        denom_taps + numer_taps, coeffs);
                    #endif
                    /* copy previous iir fb filters */
                    for (i = 0, j = 0; i < 2 * denom_taps; i += 2, j += 1)
                    {
                        (p_prev_iir->fb).p_den[j] = (p_iir->fb).p_den[j];
                    }
                    for (i = 0, j = 0; i < 2 * numer_taps; i += 2, j += 1)
                    {
                        (p_prev_iir->fb).p_num[j] = (p_iir->fb).p_num[j];
                    }
                }
                else
                {
                    L5_DBG_MSG1("OPID: %x, Current and previous Filters in FB path are same",
                                ext_op_id);
                }
            break;
            case AHM_ANC_PATH_FB_ID:
                for (i = 0, j = 0; i < 2 * denom_taps; i += 2, j += 1)
                {
                    p_iir->ec.p_den[j] = \
                    OPMSG_AHM_SET_COEFFS32(p_msg,denom_coeffs_start_offset + i);
                    coeffs[j] = (p_iir->ec).p_den[j];
                    coeffs_prev[j] = (p_prev_iir->ec).p_den[j];
                    L5_DBG_MSG3("OPID: %x, ANC EC DENOM IIR COEFF NUM [%d] = 0x%08X",
                                ext_op_id, j, (p_iir->ec).p_den[j]);
                }
                for (i = 0, j = 0; i < 2 * numer_taps; i += 2, j += 1)
                {
                    p_iir->ec.p_num[j] = \
                    OPMSG_AHM_SET_COEFFS32(p_msg,num_coeffs_start_offset + i);
                    coeffs[j + denom_taps] = (p_iir->ec).p_num[j];
                    coeffs_prev[j + denom_taps] = (p_prev_iir->ec).p_num[j];
                    L5_DBG_MSG3("OPID: %x, ANC EC NUMER IIR COEFF NUM [%d] = 0x%08X",
                                ext_op_id, j, (p_iir->ec).p_num[j]);
                }
                if (!check_filters_equal(coeffs_prev, coeffs, no_of_taps))
                {
                    cfg[AHM_ANC_FILTER_EC_ID] = TRUE;
                    L5_DBG_MSG1("OPID: %x, Mismatch between cur and prev filters in EC path",
                                ext_op_id);
                    #ifndef RUNNING_ON_KALSIM
                        stream_anc_set_anc_iir_foreground_coeffs(
                        (STREAM_ANC_INSTANCE) instance, (STREAM_ANC_PATH) path,
                        denom_taps + numer_taps, coeffs);
                    #endif
                    /* copy previous iir ec filters */
                    for (i = 0, j = 0; i < 2 * denom_taps; i += 2, j += 1)
                    {
                        (p_prev_iir->ec).p_den[j] = (p_iir->ec).p_den[j];
                    }
                    for (i = 0, j = 0; i < 2 * numer_taps; i += 2, j += 1)
                    {
                        (p_prev_iir->ec).p_num[j] = (p_iir->ec).p_num[j];
                    }
                }
                else
                {
                    L5_DBG_MSG1("OPID: %x, Current and previous filters in EC path are same",
                                ext_op_id);
                }
            break;
            default:
                L2_DBG_MSG1("OPID: %x, Invalid filter path", ext_op_id);
                UNUSED(coeffs);
        }
    }
    else if(mode == AHM_FEEDFORWARD_ENABLE)
    {
        switch (path)
        {
             case AHM_ANC_PATH_FFA_ID:
                for (i = 0, j = 0; i < 2 * denom_taps; i += 2, j += 1)
                {
                    p_iir->ff.p_den[j] = \
                    OPMSG_AHM_SET_COEFFS32(p_msg,denom_coeffs_start_offset + i);
                    coeffs[j] = (p_iir->ff).p_den[j];
                    coeffs_prev[j] = (p_prev_iir->ff).p_den[j];
                    L5_DBG_MSG3("OPID: %x, ANC FFA DENOM IIR COEFF NUM [%d] = 0x%08X",
                                 ext_op_id, j, (p_iir->ff).p_den[j]);
                }
                for (i = 0, j = 0; i < 2 * numer_taps; i += 2, j += 1)
                {
                    p_iir->ff.p_num[j] = \
                    OPMSG_AHM_SET_COEFFS32(p_msg,num_coeffs_start_offset + i);
                    coeffs[j + denom_taps] = (p_iir->ff).p_num[j];
                    coeffs_prev[j + denom_taps] = (p_prev_iir->ff).p_num[j];
                    L5_DBG_MSG3("OPID: %x, ANC FFA NUMER IIR COEFF NUM [%d] = 0x%08X",
                                ext_op_id, j, (p_iir->ff).p_num[j]);
                }
        
                if (!check_filters_equal(coeffs_prev, coeffs, no_of_taps))
                {
                    cfg[AHM_ANC_FILTER_FF_ID] = TRUE;
                    L5_DBG_MSG1("OPID: %x, Mismatch between cur and prev filters in FF path",
                                ext_op_id);
                    #ifndef RUNNING_ON_KALSIM
                        stream_anc_set_anc_iir_foreground_coeffs(
                        (STREAM_ANC_INSTANCE) instance, (STREAM_ANC_PATH) path,
                        denom_taps + numer_taps, coeffs);
                    #endif
                    /* copy previous iir ff filters */
                    for (i = 0, j = 0; i < 2 * denom_taps; i += 2, j += 1)
                    {
                        (p_prev_iir->ff).p_den[j] = (p_iir->ff).p_den[j];
                    }
                    for (i = 0, j = 0; i < 2 * numer_taps; i += 2, j += 1)
                    {
                        (p_prev_iir->ff).p_num[j] = (p_iir->ff).p_num[j];
                    }
                }
                else
                {
                    L5_DBG_MSG1("OPID: %x, Current and previous filters in FF path are same",
                                ext_op_id);
                }
             break;

             default:
                L2_DBG_MSG1("OPID: %x, Invalid filter path", ext_op_id);
                UNUSED(coeffs);
        }
    }
    else
    {
        L2_DBG_MSG1("OPID: %x, Unsupported configuration", ext_op_id);
    }
}
