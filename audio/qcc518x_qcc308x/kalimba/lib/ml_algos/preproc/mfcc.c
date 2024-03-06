/****************************************************************************
 * Copyright (c) 2023 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  mfcc.c
 * \ingroup  ml_algos\preproc
 *
 * Source file for Mel frequency Cepstral Coefficient (MFCC) preprocessing
 *
 */

 /****************************************************************************
 Include Files
 */
#include "mfcc.h"
#include "preproc_common.h"
#include "math_lib.h"
#include "mfcc_default_tables.h"
#include "audio_log/audio_log.h"

 /****************************************************************************
 Utility functions
 */

asm int get_fft_exp(int x)
{
    r0 = @{x};
    r1 = SIGNDET r0;
    r0 = 30 - r1;
    @{} = r0;
}

 /**
  * \brief Inline function to copy input data into the FFT input buffers.
  *
  * \param mfcc_data: MFCC data structure.
  * \param input: Input buffer.
  */
static inline void mfcc_copy_input_data(tMfccStruct* mfcc_data, int* input)
{
    unsigned fft_len = mfcc_data->n_fft;
    /* Copy data from input to real input*/
    memcpy(mfcc_data->internal_buffers[INT_BUFF_INDEX_ZERO],
           input,
           fft_len * sizeof(int));
    /* Fill imaginary input with zeroes */
    memset(mfcc_data->internal_buffers[INT_BUFF_INDEX_ONE],
           0,
           fft_len * sizeof(int));
}

/**
 * \brief Inline function to copy output data into the output buffers.
 *
 * \param mfcc_data: MFCC data structure.
 * \param output: Output buffer.
 */
static inline void mfcc_copy_output_data(tMfccStruct* mfcc_data, int* output)
{
    unsigned n_mfcc = mfcc_data->n_mfcc;
    /* MFCC Coefficients are present in internal_buffers at index 0
     * and in format Q8.24. Copy them to the output buffer.
     */
    memcpy(output,
           mfcc_data->internal_buffers[INT_BUFF_INDEX_ZERO],
           n_mfcc * sizeof(int));
}

/**
 * \brief Function to update the scratch buffers required by MFCC.
 *
 * \param mfcc_data: MFCC data structure.
 * \param dm1_scratch: DM1 scratch buffer.
 * \param dm2_scratch: DM2 scratch buffer.
 */
void mfcc_update_scratch_buffers(tMfccStruct *mfcc_data, int *dm1_scratch, int *dm2_scratch)
{
    mfcc_data->internal_buffers[INT_BUFF_INDEX_ZERO] = dm1_scratch;
    mfcc_data->internal_buffers[INT_BUFF_INDEX_ONE] = dm2_scratch;
    mfcc_data->internal_buffers[INT_BUFF_INDEX_TWO] = dm2_scratch + mfcc_data->n_fft + 4;
    /* internal_buffers[INT_BUFF_INDEX_ZERO] act as the real input and
     * internal_buffers[INT_BUFF_INDEX_ONE] act as the imaginary input
     */
    mfcc_data->pFft.real = mfcc_data->internal_buffers[INT_BUFF_INDEX_ZERO];
    mfcc_data->pFft.imag = mfcc_data->internal_buffers[INT_BUFF_INDEX_ONE];
}

/**
 * \brief Function to compute the input spectrum.
 *
 * \param mfcc_data: MFCC data structure.
 * 
 */
