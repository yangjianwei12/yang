/**
 * \file  @@@cap_name@@@_defs.h
 * \ingroup capabilities
 *
 *
 */

#ifndef _@@@cap_name^U@@@_DEFS_H
#define _@@@cap_name^U@@@_DEFS_H

#define @@@cap_name^U@@@_CAP_VERSION_MAJOR 1
#define @@@cap_name^U@@@_CAP_VERSION_MINOR 0

/* Number of input terminal connections supported by the capability */              
#define NUM_INPUTS	1
/* Number of output terminal connections supported by the capability */
#define NUM_OUTPUTS	1 
/* Input sample rate for the capability */
#define INPUT_SAMPLE_RATE 16000
/* Output sample rate for the capability */
#define OUTPUT_SAMPLE_RATE 16000

/* This defines(in seconds) the size of data required by the preprocessing algorithms to process
 * input audio into tensors.
 * This together with the INPUT_SAMPLE_RATE defines the size of the buffer at
 * the input terminal of the capability. The buffer at the input terminal of the capability
 * should atleast hold one complete frame of data required to run the preprocessing algorithms.
 * By default - the size of the input buffer created is 2 * INPUT_BLOCK_PERIOD * INPUT_SAMPLE_RATE.
 * Please refer *_buffer_details() function in *_cap.c file.
 */
#define INPUT_BLOCK_PERIOD 0.010

/* This defines(in seconds) the size of data generated by the postprocessinng algorithms.
 * This together with the OUTPUT_SAMPLE_RATE defines the size of the buffer at the
 * output terminal of the capability. The buffer at the output terminal of the capability
 * should atleast hold one complete frame of output data generated by the postpreocessing
 * algorithms. By default - the size of the output buffer created is
 * 2 * OUTPUT_BLOCK_PERIOD * OUTPUT_SAMPLE_RATE. Please refer *_buffer_details() function
 * in *_cap.c file
 */
#define OUTPUT_BLOCK_PERIOD 0.010

/* The section below defines the input and output tensors of the model loaded.
 * The capability uses these compiler defines to ensure that the correct model
 * is loaded.
 * Please ensure that this matches the details from the "model_info.txt" file
 * which is generated when an onnx model is converted into a keai model.
 */
#define NUM_IP_TENSORS 1
#define INPUT_TENSOR_ID 0
#define NUM_OP_TENSORS 1
#define OUTPUT_TENSOR_ID 2
#define IP_TENSOR_SIZE 160
#define OP_TENSOR_SIZE 160




#endif /* _@@@cap_name^U@@@_DEFS */