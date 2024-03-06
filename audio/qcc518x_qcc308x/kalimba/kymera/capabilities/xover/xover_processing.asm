// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// $Change$  $DateTime$
// *****************************************************************************

// ASM function for xover operator data processing
// The function(s) obey the C compiler calling convention (see documentation, CS-124812-UG)

#include "core_library.h"
#include "cbuffer_asm.h"
#include "portability_macros.h"
#include "xover.h"
#include "xover_gen_asm.h"
#include "stack.h"
#include "xover_wrapper_defs.h"
#include "xover_wrapper_asm_defs.h"

#include "patch_library.h"

// *****************************************************************************
// MODULE:
//    $_xover_processing
//
// DESCRIPTION:
//    Data processing function.
//
// INPUTS:
//    - r0 = Xover "extra data" object
//    - r1 = number of samples to process
//
// OUTPUTS:
//    - None
//
// TRASHED REGISTERS:
//    C compliant
//
// *****************************************************************************
.MODULE $M.xover_proc;
    .CODESEGMENT PM;
$_xover_processing:

   // *****************************************************************************
   // for now, we assume we get to the buffer parameters directly with some offset constants
   // *****************************************************************************
   PUSH_ALL_C

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($xover_cap.XOVER_PROCESSING_ASM.XOVER_PROC.XOVER_PROCESSING.PATCH_ID_0, r4)     // cap_xover_patchers


   // *****************************************************************************
   // check initialize flag
   // *****************************************************************************
   Null = M[r0 + $xover_wrapper.xover_exop_struct.REINITFLAG_FIELD];
   if Z jump bypass_reinit_xover;
   pushm <r0,r1>;
   call $_xover_initialize;
   popm <r0,r1>;

bypass_reinit_xover:
   // ******************************************************************
   // set r9 to extra_op data and r4 to number of samples to process
   // ******************************************************************
   r9 = r0;
   r4 = r1;

   // ******************************************************************
   // mode control : Set Current Mode
   // ******************************************************************
   r1 = M[r9 + $xover_wrapper.xover_exop_struct.HOST_MODE_FIELD];
   r3 = M[r9 + $xover_wrapper.xover_exop_struct.OBPM_MODE_FIELD];
   r2 = M[r9 + $xover_wrapper.xover_exop_struct.OVR_CONTROL_FIELD];
   Null = r2 AND $M.XOVER.CONTROL.MODE_OVERRIDE;
   if NZ r1 = r3;
   M[r9 + $xover_wrapper.xover_exop_struct.CUR_MODE_FIELD]  = r1;

   // ****************************
   // for chan = 1:num_channels
   // ****************************
   r5 = 0;
   xover_channel_proc_loop:
      // ******************************************
      // see whether this channel pair is valid
      // ******************************************
      r0 = M[r9 + $xover_wrapper.xover_exop_struct.CHAN_FLAGS_FIELD];
      r1 = 1 LSHIFT r5;
      Null = r0 AND r1;
      if Z jump skip_chan_proc_next;

      Words2Addr(r5);           // arch4: channel_idx in addr
      r6 = r9 + r5;             // pointer to the current channel

      // ************************************************************************************************
      // save <samples_to_process, current_channel_index, current_channel_ptr, root_object_ptr>
      // ************************************************************************************************
      pushm <r4,r5,r6,r9>;

      // *****************************
      // check pass-through mode
      // *****************************
      r0 = M[r9 + $xover_wrapper.xover_exop_struct.CUR_MODE_FIELD];
      r7 = 1;
      Null = r0 - $M.XOVER.SYSMODE.PASS_THRU;
      if Z jump bypass_or_mute_proc_mode;

      // *****************************
      // check mute mode
      // *****************************
      r7 = 0;
      Null = r0 - $M.XOVER.SYSMODE.MUTE;
      if Z jump bypass_or_mute_proc_mode;

      // **********************************************************
      // Extract the pointer to the current channel data_object
      // **********************************************************
      r8 = M[r6 + $xover_wrapper.xover_exop_struct.XOVER_OBJECT_FIELD];   // pointer to the current_channel data_object

      // ************************************************************************
      // Set the "samples_to_process" value for the current_channel data_object
      // ************************************************************************
      M[r8 + $audio_proc.xover_2band.SAMPLES_TO_PROCESS] = r4;

      // ******************************************************************************
      // Set the input/output buffer pointers for the current_channel data_object
      // ******************************************************************************
      r0 = M[r6 + $xover_wrapper.xover_exop_struct.IP_BUFFER_FIELD];
      M[r8 + $audio_proc.xover_2band.INPUT_ADDR_FIELD] = r0;              // input cbuffer pointer

