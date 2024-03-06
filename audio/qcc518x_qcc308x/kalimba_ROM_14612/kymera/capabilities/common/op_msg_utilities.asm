// *****************************************************************************
// Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
// *****************************************************************************

// *****************************************************************************
// NAME:
//    op_msg_utilities
//
// DESCRIPTION:
//    These are utility function to assist in handling parameters and the
//    associated operator messages
//
// *****************************************************************************

#include "stack.h"
#include "portability_macros.h"

#include "patch_library.h"

// *****************************************************************************
// Num blocks bit fields:
//
//        num_blocks
//
//   1b     3b                    12b
// +---+---------+------------------------------------+
// | D |    E    |                 N                  |
// +---+---------+------------------------------------+
//
//   - D: Restore defaults flag
//   - E: Encoding format
//   - N: Number of Parameter Data Blocks
//
// *****************************************************************************
#define CPS_NUM_BLOCKS_MASK       (0x0FFF)
#define CPS_PARAM_ENCODING_MASK   (0x7000)

// *****************************************************************************
// Size and content of the Parameter Value Package depend on parameter encoding
// Currently supported parameter encodings are:
//
//  Packed 24-bit format
// +-------------+-------------+-------------+
// |    MSW1     | LSB1 / MSB2 |    LSW2     |
// +-------------+-------------+-------------+
//
//  Packed 32-bit format
// +-------------+-------------+
// |     MSW     |     LSW     |
// +-------------+-------------+
//
// *****************************************************************************
#define CPS_PACKED_24B_PARAMS     (0x0000)
#define CPS_PACKED_32B_PARAMS     (0x1000)

.MODULE $M.OP_MSG_UTILITIES;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

// *****************************************************************************
// MODULE:
//    $_cps_mask16bit
//   void cps_mask16bit(unsigned *in_ptr,unsigned length);
//
// DESCRIPTION:
//    Zero the upper bits of an array of words
//
// INPUTS:
//    - r0 = pointer to array
//    - r1 = array length (in words)
//
// OUTPUTS:
//     none
//
// TRASHED REGISTERS:
//    Follows C Guidelines
//
// *****************************************************************************
$_cps_mask16bit:

   push r10;

   r10 = r1;

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($common_cap.OP_MSG_UTILITIES_ASM.OP_MSG_UTILITIES.CPS_MASK16BIT.PATCH_ID_0, r1)

   r1 = 0xFFFF;
   I3 = r0;
   do lp_cps_mask16bit_loop;
      r0 = M[I3,0];
      r0 = r0 AND r1;
      M[I3,MK1]=r0;
   lp_cps_mask16bit_loop:

   pop r10;
   rts;

// *****************************************************************************
// MODULE:
//    $_cpsSetDefaults
//    void cpsSetDefaults(unsigned *in_ptr, unsigned *out_ptr,unsigned length);
//
// DESCRIPTION:
//    Copy Packed Defaults to array
//
// INPUTS:
//    - r0 = Defaults Ptr
//    - r1 = Param Array
//    - r2 = Number of Parameters
//
// OUTPUTS:
//     none
//
// TRASHED REGISTERS:
//    Follows C Guidelines
//
// *****************************************************************************
.VAR dfltconst[5] = 0x00ff00,-8,0x00ffff,16,8;

$_cpsSetDefaults:

   LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($common_cap.OP_MSG_UTILITIES_ASM.OP_MSG_UTILITIES.CPSSETDEFAULTS.PATCH_ID_0)

/* default parameters are not packed on the 32-bit Kalimba */

   push r10;
   pushm <I0, I4>;

   I4  = r0;
   I0  = r1;
   r10 = r2;

   r0 = M[I4,MK1];   // preload
   do cp_loop;
      M[I0,MK1] = r0, r0 = M[I4,MK1];
   cp_loop:

   popm <I0, I4>;
   pop r10;
   rts;


