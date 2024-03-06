/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  anc_hw_manager_gain.c
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) gain interface functions.
 *
 */

/****************************************************************************
Include Files
*/

#include "anc_hw_manager_gain.h"
#include "opmgr/opmgr_operator_data.h"

/******************************************************************************
Private Constant Definitions
*/

#define AHM_KALSIM_MSG_ID                       0xFFFF

typedef bool (*AHM_ANC_GAIN_FN)(AHM_ANC_INSTANCE instance,
                                AHM_ANC_PATH path,
                                uint16 gain);

/****************************************************************************
Public Function Definitions
*/

#ifdef RUNNING_ON_KALSIM
bool ahm_write_gain(AHM_ANC_CONFIG *p_config,
                    AHM_GAIN_BANK *p_gains,
                    AHM_GAIN_BANK *p_prev_gains,
                    OPERATOR_DATA *op_data)
{
    unsigned msg_size = OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER_WORD_SIZE;
    unsigned *trigger_message = NULL;
    OPMSG_REPLY_ID message_id;

    static unsigned prev_channel = INT_MAX;
    static unsigned prev_ff_path = INT_MAX;
    static unsigned prev_fb_path = INT_MAX;

    /* Don't send a message if the gains haven't changed */
    if ((p_gains->ff.coarse == p_prev_gains->ff.coarse) &&
        (p_gains->ff.fine == p_prev_gains->ff.fine) &&
        (p_gains->fb.coarse == p_prev_gains->fb.coarse) &&
        (p_gains->fb.fine == p_prev_gains->fb.fine) &&
        (p_gains->ec.coarse == p_prev_gains->ec.coarse) &&
        (p_gains->ec.fine == p_prev_gains->ec.fine) &&
        (p_gains->rx_ffa_mix.coarse == p_prev_gains->rx_ffa_mix.coarse) &&
        (p_gains->rx_ffa_mix.fine == p_prev_gains->rx_ffa_mix.fine) &&
        (p_gains->rx_ffb_mix.coarse == p_prev_gains->rx_ffb_mix.coarse) &&
        (p_gains->rx_ffb_mix.fine == p_prev_gains->rx_ffb_mix.fine) &&
        (prev_channel == p_config->channel) &&
        (prev_ff_path == p_config->ff_path) &&
        (prev_fb_path == p_config->fb_path))
    {
        return TRUE;
    }

    prev_channel = p_config->channel;
    prev_ff_path = p_config->ff_path;
    prev_fb_path = p_config->fb_path;

    trigger_message = xzpnewn(msg_size, unsigned);
    if (trigger_message == NULL)
    {
        return FALSE;
    }

    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                             ID,
                             AHM_KALSIM_MSG_ID);
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                             CHANNEL,
                             p_config->channel);
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                             FF_PATH,
                             p_config->ff_path);
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                             FB_PATH,
                             p_config->fb_path);
    p_prev_gains->ff = p_gains->ff;
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                             FF_COARSE,
                             p_gains->ff.coarse);
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                             FF_FINE,
                             p_gains->ff.fine);

    if (p_config->fb_path != AHM_ANC_PATH_NONE_ID)
    {
        /* Only update EC and FB gains if in hybrid mode */
        p_prev_gains->fb = p_gains->fb;
        OPMSG_CREATION_FIELD_SET(trigger_message,
                                 OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                                 FB_COARSE,
                                 p_gains->fb.coarse);
        OPMSG_CREATION_FIELD_SET(trigger_message,
                                 OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                                 FB_FINE,
                                 p_gains->fb.fine);
        p_prev_gains->ec = p_gains->ec;
        OPMSG_CREATION_FIELD_SET(trigger_message,
                                 OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                                 EC_COARSE,
                                 p_gains->ec.coarse);
        OPMSG_CREATION_FIELD_SET(trigger_message,
                                 OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                                 EC_FINE,
                                 p_gains->ec.fine);
    }

    p_prev_gains->rx_ffa_mix = p_gains->rx_ffa_mix;
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                             RX_FFA_MIX_COARSE,
                             p_gains->rx_ffa_mix.coarse);
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                             RX_FFA_MIX_FINE,
                             p_gains->rx_ffa_mix.fine);
    p_prev_gains->rx_ffb_mix = p_gains->rx_ffb_mix;
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                             RX_FFB_MIX_COARSE,
                             p_gains->rx_ffb_mix.coarse);
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                             RX_FFB_MIX_FINE,
                             p_gains->rx_ffb_mix.fine);
    OPMSG_CREATION_FIELD_SET32(trigger_message,
                               OPMSG_UNSOLICITED_AHM_KALSIM_EVENT_TRIGGER,
                               OPID,
                               INT_TO_EXT_OPID(op_data->id));
    message_id = OPMSG_REPLY_ID_AHM_KALSIM_EVENT_TRIGGER;

    L2_DBG_MSG1("AHM send msg at %d", time_get_time());
    common_send_unsolicited_message(op_data, (unsigned)message_id, msg_size,
                                    trigger_message);

    pdelete(trigger_message);

    return TRUE;
}
#else
/**
 * \brief  Write a single gain value to the ANC HW
 *
 * \param  gain     Gain value to write
 * \param  path     ANC path to update the gain on
 * \param  channel  ANC channel to update the gain on
 * \param  fn       Function to write the gain with
 *
 * \return  None
 *
 */
