/****************************************************************************
 * Copyright (c) 2023 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  pcen.c
 * \ingroup  ml_algos\preproc
 *
 * Source file for Per-Channel Energy Normalisation (PCEN) preprocessing
 *
 */

/****************************************************************************
Include Files
*/
#include "pcen.h"
#include "preproc_common.h"
#include <audio_log/audio_log.h>
#include "math_lib.h"
#include "vector_ops_fixed_kalimba.h"

/****************************************************************************
Utility functions
*/
asm int fractional_div(int x, int y)
{
    r0 = @{x};
    r1 = @{y};
    call $_pl_fractional_divide;
    @{} = r0;
}

void pcen_temporal_smoothing(tPcenStruct *pcen_data)
{
    accum temp_acc;
    int alpha = pcen_data->b_q31;
    int one_minus_alpha = INT_MAX - alpha;
    int *delay_line = pcen_data->delay_line;

#pragma loop minitercount(1)
    for(unsigned i=0; i < pcen_data->num_spectral_channels; i++)
    {
        /* (alpha * input_buff) + ((1-alpha) * delay_line) */
        temp_acc =  __builtin_reinterpret_int_to_fract(alpha) *
                    __builtin_reinterpret_int_to_fract(pcen_data->spec[i]);
        temp_acc += __builtin_reinterpret_int_to_fract(one_minus_alpha) *
                    __builtin_reinterpret_int_to_fract(delay_line[i]);
        pcen_data->filtered_spec[i] = __builtin_reinterpret_accum_to_int(temp_acc);
        /* update the delay line */
        delay_line[i] = pcen_data->filtered_spec[i];
    }
}

/*
 * The equation for computing the AGC block is:
 * agc_spec = exp(-gain * (log(eps) + log1p(s / eps))) ---- (I)
 *
 * Simplifying the above equation for s > 0 with the assumption that
 * log1p(x) == log(x), we can rewrite the above equation as:
 * agc_spec = exp(-gain * (log(eps) + log(s) - log(eps))
 * agc_spec = exp(-gain * log(s)) ---- (II)
 *
 * For s == 0, equation (I) can be rewritten as
 * agc_spec = exp(-gain * (log(eps) + log(1+s/eps))
 * agc_spec = exp(-gain * (log(eps) + log(1)), since s==0
 * agc_spec = exp(-gain * log(eps)), since log(1) == 0
 * agc_spec = exp(-gain * -13.8155) ---- (III)
 *
 * Note: In this module, we are actually computing:
 * exp(gain * log(s)) for s > 0
 * exp(gain * -13.8155) for s == 0
 * In other words, we are computing 1/agc_spec and the result is in
 * Q1.31.
 */
void pcen_adaptive_gain_control(tPcenStruct *pcen_data)
{
    accum temp_acc;
    sat fract temp;
    int data;
    int exp_out;
    sat fract one_by_log2_e = 0.6931r; /* 1/log2(e) */
    accum log_epsilon = -13.8155k; /* log(1e-6) in Q9.63 */

#pragma loop minitercount(1)
    for(unsigned i=0; i < pcen_data->num_spectral_channels; i++)
    {
        data = pcen_data->filtered_spec[i];
        if(data == 0)
        {
            /* If input is 0, then output is log(epsilon) * gain */
            /* Q9.63 = Q9.63 * Q1.31 */
            temp_acc = log_epsilon * __builtin_reinterpret_int_to_fract(0-pcen_data->gain_q31);
            /* temp_acc in Q9.63, convert into int Q8.24
             * shift right by 7 to ensure that correct value lands up in
             * RMAC(B)1
             */
            data = __builtin_reinterpret_accum_to_int(temp_acc >> 7);
        }
        else
        {
            /* Take the input and reinterpret as an accum. temp_acc has the input in
             * Q9.63
             */
            temp_acc = __builtin_reinterpret_int_to_accum_se(data);
            /* Input in Q9.63, output is in Q8.24 */
            temp = ml_log2(temp_acc);
            temp = temp * one_by_log2_e;/* Q8.24 * Q1.31 = Q8.24 */
            /* Q8.24 * Q1.31 = Q8.24 */
            temp = temp * __builtin_reinterpret_int_to_fract(0-pcen_data->gain_q31);
            data = __builtin_reinterpret_fract_to_int(temp);
        }
        /* Convert data from Q8.24 into Q1.31. assumption is that the range of
         * values in data is in range [0, -16] */
        data = data << 3;
        vector_exp_kalimba(&exp_out, &data, 1, 1);
        pcen_data->agc_spec[i] = exp_out;
    }
}

