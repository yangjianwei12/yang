/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
#include "stack.h"          // for PUSH_ALL_C macros
#include "codec_library.h"  // for ENCODER_DATA_OBJECT_FIELD
#include "swbs_struct_asm_defs.h"
#include "sco_common_struct_asm_defs.h"
#include "opmgr_for_ops_asm_defs.h"

#include "patch_library.h"

#ifdef ADAPTIVE_R2_BUILD
#include "aptXAdaptive_asm_r2.h"
#else
#include "aptXAdaptive_asm.h"
#endif

#define SDU_RFU_CHUNK_ID 0x0100
#define WBM_RFU_CHUNK_ID 0x0200
#define SDU_HEADER_SIZE       4
#define WBM_HEADER_SIZE      10
#define DEC_WEAK_HEADER_SIZE  4

#define SWBS_CODEC_MODE0 0
#define SWBS_CODEC_MODE4 4

#define SWBS_MODE0_AUDIO_BLOCK_SIZE 240
#define SWBS_MODE4_AUDIO_BLOCK_SIZE 180

#define SWBS_SMALLEST_VALID_PACKET_IN_WORDS 30
#define SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES  60
#define SWBS_EXTENDED_ENCODED_BLOCK_SIZE_IN_BYTES  120
#define SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES_WITH_BIT_ERROR ((2*SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES)+DEC_WEAK_HEADER_SIZE)
#define SWBS_DEFAULT_SDU_WBM_BLOB_SIZE ((2*SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES)+SDU_HEADER_SIZE+WBM_HEADER_SIZE)

.MODULE $M.swbs_enc_c_stubs;
    .CODESEGMENT PM;

// void swbsenc_init_encoder(OPERATOR_DATA *op_data)
$_swbsenc_init_encoder:
    pushm <r7, r8, r9, rLink>;

    r7 = r0;

    // set up the aptX adaptive library to point to the correct data
    r0 = r7;
    call $base_op.base_op_get_instance_data;
    r9 = M[r0 + $swbs_struct.SWBS_ENC_OP_DATA_struct.CODEC_DATA_FIELD];

    popm <r7, r8, r9, rLink>;
    rts;