static void ahm_write_gain_value(uint16 gain,
                                 AHM_ANC_PATH path,
                                 AHM_ANC_INSTANCE channel,
                                 AHM_ANC_GAIN_FN fn)
{
    if (channel == AHM_ANC_INSTANCE_BOTH_ID)
    {
        fn(AHM_ANC_INSTANCE_ANC0_ID, path, gain);
        fn(AHM_ANC_INSTANCE_ANC1_ID, path, gain);
    }
    else
    {
        fn(channel, path, gain);
    }
}

/**
 * \brief  Update the gain in the ANC HW.
 * \param  p_config  Pointer to AHM config
 * \param  p_gains  Pointer to current gain bank
 * \param  p_prev_gains Pointer to previous gain bank
 *
 * \return  boolean indicating success or failure.
 *
 * Any changes in the gain value since the previous value was set is written
 * to the HW.
 *
 */
bool ahm_write_gain(AHM_ANC_CONFIG *p_config,
                    AHM_GAIN_BANK *p_gains,
                    AHM_GAIN_BANK *p_prev_gains)
{
    AHM_ANC_PATH anc_path;
    AHM_ANC_INSTANCE anc_channel = p_config->channel;

    /* Only update EC and FB gains if in hybrid mode */
    if (p_config->fb_path != AHM_ANC_PATH_NONE_ID)
    {
        /* Update EC gain */
        if (p_gains->ec.fine != p_prev_gains->ec.fine)
        {
            L5_DBG_MSG1("EC Fine: %d", p_gains->ec.fine);
            ahm_write_gain_value(p_gains->ec.fine,
                                 AHM_ANC_PATH_FB_ID,
                                 anc_channel,
                                 stream_anc_set_anc_foreground_fine_gain);
            p_prev_gains->ec.fine = p_gains->ec.fine;
        }
        if (p_gains->ec.coarse != p_prev_gains->ec.coarse)
        {
           L5_DBG_MSG1("EC Coarse: %d", p_gains->ec.coarse);
           ahm_write_gain_value((uint16)p_gains->ec.coarse,
                                 AHM_ANC_PATH_FB_ID,
                                 anc_channel,
                                 stream_anc_set_anc_coarse_gain);
            p_prev_gains->ec.coarse = p_gains->ec.coarse;
        }

        /* Update FB gain */
        anc_path = p_config->fb_path;
        if (p_gains->fb.fine != p_prev_gains->fb.fine)
        {
            L5_DBG_MSG1("FB Fine: %d", p_gains->fb.fine);
            ahm_write_gain_value(p_gains->fb.fine,
                                 anc_path,
                                 anc_channel,
                                 stream_anc_set_anc_foreground_fine_gain);
            p_prev_gains->fb.fine = p_gains->fb.fine;
        }
        if (p_gains->fb.coarse != p_prev_gains->fb.coarse)
        {
            L5_DBG_MSG1("FB Coarse: %d", p_gains->fb.coarse);
            ahm_write_gain_value(p_gains->fb.coarse,
                                 anc_path,
                                 anc_channel,
                                 stream_anc_set_anc_coarse_gain);
            p_prev_gains->fb.coarse = p_gains->fb.coarse;
        }
    }

    /* Update FF gain */
    anc_path = p_config->ff_path;
    if (p_gains->ff.fine != p_prev_gains->ff.fine)
    {
        L5_DBG_MSG1("FF Fine: %d", p_gains->ff.fine);
        ahm_write_gain_value(p_gains->ff.fine,
                             anc_path,
                             anc_channel,
                             stream_anc_set_anc_foreground_fine_gain);
        p_prev_gains->ff.fine = p_gains->ff.fine;
    }
    if (p_gains->ff.coarse != p_prev_gains->ff.coarse)
    {
       L5_DBG_MSG1("FF Coarse: %d", p_gains->ff.coarse);
       ahm_write_gain_value(p_gains->ff.coarse,
                             anc_path,
                             anc_channel,
                             stream_anc_set_anc_coarse_gain);
        p_prev_gains->ff.coarse = p_gains->ff.coarse;
    }

    if (p_gains->rx_ffa_mix.fine != p_prev_gains->rx_ffa_mix.fine)
    {
        L5_DBG_MSG1("FB RxMix Fine: %d", p_gains->rx_ffa_mix.fine);
        ahm_write_gain_value(p_gains->rx_ffa_mix.fine,
                             AHM_ANC_PATH_FFA_ID,
                             anc_channel,
                             stream_anc_set_anc_rx_mix_foreground_fine_gain);
        p_prev_gains->rx_ffa_mix.fine = p_gains->rx_ffa_mix.fine;
    }
    if (p_gains->rx_ffa_mix.coarse != p_prev_gains->rx_ffa_mix.coarse)
    {
        L5_DBG_MSG1("FB RxMix Coarse: %d", p_gains->rx_ffa_mix.coarse);
        ahm_write_gain_value(p_gains->rx_ffa_mix.coarse,
                             AHM_ANC_PATH_FFA_ID,
                             anc_channel,
                             stream_anc_set_anc_rx_mix_coarse_gain);
        p_prev_gains->rx_ffa_mix.coarse = p_gains->rx_ffa_mix.coarse;
    }
    if (p_gains->rx_ffb_mix.fine != p_prev_gains->rx_ffb_mix.fine)
    {
        L5_DBG_MSG1("FF RxMix Fine: %d", p_gains->rx_ffb_mix.fine);
        ahm_write_gain_value(p_gains->rx_ffb_mix.fine,
                             AHM_ANC_PATH_FFB_ID,
                             anc_channel,
                             stream_anc_set_anc_rx_mix_foreground_fine_gain);
        p_prev_gains->rx_ffb_mix.fine = p_gains->rx_ffb_mix.fine;
    }
    if (p_gains->rx_ffb_mix.coarse != p_prev_gains->rx_ffb_mix.coarse)
    {
       L5_DBG_MSG1("FF RxMix Coarse: %d", p_gains->rx_ffb_mix.coarse);
       ahm_write_gain_value(p_gains->rx_ffb_mix.coarse,
                             AHM_ANC_PATH_FFB_ID,
                             anc_channel,
                             stream_anc_set_anc_rx_mix_coarse_gain);
        p_prev_gains->rx_ffb_mix.coarse = p_gains->rx_ffb_mix.coarse;
    }
    return TRUE;
}
#endif /* RUNNING_ON_KALSIM */