void mfcc_compute_spectrum(tMfccStruct* mfcc_data)
{
    unsigned r_fft_len = (mfcc_data->n_fft >> 1) + 1;
    int* real, * imag, *fft_output;

    ml_fft_scaled(&mfcc_data->pFft);
    ml_bitreverse_array(mfcc_data->internal_buffers[INT_BUFF_INDEX_ZERO], mfcc_data->internal_buffers[INT_BUFF_INDEX_TWO], mfcc_data->pFft.numPoints);
    ml_bitreverse_array(mfcc_data->internal_buffers[INT_BUFF_INDEX_ONE], mfcc_data->internal_buffers[INT_BUFF_INDEX_ZERO], mfcc_data->pFft.numPoints);

    /* post fft computation, the real part is in internal_buffers[INT_BUFF_INDEX_TWO] and the imaginary part is in internal_buffers[INT_BUFF_INDEX_ZERO]*/
    real = mfcc_data->internal_buffers[INT_BUFF_INDEX_TWO];
    imag = mfcc_data->internal_buffers[INT_BUFF_INDEX_ZERO];
    fft_output = mfcc_data->internal_buffers[INT_BUFF_INDEX_ONE];

    /* interleave the real and imaginary data and copy to the internal_buffers[INT_BUFF_INDEX_ONE] */
    for (unsigned i = 0, j = 0; i < r_fft_len; i++, j += 2)
    {
        fft_output[j] = real[i];
        fft_output[j + 1] = imag[i];
    }

    // Zero out the DC components
    fft_output[INT_BUFF_INDEX_ZERO] = 0;
    fft_output[INT_BUFF_INDEX_ONE] = 0;
}

/**
 * \brief Function to compute the input magnitude spectrum.
 *
 * \param mfcc_data: MFCC data structure.
 *
 */
void mfcc_compute_mag_spectrum(tMfccStruct* mfcc_data)
{
    /* Spectrum is interleaved format in the internal_buffer at index 1 */
    unsigned r_fft_len = (mfcc_data->n_fft >> 1) + 1;
    int *fft_output = mfcc_data->internal_buffers[INT_BUFF_INDEX_ONE];
    int* mag_spec_output = mfcc_data->internal_buffers[INT_BUFF_INDEX_ZERO];
    sat fract real, imag, temp;
    accum temp_acc = 0k;
    for (unsigned i = 0, j = 0; i < r_fft_len * 2; i+=2,j++)
    {
        real = __builtin_reinterpret_int_to_fract(fft_output[i]);
        imag = __builtin_reinterpret_int_to_fract(fft_output[i + 1]);
        temp_acc = real * real;
        temp_acc += imag * imag;
        temp = (sat fract)temp_acc;
        mag_spec_output[j] = __builtin_reinterpret_fract_to_int(temp);
    }
}

/**
 * \brief Function to compute the MEL/LOG-MEL spectrum.
 *
 * \param mfcc_data: MFCC data structure.
 * \param log_required: Flag indicating if log mel is required.
 *
 */
void mfcc_compute_mel_spectrum(tMfccStruct* mfcc_data, bool log_required)
{
    unsigned n_mel_banks = mfcc_data->n_mel_banks;
    int* mag_spec_output = mfcc_data->internal_buffers[INT_BUFF_INDEX_ZERO];
    int* mel_spec_output = mfcc_data->internal_buffers[INT_BUFF_INDEX_TWO];
    unsigned start_idx, end_idx, coeff_index = 0;
    accum temp_acc;
    sat fract one_by_log2_10 = 0.3010299956639812r; /* Q1.31 */
    sat fract temp, temp_1;
    int fft_exp;
    for (unsigned i = 0, j=0; i < n_mel_banks; i++,j+=2)
    {
        start_idx = mfcc_data->mel_bank_index[j];
        end_idx = start_idx + mfcc_data->mel_bank_index[j + 1];
        temp_acc = 0k;
        for (unsigned k = start_idx; k < end_idx; k++)
        {
            temp_acc += __builtin_reinterpret_int_to_fract(mag_spec_output[k]) *
                        mfcc_data->mel_bank_coeff[coeff_index];
            coeff_index++;
        }
        mel_spec_output[i] = __builtin_reinterpret_accum_to_int(temp_acc);
    }
    if (log_required)
    {
        for (unsigned i = 0; i < n_mel_banks; i++)
        {
            /* Take the input and reinterpret as an accum. temp_acc has the input in
             * Q9.63
             */
            temp_acc = __builtin_reinterpret_int_to_accum_se(mel_spec_output[i]);
            /* Input in Q9.63, output is in Q8.24 */
            temp = ml_log2(temp_acc);
            temp = temp * one_by_log2_10;/* Q8.24 * Q1.31 = Q8.24 */
            /* Adjust for the scale factor in FFT */
            fft_exp = mfcc_data->fft_exp << 25; /* Q8.24 - once extra shift since we need the square*/
            temp_1 = __builtin_reinterpret_int_to_fract(fft_exp) * one_by_log2_10; /* Q8.24 * Q1.31 = Q8.24 */
            temp += temp_1; /* In Q8.24 */
            mel_spec_output[i] = __builtin_reinterpret_fract_to_int(temp) * 10;
        }
    }
}