void pcen_dynamic_range_compression(tPcenStruct *pcen_data)
{
    sat fract temp;
    accum temp_acc;
    accum one_in_q63 = 1k;
    sat fract one_by_log2_e = 0.6931r; /* 1/log2(e) */
    int spec, inv_agc_spec, data;
    int exp_out;
    int one_in_q29 = 536870912; /* 1 in Q3.29 */
    if(pcen_data->power_q31 == 0)
    {
        /*  The equation for computing the drc_spec when power is 0:
         *  drc_spec = log1p(agc_spec .* spec);
         *  Since we have 1/agc_spec from the AGC block, we can rewrite the
         *  above equation as: drc_spec = log1p(spec/(inv_agc_spec))
         *  drc_spec = log(1+spec/inv_agc_spec)
         *  Since inv_agc_spec > spec, we can use fractional division
         */
#pragma loop minitercount(1)
        for(unsigned i=0; i < pcen_data->num_spectral_channels; i++)
        {
            /* Step 1: Compute spec/(1/agc_spec) */
            spec = pcen_data->spec[i]; /* spec in Q1.31 */
            inv_agc_spec = pcen_data->agc_spec[i]; /* 1/agc_spec in Q1.31 */
            data = fractional_div(spec, inv_agc_spec);
            /* Take tmp and reinterpret it as accum. It will be in Q9.63 */
            temp_acc = __builtin_reinterpret_int_to_accum_se(data);
            temp_acc += one_in_q63;
            /* Input in Q9.63, output is in Q8.24 */
            temp = ml_log2(temp_acc);
            temp = temp * one_by_log2_e;/* Q8.24 * Q1.31 = Q8.24 */
            data = __builtin_reinterpret_fract_to_int(temp);
            pcen_data->drc_spec[i] = data;
        }
    }
    else if(pcen_data->bias == 0)
    {
        /* The equation for computing the drc_spec when bias is 0:
         * drc_spec = exp(obj.power * (log(spec) + log(agc_spec)));
         * We have 1/agc_spec from the AGC block, hence the above equation
         * can be rewritten as:
         */
#pragma loop minitercount(1)
        for(unsigned i=0; i<pcen_data->num_spectral_channels; i++)
        {
            /* Step1: Compute data = spec/(1/agc_spec) */
            spec = pcen_data->spec[i]; /* spec in Q1.31 */
            inv_agc_spec = pcen_data->agc_spec[i]; /* 1/agc_spec in Q1.31 */
            data = fractional_div(spec, inv_agc_spec);

            /* Step2: Take temp = log(data) */
            /* Take data and reinterpret it as accum. It will be in Q9.63 */
            temp_acc = __builtin_reinterpret_int_to_accum_se(data);
            /* Input in Q9.63, output is in Q8.24 */
            temp = ml_log2(temp_acc);
            temp = temp * one_by_log2_e;/* Q8.24 * Q1.31 = Q8.24 */

            /* Step3: Multiply by power Q8.24 = Q8.24 * Q1.31*/
            temp = temp * __builtin_reinterpret_int_to_fract(pcen_data->power_q31);
            data = __builtin_reinterpret_fract_to_int(temp);

            /* Calculate exp() */
            /* Convert data from Q8.24 into Q1.31. assumption is that the range of
             * values in data is in range [0, -16] */
            data = data << 3;
            data = 0-data;
            vector_exp_kalimba(&exp_out, &data, 1, 1);
            exp_out = exp_out >> 7; /* Q1.31 -> Q8.24 */
            pcen_data->drc_spec[i] = exp_out;
        }
    }
    else
    {
        /* The equation for computing the drc_spec when neither bias is 0, nor
         * power is 0:
         * drc_spec = (bias^power) * expm1(power * log1p(spec .* agc_spec / bias));
         */
#pragma loop minitercount(1)
        for (unsigned i = 0; i < pcen_data->num_spectral_channels; i++)
        {
            /* Step 1: Compute data = (spec * agc_spec)/bias */
            spec = pcen_data->spec[i]; /* spec in Q1.31 */
            inv_agc_spec = pcen_data->agc_spec[i]; /* 1/agc_spec in Q1.31 */
            data = fractional_div(spec, inv_agc_spec);
            /* Divide by bias only if bias is 2 */
            if (pcen_data->bias_raised_by_power_q29 != 0)
            {
                data = data >> 1; /* Q1.31 */
            }

            /* Step 2: Compute log1p(spec .* agc_spec/bias) */
            temp_acc = __builtin_reinterpret_int_to_accum_se(data);
            temp_acc += one_in_q63;
            /* Input in Q9.63, output is in Q8.24 */
            temp = ml_log2(temp_acc);
            temp = temp * one_by_log2_e;/* Q8.24 * Q1.31 = Q8.24 */

            /* Step 3: Compute power * log1p(spec .* agc_spec/bias) */
            temp = temp * __builtin_reinterpret_int_to_fract(pcen_data->power_q31); /* Q8.24 * Q1.31 = Q8.24 */
            data = __builtin_reinterpret_fract_to_int(temp);

            /* Step 4: Compute exp(power * log1p(spec .* agc_spec/bias)) - 1 */
            /* Convert data from Q8.24 into Q1.31. Assumption is that the range
             * is within the kalimba sat fract range.
             */
            data = data << 7;
            /* Input in Q1.31, Output in Q3.29 */
            exp_out = exp_kalimba_positive(data);
            /* Subtract 1 */
            data = exp_out - one_in_q29; /* Result in Q3.29 */

            /* Step 5: Multiply result by (obj.bias^obj.power) */
            if (pcen_data->bias_raised_by_power_q29 != 0)
            {
                /* (Q3.29 * Q3.29) >> 31 = Q5.27 */
                data = __builtin_reinterpret_fract_to_int(__builtin_reinterpret_int_to_fract(data) * \
                                                          __builtin_reinterpret_int_to_fract(pcen_data->bias_raised_by_power_q29));
                data = data >> 3; /* Q5.27 >> 3 = Q8.24 */
            }
            else
            {
                data = data >> 5; /* Q3.29 >> 5 = Q8.24 */
            }
            pcen_data->drc_spec[i] = data;
        }
    }
}