// void swbsenc_process_frame( OPERATOR_DATA *op_data)
$_swbsenc_process_frame:
    PUSH_ALL_C

    LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SWBS_ENC_C_STUBS._SWBSENC_PROCESS_FRAME.PATCH_ID_0,r9)

    // set up the aptX adaptive library to point to the correct data
    push r0;
    call $base_op.base_op_get_instance_data;
    r7 = r0;
    pop r0;
    r9 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.CODEC_DATA_FIELD];

   // get the Input buffer
   r0 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.SCO_SEND_OP_DATA_FIELD+
               $swbs_struct.SCO_COMMON_SEND_OP_DATA_struct.BUFFERS_FIELD +
               $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_read_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = read address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;

   // Copy data from system input buffer to codec input axbuf.
   r2 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXINBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_WPTR_OFFSET];

   r10 = SWBS_MODE0_AUDIO_BLOCK_SIZE;
   r6 = SWBS_MODE4_AUDIO_BLOCK_SIZE;
   r1 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.CODECMODE_FIELD];
   Null = r1 - SWBS_CODEC_MODE4;
   if EQ r10 = r6;

   r4 = -8;                           // align input data to 24-bit for aptX adaptive
   do input_copy;                     // copy from circular buffer to linear axbuffer
   r0 = M[I0,M1];
   r0 = r0 ASHIFT r4;
   M[I1,M1] = r0;
   input_copy:

   M[r2 + $AXBUF_WPTR_OFFSET] = I1;    // set input buffer write pointer

   r0 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.SCO_SEND_OP_DATA_FIELD+
               $swbs_struct.SCO_COMMON_SEND_OP_DATA_struct.BUFFERS_FIELD +
               $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];
   r1 = I0;
   call $cbuffer.set_read_address;

   // Reset circular buffer registers, codec uses linear axbuf
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   r2 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXOUTBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_DAT_OFFSET];
   M[r2 + $AXBUF_RPTR_OFFSET] = I1;   // set read pointer to be base address of data
   M[r2 + $AXBUF_WPTR_OFFSET] = I1;   // set write pointer to be base address of data

   r0 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.CODEC_DATA_FIELD];
   r1 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXINBUF_FIELD;
   r2 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXOUTBUF_FIELD;


    // aptX Adaptive encode
   call $aptx_adaptive.voice_encode;    /// wrapper around adaptive codec


      // get the Output buffer
   r0 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.SCO_SEND_OP_DATA_FIELD +
               $swbs_struct.SCO_COMMON_SEND_OP_DATA_struct.BUFFERS_FIELD +
               $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_write_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = write address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;

   r2 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXOUTBUF_FIELD;
   I1 = M[r2 + $AXBUF_DAT_OFFSET];

   r10 = 15;                              // voice encoder will produce 60 bytes (32kHz mono)
   Null = r1 - r10;
   if LT jump no_space;
   do output_copy;                        // copy and shift from 32-bit linear buffer to 16-bit circular buffer
   r0 = M[I1,M1];
   r1 = r0 LSHIFT -16;                    // aptX adaptive produces 32-bit codewords
   r0 = r0 AND 0xFFFF;                    // Shift and mask to get two 16-bit codewords
   M[I0,M1] = r1;
   M[I0,M1] = r0;
   output_copy:

   r0 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.SCO_SEND_OP_DATA_FIELD +
               $swbs_struct.SCO_COMMON_SEND_OP_DATA_struct.BUFFERS_FIELD +
               $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];

   r1 = I0;

   call $cbuffer.set_write_address;

   // In Mode 4 the codec reads 176/176/176/192 samples so there might be some data left in the input buffer.
   // Do a consume to move the spare data to the top of the buffer for the next call.

   r0 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXINBUF_FIELD;
   call $_AxDataConsumeASM;

   // Reset circular buffer registers
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

no_space:

   POP_ALL_C
   rts;


// void swbs_enc_reset_aptx_data(OPERATOR_DATA *op_data)
$_swbs_enc_reset_aptx_data:
    pushm <r4, r5, r9, rLink>;

//   LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SWBS_ENC_C_STUBS._SWBS_ENC_RESET_SBC_DATA.PATCH_ID_0)

    push I0;

    // Insert code to reset aptX data if required

    pop I0;
    popm <r4, r5, r9, rLink>;
    rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $_swbsdec_init_dec_param
//
// DESCRIPTION:
//    Function for initialising the swbs decoder parameters.
//
// INPUTS:
//    - r0 = pointer to operator data
//
// OUTPUTS:
//
// TRASHED REGISTERS:
//    r4, r9
// NOTES:
//
// *****************************************************************************
.MODULE $M.swbs_dec._swbsdec_init_dec_param;
   .CODESEGMENT PM;

$_swbsdec_init_dec_param:

   pushm <r4, r9, rLink>;

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SWBS_DEC._SWBSDEC_INIT_DEC_PARAM._SWBSDEC_INIT_DEC_PARAM.PATCH_ID_0,r4)

   push r0;
   call $base_op.base_op_get_instance_data;
   r4 = r0;
   pop r0;
   r9 = M[r4 + $wbs_struct.WBS_DEC_OP_DATA_struct.CODEC_DATA_FIELD];

   popm <r4, r9, rLink>;
   rts;

.ENDMODULE;

// r0 = op_data
// r1 = packet_size

.MODULE $M.swbs_run_plc;

   .DATASEGMENT DM;
   .CODESEGMENT PM;

