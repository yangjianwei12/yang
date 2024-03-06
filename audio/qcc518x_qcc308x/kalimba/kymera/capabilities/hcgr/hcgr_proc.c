/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  hcgr_proc.c
 * \ingroup hcgr
 *
 * Howling Control and Gain Recovery operator processing library
 *
 */

/****************************************************************************
Include Files
*/
#include "hcgr_proc.h"
/******************************************************************************
Private Function Definitions
*/

/******************************************************************************
Public Function Implementations
*/

void hcgr_init_filter_path(hcgr_t *p_hcgr, HCGR_PARAMETERS *p_params)
{
    if ((p_params->OFFSET_HCGR_CONFIG &
            HCGR_CONFIG_IS_FF_AND_FB_HCGR) > 0)
    {
        p_hcgr->hcgr_filter_path = AHM_ANC_FILTER_FF_AND_FB_ID;
    }
    else
    {
       p_hcgr->hcgr_filter_path = AHM_ANC_FILTER_FB_ID;
    }
    return;
}
void hcgr_proc_intialize(hcgr_t *p_hcgr, HCGR_PARAMETERS *p_params,
                         void *f_handle, unsigned sample_rate)
{
    /* Get pointers */
    HC100_DMX *p_hc;               /* Pointer to Howling control data */
    p_hc = p_hcgr->p_hc;

    /* Initialize the HC100 */
    int counter_table[] = {
        p_params->OFFSET_STEP_SIZE_0DB_FS,
        p_params->OFFSET_STEP_SIZE_6DB_FS,
        p_params->OFFSET_STEP_SIZE_12DB_FS,
        p_params->OFFSET_STEP_SIZE_18DB_FS,
        p_params->OFFSET_STEP_SIZE_24DB_FS,
        p_params->OFFSET_STEP_SIZE_30DB_FS,
        p_params->OFFSET_STEP_SIZE_36DB_FS,
        p_params->OFFSET_STEP_SIZE_42DB_FS,
    };

    p_hc->counter_table = counter_table;
    p_hc->counter_limit = p_params->OFFSET_COUNTER_TRIGGER;
    p_hc->ptpr_threshold = p_params->OFFSET_PTPR_THRESHOLD_MANT;
    p_hc->ptpr_threshold_bexp = p_params->OFFSET_PTPR_THRESHOLD_EXP;
    p_hc->ptpr_threshold_dc_bexp = \
        p_params->OFFSET_PTPR_THRESHOLD_DC_BEXP;
    p_hc->papr_threshold_shift = p_params->OFFSET_PAPR_THRESHOLD_SHIFT;
    p_hc->pnpr_threshold = p_params->OFFSET_PNPR_THRESHOLD;
    p_hc->ifpr_growth_scale = p_params->OFFSET_IFPR_GROWTH_SCALE;
    p_hc->bin1_trigger_detect_count = \
        p_params->OFFSET_BIN1_TRIGGER_DETECT_COUNT;
    p_hc->bin1_frame_reset_count = \
        p_params->OFFSET_BIN1_FRAME_RESET_COUNT;
    p_hc->counter_regular = p_params->OFFSET_COUNTER_REGULAR;
    p_hc->no_hc_below_bin_num = p_params->OFFSET_NOP_BELOW_BIN;

    aanc_hc100_initialize(f_handle, p_hc, p_hcgr->p_afb);

    p_hcgr->attack_tc = \
        aanc_utils_convert_time_to_tc(p_params->OFFSET_GAIN_ATTACK_TIME,
                                      p_hcgr->ahm_minimal_period_samples,
                                      sample_rate);
    p_hcgr->slow_recovery_thresh = \
        p_params->OFFSET_SLOW_RECOVERY_GAIN_THRESHOLD;

    /* Initialize latch values */
    p_hcgr->latch_max_bin = HCGR_RESET_VALUE;
    p_hcgr->latch_bexp = HCGR_RESET_VALUE;

    p_hcgr->minimum_fb_gain = p_params->OFFSET_MINIMUM_FB_GAIN;
    p_hcgr->fb_release_active = FALSE;

    p_hcgr->minimum_ff_gain = p_params->OFFSET_MINIMUM_FF_GAIN;
    p_hcgr->ff_release_active = FALSE;
    return;
}

