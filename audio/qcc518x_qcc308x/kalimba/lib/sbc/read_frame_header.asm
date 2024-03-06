// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************
#ifndef SBC_WBS_ONLY
#ifndef SBCDEC_READ_FRAME_HEADER_INCLUDED
#define SBCDEC_READ_FRAME_HEADER_INCLUDED

#include "sbc.h"

// *****************************************************************************
// MODULE:
//    $sbcdec.read_frame_header
//
// DESCRIPTION:
//    Read Frame Header
//
// INPUTS:
//    - I0 = buffer to read words from
//
// OUTPUTS:
//    - I0 = buffer to read words from (updated)
//
// TRASHED REGISTERS:
//    r0-r4, r6, r8, I1, I2
//
// NOTES:
//  1) Find the syncword,
//  2) set: crc_checkword = 0x0f
//  3) Read in the frame header fields (doing CRC of appropriate bits)
//
//
//     @verbatim
//     frame_header()
//     (
//        $syncword                                            8 BsMsbf
//        $sampling_freq                                       2 UiMsbf
//        $blocks                                              2 UiMsbf
//        $channel_mode                                        2 UiMsbf
//        $allocation_method                                   1 UiMsbf
//        $subbands                                            1 UiMsbf
//        $bitpool                                             8 UiMsbf
//        $crc_check                                           8 UiMsbf
//        If (channel_mode==JOINT_STEREO)
//        (
//           for (sb=0;sb<nrof_subbands-1;sb++)
//           (
//              $join[sb]                                      1 UiMsbf
//           )
//           $RFA                                              1 UiMsbf
//        )
//     )
//     @endverbatim
//
// *****************************************************************************

.MODULE $M.sbcdec.read_frame_header;
   .CODESEGMENT SBCDEC_READ_FRAME_HEADER_PM;
   .DATASEGMENT DM;

   $sbcdec.read_frame_header:

   // push rLink onto stack
   push rLink;

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($sbcdec.READ_FRAME_HEADER_ASM.READ_FRAME_HEADER.PATCH_ID_0, r1)
#endif

   // default is no faults detected

   M[r9 + $sbc.mem.FRAME_CORRUPT_FIELD] = 0;



   // read in the sync word
   call $sbcdec.get8bits;

   // Check if the SYNC WORD is for the custom mode.The
   // header for the custom mode is bit different
   // from the standard SBC header.
   Null = r1 - $sbc.SYNC_WORD_CUSTOM_MODE;
   if NZ jump standard_header;

   // *****************************************************************************
   //
   //  Frame header for custom mode. No CRC field is present in the header
   //
   //     frame_header()
   //     (
   //        $syncword                                            8 BsMsbf
   //        $config_id                                           8 UiMsbf
   //     }
   //
   // *****************************************************************************

   // There is no CRC present for the custom mode
   // Hack for the CRC Check to pass
   r1 = 0;
   M[r9 + $sbc.mem.FRAMECRC_FIELD] = r1;
   M[r9 + $sbc.mem.CRC_CHECKSUM_FIELD] = r1;

   // read the config_id for the custom mode
   call $sbcdec.get8bits;

   //Get the current config_id
   r2 = M[r9 + $sbc.mem.WBS_SBC_FRAME_OK_FIELD] ;
   // populate the actual value of the config_id for custom mode
   M[r9 + $sbc.mem.WBS_SBC_FRAME_OK_FIELD] = r1;

   // Currently supported values for the custom mode are 1,2,3
   // and 16
   r0 = r1 - 1;
   if Z jump custom_mode_1;
   r0 = r1 - 2;
   if Z jump custom_mode_2;
   r0 = r1 - 3;
   if Z jump custom_mode_3;
   r0 = r1 - 4;
   if Z jump custom_mode_4;
   r0 = r1 - 5;
   if Z jump custom_mode_5;
   r0 = r1 - 6;
   if Z jump custom_mode_6;
   r0 = r1 - 16;
   if Z jump custom_mode_16;

   // Invalid configuration, set the frame as corrupt.
   jump corrupt_frame_error;

custom_mode_16:

   r0 = 3;// 48KHz
   M[r9 + $sbc.mem.SAMPLING_FREQ_FIELD] = r0;
   r0 = 12;
   M[r9 + $sbc.mem.NROF_BLOCKS_FIELD] = r0;
   r0 = 3;
   M[r9 + $sbc.mem.CHANNEL_MODE_FIELD] = r0;
   r0 = 2;
   M[r9 + $sbc.mem.NROF_CHANNELS_FIELD] = r0;
   r0 = 0;//Loudness
   M[r9 + $sbc.mem.ALLOCATION_METHOD_FIELD] = r0;
   r0 = 4;
   M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD] = r0;
   r0 = 23;
   M[r9 + $sbc.mem.BITPOOL_FIELD] = r0;
   r0 = 41;// Produces 41 bytes per frame
   M[r9 + $sbc.mem.CUR_FRAME_LENGTH_FIELD] = r0;

   //r8 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];
   r8 = 3; // #subbands - 1
   I1 = r9 + $sbc.mem.JOIN_FIELD;
   join_stereo_loop:
      call $sbcdec.get1bit;
      M[I1, MK1] = r1;
      r8 = r8 - 1;
   if NZ jump join_stereo_loop;
   // join[last subband] = 0
   r0 = 0;
   M[I1, 0] = r0;
   // read in dummy RFA bit
   call $sbcdec.get1bit;

   jump $pop_rLink_and_rts;