// *****************************************************************************
// MODULE:
//	  $_cpsComputeGetParamsResponseSize
//    unsigned cpsComputeGetParamsResponseSize(unsigned *in_ptr,unsigned length, unsigned num_params);
//
// DESCRIPTION:
//    Copy Packed Defaults to array
//
// INPUTS:
//    - r0 = Pointer to Block Descriptions
//    - r1 = Number of Block Descriptions
//    - r2 = Number of Parameters
//
// OUTPUTS:
//     r0 - Size of response (zero if error)
//
// TRASHED REGISTERS:
//    Follows C Guidelines
//
// *****************************************************************************
$_cpsComputeGetParamsResponseSize:

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($common_cap.OP_MSG_UTILITIES_ASM.OP_MSG_UTILITIES.CPSCOMPUTEGETPARAMSRESPONSESIZE.PATCH_ID_0,r3)

   pushm <r4,r5,r10>;
   push  M1;

   r10 = r1 AND CPS_NUM_BLOCKS_MASK;    // r10 = num blocks
   I3  = r0;
   r3 = 0xFFFF;
   M1 = MK1;
   r5 = NULL;
   r0 = NULL;  // Assume Error

   do cpsComputeGetParamsResponseSize_loop;
      r4 = M[I3,M1];  // Param Offset
      r4 = r4 AND r3,   r1 = M[I3,M1];  // Num Params in Block
      r1 = r1 AND r3;
      // A block must have at least one parameter
      if Z jump jp_cpsComputeGetParamsResponseSize_error;
      NULL = r4 - r2;   // Check Parameters
      if POS jump jp_cpsComputeGetParamsResponseSize_error;
      r4 = r4 + r1;
      NULL = r2 - r4;   // Check Parameters
      if NEG jump jp_cpsComputeGetParamsResponseSize_error;
      // OK.  Increment Size
      // Size of Parameter Value Package depends on encoding:
      // (0) Packed 24-bit Parameters
      //    Block size equals 2 + blk_size + ceiling(blk_size/2)
      //    [msw 1] [lsb 1 | msb 2] [lsw 2]
      // (1) Packed 32-bit Parameters
      //    Block size equals 2 + 2*blk_size
      //    [ msw ] [ lsw ]
      r5 = r5 + 2;
      r5 = r5 + r1;
      r5 = r5 + r1;
   cpsComputeGetParamsResponseSize_loop:
   r0 = r5;    // Return Size
   // Add number of blocks into the response
   r0 = r0 + 1;

jp_cpsComputeGetParamsResponseSize_error:

   pop M1;
   popm <r4,r5,r10>;

   rts;

// *****************************************************************************
// MODULE:
//    $_cpsGetParameters
//    void $cpsGetParameters(unsigned *in_ptr,unsigned length, unsigned *params,unsigned *out_ptr);
//
// DESCRIPTION:
//    Copy Packed Defaults to array
//
// INPUTS:
//    - r0 = Pointer to Block Descriptions
//    - r1 = Number of Block Descriptions
//    - r2 = Pointer to Parameters
//    - r3 = Pointer to Response
//
// OUTPUTS:
//     none
//
// TRASHED REGISTERS:
//    Follows C Guidelines
//
// *****************************************************************************
$_cpsGetParameters:


   pushm <I0,M1>;
   pushm <r5,r7,r8,r9,r10>;
   I3=r0;               // Block Descriptions

   LIBS_SLOW_SW_ROM_PATCH_POINT($common_cap.OP_MSG_UTILITIES_ASM.OP_MSG_UTILITIES.CPSGETPARAMETERS.PATCH_ID_0,r8)

   I7=r3;               // Result
   M1 = MK1;
   r10 = r1 AND CPS_NUM_BLOCKS_MASK;  // Number of Blocks
   r8 = -1;                           // Invert 0xFFFFFF (arch4: 0xFFFFFFFF)
   r9 = r8 LSHIFT (16 - DAWTH);       // Mask16 0x00FFFF (arch4: 0x0000FFFF)

   // Add number of blocks (and parameter encoding) into the response
   M[I7,MK1] = r1;

// Packed 32-bit params: one 32-bit parameter into two 16-bit words
cpsGetParametersPacked32:
   r0=M[I3,M1];      // read first param_id
   do cpsGetParametersPacked32_loop1;
      // Read Descriptor Block and Mask upper bits
      r0 = r0 AND r9,   r3=M[I3,M1];
      r3 = r3 AND r9;

      // Setup Output Block
      r7 = r7 XOR r7,      M[I7,MK1] = r0;  // First Offset
      r7 = r7 + r3,        M[I7,MK1] = r3;  // Number of Parameters

      Words2Addr(r0);   // r0 = parameter_id in addr (arch4)
      I0 = r2 + r0;     // Location in Parameters Array
      cpsGetParametersPacked32_loop2:
         r0 = M[I0,M1];      // Parameter
         r1 = r0 LSHIFT -16;
         M[I7,M1] = r1;      // [ msw ]
         r1 = r0 AND r9;
         M[I7,M1] = r1;      // [ lsw ]

         r7 = r7 - 1;
      if GT jump cpsGetParametersPacked32_loop2;

      r0=M[I3,M1];      // read next param_id
   cpsGetParametersPacked32_loop1:

cpsGetParameters_done:
   popm <r5,r7,r8,r9,r10>;
   popm <I0,M1>;
   rts;