#ifdef INSTALL_OPERATOR_XOVER_3BAND
      // **********************************************************
      // Set the output buffers depending on 2- or 3-band operation
      // **********************************************************
      r0 = M[r9 + $xover_wrapper.xover_exop_struct.CAP_CONFIG_FIELD];
      Null = r0 - $audio_proc.xover_2band.XOVER_3BAND;
      if Z jump xover_3band_init_output;
#endif // INSTALL_OPERATOR_XOVER_3BAND

      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD;
      r0 = M[r0 + r5]; // double increment because 2x outputs/channel
      M[r8 + $audio_proc.xover_2band.OUTPUT_ADDR_FIELD_LOW] = r0;         // low_freq output cbuffer pointer

      r0 = r6 + ($xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD + 1*ADDR_PER_WORD);
      r0 = M[r0 + r5]; // double increment because 2x outputs/channel
      M[r8 + $audio_proc.xover_2band.OUTPUT_ADDR_FIELD_HIGH] = r0;       // high_freq output cbuffer pointer

#ifdef INSTALL_OPERATOR_XOVER_3BAND
      jump xover_output_init_done;

   xover_3band_init_output:
      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD;
      r0 = r0 + r5;
      r0 = M[r0 + r5]; // triple increment because 3x outputs/channel
      M[r8 + $audio_proc.xover_2band.OUTPUT_ADDR_FIELD_LOW] = r0;         // low_freq output cbuffer pointer

      r0 = r6 + ($xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD + 1*ADDR_PER_WORD);
      r0 = r0 + r5;
      r0 = M[r0 + r5]; // triple increment because 3x outputs/channel
      M[r8 + $audio_proc.xover_2band.OUTPUT_ADDR_FIELD_MID] = r0;        // mid_freq output cbuffer pointer

      r0 = r6 + ($xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD + 2*ADDR_PER_WORD);
      r0 = r0 + r5;
      r0 = M[r0 + r5]; // triple increment because 3x outputs/channel
      M[r8 + $audio_proc.xover_2band.OUTPUT_ADDR_FIELD_HIGH] = r0;       // high_freq output cbuffer pointer

      // ***********************************
      // call the xover process function
      // ***********************************
   xover_output_init_done:
#endif // INSTALL_OPERATOR_XOVER_3BAND

      call $audio_proc.xover_2band.stream_process;
      jump xover_process_done;

bypass_or_mute_proc_mode:
      // r7 == 0 ? mute() : bypass()

      // *******************************************************
      // extract the current channel input_cbuffer read address
      // *******************************************************
      r0 = M[r6 + $xover_wrapper.xover_exop_struct.IP_BUFFER_FIELD];
      call $cbuffer.get_read_address_and_size_and_start_address;
      I0 = r0;
      push r2;
      pop B0;
      L0 = r1;

#ifdef INSTALL_OPERATOR_XOVER_3BAND
      r0 = M[r9 + $xover_wrapper.xover_exop_struct.CAP_CONFIG_FIELD];
      Null = r0 - $audio_proc.xover_2band.XOVER_3BAND;
      if Z jump bypass_mute_3band;