custom_mode_1:

   r0 = 0;// 16KHz
   M[r9 + $sbc.mem.SAMPLING_FREQ_FIELD] = r0;
   r0 = 10;
   M[r9 + $sbc.mem.NROF_BLOCKS_FIELD] = r0;
   r0 = 0;
   M[r9 + $sbc.mem.CHANNEL_MODE_FIELD] = r0;
   r0 = 1;
   M[r9 + $sbc.mem.NROF_CHANNELS_FIELD] = r0;
   r0 = 0;//Loudness
   M[r9 + $sbc.mem.ALLOCATION_METHOD_FIELD] = r0;
   r0 = 8;
   M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD] = r0;
   r0 = 27;
   M[r9 + $sbc.mem.BITPOOL_FIELD] = r0;
   r0 = 40;// Produces 40 bytes per frame
   M[r9 + $sbc.mem.CUR_FRAME_LENGTH_FIELD] = r0;

   jump $pop_rLink_and_rts;


custom_mode_2:
   r0 = 0; //16KHz
   M[r9 + $sbc.mem.SAMPLING_FREQ_FIELD] = r0;
   r0 = 12;
   M[r9 + $sbc.mem.NROF_BLOCKS_FIELD] = r0;
   r0 = 0;
   M[r9 + $sbc.mem.CHANNEL_MODE_FIELD] = r0;
   r0 = 1;
   M[r9 + $sbc.mem.NROF_CHANNELS_FIELD] = r0;
   r0 = 0;//Loudness
   M[r9 + $sbc.mem.ALLOCATION_METHOD_FIELD] = r0;
   r0 = 8;
   M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD] = r0;
   r0 = 29;
   M[r9 + $sbc.mem.BITPOOL_FIELD] = r0;
   r0 = 50;// Produces 50 bytes per frame
   M[r9 + $sbc.mem.CUR_FRAME_LENGTH_FIELD] = r0;
   jump $pop_rLink_and_rts;

custom_mode_3:
   r0 = 0;//16KHz
   M[r9 + $sbc.mem.SAMPLING_FREQ_FIELD] = r0;
   r0 = 13;
   M[r9 + $sbc.mem.NROF_BLOCKS_FIELD] = r0;
   r0 = 0;
   M[r9 + $sbc.mem.CHANNEL_MODE_FIELD] = r0;
   r0 = 1;
   M[r9 + $sbc.mem.NROF_CHANNELS_FIELD] = r0;
   r0 = 0;//Loudness
   M[r9 + $sbc.mem.ALLOCATION_METHOD_FIELD] = r0;
   r0 = 8;
   M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD] = r0;
   r0 = 27;
   M[r9 + $sbc.mem.BITPOOL_FIELD] = r0;
   r0 = 50;// Produces 50 bytes per frame
   M[r9 + $sbc.mem.CUR_FRAME_LENGTH_FIELD] = r0;
   jump $pop_rLink_and_rts;

custom_mode_4:
   r0 = 3;//48KHz
   M[r9 + $sbc.mem.SAMPLING_FREQ_FIELD] = r0;
   r0 = 12;
   M[r9 + $sbc.mem.NROF_BLOCKS_FIELD] = r0;
   r0 = 0;
   M[r9 + $sbc.mem.CHANNEL_MODE_FIELD] = r0;
   r0 = 1;
   M[r9 + $sbc.mem.NROF_CHANNELS_FIELD] = r0;
   r0 = 0;//Loudness
   M[r9 + $sbc.mem.ALLOCATION_METHOD_FIELD] = r0;
   r0 = 8;
   M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD] = r0;
   r0 = 20;
   M[r9 + $sbc.mem.BITPOOL_FIELD] = r0;
   r0 = 36;// Produces 36 bytes per frame
   M[r9 + $sbc.mem.CUR_FRAME_LENGTH_FIELD] = r0;
   jump $pop_rLink_and_rts;
custom_mode_5:
   r0 = 3;//48KHz
   M[r9 + $sbc.mem.SAMPLING_FREQ_FIELD] = r0;
   r0 = 13;
   M[r9 + $sbc.mem.NROF_BLOCKS_FIELD] = r0;
   r0 = 0;
   M[r9 + $sbc.mem.CHANNEL_MODE_FIELD] = r0;
   r0 = 1;
   M[r9 + $sbc.mem.NROF_CHANNELS_FIELD] = r0;
   r0 = 0;//Loudness
   M[r9 + $sbc.mem.ALLOCATION_METHOD_FIELD] = r0;
   r0 = 8;
   M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD] = r0;
   r0 = 18;
   M[r9 + $sbc.mem.BITPOOL_FIELD] = r0;
   r0 = 36;// Produces 36 bytes per frame
   M[r9 + $sbc.mem.CUR_FRAME_LENGTH_FIELD] = r0;
   jump $pop_rLink_and_rts;
