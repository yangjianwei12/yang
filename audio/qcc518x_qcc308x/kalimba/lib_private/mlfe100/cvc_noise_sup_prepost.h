#ifndef __CVC_NOISE_SUP_PREPOST__
#define __CVC_NOISE_SUP_PREPOST__
/*============================================================================
@file cvc_noise_sup_prepost.h

header defining ML pre/post/model data structures.

Copyright (c) 2023 QUALCOMM Technologies Incorporated.
All Rights Reserved.
Qualcomm Confidential and Proprietary
============================================================================*/
#include "cvclib_c.h"
#include "chdelay100/chdelay100_library_c.h"
#include "mlfe100_internal.h"


/* Masking model parameters
    TBD: Move this (along with all non core MLFE objects) to another file
 */
typedef struct MLFE100_MASKING_PARAM {
    unsigned l2_mlfe_gain;      // 8.24: Gain (in l2
    unsigned mlfe_thresh;       // 1.31: Threshhold 
    unsigned mlfe_aggr;         // 1.31: Agressiveness
    unsigned mlfe_swb_atten;    // 1.31: attenuation for swb masking
}   mlfe100_masking_param_t;


// For Model 5c: No pre compander or post expander scaling needed
//  Pre model scale from Matlab: scale_input_matlab = 2^7
//  Pre model scaling from offline tool: scale_input_offline = 2^-7
//  Total input Scale  = scale_input_matlab * scale_input_offline = 1 : No input scale needed!

//  Post model scale from Matlab: scale_output_matlab = 2^-7
//  Post model scaling (output1) from offline tool: scale_input_offline = 2^5
//  Post model scaling (output2) from offline tool: scale_input_offline = 2^1
//  Mixed output = input2 * ouput2 + input1*(2^-3)
//  output of mix will be need to be scaled by 2^8
// Total output scale = 2^8 * scale_output_matlab = 2

// post compander scale = input1_scale * (Vahid_scale_fact * Vahid_model_scale) = 2^-7 * 2^7 = .5 in Q3.29
#define POST_COMP_SCALE 268435456 
// pre expander scale =  input2_scale + MatMul2_scale  / (Vahid_scale_fact * Vahid_model_scale) 
//                    = 2^7 * 2^1 / (Vahid_scale_fact * Vahid_model_scale) = 2^6 * 2^1 / 2^7 = 2 in Q3.29
#define PRE_EXP_SCALE 1073741824 

typedef struct model_5_pre_proc_data {
    chdelay_data_t* swb_delay_struct;                                  // set by dyn
    t_filter_bank_channel_object* swb_scratch;                         // set by dyn
    chdelay_data_t* delay_struct;                                      // set by dyn
    int* scratch_tensor;                                               // set by dyn
    void *mulaw_cmpd100_data;                                          // set by dyn
    int in_tensor_cnt;                                                 // set in preproc_init from model_data          
    int in_tensor_size[MAX_MODEL_INPUT_COUNT];                         // set in preproc_init from model_data         
    int* in_tensor_addr[MAX_MODEL_INPUT_COUNT];                        // set in preproc_init from model_data               
    int in_tensor_scale[MAX_MODEL_INPUT_COUNT];                        // unused
    int input_num_bins[MAX_MLFE_INPUT_COUNT];                          // set in preproc_init from mlfe_data                           
    int input_cnt;                                                     // set in preproc_init from mlfe_data
    t_filter_bank_channel_object* input_addr[MAX_MLFE_INPUT_COUNT];    // set in preproc_init from mlfe_data
    mlfe100_masking_param_t *param_ptr;                                                  // set in preproc_init from mlfe_data
    int *bypass_flag_ptr;                                              // set in postproc_init from mlfe_data
     
} model_5_pre_proc_data_t;

typedef struct model_5_post_proc_data_t{
    void *mulaw_expd100_data;                                          // set by dyn
    int out_tensor_cnt;                                                // set in postproc_init from model_data     
    int out_tensor_size[MAX_MODEL_OUTPUT_COUNT];                       // set in postproc_init from model_data 
    int* out_tensor_addr[MAX_MODEL_OUTPUT_COUNT];                      // set in postproc_init from model_data                         
    int out_tensor_scale[MAX_MODEL_OUTPUT_COUNT];                      // unused
    int output_num_bins[MAX_MLFE_OUTPUT_COUNT];                        // set in postproc_init from mlfe_data          
    int output_cnt;                                                    // set in postproc_init from mlfe_data  
    t_filter_bank_channel_object* output_addr[MAX_MLFE_OUTPUT_COUNT];  // set in postproc_init from mlfe_data 
    mlfe100_masking_param_t *param_ptr;                                                  // set in postproc_init from mlfe_data 
    model_5_pre_proc_data_t *pre_proc_data;                            // set in postproc_init from mlfe_data
    int renorm_headroom;                                               // set in postproc_init
    int *bypass_flag_ptr;                                              // set in postproc_init from mlfe_data
    
} model_5_post_proc_data_t;

typedef struct model_5_data_t {
    int in_tensor_size[MAX_MODEL_INPUT_COUNT];                         // set in model_init() from ml_engine_data
    int *in_tensor_addr[MAX_MODEL_INPUT_COUNT];                        // set in model_init() from ml_engine_data  
    int in_tensor_cnt;                                                 // set in model_init() from ml_engine_data 
    int out_tensor_size[MAX_MODEL_OUTPUT_COUNT];                       // set in model_init() from ml_engine_data  
    int *out_tensor_addr[MAX_MODEL_OUTPUT_COUNT];                      // set in model_init() from ml_engine_data
    int out_tensor_cnt;                                                // set in model_init() from ml_engine_data 
    
} model_5_data_t;


void preproc_initialize(void *mlfe_data_ptr, void *preproc_data_ptr);
void postproc_initialize(void *mlfe_data_ptr, void *postproc_data_ptr);
void model_initialize(void *usecase_info, void *model_data_ptr);
void model_5_process_inputs(void *preproc_data_ptr);
void model_5_process_outputs(void *postproc_data_ptr);

void mulaw_compand100_initialize(void *preproc_data, void *mulaw_compand100_data, int input_num);
void mulaw_expand100_initialize(void *postproc_data, void *mulaw_expand100_data, int input_num);

#endif   //#define __CVC_NOISE_SUP_PREPOST__