$_swbs_run_plc:

   PUSH_ALL_C

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SWBS_DEC._SWBS_RUN_PLC._SWBS_RUN_PLC.PATCH_ID_0,r5)
   r5 = r1;  // Store packet_size for later

   call $base_op.base_op_get_instance_data;
   r4 = r0;
   r1 = r4 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;  // flattened

   // r0 = pDecState
   // r1 = audioOut
   // r2 = packet_size
   r0 = M[r4 + $swbs_struct.SWBS_DEC_OP_DATA_struct.CODEC_DATA_FIELD];
   r2 = r5;
   call $aptx_adaptive.run_plc;

   // Copy data from codec output axbuf into system output buffer
   // Get the Output buffer
   r0 = M[r4 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_write_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = write address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   // Set up circular buffer
   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;

   r2 = r4 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;
   I1 = M[r2 + $AXBUF_DAT_OFFSET];

   r10 = r5;                             // Get packet_size from above

   Null = r1 - r10;
   if LT jump no_space;
   do output_copy;                       // copy and shift from linear buffer to circular buffer
   r0 = M[I1,M1];
   r0 = r0 ASHIFT 8;                     // aptX adaptive produces 24-bit aligned audio (in 32-bit word)
   M[I0,M1] = r0;
   output_copy:

   M[r2 + $AXBUF_WPTR_OFFSET ] = Null;   // Reset write pointer

   r0 = M[r4 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];
   r1 = I0;

   call $cbuffer.set_write_address;

no_space:

   // Reset circular buffer registers
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   POP_ALL_C
   rts;

.ENDMODULE;



// *****************************************************************************
// FUNCTION:
//    $sco_decoder.swbs.process:
//
// DESCRIPTION:
//    Decoding SCO SWBS packets into DAC audio samples.
//
//    The SCO c-buffer contains SWBS packet words to be decoded into DAC
//    audio samples. Refer to the function description of Frame_encode for
//    SWBS packet word definition.  DAC audio could be mono-channel(left
//    only) or stereo-channels(both left and right).
//
// INPUTS:
// r0 op data
// r1 swbs_packet_length
// OUTPUTS:
// r0 Output packet size

// TRASHED REGISTERS:
//    Assumes everything
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
//
// NOTES:
// *****************************************************************************

.MODULE $M.sco_decoder.swbs._process;

   .DATASEGMENT DM;
   .CODESEGMENT PM;

$_sco_decoder_swbs_process:
// r0 op data
// r1 swbs_packet_length

   PUSH_ALL_C

   // save OP DATA
   r7 = r0;

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SCO_DECODER.SWBS._PROCESS._SCO_DECODER_SWBS_PROCESS.PATCH_ID_0,r5)

   // save the swbs packet size
   r5 = r1;

   // get extra op data
   call $base_op.base_op_get_instance_data;
   r3 = r0;

   // get the Input buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_read_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = read address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;

   // Copy data from system input buffer to codec input axbuf.
   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_DAT_OFFSET];
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;    // set relative read pointer to be 0
   I2 = I1;                           // store base address
   r0 = M[r2 + $AXBUF_WPTR_OFFSET];
   I1 = I1 + r0;

   r10 = 15;                          // Copy 60 bytes
   r4 = 16;                           // Data is packed in 16 bit words
   do input_copy;                     // copy from circular buffer to linear buffer
   r0 = M[I0,M1];
   r0 = r0 LSHIFT r4,
   r1 = M[I0,M1];
   r1 = r1 AND 0xFFFF;                // Mask upper 16-bits off in case there is stale data
   r0 = r0 OR r1;
   M[I1,M1] = r0;
   input_copy:

#ifdef ESCO_SUPPORT_ERRORMASK         // If error mask support is included the decoder expects a
   r0 = 0;                            // weak header and mask every packet.
   M[I1,M1] = r0;                     // Write a 0-filled weak header since there is no WBM data
   r10 = 15;
   do dummy_wbm;                      // Write a 0-filled WBM since there isn't a supplied mask
   M[I1,M1] = r0;
   dummy_wbm:
#endif

input_processed:
   r0 = I1 - I2;                      // calculate relative write pointer to base
   M[r2 + $AXBUF_WPTR_OFFSET] = r0;   // set write pointer

   r4 = 0;
#ifdef ESCO_SUPPORT_ERRORMASK
   Null = r0 - SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES_WITH_BIT_ERROR;