#endif // INSTALL_OPERATOR_XOVER_3BAND

      // **************************************************************************
      // extract the current channel low_freq output_cbuffer write address
      // **************************************************************************
      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD;
      r0 = M[r0 + r5];
      call $cbuffer.get_write_address_and_size_and_start_address;
      I1 = r0;
      push r2;
      pop B1;
      L1 = r1;

      // **************************************************************************
      // extract the current channel high_freq output_cbuffer write address
      // **************************************************************************
      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD + 1*ADDR_PER_WORD;
      r0 = M[r0 + r5];
      call $cbuffer.get_write_address_and_size_and_start_address;
      I4 = r0;
      push r2;
      pop B4;
      L4 = r1;

      // ******************************************************
      // write output_buffer values based on mute/bypass mode
      // ******************************************************

      r10 = r4;
      do bypass_loop;
         Null = r7, r0 = M[I0,MK1];
         if Z r0 = Null;
         M[I1,MK1] = r0 , M[I4,MK1] = r0;
      bypass_loop:

#ifdef INSTALL_OPERATOR_XOVER_3BAND
      jump cleanup_bypass;

   bypass_mute_3band:
      // **************************************************************************
      // 3-band: extract the current channel low_freq output_cbuffer write address
      // **************************************************************************
      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD;
      r0 = r0 + r5; // double increment because 3x outputs per channel
      r0 = M[r0 + r5];
      call $cbuffer.get_write_address_and_size_and_start_address;
      I1 = r0;
      push r2;
      pop B1;
      L1 = r1;

      // **************************************************************************
      // 3-band: extract the current channel mid_freq output_cbuffer write address
      // **************************************************************************
      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD + 1*ADDR_PER_WORD;
      r0 = r0 + r5; // double increment because 3x outputs per channel
      r0 = M[r0 + r5];
      call $cbuffer.get_write_address_and_size_and_start_address;
      I4 = r0;
      push r2;
      pop B4;
      L4 = r1;

      // ***************************************************************************
      // 3-band: extract the current channel high_freq output_cbuffer write address
      // ***************************************************************************

      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD + 2*ADDR_PER_WORD;
      r0 = r0 + r5; // double increment because 3x outputs per channel
      r0 = M[r0 + r5];
      call $cbuffer.get_write_address_and_size_and_start_address;
      I5 = r0;
      push r2;
      pop B5;
      L5 = r1;

      r10 = r4;
      do bypass_loop2;
         Null = r7, r0 = M[I0,MK1];
         if Z r0 = Null;
         M[I1,MK1] = r0 , M[I4,MK1] = r0;
         M[I5,MK1] = r0;
      bypass_loop2:

      // *****************************************************************
      // clear all base/length registers used in pass-through/mute modes
      // *****************************************************************
   cleanup_bypass:
   #endif // INSTALL_OPERATOR_XOVER_3BAND

      push Null;
      pop B0;
      push Null;
      pop B1;
      push Null;
      pop B4;
      push Null;
      pop B5;
      L0 = 0;
      L1 = 0;
      L4 = 0;
      L5 = 0;

xover_process_done:
      popm <r4,r5,r6,r9>;

      // *******************************************
      // update read address in input buffer
      // *******************************************
      r0 = M[r6 + $xover_wrapper.xover_exop_struct.IP_BUFFER_FIELD];
      r1 = r4;
      call $cbuffer.advance_read_ptr;

#ifdef INSTALL_OPERATOR_XOVER_3BAND
      // ******************************************************
      // test for 3-band buffer updating
      // ******************************************************
      r0 = M[r9 + $xover_wrapper.xover_exop_struct.CAP_CONFIG_FIELD];
      Null = r0 - $audio_proc.xover_2band.XOVER_3BAND;
      if Z jump update_3band;
#endif // INSTALL_OPERATOR_XOVER_3BAND

update_2band:
      // ***********************************************
      // update write address in low_freq output buffer
      // ***********************************************
      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD;
      r0 = M[r0 + r5]; // double increment because 2x outputs/channel
      r1 = r4;
      call $cbuffer.advance_write_ptr;

      // ***************************************************
      // update write address in high_freq output buffer
      // ***************************************************
      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD + 1*ADDR_PER_WORD;
      r0 = M[r0 + r5]; // double increment because 2x outputs/channel
      r1 = r4;
      call $cbuffer.advance_write_ptr;

#ifdef INSTALL_OPERATOR_XOVER_3BAND
      jump update_done;

