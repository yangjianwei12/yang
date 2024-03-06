// *****************************************************************************
// Copyright (c) 2020 Qualcomm Technologies, Inc.                            *
// *****************************************************************************

// void bitreverse_array( int *in, int *out, int size);
// note: out must be a circular buffer, size must be power of 2
.MODULE $M.ml.bitreverse;
   .CODESEGMENT MATH_PM;
   .DATASEGMENT DM;

$_ml_bitreverse_array:
   push rLink;
   pushm <I0, I4, M1, M2>;
   I4 = r0;
   r0 = r1;
   push R0;
   pop B0;
   call $math.address_bitreverse;
   I0 = r1;
   r10 = r2;
   call $math.bitreverse_array;
   push NULL;
   pop B0;
   popm <I0, I4, M1, M2>;
   jump $pop_rLink_and_rts;

.ENDMODULE;



// sat fract ml_sqrt(sat fract);
// finds out the sqrt of the given number
// result is given in r1, so needs to be transferred to r0
.MODULE $M.ml.sqrt;
   .CODESEGMENT MATH_PM;
   .DATASEGMENT DM;

$_ml_sqrt:
    pushm <rMAC, r1, r2, rLink>;
    pushm <I3, M1>;
    call $math.sqrt48;
    popm <I3, M1>;
    popm < rMAC, r1, r2, rLink>;
    rts;
.ENDMODULE;

/**
 * @brief ml_twiddle_init Loads the twiddle values from ROM (slow) to RAM (faster)
 * @param num_points
 */
.MODULE $M.ml.twiddle.load;
   .CODESEGMENT MATH_PM;
   .DATASEGMENT DM;

$_ml_twiddle_load:
    M[$fft.NUM_POINTS] = r0;
    M[$fft.twiddle_real_address] = r1;
    M[$fft.twiddle_imag_address] = r2;
    rts;
.ENDMODULE;


// Calculate y = log2(x) in taylor series method
//    input ( 0 <= x < 256 ) as accum
//    result Q8.24, multiply with 128 as sat fract
.MODULE $M.ml.log2;
   .CODESEGMENT MATH_PM;
   .DATASEGMENT DM;

$_ml_log2:
   push rLink;
   push r6;
   call $math.log2_taylor;
   pop r6;
   jump $pop_rLink_and_rts;
.ENDMODULE;


.MODULE $M.ml.sin;
   .CODESEGMENT MATH_PM;
   .DATASEGMENT DM;

$_ml_sin:
    pushm <r2, r3, rLink>;
    push rmac;
    pushm <I0, M0>;
    call $math.sin;
    r0 = r1;
    popm <I0, M0>;
    pop rmac;
    popm <r2, r3, rLink>;
    rts;

.ENDMODULE;

.MODULE $M.scaled_fft;
    .CODESEGMENT PM;
    .DATASEGMENT DM;

$_scaled_fft:
   pushm <r4, r5, r6, r7, r8, r9, rLink>;
   pushm <I0, I1, I2, I4, I5, M0, M1, M2, M3>;
   I7 = r0;
   r8 = r1;
   call $math.scaleable_fft;
   popm <I0, I1, I2, I4, I5, M0, M1, M2, M3>;
   popm <r4, r5, r6, r7, r8, r9, rLink>;
   rts;
.ENDMODULE;