// *****************************************************************************
// MODULE:
//    $_cpsGetDefaults
//    void cpsGetDefaults(unsigned *in_ptr,unsigned length, unsigned *params,unsigned *out_ptr);
//
// DESCRIPTION:
//    Copy Packed Defaults to array
//
// INPUTS:
//    - r0 = Pointer to Block Descriptions
//    - r1 = Number of Block Descriptions
//    - r2 = Pointer to Defaults
//    - r3 = Pointer to Response
//
// OUTPUTS:
//     none
//
// TRASHED REGISTERS:
//    Follows C Guidelines
//
// *****************************************************************************
$_cpsGetDefaults:

   pushm <I0,M1>;
   pushm <r5,r6,r7,r8,r9,r10>;

   I3=r0;               // Block Descriptions

   LIBS_SLOW_SW_ROM_PATCH_POINT($common_cap.OP_MSG_UTILITIES_ASM.OP_MSG_UTILITIES.CPSGETDEFAULTS.PATCH_ID_0,r8)


   I7=r3;               // Result
   M1 = MK1;
   r10 = r1 AND CPS_NUM_BLOCKS_MASK;  // Number of Blocks
   r8 = -1;                           // Invert 0xFFFFFF (arch4: 0xFFFFFFFF)
   r9 = r8 LSHIFT (16-DAWTH);         // Mask16 0x00FFFF (arch4: 0x0000FFFF)

   // Add number of blocks (and parameter encoding) into the response
   M[I7,MK1] = r1;

// Packed 32-bit defaults: one 32-bit value into two 16-bit words
cpsGetDefaultsPacked32:
   r0=M[I3,M1];      // read first param_id
   do cpsGetDefaultsPacked32_loop1;
      // Read Descriptor Block and Mask upper bits
      r0 = r0 AND r9,   r3=M[I3,M1];
      r3 = r3 AND r9;

      // Setup Output Block
      r7 = r7 XOR r7,      M[I7,MK1] = r0;  // First Offset
      r7 = r7 + r3,        M[I7,MK1] = r3;  // Number of Parameters

      Words2Addr(r0);   // r0 = parameter_id in addr (arch4)
      I0 = r2 + r0;     // Location in Defaults Array
      cpsGetDefaultsPacked32_loop2:
         r0 = M[I0,M1];      // Default
         r1 = r0 LSHIFT -16;
         M[I7,M1] = r1;      // [ msw ]
         r1 = r0 AND r9;
         M[I7,M1] = r1;      // [ lsw ]

         r7 = r7 - 1;
      if GT jump cpsGetDefaultsPacked32_loop2;

      r0=M[I3,M1];      // read next param_id
   cpsGetDefaultsPacked32_loop1:

cpsGetDefaults_done:
   popm <r5,r6,r7,r8,r9,r10>;
   popm <I0,M1>;
   rts;


// *****************************************************************************
// MODULE:
//    $_cpsSetParameters
//    void cpsSetParameters(unsigned *in_ptr,unsigned length, unsigned *params,unsigned num_params);
//
// DESCRIPTION:
//    Copy Packed Defaults to array
//
// INPUTS:
//    - r0 = Pointer to Block Descriptions
//    - r1 = Number of blocks
//    - r2 = Pointer to Parameters
//    - r3 = Number of Parameters
//
// OUTPUTS:
//     r0 = result
//
// TRASHED REGISTERS:
//    Follows C Guidelines
//
// *****************************************************************************
$_cpsSetParameters:

   push M1;
   pushm <r4,r5,r7,r8,r9,r10>;

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($common_cap.OP_MSG_UTILITIES_ASM.OP_MSG_UTILITIES.CPSSETPARAMETERS.PATCH_ID_0,r7)

   Null = r1;
   if Z jump cpsSetParameters_done;

   M1 = MK1;
   I3 = r0;                          // Block Descriptions
   r7 = r1 AND CPS_NUM_BLOCKS_MASK;  // Number of Blocks

   r8 = -1;                          // Invert 0xFFFFFF (arch4: 0xFFFFFFFF)
   r9 = r8 LSHIFT (16 - DAWTH);      // Mask16 0x00FFFF (arch4: 0x0000FFFF)

