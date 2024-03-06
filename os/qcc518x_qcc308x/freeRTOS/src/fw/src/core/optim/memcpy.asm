// Copyright (c) 2016 Qualcomm Technologies International, Ltd.
//   %%version
#include "kaldwarfregnums.h"
#include "dwarf_constants.h"

#ifdef INSTALL_FAST_MEMCPY

.MODULE $M._memcpy;
    .CODESEGMENT CD_MAXIM;

    .MAXIM;

/* We implement memcpy here to override the library version so it can use
 * maxim instructions (which should be faster than minim) and so that we
 * can save and restore the index registers to make it interrupt safe.
 *
 * Dest   = r0
 * Source = r1
 * Length = r2
 *
 * Return = r0 (dest)
 */
$_memcpy:
#if 1
    /* Copying enough to need optimisation? */
    Null = r2 - 16;
    if LE jump $_memcpy_simple;
#endif

    pushm <I3, I7>;
.memcpy_push:
    I3 = r1; /* Source */
    I7 = r0; /* Destination */

    /* If destination pointer is already aligned, no lead-in */
    r10 = r0 AND 0x03;
    if Z jump mc_core;

//
// Copy bytes until the destination pointer (I7/r0) is word-aligned
//
mc_intro:
    r10 = 4 - r10; /* Bytes to copy to get to alignment */
    r3 = r2 - r10; /* New length */
    if NEG r10 = r2; /* If r2 < r10, only copy r2 bytes */

    // Copy <r10> bytes to align I7
    r3 = Null + Null;
    do loop_intro;
    rMAC = MBU[I3 + r3];
    MB[I7 + r3] = rMAC;
    r3 = r3 + 1;
loop_intro:
    // Update src/dest/count - r3 is number of bytes actually copied
    I3 = I3 + r3;
    I7 = I7 + r3;
    r1 = r1 + r3;
    r2 = r2 - r3;

//
// Decide whether we're doing an aligned copy or unaligned copy,
// or indeed, if there's anything to word-copy at all!
//
mc_core:
    r10 = r2 LSHIFT -2; // Number of whole words still to copy
    if Z jump mc_outro; 

    r10 = r10 - 1;      // Pre-decrement for the loops later
    r2 = r2 AND 0x03;   // Number of bytes still to copy afterwards
    r3 = r1 AND 0x03;   // New byte offset of the source
    if NE jump mc_unaligned; // Source is not word-aligned

//
// Do an aligned copy of as many words as we can
//
mc_aligned:
    rMAC = M[I3,4];
    do loop_aligned;
    rMAC = M[I3,4], M[I7,4] = rMAC;
loop_aligned:
    M[I7,4] = rMAC;

//
// Transfer any remaining bytes one at a time
//
mc_outro:
    r10 = r2;
    r3 = Null + Null;
    do loop_outro;
    rMAC = MBS[I3 + r3];
    MB[I7 + r3] = rMAC;
    r3 = r3 + 1;
loop_outro:
    popm <I3, I7>;
    rts;

//
// Fast n offset to 0 offset copy.
//
// Note: We can never read from I7 as it may be write-only memory
//
// Input parameters:
// Source: r1 
// Dest  : r0 (ignored in favour of I7)
// Write : I7 (same as dest, word-aligned)
// Loop  : r10 = Number of words to copy - 1
// Offset : r3 = Byte offset of the source
// Remainder : r2 = Bytes to copy after the words are done
//
// Scratch regs used:
//  I3 = source word address
//  r0 = source word
//  r1 = dest word
//  r3 = tmp
//  r4 = left shift
//  r5 = right shift
//  r6 = byte offset of src
//
// r0 must be preserved as it's the return value (dest addr)
//
mc_unaligned:
    pushm <r0, r4, r5, r6>;
    // Work out the shifts
    r6 = r3;
    r4 = r3 LSHIFT 3;
    r4 = 32 - r4;
    r5 = r4 - 32;

    // Set up the source index register
    r3 = r1 AND ~0x03;
    I3 = r3;        // Source, word-aligned downward

    // Read the first source word, and prepare it
    // as the 'leftover'
    r3 = M[I3, 4];
    r3 = r3 LSHIFT r5; // r5 is -ve so RSHIFT

    // Preload the second source word
    r0 = M[I3, 4];

    do loop_unaligned;

    // Build the dest word
    r1 = r0 LSHIFT r4;
    r1 = r1 OR r3;

    // Store the source leftover
    r3 = r0 LSHIFT r5; // r5 is -ve, so RSHIFT

    // Write dest word and read next src word
    r0 = M[I3,4], M[I7,4] = r1;

loop_unaligned:
    // The loop terminates one word early to ensure that the source isn't
    // over-read.

    // Write the last destination word
    r1 = r0 LSHIFT r4;
    r1 = r1 OR r3;
    M[I7,4] = r1;

    // Set up for the trailing copy by correcting I3
    I3 = I3 - 4;
    I3 = I3 + r6;
    
    popm <r0, r4, r5, r6>;
    jump mc_outro;

.memcpy_end:

.ENDMODULE;

#endif /* INSTALL_FAST_MEMCPY */
