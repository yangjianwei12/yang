// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

// C stubs for "math" library
// These obey the C compiler calling convention (see documentation)
// Comments show the syntax to call the routine also see matching header file

.MODULE $M.math_c_stubs;
   .CODESEGMENT MATH_PM;

// int sqrt( int x);
$_qsqrt:
   pushm <r4, r5, rLink>;
   pushm <I0, M0>;
   call $math.qsqrt;
   r0 = r1;          // TODO optimise: sqrt to return r0 instead of r1
   popm <I0, M0>;
   popm <r4, r5, rLink>;
   rts;

// void fft( fft_struc *x);
// note: result is bit reversed
$_fft:
   pushm <r4, r5, r6, r7, r8, r9, rLink>;
   pushm <I0, I1, I2, I4, I5, M0, M1, M2, M3>;
   I7 = r0;
   call $math.fft;
//   r8 = 0.60950;     // !!! 6-bit room for overflow before fft (norm/64)
//   r8 = 1.0;
//   call $math.scaleable_fft;      // !!! add another arg to prototype?
   popm <I0, I1, I2, I4, I5, M0, M1, M2, M3>;
   popm <r4, r5, r6, r7, r8, r9, rLink>;
   rts;

// void bitreverse_array( int *in, int *out, int size);
// note: out must be a circular buffer, size must be power of 2
$_bitreverse_array:
   push rLink;
   pushm <I0, I4, M1, M2>;
   I4 = r0;
   r0 = r1;
   call $math.address_bitreverse;
   I0 = r1;
   r10 = r2;
   call $math.bitreverse_array;
   popm <I0, I4, M1, M2>;
   pop rLink;
   rts;

// int log2( long long x);
//  input: positive Q1.63 ( 0 <= x < 1 )
//  for log2(0) this routine returns = log2(1/2^(NBITS*2)).
//  output: Q8.24
// note. possibility for range extention (up to 256?), the asm function takes 72-bit rMAC as input
$_log2:
   push rLink;
   push r6;
   rMAC0 = r0;          // LS
   rMAC12 = r1 (SE);    // MS
   call $math.log2_taylor;
   // note. faster log2_table available (precision?)
   pop r6;
   pop rLink;
   rts;

// int pow2( int x);
//  input Q8.24 negative number
//  result Q1.31
$_qpow2:
    pushm <FP(=SP), r6, r7, rLink>;
    call $math.pow2_taylor;
    popm <FP, r6, r7, rLink>;
    rts;

.ENDMODULE;
