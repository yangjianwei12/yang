// *****************************************************************************
// Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.
// *****************************************************************************




// *****************************************************************************
// NAME:
//    Memory operations
//
// DESCRIPTION:
//    This provides a set of functions that abstract certain memory operations on
//    the hydra platform.
//       $mem.ext_copy_to_ram
//       $mem.ext_unpack8_to_ram
//
// *****************************************************************************


// *****************************************************************************
// MODULE:
//    $mem.ext_copy_to_ram
//
//
// DESCRIPTION:
//    Will copy data "FORMAT_UNPACKED32" to RAM.
//
// INPUTS:
//    - r0 = address of data block to be copied
//    - r1 = size of data block in destination RAM
//    - r2 = destination address
//
// OUTPUTS:
//    - None
//
// TRASHED REGISTERS:
//    r0, r10
//
// *****************************************************************************
.MODULE $M.mem.ext;
    .CODESEGMENT EXT_DEFINED_PM;
 
    $mem.ext_copy_to_ram:
    $_mem_ext_copy_to_ram:
 
    r10 = r1 - 1;
    if NEG rts;
 
    pushm <I0, I4>;
 
    // Unpacking data for 32-bit Kalimba is just a plain copy.
    I0 = r2;
    I4 = r0;
    r0 = M[I4, ADDR_PER_WORD];
    do ext_copy_loop;
        M[I0, ADDR_PER_WORD] = r0,
         r0 = M[I4, ADDR_PER_WORD];
    ext_copy_loop:
    M[I0, ADDR_PER_WORD] = r0;
 
    popm <I0, I4>;
    rts;


// *****************************************************************************
// MODULE:
//    $mem.ext_unpack8_to_ram
//
//
// DESCRIPTION:
//    Unpack "FORMAT_PACKED8" to RAM.
//
// INPUTS:
//    - r0 = address of source data block
//    - r1 = size of data block in destination RAM
//    - r2 = destination address
//
// OUTPUTS:
//    - None
//
// TRASHED REGISTERS:
//    r0, r1, r2, r3, r10
//
// *****************************************************************************
    $mem.ext_unpack8_to_ram:
    $_mem_ext_unpack8_to_ram:
 
    r10 = r1 - 0;       // in octets
    if LE rts;
 
    pushm <I0, I4>;
 
    // Expanding signed octets to 32-bit words
    I0 = r2;
    I4 = r0;
    r3 = 0;
    r2 = 1;
    do unpk8_loop;
       r1 = MBS[I4 + r3];
       r3 = r3 + r2,
        M[I0, ADDR_PER_WORD] = r1;
    unpk8_loop:

    popm <I0, I4>;
    rts;

.ENDMODULE;