/**
 * \brief Function to compute the DCT.
 *
 * \param mfcc_data: MFCC data structure.
 *
 */
void mfcc_compute_dct(tMfccStruct* mfcc_data)
{
    /* Here, we compute Type-II DCT with ortho norm. The formulae
     * for computing is: Y_k = 2 * scale * sum(X_n * cos(pi * k * (2n+1)/2N)) for
     * n=0 till N-1.
     * k=0 till K-1.
     * where:
     * scale = sqrt(1/4N) for k=0
     *         sqrt(1/2N) otherwise
     * 
     * For MFCC, since DCT is taken on the log Mel Spectrogram and the result gives us
     * MFCCoefficients, N here translates into the number of mel filterbanks and K translates
     * into the number of required MFC Coeffients.
     * In this implementation, the DCT table stores scale * (cos(pi * k * (2n+1)/2N)).
     * where N = total number of Mel Bands
     *       K = Number of MFCC Coefficients.
     * This makes the size of the DCT Table as [#MFCC_COEFFICIENTS * #MEL_BANDS]
     */
    /* Log Mel Spectrogram in Q8.24 */
    int* mel_spec_output = mfcc_data->internal_buffers[INT_BUFF_INDEX_TWO];
    int* dct_output = mfcc_data->internal_buffers[INT_BUFF_INDEX_ZERO];
    unsigned num_mfcc_coeff = mfcc_data->n_mfcc;
    unsigned n_mel_banks = mfcc_data->n_mel_banks;
    unsigned coeff_index = 0;
    accum temp_acc;
    for (unsigned i = 0; i < num_mfcc_coeff; i++)
    {
        temp_acc = 0k;
        for (unsigned j = 0; j < n_mel_banks; j++)
        {
            temp_acc += __builtin_reinterpret_int_to_fract(mel_spec_output[j]) *
                        mfcc_data->dct_table[coeff_index]; /* (Q8.24 * Q1.31) >> 31 = Q8.24 */
            coeff_index++;
        }
        /* Multiply by 2 to get the DCT Output. Output is still in Q8.24 */
        dct_output[i] = __builtin_reinterpret_accum_to_int(temp_acc) << 1;
    }
}

/**
 * \brief Function to get the memory requirements for mfcc in bytes.
 *
 * \param n_fft: Number of FFT points (Should be power of 2 upto 1024)
 * \param dm1_buffer: Pointer to get the DM1 Scratch buffer requirement.
 * \param dm2_buffer: Pointer to get the DM2 Scratch buffer requirement.
 */
void mfcc_get_size(unsigned n_fft, unsigned *dm1_buffer, unsigned *dm2_buffer)
{
    /* For mfcc computation, we require 3 buffers of size n_fft+4 */
    unsigned dm1_scratch_size = (n_fft + 4) * sizeof(int);
    unsigned dm2_scratch_size = dm1_scratch_size * 2;
    *dm1_buffer = dm1_scratch_size;
    *dm2_buffer = dm2_scratch_size;
}

