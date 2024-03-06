// *****************************************************************************
// Copyright (c) 2022 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef CHDELAY100_C_H
#define CHDELAY100_C_H

// ***************************************
// Channel Delay data structure for C
// ***************************************

//  WARNING!! DO NOT EDIT WITHOUT EDITING chdelay100_library.h
typedef struct  
{
   t_filter_bank_channel_object *input_data_ptr;
   int *delaybuf_start_ptr;
   int delay_in_frames;
   int num_bins;
   int *delaybuf_read_ptr;
   int *delaybuf_write_ptr;
   int delaybuf_framesize;
   int delaybuf_len;
   t_filter_bank_channel_object *output_buf_ptr;
   
} chdelay_data_t;

void chdelay100_initialize(chdelay_data_t* delay_struct, void* module_init_function);

void chdelay100_process(chdelay_data_t* delay_struct, void* module_control_function);
#endif // CHDELAY100_C_H