#else
   Null = r0 - SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES;
#endif
   if LT jump not_enough_input_data;  // Full SWBS packet are 60 or 124 (w/WBM) bytes


   // Reset circular buffer registers, codec uses linear axbuf
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_DAT_OFFSET];
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;   // set relative read pointer to be zero
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;   // set relative write pointer to be zero
// INPUTS:
//    r1    - Packet timestamp
//    r2    - Packet status
//    r5    - payload size in bytes
//    I0,L0,B0 - Input CBuffer
//    R9    - data object pointer
//
//    decoupled variant
//    r8    - Output CBuffer

   r0 = M[r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.CODEC_DATA_FIELD];
   r1 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;
   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;

   r4 = r3;
   call $aptx_adaptive.voice_decode;    /// aptX adaptive decoder call
   r3 = r4;

// OUTPUTS:
//    r5    - Output packet status
//    I0    - Input buffer updated (This is not alway true)

//   r0 = r5;

   // Copy data from codec output axbuf into system output buffer

      // get the Output buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_write_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = write address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;


   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;
   I1 = M[r2 + $AXBUF_DAT_OFFSET];
   r0 = M[r2 + $AXBUF_WPTR_OFFSET];
   Addr2Words(r0);                      // Convert output byte count to sample count
   r10 = 240;                           // Max output data is 240 samples (Mode0 - 32kHz)
   Null = r0 - r10;
   if LT r10 = r0;

  // We have finished with internal buffer, reset pointers
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;

default_mode:
   r4 = 0;
   Null = r1 - r10;
   if LT jump no_space;
   r4 = r10;
   do output_copy;                      // copy and shift from linear buffer to circular buffer
   r0 = M[I1,M1];
   r0 = r0 ASHIFT 8;                    // aptX adaptive produces 24-bit aligned audio (in 32-bit word)
   M[I0,M1] = r0;
   output_copy:

   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;      // set relative output write pointer to be zero

   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;      // set relative input write pointer to be zero

no_space:
not_enough_input_data:

   // Reset circular buffer registers
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   r0 = r4;                             // Return packet size

   POP_ALL_C
   rts;

.ENDMODULE;


// *****************************************************************************
// FUNCTION:
//    $_sco_decoder_swbs_validate
//
// DESCRIPTION:
//    C callable version of the $sco_decoder.swbs.validate function.
//    Validates the packet. Must be called before decode.
//    For SWBS this only checks size, input data and output space.
//    There is no sync word checking or data validation.
//
// INPUTS:
//    r0 - swbs_dec.CODEC_DATA_FIELD
//    r1 - payload length in bytes
//    r2 - pointer which will be populated with the swbs packet length.
//
// OUTPUTS:
//    r0 - Output in samples, 0 is no space, 1 is no data
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
//
// NOTES:
// *****************************************************************************

.MODULE $M.sco_decoder.swbs._validate;

   .DATASEGMENT DM;
   .CODESEGMENT PM;

$_sco_decoder_swbs_validate:

   PUSH_ALL_C

    // save OP DATA
   r7 = r0;

   // payload length
   r5 = r1;// this should be already in bytes.

   LIBS_SLOW_SW_ROM_PATCH_POINT($swbs_cap.WBS_C_STUBS_ASM.SCO_DECODER.SWBS._VALIDATE._SCO_DECODER_SWBS_VALIDATE.PATCH_ID_0,r6)

    // save the pointer to the swbs packet length
   push r2;

   // Calc amount of data in input buffer - return r0 = 1 if no data
   r6 = 1;
   // get extra op data
   r0 = r7;
   call $base_op.base_op_get_instance_data;
   r3 = r0;
   // get the Input buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];
   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.calc_amount_data_in_words;

   Null = r0 - SWBS_SMALLEST_VALID_PACKET_IN_WORDS; // Smallest input packet supported is 60 bytes
   if LT jump return;  // No data

   // Calc amount of space in output buffer - return r0 = 0 if no space
   r6 = 0;
   // get extra op data
   r0 = r7;
   call $base_op.base_op_get_instance_data;
   r3 = r0;
   // get the Input buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];
   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.calc_amount_space_in_words;
   // OUTPUTS:
   // r0 = amount of space (for new data) in words or addresses
   // r2 = buffer size in words or addresses
   // save the amount of space available in output buffer


   r4 = SWBS_MODE0_AUDIO_BLOCK_SIZE;
   r2 = SWBS_MODE4_AUDIO_BLOCK_SIZE;
   r1 = M[r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.CODECMODE_FIELD];
   Null = r1 - SWBS_CODEC_MODE4;
   if EQ r4 = r2;

   Null = r0 - r4;      // Check if there is enough enoutput space
   if LT jump return;   // No space
   r6 = r4;             // Set output samples length

   // Else return output samples length