/**
 * \brief Function for mfcc creation
 *
 * \param handle to the MFCC data structure
 * \param n_fft: Number of FFT points (Should be power of 2 upto 1024)
 * \param n_mel_banks: Number of Mel Filter Banks.
 * \param n_mfcc: Number of MFCC Coefficients.
 * \param mel_bank_index: Pointer to the Mel Bank Index Table.
 * \param mel_bank_coeff: Pointer to the Mel Bank Coeff Table.
 * \param dct_table: Pointer to the DCT-Type II Table.
 * 
 * \return True if instance of MFCC was successfully created. Else False.
 */
bool mfcc_create(tMfccStruct* mfcc_data,
                 unsigned   n_fft,
                 unsigned   n_mel_banks,
                 unsigned   n_mfcc,
                 const unsigned*  mel_bank_index,
                 const sat fract* mel_bank_coeff,
                 const sat fract* dct_table
)
{
    /* Parameter checking */
    /* Do not proceed if the:
     * fft length is more than MAX_FFT_LEN OR
     * if n_mel_banks > n_fft
     * n_mfcc > n_mel_banks
     */
    if ((n_fft > MAX_FFT_LEN) ||
        (n_mel_banks > n_fft) ||
        (n_mfcc > n_mel_banks))
    {
        L0_DBG_MSG3("MFCC: Incorrect parameter config. n_fft: %d\
                     n_mel_banks: %d, n_mfcc: %d",
                     n_fft, n_mel_banks, n_mfcc);
        return FALSE;
    }

    mfcc_data->n_fft = n_fft;
    mfcc_data->n_mel_banks = n_mel_banks;
    mfcc_data->pFft.numPoints = n_fft;
    mfcc_data->n_mfcc = n_mfcc;

    /* Compute the FFT exponent */
    mfcc_data->fft_exp = get_fft_exp(n_fft);

    /* allocate twiddle factors */
    ml_twiddle_init(n_fft);

    /* Initialise the MelFilterbank. If mel_bank_index is NULL, then use
     * the default table which is for 32 filterbanks and a FFT size of
     * 512.
     */
    if (NULL == mel_bank_index)
    {
        mfcc_data->mel_bank_index = default_mel_bank_index;
        mfcc_data->mel_bank_coeff = default_mel_bank_coeff;
    }
    else
    {
        mfcc_data->mel_bank_index = mel_bank_index;
        mfcc_data->mel_bank_coeff = mel_bank_coeff;
    }

    /* Initialise the DCT table. If the dct_table argument is
     * NULL, use the default table which is for 13 MFCCoefficients
     * from 32 Mel Filterbanks. If the dct_table argument is not
     * NULL, then use that table
     */
    if (NULL == dct_table)
    {
        mfcc_data->dct_table = default_dct_table;
    }
    else
    {
        mfcc_data->dct_table = dct_table;
    }

    return TRUE;
}


/**
 * \brief Function for mfcc application.
 *
 * \param handle to the mfcc data structure
 * \param pointer to the input data
 * \param pointer to the output data
 * \param pointer to the DM1 scratch buffer
 * \param pointer to the DM2 scratch buffer
 */
void mfcc_update(tMfccStruct* mfcc_data,
                 int* input,
                 int* output,
                 int* dm1_scratch,
                 int* dm2_scratch
)
{
    mfcc_update_scratch_buffers(mfcc_data, dm1_scratch, dm2_scratch);

    mfcc_copy_input_data(mfcc_data, input);

    /* Part I - calculate the spectrum */
    mfcc_compute_spectrum(mfcc_data);

    /* Part II - calculate the magnitude spectrum */
    mfcc_compute_mag_spectrum(mfcc_data);

    /* Part III - calculate the mel spectrum */
    mfcc_compute_mel_spectrum(mfcc_data, TRUE);

    /* Part IV - calculate the DCT */
    mfcc_compute_dct(mfcc_data);

    mfcc_copy_output_data(mfcc_data, output);

}

/**
 * \brief Function for mfcc destroy and release memory
 *
 * \param handle to the mfcc data structure
 */
void mfcc_destroy(tMfccStruct* mfcc_data)
{
    ml_twiddle_release();
}