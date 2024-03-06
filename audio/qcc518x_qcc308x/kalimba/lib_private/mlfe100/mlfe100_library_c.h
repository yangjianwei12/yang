#ifndef __MLFE100_LIBRARY_C__
#define __MLFE100_LIBRARY_C__
/*============================================================================
@file mlfe100_library_c.h

header defining ML front end functions and data structures.

Copyright (c) 2023 QUALCOMM Technologies Incorporated.
All Rights Reserved.
Qualcomm Confidential and Proprietary
============================================================================*/
#include "cvclib_c.h"
#include "ml_engine.h"
#include "ml_engine_usecase_manager.h"
#include "mlfe100_internal.h"
#include "cvc_noise_sup_prepost.h"

// #define STANDALONE_TEST
// #define NODESCALE
// #define NOENGINE


#define MATRIX_MODEL_SCALE 8


#ifdef STANDALONE_TEST
    #define MODEL_INPUT_COUNT 2
    #define MODEL_OUTPUT_COUNT 2
    #define MLFE_INPUT_COUNT 1
    #define MLFE_OUTPUT_COUNT 1
#endif

// number of bins for WB
#define NUMBINS_PROC_WB    256 // for HR models
// number of bins for SWB
#define NUMBINS_PROC_SWB  512  // for HR models

typedef void (*PRE_PROC_FUNC) (void*);
typedef void (*POST_PROC_FUNC) (void*);

typedef struct MLFE100_STRUC {
    ML_ENGINE_OP_DATA * ml_engine_container;                           // ml_engine data    
    void* param_ptr;                                                   // set by dyn        
    int input_cnt;                                                     // set by dyn        
    t_filter_bank_channel_object* input_addr[MAX_MLFE_INPUT_COUNT];    // set by dyn        
    int output_cnt;                                                    // set by dyn        
    t_filter_bank_channel_object* output_addr[MAX_MLFE_OUTPUT_COUNT];  // set by dyn        
    void *pre_proc_data;                                               // set by dyn        
    void *post_proc_data;                                              // set by dyn
    void *model_data;                                                  // set by dyn
    int input_num_bins[MAX_MLFE_INPUT_COUNT];                          // set by dyn        
    int output_num_bins[MAX_MLFE_OUTPUT_COUNT];                        // set by dyn        
    PRE_PROC_FUNC pre_process;                                         
    POST_PROC_FUNC post_process;                                       
    int bypass_flag;

} mlfe100_data_t;


// initialize function
void mlfe_initialize(mlfe100_data_t *data_ptr);
// process function
void mlfe_process(mlfe100_data_t *data_ptr);

#endif//#define __MLFE100_LIBRARY_C__