return:
   pop r2;
   // populate the swbs packet length
   M[r2] = r5;

   r0 = r6; // populate the return value
   POP_ALL_C
   rts;


.ENDMODULE;

#ifdef ESCO_SUPPORT_ERRORMASK
// *****************************************************************************
// FUNCTION:
//    $sco_decoder.swbs.process_bit_error:
//
// DESCRIPTION:
//
// INPUTS:
// OUTPUTS:
// TRASHED REGISTERS:
//    Assumes everything
//
// *****************************************************************************

.MODULE $M.sco_decoder.swbs._process_bit_error;

   .DATASEGMENT DM;
   .CODESEGMENT PM;

$_sco_decoder_swbs_process_bit_error:

// r0 op data
// r1 swbs_packet_length
   PUSH_ALL_C
   push M3;
   // save OP DATA
   r7 = r0;

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SCO_DECODER.SWBS._PROCESS._SCO_DECODER_SWBS_PROCESS_BIT_ERROR.PATCH_ID_0,r5)

   // save the swbs audio payload size
   r5 = r1;

   // get extra op data
   call $base_op.base_op_get_instance_data;
   r3 = r0;

   // get the Input buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_read_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = read address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;

   // Get codec input axbuf.
   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_DAT_OFFSET];
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;    // set relative read pointer to be 0
   I2 = I1;                           // store base address

   /* SDU blob - note bytes in SCO buffer are reversed
   <-------- uint16 --------->
   +=========================+
   | Byte[1]   : Byte[0]     | RFU (8bit) + Chunk ID (8bit)
   +-------------------------+
   | Byte[3]   : Byte[2]     | Length (16bit)
   +-------------------------+
   | SDU[1]    : SDU[0]      |
   :                         : Audio frame
   | SDU[n-1]  : SDU[n-2]    |
   +-------------------------+
   */

   r1 = M[I0,M1];                     // Dummy read SDU RFU and Chunk ID
   rMAC = M[I0,M1];                   // Read SDU length (including header)
   rMAC = rMAC AND 0xFFFF;            // Mask upper 16-bits off in case there is stale data
   rMAC = rMAC ASHIFT -8;
   rMAC = rMAC - SDU_HEADER_SIZE;

   I5 = I0;                           // Store pointer to start of SDU
   r6 = 0;                            // Flag for doubles sized packet
   M3 = 0;

   Null = r5 - SWBS_DEFAULT_SDU_WBM_BLOB_SIZE;
   if EQ jump process_data;
   M3 = SWBS_EXTENDED_ENCODED_BLOCK_SIZE_IN_BYTES;
   r6 = 1;