// Packed 32-bit params: two 16-bit words into one 32-bit value
cpsSetParametersPacked32:
   cpsSetParametersPacked32_loop1:
      r0  = M[I3,MK1];                   // Offset
      r0  = r0 AND r9,   r1 = M[I3,MK1]; // Number of values
      Words2Addr(r0);                    // offset in addr (arch4)
      I7  = r2 + r0;                     // Params+offset
      Addr2Words(r0);                    // offset in words (arch4)
      r10 = r1 AND r9;

      // Validate Parammeters: offset + nr. of values in the block can not go
      // beyond the number of setable parameters for this capability (r3)
      r0 = r0 + r10;
      NULL = r0 - r3;
      if GT jump cpsSetParameters_error;

      do cpsSetParametersPacked32_loop2;
         r0 = M[I3,MK1];            // [msw]
         r0 = r0 LSHIFT 16;
         r1 = M[I3,MK1];            // [lsw]
         r1 = r1 AND r9;
         r0 = r0 OR r1;
         M[I7,MK1] = r0;
      cpsSetParametersPacked32_loop2:
      r7 = r7 - 1;
   if GT jump cpsSetParametersPacked32_loop1;

cpsSetParameters_done:
   r0 = NULL;
   popm <r4,r5,r7,r8,r9,r10>;
   pop M1;
   rts;

cpsSetParameters_error:
   r0 = 1;
   popm <r4,r5,r7,r8,r9,r10>;
   pop M1;
   rts;


.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $_cpsPack2Words
//
// DESCRIPTION:
//    Packs two "unsigned" words into 16-bit words
//
// INPUTS:
//    - r0 = input word1
//    - r1 = input word2
//    - r1 = pointer, address to pack word into
//
// OUTPUTS:
//    - r0: updated address
//
// TRASHED REGISTERS:
//    C calling convention respected.
//
// NOTES:
// *****************************************************************************
$_cpsPack2Words:
   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($common_cap.OP_MSG_UTILITIES_ASM.OP_MSG_UTILITIES.CPSPACK2WORDS.PATCH_ID_0, r3)

   r3 = r0 LSHIFT -16;
   M[r2 + 0*ADDR_PER_WORD] = r3;  // Output MSW1

   r0 = r0 AND 0xFFFF;
   M[r2 + 1*ADDR_PER_WORD] = r0;  // Output LSW1

   r3 = r1 LSHIFT -16;
   M[r2 + 2*ADDR_PER_WORD] = r3;  // Output MSW2

   r1 = r1 AND 0xFFFF;
   M[r2 + 3*ADDR_PER_WORD] = r1;  // Output LSW2

   // Packed 32-bit stats ratio is 4:2
   r0 = r2 + 4*ADDR_PER_WORD;
   rts;

// *****************************************************************************
// MODULE:
//    $_cpsPack1Word
//
// DESCRIPTION:
//    Packs one "unsigned" word into 16-bit words
//
// INPUTS:
//    - r0 = input word
//    - r1 = pointer, address to pack word into
//
// OUTPUTS:
//    - r0: updated address
//
// TRASHED REGISTERS:
//    C calling convention respected.
//
// NOTES:
// *****************************************************************************
$_cpsPack1Word:
   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($common_cap.OP_MSG_UTILITIES_ASM.OP_MSG_UTILITIES.CPSPACK1WORD.PATCH_ID_0, r2)

   r2 = r0 LSHIFT -16;
   M[r1 + 0*ADDR_PER_WORD] = r2;  // Output MSW

   r0 = r0 AND 0xFFFF;
   M[r1 + 1*ADDR_PER_WORD] = r0;  // Output LSW

   // Packed 32-bit stats ratio is 2:1
   r0 = r1 + 2*ADDR_PER_WORD;
   rts;


// *****************************************************************************
// MODULE:
//    $_cpsCheckParameterEncoding
//    unsigned cpsCheckParameterEncoding(unsigned num_blocks)
//
// DESCRIPTION:
//    Two parameter encodings (24-bit Packed & 32-bit Packed) are currently
//    supported. The bit-width of parameters in a message must match the data
//    word size of the target DSP. Any request that does not follow this rule
//    (for example, attempt to set 32-bit Packed params on a 24-bit DSP) should
//    get UNSUPPORTED_ENCODING code in response. This function exists to help
//    operators determine if a message request has the proper parameter encoding
//
// INPUTS:
//    - r0 = num_blocks (contains a bit field to identify parameter encoding)
//
// OUTPUTS:
//    - r0 = 0 if requested parameter encoding is correct for this architecture
//         = 1 .. NOT correct (UNSUPPORTED_ENCODING)
//
// TRASHED REGISTERS:
//    None
//
// *****************************************************************************
$_cpsCheckParameterEncoding:
   LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($common_cap.OP_MSG_UTILITIES_ASM.OP_MSG_UTILITIES.CPSCHECKPARAMETERENCODING.PATCH_ID_0)
   // check Parameter Encoding
   r0 = r0 AND CPS_PARAM_ENCODING_MASK;
   r0 = r0 - CPS_PACKED_32B_PARAMS;
   if NZ r0 = 1;                      // 32-bit DSP expects Packed 32-bit params
   rts;