void ahm_initialize_prev_gain(AHM_GAIN_BANK *p_prev_gains)
{
    p_prev_gains->ff.fine = AHM_INVALID_FINE_GAIN;
    p_prev_gains->ff.coarse = AHM_INVALID_COARSE_GAIN;
    p_prev_gains->fb.fine = AHM_INVALID_FINE_GAIN;
    p_prev_gains->fb.coarse = AHM_INVALID_COARSE_GAIN;
    p_prev_gains->ec.fine = AHM_INVALID_FINE_GAIN;
    p_prev_gains->ec.coarse = AHM_INVALID_COARSE_GAIN;
    p_prev_gains->rx_ffa_mix.fine = AHM_INVALID_FINE_GAIN;
    p_prev_gains->rx_ffa_mix.coarse = AHM_INVALID_COARSE_GAIN;
    p_prev_gains->rx_ffb_mix.fine = AHM_INVALID_FINE_GAIN;
    p_prev_gains->rx_ffb_mix.coarse = AHM_INVALID_COARSE_GAIN;

    return;
}

int ahm_get_gain_difference_db(uint16 fine_gain1,
                               int16 coarse_gain1,
                               uint16 fine_gain2,
                               int16 coarse_gain2)
{
    int gain1_db;
    int gain2_db;
    int gain_diff_db;

    gain1_db = ahm_calc_gain_db(fine_gain1, coarse_gain1);
    gain2_db = ahm_calc_gain_db(fine_gain2, coarse_gain2);
    gain_diff_db = gain1_db - gain2_db;

    return gain_diff_db;
}