process_data:
   r10 = 15;                          // Copy 60 bytes of SDU
   r4 = 16;                           // Data is packed in 16 bit words
   do input_copy;                     // copy from circular buffer to linear buffer and pack as 32 bits
       r0 = M[I0,M1];
       r0 = r0 LSHIFT r4,
       r1 = M[I0,M1];
       r1 = r1 AND 0xFFFF;            // Mask upper 16-bits off in case there is stale data
       r0 = r0 OR r1;
       M[I1,M1] = r0;
   input_copy:

   Null = r6 - 2;                     // Check if we're processing the second half of a double packet
   if EQ jump wbm_header;
   r0 = M[I0,M3];                     // Only advance I0 For first half of double packet/for single packet M3 is 0

   /* WBM blob - note bytes in SCO buffer are reversed
   <-------- uint16 --------->
   +=========================+
   | Byte[1]   : Byte[0]     | RFU (8bit) + Chunk ID (8bit)
   +-------------------------+
   | Byte[3]   : Byte[2]     | Length (16bit)
   +-------------------------+
   | Byte[5]   : Byte[4]     | RFU (8 bit) + PDU (8bit)
   +-------------------------+
   | Byte[7]   : Byte[6]     | Offset (16bit)
   +-------------------------+
   | Byte[9]   : Byte[8]     | The number of weak bits set (14bit) + CRC State (2bits)
   +-------------------------+
   | WBM[1]    : WBM[0]      | Weak bit mask (upto 1019 bytes)
   :                         :
   | WBM[n-1]  : WBM[n-2]    |
   +-------------------------+
   */
wbm_header:
   r0 = M[I0,M1];                  // WBM RFU + Chunk ID
   r0 = r0 AND 0xFFFF;
   Null = r0 - WBM_RFU_CHUNK_ID;
   if NE jump wbm_error;           // Check if WBM RFU and Chunk ID is correct

   r0 = M[I0,M1];                  // WBM blob Length
   r0 = r0 AND 0xFFFF;
   r0 = r0 ASHIFT -8;
   r0 = r0 - WBM_HEADER_SIZE;      // WBM payload length = blob length - header length
   Null = r0 - rMAC;
   if NE jump wbm_error;           // Check if WBM payload length matches SDU payload length

   rMACB = rMAC + SDU_HEADER_SIZE;  // SDU payload length + SDU header length
   rMACB = rMACB + WBM_HEADER_SIZE; //                    + WBM header length
   rMACB = rMACB + r0;              //                    + WBM payload length
   Null = r5 - rMACB;
   if NE jump wbm_error;           // Check if complete blob length matches swbs_packet_length

   r0 = M[I0,M1];                  // Dummy read for RFU and PDU (unused)
   r0 = M[I0,M1];                  // Offset not used as there will only ever be one packet in SCO

   r0 = M[I0,M1];                  // Weak mask header

   Null = r6 - 2;
   if NE jump wbm_data;
   r1 = M[I0,M3];                  // For second half of double packet move I0
   r6 = 0;
wbm_data:

   I4 = I1;
   r0 = r0 AND 0xC0;
   r0 = r0 LSHIFT 8;               // Extract CRC state
   M[I1,M1] = r0;                  // Store partial weak header

// read in weak bit mask
   r10 = 15;                       // Copy 60 bytes of WBM
   r4 = 16;                        // Data is packed in 16 bit words
   rMACB = 0;                       // Counter for weak bits
   do input_copy_bit_err;          // copy from circular buffer to linear buffer
       r0 = M[I0,M1];
       r0 = r0 LSHIFT r4,
       r1 = M[I0,M1];
       r1 = r1 AND 0xFFFF;         // Mask upper 16-bits off in case there is stale data
       r0 = r0 OR r1;
       M[I1,M1] = r0;
       r0 = ONEBITCOUNT r0;        // Count the weak bits per word
       rMACB = rMACB + r0;           // Accumulate the weak bit count
   input_copy_bit_err:

   r0 = M[I4,M0];                  // Load the partial weak header
   r0 = r0 OR rMACB;                // Combine weak bit count into header
   M[I4,M0] = r0;                  // Store complete weak header
   jump input_processed;

wbm_error:                             // If there is an error in the WBM blob then just create
   r0 = 0xFFFF;                        // a WMB that will cause the PLC to be called by the Unpacker
   M[I1,M1] = r0;                      // Set the weak header to 0xFFFF
   r10 = 15;
   r0 = 0xFFFFFFFF;
   do dummy_wbm;                       // Write a 0xFFFFFFFF-filled WBM since there isn't a valid mask
   M[I1,M1] = r0;
   dummy_wbm:

   Null = r6 - 2;                     // If we have a header error in the second half packet we need to
   if EQ r6 = 0;                      // reset the flag so it doesn't keep looping

