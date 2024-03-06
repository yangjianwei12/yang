/****************************************************************************
* Copyright (c) 2023 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  mfcc.h
 * \ingroup  ml_algos\preproc
 *
 * Header file for Mel frequency Cepstral Coefficient (MFCC) preprocessing
 *
 */

#ifndef MFCC_H
#define MFCC_H

 /****************************************************************************
 Include Files
 */
#include <stdfix.h>
#include "math_library_c_stubs.h"
#include "types.h"

/* Max FFT currently supported by the library is 1024 */
#define MAX_FFT_LEN 1024

/* Number of internal buffers being used and their indices */
#define INT_BUFF_INDEX_ZERO 0
#define INT_BUFF_INDEX_ONE  1
#define INT_BUFF_INDEX_TWO  2
#define NUM_INTERNAL_BUFF   3

typedef struct tMfccStruct
{
    unsigned n_fft;
    unsigned n_mel_banks;
    unsigned fft_exp;
    unsigned n_mfcc;
    tFFTStruct pFft;
    int* internal_buffers[NUM_INTERNAL_BUFF];
    /* Index for the Mel FilterBank */
    const unsigned *mel_bank_index;
    /* Number of Mel FilterBank Coefficients */
    unsigned len_mel_bank_coeff;
    /* Filter Coefficient for the Mel FilterBank */
    const sat fract *mel_bank_coeff;
    /* DCT Table, size will be n_mfcc * n_mel_banks in Q31 */
    const sat fract *dct_table;
}tMfccStruct;

/****************************************************************************
Interface functions
*/

/**
 * \brief Function to get the memory requirements for mfcc in bytes.
 *
 * \param n_fft: Number of FFT points (Should be power of 2 upto 1024)
 * \param dm1_buffer: Pointer to get the DM1 Scratch buffer requirement.
 * \param dm2_buffer: Pointer to get the DM2 Scratch buffer requirement.
 */
void mfcc_get_size(unsigned n_fft, unsigned* dm1_buffer, unsigned* dm2_buffer);

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
bool mfcc_create(tMfccStruct *mfcc_data,
                 unsigned n_fft,
                 unsigned n_mel_banks,
                 unsigned n_mfcc,
                 const unsigned *mel_bank_index,
                 const sat fract *mel_bank_coeff,
                 const sat fract *dct_table
                );

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
                );

/**
 * \brief Function for mfcc destroy and release memory
 *
 * \param handle to the mfcc data structure
 */
void mfcc_destroy(tMfccStruct* mfcc_data);

/****************************************************************************
Utility functions
*/
void mfcc_compute_spectrum(tMfccStruct* mfcc_data);
void mfcc_compute_mag_spectrum(tMfccStruct* mfcc_data);
void mfcc_compute_mel_spectrum(tMfccStruct* mfcc_data, bool log_required);
void mfcc_compute_dct(tMfccStruct* mfcc_data);
void mfcc_update_scratch_buffers(tMfccStruct* mfcc_data, int* dm1_scratch, int* dm2_scratch);


#endif /* MFCC_H */