bool hcgr_proc_create(hcgr_t *p_hcgr, HCGR_PARAMETERS *p_params, void **f_handle)
{
    /* Allocate AFB */
    p_hcgr->p_afb = xzpmalloc(aanc_afb_bytes());
    if (p_hcgr->p_afb == NULL)
    {
        L2_DBG_MSG1("OPID: %x, HCGR failed to allocate AFB on HCGR", p_hcgr->ext_op_id);
        return FALSE;
    }
    aanc_afb_create(p_hcgr->p_afb);

    /* Allocate HC Memory */
    p_hcgr->p_hc = (HC100_DMX*) xzppmalloc(aanc_hc100_dmx_bytes(),
                                           MALLOC_PREFERENCE_NONE);
    p_hcgr->p_hc_dm1 = (uint8*) xzppmalloc(aanc_hc100_dm1_bytes(),
                                           MALLOC_PREFERENCE_DM1);
    p_hcgr->p_hc_dm2 = (uint8*) xzppmalloc(aanc_hc100_dm2_bytes(),
                                           MALLOC_PREFERENCE_DM2);

    /* Create HC100 data structure  */
    aanc_hc100_create(p_hcgr->p_hc, p_hcgr->p_hc_dm1, p_hcgr->p_hc_dm2);

    hcgr_init_filter_path(p_hcgr, p_params);

    p_hcgr->ahm_minimal_period_samples = HCGR_DEFAULT_AHM_MINIMAL_PERIOD_SAMPLES;
    p_hcgr->gain_recovery_rate = p_params->OFFSET_GAIN_RECOVERY_RATE;

    if (!load_aanc_handle(f_handle))
    {
        L2_DBG_MSG1("OPID: %x, HCGR failed to load feature handle", p_hcgr->ext_op_id);
        return FALSE;
    }
    return TRUE;
}