input_processed:
   r0 = I1 - I2;                      // calculate relative write pointer to base (excludes bit error data if not completed, if completed includes it)
   M[r2 + $AXBUF_WPTR_OFFSET] = r0;   // set write pointer

   r4 = 0;
   Null = r0 - SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES_WITH_BIT_ERROR;
   if LT jump not_enough_input_data;  // Full SWBS w/WBM packets are 60 bytes + 4 bytes + 60 bytes

   // Store circular buffer registers for later
   push L0;
   push B0;
   // Store blob size in case we have a double packet
   push rMAC;

   // Reset circular buffer registers, codec uses linear axbuf
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_DAT_OFFSET];
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;   // set relative read pointer to be zero
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;   // set relative write pointer to be zero

   r0 = M[r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.CODEC_DATA_FIELD];
   r1 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;
   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;

   r4 = r3;
   call $aptx_adaptive.voice_decode;    /// aptX adaptive decoder call
   r3 = r4;

   // Copy data from codec output axbuf into system output buffer

      // get the Output buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_write_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = write address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;


   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;
   I1 = M[r2 + $AXBUF_DAT_OFFSET];
   r0 = M[r2 + $AXBUF_WPTR_OFFSET];
   Addr2Words(r0);                      // Convert output byte count to sample count
   r10 = 240;                           // Max output data is 240 samples (Mode0 - 32kHz)
   Null = r0 - r10;
   if LT r10 = r0;

default_mode:
   r4 = 0;
   Null = r1 - r10;
   if LT jump no_space;
   r4 = r10;
   do output_copy;                      // copy and shift from linear buffer to circular buffer
   r0 = M[I1,M1];
   r0 = r0 ASHIFT 8;                    // aptX adaptive produces 24-bit aligned audio (in 32-bit word)
   M[I0,M1] = r0;
   output_copy:

   /* Update cbuffer write address, used to be done outside this function */
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];
   r1 = I0;
   call $cbuffer.set_write_address;

   // Update codec input buffer pointers
   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;    // set relative read pointer to be 0
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;      // set relative input write pointer to be zero

   // Restore blob size in case we have a second half of a packet to process
   pop rMAC;
   // Restore circular buffer registers in case we have a second half of a packet to process
   pop B0;
   pop L0;

   Null = r6;
   if Z jump processing_finished;
   // Blob was bigger than one frame
   // Set the buffers up to process the second frame
   r6 = 2;
   I0 = I5;                             // Back to start of SDU
   r0 = M[I0,M3];                       // Move I0 to second half of SDU
   I1 = I2;                             // Move I1 to start of axbuf
   jump process_data;

no_space:
not_enough_input_data:
processing_finished:
   // Reset circular buffer registers
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   Null = r5 - SWBS_DEFAULT_SDU_WBM_BLOB_SIZE;
   if NE r4 = r4 + r4;

   r0 = r4;                             // Return packet size

   pop M3;
   POP_ALL_C
   rts;

.ENDMODULE;


// *****************************************************************************
// FUNCTION:
//    $_sco_decoder_swbs_validate_wbm
//
// DESCRIPTION:
//    C callable version of the $sco_decoder.swbs.validate_wbm function.
//    Validates the packet. Must be called before decode.
//    For SWBS this only checks size of audio payload, input data and output space.
//    There is no sync word checking or data validation, apart from the SDU number
//
// INPUTS:
//    r0 - swbs_dec.CODEC_DATA_FIELD
//    r1 - payload length in bytes
//    r2 - pointer which will be populated with the swbs packet length.
//
// OUTPUTS:
//    r0 - Output in samples, 0 is no space, 1 is no data
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
//
// NOTES:
// *****************************************************************************

.MODULE $M.sco_decoder.swbs._validate_wbm;

   .DATASEGMENT DM;
   .CODESEGMENT PM;