custom_mode_6:
   r0 = 3;//48KHz
   M[r9 + $sbc.mem.SAMPLING_FREQ_FIELD] = r0;
   r0 = 15;
   M[r9 + $sbc.mem.NROF_BLOCKS_FIELD] = r0;
   r0 = 0;
   M[r9 + $sbc.mem.CHANNEL_MODE_FIELD] = r0;
   r0 = 1;
   M[r9 + $sbc.mem.NROF_CHANNELS_FIELD] = r0;
   r0 = 0;//Loudness
   M[r9 + $sbc.mem.ALLOCATION_METHOD_FIELD] = r0;
   r0 = 8;
   M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD] = r0;
   r0 = 19;
   M[r9 + $sbc.mem.BITPOOL_FIELD] = r0;
   r0 = 42;// Produces 42 bytes per frame
   M[r9 + $sbc.mem.CUR_FRAME_LENGTH_FIELD] = r0;
   jump $pop_rLink_and_rts;

standard_header:

   // now decode the rest of the header
   // from after the sync word.

   // crc_checksum = 0x0f
   r0 = 0x0f;

   M[r9 + $sbc.mem.CRC_CHECKSUM_FIELD] = r0;
   // read sampling freq
   call $sbcdec.get2bits;
   call $sbc.crc_calc;

   M[r9 + $sbc.mem.SAMPLING_FREQ_FIELD] = r1;


   // read nrof_blocks
   call $sbcdec.get2bits;
   call $sbc.crc_calc;
   r1 = r1 * 4 (int);
   r1 = r1 + 4;

   M[r9 + $sbc.mem.NROF_BLOCKS_FIELD] = r1;



   // read channel_mode
   call $sbcdec.get2bits;
   call $sbc.crc_calc;
   r0 = 1;

   M[r9 + $sbc.mem.CHANNEL_MODE_FIELD] = r1;
   if NZ r0 = r0 + r0;
   M[r9 + $sbc.mem.NROF_CHANNELS_FIELD] = r0;




   // read allocation_method
   call $sbcdec.get1bit;
   call $sbc.crc_calc;

   M[r9 + $sbc.mem.ALLOCATION_METHOD_FIELD] = r1;



   // read nrof_subbands
   call $sbcdec.get1bit;
   call $sbc.crc_calc;
   r1 = r1 * 4 (int);
   r1 = r1 + 4;
   // if the nrof_subbands has changed since the last frame
   // then we reset the synthesis 'v' buffers

   r0 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];
   Null = r1 - r0;
   if NZ call $sbcdec.silence_decoder;
   M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD] = r1;



   // read bitpool
   call $sbcdec.get8bits;
   call $sbc.crc_calc;

   M[r9 + $sbc.mem.BITPOOL_FIELD] = r1;



   // check that the bitpool is within the allowed bounds of the spec
   // maximum bitpool is 16*nrof_subbands for MONO and DUAL_CHANNEL
   //                and 32*nrof_subbands for STEREO abd JOINT_STEREO

   r2 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];
   r2 = r2 * 16 (int);
   r0 = M[r9 + $sbc.mem.CHANNEL_MODE_FIELD];

   Null = r0 - $sbc.STEREO;
   if POS r2 = r2 + r2;
   // also bitpool should not exceed 250
   r0 = r2 - 250;
   if GT r2 = r2 - r0;
   // check bitpool and treat frame as corrupt if it's out of bounds
   Null = r2 - r1;
   if NEG jump corrupt_frame_error;
   Null = r1 - 2;
   if NEG jump corrupt_frame_error;


   // read framecrc
   call $sbcdec.get8bits;

   M[r9 + $sbc.mem.FRAMECRC_FIELD] = r1;



   // read join array if joint_stereo

   r1 = M[r9 + $sbc.mem.CHANNEL_MODE_FIELD];

   Null = r1 - 3;
   if NZ jump not_jointstereo;

      r8 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];


      r8 = r8 - 1;

      I1 = r9 + $sbc.mem.JOIN_FIELD;


      join_loop:
         call $sbcdec.get1bit;
         call $sbc.crc_calc;
         M[I1, MK1] = r1;
         r8 = r8 - 1;
      if NZ jump join_loop;
      // join[last subband] = 0
      r0 = 0;
      M[I1, 0] = r0;

      // read in dummy RFA bit
      call $sbcdec.get1bit;
      call $sbc.crc_calc;
   not_jointstereo:


   // calculate the frame length
   // and check that we have enough data to do the frame decode
   call $sbc.calc_frame_length;

   M[r9 + $sbc.mem.CUR_FRAME_LENGTH_FIELD] = r0;


   // pop rLink from stack
   jump $pop_rLink_and_rts;



   corrupt_frame_error:
      r0 = 1;

      M[r9 + $sbc.mem.FRAME_CORRUPT_FIELD] = r0;

      // pop rLink from stack
      jump $pop_rLink_and_rts;

.ENDMODULE;

#endif
#endif