/****************************************************************************
Interface functions
*/

/**
 * \brief Function to get the size of memory required for PCEN
 *
 * \param num_spectral_channels: Number of spectral channels in the input
 * \return size of memory required for buffers
*/
unsigned pcen_get_size(unsigned num_spectral_channels)
{
    /* memory required for delay_line, buffers for - filtered_spec and agc_spec, and drc_spec */
    return num_spectral_channels * sizeof(int) * 3;
}

/**
 * \brief Function for pcen creation
 *
 * \param handle to the PCEN data structure
 * \param b_q31: Coefficient of the temporal IIR filter
 * \param gain_q31: Gain to be applied
 * \param bias: the bias factor to be applied
 * \param power_q31: compression exponent
 * \param num_spectral_channels: Number of spectral channels in the input
 * \param buffer: pointer to the buffer
 *
 * \return TRUE if successful, FALSE otherwise
 */
int pcen_create(tPcenStruct *pcen_data,
                 unsigned b_q31,
                 unsigned gain_q31,
                 unsigned bias,
                 unsigned power_q31,
                 unsigned num_spectral_channels,
                 int *buffer)
{
    tPcenStruct *pPcen = (tPcenStruct*)pcen_data;
    if(NULL == pPcen)
    {
        return FALSE;
    }

    if((int)b_q31 < 0)
    {
        L0_DBG_MSG1("PCEN: Invalid parameter b = %d. Required a positive value", b_q31);
        return FALSE;
    }
    if((int)gain_q31 < 0)
    {
        L0_DBG_MSG1("PCEN: Invalid parameter gain = %d. Required a positive value", gain_q31);
        return FALSE;
    }
    if((int)power_q31 < 0)
    {
        L0_DBG_MSG1("PCEN: Invalid parameter power = %d. Required a positive value", power_q31);
        return FALSE;
    }
    if((int)num_spectral_channels < 0)
    {
        L0_DBG_MSG1("PCEN: Invalid parameter num_spectral_channels = %d. Required a positive value", num_spectral_channels);
        return FALSE;
    }
    if(!(bias == 0 || bias == 1 || bias == 2))
    {
        L0_DBG_MSG1("PCEN: Invalid parameter bias = %d. Accepted values 0, 1 or 2", bias);
        return FALSE;
    }

    pPcen->b_q31 = b_q31;
    pPcen->gain_q31 = gain_q31;
    pPcen->bias = bias;
    pPcen->power_q31 = power_q31;
    pPcen->num_spectral_channels = num_spectral_channels;
    pPcen->delay_line = (int *)buffer;

    for(unsigned i=0; i<num_spectral_channels; i++)
    {
        /* Initialse delay line to 1 in Q0.31 */
        pPcen->delay_line[i] = INT_MAX;
    }
    /* Temporary buffer to be used for filtered spec and agc spec */
    int *buffer_fs_as = (int *)(buffer + num_spectral_channels);

    /* Temporary buffer to be used for drc spec */
    int *buffer_ds = (int *)(buffer + 2 * num_spectral_channels);

    pPcen->filtered_spec = buffer_fs_as;
    pPcen->agc_spec = buffer_fs_as;
    pPcen->drc_spec = buffer_ds;

    /* Compute bias^power for cases when bias is 2*/
    /* y = 2^power
     * y = exp(power * ln(2)) = exp(power*0.69314718056)
     */
    pPcen->bias_raised_by_power_q29 = 0;
    if (bias == 2)
    {
        sat fract log_e_2 = 0.69314718056r;
        sat fract temp = __builtin_reinterpret_int_to_fract(pPcen->power_q31) * log_e_2;
        pPcen->bias_raised_by_power_q29 = exp_kalimba_positive(__builtin_reinterpret_fract_to_int(temp));
    }
    return TRUE;
}

/**
 * \brief Function for pcen application.
 *
 * \param handle to the pcen data structure
 * \param pointer to the input data - should be a linear buffer
 * \param pointer to the output data - should be a linear buffer
 */
void pcen_update(tPcenStruct *pcen_data,
                 int *input,
                 int *output)
{
    pcen_data->spec = input;
    pcen_data->drc_spec = output;
    pcen_temporal_smoothing(pcen_data);
    pcen_adaptive_gain_control(pcen_data);
    pcen_dynamic_range_compression(pcen_data);
}
