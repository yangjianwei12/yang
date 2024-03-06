/****************************************************************************
 * Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  cbops_sidetone_filter.c
 * \ingroup cbops
 *
 * This file contains functions for the sidetone filter cbops operator
 */

/****************************************************************************
Include Files
 */
#include "pmalloc/pl_malloc.h"
#include "cbops_c.h"
#include "cbops_sidetone_filter.h"
#include "platform/pl_fractional.h"
#include "peq_c.h"
#include "pl_timers/pl_timers.h"

/****************************************************************************
Public Function Definitions
*/

/*
 * create_sidetone_filter_op
 * It fits into multi-channel model, but it only ever actually uses single in/out channel.
 */
cbops_op* create_sidetone_filter_op(unsigned input_idx, unsigned output_idx,
                                          unsigned max_stages, cbops_sidetone_params *st_params,
                                          unsigned *peq_params)
{
    cbops_op *op;
    patch_fn_shared(cbops_lib);

    op = (cbops_op*)xzpmalloc(sizeof_cbops_op(cbops_sidetone_filter_op, 1, 1) +
                              SH_PEQ_OBJECT_SIZE(max_stages));
    if(op)
    {
        cbops_sidetone_filter_op  *params;
        t_peq_object *peq;
        
        op->function_vector    = cbops_sidetone_filter_table;

        /* Setup cbop param struct header info */
        params = (cbops_sidetone_filter_op*)cbops_populate_param_hdr(op, 1, 1, &input_idx, &output_idx);

        /* Set up the parameters - there are no channel-specific params as such, as it only works on a
         * single channel.
         */

        params->inv_dac_gain = FRACTIONAL(1.0); // TODO - handle gain adjustment for post volume
        peq = (t_peq_object *)params->peq;
        params->params = st_params;
        peq->max_stages = max_stages;
        peq->params_ptr = (t_peq_params *) peq_params;
    }

    return(op);
}

/**
 * set step for ramping of sidetone gain
 *
 * \param op sidetone filter cbops operator
 * \param exp_ramp exponential ramping parameter, it is the desired growth of sidetone gain in 10ms,
 *     pass FRACTIONAL(10**(dB_per_sec/2000.0) - 1.0), or 0 if no exponential ramping required.
 * \param lin_ramp linear ramping parameter, it is desired sidetone linear gain increase in 10ms,
 *     pass FRACTIONAL(full_scale_percent_per_sec/10000.0), or 0 if no linear ramping required.
 * \param run_period running period for filter in microseconds,
 *     Note: operator is expected to run at regular intervals and for accuracy of ramping speed
 *     this is not expected to exceed 10ms.
 */
void cbops_sidetone_filter_set_ramping_step(cbops_op *op, unsigned exp_ramp, unsigned lin_ramp, unsigned run_period)
{
    cbops_sidetone_filter_op  *params;
    unsigned run_period_frac;

    patch_fn_shared(cbops_lib);
    if(NULL == op)
    {
        return;
    }

    /* get params */
    params = CBOPS_PARAM_PTR(op, cbops_sidetone_filter_op);

    /* parameters are for 10ms, scale them for run_period, assuming run_period<=10ms */
    run_period_frac = frac_div(run_period, 10*MILLISECOND);

    /* scale down exponential param,
     * Note: function f(x) = (10**(x/2000.0) - 1.0) is very
     *       linear for the rang of x that we use
     */
    params->exp_ramping_step = frac_mult(exp_ramp, run_period_frac);

    /* scale down linear param, extra 1.0/256 since gains are stored in Q9.x internally by this operator */
    params->lin_ramping_step = frac_mult(lin_ramp, frac_mult(run_period_frac, FRACTIONAL(1.0/256)));
}
