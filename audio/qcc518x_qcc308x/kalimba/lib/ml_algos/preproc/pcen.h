/****************************************************************************
* Copyright (c) 2023 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  pcen.h
 * \ingroup  ml_algos\preproc
 *
 * Header file for Per-Channel Energy Normalisation (PCEN) preprocessing
 *
 */

#ifndef PCEN_H
#define PCEN_H

/****************************************************************************
Include Files
*/
#include <stdfix.h>

typedef struct tPcenStruct
{
    /* Number of spectral channels */
    unsigned num_spectral_channels;
    /* Coefficient of the temporal IIR filter. 32 bit value expressed in Q31
     * It is computed as b = sqrt((1+4*T^2)-1)/(2*T^2);
     * Constant T is computed as:T = time_constant * sr / hop_length
     * where
     * time_constant: number > 0: the time constant for IIR filtering. Default: 0.4
     * sr: number > 0: sampling rate of the input audio
     * hop_length: number > 0: hop length used while framing
     * */
    unsigned b_q31;
    /* The gain factor. Positive number with typical values slightly less than 1
     * This factor is expressed in Q31
     */
    unsigned gain_q31;
    /* Bias point of the non linear compression. Accepted values are either 0,1 or 2
     * This is an integer value.
     */
    unsigned bias;
    /* The compression exponent.  Typical values should be between 0 and 0.5.
     * Smaller values of 'power' result in stronger compression.
     * At the limit 'power=0', polynomial compression becomes logarithmic
     * This factor is expressed in Q1.31
     */
    unsigned power_q31;
    /* bias ^ power_q31 in Q3.29 */
    unsigned bias_raised_by_power_q29;
    /* Internal delay line used by the IIR filter */
    int *delay_line;
    /* Internal buffer pointer for input spectrum */
    int *spec;
    /* Internal buffer pointer for the filtered spectrum */
    int *filtered_spec;
    /* Internal buffer pointer for the agc spectrum */
    int *agc_spec;
    /* Internal buffer pointer for the drc spectrum */
    int *drc_spec;
}tPcenStruct;

/****************************************************************************
Interface functions
*/
/**
 * \brief Function to get the size of memory required for PCEN
 * 
 * \param num_spectral_channels: Number of spectral channels in the input
 * \return size of memory required for buffers
*/
unsigned pcen_get_size(unsigned num_spectral_channels);
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
 * \return TRUE if successful, FALSE otherwise
 */
int pcen_create(tPcenStruct *pcen_data,
                 unsigned b_q31,
                 unsigned gain_q31,
                 unsigned bias,
                 unsigned power_q31,
                 unsigned num_spectral_channels,
                 int *buffer);

/**
 * \brief Function for pcen application.
 *
 * \param handle to the pcen data structure
 * \param pointer to the input data
 * \param pointer to the output data
 */
void pcen_update(tPcenStruct *pcen_data,
                 int *input,
                 int *output);

/****************************************************************************
Utility functions
*/
void pcen_temporal_smoothing(tPcenStruct *pcen_data);
void pcen_adaptive_gain_control(tPcenStruct *pcen_data);
void pcen_dynamic_range_compression(tPcenStruct *pcen_data);

#endif /* PCEN_H */
