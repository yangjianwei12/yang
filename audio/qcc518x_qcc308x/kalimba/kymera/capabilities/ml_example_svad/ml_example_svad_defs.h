/**
 * \file  ml_example_svad_defs.h
 * \ingroup capabilities
 *
 *
 */

#ifndef _ML_EXAMPLE_SVAD_DEFS_H
#define _ML_EXAMPLE_SVAD_DEFS_H

#define ML_EXAMPLE_SVAD_CAP_VERSION_MAJOR 1
#define ML_EXAMPLE_SVAD_CAP_VERSION_MINOR 0
              
#define NUM_INPUTS	1
#define NUM_OUTPUTS	1 

/* Block size of the SVAD capability is 40ms */
#define ML_EXAMPLE_SVAD_BLOCK_SIZE_SEC 0.040
/* Roll over size of the SVAD capability is 20ms */
#define ML_EXAMPLE_SVAD_ROLL_OVER_SIZE_SEC 0.020
/* Sample rate supported by the SVAD capability is 16000 Hz */
#define ML_EXAMPLE_SVAD_INPUT_SAMPLE_RATE  16000
/* output sample rate does not matter for ML_EXAMPLE_SVAD since the output is in form of a notification */
#define ML_EXAMPLE_SVAD_OUTPUT_SAMPLE_RATE 1
/* output tensor size for each batch */
#define ML_EXAMPLE_SVAD_OUTPUT_TENSOR_SIZE 1

#define ROLLING_READ_PTR_OFFSET(tensor_size, algo_output_size)  (tensor_size - algo_output_size) /*!< write pointer after rolling should be 1 sample behind read pointer */

#endif /* _ML_EXAMPLE_SVAD_DEFS */