$_sco_decoder_swbs_validate_wbm:

   PUSH_ALL_C

    // save OP DATA
   r7 = r0;

   // payload length
   r5 = r1;// this should be already in bytes.

   LIBS_SLOW_SW_ROM_PATCH_POINT($swbs_cap.WBS_C_STUBS_ASM.SCO_DECODER.SWBS._VALIDATE._SCO_DECODER_SWBS_VALIDATE_WBM.PATCH_ID_0,r6)

    // save the pointer to the swbs packet length
   push r2;

   // Calc amount of data in input buffer - return r0 = 1 if no data
   r6 = 1;
   // get extra op data
   r0 = r7;
   call $base_op.base_op_get_instance_data;
   r3 = r0;
   // get the Input buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];
   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.calc_amount_data_in_words;

   Null = r0 - SWBS_SMALLEST_VALID_PACKET_IN_WORDS; // Smallest input packet supported is 60 bytes (largest is 134 bytes)
   if LT jump no_input_data;  // No data

   // Check SDU header
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];
   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_read_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = read address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;

   /* SDU blob - note bytes in SCO buffer are reversed
   <-------- uint16 --------->
   +=========================+
   | Byte[1]   : Byte[0]     | RFU (8bit) + Chunk ID (8bit)
   +-------------------------+
   | Byte[3]   : Byte[2]     | Length (16bit)
   +-------------------------+
   | SDU[1]    : SDU[0]      |
   :                         : Audio frame
   | SDU[n-1]  : SDU[n-2]    |
   +-------------------------+
   */

   // Check the SDU RFU and Chunk ID
   r4 = M[I0,M1];
   r4 = r4 AND 0xFFFF;       // Mask off lower 16 bits
   Null = r4 - SDU_RFU_CHUNK_ID;
   if NE jump header_error;

   // Check the audio payload length
   r4 = M[I0,M1];
   r4 = r4 AND 0xFFFF;          // Mask off lower 16 bits
   r4 = r4 ASHIFT -8;
   r4 = r4 - SDU_HEADER_SIZE;   // Subtract header size
   Null = r4 - SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES;  // Check length against default audio payload size
   if EQ jump header_size_ok;
   Null = r4 - SWBS_EXTENDED_ENCODED_BLOCK_SIZE_IN_BYTES;  // Check length against extended audio payload size
   if NE jump header_error;

header_size_ok:
   r1 = r4 * 2 (int);           // SDU payload + WBM payload (assume payloads match)
   r1 = r1 + SDU_HEADER_SIZE;   //             + SDU header
   r1 = r1 + WBM_HEADER_SIZE;   //             + WBM header
   Null = r1 - r5;
   if NE jump header_error;     // Check computed blob size against received blob size

   // Calc amount of space in output buffer - return r0 = 0 if no space
   r6 = 0;
   // get extra op data
   r0 = r7;
   call $base_op.base_op_get_instance_data;
   r3 = r0;
   // get the Input buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];
   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.calc_amount_space_in_words;
   // OUTPUTS:
   // r0 = amount of space (for new data) in words or addresses
   // save the amount of space available in output buffer

   r4 = SWBS_MODE0_AUDIO_BLOCK_SIZE;
   r2 = SWBS_MODE4_AUDIO_BLOCK_SIZE;
   r1 = M[r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.CODECMODE_FIELD];
   Null = r1 - SWBS_CODEC_MODE4;
   if EQ r4 = r2;

   Null = r5 - SWBS_DEFAULT_SDU_WBM_BLOB_SIZE; // Check for double packets
   if GT r4 = r4 + r4;                         // Output amount is doubled

   Null = r0 - r4;               // Check if there is enough output space
   if LT jump no_output_space;   // No space
   r6 = r4;                      // Set output samples length

no_input_data:
no_output_space:
header_error:
   // Reset circular buffer registers
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   pop r2;
   // populate the computed swbs packet length (complete blob)
   M[r2] = r5;

   r0 = r6; // populate the return value (output samples)
   POP_ALL_C
   rts;


.ENDMODULE;

#endif