void hcgr_proc_recovery(hcgr_t *p_hcgr, HCGR_PARAMETERS *p_params)
{
   HC100_DMX *p_hc = p_hcgr->p_hc;               /* Pointer to Howling control data */
   AHM_SHARED_FINE_GAIN *p_shared_fb_gain = p_hcgr->p_fb_fine_gain;
   AHM_SHARED_FINE_GAIN *p_shared_ff_gain = p_hcgr->p_ff_fine_gain;

   /* Recovery process on FB path */
   if (!p_hcgr->fb_recovery_active)
   {
       /* Activate on first howling detection */
       if (p_hc->tone_detected_flag)
       {
           p_hcgr->fb_recovery_active = TRUE;
           p_hcgr->recovery_active = TRUE;

           /* Latching happens only once and is independent of
              FF or FB path*/
           p_hcgr->latch_max_bin = (int)p_hc->max_bin;
           p_hcgr->latch_bexp = \
               *p_hcgr->p_afb->afb.freq_output_object_ptr->exp_ptr;
           p_hcgr->latch_peak_power = p_hc->peak_power;
           p_hcgr->latch_average_power_thresh = \
                p_hc->average_power_thresh;
           p_hcgr->latch_low_neighbour_pwr = p_hc->low_neighbour_pwr;
           p_hcgr->latch_high_neighbour_pwr = p_hc->high_neighbour_pwr;
           p_hcgr->latch_neighbour_peak_thresh = \
               p_hc->neighbour_peak_thresh;
           p_hcgr->latch_prev_det_scaled = p_hc->prev_det_scaled;

           if (p_hcgr->p_fb_nominal_gain != NULL)
           {
               p_hcgr->fb_hw_target_gain = p_hcgr->p_fb_nominal_gain->fine;
           }
           else
           {
               L5_DBG_MSG1("OPID: %x, HCGR Setting default target gain. Nominal \
                           gain not linked on FB path", p_hcgr->ext_op_id);
               p_hcgr->fb_hw_target_gain = HCGR_DEFAULT_TARGET_GAIN;
           }

           /* Store minimum delta gain. This will be used for
              subsequent HC detections */
           p_hcgr->min_fb_delta_gain = (p_hcgr->minimum_fb_gain << \
               HCGR_DIV_PRECISION ) / p_hcgr->fb_hw_target_gain;
           p_hcgr->min_fb_delta_gain = \
               p_hcgr->min_fb_delta_gain << HCGR_DIV_TO_DELTA_SHIFT;
           /* Update shared delta gain to target max reduction */
           p_shared_fb_gain->gain_delta = p_hcgr->min_fb_delta_gain;

           /* Calculate current gain w.r.t recovery target */
           p_shared_fb_gain->gain_current =
               (p_hcgr->p_fb_current_gain->fine << HCGR_DIV_PRECISION) / \
                   p_hcgr->fb_hw_target_gain;
           p_shared_fb_gain->gain_current =
               p_shared_fb_gain->gain_current << HCGR_DIV_TO_DELTA_SHIFT;

           p_shared_fb_gain->tc_attack = p_hcgr->attack_tc;
           p_shared_fb_gain->tc_release = HCGR_GAIN_RELEASE_TC;
       }
   }

   if (p_hcgr->fb_recovery_active)
   {
       if (!p_hc->tone_detected_flag)
       {
           /* Howling is mitigated at this point. Start FB gain recovery. */
           p_hcgr->fb_release_active = TRUE;
           if (p_shared_fb_gain->gain_current > p_hcgr->slow_recovery_thresh)
           {
               if (p_hcgr->fb_slowest_recovery == TRUE)
               {
                   p_hcgr->gain_recovery_rate = \
                       p_params->OFFSET_GAIN_RECOVERY_RATE_SLOWEST;
               }
               else
               {
                   p_hcgr->gain_recovery_rate = \
                       p_params->OFFSET_GAIN_RECOVERY_RATE_SLOW;
               }
           }
           else
           {
               p_hcgr->gain_recovery_rate = \
                   p_params->OFFSET_GAIN_RECOVERY_RATE;
           }
           p_shared_fb_gain->gain_delta = \
               aanc_utils_scale_gain(p_shared_fb_gain->gain_current,
                                     p_hcgr->gain_recovery_rate);

           if (p_shared_fb_gain->gain_current >= AHM_DELTA_NOMINAL)
           {
               /* FB gain is completely recovered at this point.
                  Recovery is complete on FB path */
               p_shared_fb_gain->gain_delta = AHM_DELTA_NOMINAL;
               p_hcgr->fb_recovery_active = FALSE;
           }
       }
       else
       {
           /* We are here either because howling is not yet mitigated OR
              there is a subsequent detection during recovery */
           p_shared_fb_gain->gain_delta = p_hcgr->min_fb_delta_gain;
           if (p_hcgr->fb_release_active == TRUE)
           {
               /* We are here because of subsequent detections
                  during recovery */
               p_hcgr->fb_slowest_recovery = TRUE;
           }
       }
   }
   /* Recovery process on FF path */
   if (p_hcgr->hcgr_filter_path == AHM_ANC_FILTER_FF_AND_FB_ID)
   {
       if (!p_hcgr->ff_recovery_active)
       {
           /* Activate on first howling detection */
           if (p_hc->tone_detected_flag)
           {
               p_hcgr->ff_recovery_active = TRUE;
               p_hcgr->recovery_active = TRUE;

               if (p_hcgr->p_ff_nominal_gain != NULL)
               {
                   p_hcgr->ff_hw_target_gain = p_hcgr->p_ff_nominal_gain->fine;
               }
               else
               {
                   L5_DBG_MSG1("OPID: %x, HCGR Setting default target gain. Nominal \
                               gain not linked for FF path", p_hcgr->ext_op_id);
                   p_hcgr->ff_hw_target_gain = HCGR_DEFAULT_TARGET_GAIN;
               }

               /* Store minimum delta gain. This will be used for
               subsequent HC detections */
               p_hcgr->min_ff_delta_gain = (p_hcgr->minimum_ff_gain << \
                   HCGR_DIV_PRECISION ) / p_hcgr->ff_hw_target_gain;
               p_hcgr->min_ff_delta_gain = \
                   p_hcgr->min_ff_delta_gain << HCGR_DIV_TO_DELTA_SHIFT;
               /* Update shared delta gain to target max reduction */
               p_shared_ff_gain->gain_delta = p_hcgr->min_ff_delta_gain;

               /* Calculate current gain w.r.t recovery target */
               p_shared_ff_gain->gain_current =
                   (p_hcgr->p_ff_current_gain->fine << HCGR_DIV_PRECISION) / \
                       p_hcgr->ff_hw_target_gain;
               p_shared_ff_gain->gain_current =
                   p_shared_ff_gain->gain_current << HCGR_DIV_TO_DELTA_SHIFT;

               p_shared_ff_gain->tc_attack = p_hcgr->attack_tc;
               p_shared_ff_gain->tc_release = HCGR_GAIN_RELEASE_TC;
           }
       }

       if (p_hcgr->ff_recovery_active)
       {
           if (!p_hc->tone_detected_flag)
           {
               /* Howling is mitigated at this point. Start FF gain recovery. */
               p_hcgr->ff_release_active = TRUE;
               if (p_shared_ff_gain->gain_current > p_hcgr->slow_recovery_thresh)
               {
                   if (p_hcgr->ff_slowest_recovery == TRUE)
                   {
                       p_hcgr->gain_recovery_rate = \
                           p_params->OFFSET_GAIN_RECOVERY_RATE_SLOWEST;
                   }
                   else
                   {
                       p_hcgr->gain_recovery_rate = \
                           p_params->OFFSET_GAIN_RECOVERY_RATE_SLOW;
                   }
               }
               else
               {
                   p_hcgr->gain_recovery_rate = \
                       p_params->OFFSET_GAIN_RECOVERY_RATE;
               }
               p_shared_ff_gain->gain_delta = \
                   aanc_utils_scale_gain(p_shared_ff_gain->gain_current,
                                       p_hcgr->gain_recovery_rate);

               if (p_shared_ff_gain->gain_current >= AHM_DELTA_NOMINAL)
               {
                   /* FF gain is completely recovered at this point.
                   Recovery is complete on FF path */
                   p_shared_ff_gain->gain_delta = AHM_DELTA_NOMINAL;
                   p_hcgr->ff_recovery_active = FALSE;
               }
           }
           else
           {
               /* We are here either because howling is not yet mitigated OR
               there is a subsequent detection during recovery */
               p_shared_ff_gain->gain_delta = p_hcgr->min_ff_delta_gain;
               if (p_hcgr->ff_release_active == TRUE)
               {
                   /* We are here because of subsequent detections
                   during recovery */
                   p_hcgr->ff_slowest_recovery = TRUE;
               }
           }
       }
   }

   if (p_hcgr->recovery_active == TRUE)
   {
       if ((p_hcgr->ff_recovery_active == FALSE) &&
           (p_hcgr->fb_recovery_active == FALSE))
       {
           /* Recovery on both paths complete */
           p_hcgr->recovery_active = FALSE;
           p_hcgr->fb_release_active = FALSE;
           p_hcgr->fb_slowest_recovery = FALSE;
           p_hcgr->ff_release_active = FALSE;
           p_hcgr->ff_slowest_recovery = FALSE;
           /* Release latched values */
           p_hcgr->latch_max_bin = HCGR_RESET_VALUE;
           p_hcgr->latch_bexp = HCGR_RESET_VALUE;
           p_hcgr->latch_peak_power = HCGR_RESET_VALUE;
           p_hcgr->latch_average_power_thresh = HCGR_RESET_VALUE;
           p_hcgr->latch_low_neighbour_pwr = HCGR_RESET_VALUE;
           p_hcgr->latch_high_neighbour_pwr = HCGR_RESET_VALUE;
           p_hcgr->latch_neighbour_peak_thresh = HCGR_RESET_VALUE;
           p_hcgr->latch_prev_det_scaled = HCGR_RESET_VALUE;
       }
   }
   return;
}