update_3band:
      // ***********************************************
      // update write address in low_freq output buffer
      // ***********************************************
      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD;
      r0 = r0 + r5;
      r0 = M[r0 + r5]; // triple increment because 3x outputs/channel
      r1 = r4;
      call $cbuffer.advance_write_ptr;

      // ***************************************************
      // update write address in mid freq output buffer
      // ***************************************************
      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD + 1*ADDR_PER_WORD;
      r0 = r0 + r5;
      r0 = M[r0 + r5]; // triple increment because 3x outputs/channel
      r1 = r4;
      call $cbuffer.advance_write_ptr;

      // ***************************************************
      // update write address in high_freq output buffer
      // ***************************************************
      r0 = r6 + $xover_wrapper.xover_exop_struct.OP_BUFFER_FIELD + 2*ADDR_PER_WORD;
      r0 = r0 + r5;
      r0 = M[r0 + r5]; // triple increment because 3x outputs/channel
      r1 = r4;
      call $cbuffer.advance_write_ptr;

   update_done:
#endif // INSTALL_OPERATOR_XOVER_3BAND
      Addr2Words(r5);                         // arch4: channel_idx in words

skip_chan_proc_next:
   r5 = r5 + 1;                               // process next channel
   Null = r5 - XOVER_CAP_MAX_IO_GROUPS;
   if NZ jump xover_channel_proc_loop;

   POP_ALL_C
   rts;


.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $_xover_initialize
//
// DESCRIPTION:
//    Initialize function
//
// INPUTS:
//    - r0 = XOVER "extra data" object
//
// OUTPUTS:
//    - None
//
// TRASHED REGISTERS:
//    C compliant
//
// *****************************************************************************
.MODULE $M.xover_initialize;
    .CODESEGMENT PM;
$_xover_initialize:

   PUSH_ALL_C

   r9 = r0;

   LIBS_SLOW_SW_ROM_PATCH_POINT($xover_cap.XOVER_PROCESSING_ASM.XOVER_INITIALIZE.XOVER_INITIALIZE.PATCH_ID_0, r3)     // cap_xover_patchers

   r5 = 0;

   // ***************************
   // Extract the sample rate
   // ***************************
   r3 = M[r9 + $xover_wrapper.xover_exop_struct.SAMPLE_RATE_FIELD];

   // ***************************
   // Extract cap_config
   // ***************************
   r4 = M[r9 + $xover_wrapper.xover_exop_struct.CAP_CONFIG_FIELD];

   xover_channel_init_loop:
      push r5;

      // ******************************************
      // see whether this channel pair is valid
      // ******************************************
      r0 = M[r9 + $xover_wrapper.xover_exop_struct.CHAN_FLAGS_FIELD];
      r1 = 1 LSHIFT r5;
      Null = r0 AND r1;
      if Z jump done_init_xover;

      // arch4: convert channel index - address offset into XOVER object array
      Words2Addr(r5);
      r6 = r9 + r5;

      // *******************************************************
      // Set the sample_rate field for the current data_object
      // *******************************************************
      r8 = M[r6 + $xover_wrapper.xover_exop_struct.XOVER_OBJECT_FIELD];
      M[r8 + $audio_proc.xover_2band.SAMPLE_RATE_FIELD] = r3;

      // *******************************************************
      // Set the cap_config field for the current data_object
      // *******************************************************
      M[r8 + $audio_proc.xover_2band.CAP_CONFIG_FIELD] = r4;

      // ******************************************
      // initialize all of the xover data objects
      // ******************************************
      pushm <r3,r4,r9>;
      call $audio_proc.xover.initialize;
      popm <r3,r4,r9>;

done_init_xover:
      pop r5;

      // ****************
      // next channel
      // ****************
      r5 = r5 + 1;
      Null = r5 - XOVER_CAP_MAX_IO_GROUPS;
   if NZ jump xover_channel_init_loop;

   // *********************
   // clear reinit flag
   // *********************
   M[r9 + $xover_wrapper.xover_exop_struct.REINITFLAG_FIELD] = Null;

   POP_ALL_C
   rts;

.ENDMODULE;
