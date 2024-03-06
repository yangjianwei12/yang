// *****************************************************************************
// Copyright (c) 2008 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************
#ifndef AUDIO_XOVER_C_HEADER_INCLUDED
#define AUDIO_XOVER_C_HEADER_INCLUDED

/*****************************************************************************
Include Files
*/
#include "mem_utils/memory_table.h"
#include "mem_utils/shared_memory_ids.h"

/****************************************************************************
Private Const Declarations
*/

/* xover data object size, in AU */
#define XOVER_BASE_STRUC_SIZE (sizeof(t_xover_object))
#define XOVER_OBJECT_SIZE     XOVER_BASE_STRUC_SIZE

#define XOVER_HEADROOM_SHIFTS              0;



/****************************************************************************
Private Type Definitions
*/


typedef struct {
   unsigned int xover_config;
   unsigned int xover_core_type;
   unsigned int lp_type;
   unsigned int lp_fc;
   unsigned int lp_order;
   unsigned int hp_type;
   unsigned int hp_fc;
   unsigned int hp_order;
   unsigned int lp_type_f2;
   unsigned int lp_fc_f2;
   unsigned int lp_order_f2;
   unsigned int hp_type_f2;
   unsigned int hp_fc_f2;
   unsigned int hp_order_f2;
   unsigned int band_invert;

} t_xover_params;

// XOVER Data Structure
typedef struct  {
   void* input_frame_buffer;
   void* output_low_frame_buffer;
   void* output_mid_frame_buffer;
   void* output_high_frame_buffer;
   void* input_buffer;
   void* output_buffer_low;
   void* output_buffer_mid;
   void* output_buffer_high;
   void* internal_buffers[4];
   unsigned int sample_rate;
   t_xover_params* params_ptr;
   unsigned int samples_to_process;
   void* peq_object_ptr_low;
   void* peq_object_ptr_high;
   unsigned *peq_cap_params_low;      /* pointer to cap_params of the low_freq peq_data_object */
   unsigned *peq_cap_params_high;     /* pointer to cap_params of the high_freq peq_data_object */
   void* peq_object_ptr_low_f2;
   void* peq_object_ptr_high_f2;
   unsigned *peq_cap_params_low_f2;   /* pointer to cap_params of the f2 low_freq peq_data_object */
   unsigned *peq_cap_params_high_f2;  /* pointer to cap_params of the f2 high_freq peq_data_object */
   void* peq_object_ptr_apc_comp_f2;  /* pointer to PEQ object for f2 APC compensation */
   bool cap_config;                   /* number of bands */
} t_xover_object;


#endif // AUDIO_XOVER_C_HEADER_INCLUDED
